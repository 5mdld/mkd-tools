//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#include "MKD/dictionary/dictionary_search.hpp"
#include "search/search_session.hpp"

namespace MKD
{
    struct DictionarySearch::Impl
    {
        detail::search::SearchProfile profile;

        explicit Impl(const Dictionary& dictionary) : profile(dictionary)
        {
        }
    };

    DictionarySearch::DictionarySearch(const Dictionary& dictionary) : impl_(std::make_unique<Impl>(dictionary))
    {
    }

    DictionarySearch::~DictionarySearch() = default;
    DictionarySearch::DictionarySearch(DictionarySearch&&) noexcept = default;
    DictionarySearch& DictionarySearch::operator=(DictionarySearch&&) noexcept = default;

    Result<SearchResult> DictionarySearch::search(std::string_view query, const SearchOptions& options,
                                                  std::stop_token stop) const
    {
        return detail::search::SearchSession(impl_->profile, query, options, stop).run();
    }

    std::span<const SearchScope> DictionarySearch::scopes(SearchFallback fallback) const noexcept
    {
        return impl_->profile.scopes(fallback);
    }

    bool DictionarySearch::supportsDictionaryRules() const noexcept { return !impl_->profile.unsupportedReason(); }
}
