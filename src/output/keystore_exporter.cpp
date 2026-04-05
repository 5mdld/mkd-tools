//
// kiwakiwaaにより 2026/02/14 に作成されました。
//

#include "MKD/output/keystore_exporter.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <unordered_set>

namespace MKD
{
    Result<ExportResult> KeystoreExporter::exportKeystore(const Keystore& keystore, const ExportOptions& options)
    {
        auto entries = collectEntries(keystore, KeystoreIndex::Prefix);
        if (!entries)
            return std::unexpected(entries.error());

        ExportResult combined;

        const fs::path dir = options.createSubdirectories
                                 ? options.outputDirectory / "Keystore"
                                 : options.outputDirectory;

        if (hasFlag(options.keystoreExportMode, KeystoreExportMode::Forward))
        {
            const fs::path path = dir / std::format("{}.tsv", keystore.filename());
            if (shouldSkipExisting(path, options.overwriteExisting))
            {
                combined.skipped++;
            }
            else if (auto r = writeForward(*entries, path))
            {
                combined += *r;
            }
            else
            {
                return std::unexpected(std::format("Forward export failed: {}", r.error()));
            }
        }

        if (hasFlag(options.keystoreExportMode, KeystoreExportMode::Inverse))
        {
            const fs::path path = dir / std::format("{}_inverse.tsv", keystore.filename());
            if (shouldSkipExisting(path, options.overwriteExisting))
            {
                combined.skipped++;
            }
            else if (auto r = writeInverse(keystore, *entries, path))
            {
                combined += *r;
            }
            else
            {
                return std::unexpected(std::format("Inverse export failed: {}", r.error()));
            }
        }

        return combined;
    }


    Result<std::vector<KeystoreExporter::ForwardEntry>> KeystoreExporter::collectEntries(const Keystore& keystore, const KeystoreIndex index)
    {
        const size_t count = keystore.indexSize(index);
        std::vector<ForwardEntry> entries;
        entries.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            auto entry = keystore.entryAt(index, i);
            if (!entry)
                return std::unexpected(std::format("Entry {}: {}", i, entry.error()));

            auto entryIds = keystore.entryIdsAt(index, i);
            if (!entryIds)
                return std::unexpected(std::format("Entry {} references: {}", i, entryIds.error()));

            entries.push_back({
                .key = entry->key,
                .entryIds = std::move(*entryIds),
            });
        }

        return entries;
    }


    Result<ExportResult> KeystoreExporter::writeForward(std::span<const ForwardEntry> entries, const fs::path& path)
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec)
            return std::unexpected(std::format("Cannot create directory: {}", ec.message()));

        std::ofstream out(path, std::ios::binary);
        if (!out)
            return std::unexpected(std::format("Cannot open {}", path.string()));

        ExportResult result;
        result.totalResources = entries.size();

        for (const auto& [key, entryIds] : entries)
        {
            out << key << '\t';
            for (size_t i = 0; i < entryIds.size(); ++i)
            {
                if (i > 0) out << '\t';
                out << entryIds[i].pageId << '-' << entryIds[i].itemId;
            }
            out << '\n';
            result.exported++;
        }

        result.totalBytes = static_cast<uint64_t>(out.tellp());
        return result;
    }


    Result<ExportResult> KeystoreExporter::writeInverse(const Keystore& keystore, std::span<const ForwardEntry> entries, const fs::path& path)
    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
        if (ec)
            return std::unexpected(std::format("Cannot create directory: {}", ec.message()));

        // Pack (page, item) into a single uint64
        constexpr auto pack = [](const EntryId& ref) -> uint64_t {
            return (static_cast<uint64_t>(ref.pageId) << 16) | ref.itemId;
        };

        std::unordered_set<uint64_t> seen;
        seen.reserve(entries.size());
        std::vector<uint64_t> sorted;
        sorted.reserve(entries.size());

        for (const auto& entry : entries)
        {
            for (const auto& ref : entry.entryIds)
            {
                if (const auto packed = pack(ref); seen.insert(packed).second)
                    sorted.push_back(packed);
            }
        }

        std::ranges::sort(sorted);

        std::ofstream out(path, std::ios::binary);
        if (!out)
            return std::unexpected(std::format("Cannot open {}", path.string()));

        ExportResult result;
        result.totalResources = sorted.size();

        for (const auto packed : sorted)
        {
            const auto page = static_cast<uint32_t>(packed >> 16);
            const auto entry = static_cast<uint16_t>(packed & 0xFFFF);
            const EntryId id{.pageId = page, .itemId = entry};
            const auto keys = keystore.keysForEntry(id);

            out << std::format("{:06}-{:04X}", page, entry) <<'\t';
            for (size_t i = 0; i < keys.size(); ++i)
            {
                if (i > 0) out << '\t';
                out << keys[i];
            }
            out << '\n';
            result.exported++;
        }

        result.totalBytes = static_cast<uint64_t>(out.tellp());
        return result;
    }
}
