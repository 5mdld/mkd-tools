//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "base_exporter.hpp"
#include "monokakido/resource/font.hpp"

namespace monokakido
{
    class FontExporter : BaseExporter
    {
    public:
        static std::expected<ExportResult, std::string> exportFont(const Font& font, const ExportOptions& options);
    };
}
