//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "base_exporter.hpp"
#include "monokakido/resource/rsc/rsc.hpp"

namespace monokakido
{
    class RscExporter : BaseExporter
    {
    public:
        static std::expected<ExportResult, std::string> exportAll(const Rsc& rsc, const ExportOptions& options);
    };
}
