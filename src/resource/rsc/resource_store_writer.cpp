//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#include "resource_store_writer.hpp"
#include "binary/binary_buffer.hpp"
#include "resource/detail/zlib.hpp"

#include <algorithm>
#include <limits>

namespace MKD
{
    Result<ResourceStoreWriter> ResourceStoreWriter::createIndexed(const std::filesystem::path& directoryPath,
                                                                   const Options& options)
    {
        return createImpl(Mode::Indexed, directoryPath, options);
    }


    Result<ResourceStoreWriter> ResourceStoreWriter::createSequential(const std::filesystem::path& directoryPath,
                                                                      const Options& options)
    {
        return createImpl(Mode::Sequential, directoryPath, options);
    }


    ResourceStoreWriter::ResourceStoreWriter(const Mode mode, const Options& options,
                                             detail::SequentialBlobWriter&& blobWriter)
        : mode_(mode), options_(options), blobWriter_(std::move(blobWriter))
    {
        if (options_.useEncryption && !options_.dictId.empty())
            encryptionKey_ = ResourceStoreCrypto::deriveKey(options_.dictId);
    }


    Result<ResourceStoreWriter> ResourceStoreWriter::createImpl(const Mode mode,
                                                                const std::filesystem::path& directoryPath,
                                                                const Options& options)
    {
        if (options.chunkSize == 0 || options.compressionLevel < -1 || options.compressionLevel > 9)
            return std::unexpected("Invalid resource writer chunk size or compression level");

        if (options.useEncryption && options.dictId.empty())
            return std::unexpected("Dictionary ID required for encrypted resources");

        if (options.type != ResourceType::Contents && options.type != ResourceType::Fonts &&
            options.type != ResourceType::AudioLegacy)
            return std::unexpected("Resource type does not use RSC storage");

        auto blobWriter = detail::SequentialBlobWriter::create(
            directoryPath, options.type, options.maxFileSize);

        if (!blobWriter)
            return std::unexpected(blobWriter.error());

        return ResourceStoreWriter(mode, options, std::move(*blobWriter));
    }


    Result<void> ResourceStoreWriter::add(const uint32_t itemId, std::vector<uint8_t> data)
    {
        if (finished_)
            return std::unexpected("Cannot add items after finalize()");
        if (mode_ != Mode::Indexed)
            return std::unexpected("add(itemId, data) is only valid for indexed writers, use addData() instead");

        if (data.size() > std::numeric_limits<uint32_t>::max() - 8)
            return std::unexpected("Resource item exceeds the format size limit");
        pendingItems_.push_back({.itemId = itemId, .data = std::move(data)});

        if (pendingItems_.size() >= options_.chunkSize)
            return flushChunk();

        return {};
    }


    Result<void> ResourceStoreWriter::addData(const std::span<const uint8_t> data)
    {
        if (finished_)
            return std::unexpected("Cannot add items after finalize()");
        if (mode_ != Mode::Sequential)
            return std::unexpected("addData() is only valid for sequential writers, use add() instead");

        if (data.size() > std::numeric_limits<uint32_t>::max() - 4 ||
            blobWriter_.currentOffset() > std::numeric_limits<uint32_t>::max() - 4 - data.size())
            return std::unexpected("Resource payload exceeds the format size limit");

        detail::BinaryBuffer buffer(data.size() + 4);
        buffer.writeLittle(static_cast<uint32_t>(data.size()));
        buffer.writeBytes(data);
        auto location = blobWriter_.write(buffer.data());
        if (!location)
            return std::unexpected(location.error());

        mapRecords_.push_back({
            .chunkGlobalOffset = static_cast<uint32_t>(location->globalOffset),
            .itemOffset = 0xFFFFFFFF
        });

        return {};
    }


    Result<void> ResourceStoreWriter::finalize()
    {
        if (finished_)
            return std::unexpected("finalize() already called");

        if (!pendingItems_.empty())
            if (auto result = flushChunk(); !result)
                return std::unexpected(result.error());

        if (auto blobResult = blobWriter_.finalize(); !blobResult)
            return std::unexpected(blobResult.error());

        if (auto indexResult = writeIndexFiles(); !indexResult)
            return std::unexpected(indexResult.error());

        finished_ = true;
        return {};
    }


    size_t ResourceStoreWriter::size() const noexcept
    {
        return !idxRecords_.empty()
                   ? idxRecords_.size() + pendingItems_.size()
                   : mapRecords_.size() + pendingItems_.size();
    }


    bool ResourceStoreWriter::empty() const noexcept
    {
        return idxRecords_.empty() && mapRecords_.empty() && pendingItems_.empty();
    }


    ResourceStoreWriter::SerializedChunk ResourceStoreWriter::serializeItems(std::span<const PendingItem> items) const
    {
        const bool newFormat = encryptionKey_.has_value();

        // calculate total size and record item offsets
        size_t totalSize = 0;
        std::vector<uint32_t> offsets;
        offsets.reserve(items.size());

        for (const auto& [itemId, data] : items)
        {
            offsets.push_back(static_cast<uint32_t>(totalSize));
            totalSize += (newFormat || data.empty() ? 8 : 4) + data.size();
        }

        // write item headers and data
        detail::BinaryBuffer buffer(totalSize);
        for (const auto& [itemId, data] : items)
        {
            if (newFormat || data.empty())
                buffer.writePadding(sizeof(uint32_t));

            buffer.writeLittle(static_cast<uint32_t>(data.size()));
            buffer.writeBytes(data);
        }

        return {std::move(buffer.take()), std::move(offsets)};
    }


    Result<void> ResourceStoreWriter::flushChunk()
    {
        uint64_t size = 0;
        for (const auto& item : pendingItems_)
            size += 8 + item.data.size();

        if (size > std::numeric_limits<uint32_t>::max() - 32)
            return std::unexpected("Resource chunk exceeds the format size limit");

        const auto [serialized, itemOffsets] = serializeItems(pendingItems_);

        auto compressedSpan = detail::zlib::compress(serialized, options_.compressionLevel);
        if (!compressedSpan)
            return std::unexpected(compressedSpan.error());

        if (compressedSpan->size() > std::numeric_limits<uint32_t>::max() - 32)
            return std::unexpected("Compressed chunk exceeds the format size limit");

        detail::BinaryBuffer buffer;
        if (encryptionKey_)
        {
            auto encrypted = ResourceStoreCrypto::encrypt(*compressedSpan, *encryptionKey_);
            buffer.writeLittle(uint32_t{0}); // new-format marker
            buffer.writeLittle(static_cast<uint32_t>(encrypted.size()));
            buffer.writeBytes(encrypted);
        }
        else
        {
            buffer.writeLittle(static_cast<uint32_t>(compressedSpan->size()));
            buffer.writeBytes(*compressedSpan);
        }

        if (buffer.position() > std::numeric_limits<uint32_t>::max() ||
            blobWriter_.currentOffset() > std::numeric_limits<uint32_t>::max() - buffer.position())
            return std::unexpected("Resource blob offsets exceed the format size limit");

        auto location = blobWriter_.write(buffer.data());
        if (!location)
            return std::unexpected(location.error());

        // record index entries
        const auto chunkOffset = static_cast<uint32_t>(location->globalOffset);
        for (size_t i = 0; i < pendingItems_.size(); ++i)
        {
            const auto mapIndex = static_cast<uint32_t>(mapRecords_.size());
            mapRecords_.push_back({
                .chunkGlobalOffset = chunkOffset,
                .itemOffset = itemOffsets[i],
            });

            idxRecords_.push_back({
                .itemId = pendingItems_[i].itemId,
                .mapIdx = mapIndex,
            });
        }

        pendingItems_.clear();
        return {};
    }


    Result<void> ResourceStoreWriter::writeIndexFiles() const
    {
        if (mapRecords_.size() > std::numeric_limits<uint32_t>::max())
            return std::unexpected("Too many resource records");

        auto sortedIdx = idxRecords_;
        std::ranges::sort(sortedIdx, {}, &ResourceStoreIndexRecord::itemId);
        for (size_t i = 1; i < sortedIdx.size(); ++i)
            if (sortedIdx[i - 1].itemId == sortedIdx[i].itemId)
                return std::unexpected("Duplicate resource item ID");

        const auto directory = blobWriter_.directory();

        // The map preserves write order, while the ID index is sorted independently.
        // Write contents.map
        {
            const auto mapPath = directory / "contents.map";
            std::ofstream file(mapPath, std::ios::binary | std::ios::trunc);
            if (!file)
                return std::unexpected(std::format("Failed to open: {}", mapPath.string()));

            ResourceStoreMapHeader header{
                .version = encryptionKey_ ? 0x01u : 0x00u,
                .recordCount = static_cast<uint32_t>(mapRecords_.size()),
            };

            detail::BinaryBuffer buffer;
            buffer.writeLittle(header.version);
            buffer.writeLittle(header.recordCount);
            for (const auto& record : mapRecords_)
            {
                buffer.writeLittle(record.chunkGlobalOffset);
                buffer.writeLittle(record.itemOffset);
            }
            file.write(reinterpret_cast<const char*>(buffer.data().data()),
                       static_cast<std::streamsize>(buffer.position()));

            if (!file)
                return std::unexpected(std::format("Failed to write: {}", mapPath.string()));
        }

        // write contents.idx
        if (mode_ == Mode::Indexed)
        {
            const auto idxPath = directory / "contents.idx";
            std::ofstream file(idxPath, std::ios::binary | std::ios::trunc);
            if (!file)
                return std::unexpected(std::format("Failed to open: {}", idxPath.string()));

            ResourceStoreIndexHeader header{
                .length = static_cast<uint32_t>(sortedIdx.size()),
                .padding = 0
            };

            detail::BinaryBuffer buffer;
            buffer.writeLittle(header.length);
            buffer.writeLittle(header.padding);
            for (const auto& record : sortedIdx)
            {
                buffer.writeLittle(record.itemId);
                buffer.writeLittle(record.mapIdx);
            }
            file.write(reinterpret_cast<const char*>(buffer.data().data()),
                       static_cast<std::streamsize>(buffer.position()));

            if (!file)
                return std::unexpected(std::format("Failed to write: {}", idxPath.string()));
        }

        return {};
    }
}
