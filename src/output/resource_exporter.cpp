//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#include "MKD/output/resource_exporter.hpp"

#include <pugixml.h>
#include <sstream>

namespace MKD
{
    namespace
    {
        const auto notify = [](const ExportEvent& event, const ExportCallback& callback) {
            if (callback)
                callback(event);
        };

        /*
         *  Determines a reporting interval
         */
        constexpr size_t updateInterval(const size_t totalItems)
        {
            return std::max<size_t>(1, totalItems / 400);
        }

        bool shouldReport(const size_t completed, const size_t total, const size_t interval)
        {
            return completed == total || completed % interval == 0;
        }
    }

    std::expected<ExportResult, std::string> ResourceExporter::exportAll(
        const Rsc& rsc, const ExportOptions& options, const ResourceType type)
    {
        ExportResult result;
        result.totalResources = rsc.size();

        if (result.totalResources == 0)
            return result;

        const size_t interval = updateInterval(result.totalResources);
        std::vector<uint8_t> buffer;

        for (const auto& [itemId, data] : rsc)
        {
            auto [subdir, filename, finalData] = prepareRscItem(itemId, data, options, buffer);
            fs::path outputDir = options.createSubdirectories
                                     ? options.outputDirectory / subdir
                                     : options.outputDirectory;

            if (const fs::path outputPath = outputDir / filename;
                shouldSkipExisting(outputPath, options.overwriteExisting))
            {
                result.skipped++;
            }
            else if (auto writeResult = writeData(finalData, outputPath))
            {
                result.exported++;
                result.totalBytes += finalData.size();
            }
            else
            {
                result.failed++;
                result.errors.push_back(std::format("Entry {}: {}", itemId, writeResult.error()));
            }

            if (const size_t completed = result.exported + result.skipped + result.failed;
                shouldReport(completed, result.totalResources, interval))
            {
                notify(ProgressEvent{
                           .type = type,
                           .completedItems = completed,
                           .totalItems = result.totalResources,
                           .bytesWritten = result.totalBytes
                       }, options.progressCallback);
            }
        }

        return result;
    }


    std::expected<ExportResult, std::string> ResourceExporter::exportAll(
        const Nrsc& nrsc, const ExportOptions& options, const ResourceType type)
    {
        ExportResult result;
        result.totalResources = nrsc.size();

        if (result.totalResources == 0)
            return result;

        const size_t interval = updateInterval(result.totalResources);

        for (const auto& [id, data] : nrsc)
        {
            const bool isAudio = id.find('.') == std::string_view::npos;

            fs::path outputDir = options.outputDirectory;
            if (options.createSubdirectories)
                outputDir /= isAudio ? "audio" : "graphics";

            if (const fs::path outputPath = outputDir / (isAudio ? std::format("{}.aac", id) : std::string(id));
                shouldSkipExisting(outputPath, options.overwriteExisting))
            {
                result.skipped++;
            }
            else if (auto writeResult = writeData(data, outputPath))
            {
                result.exported++;
                result.totalBytes += data.size();
            }
            else
            {
                result.failed++;
                result.errors.push_back(std::format("{}: {}", id, writeResult.error()));
            }

            if (const size_t completed = result.exported + result.skipped + result.failed;
                shouldReport(completed, result.totalResources, interval))
            {
                notify(ProgressEvent{
                           .type = type,
                           .completedItems = completed,
                           .totalItems = result.totalResources,
                           .bytesWritten = result.totalBytes
                       }, options.progressCallback);
            }
        }

        return result;
    }


    std::expected<ExportResult, std::string> ResourceExporter::exportFont(
        const Font& font, const ExportOptions& options)
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

        if (const fs::path outputPath = outputDir / std::format("{}.{}", font.name(), extension.value());
            shouldSkipExisting(outputPath, options.overwriteExisting))
        {
            result.skipped = 1;
        }
        else if (auto writeResult = writeData(fontData, outputPath))
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


    std::tuple<std::string, std::string, std::span<const uint8_t> > ResourceExporter::prepareRscItem(
        uint32_t itemId, std::span<const uint8_t> data, const ExportOptions& options, std::vector<uint8_t>& buffer)
    {
        buffer.clear();

        // Check if it's audio data
        if (isAudioData(data))
            return {"audio", std::format("{:010}.aac", itemId), data};

        // It's XML
        if (options.prettyPrintXml)
        {
            if (pugi::xml_document doc; doc.load_buffer(data.data(), data.size()))
            {
                std::ostringstream oss;
                doc.save(oss, "  ", pugi::format_default, pugi::encoding_utf8);
                std::string prettyXml = oss.str();
                buffer.assign(prettyXml.begin(), prettyXml.end());
                return {"entries", std::format("{:010}.xml", itemId), buffer};
            }
        }

        return {"entries", std::format("{:010}.xml", itemId), data};
    }


    bool ResourceExporter::isAudioData(const std::span<const uint8_t> data)
    {
        if (data.size() < 8)
            return false;

        // ADTS header (FF F1 or FF F9)
        if (data[0] == 0xFF && (data[1] == 0xF1 || data[1] == 0xF9))
            return true;

        return false;
    }
}
