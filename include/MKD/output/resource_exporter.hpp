//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"
#include "MKD/output/export_result.hpp"
#include "MKD/output/base_exporter.hpp"
#include "MKD/resource/rsc.hpp"
#include "MKD/resource/nrsc.hpp"
#include "MKD/resource/font.hpp"

namespace MKD
{
    class ResourceExporter final : public BaseExporter
    {
    public:
        static Result<ExportResult> exportAll(const Rsc& rsc, const ExportOptions& options, ResourceType type);

        static Result<ExportResult> exportAll(const Nrsc& nrsc, const ExportOptions& options, ResourceType type);

        static Result<ExportResult> exportFont(const Font& font, const ExportOptions& options);

    private:

        static std::vector<uint8_t> prettyPrintXml(std::span<const uint8_t> data);
    };
}