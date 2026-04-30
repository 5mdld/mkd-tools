//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "search_logic_factory.hpp"

#include "dic_search_logic.hpp"
#include "kldic_complex_search_logic.hpp"
#include "klkanwa_dic_search_logic.hpp"

namespace MKD::detail::search
{
    SearchLogicClass searchLogicClassForDictionary(const Dictionary& dictionary)
    {
        const auto& productSearchClass = dictionary.searchConfiguration().productSearchClass;
        if (productSearchClass && *productSearchClass == "KLKanwaDicSearchLogic")
            return SearchLogicClass::KLKanwa;

        return SearchLogicClass::Dic;
    }


    std::unique_ptr<SearchLogic> makeSearchLogic(
        const Dictionary& dictionary,
        std::atomic<bool>& cancelled)
    {
        switch (searchLogicClassForDictionary(dictionary))
        {
            case SearchLogicClass::KLKanwa:
                return std::make_unique<KLKanwaDicSearchLogic>(dictionary, cancelled);
            case SearchLogicClass::Dic:
                return std::make_unique<DicSearchLogic>(dictionary, cancelled);
        }

        return std::make_unique<DicSearchLogic>(dictionary, cancelled);
    }
}
