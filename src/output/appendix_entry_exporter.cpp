//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "MKD/output/appendix_entry_exporter.hpp"
#include "unicode/unicode.hpp"

#include <format>
#include <fstream>
#include <unordered_map>

namespace MKD
{
    namespace
    {
        std::string displayName(const AppendixEntryList& list)
        {
            if (!list.title().empty())
                return std::string(list.title());

            return fs::path(list.filename()).stem().string();
        }

        std::string safeFilename(std::string name)
        {
            for (char& c : name)
            {
                const auto byte = static_cast<unsigned char>(c);
                if (byte < 0x20 || c == '/' || c == '\\' || c == ':')
                    c = '_';
            }

            while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
                name.pop_back();

            return name.empty() ? "entries" : name;
        }

        std::string outputStem(const AppendixEntryList& list, std::unordered_map<std::string, size_t>& seen)
        {
            const auto base = safeFilename(displayName(list));
            const size_t ordinal = ++seen[base];
            if (ordinal == 1)
                return base;

            return std::format("{}-{}", base, ordinal);
        }

        std::string entryIdString(const EntryId& id)
        {
            return std::format("{:06}-{:04X}", id.pageId, id.itemId);
        }

        std::string firstCharacterString(const std::optional<char16_t> firstCharacter)
        {
            if (!firstCharacter || *firstCharacter == u'\0')
                return {};

            const char16_t value = *firstCharacter;
            try
            {
                return detail::unicode::toUtf8(std::u16string_view(&value, 1));
            }
            catch (...)
            {
                return {};
            }
        }

        std::string headlineForEntryId(const std::span<const HeadlineStore> headlines, const EntryId& id)
        {
            for (const auto& store : headlines)
            {
                if (const auto components = store.componentsForEntryId(id))
                    return components->fullUtf8();
            }

            return {};
        }
    }


    Result<ExportResult> AppendixEntryExporter::exportEntryLists(
        std::span<const AppendixEntryList> lists,
        std::span<const HeadlineStore> headlines,
        const ExportOptions& options)
    {
        ExportResult combined;
        if (lists.empty())
            return combined;

        const fs::path dir = options.createSubdirectories
                                 ? options.outputDirectory / resourceTypeName(ResourceType::AppendixEntries)
                                 : options.outputDirectory;

        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec)
            return std::unexpected(std::format("Failed to create output directory: {}", ec.message()));

        std::unordered_map<std::string, size_t> seen;
        seen.reserve(lists.size());

        for (const auto& list : lists)
        {
            const fs::path path = dir / std::format("{}.tsv", outputStem(list, seen));
            if (shouldSkipExisting(path, options.overwriteExisting))
            {
                combined.totalResources += list.size();
                combined.skipped += list.size();
                continue;
            }

            if (auto result = exportEntryList(list, headlines, path))
            {
                combined += *result;
            }
            else
            {
                combined.failed++;
                combined.errors.push_back(std::format("{}: {}", list.filename(), result.error()));
            }
        }

        return combined;
    }


    Result<ExportResult> AppendixEntryExporter::exportEntryList(
        const AppendixEntryList& list,
        std::span<const HeadlineStore> headlines,
        const fs::path& path)
    {
        std::ofstream out(path, std::ios::binary);
        if (!out)
            return std::unexpected(std::format("Cannot open {}", path.string()));

        ExportResult result;
        result.totalResources = list.size();

        out << "entry_id\tlabel\tfirst_character\theadline\n";

        for (size_t i = 0; i < list.size(); ++i)
        {
            auto entry = list.entryAt(i);
            if (!entry)
            {
                result.failed++;
                result.errors.push_back(std::format("Entry {}: {}", i, entry.error()));
                continue;
            }

            const auto label = entry->label.value_or(std::string_view{});
            out << entryIdString(entry->entryId) << '\t'
                << label << '\t'
                << firstCharacterString(entry->firstCharacter) << '\t'
                << headlineForEntryId(headlines, entry->entryId) << '\n';

            if (!out)
                return std::unexpected("Failed to write TSV row");

            result.exported++;
        }

        result.totalBytes = static_cast<uint64_t>(out.tellp());
        return result;
    }
}
