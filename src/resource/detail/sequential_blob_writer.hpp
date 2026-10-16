//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/resource_type.hpp"

#include <filesystem>
#include <fstream>
#include <span>


namespace MKD::detail
{
    struct BlobLocation
    {
        size_t globalOffset;
        size_t length;
        size_t sequence;
        size_t fileOffset;
    };

    class SequentialBlobWriter
    {
    public:
        static Result<SequentialBlobWriter> create(const std::filesystem::path& directory,
                                                   ResourceType type,
                                                   size_t maxFileSize);


        Result<BlobLocation> write(std::span<const uint8_t> data);

        Result<void> finalize();

        [[nodiscard]] size_t currentOffset() const noexcept;

        [[nodiscard]] const std::filesystem::path& directory() const noexcept;

    private:
        explicit SequentialBlobWriter(const std::filesystem::path& directory,
                                      ResourceType type,
                                      std::ofstream file,
                                      size_t maxFileSize,
                                      size_t fileSequence);

        Result<void> rollToNextFile();

        std::filesystem::path directory_;
        ResourceType resourceType_;
        size_t maxFileSize_;

        std::ofstream currentFile_;
        size_t fileSequence_;
        size_t currentFileSize_ = 0;
        size_t globalOffset_ = 0;
    };
}
