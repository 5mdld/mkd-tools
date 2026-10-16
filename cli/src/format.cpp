//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/format.hpp"

#include "unicode/properties.hpp"
#include "unicode/utf8.hpp"

#include <algorithm>
#include <ostream>
#include <print>

namespace MKDCLI
{
    std::size_t displayWidth(std::string_view text)
    {
        std::size_t width = 0;
        namespace unicode = MKD::detail::unicode;

        for (const auto cluster : unicode::graphemeClusters(text))
        {
            std::size_t offset = 0;
            const auto codepoint = unicode::nextCodepoint(cluster, offset);
            const auto property = unicode::eastAsianWidth(codepoint);

            const bool wide = property == unicode::EastAsianWidth::Wide
                || property == unicode::EastAsianWidth::Fullwidth;
            width += wide ? 2 : 1;
        }
        return width;
    }

    void printProductList(std::ostream& out, std::span<const ProductRow> products)
    {
        std::size_t idWidth = 0;
        std::size_t nameWidth = 0;

        for (const auto& product : products)
        {
            idWidth = std::max(idWidth, displayWidth(product.id));
            nameWidth = std::max(nameWidth, displayWidth(product.name));
        }

        for (const auto& product : products)
        {
            std::println(out, "  {:<{}}  {:<{}}  {}",
                         product.id, idWidth,
                         product.name, nameWidth,
                         product.genre);
        }
    }

    void printExportSummary(std::ostream& out, const MKD::ExportResult& result)
    {
        std::print(out, "Export: {} exported", result.exported);

        if (result.skipped > 0)
            std::print(out, ", {} skipped", result.skipped);

        if (result.failed > 0)
            std::print(out, ", {} failed", result.failed);

        if (result.totalBytes > 0)
        {
            constexpr double bytesPerMiB = 1024.0 * 1024.0;
            std::print(out, " ({:.1f} MiB)", result.totalBytes / bytesPerMiB);
        }

        std::println(out);

        if (!result.errors.empty())
        {
            std::println(out, "\nErrors:");
            for (const auto& error : result.errors)
                std::println(out, "  • {}", error);
        }
    }
}
