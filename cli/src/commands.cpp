//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/commands.hpp"
#include "MKDCLI/format.hpp"
#include "MKDCLI/progress.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace MKDCLI
{
    int runCommand(const MKD::DictionarySource& source, const ListCommand&)
    {
        auto available = source.findAllAvailable();
        if (!available)
        {
            std::cerr << "Error: " << available.error() << "\n";
            return 1;
        }

        if (available->empty())
        {
            std::cout << "No dictionaries found.\n";
            return 0;
        }

        std::ranges::sort(*available, [](const auto& a, const auto& b)
        {
            return a.id < b.id;
        });

        std::cout << "Available dictionary products (" << available->size() << " found)\n\n";

        std::vector<ProductRow> rows;
        rows.reserve(available->size());
        for (const auto& [productId, path] : *available)
        {
            auto meta = MKD::DictionaryMetadata::loadFromPath(
                path / "Contents" / (path.stem().string() + ".json")
            );

            if (!meta)
            {
                rows.push_back({
                    .id = productId,
                    .name = "(metadata unavailable)",
                    .genre = {},
                });
                continue;
            }

            rows.push_back({
                .id = productId,
                .name = meta->displayName ? meta->displayName->resolve() : "-",
                .genre = meta->genre ? meta->genre->resolve() : "",
            });
        }

        printProductList(std::cout, rows);
        std::cout << "\n";
        return 0;
    }


    int runCommand(const MKD::DictionaryProduct& product, const InfoCommand& command)
    {
        auto printField = [](std::string_view label, std::string_view value, size_t indent = 2)
        {
            std::cout << std::string(indent, ' ') << label << "  "
                << (value.empty() ? "-" : value) << "\n";
        };

        const auto& meta = product.metadata();

        // product info
        std::cout << command.productId << "\n\n";

        printField("Name:       ", meta.displayName.value_or(MKD::LocalizedString{}).resolve());
        printField("Description:", meta.description.value_or(MKD::LocalizedString{}).resolve());
        printField("Category:   ", meta.category.value_or("-"));
        printField("Version:    ", meta.version.value_or("-"));

        // content info
        const auto& dicts = product.dictionaries();
        std::cout << "\n  Contents: (" << dicts.size() << ")\n";

        for (const auto& dict : dicts)
        {
            const auto& content = dict.content();

            std::cout << "\n    " << content.identifier << "\n";

            printField("Title:    ", content.title.value_or(MKD::LocalizedString{}).resolve(), 6);
            printField("Publisher:", content.publisher.value_or(MKD::LocalizedString{}).resolve(), 6);
            printField("Edition:  ", content.edition.value_or(MKD::LocalizedString{}).resolve(), 6);
            printField("Directory:", content.directory, 6);

            // Resource availability for this content
            std::cout << "\n      Resources:\n";

            auto resourceLine = [](std::string_view name, const size_t count)
            {
                std::cout << "        " << (count > 0 ? "●" : "○")
                    << " " << name;
                if (count > 0)
                    std::cout << " (" << count << ")";
                std::cout << "\n";
            };

            resourceLine("Entries", dict.resourceCount(MKD::ResourceType::Contents));
            resourceLine("Appendix entries", dict.resourceCount(MKD::ResourceType::AppendixEntries));
            resourceLine("Audio", dict.resourceCount(MKD::ResourceType::Audio));
            resourceLine("Graphics", dict.resourceCount(MKD::ResourceType::Graphics));
            resourceLine("Fonts", dict.resourceCount(MKD::ResourceType::Fonts));
            resourceLine("Keystores", dict.resourceCount(MKD::ResourceType::Keystores));
            resourceLine("Headlines", dict.resourceCount(MKD::ResourceType::Headlines));
        }

        std::cout << "\n";
        return 0;
    }


    int runCommand(const MKD::DictionaryProduct& product, const ExportCommand& command)
    {
        const auto& dicts = product.dictionaries();
        if (dicts.empty())
        {
            std::cerr << "Error: No content entries found in product.\n";
            return 1;
        }

        MKD::ExportResult totalResult;
        bool anyFailed = false;

        for (const auto& dict : dicts)
        {
            std::cerr << "Exporting content: " << dict.id() << "\n";

            auto options = command.options;

            // separate folders only when the product has multiple contents
            if (dicts.size() > 1)
                options.outputDirectory /= dict.id();

            ExportProgress progress;
            options.progressCallback = [&progress](const MKD::ExportEvent& event)
            {
                progress.onEvent(event);
            };

            auto result = dict.exportWithOptions(options);

            progress.finish();
            totalResult += result;

            if (!result.isSuccess())
                anyFailed = true;
        }

        if (dicts.size() > 1)
            std::cerr << "\nCombined results:\n";

        printExportSummary(std::cerr, totalResult);

        return anyFailed ? 1 : 0;
    }
}
