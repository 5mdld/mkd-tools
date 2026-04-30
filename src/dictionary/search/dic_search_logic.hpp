//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "search_common.hpp"
#include "search_logic.hpp"

#include <atomic>

namespace MKD::detail::search
{
    class DicSearchLogic : public SearchLogic
    {
    public:
        DicSearchLogic(const Dictionary& dictionary, std::atomic<bool>& cancelled);

        [[nodiscard]] Result<SearchResult> search(std::string_view query, const SearchOptions& options) override;

    protected:
        [[nodiscard]] bool isCancelled() const noexcept;

        [[nodiscard]] virtual Result<SearchResult> searchInDictionary(std::string_view query, size_t limit);
        [[nodiscard]] Result<SearchResult> searchWithKeys(const std::vector<std::string>& keys, size_t limit);
        [[nodiscard]] Result<SearchResult> searchWithKeysAndFlags(
            const std::vector<std::string>& keys,
            size_t limit,
            uint32_t flags);

        [[nodiscard]] virtual Result<SearchResult> searchSingleKey(std::string_view key, size_t limit);
        [[nodiscard]] Result<SearchResult> japaneseCompoundSearch(std::string_view key) const;
        [[nodiscard]] Result<SearchResult> simpleSearch(std::string_view key, SearchMode mode) const;
        [[nodiscard]] static std::vector<std::string> normalizedKeysForQuery(std::string_view query) ;

        const Dictionary& dictionary_;
        std::atomic<bool>& cancelled_;
        ScopeStoreMap scopeStores_;
        SearchScope scope_ = SearchScope::Headword;
        SearchMode mode_ = SearchMode::Prefix;
        bool useCompound_ = true;
        bool allowScopeFallback_ = true;
        bool allowStopWords_ = true;
    };
}
