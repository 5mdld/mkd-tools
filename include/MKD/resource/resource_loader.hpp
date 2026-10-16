//
// kiwakiwaaにより 2026/01/18 に作成されました。
//

#pragma once

#include "MKD/dictionary/paths.hpp"
#include "MKD/resource/font.hpp"
#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/resource_store.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/headline_store.hpp"
#include "MKD/resource/stylesheet.hpp"

#include <variant>

namespace MKD
{
    class ResourceLoader
    {
    public:
        using Audio = std::variant<ResourceStore, NamedResourceStore>;

        struct Stylesheets
        {
            std::optional<Stylesheet> normal;
            std::optional<Stylesheet> nightmode;
        };

        explicit ResourceLoader(const DictionaryPaths& paths) : paths_(paths)
        {
        }

        [[nodiscard]] Result<std::optional<ResourceStore>> loadEntries(std::string_view contentDir,
                                                                       std::string_view dictId) const;
        [[nodiscard]] Result<std::optional<NamedResourceStore>> loadGraphics(std::string_view contentDir) const;
        [[nodiscard]] Result<std::optional<Audio>>
        loadAudio(std::string_view contentDir, std::string_view dictId) const;
        [[nodiscard]] Result<std::vector<AppendixEntryList>> loadAppendixEntryLists(std::string_view contentDir) const;
        [[nodiscard]] Result<std::vector<Keystore>> loadKeystores(std::string_view contentDir) const;
        [[nodiscard]] Result<std::vector<HeadlineStore>> loadHeadlines(std::string_view contentDir) const;
        [[nodiscard]] Result<std::vector<Font>> loadFonts(std::string_view contentDir) const;
        [[nodiscard]] Result<Stylesheets> loadStylesheets(std::string_view contentDir) const;

    private:
        template <typename T>
        Result<std::vector<T>> loadCollection(ResourceType type, std::string_view contentDir,
                                              std::string_view extension, bool directories = false) const;
        const DictionaryPaths& paths_;
    };
}
