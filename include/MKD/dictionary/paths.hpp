//
// kiwakiwaaにより 2026/01/14 に作成されました。
//

#pragma once

#include "MKD/dictionary/metadata.hpp"
#include "MKD/resource/resource_type.hpp"

#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

namespace MKD
{
    class DictionaryPaths
    {
    public:

        static Result<DictionaryPaths> create(fs::path productRoot);

        [[nodiscard]] std::optional<fs::path> tryResolve(ResourceType type, std::string_view contentDir) const;

        [[nodiscard]] const fs::path& productRoot() const noexcept { return productRoot_; }

    private:

        DictionaryPaths(fs::path productRoot, fs::path contentsRoot);

        static std::optional<fs::path> existingDir(const fs::path& path);

        fs::path productRoot_;
        fs::path contentsRoot_;
    };

}
