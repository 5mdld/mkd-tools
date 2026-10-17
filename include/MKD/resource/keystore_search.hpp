//
// kiwakiwaaにより 2026/04/05 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/search_mode.hpp"

#include <string_view>
#include <stop_token>

namespace MKD
{
    struct KeystoreSearchRange
    {
        size_t begin = 0;
        size_t end = 0;
        KeystoreIndex indexType = KeystoreIndex::Prefix;

        [[nodiscard]] size_t count() const noexcept { return end > begin ? end - begin : 0; }
        [[nodiscard]] bool empty() const noexcept { return begin >= end; }
    };


    struct KeystoreSearchResult
    {
        std::string_view key;
        size_t index = 0;
        std::vector<EntryId> entryIds;

        [[nodiscard]] size_t size() const noexcept { return entryIds.size(); }
        [[nodiscard]] bool empty() const noexcept { return entryIds.empty(); }

        [[nodiscard]] auto begin() const noexcept { return entryIds.begin(); }
        [[nodiscard]] auto end() const noexcept { return entryIds.end(); }
    };

    /**
     * Return matching [begin, end) positions. Exact searches exclude longer keys.
     * Empty queries and absent indexes produce an empty range. Flags are filtered when collecting results.
     */
    [[nodiscard]] Result<KeystoreSearchRange> keystoreSearch(const Keystore& keystore,
                                                             std::string_view query,
                                                             SearchMode mode, std::stop_token stop = {});

    /**
     * Collect full key + entry-id hits for the query.
     */
    [[nodiscard]] Result<std::vector<KeystoreSearchResult>> keystoreSearchResults(const Keystore& keystore,
        std::string_view query,
        SearchMode mode, std::stop_token stop = {});
}
