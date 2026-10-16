//
// kiwakiwaaにより 2026/01/19 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "binary/binary_view.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "resource_store_index_record.hpp"
#include "resource_store_map.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

namespace MKD
{
    /**
     * Resource Store Index File Format (contents.idx + contents.map)
     *
     * The RSC index system uses a two files to map item IDs to their
     * locations in .rsc data files. The design separates ID management from
     * location data.
     *
     * File Structure Overview:
     *
     * contents.idx (Optional - ID to Index mapping):
     * ┌─────────────────────────────────────────────────────────────────┐
     * │ Header (8 bytes)                                                │
     * │  ├─ length (4 bytes): Number of IdxRecords                      │
     * │  └─ padding (4 bytes): unused                                   │
     * ├─────────────────────────────────────────────────────────────────┤
     * │ IdxRecords Array (length × 8 bytes)                             │
     * │  Each record contains:                                          │
     * │   ├─ itemId (4 bytes): Custom item ID                           │
     * │   └─ mapIdx (4 bytes): Index into contents.map                  │
     * └─────────────────────────────────────────────────────────────────┘
     *
     * contents.map (Required - Index to Location mapping)
     * ┌─────────────────────────────────────────────────────────────────┐
     * │ Header (8 bytes)                                                │
     * │  ├─ version (4 bytes): Format version                           │
     * │  └─ recordCount (4 bytes): Number of MapRecords                 │
     * ├─────────────────────────────────────────────────────────────────┤
     * │ MapRecords Array (recordCount × 8 bytes)                        │
     * │  Each record contains:                                          │
     * │   ├─ zOffset (4 bytes): Global offset to compressed chunk       │
     * │   └─ ioffset (4 bytes): Offset within decompressed chunk        │
     * └─────────────────────────────────────────────────────────────────┘
     *
     * Dictionaries typical have both .idx and a .map
     * Sequential data like fonts only have a .map file
     *
     * Lookup Process:
     *
     * with idx file:
     * ┌─────────┐     ┌─────────────┐     ┌─────────────┐     ┌──────────┐
     * │ Item ID │ --> │ contents.idx│ --> │ Map Index   │ --> │ Location │
     * │  12345  │     │ IdxRecord   │     │      7      │     │  in .rsc │
     * └─────────┘     └─────────────┘     └─────────────┘     └──────────┘
     *                  Binary search       Direct access       via MapRecord
     *
     * only map file:
     * ┌─────────┐     ┌─────────────┐     ┌──────────┐
     * │ Item ID │ --> │ Map Index   │ --> │ Location │
     * │    7    │     │   (same)    │     │  in .rsc │
     * └─────────┘     └─────────────┘     └──────────┘
     *                  Direct use          via MapRecord
     *
     */
    struct ResourceStoreIndexHeader
    {
        uint32_t length; // Number of IdxRecords in the file
        uint32_t padding;
    };


    class ResourceStoreIndex
    {
    public:
        static Result<ResourceStoreIndex> open(const std::filesystem::path& directory);
        [[nodiscard]] Result<ResourceStoreMapRecord> findById(uint32_t id) const;
        [[nodiscard]] Result<std::pair<uint32_t, ResourceStoreMapRecord>> itemAt(size_t index) const;
        [[nodiscard]] Result<size_t> indexOf(uint32_t id) const;
        [[nodiscard]] uint32_t mapVersion() const noexcept { return version_; }
        [[nodiscard]] size_t size() const noexcept { return (indexFile_ ? index_ : locations_).bytes().size() / 8; }
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        [[nodiscard]] auto items() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return itemAt(index); });
        }

        auto items() const && = delete;

    private:
        struct Map
        {
            uint32_t version;
            detail::BinaryView locations;
        };

        static Result<Map> readMap(detail::BinaryView data);
        static Result<detail::BinaryView> readIdIndex(detail::BinaryView data);
        static Result<std::optional<MappedFile>> openIndex(const std::filesystem::path& directory);
        static Result<void> validateMappings(detail::BinaryView locations, detail::BinaryView index);
        [[nodiscard]] uint32_t idAt(size_t index) const;

        uint32_t version_ = 0;
        std::optional<MappedFile> mapFile_;
        std::optional<MappedFile> indexFile_;
        detail::BinaryView locations_;
        detail::BinaryView index_;
    };
}
