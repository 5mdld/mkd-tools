//
// kiwakiwaaにより 2026/03/07 に作成されました。
//

#include "MKD/resource/named_resource_store_writer.hpp"
#include "named_resource_store_index_record.hpp"
#include "resource/detail/segmented_file_writer.hpp"
#include "resource/detail/zlib.hpp"
#include "io/build_directory.hpp"
#include "io/write_validation.hpp"
#include "binary/binary_buffer.hpp"

#include <algorithm>
#include <unordered_set>

namespace MKD
{
    struct NamedResourceStoreWriter::Impl
    {
        struct Record
        {
            std::string id;
            NamedResourceStoreIndexRecord location;
        };

        std::unique_ptr<detail::BuildDirectory> build;
        detail::SegmentedFileWriter files;
        NamedResourceWriteOptions options;
        std::vector<Record> records;
        std::unordered_set<std::string> ids;
        size_t count = 0;

        Impl(std::unique_ptr<detail::BuildDirectory> build, detail::SegmentedFileWriter files,
             const NamedResourceWriteOptions options)
            : build(std::move(build)), files(std::move(files)), options(options)
        {
        }

        Result<void> writeIndex();
    };

    NamedResourceStoreWriter::NamedResourceStoreWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    NamedResourceStoreWriter::~NamedResourceStoreWriter() = default;
    NamedResourceStoreWriter::NamedResourceStoreWriter(NamedResourceStoreWriter&&) noexcept = default;
    NamedResourceStoreWriter& NamedResourceStoreWriter::operator=(NamedResourceStoreWriter&&) noexcept = default;

    Result<NamedResourceStoreWriter> NamedResourceStoreWriter::create(const std::filesystem::path& directory,
                                                                      const NamedResourceStoreWriteOptions& options)
    {
        if (!options.fileBytes || options.resources.compressionLevel < -1 || options.resources.compressionLevel > 9 ||
            (options.resources.storage != NamedResourceStorage::Raw && options.resources.storage !=
                NamedResourceStorage::Zlib))
            return std::unexpected("Invalid named resource compression or file size");

        auto build = detail::BuildDirectory::create(directory);
        if (!build)
            return std::unexpected(build.error());

        auto files = detail::SegmentedFileWriter::create((*build)->path(), ResourceType::Graphics,
                                                         options.fileBytes, UINT16_MAX);
        if (!files)
            return std::unexpected(files.error());

        return NamedResourceStoreWriter(std::make_unique<Impl>(std::move(*build), std::move(*files),
                                                               options.resources));
    }

    Result<void> NamedResourceStoreWriter::add(std::string_view id, std::span<const uint8_t> bytes,
                                               std::optional<NamedResourceWriteOptions> options) const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto valid = detail::WriteValidation::text(id); !valid)
            return impl_->build->fail(valid.error());

        if (!impl_->ids.emplace(id).second)
            return impl_->build->fail("Duplicate named resource ID");

        const auto resource = options.value_or(impl_->options);
        const auto level = resource.compressionLevel;
        const bool compressedStorage = resource.storage == NamedResourceStorage::Zlib;
        if (level < -1 || level > 9 || bytes.size() > UINT32_MAX ||
            (resource.storage != NamedResourceStorage::Raw && !compressedStorage))
            return impl_->build->fail("Invalid named resource size or compression level");

        std::vector<uint8_t> compressed;
        if (compressedStorage)
        {
            auto result = detail::zlib::compress(bytes, level);
            if (!result)
                return impl_->build->fail(result.error());

            compressed = std::move(*result);
            bytes = compressed;
        }

        auto location = impl_->files.write({bytes});
        if (!location)
            return impl_->build->fail(location.error());

        impl_->records.push_back({
            std::string(id), {static_cast<uint16_t>(compressedStorage ? 1 : 0),
            static_cast<uint16_t>(location->sequence), 0, location->fileOffset, location->length}
        });
        ++impl_->count;
        return {};
    }

    Result<void> NamedResourceStoreWriter::Impl::writeIndex()
    {
        std::ranges::sort(records, {}, &Record::id);

        const uint64_t stringsStart = 8 + uint64_t{16} * records.size();
        uint64_t total = stringsStart;
        for (const auto& record : records)
            total += record.id.size() + 1;

        if (total > UINT32_MAX)
            return std::unexpected("Named resource index exceeds format limits");

        detail::BinaryBuffer index(stringsStart);
        index.writeLittle(uint32_t{0});
        index.writeLittle(static_cast<uint32_t>(records.size()));

        auto stringOffset = static_cast<uint32_t>(stringsStart);
        for (const auto& record : records)
        {
            index.writeLittle(record.location.format);
            index.writeLittle(record.location.fileSequence);
            index.writeLittle(stringOffset);
            index.writeLittle(record.location.fileOffset);
            index.writeLittle(record.location.length);
            stringOffset += static_cast<uint32_t>(record.id.size() + 1);
        }

        detail::OutputFile file(build->path() / "index.nidx", false);
        if (!file.isOpen())
            return std::unexpected("Cannot create named resource index");

        if (auto written = file.write(index.data()); !written)
            return written;

        for (const auto& record : records)
        {
            auto bytes = std::span(reinterpret_cast<const uint8_t*>(record.id.c_str()), record.id.size() + 1);
            if (auto written = file.write(bytes); !written)
                return written;
        }
        return file.finish().transform([](uint64_t)
        {
        });
    }

    Result<void> NamedResourceStoreWriter::finish() const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto result = impl_->files.finish(); !result)
            return impl_->build->fail(result.error());

        if (auto result = impl_->writeIndex(); !result)
            return impl_->build->fail(result.error());

        auto published = impl_->build->publish();
        if (published)
        {
            impl_->records = decltype(impl_->records){};
            impl_->ids = decltype(impl_->ids){};
        }

        return published;
    }

    size_t NamedResourceStoreWriter::size() const noexcept { return impl_->count; }
    bool NamedResourceStoreWriter::empty() const noexcept { return size() == 0; }
}
