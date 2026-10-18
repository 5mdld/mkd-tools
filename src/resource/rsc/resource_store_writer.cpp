//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#include "MKD/resource/resource_store_writer.hpp"
#include "resource_store_crypto.hpp"
#include "resource_store_index_record.hpp"
#include "resource_store_map.hpp"
#include "resource/detail/segmented_file_writer.hpp"
#include "resource/detail/zlib.hpp"
#include "io/build_directory.hpp"
#include "io/write_validation.hpp"
#include "binary/binary_buffer.hpp"

#include <algorithm>
#include <optional>
#include <unordered_set>

namespace MKD
{
    struct ResourceStoreWriter::Impl
    {
        std::unique_ptr<detail::BuildDirectory> build;
        detail::SegmentedFileWriter files;
        ResourceStoreWriteOptions options;
        std::optional<std::array<uint8_t, 32>> key;
        detail::BinaryBuffer pending;
        std::vector<ResourceStoreIndexRecord> index;
        std::vector<ResourceStoreMapRecord> map;
        std::vector<uint32_t> offsets;
        std::unordered_set<uint32_t> ids;
        size_t count = 0;

        Impl(std::unique_ptr<detail::BuildDirectory> build, detail::SegmentedFileWriter files,
             std::string_view contentIdentifier, const ResourceStoreWriteOptions& options)
            : build(std::move(build)), files(std::move(files)), options(options), pending(options.chunkBytes)
        {
            if (options.encrypt)
                key = ResourceStoreCrypto::deriveKey(contentIdentifier);
        }

        Result<void> flush();
        Result<void> writeIndices();
    };

    ResourceStoreWriter::ResourceStoreWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    ResourceStoreWriter::~ResourceStoreWriter() = default;
    ResourceStoreWriter::ResourceStoreWriter(ResourceStoreWriter&&) noexcept = default;
    ResourceStoreWriter& ResourceStoreWriter::operator=(ResourceStoreWriter&&) noexcept = default;

    Result<ResourceStoreWriter> ResourceStoreWriter::create(const std::filesystem::path& directory,
                                                            std::string_view contentIdentifier,
                                                            const ResourceStoreWriteOptions& options)
    {
        if (!options.chunkBytes || !options.fileBytes || options.compressionLevel < -1 || options.compressionLevel > 9)
            return std::unexpected("Invalid resource compression or size options");

        if (options.encrypt || !contentIdentifier.empty())
            if (auto valid = detail::WriteValidation::text(contentIdentifier); !valid)
                return std::unexpected(valid.error());

        auto build = detail::BuildDirectory::create(directory);
        if (!build)
            return std::unexpected(build.error());

        auto files = detail::SegmentedFileWriter::create((*build)->path(), ResourceType::Contents, options.fileBytes,
                                                         detail::SegmentedFileWriter::lastRscSequence);
        if (!files)
            return std::unexpected(files.error());

        return ResourceStoreWriter(
            std::make_unique<Impl>(std::move(*build), std::move(*files), contentIdentifier, options));
    }

    Result<void> ResourceStoreWriter::add(const uint32_t id, std::span<const uint8_t> bytes) const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (bytes.size() > UINT32_MAX - 40 || impl_->index.size() == UINT32_MAX)
            return impl_->build->fail("Resource item exceeds format limits");

        if (!impl_->ids.insert(id).second)
            return impl_->build->fail("Duplicate resource ID");

        const uint64_t itemBytes = bytes.size() + ((impl_->key || bytes.empty()) ? 8 : 4);
        if (!impl_->offsets.empty() && itemBytes + impl_->pending.position() > impl_->options.chunkBytes)
            if (auto flushed = impl_->flush(); !flushed)
                return impl_->build->fail(flushed.error());

        impl_->index.push_back({id, static_cast<uint32_t>(impl_->index.size())});
        impl_->offsets.push_back(static_cast<uint32_t>(impl_->pending.position()));
        if (impl_->key || bytes.empty())
            impl_->pending.writeLittle(uint32_t{0});

        impl_->pending.writeLittle(static_cast<uint32_t>(bytes.size()));
        impl_->pending.writeBytes(bytes);
        ++impl_->count;
        return {};
    }

    Result<void> ResourceStoreWriter::Impl::flush()
    {
        if (offsets.empty())
            return {};

        auto compressed = detail::zlib::compress(pending.data(), options.compressionLevel);
        if (!compressed)
            return std::unexpected(compressed.error());

        if (key)
            *compressed = ResourceStoreCrypto::encrypt(*compressed, *key);

        if (compressed->size() > UINT32_MAX - 8)
            return std::unexpected("Compressed chunk exceeds format limits");

        detail::BinaryBuffer header(8);
        if (key)
            header.writeLittle(uint32_t{0});

        header.writeLittle(static_cast<uint32_t>(compressed->size()));
        if (files.offset() > UINT32_MAX - header.position() - compressed->size())
            return std::unexpected("Resource offsets exceed format limits");

        auto location = files.write({header.data(), *compressed});
        if (!location)
            return std::unexpected(location.error());

        for (const auto offset : offsets)
            map.push_back({static_cast<uint32_t>(location->globalOffset), offset});

        offsets.clear();
        pending.clear();
        return {};
    }

    Result<void> ResourceStoreWriter::Impl::writeIndices()
    {
        detail::BinaryBuffer buffer;
        buffer.writeLittle(uint32_t{key ? 1u : 0u});
        buffer.writeLittle(static_cast<uint32_t>(map.size()));

        for (const auto record : map)
        {
            buffer.writeLittle(record.chunkGlobalOffset);
            buffer.writeLittle(record.itemOffset);
        }

        if (auto written = detail::OutputFile::writeAll(build->path() / "contents.map", buffer.data()); !written)
            return written;

        std::ranges::sort(index, {}, &ResourceStoreIndexRecord::itemId);
        buffer.clear();
        buffer.writeLittle(static_cast<uint32_t>(index.size()));
        buffer.writeLittle(uint32_t{0});

        for (const auto record : index)
        {
            buffer.writeLittle(record.itemId);
            buffer.writeLittle(record.mapIdx);
        }

        return detail::OutputFile::writeAll(build->path() / "contents.idx", buffer.data());
    }

    Result<void> ResourceStoreWriter::finish() const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto result = impl_->flush(); !result)
            return impl_->build->fail(result.error());

        if (auto result = impl_->files.finish(); !result)
            return impl_->build->fail(result.error());

        if (auto result = impl_->writeIndices(); !result)
            return impl_->build->fail(result.error());

        auto published = impl_->build->publish();
        if (published)
        {
            impl_->pending = detail::BinaryBuffer(0);
            impl_->index = decltype(impl_->index){};
            impl_->map = decltype(impl_->map){};
            impl_->offsets = decltype(impl_->offsets){};
            impl_->ids = decltype(impl_->ids){};
        }

        return published;
    }

    size_t ResourceStoreWriter::size() const noexcept { return impl_->count; }
    bool ResourceStoreWriter::empty() const noexcept { return size() == 0; }
}
