//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#include "MKD/dictionary/paths.hpp"
#include "native_format.hpp"

#include <array>
#include <span>

namespace MKD
{
    Result<DictionaryPaths> DictionaryPaths::create(fs::path productRoot)
    {
        auto contentsRoot = productRoot / detail::native::paths::productContents;
        if (!fs::is_directory(contentsRoot))
            return std::unexpected("Missing Contents directory: " + contentsRoot.string());

        return DictionaryPaths(std::move(productRoot), std::move(contentsRoot));
    }


    DictionaryPaths::DictionaryPaths(fs::path productRoot, fs::path contentsRoot)
        : productRoot_(std::move(productRoot))
          , contentsRoot_(std::move(contentsRoot))
    {
    }


    Result<std::optional<fs::path>> DictionaryPaths::resolve(
        const ResourceType type, std::string_view contentDir) const
    {
        const auto base = contentsRoot_ / contentDir;

        switch (type)
        {
        case ResourceType::Contents:
            return existingDir(base / detail::native::paths::contents);
        case ResourceType::AppendixEntries:
            return existingDir(base);
        case ResourceType::Audio:
        case ResourceType::AudioLegacy:
            return existingDir(base / detail::native::paths::audio);
        case ResourceType::Graphics:
            {
                static constexpr std::array candidates{
                    std::string_view(detail::native::paths::graphics),
                    std::string_view(detail::native::paths::legacyGraphics)
                };
                for (const auto name : candidates)
                {
                    auto path = existingDir(base / name);
                    if (!path || *path)
                        return path;
                }
                return std::optional<fs::path>{};
            }
        case ResourceType::Keystores:
            return existingDir(base / detail::native::paths::keys);
        case ResourceType::Headlines:
            return existingDir(base / detail::native::paths::headlines);
        case ResourceType::Fonts:
            return existingDir(base / detail::native::paths::fonts);
        }
        std::unreachable();
    }


    Result<std::optional<fs::path>> DictionaryPaths::existingDir(const fs::path& path)
    {
        std::error_code ec;
        const auto status = fs::status(path, ec);
        if (ec == std::errc::no_such_file_or_directory)
            return std::optional<fs::path>{};
        if (ec)
            return std::unexpected("Cannot inspect '" + path.string() + "': " + ec.message());
        if (!fs::exists(status))
            return std::optional<fs::path>{};
        if (!fs::is_directory(status))
            return std::unexpected("Resource path is not a directory: " + path.string());
        return std::optional(path);
    }
}
