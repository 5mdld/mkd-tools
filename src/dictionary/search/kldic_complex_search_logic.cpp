//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "kldic_complex_search_logic.hpp"

#include "unicode/unicode.hpp"

#include <algorithm>

namespace MKD::detail::search
{
    Result<SearchResult> KLDicComplexSearchLogic::searchInDictionary(
        const std::string_view query,
        const size_t limit)
    {
        if (usesComplexSearchForScope(scope_, dictionary_))
            return complexSearchInDictionary(query);

        return DicSearchLogic::searchInDictionary(query, limit);
    }


    bool KLDicComplexSearchLogic::usesComplexSearchForScope(SearchScope, const Dictionary&) const
    {
        return false;
    }


    uint32_t KLDicComplexSearchLogic::normalizedHeadlineAnchorOffset() const noexcept
    {
        return 2048;
    }


    Result<SearchResult> KLDicComplexSearchLogic::complexSearchInDictionary(const std::string_view query) const
    {
        auto normalizedKeys = normalizedKeysForQuery(query);
        if (normalizedKeys.empty())
            return std::unexpected("Empty search query");

        std::vector<std::string> components;
        bool requiresHeadlineFilter = false;

        for (const auto& key : normalizedKeys)
        {
            const auto clusters = unicode::graphemeClusters(key);
            for (size_t i = 0; i < clusters.size(); ++i)
            {
                if (auto component = unicode::normalizeDictionaryKey(clusters[i]); !component.empty())
                {
                    components.push_back(std::move(component));
                    requiresHeadlineFilter = requiresHeadlineFilter || i != 0;
                }
            }
        }

        components = uniqueStrings(std::move(components));
        if (components.empty())
            return SearchResult{};

        auto result = intersectComponentResults(components);
        if (!result)
            return result;

        result->matchedKeys = uniqueStrings(std::move(normalizedKeys));
        if (!requiresHeadlineFilter)
            return result;

        return filterByNormalizedSortingHeadline(*result, result->matchedKeys);
    }


    Result<SearchResult> KLDicComplexSearchLogic::intersectComponentResults(
        const std::vector<std::string>& components) const
    {
        SearchResult accumulated;
        bool hasResult = false;

        for (const auto& component : components)
        {
            if (isCancelled())
                return std::unexpected("Search cancelled");

            auto result = simpleSearch(component, SearchMode::Prefix);
            if (!result)
                continue;

            if (!hasResult)
            {
                accumulated = std::move(*result);
                hasResult = true;
            }
            else
            {
                accumulated.intersectWith(*result);
            }
        }

        return accumulated;
    }


    SearchResult KLDicComplexSearchLogic::filterByNormalizedSortingHeadline(
        const SearchResult& result,
        const std::vector<std::string>& normalizedKeys) const
    {
        SearchResult filtered;
        filtered.flags = result.flags;
        filtered.matchedKeys = result.matchedKeys;

        const auto anchorOffset = normalizedHeadlineAnchorOffset();
        for (const auto& entry : result.entries)
        {
            const EntryId normalizedHeadlineEntry{
                .pageId = entry.pageId,
                .itemId = static_cast<uint16_t>(entry.itemId + anchorOffset),
            };

            auto sortingHeadline = dictionary_.sortingHeadlineForEntryId(normalizedHeadlineEntry);
            if (!sortingHeadline)
                continue;

            const bool containsEveryKey = std::ranges::all_of(normalizedKeys, [&](const std::string& key) {
                return sortingHeadline->find(key) != std::string::npos;
            });

            if (containsEveryKey)
                filtered.entries.push_back(entry);
        }

        return filtered;
    }
}
