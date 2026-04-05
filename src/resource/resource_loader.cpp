//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#include "MKD/resource/resource_loader.hpp"

#include <algorithm>
#include <ranges>

namespace MKD
{
    ResourceLoader::ResourceLoader(const DictionaryPaths& paths)
        : paths_(paths)
    {
    }


    std::optional<ResourceStore> ResourceLoader::loadEntries(std::string_view contentDir, std::string_view dictId) const
    {
        return tryLoad<ResourceStore>(ResourceType::Contents, contentDir, dictId);
    }


    std::optional<NamedResourceStore> ResourceLoader::loadGraphics(std::string_view contentDir, std::string_view dictId) const
    {
        return tryLoad<NamedResourceStore>(ResourceType::Graphics, contentDir, dictId);
    }


    std::optional<std::variant<ResourceStore, NamedResourceStore>> ResourceLoader::loadAudio(std::string_view contentDir, std::string_view dictId) const
    {
        return tryLoadEither(ResourceType::Audio, contentDir, dictId);
    }


    std::vector<Keystore> ResourceLoader::loadKeystores(std::string_view contentDir, std::string_view dictId) const
    {
        return loadCollection<Keystore>(
            ResourceType::Keystores,
            contentDir,
            [](const auto& e) { return e.path().extension() == ".keystore"; },
            [&](const auto& e) { return Keystore::open(e.path(), std::string(dictId)); }
        );
    }


    std::vector<HeadlineStore> ResourceLoader::loadHeadlines(std::string_view contentDir) const
    {
        return loadCollection<HeadlineStore>(
            ResourceType::Headlines,
            contentDir,
            [](const auto& e) { return e.path().extension() == ".headlinestore"; },
            [&](const auto& e) { return HeadlineStore::load(e.path()); }
        );
    }


    std::vector<Font> ResourceLoader::loadFonts(std::string_view contentDir) const
    {
        return loadCollection<Font>(
            ResourceType::Fonts,
            contentDir,
            [](const auto& e) { return e.is_directory(); },
            [](const auto& e) { return Font::load(e.path()); }
        );
    }

    ResourceLoader::Stylesheets ResourceLoader::loadStylesheets(std::string_view contentDir,
                                                                std::string_view dictId) const
    {
        const auto cssDir = paths_.productRoot() / "Contents" / contentDir;
        if (!fs::is_directory(cssDir))
            return {};

        std::vector<fs::path> baseStyles;
        for (const auto& entry : fs::directory_iterator(cssDir))
        {
            if (!entry.is_regular_file())
                continue;

            const auto& path = entry.path();
            if (path.extension() != ".css")
                continue;

            if (const auto filename = path.filename().string(); filename.ends_with("-nightmode.css"))
                continue;

            // keep only e.g. RGKO12.css, not RGKO12-appendix.css
            if (path.stem().string().contains('-'))
                continue;

            baseStyles.push_back(path);
        }

        if (baseStyles.empty())
            return {};

        std::ranges::sort(baseStyles, [](const fs::path& lhs, const fs::path& rhs) {
            return lhs.filename().string() < rhs.filename().string();
        });

        const auto& basePath = baseStyles.front();
        const auto baseStem = basePath.stem().string();

        Stylesheets stylesheets;
        if (auto stylesheet = Stylesheet::load(basePath))
            stylesheets.normal = std::move(*stylesheet);

        if (const auto nightmodePath = cssDir / (baseStem + "-nightmode.css"); fs::is_regular_file(nightmodePath))
        {
            if (auto nightmode = Stylesheet::load(nightmodePath))
                stylesheets.nightmode = std::move(*nightmode);
        }

        return stylesheets;
    }


    template<Openable T>
    std::optional<T> ResourceLoader::tryLoad(const ResourceType pathType, std::string_view contentDir, std::string_view dictId) const
    {
        const auto path = paths_.tryResolve(pathType, contentDir);
        if (!path)
            return std::nullopt;

        if (!hasResourceFiles(*path))
            return std::nullopt;

        auto result = [&]() {
            if constexpr (requires { T::open(*path, std::string(dictId)); })
                return T::open(*path, std::string(dictId));
            else
                return T::open(*path);
        }();

        return result ? std::optional<T>(std::move(*result)) : std::nullopt;
    }


    std::optional<std::variant<ResourceStore, NamedResourceStore>> ResourceLoader::tryLoadEither(const ResourceType pathType, std::string_view contentDir, std::string_view dictId) const
    {
        if (auto nrsc = tryLoad<NamedResourceStore>(pathType, contentDir, dictId))
            return std::variant<ResourceStore, NamedResourceStore>(std::move(*nrsc));

        if (auto rsc = tryLoad<ResourceStore>(pathType, contentDir, dictId))
            return std::variant<ResourceStore, NamedResourceStore>(std::move(*rsc));

        return std::nullopt;
    }


    template<typename T, typename Predicate, typename Loader>
    std::vector<T> ResourceLoader::loadCollection(const ResourceType type, std::string_view contentDir, Predicate pred, Loader loader) const
    {
        const auto path = paths_.tryResolve(type, contentDir);
        if (!path) return {};

        std::vector<T> results;
        for (const auto& entry : fs::directory_iterator(*path))
        {
            if (!pred(entry)) continue;
            if (auto result = loader(entry))
                results.push_back(std::move(*result));
        }
        return results;
    }


    bool ResourceLoader::hasResourceFiles(const fs::path& path)
    {
        return std::ranges::any_of(
            fs::directory_iterator(path),
            [](const auto& entry) {
                const auto& p = entry.path();
                return p.extension() == ".rsc" || p.extension() == ".nrsc";
            }
        );
    }
}
