//
// kiwakiwaaにより 2026/04/24 に作成されました。
//

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "constants.hpp"

namespace MKD::detail::unicode
{
    enum class NormalizationForm : uint8_t
    {
        Nfd,
        Nfc
    };

    enum class DictionaryKeyNormalizeOption : uint8_t
    {
        None = 0,
        CollapseSeparators = 1 << 0,
        KeepApostrophe = 1 << 1,
        FoldSmallKana = 1 << 2,
        ExpandLatinLigatures = 1 << 3,
        Compose = 1 << 4,
        KeepTrailingProlongedSoundMark = 1 << 5
    };

    [[nodiscard]] constexpr DictionaryKeyNormalizeOption operator|(
        const DictionaryKeyNormalizeOption lhs,
        const DictionaryKeyNormalizeOption rhs
    ) noexcept
    {
        return static_cast<DictionaryKeyNormalizeOption>(
            static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
        );
    }

    constexpr DictionaryKeyNormalizeOption& operator|=(
        DictionaryKeyNormalizeOption& lhs,
        const DictionaryKeyNormalizeOption rhs
    ) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    [[nodiscard]] constexpr bool hasOption(
        const DictionaryKeyNormalizeOption options,
        const DictionaryKeyNormalizeOption option
    ) noexcept
    {
        return (static_cast<uint8_t>(options) & static_cast<uint8_t>(option)) != 0;
    }

    [[nodiscard]] std::string normalizeUtf8(std::string_view text, NormalizationForm form);
    [[nodiscard]] std::string canonicalDecompose(std::string_view text);
    [[nodiscard]] std::string canonicalCompose(std::string_view text);

    [[nodiscard]] bool isDictionaryKeyCharacter(Codepoint cp) noexcept;

    [[nodiscard]] std::string normalizeDictionaryKey(
        std::string_view text,
        DictionaryKeyNormalizeOption options = DictionaryKeyNormalizeOption::None
    );

    [[nodiscard]] std::string normalizeKanjiNumberString(std::string_view text);
}
