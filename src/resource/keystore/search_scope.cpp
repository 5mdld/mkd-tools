//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "MKD/search_scope.hpp"
#include "scope_definitions.hpp"
#include <algorithm>
#include <utility>

namespace MKD
{
    std::optional<SearchScope> searchScopeFromName(std::string_view name) noexcept
    {
        const auto matches = [name](std::string_view candidate)
        {
            return std::ranges::equal(name, candidate, [](unsigned char left, unsigned char right)
            {
                const auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
                return lower(left) == lower(right);
            });
        };
        for (const auto& definition : detail::searchScopes)
            if (matches(definition.name) || matches(definition.filename) || matches(definition.legacyFilename))
                return definition.scope;

        static constexpr std::array aliases{
            std::pair{"Index", SearchScope::Headword}, std::pair{"Vocabulary", SearchScope::Headword},
            std::pair{"Idiom/Phrasal verb", SearchScope::Idiom}, std::pair{"Jyukugo", SearchScope::Idiom},
            std::pair{"Kanyoku", SearchScope::Idiom}, std::pair{"Phrase", SearchScope::Idiom},
            std::pair{"Yakugo", SearchScope::Sense}, std::pair{"Gendai", SearchScope::Modern},
            std::pair{"Oyaji", SearchScope::Kanji}
        };
        for (const auto& [alias, scope] : aliases)
            if (matches(alias)) return scope;
        return std::nullopt;
    }

    std::optional<SearchScope> searchScopeFromFilename(std::string_view filename) noexcept
    {
        if (const auto separator = filename.find_last_of("/\\"); separator != std::string_view::npos)
            filename.remove_prefix(separator + 1);
        constexpr std::string_view extension = ".keystore";
        if (!filename.ends_with(extension)) return std::nullopt;
        filename.remove_suffix(extension.size());
        return searchScopeFromName(filename);
    }
}
