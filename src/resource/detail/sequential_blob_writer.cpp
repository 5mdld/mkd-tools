//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#include "MKD/resource/resource_type.hpp"
#include "sequential_blob_writer.hpp"

#include <algorithm>
#include <format>
#include <limits>

namespace MKD::detail
{
    Result<SequentialBlobWriter> SequentialBlobWriter::create(const std::filesystem::path& directory,
                                                              const ResourceType type,
                                                              const size_t maxFileSize)
    {
        if (maxFileSize == 0 || maxFileSize > std::numeric_limits<uint32_t>::max())
            return std::unexpected("Blob file size limit must fit a nonzero uint32");
        if (std::filesystem::is_regular_file(directory))
            return std::unexpected(std::format("Output directory path is an existing file: {}",
                                               directory.string()));

        const auto traits = resourceTraits(type);
        const auto path = directory / traits.blobFilename(traits.firstSequence);

        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file)
            return std::unexpected(std::format("Failed to open output file: {}", path.string()));

        return SequentialBlobWriter(directory, type, std::move(file), maxFileSize, traits.firstSequence);
    }


    SequentialBlobWriter::SequentialBlobWriter(const std::filesystem::path& directory,
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
        if (data.size() > std::numeric_limits<uint32_t>::max() ||
            data.size() > std::numeric_limits<size_t>::max() - globalOffset_)
            return std::unexpected("Blob data exceeds the supported size");
        // Keep each payload in one file and open another only when there is data for it.
        if (currentFileSize_ > 0 && data.size() > maxFileSize_ - std::min(currentFileSize_, maxFileSize_))
        {
            auto rolled = rollToNextFile();
            if (!rolled)
                return std::unexpected(rolled.error());
        }
        const BlobLocation location{
            .globalOffset = globalOffset_,
            .length = data.size(),
            .sequence = fileSequence_,
            .fileOffset = currentFileSize_,
        };
        if (data.empty())
            return location;

        currentFile_.write(reinterpret_cast<const char*>(data.data()),
                           static_cast<std::streamsize>(data.size()));

        if (!currentFile_)
            return std::unexpected(std::format("Write failed at global offset {}", globalOffset_));

        currentFileSize_ += data.size();
        globalOffset_ += data.size();

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


    const std::filesystem::path& SequentialBlobWriter::directory() const noexcept
    {
        return directory_;
    }


    Result<void> SequentialBlobWriter::rollToNextFile()
    {
        currentFile_.flush();
        if (!currentFile_)
            return std::unexpected("Failed to flush blob file before rollover");
        currentFile_.close();
        if (currentFile_.fail())
            return std::unexpected("Failed to close blob file before rollover");

        ++fileSequence_;
        currentFileSize_ = 0;

        const auto path = directory_ / resourceTraits(resourceType_).blobFilename(fileSequence_);

        currentFile_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!currentFile_)
            return std::unexpected(std::format("Failed to open blob file: {}", path.string()));

        return {};
    }
}
