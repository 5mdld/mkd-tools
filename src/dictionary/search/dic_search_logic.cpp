//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "dic_search_logic.hpp"

#include "MKD/resource/keystore_search.hpp"
#include "dictionary/text_normalize.hpp"
#include "unicode/unicode.hpp"

#include <algorithm>

namespace MKD::detail::search
{
    DicSearchLogic::DicSearchLogic(const Dictionary& dictionary, std::atomic<bool>& cancelled)
        : dictionary_(dictionary),
          cancelled_(cancelled),
          scopeStores_(buildScopeStoreMap(dictionary))
    {
    }


    Result<SearchResult> DicSearchLogic::search(const std::string_view query, const SearchOptions& options)
    {
        scope_ = options.scope;
        mode_ = options.type;
        useCompound_ = options.enableJapaneseCompound;
        allowScopeFallback_ = options.enableScopeFallback;
        allowStopWords_ = options.enableStopWords;

        return searchInDictionary(query, options.limit);
    }


    bool DicSearchLogic::isCancelled() const noexcept
    {
        return cancelled_.load(std::memory_order_relaxed);
    }


    Result<SearchResult> DicSearchLogic::searchInDictionary(const std::string_view query, const size_t limit)
    {
        auto keys = normalizedKeysForQuery(query);
        if (keys.empty())
            return std::unexpected("Empty search query");

        const auto originalScope = scope_;
        auto result = searchWithKeys(keys, limit);
        if (result && !result->empty())
            return result;

        if (isCancelled())
            return std::unexpected("Search cancelled");

        if (originalScope == SearchScope::Headword && keys.size() >= 2)
        {
            scope_ = originalScope;

            std::string joined;
            for (const auto& key : keys)
                joined += key;

            auto joinedResult = searchSingleKey(joined, limit);
            if (joinedResult && !joinedResult->empty())
                return joinedResult;
        }

        return result ? std::move(*result) : SearchResult{};
    }


    Result<SearchResult> DicSearchLogic::searchWithKeys(const std::vector<std::string>& keys, const size_t limit)
    {
        auto result = searchWithKeysAndFlags(keys, limit, 0);
        if (result && !result->empty())
            return result;

        const auto originalScope = scope_;

        if (allowScopeFallback_ && scope_ == SearchScope::Headword)
        {
            scope_ = SearchScope::Idiom;
            result = searchWithKeysAndFlags(keys, limit, 4);
            if (result && !result->empty())
                return result;

            if (allowStopWords_)
            {
                if (auto filtered = removeStopWords(keys); filtered.size() != keys.size())
                {
                    result = searchWithKeysAndFlags(filtered, limit, 4);
                    if (result && !result->empty())
                        return result;
                }
            }
        }
        else if (allowStopWords_ && scope_ == SearchScope::Idiom)
        {
            if (auto filtered = removeStopWords(keys); filtered.size() != keys.size())
            {
                result = searchWithKeysAndFlags(filtered, limit, 0);
                if (result && !result->empty())
                    return result;
            }
        }

        if (allowScopeFallback_
            && originalScope <= SearchScope::Idiom
            && keys.size() == 1
            && isJapaneseText(keys[0])
            && scopeStores_.isJapaneseDictionary)
        {
            scope_ = SearchScope::Example;
            result = searchWithKeysAndFlags(keys, limit, 8);
            if (result && !result->empty())
                return result;
        }

        return result ? std::move(*result) : SearchResult{};
    }


    Result<SearchResult> DicSearchLogic::searchWithKeysAndFlags(
        const std::vector<std::string>& keys,
        const size_t limit,
        const uint32_t flags)
    {
        SearchResult accumulated;

        for (const auto& key : keys)
        {
            if (isCancelled())
                return std::unexpected("Search cancelled");

            auto keyResult = searchSingleKey(key, limit);
            if (!keyResult)
                continue;

            if (accumulated.empty() && accumulated.matchedKeys.empty())
            {
                accumulated = std::move(*keyResult);
            }
            else
            {
                accumulated.intersectWith(*keyResult);

                if (accumulated.empty())
                    break;
            }
        }

        accumulated.flags |= flags;
        return accumulated;
    }


    Result<SearchResult> DicSearchLogic::searchSingleKey(const std::string_view key, size_t)
    {
        if (isCancelled())
            return std::unexpected("Search cancelled");

        if (useCompound_
            && mode_ <= SearchMode::Suffix
            && scopeUsesCompoundSearch(scope_)
            && isJapaneseText(key))
        {
            return japaneseCompoundSearch(key);
        }

        auto result = simpleSearch(key, mode_);
        if (result && !result->empty())
        {
            result->matchedKeys.emplace_back(key);
            return result;
        }

        return result;
    }


    Result<SearchResult> DicSearchLogic::japaneseCompoundSearch(const std::string_view key) const
    {
        if (key.empty())
            return std::unexpected("Empty search key");

        SearchResult accumulated;
        std::string_view candidate = key;

        while (!candidate.empty())
        {
            if (isCancelled())
                return std::unexpected("Search cancelled");

            if (auto result = simpleSearch(candidate, mode_); result && !result->empty())
            {
                accumulated = std::move(*result);
                accumulated.matchedKeys.emplace_back(candidate);
                break;
            }

            candidate = dropFirstCodepoint(candidate);
        }

        if (accumulated.empty())
            return SearchResult{};

        if (candidate.size() >= key.size())
            return accumulated;

        std::string_view remaining = key.substr(0, key.size() - candidate.size());

        while (!remaining.empty())
        {
            if (isCancelled())
                return std::unexpected("Search cancelled");

            std::string_view suffix = remaining;

            while (!suffix.empty())
            {
                if (isCancelled())
                    return std::unexpected("Search cancelled");

                auto result = simpleSearch(suffix, SearchMode::Exact);
                if (result && !result->empty())
                {
                    accumulated.intersectWith(*result);

                    if (accumulated.empty())
                        return SearchResult{};

                    accumulated.matchedKeys.emplace_back(suffix);
                    remaining = remaining.substr(suffix.size());
                    break;
                }

                suffix = dropLastCodepoint(suffix);
            }

            if (suffix.empty())
                break;
        }

        if (!accumulated.empty())
            accumulated.matchedKeys.emplace_back(key);

        std::erase_if(accumulated.entries, [&](const EntryId& entry) {
            const auto headline = dictionary_.headlineForEntryId(entry);
            if (!headline)
                return false;

            return !normalizedContains(*headline, key);
        });

        return accumulated;
    }


    Result<SearchResult> DicSearchLogic::simpleSearch(const std::string_view key, const SearchMode mode) const
    {
        if (key.empty())
            return std::unexpected("Empty search key");

        const auto stores = keystoresForScope(scopeStores_, scope_);
        if (stores.empty())
            return SearchResult{};

        SearchResult result;

        for (const auto* keystore : stores)
        {
            if (isCancelled())
                return std::unexpected("Search cancelled");

            auto hits = keystoreSearchResults(*keystore, key, mode);
            if (!hits)
                continue;

            if (hits->empty() && mode == SearchMode::Exact)
            {
                hits = keystoreSearchResults(*keystore, key, SearchMode::Prefix);
                if (!hits)
                    continue;
            }

            if (auto ids = collectEntryIds(*hits); !ids.empty())
            {
                SearchResult partial;
                partial.entries = std::move(ids);
                result.unionWith(partial);
            }
        }

        return result;
    }


    std::vector<std::string> DicSearchLogic::normalizedKeysForQuery(const std::string_view query) const
    {
        const auto normalized = unicode::normalizeDictionaryKey(
            query,
            unicode::DictionaryKeyNormalizeOption::CollapseSeparators
        );
        return splitNormalizedKeys(normalized);
    }
}
