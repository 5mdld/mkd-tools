//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#include "MKD/dictionary/paths.hpp"

#include <array>
#include <span>

using namespace std::literals::string_view_literals;

namespace MKD
{
    Result<DictionaryPaths> DictionaryPaths::create(fs::path productRoot)
    {
        auto contentsRoot = productRoot / "Contents";
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
            return existingDir(base / "contents");
        case ResourceType::AppendixEntries:
            return existingDir(base);
        case ResourceType::Audio:
        case ResourceType::AudioLegacy:
            return existingDir(base / "audio");
        case ResourceType::Graphics:
            {
                static constexpr std::array candidates = {"graphics"sv, "img"sv};
                for (const auto name : candidates)
                {
                    auto path = existingDir(base / name);
                    if (!path || *path)
                        return path;
                }
                return std::optional<fs::path>{};
            }
        case ResourceType::Keystores:
            return existingDir(base / "key");
        case ResourceType::Headlines:
            return existingDir(base / "headline");
        case ResourceType::Fonts:
            return existingDir(base / "fonts");
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
