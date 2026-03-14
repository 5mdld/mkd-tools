//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#include "dictionary_search.hpp"
#include "text_normalize.hpp"
#include "MKD/dictionary/dictionary.hpp"

#include "utf8.h"

#include <algorithm>
#include <atomic>
#include <unordered_map>

namespace MKD
{
    namespace
    {
        std::string_view dropFirstCodepoint(std::string_view s)
        {
            if (s.empty()) return {};
            auto it = s.begin();
            utf8::next(it, s.end());
            return {it, s.end()};
        }


        std::string_view dropLastCodepoint(std::string_view s)
        {
            if (s.empty()) return {};
            auto it = s.end();
            utf8::prior(it, s.begin());
            return {s.begin(), it};
        }


        std::vector<EntryId> collectEntryIds(const std::vector<KeystoreSearchResult>& results)
        {
            std::vector<EntryId> ids;
            for (const auto& r : results)
                ids.insert(ids.end(), r.entryIds.begin(), r.entryIds.end());

            std::ranges::sort(ids);
            auto [first, last] = std::ranges::unique(ids);
            ids.erase(first, last);
            return ids;
        }


        bool isJapanese(std::string_view s)
        {
            auto it = s.begin();
            const auto end = s.end();
            while (it != end)
            {
                const char32_t cp = utf8::next(it, end);
                if ((cp >= 0x3040 && cp <= 0x309F)
                    || (cp >= 0x30A0 && cp <= 0x30FF)
                    || (cp >= 0x4E00 && cp <= 0x9FFF)
                    || (cp >= 0x3400 && cp <= 0x4DBF))
                    return true;
            }
            return false;
        }


        // might not be 100% right
        // (1LL << scope) & 0x496
        bool scopeUsesCompoundSearch(SearchScope scope)
        {
            switch (scope)
            {
                case SearchScope::Idiom:
                case SearchScope::Compound:
                case SearchScope::Example:
                case SearchScope::Sense:
                case SearchScope::Collocation:
                case SearchScope::Fulltext:
                    return true;
                default:
                    return false;
            }
        }


        std::optional<SearchScope> scopeFromFilename(std::string_view filename)
        {
            constexpr std::string_view ext = ".keystore";
            if (filename.size() <= ext.size() || filename.substr(filename.size() - ext.size()) != ext)
                return std::nullopt;

            const auto name = filename.substr(0, filename.size() - ext.size());

            if (name == "headword")   return SearchScope::Headword;
            if (name == "idiom")      return SearchScope::Idiom;
            if (name == "example")    return SearchScope::Example;
            if (name == "english")    return SearchScope::English;
            if (name == "sense")      return SearchScope::Sense;
            if (name == "kanji")      return SearchScope::Kanji;
            if (name == "collocation") return SearchScope::Collocation;
            if (name == "fulltext")   return SearchScope::Fulltext;
            if (name == "category")   return SearchScope::Category;
            if (name == "compound")   return SearchScope::Idiom;
            if (name == "numeral")    return SearchScope::Numeral;
            return std::nullopt;
        }


        std::vector<std::string> splitKeys(std::string_view query)
        {
            std::vector<std::string> keys;
            size_t i = 0;

            while (i < query.size())
            {
                while (i < query.size() && query[i] == ' ')
                    ++i;

                const size_t start = i;
                while (i < query.size() && query[i] != ' ')
                    ++i;

                if (i > start)
                    keys.emplace_back(query.substr(start, i - start));
            }
            return keys;
        }


        constexpr std::string_view STOP_WORDS[] = {
            "a", "an", "her", "hers", "herself", "him", "himself", "his",
            "me", "my", "myself", "the", "their", "them", "themselves",
            "they", "you", "your", "yours", "yourself"
        };


        std::vector<std::string> removeStopWords(const std::vector<std::string>& keys)
        {
            std::vector<std::string> filtered;
            for (const auto& key : keys)
            {
                bool isStop = std::ranges::any_of(STOP_WORDS, [&](std::string_view sw) {
                    return key == sw;
                });
                if (!isStop)
                    filtered.push_back(key);
            }

            if (filtered.empty() && !keys.empty())
                filtered.push_back(keys.front());

            return filtered;
        }
    }


    void SearchResult::unionWith(const SearchResult& other)
    {
        std::vector<EntryId> merged;
        merged.reserve(entries.size() + other.entries.size());
        std::ranges::merge(entries, other.entries, std::back_inserter(merged));
        auto [first, last] = std::ranges::unique(merged);
        merged.erase(first, last);
        entries = std::move(merged);
    }


    void SearchResult::intersectWith(const SearchResult& other)
    {
        std::vector<EntryId> common;
        std::ranges::set_intersection(entries, other.entries, std::back_inserter(common));
        entries = std::move(common);
    }


    struct DictionarySearch::Impl
    {
        const Dictionary& dict;
        std::atomic<bool> cancelled{false};
        SearchScope scope = SearchScope::Headword;
        SearchMode mode = SearchMode::Prefix;
        bool useCompound = true;
        std::unordered_map<uint8_t, std::vector<const Keystore*>> scopeMap;
        bool isJapaneseDictionary = false;

        explicit Impl(const Dictionary& d) : dict(d)
        {
            // Check if this is a Japanese dictionary
            if (const auto& content = dict.content(); content.languages)
            {
                for (const auto& lang : *content.languages)
                {
                    if (lang == "ja")
                    {
                        isJapaneseDictionary = true;
                        break;
                    }
                }
            }

            // Build scope map from keystore filenames.
            const Keystore* kanjiStore = nullptr;

            for (const auto& ks : dict.keystores())
            {
                auto parsedScope = scopeFromFilename(ks.filename());
                if (!parsedScope) continue;

                scopeMap[static_cast<uint8_t>(*parsedScope)].push_back(&ks);

                if (*parsedScope == SearchScope::Kanji)
                    kanjiStore = &ks;
            }


            /*
             * For Japanese dictionaries, the app adds the Kanji keystore as
             * secondary for all scopes (when option 0x4000000 is set and no
             * subkey directory exists). We always add it since we don't track
             * subkey directories separately.
             */
            if (isJapaneseDictionary && kanjiStore)
            {
                for (auto& [scopeKey, stores] : scopeMap)
                {
                    if (scopeKey == static_cast<uint8_t>(SearchScope::Kanji))
                        continue;

                    bool alreadyHas = std::ranges::any_of(stores, [&](const Keystore* k) {
                        return k == kanjiStore;
                    });

                    if (!alreadyHas)
                        stores.push_back(kanjiStore);
                }
            }
        }

        [[nodiscard]] bool isCancelled() const noexcept
        {
            return cancelled.load(std::memory_order_relaxed);
        }

        // Get keystores for the current scope. Returns empty span if none found.
        [[nodiscard]] std::span<const Keystore* const> keystoresForScope(SearchScope s) const
        {
            auto it = scopeMap.find(static_cast<uint8_t>(s));
            if (it != scopeMap.end())
                return it->second;
            return {};
        }
    };


    DictionarySearch::DictionarySearch(const Dictionary& dict)
        : impl(std::make_unique<Impl>(dict))
    {
    }

    DictionarySearch::~DictionarySearch() = default;
    DictionarySearch::DictionarySearch(DictionarySearch&&) noexcept = default;
    DictionarySearch& DictionarySearch::operator=(DictionarySearch&&) noexcept = default;


    Result<SearchResult> DictionarySearch::search(std::string_view query, const SearchOptions& options) const
    {
        impl->scope = options.scope;
        impl->mode = options.type;
        impl->useCompound = options.enableJapaneseCompound;

        const auto normalized = normalizeSearchQuery(query);
        auto keys = splitKeys(normalized);
        if (keys.empty())
            return std::unexpected("Empty search query");

        auto result = searchWithKeys(keys, options.limit);
        if (result && !result->empty())
            return result;

        if (impl->isCancelled())
            return std::unexpected("Search cancelled");

        // joined key fallback for multi-word queries.
        if (keys.size() >= 2)
        {
            impl->scope = options.scope;

            std::string joined;
            for (const auto& k : keys)
                joined += k;

            auto joinedResult = searchSingleKey(joined, options.limit);
            if (joinedResult && !joinedResult->empty())
                return joinedResult;
        }

        return result ? std::move(*result) : SearchResult{};
    }

    void DictionarySearch::cancel() const noexcept { impl->cancelled.store(true, std::memory_order_relaxed); }
    void DictionarySearch::reset() const noexcept { impl->cancelled.store(false, std::memory_order_relaxed); }
    bool DictionarySearch::isCancelled() const noexcept { return impl->isCancelled(); }


    Result<SearchResult> DictionarySearch::searchWithKeys(const std::vector<std::string>& keys, size_t limit) const
    {
        // try current scope
        auto result = searchWithKeysAndFlags(keys, limit, 0);
        if (result && !result->empty())
            return result;

        // fallback Headword → Idiom
        if (impl->scope == SearchScope::Headword)
        {
            impl->scope = SearchScope::Idiom;
            result = searchWithKeysAndFlags(keys, limit, 4);
            if (result && !result->empty())
                return result;

            // stop word removal at Idiom scope
            if (auto filtered = removeStopWords(keys); filtered.size() != keys.size())
            {
                result = searchWithKeysAndFlags(filtered, limit, 4);
                if (result && !result->empty())
                    return result;
            }
        }

        // japanese fallback → Example scope
        if (impl->scope <= SearchScope::Idiom && keys.size() == 1 && isJapanese(keys[0]))
        {
            impl->scope = SearchScope::Example;
            result = searchWithKeysAndFlags(keys, limit, 8);
            if (result && !result->empty())
                return result;
        }

        return result ? std::move(*result) : SearchResult{};
    }


    Result<SearchResult> DictionarySearch::searchWithKeysAndFlags(const std::vector<std::string>& keys, const size_t limit, const uint32_t flags) const
    {
        SearchResult accumulated;

        for (const auto& key : keys)
        {
            if (impl->isCancelled())
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


    Result<SearchResult> DictionarySearch::searchSingleKey(std::string_view key, size_t /*limit*/) const
    {
        if (impl->isCancelled())
            return std::unexpected("Search cancelled");

        if (impl->useCompound
            && impl->mode <= SearchMode::Suffix
            && scopeUsesCompoundSearch(impl->scope)
            && isJapanese(key))
        {
            return japaneseCompoundSearch(key);
        }

        auto result = simpleSearch(key, impl->mode);
        if (result && !result->empty())
        {
            result->matchedKeys.emplace_back(key);
            return result;
        }

        return result;
    }


    Result<SearchResult> DictionarySearch::japaneseCompoundSearch(std::string_view key) const
    {
        if (key.empty())
            return std::unexpected("Empty search key");

        // front-strip to find longest matching suffix
        SearchResult accumulated;
        std::string_view candidate = key;

        while (!candidate.empty())
        {
            if (impl->isCancelled())
                return std::unexpected("Search cancelled");

            if (auto result = simpleSearch(candidate, impl->mode); result && !result->empty())
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

        // back-strip on the unmatched prefix
        std::string_view remaining = key.substr(0, key.size() - candidate.size());

        while (!remaining.empty())
        {
            if (impl->isCancelled())
                return std::unexpected("Search cancelled");

            std::string_view suffix = remaining;

            while (!suffix.empty())
            {
                if (impl->isCancelled())
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

        // validate with headline
        std::erase_if(accumulated.entries, [&](const EntryId& entry) {
            const auto headline = impl->dict.headlineForEntryId(entry);
            if (!headline)
                return false;

            return !normalizedContains(*headline, key);
        });

        return accumulated;
    }


    Result<SearchResult> DictionarySearch::simpleSearch(std::string_view key, SearchMode mode) const
    {
        if (key.empty())
            return std::unexpected("Empty search key");

        auto stores = impl->keystoresForScope(impl->scope);
        if (stores.empty())
            return SearchResult{};

        SearchResult result;

        for (const auto* keystore : stores)
        {
            if (impl->isCancelled())
                return std::unexpected("Search cancelled");

            auto hits = keystoreSearchResults(*keystore, key, mode);
            if (!hits)
                continue;

            // Exact → Prefix fallback
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
}
