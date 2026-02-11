//
// kiwakiwaaにより 2026/02/11 に作成されました。
//

#include "monokakido/resource/keystore/keystore.hpp"


namespace monokakido
{
    void KeystoreHeader::swapEndianness() noexcept
    {
        version = std::byteswap(version);
        wordsOffset = std::byteswap(wordsOffset);
        idxOffset = std::byteswap(idxOffset);
        nextOffset = std::byteswap(nextOffset);
    }


    size_t KeystoreHeader::headerSize() const noexcept
    {
        return version == 0x10000 ? 16 : 32;
    }

    std::expected<Keystore, std::string> Keystore::load(const fs::path& path)
    {
        auto readerResult = platform::fs::BinaryFileReader::open(path);
        if (!readerResult)
            return std::unexpected(readerResult.error());
        auto& reader = *readerResult;

        // Get file size for validation
        const size_t fileSize = reader.remainingBytes();
        auto headerResult = readHeader(reader);
        if (!headerResult)
            return std::unexpected(headerResult.error());
        auto& header = *headerResult;

        // Read entire file into memory
        if (auto seekResult = reader.seek(0); !seekResult)
            return std::unexpected(seekResult.error());

        auto fileDataResult = reader.readBytes(fileSize);
        if (!fileDataResult)
            return std::unexpected(fileDataResult.error());

        std::vector<uint8_t> fileData = std::move(*fileDataResult);

        // Read and validate index header
        const size_t indexEnd = header.nextOffset == 0 ? fileSize : header.nextOffset;

        auto indexHeaderResult = readIndexHeader(std::span(fileData).subspan(header.idxOffset),
                                                 indexEnd - header.idxOffset);
        if (!indexHeaderResult)
            return std::unexpected(indexHeaderResult.error());

        IndexHeader idxHeader = *indexHeaderResult;

        // Read the four indices
        auto indices = readIndices(
            std::span(fileData).subspan(header.idxOffset),
            idxHeader,
            indexEnd - header.idxOffset
        );
        if (!indices)
            return std::unexpected(indices.error());

        return Keystore(
            std::move(fileData),
            std::move(indices->indexA),
            std::move(indices->indexB),
            std::move(indices->indexC),
            std::move(indices->indexD),
            header.wordsOffset,
            header.version == KEYSTORE_V2
        );
    }


    std::expected<KeystoreLookupResult, std::string> Keystore::getByIndex(
        const KeystoreIndex indexType, const size_t index) const
    {
        const auto* indexPtr = getIndexArray(indexType);
        if (!indexPtr)
            return std::unexpected("Invalid index type");
        if (indexPtr->empty())
            return std::unexpected("Index does not exist");
        if (index >= indexPtr->size())
            return std::unexpected("Index out of bounds");

        const uint32_t wordOffset = (*indexPtr)[index];

        auto entry = getWordEntry(wordOffset);
        if (!entry)
            return std::unexpected(entry.error());

        return buildLookupResult(entry->key, entry->pagesOffset, index);
    }


    size_t Keystore::indexSize(const KeystoreIndex indexType) const noexcept
    {
        const auto* ptr = getIndexArray(indexType);
        return ptr ? ptr->size() : 0;
    }

    Keystore::Keystore(
        std::vector<uint8_t>&& fileData,
        std::vector<uint32_t>&& indexLength,
        std::vector<uint32_t>&& indexPrefix,
        std::vector<uint32_t>&& indexSuffix,
        std::vector<uint32_t>&& indexD,
        const size_t wordsOffset,
        const bool hasConversionTable)
        : fileData_(std::move(fileData))
          , indexLength_(std::move(indexLength))
          , indexPrefix_(std::move(indexPrefix))
          , indexSuffix_(std::move(indexSuffix))
          , indexD_(std::move(indexD))
          , wordsOffset_(wordsOffset)
          , hasConversionTable_(hasConversionTable)
    {
    }


    const std::vector<uint32_t>* Keystore::getIndexArray(const KeystoreIndex type) const noexcept
    {
        switch (type)
        {
            case KeystoreIndex::Length: return &indexLength_;
            case KeystoreIndex::Prefix: return &indexPrefix_;
            case KeystoreIndex::Suffix: return &indexSuffix_;
            case KeystoreIndex::Other: return &indexD_;
            default: return nullptr;
        }
    }


    std::expected<KeystoreWordEntry, std::string> Keystore::getWordEntry(const uint32_t wordOffset) const
    {
        const size_t absoluteOffset = wordsOffset_ + wordOffset;

        if (absoluteOffset + sizeof(uint32_t) + 1 >= fileData_.size())
            return std::unexpected("Word offset out of bounds");

        // Read pages offset (little-endian uint32)
        uint32_t pagesOffset;
        std::memcpy(&pagesOffset, &fileData_[absoluteOffset], sizeof(uint32_t));
        if constexpr (std::endian::native == std::endian::big)
            pagesOffset = std::byteswap(pagesOffset);

        const size_t stringStart = absoluteOffset + sizeof(uint32_t) + 1;

        const auto* nullPos = static_cast<const uint8_t*>(std::memchr(
            &fileData_[stringStart], 0, fileData_.size() - stringStart));

        if (!nullPos)
            return std::unexpected("Unterminated word string");

        const size_t stringLength = nullPos - &fileData_[stringStart];

        return KeystoreWordEntry{
            .key = std::string_view(reinterpret_cast<const char*>(&fileData_[stringStart]), stringLength),
            .pagesOffset = pagesOffset,
        };
    }


    std::expected<KeystoreLookupResult, std::string> Keystore::buildLookupResult(
        std::string_view key, const size_t pagesOffset, const size_t index) const
    {
        auto pageIter = getPageIterator(pagesOffset);
        if (!pageIter)
            return std::unexpected(pageIter.error());

        KeystoreLookupResult result;
        result.key = key;
        result.index = index;
        result.pageData_ = pageIter->data_;
        result.count_ = pageIter->remaining_;
        return result;
    }


    std::expected<PageReferenceIterator, std::string> Keystore::getPageIterator(const size_t pagesOffset) const
    {
        const size_t absoluteOffset = wordsOffset_ + pagesOffset;

        if (absoluteOffset + 2 > fileData_.size())
            return std::unexpected(std::format(
                "Pages offset out of bounds: {} + 2 > {}", absoluteOffset, fileData_.size()));

        uint16_t count;
        std::memcpy(&count, &fileData_[absoluteOffset], sizeof(uint16_t));
        if constexpr (std::endian::native == std::endian::big)
            count = std::byteswap(count);

        const size_t dataStart = absoluteOffset + 2;
        const auto pageData = std::span(fileData_).subspan(dataStart);

        // Validate by decoding all entries
        auto tempData = pageData;
        for (uint16_t i = 0; i < count; ++i)
        {
            auto decoded = decodeKeystoreEntry(tempData);
            if (!decoded)
                return std::unexpected(decoded.error());

            if (decoded->bytesConsumed > tempData.size())
                return std::unexpected("Invalid page data");

            tempData = tempData.subspan(decoded->bytesConsumed);
        }

        return PageReferenceIterator(pageData, count);
    }


    std::expected<KeystoreHeader, std::string> Keystore::readHeader(platform::fs::BinaryFileReader& reader)
    {
        auto headerResult = reader.readStructPartial<KeystoreHeader>(16);
        if (!headerResult)
            return std::unexpected(headerResult.error());

        KeystoreHeader header = *headerResult;

        if (header.version != KEYSTORE_V1 && header.version != KEYSTORE_V2)
        {
            return std::unexpected(std::format(
                "Invalid keystore version: 0x{:x}", header.version));
        }

        if (header.version == KEYSTORE_V2)
        {
            const auto extraResult = reader.readStructPartial<KeystoreHeader>(16);
            if (!extraResult)
                return std::unexpected("Failed to read v2 header extension");

            std::memcpy(&header.nextOffset, &extraResult->version, 16);
        }

        if (header.magic1 != 0)
            return std::unexpected("Invalid magic1 field");

        if (header.version == KEYSTORE_V1)
        {
            if (header.wordsOffset >= header.idxOffset)
                return std::unexpected("Invalid offset ordering in v1 header");
        }
        else
        {
            if (header.magic5 != 0 || header.magic6 != 0 || header.magic7 != 0)
                return std::unexpected("Invalid magic fields in v2 header");

            if (header.wordsOffset >= header.idxOffset)
                return std::unexpected("Invalid offset ordering in v2 header");

            if (header.nextOffset != 0 && header.idxOffset >= header.nextOffset)
                return std::unexpected("Invalid next offset in v2 header");
        }

        return header;
    }


    std::expected<IndexHeader, std::string> Keystore::readIndexHeader(std::span<const uint8_t> data, size_t maxSize)
    {
        if (data.size() < sizeof(IndexHeader))
            return std::unexpected("Index header truncated");

        IndexHeader header{};
        std::memcpy(&header, data.data(), sizeof(IndexHeader));

        if constexpr (std::endian::native == std::endian::big)
            header.swapEndianness();

        if (header.magic != INDEX_MAGIC)
        {
            return std::unexpected(std::format("Invalid index magic: expected 0x04, got 0x{:x}",
                                               header.magic));
        }

        // Validate offset ordering
        const auto checkOrder = [maxSize](const uint32_t left, const uint32_t right) {
            return right == 0 || left < right || right == maxSize;
        };

        if (!checkOrder(header.indexAOffset, header.indexBOffset) ||
            !checkOrder(header.indexBOffset, header.indexCOffset) ||
            !checkOrder(header.indexCOffset, header.indexDOffset) ||
            !checkOrder(header.indexDOffset, maxSize))
        {
            return std::unexpected("Invalid index offset ordering");
        }

        return header;
    }


    std::expected<Keystore::Indices, std::string> Keystore::readIndices(std::span<const uint8_t> data,
                                                                        const IndexHeader& header, size_t maxSize)
    {
        Indices indices;

        auto readIndex = [&](const uint32_t start, const uint32_t end)
            -> std::expected<std::vector<uint32_t>, std::string> {
            if (start == 0)
                return std::vector<uint32_t>{}; // Index doesn't exist

            const size_t actualEnd = (end == 0) ? maxSize : end;
            const size_t length = actualEnd - start;

            if (length < sizeof(uint32_t))
                return std::unexpected("Index too small");

            if (length % sizeof(uint32_t) != 0)
                return std::unexpected("Index size not multiple of 4");

            const size_t count = length / sizeof(uint32_t);
            std::vector<uint32_t> index(count);

            std::memcpy(index.data(),
                        data.data() + start,
                        length);

            if constexpr (std::endian::native == std::endian::big)
            {
                for (auto& val : index)
                {
                    val = std::byteswap(val);
                }
            }

            // first element should be count - 1
            if (!index.empty() && index[0] + 1 != count)
                return std::unexpected("Index count mismatch");

            // Remove the count element, keep only offsets
            if (!index.empty())
                index.erase(index.begin());

            return index;
        };

        auto idxA = readIndex(header.indexAOffset, header.indexBOffset);
        if (!idxA) return std::unexpected(idxA.error());
        indices.indexA = std::move(*idxA);

        auto idxB = readIndex(header.indexBOffset, header.indexCOffset);
        if (!idxB) return std::unexpected(idxB.error());
        indices.indexB = std::move(*idxB);

        auto idxC = readIndex(header.indexCOffset, header.indexDOffset);
        if (!idxC) return std::unexpected(idxC.error());
        indices.indexC = std::move(*idxC);

        auto idxD = readIndex(header.indexDOffset, maxSize);
        if (!idxD) return std::unexpected(idxD.error());
        indices.indexD = std::move(*idxD);

        return indices;
    }
}
