//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "dic_search_logic.hpp"

namespace MKD::detail::search
{
    class DicComplexSearchLogic : public DicSearchLogic
    {
    public:
        using DicSearchLogic::DicSearchLogic;

    protected:
        [[nodiscard]] Result<SearchResult> searchInDictionary(std::string_view query, size_t limit) override;

        [[nodiscard]] virtual bool usesComplexSearchForScope(SearchScope scope, const Dictionary& dictionary) const;
        [[nodiscard]] virtual uint32_t normalizedHeadlineAnchorOffset() const noexcept;

        [[nodiscard]] Result<SearchResult> complexSearchInDictionary(std::string_view query) const;
        [[nodiscard]] Result<SearchResult> intersectComponentResults(const std::vector<std::string>& components) const;
        [[nodiscard]] SearchResult filterByNormalizedSortingHeadline(
            const SearchResult& result,
            const std::vector<std::string>& normalizedKeys) const;
    };
}
