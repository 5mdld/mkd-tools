//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "kldic_complex_search_logic.hpp"

namespace MKD::detail::search
{
    class KLKanwaDicSearchLogic final : public KLDicComplexSearchLogic
    {
    public:
        using KLDicComplexSearchLogic::KLDicComplexSearchLogic;

    protected:
        [[nodiscard]] bool usesComplexSearchForScope(SearchScope scope, const Dictionary& dictionary) const override;

        [[nodiscard]] Result<SearchResult> searchSingleKey(std::string_view key, size_t limit) override;
    };
}
