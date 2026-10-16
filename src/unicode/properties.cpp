//
// kiwakiwaaにより 2026/10/16 に作成されました。
//


#include "properties.hpp"
#include "east_asian_width_data.hpp"

#include <algorithm>

namespace MKD::detail::unicode
{
    EastAsianWidth eastAsianWidth(const Codepoint codepoint) noexcept
    {
        const auto entry = std::ranges::lower_bound(data::kEastAsianWidths, codepoint, {},
                                                    [](const auto& value) { return value.codepoints.last; });

        if (entry != data::kEastAsianWidths.end() && entry->codepoints.contains(codepoint))
            return entry->property;
        return EastAsianWidth::Neutral;
    }
}
