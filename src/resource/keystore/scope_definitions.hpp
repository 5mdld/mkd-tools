//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/search_scope.hpp"
#include <array>

namespace MKD::detail
{
    struct ScopeDefinition
    {
        SearchScope scope;
        std::string_view name;
        std::string_view filename;
        std::string_view legacyFilename;
        bool generic = true;
    };

    inline constexpr std::array searchScopes{
        ScopeDefinition{SearchScope::Headword, "Headword", "headword", "headword", true},
        ScopeDefinition{SearchScope::Idiom, "Idiom", "idiom", "idiom", true},
        ScopeDefinition{SearchScope::Example, "Example", "example", "example", true},
        ScopeDefinition{SearchScope::English, "English", "english", "english", true},
        ScopeDefinition{SearchScope::Sense, "Gogi", "sense", "gogi", true},
        ScopeDefinition{SearchScope::Modern, "Modern", "metadata", "metadata", false},
        ScopeDefinition{SearchScope::Kanji, "Kanji", "kanji", "kanji", true},
        ScopeDefinition{SearchScope::Collocation, "Collocation", "collocation", "collocation", true},
        ScopeDefinition{SearchScope::CJ, "CJ", "cj", "cj", false},
        ScopeDefinition{SearchScope::JC, "JC", "jc", "jc", false},
        ScopeDefinition{SearchScope::Fulltext, "Full-Text", "fulltext", "fulltext", true},
        ScopeDefinition{SearchScope::Category, "Group", "category", "group", true},
        ScopeDefinition{SearchScope::CompoundNoun, "Compound Noun", "compound", "compound_noun", true},
        ScopeDefinition{SearchScope::Numeral, "Numeral", "numeral", "numeral", true}
    };
}
