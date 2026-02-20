//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/commands.hpp"
#include "MKDCLI/format.hpp"
#include "MKD/dictionary/dictionary.hpp"

#include <algorithm>
#include <iostream>

namespace MKDCLI
{
    int runList(MKD::DictionarySource& source, const CLIOptions& opts)
    {
        auto available = source.findAllAvailable();
        if (!available)
        {
            std::cerr << Colour::red("Error: ") << available.error() << "\n";
            return 1;
        }

        if (available->empty())
        {
            std::cerr << "No dictionaries found.\n";
            return 0;
        }

        std::ranges::sort(*available, [](const auto& a, const auto& b) {
            return a.id < b.id;
        });

        std::cerr << Colour::bold("Available dictionaries")
                << Colour::dim(" (" + std::to_string(available->size()) + " found)") << "\n\n";

        std::vector<TableRow> rows;
        for (const auto& [id, path] : *available)
        {
            TableRow row;
            row.cells.push_back(Colour::cyan(id));

            if (auto dict = MKD::Dictionary::open(id, source))
            {
                const auto& meta = dict->metadata();
                row.cells.push_back(meta.displayName().value_or("-"));
                row.cells.push_back(Colour::dim(meta.publisher().value_or("")));
            }
            else
            {
                row.cells.push_back(Colour::dim("(metadata unavailable)"));
                row.cells.emplace_back("");
            }

            rows.push_back(std::move(row));
        }

        printTable(rows);
        std::cerr << "\n";
        return 0;
    }


    int runInfo(const MKD::DictionarySource& source, const CLIOptions& opts)
    {
        auto dict = MKD::Dictionary::open(opts.dictId, source);
        if (!dict)
        {
            std::cerr << Colour::red("Error: ") << dict.error() << "\n";
            return 1;
        }

        const auto& meta = dict->metadata();

        std::cerr << Colour::bold(opts.dictId) << "\n\n";

        auto printField = [](std::string_view label, const std::optional<std::string>& value) {
            std::cerr << "  " << Colour::dim(std::string(label)) << "  "
                    << value.value_or("-") << "\n";
        };

        printField("Name:       ", meta.displayName());
        printField("Publisher:  ", meta.publisher());
        printField("Description:", meta.description());
        printField("Identifier: ", meta.contentIdentifier());

        // Resource availability
        std::cerr << "\n  " << Colour::dim("Resources:") << "\n";

        auto resourceLine = [](std::string_view name, const bool available) {
            std::cerr << "    " << (available ? Colour::green("●") : Colour::dim("○"))
                    << " " << name << "\n";
        };

        resourceLine("Entries", true);
        resourceLine("Audio", dict->hasAudio());
        resourceLine("Graphics", dict->hasGraphics());
        resourceLine("Fonts", dict->hasFonts());
        resourceLine("Keystores", dict->hasKeystores());
        resourceLine("Headlines", dict->hasHeadlines());

        std::cerr << "\n";
        return 0;
    }


    namespace
    {
        bool shouldExport(const std::vector<ResourceType>& filter, const ResourceType type)
        {
            if (filter.empty()) return true;
            return std::ranges::find(filter, type) != filter.end();
        }

        // Run a single export step, accumulate results, print status.
        template<typename ExportFn>
        void runExportStep(
            std::string_view label,
            ExportFn&& fn,
            MKD::ExportResult& total,
            bool verbose)
        {
            if (verbose)
                printSectionHeader(label);

            auto result = fn();
            if (!result)
            {
                std::cerr << Colour::red("  Failed: ") << result.error() << "\n";
                MKD::ExportResult err;
                err.errors.push_back(result.error());
                total += err;
                return;
            }

            total += *result;

            if (result->exported > 0 && verbose)
            {
                std::cerr << "  " << Colour::green(std::to_string(result->exported))
                        << " exported";
                if (result->skipped > 0)
                    std::cerr << ", " << Colour::yellow(std::to_string(result->skipped))
                            << " skipped";
                if (result->failed > 0)
                    std::cerr << ", " << Colour::red(std::to_string(result->failed))
                            << " failed";
                std::cerr << "\n";
            }
            else if (result->exported == 0 && result->skipped == 0)
            {
                std::cerr << Colour::dim("  (none)") << "\n";
            }
        }
    }


    int runExport(const MKD::DictionarySource& source, const CLIOptions& opts,
                  const MKD::ExportOptions& exportOpts)
    {
        auto dict = MKD::Dictionary::open(opts.dictId, source);
        if (!dict)
        {
            std::cerr << Colour::red("Error: ") << dict.error() << "\n";
            return 1;
        }

        if (opts.verbose)
            std::cerr << Colour::bold("Exporting " + opts.dictId) << " → " << exportOpts.outputDirectory.string() << "\n\n";

        const auto& filter = opts.onlyResources;
        MKD::ExportResult total;

        if (shouldExport(filter, ResourceType::Entries))
        {
            runExportStep("Entries", [&] { return dict->exportEntries(exportOpts); },
                          total, opts.verbose);
        }

        if (shouldExport(filter, ResourceType::Audio) && dict->hasAudio())
        {
            runExportStep("Audio", [&] { return dict->exportAudio(exportOpts); },
                          total, opts.verbose);
        }

        if (shouldExport(filter, ResourceType::Graphics) && dict->hasGraphics())
        {
            runExportStep("Graphics", [&] { return dict->exportGraphics(exportOpts); },
                          total, opts.verbose);
        }

        if (shouldExport(filter, ResourceType::Fonts) && dict->hasFonts())
        {
            runExportStep("Fonts", [&] { return dict->exportFonts(exportOpts); },
                          total, opts.verbose);
        }

        if (shouldExport(filter, ResourceType::Keystores) && dict->hasKeystores())
        {
            runExportStep("Keystores", [&] { return dict->exportKeystores(exportOpts); },
                          total, opts.verbose);
        }

        if (shouldExport(filter, ResourceType::Headlines) && dict->hasHeadlines())
        {
            runExportStep("Headlines", [&] { return dict->exportHeadlines(exportOpts); },
                          total, opts.verbose);
        }

        if (opts.verbose)
            std::cerr << "\n";

        printExportSummary(total);

        return total.isSuccess() ? 0 : 1;
    }
}
