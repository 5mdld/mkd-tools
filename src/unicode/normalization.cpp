#include "normalization.hpp"

#include "utf8.hpp"

#include <utf8proc.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace MKD::detail::unicode
{
    namespace
    {
        using Mapping = std::pair<Codepoint, Codepoint>;

        [[nodiscard]] const utf8proc_uint8_t* bytes(const std::string_view text) noexcept
        {
            return reinterpret_cast<const utf8proc_uint8_t*>(text.data());
        }

        [[nodiscard]] utf8proc_option_t utf8procOptions(const NormalizationForm form) noexcept
        {
            switch (form)
            {
                case NormalizationForm::Nfd:
                    return static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_DECOMPOSE);
                case NormalizationForm::Nfc:
                    return static_cast<utf8proc_option_t>(UTF8PROC_STABLE | UTF8PROC_COMPOSE);
            }

            return UTF8PROC_STABLE;
        }

        [[nodiscard]] bool inAnyRange(const Codepoint cp, const std::span<const CodepointRange> ranges) noexcept
        {
            return std::ranges::any_of(ranges, [cp](const CodepointRange range) {
                return range.contains(cp);
            });
        }

        [[nodiscard]] bool isOneOf(const Codepoint cp, const std::span<const Codepoint> values) noexcept
        {
            return std::ranges::find(values, cp) != values.end();
        }

        [[nodiscard]] std::optional<Codepoint> mapFromTable(
            const Codepoint cp,
            const std::span<const Mapping> mappings
        ) noexcept
        {
            const auto it = std::ranges::find_if(mappings, [cp](const Mapping mapping) {
                return mapping.first == cp;
            });

            if (it == mappings.end())
                return std::nullopt;

            return it->second;
        }

        [[nodiscard]] bool isOpeningBracket(const Codepoint cp, Codepoint& closing) noexcept
        {
            switch (cp)
            {
                case U'(':
                    closing = U')';
                    return true;
                case U'[':
                    closing = U']';
                    return true;
                case U'{':
                    closing = U'}';
                    return true;
                case 0xFF08: // （
                    closing = 0xFF09;
                    return true;
                case 0xFF3B: // ［
                    closing = 0xFF3D;
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] bool isCollapsibleWhitespace(const Codepoint cp) noexcept
        {
            return cp == kHorizontalTab
                   || cp == kSpace
                   || cp == kNoBreakSpace
                   || cp == kRightToLeftMark
                   || kDictionaryKeyCollapsibleUnicodeSpaceRange.contains(cp);
        }

        [[nodiscard]] bool isCollapsibleHyphen(const Codepoint cp) noexcept
        {
            return cp == kAsciiHyphenMinus || cp == kUnicodeHyphen || cp == kEnDash || cp == kEmDash;
        }

        void appendCollapsed(std::u32string& output, const Codepoint cp)
        {
            if (output.empty() || output.back() != cp)
                output.push_back(cp);
        }

        [[nodiscard]] std::optional<std::u32string_view> latinLigatureExpansion(const Codepoint cp) noexcept
        {
            using namespace std::string_view_literals;

            switch (cp)
            {
                case 0x00C6: // Æ
                    return U"AE"sv;
                case 0x00DF: // ß
                    return U"ss"sv;
                case 0x00E6: // æ
                    return U"ae"sv;
                case 0x0152: // Œ
                    return U"OE"sv;
                case 0x0153: // œ
                    return U"oe"sv;
                default:
                    return std::nullopt;
            }
        }

        [[nodiscard]] bool isIdeographicIterationRepeatBase(const Codepoint cp) noexcept
        {
            return inAnyRange(cp, kDictionaryKeyIdeographicIterationRepeatRanges);
        }

        [[nodiscard]] bool isDakutenIncrementBaseHiragana(const Codepoint cp) noexcept
        {
            switch (cp)
            {
                case 0x304B:
                case 0x304D:
                case 0x304F:
                case 0x3051:
                case 0x3053:
                case 0x3055:
                case 0x3057:
                case 0x3059:
                case 0x305B:
                case 0x305D:
                case 0x305F:
                case 0x3061:
                case 0x3064:
                case 0x3066:
                case 0x3068:
                case 0x306F:
                case 0x3072:
                case 0x3075:
                case 0x3078:
                case 0x307B:
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] bool isDakutenIncrementBaseKatakana(const Codepoint cp) noexcept
        {
            switch (cp)
            {
                case 0x30AB:
                case 0x30AD:
                case 0x30AF:
                case 0x30B1:
                case 0x30B3:
                case 0x30B5:
                case 0x30B7:
                case 0x30B9:
                case 0x30BB:
                case 0x30BD:
                case 0x30BF:
                case 0x30C1:
                case 0x30C4:
                case 0x30C6:
                case 0x30C8:
                case 0x30CF:
                case 0x30D2:
                case 0x30D5:
                case 0x30D8:
                case 0x30DB:
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] bool isSemiVoicedBaseKatakana(const Codepoint cp) noexcept
        {
            switch (cp)
            {
                case 0x30CF: // ハ
                case 0x30D2: // ヒ
                case 0x30D5: // フ
                case 0x30D8: // ヘ
                case 0x30DB: // ホ
                    return true;
                default:
                    return false;
            }
        }

        [[nodiscard]] Codepoint applyDakutenToHiragana(const Codepoint cp) noexcept
        {
            if (isDakutenIncrementBaseHiragana(cp))
                return cp + 1;

            if (cp == kHiraganaU)
                return kHiraganaVu;

            return cp;
        }

        [[nodiscard]] Codepoint applyDakutenToKatakana(const Codepoint cp) noexcept
        {
            if (isDakutenIncrementBaseKatakana(cp))
                return cp + 1;

            switch (cp)
            {
                case kKatakanaU:
                    return kKatakanaVu;
                case kKatakanaWa:
                    return kKatakanaVa;
                case kKatakanaWi:
                    return kKatakanaVi;
                case kKatakanaWe:
                    return kKatakanaVe;
                case kKatakanaWo:
                    return kKatakanaVo;
                default:
                    return cp;
            }
        }

        [[nodiscard]] Codepoint applySemiVoicedMarkToKatakana(const Codepoint cp) noexcept
        {
            if (isSemiVoicedBaseKatakana(cp))
                return cp + 2;

            return cp;
        }

        [[nodiscard]] Codepoint foldSmallKana(const Codepoint cp) noexcept
        {
            switch (cp)
            {
                case 0x30A1: // ァ
                case 0x30A3: // ィ
                case 0x30A5: // ゥ
                case 0x30A7: // ェ
                case 0x30A9: // ォ
                case 0x30C3: // ッ
                case 0x30E3: // ャ
                case 0x30E5: // ュ
                case 0x30E7: // ョ
                case 0x30EE: // ヮ
                    return cp + 1;
                default:
                    return cp;
            }
        }

        [[nodiscard]] std::optional<Codepoint> fullwidthAsciiMapping(const Codepoint cp) noexcept
        {
            if (kFullwidthDigitRange.contains(cp)
                || kFullwidthUppercaseLatinRange.contains(cp)
                || kFullwidthLowercaseLatinRange.contains(cp))
            {
                return cp - kFullwidthAsciiOffset;
            }

            switch (cp)
            {
                case kFullwidthDollarSign:
                    return U'$';
                case kFullwidthAmpersand:
                    return U'&';
                case kFullwidthCommercialAt:
                    return U'@';
                case kFullwidthYenSign:
                    return 0x00A5;
                default:
                    return std::nullopt;
            }
        }

        [[nodiscard]] std::optional<Codepoint> kanaBaseMapping(
            const std::span<const Codepoint> input,
            const size_t index,
            const DictionaryKeyNormalizeOption options
        ) noexcept
        {
            const Codepoint cp = input[index];
            Codepoint mapped = cp;

            if (kHiraganaLetterRange.contains(cp))
                mapped += kHiraganaToKatakanaOffset;
            else if (!kKatakanaLetterRange.contains(cp))
                return std::nullopt;

            if (hasOption(options, DictionaryKeyNormalizeOption::FoldSmallKana))
                mapped = foldSmallKana(mapped);

            if (index + 1 < input.size())
            {
                const Codepoint next = input[index + 1];
                if (next == kCombiningKatakanaHiraganaSemiVoicedSoundMark)
                    mapped = applySemiVoicedMarkToKatakana(mapped);
                else if (next == kCombiningKatakanaHiraganaVoicedSoundMark)
                    mapped = applyDakutenToKatakana(mapped);
            }

            return mapped;
        }

        [[nodiscard]] std::optional<Codepoint> kanaIterationMapping(
            const Codepoint mark,
            const std::u32string& output
        ) noexcept
        {
            if (output.empty())
                return std::nullopt;

            const Codepoint previous = output.back();
            if (mark == kHiraganaIterationMark || mark == kVoicedHiraganaIterationMark)
            {
                if (!kHiraganaLetterRange.contains(previous))
                    return std::nullopt;

                if (mark == kVoicedHiraganaIterationMark)
                    return applyDakutenToHiragana(previous);

                return previous;
            }

            if (!kKatakanaLetterRange.contains(previous))
                return std::nullopt;

            if (mark == kVoicedKatakanaIterationMark)
                return applyDakutenToKatakana(previous);

            return previous;
        }

        [[nodiscard]] std::optional<Codepoint> cyrillicShortIMapping(
            const std::span<const Codepoint> input,
            const size_t index
        ) noexcept
        {
            const Codepoint cp = input[index];
            if (cp != kCyrillicCapitalI && cp != kCyrillicSmallI)
                return std::nullopt;

            if (index + 1 >= input.size() || input[index + 1] != kCombiningBreve)
                return cp;

            return cp == kCyrillicCapitalI ? kCyrillicCapitalShortI : kCyrillicSmallShortI;
        }

        inline constexpr auto kDictionaryKeyCharacterRanges = std::array{
            CodepointRange{U'0', U'9'},
            CodepointRange{U'A', U'Z'},
            CodepointRange{U'a', U'z'},
            CodepointRange{0x0152, 0x0153},
            CodepointRange{0x0370, 0x052F},
            CodepointRange{0x3041, 0x309F},
            CodepointRange{0x30A1, 0x30F4},
            kCjkUnifiedIdeographsExtensionARange,
            kCjkUnifiedIdeographsRange,
            CodepointRange{0xF900, 0xFAFF},
            kFullwidthUppercaseLatinRange,
            kFullwidthLowercaseLatinRange,
            kDictionaryKeyCjkExtensionBRange,
            kDictionaryKeyCjkExtensionCRange,
            kDictionaryKeyCjkExtensionDRange,
            kDictionaryKeyCjkExtensionERange,
            kDictionaryKeyCjkExtensionFRange,
            kDictionaryKeyCjkExtensionIRange,
            kDictionaryKeyCjkCompatibilitySupplementRange,
            kDictionaryKeyCjkExtensionGRange,
            kDictionaryKeyCjkExtensionHRange,
            CodepointRange{0x1100, 0x11FF},
            CodepointRange{0x3130, 0x318F},
            CodepointRange{0xAC00, 0xD7AF},
            CodepointRange{0xA960, 0xA97F},
            CodepointRange{0xD7B0, 0xD7FF},
            CodepointRange{0x0E01, 0x0E5B}
        };

        inline constexpr auto kDictionaryKeySingleCharacters = std::array<Codepoint, 22>{
            0x00C6, // Æ
            0x00DF, // ß
            0x00E6, // æ
            kKatakanaHiraganaProlongedSoundMark,
            U'&',
            U'_',
            U'@',
            U'#',
            U'$',
            U'~',
            0x00A2, // ¢
            0x00A3, // £
            0x00A5, // ¥
            0x20AC, // €
            0x3005, // 々
            0x3006, // 〆
            0x3007, // 〇
            0x3013, // 〓
            0x00A9, // ©
            0x00AE, // ®
            kAsciiApostrophe,
            kRightSingleQuote
        };

        inline constexpr auto kCjkCompatibilityMappings = std::array<Mapping, 10>{
            Mapping{0x5036, 0x4FF1}, // 倶 -> 俱
            Mapping{0x5265, 0x525D}, // 剥 -> 剝
            Mapping{0x53F1, 0x20B9F}, // 叱 -> 𠮟
            Mapping{0x5451, 0x541E}, // 呑 -> 吞
            Mapping{0x5618, 0x5653}, // 嘘 -> 噓
            Mapping{0x598D, 0x59F8}, // 妍 -> 姸
            Mapping{0x5C4F, 0x5C5B}, // 屏 -> 屛
            Mapping{0x5E76, 0x5E77}, // 并 -> 幷
            Mapping{0x75E9, 0x7626}, // 痩 -> 瘦
            Mapping{0x7E4B, 0x7E6B}, // 繋 -> 繫
        };

        inline constexpr auto kSpecialCompatibilityMappings = std::array<Mapping, 11>{
            Mapping{0x00A9, U'C'}, // ©
            Mapping{0x00AE, U'R'}, // ®
            Mapping{0x00D8, U'O'}, // Ø
            Mapping{0x00F8, U'o'}, // ø
            Mapping{0x0127, U'h'}, // ħ
            Mapping{0x0131, U'i'}, // ı
            Mapping{0x0141, U'L'}, // Ł
            Mapping{0x0142, U'l'}, // ł
            Mapping{0x20A6, U'N'}, // ₦
            Mapping{0x210F, U'h'}, // ℏ
            Mapping{0x24DA, U'k'} // ⓚ
        };
    }


    std::string normalizeUtf8(const std::string_view text, const NormalizationForm form)
    {
        if (text.empty())
            return {};

        utf8proc_uint8_t* mapped = nullptr;
        const auto length = utf8proc_map(
            bytes(text),
            static_cast<utf8proc_ssize_t>(text.size()),
            &mapped,
            utf8procOptions(form)
        );

        if (length < 0)
            throw std::runtime_error(std::format("UTF-8 normalization failed: {}", utf8proc_errmsg(length)));

        std::unique_ptr<utf8proc_uint8_t, decltype(&std::free)> guard(mapped, &std::free);
        return {
            reinterpret_cast<const char*>(guard.get()),
            static_cast<size_t>(length)
        };
    }


    std::string canonicalDecompose(const std::string_view text)
    {
        return normalizeUtf8(text, NormalizationForm::Nfd);
    }


    std::string canonicalCompose(const std::string_view text)
    {
        return normalizeUtf8(text, NormalizationForm::Nfc);
    }


    bool isDictionaryKeyCharacter(const Codepoint cp) noexcept
    {
        return inAnyRange(cp, kDictionaryKeyCharacterRanges)
               || isOneOf(cp, kDictionaryKeySingleCharacters);
    }


    std::string normalizeDictionaryKey(
        const std::string_view text,
        const DictionaryKeyNormalizeOption options
    )
    {
        if (text.empty())
            return {};

        const auto decomposed = canonicalDecompose(text);
        const auto inputText = toUtf32(decomposed);
        const std::span<const Codepoint> input{inputText.data(), inputText.size()};

        std::u32string output;
        output.reserve(input.size());

        for (size_t i = 0; i < input.size(); ++i)
        {
            const Codepoint cp = input[i];

            if (cp > 0xFFFF)
            {
                if (isDictionaryKeyCharacter(cp))
                    output.push_back(cp);
                continue;
            }

            if (Codepoint closingBracket = 0; isOpeningBracket(cp, closingBracket))
            {
                if (i + 1 < input.size())
                {
                    const auto begin = input.begin() + static_cast<std::ptrdiff_t>(i + 1);
                    const auto closing = std::ranges::find(begin, input.end(), closingBracket);
                    if (closing == input.end())
                        break;

                    i = static_cast<size_t>(closing - input.begin());
                }
                continue;
            }

            if (isCollapsibleWhitespace(cp))
            {
                if (hasOption(options, DictionaryKeyNormalizeOption::CollapseSeparators))
                    appendCollapsed(output, kSpace);
                continue;
            }

            if (const auto expansion = latinLigatureExpansion(cp))
            {
                if (hasOption(options, DictionaryKeyNormalizeOption::ExpandLatinLigatures))
                    output.append(expansion->begin(), expansion->end());
                else
                    output.push_back(cp);
                continue;
            }

            if (isCollapsibleHyphen(cp))
            {
                if (hasOption(options, DictionaryKeyNormalizeOption::CollapseSeparators))
                    appendCollapsed(output, kAsciiHyphenMinus);
                continue;
            }

            if (cp == kIdeographicIterationMark)
            {
                const Codepoint previous = output.empty() ? 0 : output.back();
                output.push_back(isIdeographicIterationRepeatBase(previous) ? previous : cp);
                continue;
            }

            if (cp == kKatakanaHiraganaProlongedSoundMark)
            {
                if (hasOption(options, DictionaryKeyNormalizeOption::KeepTrailingProlongedSoundMark)
                    || i + 1 < input.size())
                {
                    output.push_back(cp);
                }
                continue;
            }

            if (cp == kHiraganaIterationMark
                || cp == kVoicedHiraganaIterationMark
                || cp == kKatakanaIterationMark
                || cp == kVoicedKatakanaIterationMark)
            {
                output.push_back(kanaIterationMapping(cp, output).value_or(cp));
                continue;
            }

            if (const auto mapped = mapFromTable(cp, kCjkCompatibilityMappings))
            {
                output.push_back(*mapped);
                continue;
            }

            if (const auto mapped = fullwidthAsciiMapping(cp))
            {
                output.push_back(*mapped);
                continue;
            }

            if (const auto mapped = kanaBaseMapping(input, i, options))
            {
                output.push_back(*mapped);
                continue;
            }

            if (cp == kCombiningKatakanaHiraganaVoicedSoundMark
                || cp == kCombiningKatakanaHiraganaSemiVoicedSoundMark)
            {
                continue;
            }

            if (cp == kAsciiApostrophe || cp == kRightSingleQuote)
            {
                if (hasOption(options, DictionaryKeyNormalizeOption::KeepApostrophe))
                    output.push_back(kAsciiApostrophe);
                continue;
            }

            if (const auto mapped = mapFromTable(cp, kSpecialCompatibilityMappings))
            {
                output.push_back(*mapped);
                continue;
            }

            if (const auto mapped = cyrillicShortIMapping(input, i))
            {
                output.push_back(*mapped);
                continue;
            }

            if (isDictionaryKeyCharacter(cp))
                output.push_back(cp);
        }

        auto result = toUtf8(output);
        if (hasOption(options, DictionaryKeyNormalizeOption::Compose))
            result = canonicalCompose(result);

        return result;
    }
}
