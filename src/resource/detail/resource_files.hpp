//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/platform/mmap_file.hpp"
#include "MKD/resource/retained_span.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace MKD::detail
{
    [[nodiscard]] Result<std::optional<std::filesystem::path>> findResourceFile(
        const std::filesystem::path& directory, std::string_view extension);

    [[nodiscard]] Result<std::vector<std::filesystem::path>> resourcePaths(
        const std::filesystem::path& directory, std::string_view extension,
        bool recursive = false, bool directories = false);

    class ResourceFiles
    {
    public:
        static Result<ResourceFiles> open(const std::filesystem::path& directory, std::string_view extension);
        [[nodiscard]] Result<RetainedSpan> read(size_t sequence, size_t offset, size_t length) const;
        [[nodiscard]] Result<RetainedSpan> readGlobal(size_t offset, size_t length) const;

    private:
        struct File
        {
            uint32_t sequence;
            size_t globalOffset;
            std::shared_ptr<const MappedFile> mapping;
        };

        static Result<uint32_t> sequenceNumber(const std::filesystem::path& path);
        std::vector<File> files_;
    };
}
