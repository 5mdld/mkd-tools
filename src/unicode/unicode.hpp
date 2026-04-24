#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace MKD::detail::unicode
{
    using Codepoint = char32_t;

    [[nodiscard]] std::optional<size_t> firstInvalidUtf8Offset(std::span<const uint8_t> bytes) noexcept;
    [[nodiscard]] std::optional<size_t> firstInvalidUtf8Offset(std::string_view text) noexcept;

    [[nodiscard]] Codepoint nextCodepoint(std::string_view text, size_t& offset);
    [[nodiscard]] Codepoint previousCodepoint(std::string_view text, size_t& offset);
    [[nodiscard]] size_t codepointCount(std::string_view text);

    [[nodiscard]] std::u32string toUtf32(std::string_view text);
    [[nodiscard]] std::string toUtf8(std::u32string_view text);
    [[nodiscard]] std::string toUtf8(std::u16string_view text);
    void appendUtf8(std::string& output, Codepoint cp);

    [[nodiscard]] Codepoint toLowercase(Codepoint cp) noexcept;
    [[nodiscard]] Codepoint hiraganaToKatakana(Codepoint cp) noexcept;
    [[nodiscard]] Codepoint keywordSearchFold(Codepoint cp) noexcept;
    [[nodiscard]] Codepoint keystoreFold(Codepoint cp) noexcept;

    [[nodiscard]] bool isJapaneseScript(Codepoint cp) noexcept;
    [[nodiscard]] bool containsJapaneseScript(std::string_view text);
}
