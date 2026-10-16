//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#include "MKD/output/resource_exporter.hpp"
#include "export_accumulator.hpp"
#include "platform/parallel.hpp"

#include <format>
#include <fstream>
#include <pugixml.h>
#include <sstream>

namespace MKD
{
    namespace
    {
        std::error_code createOutputDirs(const ExportOptions& options, const ResourceType type)
        {
            std::error_code ec;
            fs::create_directories(options.outputDirectory / resourceTypeName(type), ec);
            return ec;
        }

        fs::path buildOutputDir(const ExportOptions& options, const ResourceType type)
        {
            if (options.createSubdirectories)
                return options.outputDirectory / resourceTypeName(type);
            return options.outputDirectory;
        }

        template <typename F>
        void forEachItem(const size_t count, F&& fn)
        {
#if defined(__APPLE__) || defined(__linux__)
            parallelFor(count, std::forward<F>(fn));
#else
            for (size_t i = 0; i < count; ++i)
                fn(i);
#endif
        }
    }


    Result<ExportResult> ResourceExporter::exportAll(const ResourceStore& resourceStore, const ExportOptions& options,
                                                     const ResourceType type)
    {
        if (resourceStore.empty())
            return ExportResult{};

        if (const auto ec = createOutputDirs(options, type))
            return std::unexpected(std::format("Failed to create output directories: {}", ec.message()));

        const auto ext = type == ResourceType::Contents ? ".xml" : ".aac";
        const auto baseDir = buildOutputDir(options, type);
        const bool shouldFormat = type == ResourceType::Contents && options.prettyPrintXml;
        ExportAccumulator acc(resourceStore.size(), type, options.progressCallback);

        forEachItem(resourceStore.size(), [&](const size_t i)
        {
            auto id = resourceStore.idAt(i);
            if (!id)
            {
                acc.recordFailure(id.error());
                return;
            }
            const auto path = baseDir / std::format("{:06}{}", *id, ext);
            if (shouldSkipExisting(path, options.overwriteExisting))
            {
                acc.recordSkip();
                return;
            }
            auto item = resourceStore.itemAt(i);
            if (!item)
            {
                acc.recordFailure(item.error());
                return;
            }

            std::vector<uint8_t> buf;
            const auto output = formatIfNeeded(item->data.span(), buf, shouldFormat);

            if (auto r = writeData(output, path))
                acc.recordExport(output.size());
            else
                acc.recordFailure(std::format("Entry {}: {}", item->itemId, r.error()));
        });

        return acc.finalize();
    }


    Result<ExportResult> ResourceExporter::exportAll(
        const NamedResourceStore& namedResourceStore, const ExportOptions& options, const ResourceType type)
    {
        if (namedResourceStore.empty())
            return ExportResult{};

        if (const auto ec = createOutputDirs(options, type))
            return std::unexpected(std::format("Failed to create output directories: {}", ec.message()));

        const auto baseDir = buildOutputDir(options, type);
        ExportAccumulator acc(namedResourceStore.size(), type, options.progressCallback);

        forEachItem(namedResourceStore.size(), [&](const size_t i)
        {
            auto id = namedResourceStore.idAt(i);
            if (!id)
            {
                acc.recordFailure(std::format("Index {}: {}", i, id.error()));
                return;
            }
            const bool isAudio = id->find('.') == std::string_view::npos;
            const auto filename = isAudio ? std::format("{}.aac", *id) : std::string(*id);
            const auto outputPath = baseDir / filename;
            if (shouldSkipExisting(outputPath, options.overwriteExisting))
            {
                acc.recordSkip();
                return;
            }
            auto item = namedResourceStore.itemAt(i);
            if (!item)
                acc.recordFailure(std::format("Index {}: {}", i, item.error()));
            else if (auto result = writeData(item->data.span(), outputPath))
                acc.recordExport(item->data.size());
            else
                acc.recordFailure(std::format("{}: {}", *id, result.error()));
        });

        return acc.finalize();
    }


    Result<ExportResult> ResourceExporter::exportFont(
        const Font& font, const ExportOptions& options)
    {
        ExportResult result;
        result.totalResources = 1;

        if (const auto ec = createOutputDirs(options, ResourceType::Fonts))
            return std::unexpected(std::format("Failed to create output directories: {}", ec.message()));

        if (font.empty())
        {
            result.skipped = 1;
            return result;
        }
        auto extension = font.detectType();
        if (!extension)
            return std::unexpected(extension.error());
        if (!*extension)
            return std::unexpected("Unrecognised font type");
        const auto outputPath = buildOutputDir(options, ResourceType::Fonts) /
            std::format("{}.{}", font.name(), **extension);
        if (shouldSkipExisting(outputPath, options.overwriteExisting))
        {
            result.skipped = 1;
            return result;
        }
        std::ofstream output(outputPath, std::ios::binary);
        if (!output)
            return std::unexpected(std::format("Cannot open font output: {}", outputPath.string()));
        auto written = font.writeTo(output);
        output.flush();
        if (!written || !output)
        {
            result.failed = 1;
            result.errors.push_back(written ? "Failed to flush font output" : written.error());
        }
        else
        {
            result.exported = 1;
            result.totalBytes = *written;
        }

        return result;
    }


    std::span<const uint8_t> ResourceExporter::formatIfNeeded(const std::span<const uint8_t> data,
                                                              std::vector<uint8_t>& buf, bool shouldFormat)
    {
        if (shouldFormat)
        {
            buf = prettyPrintXml(data);
            if (!buf.empty())
                return buf;
        }
        return data;
    }


    std::vector<uint8_t> ResourceExporter::prettyPrintXml(const std::span<const uint8_t> data)
    {
        pugi::xml_document doc;
        if (!doc.load_buffer(data.data(), data.size()))
            return {};

        std::ostringstream oss;
        doc.save(oss, "  ", pugi::format_default, pugi::encoding_utf8);
        std::string str = oss.str();
        return {str.begin(), str.end()};
    }
}
