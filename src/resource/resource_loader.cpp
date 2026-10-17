//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#include "MKD/resource/resource_loader.hpp"
#include "dictionary/native_format.hpp"
#include "resource/detail/font_format.hpp"
#include "resource/detail/resource_files.hpp"
#include "resource/detail/appendix_metadata.hpp"

#include <algorithm>
#include <format>

namespace MKD
{
    Result<std::optional<ResourceStore>> ResourceLoader::loadEntries(std::string_view contentDir,
                                                                     std::string_view dictId) const
    {
        auto path = paths_.resolve(ResourceType::Contents, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::optional<ResourceStore>{};

        auto store = ResourceStore::open(**path, dictId);
        if (!store)
            return std::unexpected(std::format("Entries '{}': {}", (**path).string(), store.error()));

        return std::optional(std::move(*store));
    }

    Result<std::optional<NamedResourceStore>> ResourceLoader::loadGraphics(std::string_view contentDir) const
    {
        auto path = paths_.resolve(ResourceType::Graphics, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::optional<NamedResourceStore>{};

        auto store = NamedResourceStore::open(**path);
        if (!store)
            return std::unexpected(std::format("Graphics '{}': {}", (**path).string(), store.error()));

        return std::optional(std::move(*store));
    }

    Result<std::optional<ResourceLoader::Audio>> ResourceLoader::loadAudio(
        std::string_view contentDir, std::string_view dictId) const
    {
        auto path = paths_.resolve(ResourceType::Audio, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::optional<Audio>{};

        auto namedIndex = detail::findResourceFile(**path, ".nidx");
        if (!namedIndex)
            return std::unexpected(namedIndex.error());

        if (*namedIndex)
        {
            auto store = NamedResourceStore::open(**path);
            if (!store)
                return std::unexpected(std::format("Audio '{}': {}", (**path).string(), store.error()));

            return std::optional<Audio>(std::move(*store));
        }

        auto store = ResourceStore::open(**path, dictId);
        if (!store)
            return std::unexpected(std::format("Audio '{}': {}", (**path).string(), store.error()));

        return std::optional<Audio>(std::move(*store));
    }

    template <typename T>
    Result<std::vector<T>> ResourceLoader::loadCollection(const ResourceType type, std::string_view contentDir,
                                                          std::string_view extension, bool directories) const
    {
        auto path = paths_.resolve(type, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::vector<T>{};

        auto files = detail::resourcePaths(**path, extension, false, directories);
        if (!files)
            return std::unexpected(files.error());

        std::vector<T> resources;
        resources.reserve(files->size());
        for (const auto& file : *files)
        {
            auto resource = T::open(file);
            if (!resource)
                return std::unexpected(std::format("Resource '{}': {}", file.string(), resource.error()));

            resources.push_back(std::move(*resource));
        }
        return resources;
    }

    Result<std::vector<Keystore>> ResourceLoader::loadKeystores(std::string_view contentDir) const
    {
        return loadCollection<Keystore>(ResourceType::Keystores, contentDir, ".keystore");
    }

    Result<std::vector<HeadlineStore>> ResourceLoader::loadHeadlines(std::string_view contentDir) const
    {
        return loadCollection<HeadlineStore>(ResourceType::Headlines, contentDir, ".headlinestore");
    }

    Result<std::vector<Font>> ResourceLoader::loadFonts(std::string_view contentDir) const
    {
        auto path = paths_.resolve(ResourceType::Fonts, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::vector<Font>{};

        auto directories = detail::resourcePaths(**path, {}, false, true);
        auto files = detail::resourcePaths(**path, {});
        if (!directories)
            return std::unexpected(directories.error());

        if (!files)
            return std::unexpected(files.error());

        for (auto& file : *files)
            if (detail::FontFormat::supportsExtension(file.extension().string()))
                directories->push_back(std::move(file));

        std::ranges::sort(*directories);
        std::vector<Font> fonts;
        for (const auto& file : *directories)
        {
            auto font = Font::open(file);
            if (!font)
                return std::unexpected(font.error());

            fonts.push_back(std::move(*font));
        }
        return fonts;
    }

    Result<std::vector<AppendixEntryList>> ResourceLoader::loadAppendixEntryLists(std::string_view contentDir) const
    {
        auto path = paths_.resolve(ResourceType::AppendixEntries, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return std::vector<AppendixEntryList>{};

        auto files = detail::resourcePaths(**path, ".entries", true);
        if (!files)
            return std::unexpected(files.error());

        auto titles = detail::AppendixMetadata::open(paths_.productRoot());
        if (!titles)
            return std::unexpected(titles.error());

        std::vector<AppendixEntryList> lists;
        for (const auto& file : *files)
        {
            const auto relative = file.
                lexically_relative(paths_.productRoot() / detail::native::paths::productContents);
            const auto title = titles->find(relative.generic_string());
            auto list = AppendixEntryList::open(file, title == titles->end() ? std::string{} : title->second);
            if (!list)
                return std::unexpected(std::format("Appendix '{}': {}", file.string(), list.error()));

            lists.push_back(std::move(*list));
        }
        return lists;
    }

    Result<ResourceLoader::Stylesheets> ResourceLoader::loadStylesheets(std::string_view contentDir) const
    {
        auto path = paths_.resolve(ResourceType::AppendixEntries, contentDir);
        if (!path)
            return std::unexpected(path.error());

        if (!*path)
            return Stylesheets{};

        auto files = detail::resourcePaths(**path, ".css");
        if (!files)
            return std::unexpected(files.error());

        Stylesheets styles;
        const auto base = std::ranges::find_if(*files, [](const auto& file)
        {
            return !file.stem().string().contains('-');
        });

        if (base == files->end())
            return styles;

        auto normal = Stylesheet::open(*base);
        if (!normal)
            return std::unexpected(normal.error());

        styles.normal = std::move(*normal);
        const auto night = base->parent_path() / (base->stem().string() + "-nightmode.css");
        if (std::ranges::find(*files, night) != files->end())
        {
            auto stylesheet = Stylesheet::open(night);
            if (!stylesheet)
                return std::unexpected(stylesheet.error());

            styles.nightmode = std::move(*stylesheet);
        }
        return styles;
    }
}
