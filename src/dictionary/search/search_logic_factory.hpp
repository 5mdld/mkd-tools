//
// kiwakiwaaにより 2026/05/01 に作成されました。
//


#pragma once

#include "search_logic.hpp"

#include "MKD/dictionary/dictionary.hpp"

#include <atomic>
#include <memory>

namespace MKD::detail::search
{
    enum class SearchLogicClass
    {
        Dic,
        Kanwa,
        Kogo
    };

    [[nodiscard]] SearchLogicClass searchLogicClassForDictionary(const Dictionary& dictionary);

    [[nodiscard]] std::unique_ptr<SearchLogic> makeSearchLogic(
        const Dictionary& dictionary,
        std::atomic<bool>& cancelled);
}
