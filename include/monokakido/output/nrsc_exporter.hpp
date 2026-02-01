//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "base_exporter.hpp"
#include "monokakido/resource/nrsc/nrsc.hpp"

#include <string_view>

namespace monokakido
{
    class NrscExporter : BaseExporter
    {
    public:
        static std::expected<ExportResult, std::string> exportAll(
            const Nrsc& nrsc,
            const ExportOptions& options);

    private:

        static fs::path makeOutputPath(std::string_view id, const ExportOptions& options);
    };
}
