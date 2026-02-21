//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/commands.hpp"
#include "MKDCLI/format.hpp"
#include "MKDCLI/progress.hpp"

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

        resourceLine("Entries", dict->resourceCount(MKD::ResourceType::Entries));
        resourceLine("Audio", dict->resourceCount(MKD::ResourceType::Audio));
        resourceLine("Graphics", dict->resourceCount(MKD::ResourceType::Graphics));
        resourceLine("Fonts", dict->resourceCount(MKD::ResourceType::Fonts));
        resourceLine("Keystores", dict->resourceCount(MKD::ResourceType::Keystores));
        resourceLine("Headlines", dict->resourceCount(MKD::ResourceType::Headlines));

        std::cerr << "\n";
        return 0;
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

        auto options = exportOpts;
        ExportProgress progress;
        options.progressCallback = [&progress](const MKD::ExportEvent& event) {
            progress.onEvent(event);
        };

        auto total = dict->exportWithOptions(options);

        progress.finish();
        printExportSummary(total);

        return total.isSuccess() ? 0 : 1;
    }
}
