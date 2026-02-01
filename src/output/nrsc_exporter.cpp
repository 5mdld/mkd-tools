//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/output/nrsc_exporter.hpp"


namespace monokakido
{
    std::expected<ExportResult, std::string> NrscExporter::exportAll(const Nrsc& nrsc, const ExportOptions& options)
    {
        ExportResult stats;
        stats.totalResources = nrsc.size();

        if (stats.totalResources == 0)
            return stats;

        if (!fs::exists(options.outputDirectory))
        {
            std::error_code ec;
            fs::create_directory(options.outputDirectory, ec);
            if (ec)
                return std::unexpected(std::format("Failed to create output directory: {}", ec.message()));
        }

        for (const auto& [id, data] : nrsc)
        {
            const auto outputPath = makeOutputPath(id, options);

            if (shouldSkipExisting(outputPath, options.overwriteExisting))
            {
                stats.skipped++;
                continue;
            }

            if (auto result = writeData(data, outputPath))
            {
                stats.exported++;
                stats.totalBytes += data.size();
            }
            else
            {
                stats.failed++;
                stats.errors.push_back(std::format("{}: {}", id, result.error()));
            }
        }

        return stats;
    }


    fs::path NrscExporter::makeOutputPath(std::string_view id, const ExportOptions& options)
    {
        const bool isAudio = id.find('.') == std::string_view::npos;

        fs::path outputDir = options.outputDirectory;
        if (options.createSubdirectories)
        {
            outputDir = isAudio
                ? outputDir / "audio"
                : outputDir / "graphics";
        }

        if (isAudio)
        {
            fs::path result = outputDir / std::string(id);
            result.replace_extension(".aac");
            return result;
        }

        return outputDir / id;
    }
}