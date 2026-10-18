//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "binary/binary_view.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "named_resource_store_index_record.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace MKD
{
    /**
     * Named Resource Store Index file format (.nidx)
     *
     * The index.nidx file acts as a table of contents for .nrsc resource files.
     * It maps string IDs to their locations in the numbered resource files.
     *
     * File Structure:
     * ┌─────────────────────────────────────────────────────────────┐
     * │ Header (8 bytes)                                            │
     * │  - Zero field (4 bytes)                                     │
     * │  - Record count (4 bytes, little-endian uint32)             │
     * ├─────────────────────────────────────────────────────────────┤
     * │ Records Array (count × 16 bytes)                            │
     * │  Each record contains:                                      │
     * │   - Compression format (2 bytes)                            │
     * │   - File sequence number (2 bytes) - which .nrsc file       │
     * │   - ID string offset (4 bytes) - offset into strings region │
     * │   - File offset (4 bytes) - offset within the .nrsc file    │
     * │   - Data length (4 bytes) - size of resource data           │
     * ├─────────────────────────────────────────────────────────────┤
     * │ ID Strings Region (variable length)                         │
     * │  - Null-terminated strings concatenated together            │
     * │  - Referenced by idStringOffset in records                  │
     * └─────────────────────────────────────────────────────────────┘
     *
     * Example:
     * - Record with fileSequence=0, fileOffset=1024 -> points to byte 1024 in "0.nrsc"
     * - Record with idStringOffset=50 -> string starts at byte 50 in strings region
     */





    class NamedResourceStoreIndex
    {
    public:
        static Result<NamedResourceStoreIndex> open(const std::filesystem::path& directory);
        [[nodiscard]] Result<NamedResourceStoreIndexRecord> findById(std::string_view id) const;
        [[nodiscard]] Result<std::pair<std::string_view, NamedResourceStoreIndexRecord>> itemAt(size_t index) const;
        [[nodiscard]] Result<std::string_view> idAt(size_t index) const;
        [[nodiscard]] Result<size_t> indexOf(std::string_view id) const;
        [[nodiscard]] size_t size() const noexcept { return records_.bytes().size() / 16; }
        [[nodiscard]] bool empty() const noexcept { return size() == 0; }

        [[nodiscard]] auto items() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return itemAt(index); });
        }

        auto items() const && = delete;

    private:
        struct Layout
        {
            detail::BinaryView records;
            detail::BinaryView strings;
            size_t stringsOffset;
        };

        static Result<Layout> readLayout(detail::BinaryView data);
        static Result<NamedResourceStoreIndexRecord> readRecord(detail::BinaryView data);
        static Result<std::string_view> readId(const Layout& layout, uint32_t offset);
        static Result<void> validateRecords(const Layout& layout);
        static Result<NamedResourceStoreIndex> read(MappedFile file);
        explicit NamedResourceStoreIndex(MappedFile file, const Layout& layout);
        [[nodiscard]] std::string_view idFor(const NamedResourceStoreIndexRecord& record) const;
        MappedFile file_;
        detail::BinaryView records_;
        detail::BinaryView strings_;
        size_t stringsOffset_ = 0;
    };
}
