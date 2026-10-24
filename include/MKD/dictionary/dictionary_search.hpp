//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/entry_id.hpp"
#include "MKD/resource/search_mode.hpp"
#include "MKD/search_scope.hpp"

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

    // None performs one normalised index lookup.
    // DictionaryRules follows the product's search profile, including query expansion and scope fallbacks.
    enum class SearchFallback : uint8_t
    {
        None,
        DictionaryRules
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
        SearchFallback fallback = SearchFallback::DictionaryRules;
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

    /**
     * Entry order follows the dictionary's stored key order,
     * with dictionary-specific ordering rules when requested.
     */
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
         * Empty queries return no matches. Dictionary rules use platform linguistic support.
         */
        [[nodiscard]] Result<SearchResult> search(std::string_view query,
                                                  const SearchOptions& options = {}, std::stop_token stop = {}) const;

        /** Scopes available for the selected policy. The returned view belongs to this object. */
        [[nodiscard]] std::span<const SearchScope> scopes(
            SearchFallback fallback = SearchFallback::DictionaryRules) const noexcept;
        /** Whether the product's search class, delegate and key normalisation are recognised. */
        [[nodiscard]] bool supportsDictionaryRules() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
