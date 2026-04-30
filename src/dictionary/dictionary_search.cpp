//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#include "MKD/dictionary/dictionary_search.hpp"

#include "search/search_logic_factory.hpp"

#include <algorithm>
#include <atomic>
#include <iterator>

namespace MKD
{
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
        const Dictionary& dictionary;
        std::atomic<bool> cancelled{false};
        std::atomic<bool> searching{false};

        explicit Impl(const Dictionary& dict) : dictionary(dict) {}

        [[nodiscard]] bool isCancelled() const noexcept
        {
            return cancelled.load(std::memory_order_relaxed);
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
        impl->cancelled.store(false, std::memory_order_relaxed);
        impl->searching.store(true, std::memory_order_relaxed);

        struct SearchActivity
        {
            Impl& impl;
            ~SearchActivity()
            {
                impl.searching.store(false, std::memory_order_relaxed);
            }
        } activity{*impl};

        const auto logic = detail::search::makeSearchLogic(impl->dictionary, impl->cancelled);
        return logic->search(query, options);
    }


    void DictionarySearch::cancel() const noexcept
    {
        if (impl->searching.load(std::memory_order_relaxed))
            impl->cancelled.store(true, std::memory_order_relaxed);
    }


    void DictionarySearch::reset() const noexcept
    {
        impl->cancelled.store(false, std::memory_order_relaxed);
    }


    bool DictionarySearch::isCancelled() const noexcept
    {
        return impl->isCancelled();
    }
}
