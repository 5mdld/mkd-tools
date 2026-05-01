//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "dic_complex_search_logic.hpp"

namespace MKD::detail::search
{
    class KanwaDicSearchLogic final : public DicComplexSearchLogic
    {
    public:
        using DicComplexSearchLogic::DicComplexSearchLogic;

    protected:
        [[nodiscard]] bool usesComplexSearchForScope(SearchScope scope, const Dictionary& dictionary) const override;

        [[nodiscard]] Result<SearchResult> searchSingleKey(std::string_view key, size_t limit) override;
    };
}
