//
// kiwakiwaaにより 2026/02/27 に作成されました。
//

#pragma once

#include <expected>
#include <string>

namespace MKD
{
    template<typename T>
    using Result = std::expected<T, std::string>;
}