//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "search_common.hpp"

#include "unicode/unicode.hpp"

#include <algorithm>
#include <array>

namespace MKD::detail::search
{
    constexpr std::array<std::string_view, 20> StopWords = {
        "a", "an", "her", "hers", "herself", "him", "himself", "his",
        "me", "my", "myself", "the", "their", "them", "themselves",
        "they", "you", "your", "yours", "yourself"
    };


    std::string_view dropFirstCodepoint(const std::string_view text)
    {
        if (text.empty()) return {};
        size_t offset = 0;
        static_cast<void>(unicode::nextCodepoint(text, offset));
        return {text.data() + offset, text.size() - offset};
    }


    std::string_view dropLastCodepoint(const std::string_view text)
    {
        if (text.empty()) return {};
        size_t offset = text.size();
        static_cast<void>(unicode::previousCodepoint(text, offset));
        return {text.data(), offset};
    }


    std::vector<EntryId> collectEntryIds(const std::vector<KeystoreSearchResult>& results)
    {
        std::vector<EntryId> ids;
        for (const auto& result : results)
            ids.insert(ids.end(), result.entryIds.begin(), result.entryIds.end());

        std::ranges::sort(ids);
        auto [first, last] = std::ranges::unique(ids);
        ids.erase(first, last);
        return ids;
    }


    bool isJapaneseText(const std::string_view text)
    {
        return unicode::containsJapaneseScript(text);
    }


    bool scopeUsesCompoundSearch(const SearchScope scope)
    {
        const auto value = static_cast<uint8_t>(scope);
        return value <= 10 && ((1ULL << value) & 0x496ULL) != 0;
    }


    std::optional<SearchScope> searchScopeFromKeystoreScope(const KeystoreScope scope)
    {
        switch (scope)
        {
            case KeystoreScope::Headword: return SearchScope::Headword;
            case KeystoreScope::Idiom: return SearchScope::Idiom;
            case KeystoreScope::Example: return SearchScope::Example;
            case KeystoreScope::English: return SearchScope::English;
            case KeystoreScope::Gogi: return SearchScope::Gogi;
            case KeystoreScope::Kanji: return SearchScope::Kanji;
            case KeystoreScope::Collocation: return SearchScope::Collocation;
            case KeystoreScope::CJ: return SearchScope::CJ;
            case KeystoreScope::JC: return SearchScope::JC;
            case KeystoreScope::Fulltext: return SearchScope::Fulltext;
            case KeystoreScope::Group: return SearchScope::Group;
            case KeystoreScope::CompoundNoun:
                return SearchScope::Idiom;
            case KeystoreScope::Numeral: return SearchScope::Numeral;
        }
        return std::nullopt;
    }


    std::vector<std::string> splitNormalizedKeys(const std::string_view query)
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


    std::vector<std::string> removeStopWords(const std::vector<std::string>& keys)
    {
        std::vector<std::string> filtered;
        for (const auto& key : keys)
        {
            const bool isStopWord = std::ranges::any_of(StopWords, [&](const std::string_view stopWord) {
                return key == stopWord;
            });
            if (!isStopWord)
                filtered.push_back(key);
        }

        if (filtered.empty() && !keys.empty())
            filtered.push_back(keys.front());

        return filtered;
    }


    std::vector<std::string> uniqueStrings(std::vector<std::string> values)
    {
        std::ranges::sort(values);
        auto [first, last] = std::ranges::unique(values);
        values.erase(first, last);
        return values;
    }


    ScopeStoreMap buildScopeStoreMap(const Dictionary& dictionary)
    {
        ScopeStoreMap scopeStores;

        if (const auto& content = dictionary.content(); content.languages)
        {
            scopeStores.isJapaneseDictionary = std::ranges::find(*content.languages, "ja") != content.languages->end();
        }

        const Keystore* kanjiStore = nullptr;
        for (const auto& keystore : dictionary.keystores())
        {
            auto parsedScope = keystore.scope();
            if (!parsedScope)
                continue;

            auto searchScope = searchScopeFromKeystoreScope(*parsedScope);
            if (!searchScope)
                continue;

            scopeStores.stores[static_cast<uint8_t>(*searchScope)].push_back(&keystore);

            if (*searchScope == SearchScope::Kanji)
                kanjiStore = &keystore;
        }

        if (scopeStores.isJapaneseDictionary && kanjiStore)
        {
            for (auto& [scopeKey, stores] : scopeStores.stores)
            {
                if (scopeKey == static_cast<uint8_t>(SearchScope::Kanji))
                    continue;

                const bool alreadyHasKanji = std::ranges::any_of(stores, [&](const Keystore* keystore) {
                    return keystore == kanjiStore;
                });

                if (!alreadyHasKanji)
                    stores.push_back(kanjiStore);
            }
        }

        return scopeStores;
    }


    std::span<const Keystore* const> keystoresForScope(const ScopeStoreMap& scopeStores, const SearchScope scope)
    {
        const auto it = scopeStores.stores.find(static_cast<uint8_t>(scope));
        if (it != scopeStores.stores.end())
            return it->second;
        return {};
    }
}
