//
// Caoimheにより 2026/01/16 に作成されました。
//

#include "monokakido/resource/nrsc_data.hpp"

#include <algorithm>
#include <format>
#include <optional>

namespace monokakido::resource
{
    std::expected<NrscData, std::string> NrscData::load(const fs::path& directoryPath)
    {
        std::vector<ResourceFile> files;

        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            if (!entry.is_regular_file())
                continue;

            const auto seqNum = parseSequenceNumber(entry.path().filename());
            if (!seqNum)
                continue;

            std::error_code ec;
            const auto fileSize = fs::file_size(entry.path(), ec);
            if (ec)
                return std::unexpected(
                    std::format("Failed to get size of '{}': {}", entry.path().string(), ec.message())
                );

            files.push_back(ResourceFile{
                .sequenceNumber = *seqNum,
                .fileSize = fileSize,
                .globalOffset = 0, // Set below
                .filePath = entry.path()
            });
        }

        if (files.empty())
            return std::unexpected(std::format("No .nrsc files found in directory: {}", directoryPath.string()));

        std::ranges::sort(files, {}, &ResourceFile::sequenceNumber);

        size_t globalOffset = 0;
        for (size_t i = 0; i < files.size(); ++i)
        {
            if (files[i].sequenceNumber != i)
            {
                return std::unexpected(std::format("Missing resource file with sequence number: {}", i));
            }

            files[i].globalOffset = globalOffset;
            globalOffset += files[i].fileSize;
        }

        return NrscData{std::move(files)};
    }


    std::expected<std::span<const uint8_t>, std::string> NrscData::get(const NrscIndexRecord& record) const
    {
        const auto fileOpt = findFileForOffset(record.offset());
        if (!fileOpt)
            return std::unexpected(std::format("Invalid file offset: {}", record.offset()));


        // calculate offset within this specific file
        const ResourceFile& file = fileOpt->get();
        const uint64_t fileOffset = record.offset() - file.globalOffset;

        std::ifstream fileStream(file.filePath, std::ios::binary);
        if (!fileStream)
            return std::unexpected(std::format("Failed to open resource file '{}'", file.filePath.string()));

        fileStream.seekg(static_cast<std::streamoff>(fileOffset));
        if (!fileStream)
            return std::unexpected(std::format("Failed to seek to offset '{}'", fileOffset));

        if (readBuffer_.size() < record.len())
            readBuffer_.resize(record.len());

        fileStream.read(reinterpret_cast<char*>(readBuffer_.data()), static_cast<std::streamsize>(record.len()));
        if (!fileStream)
            return std::unexpected("Failed to read resource data");

        return readAndDecompress(record);
    }


    NrscData::NrscData(std::vector<ResourceFile>&& files)
        : files_(std::move(files)), decompressor_(std::make_unique<ZlibDecompressor>())
    {
    }


    std::optional<std::reference_wrapper<const ResourceFile>> NrscData::findFileForOffset(const uint64_t offset) const
    {
        auto it = std::ranges::upper_bound(files_, offset, {},
            [](const ResourceFile& file){ return file.globalOffset; });

        if (it == files_.begin())
            return std::nullopt;

        return std::ref(*(--it));
    }


    std::expected<std::span<const uint8_t>, std::string> NrscData::readAndDecompress(const NrscIndexRecord& record) const
    {
        const auto format = record.compressionFormat();
        const std::span<const uint8_t> compressed(readBuffer_.data(), record.len());

        switch (format)
        {
            case CompressionFormat::Uncompressed:
                return compressed;

            case CompressionFormat::Zlib:
                return decompressor_->decompress(compressed, record.len());

            default:
                return std::unexpected("Unknown compression format");
        }
    }



    std::optional<uint32_t> NrscData::parseSequenceNumber(const fs::path& filename)
    {
        if (filename.extension() != ".nrsc")
            return std::nullopt;

        const auto stem = filename.stem().string();
        try
        {
            return std::stoul(stem);
        }
        catch (const std::invalid_argument&)
        {
            return std::nullopt;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}
