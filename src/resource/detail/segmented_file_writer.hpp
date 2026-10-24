//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/resource_type.hpp"
#include "MKD/result.hpp"
#include "io/output_file.hpp"

#include <initializer_list>
#include <memory>

namespace MKD::detail
{
    struct BlobLocation
    {
        uint64_t globalOffset;
        uint32_t length;
        uint32_t sequence;
        uint32_t fileOffset;
    };

    class SegmentedFileWriter
    {
    public:
        // Monokakido's RSC loader stops after file 1024.
        static constexpr uint32_t lastRscSequence = 1024;

        static Result<SegmentedFileWriter> create(const std::filesystem::path& directory,
                                                  ResourceType type, uint32_t fileBytes,
                                                  uint32_t lastSequence = UINT32_MAX);
        Result<BlobLocation> write(std::initializer_list<std::span<const uint8_t>> parts);
        Result<void> finish();
        [[nodiscard]] uint64_t offset() const noexcept { return globalOffset_; }

    private:
        SegmentedFileWriter(std::filesystem::path directory, ResourceType type, uint32_t fileBytes,
                            uint32_t lastSequence);
        Result<void> openFile();
        std::filesystem::path directory_;
        ResourceType type_;
        uint32_t fileBytes_;
        uint32_t lastSequence_;
        uint32_t sequence_;
        uint32_t fileOffset_ = 0;
        uint64_t globalOffset_ = 0;
        std::unique_ptr<OutputFile> file_;
    };
}
