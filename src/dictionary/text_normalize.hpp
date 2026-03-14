//
// kiwakiwaaにより 2026/03/13 に作成されました。
//

#pragma once

#include <string>
#include <string_view>

namespace MKD
{
    /**
     * Normalise text for keyword search matching
     * - Hiragana → Katakana
     * - ASCII apostrophe and right single quote → combining acute accent
     * - Uppercase → lowercase
     * @param text text to normalise
     * @return normalised UTF-32 string
     */
    std::u32string normalizeForKeywordSearch(std::string_view text);

    // Check whether 'haystack' contains 'needle' after keyword normalization.
    bool normalizedContains(std::string_view haystack, std::string_view needle);

    /**
     * Normalise a search query for keystore lookup.
     * Converts Hiragana to Katakana so queries match katakana-stored keys.
     * @param query UTF-8 search query
     * @return normalised UTF-8 string
     */
    std::string normalizeSearchQuery(std::string_view query);
}
