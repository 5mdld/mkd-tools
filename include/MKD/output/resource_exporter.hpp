//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"
#include "MKD/output/export_result.hpp"
#include "MKD/output/base_exporter.hpp"
#include "MKD/resource/resource_store.hpp"
#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/font.hpp"

namespace MKD
{
    class ResourceExporter final : public BaseExporter
    {
    public:
        static Result<ExportResult> exportAll(const ResourceStore& resourceStore, const ExportOptions& options, ResourceType type);

        static Result<ExportResult> exportAll(const NamedResourceStore& namedResourceStore, const ExportOptions& options, ResourceType type);

        static Result<ExportResult> exportFont(const Font& font, const ExportOptions& options);

    private:
        static std::span<const uint8_t> formatIfNeeded(std::span<const uint8_t> data, std::vector<uint8_t>& buf, bool shouldFormat);

        static std::vector<uint8_t> prettyPrintXml(std::span<const uint8_t> data);
    };
}