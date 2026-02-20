//
// kiwakiwaaにより 2026/02/14 に作成されました。
//

#pragma once

#include "MKD/output/common.hpp"
#include "MKD/output/base_exporter.hpp"
#include "MKD/resource/keystore/keystore.hpp"

#include <expected>
#include <string>

namespace MKD
{
    [[nodiscard]] constexpr KeystoreExportMode operator|(KeystoreExportMode a, KeystoreExportMode b) noexcept
    {
        return static_cast<KeystoreExportMode>(
            static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    [[nodiscard]] constexpr bool hasFlag(KeystoreExportMode mode, KeystoreExportMode flag) noexcept
    {
        return (static_cast<uint8_t>(mode) & static_cast<uint8_t>(flag)) != 0;
    }


    class KeystoreExporter : BaseExporter
    {
    public:
        /**
         * Export a keystore's mappings to TSV files.
         *
         * Forward format (one line per key):
         *   <key>\t<page>-<item>[,<page>-<item>...]
         *
         * Inverse format (one line per page reference):
         *   <page>-<item>\t<key>[,<key>,...]
         *
         * The inverse map is built with a hash map over packed
         * page references, then sorted by page ID
         */
        static std::expected<ExportResult, std::string> exportKeystore(
            const Keystore& keystore,
            const ExportOptions& options);

    private:
        struct ForwardEntry
        {
            std::string_view key;
            std::vector<PageReference> pages;
        };

        /// Walk the entire index once, collecting all forward entries.
        static std::expected<std::vector<ForwardEntry>, std::string> collectEntries(
            const Keystore& keystore, KeystoreIndex index);

        /// Write forward mapping TSV. Streams directly to disk.
        static std::expected<ExportResult, std::string> writeForward(
            std::span<const ForwardEntry> entries, const fs::path& path);

        /// Build inverse map from pre-collected entries and write TSV.
        /// Uses packed uint64 keys
        static std::expected<ExportResult, std::string> writeInverse(
            std::span<const ForwardEntry> entries, const fs::path& path);
    };
}
