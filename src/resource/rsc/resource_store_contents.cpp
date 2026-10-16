//
// kiwakiwaaにより 2026/02/28 に作成されました。
//

#include "resource_store_contents.hpp"
#include "binary/binary_view.hpp"
#include "resource/detail/zlib.hpp"

namespace MKD
{
    ResourceStoreContents::ResourceStoreContents(detail::ResourceFiles files,
                                                 std::optional<std::array<uint8_t, 32>> key)
        : files_(std::move(files)), decryptionKey_(key), cache_(std::make_unique<ChunkCache>())
    {
    }

    Result<ResourceStoreContents> ResourceStoreContents::open(const std::filesystem::path& directory,
                                                              std::string_view dictId, const uint32_t mapVersion)
    {
        auto files = detail::ResourceFiles::open(directory, ".rsc");
        if (!files)
            return std::unexpected(files.error());

        std::optional<std::array<uint8_t, 32>> key;
        if (mapVersion == 1)
        {
            if (dictId.empty())
                return std::unexpected("Dictionary ID required for encrypted resources");
            key = ResourceStoreCrypto::deriveKey(dictId);
        }
        return ResourceStoreContents(std::move(*files), key);
    }

    Result<RetainedSpan> ResourceStoreContents::get(const ResourceStoreMapRecord& record) const
    {
        if (record.itemOffset == 0xFFFFFFFF)
            return readDirectData(record.chunkGlobalOffset);

        auto chunk = getOrLoadChunk(record.chunkGlobalOffset);
        if (!chunk)
            return std::unexpected(chunk.error());

        return parseItemFromChunk(std::move(*chunk), record.itemOffset);
    }

    Result<RetainedSpan> ResourceStoreContents::readDirectData(const size_t offset) const
    {
        auto header = files_.readGlobal(offset, 4);
        if (!header)
            return std::unexpected(header.error());

        const auto length = *detail::BinaryView(header->span()).readLittle<uint32_t>(0);
        return files_.readGlobal(offset + 4, length);
    }

    Result<RetainedSpan> ResourceStoreContents::getOrLoadChunk(size_t offset) const
    {
        {
            std::lock_guard lock(cache_->mutex);
            if (const auto it = cache_->entries.find(offset); it != cache_->entries.end())
                return it->second;
        }
        auto decoded = decodeChunkAt(offset);
        if (!decoded)
            return std::unexpected(decoded.error());

        std::lock_guard lock(cache_->mutex);
        if (cache_->entries.size() >= MAX_CACHED_CHUNKS)
            cache_->entries.clear();

        return cache_->entries.emplace(offset, std::move(*decoded)).first->second;
    }

    Result<RetainedSpan> ResourceStoreContents::decodeChunkAt(size_t offset) const
    {
        auto header = files_.readGlobal(offset, 4);
        if (!header)
            return std::unexpected(header.error());

        const auto marker = *detail::BinaryView(header->span()).readLittle<uint32_t>(0);
        size_t length = marker;
        size_t payloadOffset = offset + 4;
        if (marker == 0)
        {
            if (!decryptionKey_)
                return std::unexpected("Missing decryption key");

            auto lengthBytes = files_.readGlobal(payloadOffset, 4);
            if (!lengthBytes)
                return std::unexpected(lengthBytes.error());

            length = *detail::BinaryView(lengthBytes->span()).readLittle<uint32_t>(0);
            payloadOffset += 4;
        }

        auto payload = files_.readGlobal(payloadOffset, length);
        if (!payload)
            return std::unexpected(payload.error());

        if (marker == 0)
        {
            auto decrypted = ResourceStoreCrypto::decrypt(payload->span(), *decryptionKey_);
            if (!decrypted)
                return std::unexpected(decrypted.error());
            payload = RetainedSpan(std::move(*decrypted));
        }

        if (!detail::zlib::isCompressed(payload->span()))
            return payload;

        auto decoded = detail::zlib::decompress(payload->span(), payload->size());
        if (!decoded)
            return std::unexpected(decoded.error());

        return RetainedSpan(std::move(*decoded));
    }

    Result<RetainedSpan> ResourceStoreContents::parseItemFromChunk(
        RetainedSpan chunk, const size_t offset)
    {
        const detail::BinaryView data(chunk.span());
        auto item = data.tail(offset);
        if (!item)
            return std::unexpected(item.error());

        auto length = item->readLittle<uint32_t>(0);
        if (!length)
            return std::unexpected(length.error());

        const bool extended = *length == 0;
        if (extended)
        {
            length = item->readLittle<uint32_t>(4);
            if (!length)
                return std::unexpected(length.error());
        }

        const size_t headerSize = extended ? 8 : 4;
        auto payload = item->slice(headerSize, *length);
        if (!payload)
            return std::unexpected(payload.error());

        return chunk.slice(offset + headerSize, *length);
    }
}
