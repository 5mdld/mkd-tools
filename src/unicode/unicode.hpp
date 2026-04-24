#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "unicode_data.hpp"

namespace MKD::detail::unicode
{
    using Codepoint = char32_t;

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
    constexpr char32_t toLowercase(const char32_t cp)
    {
        return (cp <= 0xFFFF) ? kLowercaseTable[static_cast<uint16_t>(cp)] : cp;
    }

    [[nodiscard]] std::optional<size_t> firstInvalidUtf8Offset(std::span<const uint8_t> bytes) noexcept;
    [[nodiscard]] std::optional<size_t> firstInvalidUtf8Offset(std::string_view text) noexcept;

    [[nodiscard]] Codepoint nextCodepoint(std::string_view text, size_t& offset);
    [[nodiscard]] Codepoint previousCodepoint(std::string_view text, size_t& offset);
    [[nodiscard]] size_t codepointCount(std::string_view text);

    [[nodiscard]] std::u32string toUtf32(std::string_view text);
    [[nodiscard]] std::string toUtf8(std::u32string_view text);
    [[nodiscard]] std::string toUtf8(std::u16string_view text);
    void appendUtf8(std::string& output, Codepoint cp);

    [[nodiscard]] Codepoint hiraganaToKatakana(Codepoint cp) noexcept;
    [[nodiscard]] Codepoint keywordSearchFold(Codepoint cp) noexcept;

    [[nodiscard]] bool isJapaneseScript(Codepoint cp) noexcept;
    [[nodiscard]] bool containsJapaneseScript(std::string_view text);
}
