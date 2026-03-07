//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/resource_type.hpp"

#include <filesystem>
#include <fstream>
#include <span>

namespace fs = std::filesystem;

namespace MKD::detail
{
    struct BlobLocation
    {
        size_t globalOffset;
        size_t length;
    };

    class SequentialBlobWriter
    {
    public:
        static Result<SequentialBlobWriter> create(const fs::path& directory,
                                                   ResourceType type,
                                                   size_t maxFileSize);


        Result<BlobLocation> write(std::span<const uint8_t> data);

        Result<void> finalize();

        [[nodiscard]] size_t currentOffset() const noexcept;

        [[nodiscard]] size_t fileSequence() const noexcept;

        [[nodiscard]] size_t currentFileOffset() const noexcept;

        [[nodiscard]] fs::path directory() const noexcept;

    private:
        explicit SequentialBlobWriter(const fs::path& directory,
                                      ResourceType type,
                                      std::ofstream file,
                                      size_t maxFileSize,
                                      size_t fileSequence);

        Result<void> rollToNextFile();

        fs::path directory_;
        ResourceType resourceType_;
        size_t maxFileSize_;

        std::ofstream currentFile_;
        size_t fileSequence_;
        size_t currentFileSize_ = 0;
        size_t globalOffset_ = 0;
    };
}
