//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "segmented_file_writer.hpp"
#include "io/build_directory.hpp"
#include "binary/binary_buffer.hpp"

namespace MKD::detail
{
    class FragmentWriter
    {
    public:
        static Result<FragmentWriter> create(const std::filesystem::path& directory,
                                             ResourceType type = ResourceType::Fonts,
                                             uint32_t fileBytes = 3 * 1024 * 1024);
        Result<void> add(std::span<const uint8_t> bytes);
        Result<void> finish();

    private:
        FragmentWriter(std::unique_ptr<BuildDirectory> build, SegmentedFileWriter files, std::string prefix);
        std::unique_ptr<BuildDirectory> build_;
        SegmentedFileWriter files_;
        BinaryBuffer records_;
        uint32_t count_ = 0;
        std::string prefix_;
    };
}
