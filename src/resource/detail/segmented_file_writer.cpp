//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "segmented_file_writer.hpp"

#include <limits>

namespace MKD::detail
{
    SegmentedFileWriter::SegmentedFileWriter(std::filesystem::path directory, const ResourceType type,
                                             const uint32_t fileBytes, const uint32_t lastSequence)
        : directory_(std::move(directory)), type_(type), fileBytes_(fileBytes), lastSequence_(lastSequence),
          sequence_(resourceTraits(type).firstSequence)
    {
    }

    Result<SegmentedFileWriter> SegmentedFileWriter::create(const std::filesystem::path& directory,
                                                            const ResourceType type, const uint32_t fileBytes,
                                                            const uint32_t lastSequence)
    {
        if (fileBytes == 0)
            return std::unexpected("Blob file size must be nonzero");

        SegmentedFileWriter writer(directory, type, fileBytes, lastSequence);
        if (auto opened = writer.openFile(); !opened)
            return std::unexpected(opened.error());

        return writer;
    }

    Result<void> SegmentedFileWriter::openFile()
    {
        file_ = std::make_unique<OutputFile>(directory_ / resourceTraits(type_).blobFilename(sequence_), false);
        if (!file_->isOpen())
            return std::unexpected("Cannot create resource blob file");
        return {};
    }

    Result<BlobLocation> SegmentedFileWriter::write(std::initializer_list<std::span<const uint8_t>> parts)
    {
        uint64_t length = 0;
        for (const auto part : parts)
            length += part.size();

        if (!file_ || length > UINT32_MAX || globalOffset_ > UINT64_MAX - length)
            return std::unexpected("Resource payload exceeds format limits");

        if (fileOffset_ && length > fileBytes_ - std::min(fileOffset_, fileBytes_))
        {
            if (sequence_ == lastSequence_)
                return std::unexpected("Too many resource blob files");

            if (auto closed = file_->finish(); !closed)
                return std::unexpected(closed.error());

            ++sequence_;
            fileOffset_ = 0;
            if (auto opened = openFile(); !opened)
                return std::unexpected(opened.error());
        }

        if (length > UINT32_MAX - fileOffset_)
            return std::unexpected("Resource file offset exceeds format limits");

        const BlobLocation location{globalOffset_, static_cast<uint32_t>(length), sequence_, fileOffset_};
        for (const auto part : parts)
            if (auto result = file_->write(part); !result)
                return std::unexpected(result.error());

        globalOffset_ += length;
        fileOffset_ += static_cast<uint32_t>(length);
        return location;
    }

    Result<void> SegmentedFileWriter::finish()
    {
        if (!file_)
            return std::unexpected("Resource files have already finished");

        if (auto result = file_->finish(); !result)
            return std::unexpected(result.error());

        file_.reset();
        return {};
    }
}
