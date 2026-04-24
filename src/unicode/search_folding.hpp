#pragma once

#include <string_view>

#include "constants.hpp"

namespace MKD::detail::unicode
{
    [[nodiscard]] Codepoint hiraganaToKatakana(Codepoint cp) noexcept;
    [[nodiscard]] Codepoint keywordSearchFold(Codepoint cp) noexcept;

    [[nodiscard]] bool isJapaneseScript(Codepoint cp) noexcept;
    [[nodiscard]] bool containsJapaneseScript(std::string_view text);
}
