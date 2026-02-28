//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/dictionary/dictionary_info.hpp"

#include <expected>
#include <vector>


namespace MKD
{
    class DictionarySource
    {
    public:

        virtual ~DictionarySource() = default;
        [[nodiscard]] virtual Result<std::vector<DictionaryInfo>> findAllAvailable() const = 0;
        [[nodiscard]] virtual Result<DictionaryInfo> findById(std::string_view dictId) const = 0;

    };
}