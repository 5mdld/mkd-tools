//
// Public keystore search API.
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/search_mode.hpp"

#include <string_view>

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
     * Search a keystore index and return matching [begin, end) positions.
     */
    [[nodiscard]] Result<KeystoreSearchRange> keystoreSearch(const Keystore& keystore,
                                                             std::string_view query,
                                                             SearchMode mode);

    /**
     * Collect full key + entry-id hits for the query.
     */
    [[nodiscard]] Result<std::vector<KeystoreSearchResult>> keystoreSearchResults(const Keystore& keystore,
                                                                                   std::string_view query,
                                                                                   SearchMode mode);
}
