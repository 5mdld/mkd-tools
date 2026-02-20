//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "MKD/output/common.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace MKDCLI
{
    namespace Colour
    {
        void setEnabled(bool enabled) noexcept;
        bool isEnabled() noexcept;

        // Detect whether outstream is TTYs and enable colour accordingly
        void autoDetect() noexcept;

        std::string bold(std::string_view text);
        std::string dim(std::string_view text);
        std::string green(std::string_view text);
        std::string yellow(std::string_view text);
        std::string red(std::string_view text);
        std::string cyan(std::string_view text);
    }


    struct TableRow
    {
        std::vector<std::string> cells;
    };

    /**
     * Print rows with columns padded ot align
     */
    void printTable(const std::vector<TableRow>& rows, size_t indent = 2);


    /**
     * Print export results to cerr
     */
    void printExportSummary(const MKD::ExportResult& result);

    /**
     * Prints a section header, e.g "── Resources ──"
     */
    void printSectionHeader(std::string_view title);

}
