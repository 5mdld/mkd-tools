//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "constants.hpp"

namespace MKD::detail::unicode
{
    enum class EastAsianWidth
    {
        Neutral,
        Narrow,
        Ambiguous,
        Halfwidth,
        Wide,
        Fullwidth,
    };

    [[nodiscard]] EastAsianWidth eastAsianWidth(Codepoint codepoint) noexcept;
}
