

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace MKD
{
    /**
     * Appendix entry list (.entries)
     * =============================
     *
     * Entry-list appendix files are raw entry matrices.
     *
     * File Structure:
     * ┌──────────────────────────────────────────────────────────────┐
     * │ Header                                                       │
     * │  - count                       (4 bytes, uint32 LE)          │
     * │  - rows                        (4 bytes, uint32 LE)          │
     * │  - columns                     (4 bytes, uint32 LE)          │
     * │  - reserved[4]                 (uint32[4])                   │
     * │  - labelTableOffset            (uint32, 0 if absent)         │
     * │  - firstCharacterTableOffset   (uint32, 0 if absent)         │
     * ├──────────────────────────────────────────────────────────────┤
     * │ Records Array (count × 8 bytes)                              │
     * │  - pageID                      (4 bytes, uint32 LE)          │
     * │  - itemID                      (4 bytes, uint16 LE)          │
     * │  - dictionaryID                (4 bytes, uint16 LE)          │
     * ├──────────────────────────────────────────────────────────────┤
     * │ Strings Region (variable length)                             │
     * │  Null-terminated UTF-16LE strings, referenced by byte offset │
     * └──────────────────────────────────────────────────────────────┘
     * The dictionary id is present in the file because the application can
     * route entries across product contents.
     *
     * - labelTableOffset points to count uint32 offsets. Each offset is from
     *   the start of the file to a null-terminated UTF-8 label.
     *
     * - firstCharacterTableOffset points to count 4-byte slots. The first two
     *   bytes of each slot are a UTF-16 code unit.
     */

    struct AppendixEntry
    {
        EntryId entryId;
        std::optional<std::string_view> label;
        std::optional<char16_t> firstCharacter;
    };

    class AppendixEntryList
    {
    public:
        static Result<AppendixEntryList> load(const fs::path& filePath, std::string title = {});

        ~AppendixEntryList();

        AppendixEntryList(AppendixEntryList&&) noexcept;
        AppendixEntryList& operator=(AppendixEntryList&&) noexcept;

        AppendixEntryList(const AppendixEntryList&) = delete;
        AppendixEntryList& operator=(const AppendixEntryList&) = delete;

        [[nodiscard]] size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] uint32_t rows() const noexcept;

        [[nodiscard]] uint32_t columns() const noexcept;

        [[nodiscard]] bool hasLabels() const noexcept;

        [[nodiscard]] bool hasFirstCharacters() const noexcept;

        [[nodiscard]] std::string_view filename() const noexcept;

        [[nodiscard]] std::string_view title() const noexcept;

        [[nodiscard]] Result<AppendixEntry> entryAt(size_t index) const;

    private:
        struct Impl;

        explicit AppendixEntryList(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> impl_;
    };
}
