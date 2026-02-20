//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/format.hpp"

#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

namespace MKDCLI
{
    namespace
    {
        bool g_colourEnabled = false;

        std::string wrap(std::string_view text, std::string_view code)
        {
            if (!g_colourEnabled)
                return std::string(text);
            return std::string(code) + std::string(text) + "\033[0m";
        }
    }

    void Colour::setEnabled(const bool enabled) noexcept
    {
        g_colourEnabled = enabled;
    }

    bool Colour::isEnabled() noexcept
    {
        return g_colourEnabled;
    }

    void Colour::autoDetect() noexcept
    {
        g_colourEnabled = isatty(fileno(stderr)) && isatty(fileno(stdout));

        // Respect NO_COLOR convention
        if (std::getenv("NO_COLOR"))
            g_colourEnabled = false;
    }

    std::string Colour::bold(std::string_view text)   { return wrap(text, "\033[1m"); }
    std::string Colour::dim(std::string_view text)    { return wrap(text, "\033[2m"); }
    std::string Colour::green(std::string_view text)  { return wrap(text, "\033[32m"); }
    std::string Colour::yellow(std::string_view text) { return wrap(text, "\033[33m"); }
    std::string Colour::red(std::string_view text)    { return wrap(text, "\033[31m"); }
    std::string Colour::cyan(std::string_view text)   { return wrap(text, "\033[36m"); }


    namespace
    {
        // Compute the visible width of a string
        // TODO: this is too simnple rn
        size_t visibleWidth(std::string_view str)
        {
            size_t width = 0;
            bool inEscape = false;

            for (const char i : str)
            {
                if (i == '\033')
                {
                    inEscape = true;
                    continue;
                }
                if (inEscape)
                {
                    if (i == 'm')
                        inEscape = false;
                    continue;
                }
                ++width;
            }
            return width;
        }
    }


    void printTable(const std::vector<TableRow>& rows, const size_t indent)
    {
        if (rows.empty()) return;

        // Find the maximum number of columns
        size_t numCols = 0;
        for (const auto& [cells] : rows)
            numCols = std::max(numCols, cells.size());

        // Compute column widths
        std::vector<size_t> colWidths(numCols, 0);
        for (const auto& [cells] : rows)
        {
            for (size_t col = 0; col < cells.size(); ++col)
                colWidths[col] = std::max(colWidths[col], visibleWidth(cells[col]));
        }

        const std::string pad(indent, ' ');
        for (const auto& [cells] : rows)
        {
            std::cerr << pad;
            for (size_t col = 0; col < cells.size(); ++col)
            {
                const auto& cell = cells[col];
                std::cerr << cell;

                // Pad to column width
                if (col + 1 < cells.size())
                {
                    const size_t visible = visibleWidth(cell);
                    const size_t padding = colWidths[col] > visible
                        ? colWidths[col] - visible + 2
                        : 2;
                    std::cerr << std::string(padding, ' ');
                }
            }
            std::cerr << "\n";
        }
    }


    void printExportSummary(const MKD::ExportResult& result)
    {
        std::cerr << Colour::bold("Done. ");
        std::cerr << Colour::green(std::to_string(result.exported)) << " exported";

        if (result.skipped > 0)
            std::cerr << ", " << Colour::yellow(std::to_string(result.skipped)) << " skipped";

        if (result.failed > 0)
            std::cerr << ", " << Colour::red(std::to_string(result.failed)) << " failed";

        if (result.totalBytes > 0)
        {
            const double mb = static_cast<double>(result.totalBytes) / (1024.0 * 1024.0);
            std::cerr << " (" << std::fixed;
            std::cerr.precision(1);
            std::cerr << mb << " MB)";
        }

        std::cerr << "\n";

        // Print error details
        if (!result.errors.empty())
        {
            std::cerr << "\n" << Colour::red("Errors:") << "\n";
            for (const auto& err : result.errors)
                std::cerr << "  " << Colour::dim("•") << " " << err << "\n";
        }
    }


    void printSectionHeader(std::string_view title)
    {
        std::cerr << Colour::dim("── ") << Colour::bold(std::string(title))
                  << Colour::dim(" ──") << "\n";
    }
}
