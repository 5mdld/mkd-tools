//
// kiwakiwaaにより 2026/01/14 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/resource_type.hpp"

#include <filesystem>
#include <optional>
#include <string_view>

namespace fs = std::filesystem;

namespace MKD
{
    class DictionaryPaths
    {
    public:
        static Result<DictionaryPaths> create(fs::path productRoot);

        [[nodiscard]] Result<std::optional<fs::path>> resolve(ResourceType type, std::string_view contentDir) const;

        [[nodiscard]] const fs::path& productRoot() const noexcept { return productRoot_; }

    private:
        DictionaryPaths(fs::path productRoot, fs::path contentsRoot);

        static Result<std::optional<fs::path>> existingDir(const fs::path& path);

        fs::path productRoot_;
        fs::path contentsRoot_;
    };
}
