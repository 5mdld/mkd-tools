//
// kiwakiwaaにより 2026/03/10 に作成されました。
//

#include "MKD/resource/keystore_search.hpp"
#include "keystore_compare.hpp"
#include "keystore_format.hpp"
#include "unicode/unicode.hpp"

namespace MKD::detail::keystore
{
    class IndexSearch
    {
    public:
        IndexSearch(const Keystore& store, std::string_view query, const SearchMode mode, std::stop_token stop)
            : store_(store), query_(query), mode_(mode), stop_(stop),
              index_(mode == SearchMode::Suffix ? KeystoreIndex::Suffix : KeystoreIndex::Prefix),
              comparison_(mode == SearchMode::Suffix ? CompareMode::Backward : CompareMode::Forward)
        {
        }

        [[nodiscard]] Result<KeystoreSearchRange> range() const
        {
            const auto count = store_.indexSize(index_);
            if (stop_.stop_requested())
                return std::unexpected("Search cancelled");

            if (unicode::firstInvalidUtf8Offset(query_) || query_.contains('\0'))
                return std::unexpected("Search query must be valid UTF-8 without embedded nulls");

            if (query_.empty() || count == 0)
                return KeystoreSearchRange{.indexType = index_};

            auto first = bound(0, count, false);
            if (!first)
                return std::unexpected(first.error());

            auto last = bound(*first, count, true);
            if (!last)
                return std::unexpected(last.error());

            return KeystoreSearchRange{*first, *last, index_};
        }

    private:
        [[nodiscard]] Result<size_t> bound(size_t first, size_t last, bool upper) const
        {
            // An ignorable-only query must not pull unrelated punctuation into its range.
            const bool exact = mode_ == SearchMode::Exact || compare(query_, {}, comparison_) == 0;
            while (first < last)
            {
                if (stop_.stop_requested())
                    return std::unexpected("Search cancelled");

                const auto middle = first + (last - first) / 2;
                auto key = store_.keyAt(index_, middle);
                if (!key)
                    return std::unexpected(key.error());

                const auto order = exact
                                       ? compare(*key, query_, comparison_)
                                       : comparePrefix(*key, query_, comparison_);
                if (order < 0 || (upper && order == 0))
                    first = middle + 1;
                else
                    last = middle;
            }
            return first;
        }

        const Keystore& store_;
        std::string_view query_;
        SearchMode mode_;
        std::stop_token stop_;
        KeystoreIndex index_;
        CompareMode comparison_;
    };
}

namespace MKD
{
    Result<KeystoreSearchRange> keystoreSearch(const Keystore& store, std::string_view query,
                                               const SearchMode mode, std::stop_token stop)
    {
        if (mode != SearchMode::Prefix && mode != SearchMode::Exact && mode != SearchMode::Suffix)
            return std::unexpected("Invalid search mode");

        return detail::keystore::IndexSearch(store, query, mode, stop).range();
    }

    Result<std::vector<KeystoreSearchResult>> keystoreSearchResults(const Keystore& store, std::string_view query,
                                                                    SearchMode mode, std::stop_token stop)
    {
        auto range = keystoreSearch(store, query, mode, stop);
        if (!range)
            return std::unexpected(range.error());

        std::vector<KeystoreSearchResult> results;
        for (auto i = range->begin; i < range->end; ++i)
        {
            if (stop.stop_requested())
                return std::unexpected("Search cancelled");

            auto entry = store.entryAt(range->indexType, i);
            if (!entry)
                return std::unexpected(entry.error());

            if (mode != SearchMode::Exact && (entry->flags & detail::keystore::word::exactOnly))
                continue;

            auto ids = store.entryIdsAt(range->indexType, i, KeystoreIdMapping::Resolved, stop);
            if (!ids)
                return std::unexpected(ids.error());

            results.push_back({entry->key, i, std::move(*ids)});
        }
        return results;
    }
}
