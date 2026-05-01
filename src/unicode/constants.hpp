//
// kiwakiwaaにより 2026/04/24 に作成されました。
//

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace MKD::detail::unicode
{
    using Codepoint = char32_t;

    struct CodepointRange
    {
        Codepoint first;
        Codepoint last;

        [[nodiscard]] constexpr bool contains(const Codepoint cp) const noexcept
        {
            return cp >= first && cp <= last;
        }
    };

    inline constexpr Codepoint kAsciiApostrophe = 0x0027; // '
    inline constexpr Codepoint kRightSingleQuote = 0x2019; // ’
    inline constexpr Codepoint kCombiningAcute = 0x0301; // ◌́
    inline constexpr Codepoint kCombiningBreve = 0x0306; // ◌̆
    inline constexpr Codepoint kCombiningKatakanaHiraganaVoicedSoundMark = 0x3099; // ◌゙
    inline constexpr Codepoint kCombiningKatakanaHiraganaSemiVoicedSoundMark = 0x309A; // ◌゚

    inline constexpr Codepoint kUnicodeMax = 0x10FFFF;
    inline constexpr Codepoint kHighSurrogateStart = 0xD800;
    inline constexpr Codepoint kHighSurrogateEnd = 0xDBFF;
    inline constexpr Codepoint kLowSurrogateStart = 0xDC00;
    inline constexpr Codepoint kLowSurrogateEnd = 0xDFFF;
    inline constexpr Codepoint kSurrogateOffset = 0x10000;

    inline constexpr CodepointRange kHiraganaRange{0x3040, 0x309F};
    inline constexpr CodepointRange kHiraganaLetterRange{0x3041, 0x3096};
    inline constexpr CodepointRange kKatakanaRange{0x30A0, 0x30FF};
    inline constexpr CodepointRange kKatakanaLetterRange{0x30A1, 0x30FA};
    inline constexpr Codepoint kHiraganaToKatakanaOffset = 0x60;

    inline constexpr Codepoint kHorizontalTab = 0x0009;
    inline constexpr Codepoint kSpace = 0x0020;
    inline constexpr Codepoint kNoBreakSpace = 0x00A0;
    inline constexpr Codepoint kRightToLeftMark = 0x200F;
    inline constexpr CodepointRange kDictionaryKeyCollapsibleUnicodeSpaceRange{0x2002, 0x200B};

    inline constexpr Codepoint kAsciiHyphenMinus = 0x002D;
    inline constexpr Codepoint kUnicodeHyphen = 0x2010;
    inline constexpr Codepoint kEnDash = 0x2013;
    inline constexpr Codepoint kEmDash = 0x2014;

    inline constexpr Codepoint kFullwidthAsciiOffset = 0xFEE0;
    inline constexpr CodepointRange kFullwidthDigitRange{0xFF10, 0xFF19};
    inline constexpr CodepointRange kFullwidthUppercaseLatinRange{0xFF21, 0xFF3A};
    inline constexpr CodepointRange kFullwidthLowercaseLatinRange{0xFF41, 0xFF5A};
    inline constexpr Codepoint kFullwidthDollarSign = 0xFF04;
    inline constexpr Codepoint kFullwidthAmpersand = 0xFF06;
    inline constexpr Codepoint kFullwidthCommercialAt = 0xFF20;
    inline constexpr Codepoint kFullwidthYenSign = 0xFFE5;

    inline constexpr Codepoint kIdeographicIterationMark = 0x3005;
    inline constexpr Codepoint kHiraganaIterationMark = 0x309D;
    inline constexpr Codepoint kVoicedHiraganaIterationMark = 0x309E;
    inline constexpr Codepoint kKatakanaHiraganaProlongedSoundMark = 0x30FC;
    inline constexpr Codepoint kKatakanaIterationMark = 0x30FD;
    inline constexpr Codepoint kVoicedKatakanaIterationMark = 0x30FE;

    inline constexpr Codepoint kKatakanaU = 0x30A6;
    inline constexpr Codepoint kKatakanaVu = 0x30F4;
    inline constexpr Codepoint kKatakanaWa = 0x30EF;
    inline constexpr Codepoint kKatakanaWi = 0x30F0;
    inline constexpr Codepoint kKatakanaWe = 0x30F1;
    inline constexpr Codepoint kKatakanaWo = 0x30F2;
    inline constexpr Codepoint kKatakanaVa = 0x30F7;
    inline constexpr Codepoint kKatakanaVi = 0x30F8;
    inline constexpr Codepoint kKatakanaVe = 0x30F9;
    inline constexpr Codepoint kKatakanaVo = 0x30FA;

    inline constexpr Codepoint kHiraganaU = 0x3046;
    inline constexpr Codepoint kHiraganaVu = 0x3094;
    inline constexpr Codepoint kCyrillicCapitalI = 0x0418;
    inline constexpr Codepoint kCyrillicSmallI = 0x0438;
    inline constexpr Codepoint kCyrillicCapitalShortI = 0x0419;
    inline constexpr Codepoint kCyrillicSmallShortI = 0x0439;

    inline constexpr CodepointRange kCjkUnifiedIdeographsRange{0x4E00, 0x9FFF};
    inline constexpr CodepointRange kCjkUnifiedIdeographsExtensionARange{0x3400, 0x4DBF};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionBRange{0x20000, 0x2A6DF};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionCRange{0x2A700, 0x2B73F};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionDRange{0x2B740, 0x2B81F};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionERange{0x2B820, 0x2CEAF};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionFRange{0x2CEB0, 0x2EBEF};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionIRange{0x2EBF0, 0x2EE5F};
    inline constexpr CodepointRange kDictionaryKeyCjkCompatibilitySupplementRange{0x2F800, 0x2FA1F};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionGRange{0x30000, 0x3134F};
    inline constexpr CodepointRange kDictionaryKeyCjkExtensionHRange{0x31350, 0x323AF};

    inline constexpr auto kDictionaryKeyIdeographicIterationRepeatRanges = std::array{
        CodepointRange{0x3400, 0x4DB5},
        CodepointRange{0x4E00, 0x9FCC},
        CodepointRange{0x20000, 0x2A6D6},
        CodepointRange{0x2A700, 0x2B734},
        CodepointRange{0x2B740, 0x2B81D}
    };

    inline constexpr std::array<std::string_view, 10> kKanjiNumberChars = {
        "〇", "一", "二", "三", "四", "五", "六", "七", "八", "九",
    };

    [[nodiscard]] constexpr bool isUnicodeScalarValue(const Codepoint cp) noexcept
    {
        return cp <= kUnicodeMax && !(cp >= kHighSurrogateStart && cp <= kLowSurrogateEnd);
    }
}
