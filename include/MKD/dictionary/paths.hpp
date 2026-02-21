//
// kiwakiwaaにより 2026/01/14 に作成されました。
//

#pragma once

#include "metadata.hpp"
#include "MKD/resource/resource_type.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace MKD
{
    class DictionaryPaths
    {
    public:

        static std::expected<DictionaryPaths, std::string> create(const fs::path& rootPath, const DictionaryMetadata& metadata);

        [[nodiscard]] fs::path resolve(ResourceType type) const;
        [[nodiscard]] std::optional<fs::path> tryResolve(ResourceType type) const;
        [[nodiscard]] std::expected<fs::path, std::string> validate(ResourceType type) const;

    private:

        DictionaryPaths(fs::path rootPath, fs::path contentDirectory);

        fs::path rootPath_;
        fs::path contentDirectory_;

    };

}
