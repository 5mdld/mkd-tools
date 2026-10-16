//
// kiwakiwaaにより 2026/02/14 に作成されました。
//

#include "MKD/output/headline_exporter.hpp"
#include "unicode/unicode.hpp"

#include <format>
#include <fstream>

namespace MKD
{
    Result<ExportResult> HeadlineExporter::exportHeadlines(const HeadlineStore& store, const ExportOptions& options)
    {
        ExportResult result;
        result.totalResources = store.size();

        if (result.totalResources == 0)
            return result;

        const fs::path dir = options.createSubdirectories
                                 ? options.outputDirectory / "Headline"
                                 : options.outputDirectory;
        fs::path outputPath = dir / std::format("{}.tsv", store.filename());

        if (options.createSubdirectories)
            fs::create_directories(dir);

        if (shouldSkipExisting(outputPath, options.overwriteExisting))
        {
            result.skipped = result.totalResources;
            return result;
        }

        std::ofstream out(outputPath, std::ios::binary);
        if (!out)
        {
            return std::unexpected(
                std::format("Failed to create output file: {}", outputPath.string())
            );
        }

        for (auto entry : store.entries())
        {
            if (!entry)
            {
                ++result.failed;
                result.errors.push_back(entry.error());
                continue;
            }
            const auto& components = *entry;
            if (auto rowResult = writeRow(out, components); !rowResult)
            {
                result.failed++;
                result.errors.push_back(
                    std::format("Entry {}-{}: {}",
                                components.entryId.pageId,
                                components.entryId.itemId,
                                rowResult.error()));
            }
            else
            {
                result.exported++;
                result.totalBytes += *rowResult;
            }
        }

        return result;
    }


    Result<size_t> HeadlineExporter::writeRow(std::ofstream& out, const HeadlineComponents& components)
    {
        auto row = std::format("{:06}-{:04X}\t", components.entryId.pageId, components.entryId.itemId);
        for (const auto field : {
                 std::u16string_view(components.headline), std::u16string_view(components.prefix),
                 std::u16string_view(components.suffix)
             })
        {
            if (field.empty())
                continue;
            row.append(detail::unicode::toUtf8(field));
            row.push_back('\t');
        }
        row.push_back('\n');
        out.write(row.data(), static_cast<std::streamsize>(row.size()));
        if (!out)
            return std::unexpected("Failed to write TSV row");
        return row.size();
    }
}
