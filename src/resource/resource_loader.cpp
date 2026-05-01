//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#include "MKD/resource/resource_loader.hpp"

#include <pugixml.h>

#include <algorithm>
#include <unordered_map>
#include <ranges>

namespace MKD
{
    namespace
    {
        using AppendixTitleMap = std::unordered_map<std::string, std::string>;

        pugi::xml_node nextElementSibling(pugi::xml_node node)
        {
            for (node = node.next_sibling(); node; node = node.next_sibling())
            {
                if (node.type() == pugi::node_element)
                    return node;
            }
            return {};
        }

        std::optional<std::string> plistStringValue(pugi::xml_node dict, std::string_view key)
        {
            for (auto node = dict.first_child(); node; node = node.next_sibling())
            {
                if (node.type() != pugi::node_element || std::string_view(node.name()) != "key")
                    continue;

                if (std::string_view(node.child_value()) != key)
                    continue;

                const auto value = nextElementSibling(node);
                if (value && std::string_view(value.name()) == "string")
                    return value.child_value();

                return std::nullopt;
            }

            return std::nullopt;
        }

        void collectAppendixEntryTitles(pugi::xml_node node, AppendixTitleMap& titles)
        {
            if (node.type() == pugi::node_element && std::string_view(node.name()) == "dict")
            {
                const auto contents = plistStringValue(node, "contents");
                if (contents && contents->ends_with(".entries"))
                {
                    if (auto name = plistStringValue(node, "name"); name && !name->empty())
                        titles.emplace(*contents, std::move(*name));
                }
            }

            for (auto child = node.first_child(); child; child = child.next_sibling())
                collectAppendixEntryTitles(child, titles);
        }

        AppendixTitleMap loadAppendixEntryTitles(const fs::path& productRoot)
        {
            const auto path = productRoot / "Contents" / "Appendix.plist";
            if (!fs::is_regular_file(path))
                return {};

            pugi::xml_document doc;
            const auto pathString = path.string();
            if (!doc.load_file(pathString.c_str()))
                return {};

            AppendixTitleMap titles;
            collectAppendixEntryTitles(doc.document_element(), titles);
            return titles;
        }
    }


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


    std::vector<AppendixEntryList> ResourceLoader::loadAppendixEntryLists(std::string_view contentDir) const
    {
        const auto contentsRoot = paths_.productRoot() / "Contents";
        const auto contentRoot = contentsRoot / contentDir;
        if (!fs::is_directory(contentRoot))
            return {};

        std::vector<fs::path> files;
        std::error_code ec;
        for (fs::recursive_directory_iterator it(contentRoot, ec), end; it != end; it.increment(ec))
        {
            if (ec)
                break;

            if (it->is_regular_file() && it->path().extension() == ".entries")
                files.push_back(it->path());
        }

        std::ranges::sort(files, [](const fs::path& lhs, const fs::path& rhs) {
            return lhs.generic_string() < rhs.generic_string();
        });

        const auto titles = loadAppendixEntryTitles(paths_.productRoot());
        std::vector<AppendixEntryList> lists;
        lists.reserve(files.size());

        for (const auto& file : files)
        {
            std::string title;
            auto relativePath = fs::relative(file, contentsRoot, ec);
            if (ec)
            {
                ec.clear();
                relativePath.clear();
            }

            if (const auto it = titles.find(relativePath.generic_string()); it != titles.end())
                title = it->second;

            if (auto list = AppendixEntryList::load(file, std::move(title)))
                lists.push_back(std::move(*list));
        }

        return lists;
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
