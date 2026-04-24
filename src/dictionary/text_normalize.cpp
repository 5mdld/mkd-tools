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
}
