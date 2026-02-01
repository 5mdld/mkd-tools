//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/output/font_exporter.hpp"


namespace monokakido
{
    std::expected<ExportResult, std::string> FontExporter::exportFont(const Font& font, const ExportOptions& options)
    {
        ExportResult result;
        result.totalResources = 1;

        auto fontDataResult = font.getData();
        if (!fontDataResult)
            return std::unexpected(std::format("Failed to get font data: {}", fontDataResult.error()));

        const auto fontData = *fontDataResult;
        if (fontData.empty())
        {
            result.skipped = 1;
            return result;
        }

        auto extension = font.detectType();
        if (!extension)
            return std::unexpected("Unrecognized font type");

        const fs::path outputDir = options.createSubdirectories
                                       ? options.outputDirectory / "fonts"
                                       : options.outputDirectory;

        const fs::path outputPath = outputDir / std::format("{}.{}", font.name(), extension.value());

        if (shouldSkipExisting(outputPath, options.overwriteExisting))
        {
            result.skipped = 1;
            return result;
        }

        if (auto writeResult = writeData(fontData, outputPath))
        {
            result.exported = 1;
            result.totalBytes = fontData.size();
        }
        else
        {
            result.failed = 1;
            result.errors.push_back(
                std::format("Font export failed: {}", writeResult.error()));
        }

        return result;

    }
}