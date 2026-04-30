//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/keystore_search.hpp"

#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace MKD::detail::search
{
    struct ScopeStoreMap
    {
        std::unordered_map<uint8_t, std::vector<const Keystore*>> stores;
        bool isJapaneseDictionary = false;
    };

    [[nodiscard]] std::string_view dropFirstCodepoint(std::string_view text);
    [[nodiscard]] std::string_view dropLastCodepoint(std::string_view text);

    [[nodiscard]] std::vector<EntryId> collectEntryIds(const std::vector<KeystoreSearchResult>& results);
    [[nodiscard]] bool isJapaneseText(std::string_view text);
    [[nodiscard]] bool scopeUsesCompoundSearch(SearchScope scope);
    [[nodiscard]] std::optional<SearchScope> searchScopeFromKeystoreScope(KeystoreScope scope);

    [[nodiscard]] std::vector<std::string> splitNormalizedKeys(std::string_view query);
    [[nodiscard]] std::vector<std::string> removeStopWords(const std::vector<std::string>& keys);
    [[nodiscard]] std::vector<std::string> uniqueStrings(std::vector<std::string> values);

    [[nodiscard]] ScopeStoreMap buildScopeStoreMap(const Dictionary& dictionary);
    [[nodiscard]] std::span<const Keystore* const> keystoresForScope(
        const ScopeStoreMap& scopeStores,
        SearchScope scope);
}
