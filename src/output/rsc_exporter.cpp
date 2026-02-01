//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/output/rsc_exporter.hpp"


namespace monokakido
{
    std::expected<ExportResult, std::string> RscExporter::exportAll(const Rsc& rsc, const ExportOptions& options)
    {
        ExportResult result;
        result.totalResources = rsc.size();

        if (result.totalResources == 0)
            return result;

        if (!fs::exists(options.outputDirectory))
        {
            std::error_code ec;
            fs::create_directory(options.outputDirectory, ec);
            if (ec)
                return std::unexpected(std::format("Failed to create output directory: {}", ec.message()));
        }

        const fs::path outputDir = options.createSubdirectories
                                       ? options.outputDirectory / "entries"
                                       : options.outputDirectory;


        for (const auto& [itemId, data] : rsc)
        {
            fs::path outputPath = outputDir / std::format("{:10}.xml", itemId);

            if (shouldSkipExisting(outputPath, options.overwriteExisting))
            {
                result.skipped++;
                continue;
            }

            std::span<const uint8_t> dataToWrite = data;
            // std::vector<uint8_t> prettyData;

            if (options.prettyPrintXml)
            {
                // TODO: Implement XML pretty-printing if needed
                // For now, just write as-is
            }

            if (auto writeResult = writeData(dataToWrite, outputPath))
            {
                result.exported++;
                result.totalBytes += dataToWrite.size();
            }
            else
            {
                result.failed++;
                result.errors.push_back(
                    std::format("Entry {}: {}", itemId, writeResult.error())
                );
            }
        }

        return result;
    }

}