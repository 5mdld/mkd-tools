//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "MKD/output/export_result.hpp"

#include <cstddef>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

namespace MKDCLI
{
    struct ProductRow
    {
        std::string id;
        std::string name;
        std::string genre;
    };

    [[nodiscard]] std::size_t displayWidth(std::string_view text);

    void printProductList(std::ostream& out, std::span<const ProductRow> products);
    void printExportSummary(std::ostream& out, const MKD::ExportResult& result);
}
