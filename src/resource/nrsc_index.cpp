//
// Caoimheにより 2026/01/16 に作成されました。
//

#include "monokakido/resource/nrsc_index.hpp"
#include "monokakido/core/platform/fs.hpp"

#include <bit>
#include <format>
#include <fstream>

namespace monokakido::resource
{
    CompressionFormat NrscIndexRecord::compressionFormat() const
    {
        uint16_t fmt = format;
        if constexpr (std::endian::native == std::endian::big)
            fmt = std::byteswap(fmt);

        switch (fmt)
        {
            case 0: return CompressionFormat::Uncompressed;
            case 1: return CompressionFormat::Zlib;
            default:
                throw std::runtime_error(std::format("Invalid compression format: {}", fmt));
        }
    }

    size_t NrscIndexRecord::fileSeq() const noexcept
    {
        return fileSequence;
    }

    size_t NrscIndexRecord::idOffset() const noexcept
    {
        return idStringOffset;
    }

    uint64_t NrscIndexRecord::offset() const noexcept
    {
        return fileOffset;
    }

    size_t NrscIndexRecord::len() const noexcept
    {
        return length;
    }

    void NrscIndexRecord::toLittleEndian() noexcept
    {
        if constexpr (std::endian::native == std::endian::big)
        {
            format = std::byteswap(format);
            fileSequence = std::byteswap(fileSequence);
            idStringOffset = std::byteswap(idStringOffset);
            fileOffset = std::byteswap(fileOffset);
            length = std::byteswap(length);
        }
    }


    std::expected<NrscIndex, std::string> NrscIndex::load(const fs::path& directoryPath)
    {
        const auto indexPath = directoryPath / "index.nidx";
        if (!fs::exists(indexPath))
            return std::unexpected(std::format("index.nidx not found in: {}", directoryPath.string()));

        std::ifstream file(indexPath, std::ios::binary);
        if (!file)
            return std::unexpected(std::format("Failed to open index file: {}", indexPath.string()));

        // read header
        auto headerResult = readHeader(file);
        if (!headerResult)
            return std::unexpected(headerResult.error());
        const auto& [zeroField, recordCount] = headerResult.value();

        // Calculate sizes
        const size_t totalHeaderSize = HEADER_SIZE + recordCount * RECORD_SIZE;
        const auto fileSize = fs::file_size(indexPath);
        const auto stringRegionSize = fileSize - totalHeaderSize;

        // read records
        auto recordsResult = readRecords(file, recordCount);
        if (!recordsResult)
            return std::unexpected(recordsResult.error());

        // read ID strings
        auto stringsResult = readIdStrings(file, stringRegionSize);
        if (!stringsResult)
            return std::unexpected(stringsResult.error());

        return NrscIndex(
            std::move(*recordsResult),
            std::move(*stringsResult),
            totalHeaderSize
        );
    }


    std::expected<NrscIndexRecord, std::string> NrscIndex::findById(std::string_view id) const
    {
        const auto it = std::lower_bound(records_.begin(), records_.end(), id,
            [this](const NrscIndexRecord& record, std::string_view searchId)
            {
                const auto idResult = this->getIdAt(record.idOffset());
                if (!idResult)
                    return false;

                return *idResult < searchId;
            });

        if (it == records_.end())
            return std::unexpected(std::format("Resource not found: {}", id));

        auto foundIdResult = getIdAt(it->idOffset());
        if (!foundIdResult)
            return std::unexpected(foundIdResult.error());

        if (*foundIdResult != id)
            return std::unexpected(std::format("Resource not found: {}", id));

        return *it;
    }


    std::expected<std::pair<std::string_view, NrscIndexRecord>, std::string> NrscIndex::getByIndex(const size_t index) const
    {
        if (index >= records_.size())
            return std::unexpected(std::format("Index {} out of range (size: {})", index, records_.size()));

        const auto& record = records_.at(index);
        auto idResult = getIdAt(record.idOffset());
        if (!idResult)
            return std::unexpected(idResult.error());

        return std::make_pair(*idResult, record);
    }


    size_t NrscIndex::size() const noexcept
    {
        return records_.size();
    }


    NrscIndex::NrscIndex(std::vector<NrscIndexRecord>&& records, std::string&& idStrings, const size_t headerSize)
        : records_(std::move(records))
        , idStrings_(std::move(idStrings))
        , headerSize_(headerSize)
    {
    }


    std::expected<std::string_view, std::string> NrscIndex::getIdAt(size_t offset) const
    {
        const size_t adjustedOffset = offset - headerSize_;

        if (adjustedOffset > 0 && idStrings_.at(adjustedOffset - 1) != '\0')
            return std::unexpected(std::format("Invalid ID offset: {} (not at string boundary)", offset));

        if (adjustedOffset >= idStrings_.size())
            return std::unexpected(std::format("ID offset {} out of range", offset));

        // find the null terminator
        const auto nullPos = idStrings_.find('\0', adjustedOffset);
        if (nullPos == std::string::npos)
        {
            return std::unexpected(std::format("ID string at offset {} not null-terminated", offset));
        }

        const size_t length = nullPos - adjustedOffset;
        return std::string_view(idStrings_.data() + adjustedOffset, length);
    }


    std::expected<IndexHeader, std::string> NrscIndex::readHeader(std::ifstream& file)
    {
        IndexHeader header{};
        file.read(reinterpret_cast<char*>(&header), HEADER_SIZE);

        if (!file)
            return std::unexpected(platform::fs::makeFilestreamError(file, "read header"));

        if constexpr (std::endian::native == std::endian::big)
            header.recordCount = std::byteswap(header.recordCount);

        return header;
    }


    std::expected<std::vector<NrscIndexRecord>, std::string> NrscIndex::readRecords(std::ifstream& file, uint32_t count)
    {
        std::vector<NrscIndexRecord> records(count);

        const auto bytesToRead = static_cast<std::streamsize>(count * RECORD_SIZE);
        file.read(reinterpret_cast<char*>(records.data()), bytesToRead);

        if (!file)
            return std::unexpected(platform::fs::makeFilestreamError(file, "read records"));

        for (auto& record : records)
        {
            record.toLittleEndian();
        }

        return records;
    }


    std::expected<std::string, std::string> NrscIndex::readIdStrings(std::ifstream& file, const size_t stringRegionSize)
    {
        std::string idStrings;
        idStrings.resize(stringRegionSize);

        const auto bytesToRead = static_cast<std::streamsize>(stringRegionSize);
        file.read(idStrings.data(), bytesToRead);

        if (!file)
            return std::unexpected(platform::fs::makeFilestreamError(file, "read ID strings region"));

        return idStrings;
    }
}
