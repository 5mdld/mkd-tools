//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/product_writer.hpp"

namespace MKD::detail
{
    struct ProductBuild;

    class ProductInformationWriter
    {
    public:
        static Result<void> validate(const ProductInformation& information);
        static Result<void> write(ProductBuild& product);
    };
}
