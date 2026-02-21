//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#include "MKD/dictionary/paths.hpp"

#include <format>

using namespace std::literals::string_view_literals;

namespace MKD
{

    namespace detail
    {
        std::optional<fs::path> findResourcePath(
            const fs::path& contentDir,
            std::span<const std::string_view> candidateNames)
        {
            for (const auto& name : candidateNames)
            {
                if (fs::path path = contentDir / name; fs::exists(path / "index.nidx"))
                    return std::move(path);
            }
            return std::nullopt;
        }
    }

    std::expected<DictionaryPaths, std::string> DictionaryPaths::create(const fs::path& rootPath,
                                                                        const DictionaryMetadata& metadata)
    {
        const auto contentDirectoryName = metadata.contentDirectoryName();
        if (!contentDirectoryName)
            return std::unexpected(std::format("Failed to get content directory for '{}'", rootPath.stem().string()));

        return DictionaryPaths(rootPath, std::move(rootPath / "Contents" / contentDirectoryName.value()));
    }


    DictionaryPaths::DictionaryPaths(fs::path rootPath, fs::path contentDirectory)
        : rootPath_(std::move(rootPath)), contentDirectory_(std::move(contentDirectory))
    {
    }

    fs::path DictionaryPaths::resolve(const ResourceType type) const
    {
        switch (type)
        {
            case ResourceType::Audio: return contentDirectory_ / "audio";
            case ResourceType::Entries: return contentDirectory_ / "contents";
            case ResourceType::Graphics:
            {
                static constexpr std::array candidates = {"graphics"sv, "img"sv};
                if (auto path = detail::findResourcePath(contentDirectory_, candidates))
                    return *path;
                return contentDirectory_ / "graphics";
            }
            case ResourceType::Fonts: return contentDirectory_ / "fonts";
            case ResourceType::Headlines: return contentDirectory_ / "headline";
            case ResourceType::Keystores: return contentDirectory_ / "key";
            default: return rootPath_;
        }
    }


    std::optional<fs::path> DictionaryPaths::tryResolve(const ResourceType type) const
    {
        auto path = resolve(type);
        return fs::exists(path) ? std::make_optional(path) : std::nullopt;
    }


    std::expected<fs::path, std::string> DictionaryPaths::validate(const ResourceType type) const
    {
        auto path = resolve(type);
        if (!fs::exists(path))
        {
            return std::unexpected(std::format("Path '{}' does not exist", path.string()));
        }
        return path;
    }


}
