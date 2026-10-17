//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <cstdint>
#include <span>

namespace MKD::detail
{
    struct ProductBuild;

    class ProductIcon
    {
    public:
        static Result<void> write(ProductBuild& product, std::span<const uint8_t> bytes);
    };
}
