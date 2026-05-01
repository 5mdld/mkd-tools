//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary_search.hpp"

#include <string_view>

namespace MKD::detail::search
{
    class SearchLogic
    {
    public:
        virtual ~SearchLogic() = default;

        [[nodiscard]] virtual Result<SearchResult> search(std::string_view query, const SearchOptions& options) = 0;
    };
}
