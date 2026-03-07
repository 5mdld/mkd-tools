//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#include "MKD/resource/resource_type.hpp"
#include "sequential_blob_writer.hpp"

#include <format>

namespace MKD::detail
{
    Result<SequentialBlobWriter> SequentialBlobWriter::create(const fs::path& directory,
                                                              const ResourceType type,
                                                              const size_t maxFileSize)
    {
        if (fs::is_regular_file(directory))
            return std::unexpected(std::format("Output directory path is an existing file: {}",
                                               directory.string()));

        const auto traits = resourceTraits(type);
        const auto path = directory / traits.blobFilename(traits.firstSequence);

        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file)
            return std::unexpected(std::format("Failed to open output file: {}", path.string()));

        return SequentialBlobWriter(directory, type, std::move(file), maxFileSize, traits.firstSequence);
    }


    SequentialBlobWriter::SequentialBlobWriter(const fs::path& directory,
                                               ResourceType type,
                                               std::ofstream file,
                                               size_t maxFileSize,
                                               size_t fileSequence)
        : directory_(directory),
          resourceType_(type),
          maxFileSize_(maxFileSize),
          currentFile_(std::move(file)),
          fileSequence_(fileSequence)
    {
    }


    Result<BlobLocation> SequentialBlobWriter::write(const std::span<const uint8_t> data)
    {
        if (data.empty())
            return BlobLocation{.globalOffset = globalOffset_, .length = 0};

        const BlobLocation location{
            .globalOffset = globalOffset_,
            .length = data.size(),
        };

        currentFile_.write(reinterpret_cast<const char*>(data.data()),
                           static_cast<std::streamsize>(data.size()));

        if (!currentFile_)
            return std::unexpected(std::format("Write failed at global offset {}", globalOffset_));

        currentFileSize_ += data.size();
        globalOffset_ += data.size();

        if (currentFileSize_ > 0 && currentFileSize_ > maxFileSize_)
            if (const auto result = rollToNextFile(); !result)
                return std::unexpected("Failed to roll to next blob file");

        return location;
    }


    Result<void> SequentialBlobWriter::finalize()
    {
        currentFile_.flush();
        if (!currentFile_)
            return std::unexpected("Failed to flush final blob file");

        currentFile_.close();
        return {};
    }


    size_t SequentialBlobWriter::currentOffset() const noexcept
    {
        return globalOffset_;
    }


    size_t SequentialBlobWriter::fileSequence() const noexcept
    {
        return fileSequence_;
    }


    size_t SequentialBlobWriter::currentFileOffset() const noexcept
    {
        return currentFileSize_;
    }


    fs::path SequentialBlobWriter::directory() const noexcept
    {
        return directory_;
    }


    Result<void> SequentialBlobWriter::rollToNextFile()
    {
        currentFile_.flush();
        currentFile_.close();

        ++fileSequence_;
        currentFileSize_ = 0;

        const auto path = directory_ / resourceTraits(resourceType_).blobFilename(fileSequence_);

        currentFile_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!currentFile_)
            return std::unexpected(std::format("Failed to open blob file: {}", path.string()));

        return {};
    }
}
