//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "dictionary/native_search.hpp"

#include <map>

namespace MKD::detail::search
{
    class SearchProfile
    {
    public:
        explicit SearchProfile(const Dictionary& dictionary);

        [[nodiscard]] const Dictionary& dictionary() const noexcept { return dictionary_; }
        [[nodiscard]] native::search::Family family() const noexcept { return class_.family; }
        [[nodiscard]] unicode::DictionaryKeyNormalizeOption normalization() const noexcept { return normalization_; }
        [[nodiscard]] bool normalizationKnown() const noexcept { return normalizationKnown_; }
        [[nodiscard]] bool japanese() const noexcept;
        [[nodiscard]] bool hasEnglish() const noexcept;
        [[nodiscard]] bool foldsKanjiNumbers() const noexcept { return delegate_.foldsKanjiNumbers; }
        [[nodiscard]] bool pageEntriesFirst() const noexcept { return delegate_.pageEntriesFirst; }
        [[nodiscard]] bool allowsScopeFallback() const noexcept
        {
            return dictionary_.searchConfiguration().allowsScopeFallback;
        }
        [[nodiscard]] bool hasHeadlineKeyIdentifiers() const noexcept;
        [[nodiscard]] const Keystore* headwordStore() const noexcept;
        [[nodiscard]] bool splitsQuery(SearchScope scope) const noexcept;
        [[nodiscard]] bool complexSearch(SearchScope scope) const noexcept;
        static bool compoundSearch(SearchScope scope) noexcept;
        [[nodiscard]] std::string_view language() const noexcept;
        [[nodiscard]] std::span<const Keystore* const> stores(SearchScope scope, SearchFallback fallback) const;
        [[nodiscard]] std::span<const SearchScope> scopes(SearchFallback fallback) const noexcept;
        [[nodiscard]] const std::optional<std::string>& unsupportedReason() const noexcept { return unsupported_; }
        [[nodiscard]] uint16_t headlineOffset(SearchScope scope) const noexcept;
        [[nodiscard]] unicode::DictionaryKeyNormalizeOption modernNormalization() const noexcept;

    private:
        void selectDefinitions();
        void selectNormalization();
        void mapNativeStores();
        void addStores(SearchScope destination, SearchScope source);

        const Dictionary& dictionary_;
        native::search::ClassDefinition class_ = native::search::defaultClass;
        native::search::DelegateDefinition delegate_{};
        unicode::DictionaryKeyNormalizeOption normalization_{};
        std::map<SearchScope, std::vector<const Keystore*>> directStores_;
        std::map<SearchScope, std::vector<const Keystore*>> nativeStores_;
        std::vector<SearchScope> nativeScopes_;
        std::vector<SearchScope> directScopes_;
        std::optional<std::string> unsupported_;
        bool normalizationKnown_ = true;
    };
}
