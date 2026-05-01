//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "kanwa_dic_search_logic.hpp"

namespace MKD::detail::search
{
    bool KanwaDicSearchLogic::usesComplexSearchForScope(const SearchScope scope, const Dictionary&) const
    {
        return scope == SearchScope::Example || scope == SearchScope::Gogi;
    }


    Result<SearchResult> KanwaDicSearchLogic::searchSingleKey(const std::string_view key, size_t)
    {
        if (scope_ == SearchScope::Idiom && mode_ <= SearchMode::Suffix)
        {
            auto result = simpleSearch(key, mode_);
            if (result && !result->empty())
                result->matchedKeys.emplace_back(key);
            return result;
        }

        return DicComplexSearchLogic::searchSingleKey(key, 0);
    }
}