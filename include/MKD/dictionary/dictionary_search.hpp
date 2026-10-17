//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/entry_id.hpp"
#include "MKD/resource/search_mode.hpp"
#include "MKD/resource/keystore_scope.hpp"

#include <memory>
#include <optional>
#include <string_view>
#include <span>
#include <stop_token>
#include <string>
#include <vector>

namespace MKD
{
    class Dictionary;

    enum class SearchScope : uint8_t
    {
        Headword = 0,
        Idiom = 1,
        Example = 2,
        English = 3,
        Gogi = 4,
        Modern = 5,
        Kanji = 6,
        Collocation = 7,
        CJ = 8,
        JC = 9,
        Fulltext = 10,
        Group = 11,
        CompoundNoun = 100,
        Numeral = 101,

        Sense = Gogi,
        Gendai = Modern,
        Metadata = Modern,
        Category = Group,
        Compound = CompoundNoun
    };

    [[nodiscard]] constexpr std::optional<SearchScope> searchScopeFromName(const std::string_view name) noexcept
    {
        for (const auto& names : detail::keystoreScopes)
            if (name == names.name) return static_cast<SearchScope>(names.scope);
        if (name == "Index" || name == "Vocabulary") return SearchScope::Headword;
        if (name == "Idiom/Phrasal verb" || name == "Jyukugo" || name == "Kanyoku" || name == "Phrase")
            return SearchScope::Idiom;
        if (name == "Yakugo") return SearchScope::Gogi;
        if (name == "Gendai" || name == "Metadata" || name == "MetaData") return SearchScope::Modern;
        if (name == "Oyaji") return SearchScope::Kanji;
        return std::nullopt;
    }


    // None performs one normalised index lookup.
    // Native adds dictionary search rules and fallbacks.
    enum class SearchFallback : uint8_t
    {
        None,
        Native
    };

    enum class SearchOrigin : uint8_t
    {
        Query,
        Inflection,
        Compound,
        JoinedQuery
    };

    struct SearchOptions
    {
        SearchScope scope = SearchScope::Headword;
        SearchMode match = SearchMode::Prefix;
        SearchFallback fallback = SearchFallback::Native;
        // Zero leaves the result unlimited. Limits apply after filtering and ordering.
        size_t limit = 0;
    };

    /** An owned normalised query term or variation which contributed to the result. */
    struct SearchMatch
    {
        std::string key;
        SearchMode match = SearchMode::Prefix;
        SearchOrigin origin = SearchOrigin::Query;
        auto operator<=>(const SearchMatch&) const = default;
    };

    /** Entry order follows the dictionary's stored key order, with native ordering rules when requested. */
    struct SearchResult
    {
        std::vector<EntryId> entries;
        std::vector<SearchMatch> matches;
        SearchScope scope = SearchScope::Headword;
        bool truncated = false;

        [[nodiscard]] size_t size() const noexcept { return entries.size(); }
        [[nodiscard]] bool empty() const noexcept { return entries.empty(); }
        [[nodiscard]] auto begin() const noexcept { return entries.begin(); }
        [[nodiscard]] auto end() const noexcept { return entries.end(); }
    };

    /** Borrows a dictionary which must remain at the same address throughout this object's lifetime. */
    class DictionarySearch
    {
    public:
        explicit DictionarySearch(const Dictionary& dictionary);
        ~DictionarySearch();
        DictionarySearch(DictionarySearch&&) noexcept;
        DictionarySearch& operator=(DictionarySearch&&) noexcept;
        DictionarySearch(const DictionarySearch&) = delete;
        DictionarySearch& operator=(const DictionarySearch&) = delete;

        /**
         * Calls have independent cancellation and may run concurrently.
         * Empty queries return no matches. Native rules use platform linguistic support.
         */
        [[nodiscard]] Result<SearchResult> search(std::string_view query,
                                                  const SearchOptions& options = {}, std::stop_token stop = {}) const;

        /** Scopes available for the selected policy. The returned view belongs to this object. */
        [[nodiscard]] std::span<const SearchScope> scopes(
            SearchFallback fallback = SearchFallback::Native) const noexcept;
        /** Whether the dictionary's native class, delegate, indexes and normalisation are recognised. */
        [[nodiscard]] bool supportsNativeSearch() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
