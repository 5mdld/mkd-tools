//
// kiwakiwaaにより 2026/03/07 に作成されました。
//

#pragma once


#include <array>
#include <cstdint>

#include "unicode_data.hpp"

namespace MKD::detail::unicode
{
    constexpr auto buildLowercaseTable()
    {
        std::array<char32_t, 65536> t{};
        for (uint32_t i = 0; i < 65536; ++i) t[i] = static_cast<char32_t>(i);
        for (const auto& [u, l] : data::kCasePairs) t[u] = l;
        return t;
    }

    inline constexpr auto kLowercaseTable = buildLowercaseTable();


    /**
     * Maps a Unicode codepoint to its lowercase equivalent using the case map
     *
     * @param cp Unicode codepoint
     * @return Lowercase mapping or original if no mapping
     */
    constexpr char32_t toLowercase(char32_t cp)
    {
        return (cp <= 0xFFFF) ? kLowercaseTable[static_cast<uint16_t>(cp)] : cp;
    }
}
