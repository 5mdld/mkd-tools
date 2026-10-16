//
// kiwakiwaaにより 2026/03/10 に作成されました。
//

#include "MKD/resource/keystore_search.hpp"
#include "keystore_compare.hpp"
#include "unicode/unicode.hpp"

#include <algorithm>
#include <utility>

namespace MKD
{
    namespace detail
    {
        keystore::CompareMode compareModeForSearch(const SearchMode mode)
        {
            switch (mode)
            {
            case SearchMode::Prefix:
            case SearchMode::Exact:
                return keystore::CompareMode::Forward;
            case SearchMode::Suffix:
                return keystore::CompareMode::Backward;
            }
            std::unreachable();
        }


        KeystoreIndex indexForSearch(const SearchMode mode)
        {
            switch (mode)
            {
            case SearchMode::Prefix:
            case SearchMode::Exact: // todo should use KeystoreIndex::Length later
                return KeystoreIndex::Prefix;
            case SearchMode::Suffix:
                return KeystoreIndex::Suffix;
            }
            std::unreachable();
        }

        Result<size_t> indexLowerBound(const Keystore& keystore,
                                       const KeystoreIndex indexType,
                                       std::string_view searchKey,
                                       const keystore::CompareMode mode)
        {
            size_t first = 0;
            size_t count = keystore.indexSize(indexType);
            while (count != 0)
            {
                const size_t step = count / 2;
                const size_t middle = first + step;
                auto key = keystore.keyAt(indexType, middle);
                if (!key)
                    return std::unexpected(key.error());
                if (keystore::compare(*key, searchKey, mode) < 0)
                {
                    first = middle + 1;
                    count -= step + 1;
                }
                else
                    count = step;
            }
            return first;
        }


        /*
         * Create the "next key" utf8 string for upperbound search
         * returns empty if no upper bound needed
         * that is search to end of index
         */
        std::string createNextKey(std::string_view searchKey, const SearchMode mode)
        {
            const bool reverse = (mode == SearchMode::Suffix);

            std::u32string norm;
            size_t offset = 0;

            while (offset < searchKey.size())
            {
                const char32_t cp = unicode::nextCodepoint(searchKey, offset);
                if (keystore::isIgnorable(cp)) continue;
                norm.push_back(keystore::foldCase(cp));
            }

            if (reverse)
                std::ranges::reverse(norm);

            if (norm.empty()) return {};

            // increment last cp
            // if max then pop or return empty if all max
            while (!norm.empty() && norm.back() == 0x10FFFF)
                norm.pop_back();

            if (norm.empty()) return {};

            ++norm.back();

            if (reverse)
                std::ranges::reverse(norm);

            return unicode::toUtf8(norm);
        }
    }


    Result<KeystoreSearchRange> keystoreSearch(const Keystore& keystore, std::string_view query, const SearchMode mode)
    {
        if (query.empty())
            return std::unexpected("Empty search query");

        const auto indexType = detail::indexForSearch(mode);
        const auto cmpMode = detail::compareModeForSearch(mode);
        const size_t indexSize = keystore.indexSize(indexType);

        if (indexSize == 0)
            return std::unexpected("Index is empty or does not exist");

        auto lowerBound = detail::indexLowerBound(keystore, indexType, query, cmpMode);
        if (!lowerBound) return std::unexpected(lowerBound.error());
        size_t upperBound = indexSize; // find upper bound via "next key"

        if (*lowerBound < indexSize)
        {
            const std::string nextKey = detail::createNextKey(query, mode);
            if (!nextKey.empty())
            {
                auto upper = detail::indexLowerBound(keystore, indexType, nextKey, cmpMode);
                if (!upper) return std::unexpected(upper.error());
                upperBound = *upper;
            }
        }

        return KeystoreSearchRange{
            .begin = *lowerBound,
            .end = upperBound,
            .indexType = indexType
        };
    }


    Result<std::vector<KeystoreSearchResult>> keystoreSearchResults(const Keystore& keystore,
                                                                    std::string_view query,
                                                                    const SearchMode mode)
    {
        auto range = keystoreSearch(keystore, query, mode);
        if (!range) return std::unexpected(range.error());

        std::vector<KeystoreSearchResult> results;
        results.reserve(range->count());

        std::u32string queryNorm;
        if (mode == SearchMode::Exact)
            queryNorm = detail::keystore::normalizeKeyToUTF32(query, false);

        for (size_t i = range->begin; i < range->end; i++)
        {
            auto entry = keystore.entryAt(range->indexType, i);
            if (!entry) return std::unexpected(entry.error());

            // Flag 0x08 on the word entry marks exact-search-only keys.
            // For non-exact searches, skip these entries.
            if (mode != SearchMode::Exact && (entry->flags & 0x08))
                continue;

            if (mode == SearchMode::Exact)
            {
                auto entryNorm = detail::keystore::normalizeKeyToUTF32(entry->key, false);
                if (entryNorm != queryNorm)
                    continue;
            }

            auto entryIds = keystore.entryIdsAt(range->indexType, i);
            if (!entryIds)
                return std::unexpected(entryIds.error());

            results.push_back({
                .key = entry->key,
                .index = entry->index,
                .entryIds = std::move(*entryIds),
            });
        }

        return results;
    }
}
