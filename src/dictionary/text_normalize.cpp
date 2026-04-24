//
// kiwakiwaaにより 2026/03/13 に作成されました。
//

#include "text_normalize.hpp"
#include "unicode/unicode.hpp"

#include <algorithm>

namespace MKD
{
    namespace
    {
        char32_t normalizeCodepoint(const char32_t cp)
        {
            return detail::unicode::keywordSearchFold(cp);
        }
    }


    std::u32string normalizeForKeywordSearch(std::string_view text)
    {
        std::u32string result;
        size_t offset = 0;

        while (offset < text.size())
        {
            const char32_t cp = detail::unicode::nextCodepoint(text, offset);
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
        size_t offset = 0;

        while (offset < query.size())
        {
            const char32_t cp = detail::unicode::nextCodepoint(query, offset);
            detail::unicode::appendUtf8(result, detail::unicode::hiraganaToKatakana(cp));
        }

        return result;
    }
}
