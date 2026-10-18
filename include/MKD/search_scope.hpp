//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace MKD
{
    enum class SearchScope : uint8_t
    {
        Headword = 0,
        Idiom = 1,
        Example = 2,
        English = 3,
        Sense = 4,
        Modern = 5,
        Kanji = 6,
        Collocation = 7,
        CJ = 8,
        JC = 9,
        Fulltext = 10,
        Category = 11,
        CompoundNoun = 100,
        Numeral = 101
    };

    /** Accepts native metadata names and index stems, including profile-specific aliases. */
    [[nodiscard]] std::optional<SearchScope> searchScopeFromName(std::string_view name) noexcept;
    [[nodiscard]] std::optional<SearchScope> searchScopeFromFilename(std::string_view filename) noexcept;
}
