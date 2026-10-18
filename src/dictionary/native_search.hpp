//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "unicode/normalization.hpp"
#include "resource/keystore/scope_definitions.hpp"
#include "MKD/dictionary/dictionary_search.hpp"

#include <array>
#include <string_view>

namespace MKD::detail::native::search
{
    enum class Family
    {
        General,
        Kanwa,
        Kogo,
        Complex
    };

    struct ClassDefinition
    {
        std::string_view name;
        Family family = Family::General;
        bool usesModernEntryRange = false;
    };

    inline constexpr ClassDefinition defaultClass{.name = "HMDicSearchLogic"};
    inline constexpr ClassDefinition kanwaClass{.name = "KLKanwaDicSearchLogic", .family = Family::Kanwa};
    inline constexpr std::array classes{
        defaultClass,
        kanwaClass,
        ClassDefinition{.name = "KLKogoDicSearchLogic", .family = Family::Kogo, .usesModernEntryRange = true},
        ClassDefinition{.name = "OZK_Search", .family = Family::Kogo, .usesModernEntryRange = true},
        ClassDefinition{.name = "SHIGOSHU_SearchLogic", .family = Family::Complex},
        ClassDefinition{.name = "KLDicComplexSearchLogic"}
    };

    inline constexpr std::array kanwaScopes{
        ScopeDefinition{SearchScope::Headword, "Oyaji", "headword"},
        ScopeDefinition{SearchScope::Idiom, "Jyukugo", "idiom"},
        ScopeDefinition{SearchScope::Sense, "Gogi", "sense"},
        ScopeDefinition{SearchScope::Example, "Example", "example"},
    };

    struct DelegateDefinition
    {
        std::string_view name;
        unicode::DictionaryKeyNormalizeOption normalization = unicode::DictionaryKeyNormalizeOption::None;
        unicode::DictionaryKeyNormalizeOption modernNormalization =
            unicode::DictionaryKeyNormalizeOption::CollapseSeparators |
            unicode::DictionaryKeyNormalizeOption::FoldSmallKana;
        bool includesSupplementaryHeadwords = false;
        bool foldsKanjiNumbers = false;
        bool pageEntriesFirst = false;
    };

    inline constexpr std::array delegates{
        DelegateDefinition{.name = "DAIJIRIN_Delegate"},
        DelegateDefinition{.name = "DAIJISEN_Delegate", .includesSupplementaryHeadwords = true},
        DelegateDefinition{.name = "NHKACCENT_Delegate", .pageEntriesFirst = true},
        DelegateDefinition{.name = "SMK7_Delegate"},
        DelegateDefinition{.name = "KankenKJ2_Delegate"},
        DelegateDefinition{
            .name = "SHINJIGEN_Delegate",
            .normalization = unicode::DictionaryKeyNormalizeOption::FoldSmallKana
        },
        DelegateDefinition{.name = "KJT_Delegate"},
        DelegateDefinition{.name = "KLKanwaDicProductDelegate"},
        DelegateDefinition{
            .name = "SKOGO_Delegate",
            .modernNormalization = unicode::DictionaryKeyNormalizeOption::CollapseSeparators,
            .foldsKanjiNumbers = true
        },
        DelegateDefinition{.name = "KOGO3_Delegate", .foldsKanjiNumbers = true},
        DelegateDefinition{.name = "OKG11_Delegate"},
        DelegateDefinition{.name = "KLKogoDicProductDelegate"}
    };

    inline constexpr std::array<std::string_view, 20> idiomStopWords{
        "a", "an", "her", "hers", "herself", "him", "himself", "his", "me", "my", "myself",
        "the", "their", "them", "themselves", "they", "you", "your", "yours", "yourself"
    };

    inline constexpr std::array<std::string_view, 10> headlineSkippedWords{
        "", "in", "of", "on", "with", "at", "a", "the", "for", "to"
    };

    struct NormalizationDefinition
    {
        std::string_view name;
        unicode::DictionaryKeyNormalizeOption options = unicode::DictionaryKeyNormalizeOption::None;
    };

    inline constexpr NormalizationDefinition defaultNormalization{.name = "default"};
    inline constexpr std::array normalizations{
        defaultNormalization,
        NormalizationDefinition{
            .name = "ja_keep_last_choon",
            .options = unicode::DictionaryKeyNormalizeOption::KeepTrailingProlongedSoundMark
        },
        NormalizationDefinition{.name = "kogo", .options = unicode::DictionaryKeyNormalizeOption::FoldSmallKana}
    };
}
