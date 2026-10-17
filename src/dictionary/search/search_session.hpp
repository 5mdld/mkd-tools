//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "search_query.hpp"

namespace MKD::detail::search
{
    class SearchSession
    {
    public:
        SearchSession(const SearchProfile& profile, std::string_view query,
                      const SearchOptions& options, std::stop_token stop);
        [[nodiscard]] Result<SearchResult> run();

    private:
        struct Matches
        {
            std::vector<EntryId> entries;
            std::vector<SearchMatch> keys;
            SearchScope scope;

            [[nodiscard]] bool empty() const noexcept { return entries.empty(); }
            void unite(Matches other);
            void intersect(Matches other);
            void sort();
            void sortKeys();
        };

        [[nodiscard]] Result<void> validate() const;
        [[nodiscard]] Result<SearchResult> finish(Matches result) const;
        [[nodiscard]] Result<Matches> nativeQuery();
        [[nodiscard]] Result<Matches> fallbackScopes(const std::vector<std::string>& keys);
        [[nodiscard]] Result<Matches> terms(const std::vector<std::string>& keys, SearchScope scope);
        struct HeadlineKey
        {
            EntryId key;
            EntryId entry;
        };

        [[nodiscard]] Result<void> intersectHeadwords(Matches& result, Matches other) const;
        [[nodiscard]] Result<std::vector<HeadlineKey>> mappedHeadwords(std::span<const EntryId> keys,
                                                                      const Keystore& store) const;
        [[nodiscard]] Result<Matches> joinedQuery(const std::vector<std::string>& keys) const;
        [[nodiscard]] Result<Matches> headlineFallback(const std::vector<std::string>& keys);
        [[nodiscard]] Result<Matches> key(std::string_view text, SearchScope scope) const;
        [[nodiscard]] Result<Matches> simple(std::string_view text, SearchScope scope, SearchMode mode) const;
        [[nodiscard]] Result<Matches> lookup(std::string_view text, SearchScope scope, SearchMode mode,
                                             SearchOrigin origin = SearchOrigin::Query) const;
        [[nodiscard]] Result<void> appendStore(Matches& result, const Keystore& store,
                                               std::string_view text, SearchMode mode, SearchOrigin origin) const;

        struct CompoundMatch
        {
            Matches matches;
            std::string_view prefix;
        };

        [[nodiscard]] Result<Matches> compound(std::string_view text, SearchScope scope) const;
        [[nodiscard]] Result<CompoundMatch> compoundSuffix(std::string_view text, SearchScope scope) const;
        [[nodiscard]] Result<void> intersectCompoundPrefix(Matches& result, std::string_view prefix) const;
        [[nodiscard]] Result<void> filterCompoundHeadlines(Matches& result, std::string_view text) const;
        [[nodiscard]] Result<Matches> complex(const std::vector<std::string>& keys, SearchScope scope) const;
        [[nodiscard]] Result<Matches> intersectComponents(const std::vector<std::string>& components,
                                                          SearchScope scope) const;
        [[nodiscard]] Result<void> filterSortingHeadlines(Matches& result, const std::vector<std::string>& keys) const;
        [[nodiscard]] Result<void> orderModernEntries(Matches& matches) const;
        [[nodiscard]] Result<std::optional<std::string>> headline(EntryId id, bool sorting) const;
        [[nodiscard]] bool cancelled() const noexcept { return stop_.stop_requested(); }

        const SearchProfile& profile_;
        std::string_view text_;
        const SearchOptions& options_;
        std::stop_token stop_;
        SearchQuery query_;
    };
}
