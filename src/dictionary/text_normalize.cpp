//
// kiwakiwaaにより 2026/03/13 に作成されました。
//

#include "text_normalize.hpp"
#include "resource/detail/unicode/unicode_case_map.hpp"

#include "utf8.h"

#include <algorithm>

namespace MKD
{
    namespace
    {
        constexpr char32_t HIRAGANA_START = 0x3041;
        constexpr char32_t HIRAGANA_END   = 0x3096;
        constexpr char32_t HIRAGANA_TO_KATAKANA_OFFSET = 0x60;

        constexpr char32_t ASCII_APOSTROPHE   = 0x0027;
        constexpr char32_t RIGHT_SINGLE_QUOTE = 0x2019;
        constexpr char32_t COMBINING_ACUTE    = 0x0301;

        char32_t normalizeCodepoint(const char32_t cp)
        {
            if (cp == ASCII_APOSTROPHE || cp == RIGHT_SINGLE_QUOTE)
                return COMBINING_ACUTE;

            if (cp >= HIRAGANA_START && cp <= HIRAGANA_END)
                return cp + HIRAGANA_TO_KATAKANA_OFFSET;

            // ascii case fold
            if (cp >= 0x41 && cp <= 0x5A)
                return cp | 0x20;

            if (cp < 0x80)
                return cp;

            return detail::unicode::toLowercase(cp);
        }
    }


    std::u32string normalizeForKeywordSearch(std::string_view text)
    {
        std::u32string result;
        auto it = text.begin();
        const auto end = text.end();

        while (it != end)
        {
            const char32_t cp = utf8::next(it, end);
            result.push_back(normalizeCodepoint(cp));
        }

        return result;
    }


    bool normalizedContains(std::string_view haystack, std::string_view needle)
    {
        const auto normHay = normalizeForKeywordSearch(haystack);
        const auto normNeedle = normalizeForKeywordSearch(needle);

        if (normNeedle.empty())
            return true;

        if (normNeedle.size() > normHay.size())
            return false;

        return std::ranges::search(normHay, normNeedle).begin() != normHay.end();
    }

    /* TODO: app uses NFD decomposing and...
    * - remove bracketed text () [] {}
    * - normalize whitespace and hyphen sequences
    * - collapse duplicate separators
    * - normalize kana (hiragana ↔ katakana adjustments and dakuten/handakuten handling)
    * - normalize fullwidth/halfwidth characters
    * - perform several compatibility mappings (e.g., ligatures, special Latin characters)
    * -  normalize iteration mark 々 depending on previous character
    */
    std::string normalizeSearchQuery(std::string_view query)
    {
        std::string result;
        auto it = query.begin();
        const auto end = query.end();

        while (it != end)
        {
            const char32_t cp = utf8::next(it, end);
            char32_t out = cp;

            if (cp >= HIRAGANA_START && cp <= HIRAGANA_END)
                out = cp + HIRAGANA_TO_KATAKANA_OFFSET;

            utf8::append(out, std::back_inserter(result));
        }

        return result;
    }
}
