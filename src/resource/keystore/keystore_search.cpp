//
// kiwakiwaaにより 2026/03/10 に作成されました。
//

#include "keystore_search.hpp"
#include "../detail/unicode/unicode_case_map.hpp"

#include <algorithm>
#include "utf8.h"


namespace MKD
{
    namespace detail
    {
        char32_t foldCase(const char32_t cp)
        {
            // ascii uppercase to lowercase
            if (cp >= 0x41 && cp <= 0x5A)
                return cp | 0x20;

            // skip CJK / kana
            if (cp >= 0x3000 && cp < 0xA000)
                return cp;

            // everything else goes through the case map
            return detail::unicode::toLowercase(cp);
        }


        constexpr bool isIgnorable(const char32_t cp)
        {
            return cp == 0x20 || cp == 0x2D; // space or hyphen
        }


        size_t normalizedLength(std::string_view s)
        {
            auto it = s.begin();
            const auto end = s.end();
            size_t len = 0;
            while (it != end)
            {
                if (const char32_t cp = utf8::next(it, end); !isIgnorable(cp))
                    ++len;
            }
            return len;
        }


        std::u32string normalizeToU32(std::string_view s, const bool reverse)
        {
            std::u32string result;
            auto it = s.begin();
            const auto end = s.end();
            while (it != end)
            {
                const char32_t cp = utf8::next(it, end);
                if (isIgnorable(cp)) continue;
                result.push_back(foldCase(cp));
            }

            if (reverse)
                std::ranges::reverse(result);

            return result;
        }
    }


    int keystoreCompare(std::string_view a, std::string_view b, CompareMode mode)
    {
        if (mode == CompareMode::Length) // first by codepoint len then forward (prefix) with fold case
        {
            const size_t lenA = detail::normalizedLength(a);
            const size_t lenB = detail::normalizedLength(b);
            if (lenA < lenB) return -1;
            if (lenA > lenB) return 1;

            mode = CompareMode::Forward;
        }

        if (mode == CompareMode::Backward) // for suffix search
        {
            auto pa = a.end(); // reverse both strings
            auto pb = b.end();
            const auto beginA = a.begin();
            const auto beginB = b.begin();

            while (true)
            {
                char32_t ca = 0;
                char32_t cb = 0;

                while (pa != beginA)
                {
                    if (const char32_t cp = utf8::prior(pa, beginA); !detail::isIgnorable(cp))
                    {
                        ca = detail::foldCase(cp);
                        break;
                    }
                }

                while (pb != beginB)
                {
                    if (const char32_t cp = utf8::prior(pb, beginB); !detail::isIgnorable(cp))
                    {
                        cb = detail::foldCase(cp);
                        break;
                    }
                }

                if (ca == 0 && cb == 0) return 0;
                if (ca < cb) return -1;
                if (ca > cb) return 1;
            }
        }
        else // Forward
        {
            auto pa = a.begin();
            auto pb = b.begin();
            const auto endA = a.end();
            const auto endB = b.end();

            while (true)
            {
                char32_t ca = 0;
                char32_t cb = 0;

                while (pa != endA)
                {
                    if (const char32_t cp = utf8::next(pa, endA); !detail::isIgnorable(cp))
                    {
                        ca = detail::foldCase(cp);
                        break;
                    }
                }

                while (pb != endB)
                {
                    if (const char32_t cp = utf8::next(pb, endB); !detail::isIgnorable(cp))
                    {
                        cb = detail::foldCase(cp);
                        break;
                    }
                }

                if (ca == 0 && cb == 0) return 0;
                if (ca < cb) return -1;
                if (ca > cb) return 1;
            }
        }
    }


    namespace
    {
        CompareMode compareModeForSearch(const SearchMode mode)
        {
            switch (mode)
            {
                case SearchMode::Prefix:
                case SearchMode::Exact:
                    return CompareMode::Forward;
                case SearchMode::Suffix:
                    return CompareMode::Backward;
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


        size_t indexLowerBound(const Keystore& keystore,
                               const KeystoreIndex indexType,
                               std::string_view searchKey,
                               const CompareMode mode)
        {
            const size_t count = keystore.indexSize(indexType);
            if (count == 0 || searchKey.empty())
                return 0;

            size_t lowerBound = 0;
            size_t rangeSize = count;

            while (rangeSize > 0)
            {
                const size_t mid = lowerBound + (rangeSize >> 1);

                // get the entry key at this pos
                // todo: maybe not return all decoded pages here since we only need the key for comparison
                const auto entry = keystore.getByIndex(indexType, mid);
                if (!entry)
                    break;

                if (const int cmp = keystoreCompare(entry->key, searchKey, mode); cmp >= 0)
                {
                    rangeSize >>= 1;
                }
                else
                {
                    lowerBound = mid + 1;
                    rangeSize -= (rangeSize >> 1) + 1;
                }
            }

            return lowerBound;
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
            auto it = searchKey.begin();
            const auto end = searchKey.end();

            while (it != end)
            {
                const char32_t cp = utf8::next(it, end);
                if (detail::isIgnorable(cp)) continue;
                norm.push_back(detail::foldCase(cp));
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

            std::string result;
            utf8::utf32to8(norm.begin(), norm.end(), std::back_inserter(result));
            return result;
        }
    }


    Result<KeystoreSearchRange> keystoreSearch(const Keystore& keystore, std::string_view query, const SearchMode mode)
    {
        if (query.empty())
            return std::unexpected("Empty search query");

        const auto indexType = indexForSearch(mode);
        const auto cmpMode = compareModeForSearch(mode);
        const size_t indexSize = keystore.indexSize(indexType);

        if (indexSize == 0)
            return std::unexpected("Index is empty or does not exist");

        const size_t lowerBound = indexLowerBound(keystore, indexType, query, cmpMode);
        size_t upperBound = indexSize; // find upper bound via "next key"

        if (lowerBound < indexSize)
        {
            const std::string nextKey = createNextKey(query, mode);
            if (!nextKey.empty())
                upperBound = indexLowerBound(keystore, indexType, nextKey, cmpMode);
        }

        return KeystoreSearchRange{
            .begin = lowerBound,
            .end = upperBound,
            .indexType = indexType
        };
    }


    Result<std::vector<KeystoreLookupResult>> keystoreSearchResults(const Keystore& keystore,
                                                                    std::string_view query,
                                                                    const SearchMode mode)
    {
        auto range = keystoreSearch(keystore, query, mode);
        if (!range) return std::unexpected(range.error());

        std::vector<KeystoreLookupResult> results;
        results.reserve(range->count());

        std::u32string queryNorm;
        if (mode == SearchMode::Exact)
            queryNorm = detail::normalizeToU32(query, false);

        for (size_t i = range->begin; i < range->end; i++)
        {
            auto entry = keystore.getByIndex(range->indexType, i);
            if (!entry) continue; // shouldnt happen

            if (mode == SearchMode::Exact)
            {
                auto entryNorm = detail::normalizeToU32(entry->key, false);
                if (entryNorm != queryNorm)
                    continue;
            }

            results.push_back(std::move(*entry));
        }

        return results;
    }
}