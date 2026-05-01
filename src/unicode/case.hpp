//
// kiwakiwaaにより 2026/04/24 に作成されました。
//

#pragma once

#include <array>
#include <cstdint>

#include "constants.hpp"
#include "unicode_data.hpp"

namespace MKD::detail::unicode
{
    constexpr auto buildLowercaseTable()
    {
        std::array<char32_t, 65536> table{};
        for (uint32_t i = 0; i < table.size(); ++i)
            table[i] = static_cast<char32_t>(i);

        for (const auto& [upper, lower] : data::kCasePairs)
            table[upper] = lower;

        return table;
    }

    inline constexpr auto kLowercaseTable = buildLowercaseTable();

    [[nodiscard]] constexpr char32_t toLowercase(const char32_t cp)
    {
        return (cp <= 0xFFFF) ? kLowercaseTable[static_cast<uint16_t>(cp)] : cp;
    }
}
