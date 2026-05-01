//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "search_logic_factory.hpp"

#include "dic_search_logic.hpp"
#include "dic_complex_search_logic.hpp"
#include "kanwa_dic_search_logic.hpp"
#include "kogo_dic_search_logic.hpp"

namespace MKD::detail::search
{
    SearchLogicClass searchLogicClassForDictionary(const Dictionary& dictionary)
    {
        const auto& productSearchClass = dictionary.searchConfiguration().productSearchClass;
        if (productSearchClass && *productSearchClass == "KLKanwaDicSearchLogic")
            return SearchLogicClass::Kanwa;
        if (productSearchClass && *productSearchClass == "KLKogoDicSearchLogic")
            return SearchLogicClass::Kogo;

        return SearchLogicClass::Dic;
    }


    std::unique_ptr<SearchLogic> makeSearchLogic(
        const Dictionary& dictionary,
        std::atomic<bool>& cancelled)
    {
        switch (searchLogicClassForDictionary(dictionary))
        {
            case SearchLogicClass::Kanwa:
                return std::make_unique<KanwaDicSearchLogic>(dictionary, cancelled);
            case SearchLogicClass::Kogo:
                return std::make_unique<KogoDicSearchLogic>(dictionary, cancelled);
            case SearchLogicClass::Dic:
                return std::make_unique<DicSearchLogic>(dictionary, cancelled);
        }

        return std::make_unique<DicSearchLogic>(dictionary, cancelled);
    }
}
