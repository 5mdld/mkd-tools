//
// kiwakiwaaにより 2026/01/22 に作成されました。
//

#include "MKD/resource/rsc/rsc_data.hpp"

#include <algorithm>
#include <format>

namespace MKD
{
    Result<RscData> RscData::load(const fs::path& directoryPath, std::string_view dictId, const uint32_t mapVersion)
    {
        auto files = discoverFiles(directoryPath);
        if (!files) return std::unexpected(files.error());

        std::ranges::sort(*files, {}, &RscResourceFile::sequenceNumber);

        // Calculate and set global offset for each .rsc file
        size_t globalOffset = 0;
        for (size_t i = 0; i < files->size(); ++i)
        {
            if ((*files)[i].sequenceNumber != i)
                return std::unexpected(std::format("Missing resource file with sequence number: {}", i));

            (*files)[i].globalOffset = globalOffset;
            globalOffset += (*files)[i].length;
        }

        std::optional<std::array<uint8_t, 32>> key;
        if (mapVersion == 1 && !dictId.empty())
            key = RscCrypto::deriveKey(dictId);

        return RscData{
            std::move(*files),
            key
        };
    }


    Result<std::span<const uint8_t>> RscData::get(const MapRecord& record) const
    {
        // Special case: intraBlock offset of 0xFFFFFFFF means direct data access (no chunk decompression is used)
        if (record.ioffset == 0xFFFFFFFF)
        {
            return readDirectData(record.zOffset);
        }

        // Load the chunk if we haven't loaded it yet or if it's a different chunk
        if (chunkBuffer_.empty() || record.zOffset != currentChunkOffset_)
        {
            if (auto result = loadChunk(record.zOffset); !result)
                return std::unexpected(result.error());
        }

        // Parse the item from the decompressed chunk
        return parseItemFromChunk(record.ioffset);
    }


    RscData::RscData(std::vector<RscResourceFile>&& files, const std::optional<std::array<uint8_t, 32> >& decryptionKey)
        : files_(std::move(files)), decryptionKey_(decryptionKey)
    {
        decompressor_ = std::make_unique<ZlibDecompressor>();
    }


    Result<std::vector<RscResourceFile>> RscData::discoverFiles(const fs::path& directoryPath)
    {
        std::vector<RscResourceFile> files;

        for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".rsc")
                continue;

            const auto seqNum = detail::parseSequenceNumber(entry.path().filename(), ".rsc");
            if (!seqNum) continue;

            std::error_code ec;
            const auto fileSize = fs::file_size(entry.path(), ec);
            if (ec)
                return std::unexpected(
                    std::format("Failed to get size of '{}': {}", entry.path().string(), ec.message())
                );

            files.push_back(RscResourceFile{
                .sequenceNumber = *seqNum,
                .globalOffset = 0, // Set after discovery
                .length = fileSize,
                .filePath = entry.path()
            });
        }

        if (files.empty())
            return std::unexpected(std::format("No .rsc files found in directory: {}", directoryPath.string()));

        return files;
    }


    Result<std::pair<const RscResourceFile&, size_t>> RscData::findFileByOffset(const size_t globalOffset) const
    {
        // Binary search to find which file contains this offset
        auto it = std::ranges::upper_bound(
            files_,
            globalOffset,
            std::less{},
            [](const RscResourceFile& f) { return f.globalOffset; }
        );

        if (it == files_.begin())
            return std::unexpected("Offset is before the first file");

        // Go back to the file that starts at or before this offset
        --it;

        if (const size_t fileEnd = it->globalOffset + it->length; globalOffset >= fileEnd)
            return std::unexpected("Offset is beyond file boundaries");

        size_t localOffset = globalOffset - it->globalOffset;
        return std::pair{std::ref(*it), localOffset};
    }


    Result<BinaryFileReader> RscData::openReaderAt(size_t globalOffset) const
    {
        auto location = findFileByOffset(globalOffset);
        if (!location) return std::unexpected(location.error());
        auto& [file, localOffset] = *location;

        auto reader = BinaryFileReader::open(file.filePath);
        if (!reader) return std::unexpected(reader.error());

        if (auto s = reader->seek(localOffset); !s)
            return std::unexpected(s.error());

        return std::move(*reader);
    }


    Result<void> RscData::loadChunk(size_t globalOffset) const
    {
        auto reader = openReaderAt(globalOffset);
        if (!reader) return std::unexpected(reader.error());

        auto data = readAndProcessChunk(*reader);
        if (!data) return std::unexpected(data.error());

        chunkBuffer_ = std::move(*data);
        currentChunkOffset_ = globalOffset;
        return {};
    }


    Result<std::vector<uint8_t>> RscData::readAndProcessChunk(BinaryFileReader& reader) const
    {
        auto marker = reader.read<uint32_t>();
        if (!marker) return std::unexpected(marker.error());

        auto data = *marker == 0
            ? readAndDecryptData(reader)
            : reader.readBytes(*marker).transform([](auto bytes) { return bytes; });

        if (!data) return std::unexpected(data.error());

        // decompress data if needed
        if (!ZlibDecompressor::isZlibCompressed(*data))
            return data;

        if (auto result = decompressor_->decompress(*data, data->size()); !result)
            return std::unexpected(result.error());

        return decompressor_->takeBuffer();
    }


    Result<std::vector<uint8_t>> RscData::readAndDecryptData(BinaryFileReader& reader) const
    {
        if (!decryptionKey_)
            return std::unexpected("Missing decryption key");

        auto seq = reader.sequence();
        const auto length = seq.readValue<uint32_t>();
        auto encrypted = seq.readBytes(length);

        if (!seq) return std::unexpected(seq.error());

        return RscCrypto::decrypt(encrypted, *decryptionKey_);
    }


    Result<std::span<const uint8_t>> RscData::readDirectData(const size_t globalOffset) const
    {
        auto reader = openReaderAt(globalOffset);
        if (!reader) return std::unexpected(reader.error());

        auto seq = reader->sequence();
        const auto length = seq.readValue<uint32_t>();
        directDataBuffer_ = seq.readBytes(length);

        if (!seq) return std::unexpected(seq.error());

        return std::span<const uint8_t>(directDataBuffer_);
    }


    Result<std::span<const uint8_t>> RscData::parseItemFromChunk(const size_t offset) const
    {
        if (offset >= chunkBuffer_.size())
            return std::unexpected("Invalid offset within chunk");

        const uint8_t* data = chunkBuffer_.data() + offset;
        const size_t remaining = chunkBuffer_.size() - offset;

        if (remaining < 4)
            return std::unexpected("Insufficient data for item header");

        const uint32_t firstWord = *reinterpret_cast<const uint32_t*>(data);
        const bool isNewFormat = firstWord == 0;

        if (isNewFormat && remaining < 8)
            return std::unexpected("Insufficient data for new format header");

        const size_t headerSize = isNewFormat ? 8 : 4;
        const size_t contentLength = isNewFormat
            ? *reinterpret_cast<const uint32_t*>(data + 4)
            : firstWord;

        if (remaining < headerSize + contentLength)
            return std::unexpected("Insufficient data for item content");

        return std::span(data + headerSize, contentLength);
    }
}
