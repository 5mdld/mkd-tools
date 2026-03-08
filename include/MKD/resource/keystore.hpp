//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/common.hpp"
#include "MKD/resource/page_reference.hpp"
#include "MKD/resource/keystore_lookup_result.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace MKD
{
    /**
     * Keystore File Format (.keystore)
     * =================================
     *
     * Keystores map search terms to locations within dictionary content files.
     *
     * File Structure:
     * ┌─────────────────────────────────────────────────────────┐
     * │ File Header                                             │
     * ├─────────────────────────────────────────────────────────┤
     * │ Words Section                                           │
     * ├─────────────────────────────────────────────────────────┤
     * │ Index Section                                           │
     * ├─────────────────────────────────────────────────────────┤
     * │ Conversion Table (optional, v2 only)                    │
     * └─────────────────────────────────────────────────────────┘
     *
     * FILE HEADER
     * -----------
     * Two versions exist, distinguished by the version field:
     *
     * V1 header (16 bytes):
     * ┌─────────────────────────────────────────────────────────┐
     * │ version          (4 bytes)  0x10000                     │
     * │ magic1           (4 bytes)  must be 0                   │
     * │ wordsOffset      (4 bytes)  offset to words section     │
     * │ indexOffset      (4 bytes)  offset to index section     │
     * └─────────────────────────────────────────────────────────┘
     *
     * V2 header (32 bytes):
     * ┌─────────────────────────────────────────────────────────┐
     * │ version          (4 bytes)  0x20000                     │
     * │ magic1           (4 bytes)  must be 0                   │
     * │ wordsOffset      (4 bytes)  offset to words section     │
     * │ indexOffset       (4 bytes)  offset to index section    │
     * │ convTableOffset  (4 bytes)  offset to conversion table  │
     * │ magic5           (4 bytes)  must be 0                   │
     * │ magic6           (4 bytes)  must be 0                   │
     * │ magic7           (4 bytes)  must be 0                   │
     * └─────────────────────────────────────────────────────────┘
     *
     * WORDS SECTION
     * -------------
     * Contains word entries and page reference blocks. Index arrays point into this section via word offsets.
     * Word entry:
     * ┌─────────────────────────────────────────────────────────┐
     * │ pagesOffset      (4 bytes)  words-section-relative      │
     * │ separator         (1 byte)   0x00                       │
     * │ key               (variable) null-terminated UTF-8      │
     * └─────────────────────────────────────────────────────────┘
     *
     * Page references:
     * ┌─────────────────────────────────────────────────────────┐
     * │ Variable-length encoded page references                 │
     * │ (decoded by decodePageReferences)                       │
     * └─────────────────────────────────────────────────────────┘
     *
     * INDEX SECTION
     * -------------
     * Begins at indexOffset. Contains a header followed by up to
     * four index arrays, each providing a different sort order
     * over the word entries.
     *
     * Index header (20 bytes):
     * ┌─────────────────────────────────────────────────────────┐
     * │ magic            (4 bytes)  must be 0x04                │
     * │ indexAOffset     (4 bytes)  relative to index section   │
     * │ indexBOffset     (4 bytes)  relative to index section   │
     * │ indexCOffset     (4 bytes)  relative to index section   │
     * │ indexDOffset     (4 bytes)  relative to index section   │
     * └─────────────────────────────────────────────────────────┘
     *
     * An offset of 0 means that index is not present.
     *
     * Each index array:
     * ┌─────────────────────────────────────────────────────────┐
     * │ count            (4 bytes)  number of entries           │
     * │ offsets          (4 bytes × count)                      │
     * │                  each is a word offset into the words   │
     * │                  section                                │
     * └─────────────────────────────────────────────────────────┘
     *
     * Index A — sorted by key length
     * Index B — sorted by key (for prefix search)
     * Index C — sorted by key suffix
     * Index D — other / conversion-related
     *
     *
     * CONVERSION TABLE (optional, v2 only)
     * ------------------------------------
     * Located at convTableOffset. Used by specific dictionaries
     * (KNEJ, KNJE, maybe others too...) to remap page references after lookup.
     */

    constexpr uint32_t KEYSTORE_V1 = 0x10000;
    constexpr uint32_t KEYSTORE_V2 = 0x20000;
    constexpr uint32_t INDEX_MAGIC = 0x04;

    enum class KeystoreIndex : size_t
    {
        Length = 0, // Index A — sorted by key length
        Prefix = 1, // Index B — sorted by key (prefix search)
        Suffix = 2, // Index C — sorted by key suffix
        Other = 3, // Index D — conversion table / other
    };

    struct KeystoreHeader : BinaryStruct<KeystoreHeader>
    {
        uint32_t version; // 0x10000 or 0x20000
        uint32_t magic1; // Must be 0
        uint32_t wordsOffset; // Offset to words section
        uint32_t indexOffset; // Offset to index section
        uint32_t conversionTableOffset; // Offset to conversion table (or 0)
        uint32_t magic5; // Must be 0
        uint32_t magic6; // Must be 0
        uint32_t magic7; // Must be 0

        [[nodiscard]] size_t headerSize() const noexcept;

        void swapEndianness() noexcept;
    };

    struct IndexHeader : BinaryStruct<IndexHeader>
    {
        uint32_t magic; // Must be 0x04
        uint32_t indexAOffset;
        uint32_t indexBOffset;
        uint32_t indexCOffset;
        uint32_t indexDOffset;

        void swapEndianness() noexcept;
    };

    struct ConversionEntry
    {
        uint32_t page;
        uint16_t item;
        uint16_t padding;
    };

    static_assert(sizeof(ConversionEntry) == 8);

    class Keystore
    {
    public:
        ~Keystore();
        Keystore(Keystore&&) noexcept;
        Keystore& operator=(Keystore&&) noexcept;

        /**
         * Load a keystore file from disk.
         *
         * The entire file is read into memory. Index arrays and the optional
         * conversion table are parsed eagerly.
         *
         * @param path    Path to the .keystore file
         * @param dictId  Dictionary identifier (e.g. "KNEJ"). Used to decide
         *                whether automatic key-ID conversion is applied.
         * @return Loaded Keystore, or an error string
         */
        static Result<Keystore> load(const fs::path& path, const std::string& dictId);


        /**
         * Look up an entry by its position within an index.
         *
         * For dictionaries that require it (KNEJ, KNJE), page references are
         * automatically converted via the embedded conversion table.
         *
         * @param indexType  Which index to query
         * @param index      Position within that index
         * @return Lookup result with key and page references, or an error
         */
        [[nodiscard]] Result<KeystoreLookupResult> getByIndex(KeystoreIndex indexType, size_t index) const;

        /**
         * @return Number of entries in the given index (0 if the index doesn't exist).
         */
        [[nodiscard]] size_t indexSize(KeystoreIndex indexType) const noexcept;


        [[nodiscard]] std::string_view filename() const;

    private:
        [[nodiscard]] const std::vector<uint32_t>* getIndexArray(KeystoreIndex type) const noexcept;

        /**
         * Parse a word entry from the words section.
         * Layout: uint32_le pages_offset | 0x00 | null-terminated string
         */
        struct WordEntry
        {
            std::string_view key;
            size_t pagesOffset; // words-section-relative
        };

        [[nodiscard]] Result<WordEntry> parseWordEntry(uint32_t wordOffset) const;

        /**
         * Decode page references at the given words-section-relative offset.
         */
        [[nodiscard]] Result<std::vector<PageReference>> decodePages(size_t pagesOffset) const;

        /**
         * @return true if this dictionary needs automatic key-ID conversion.
         */
        [[nodiscard]] bool needsConversion() const noexcept;

        /**
         * Apply the conversion table in-place.
         */
        void applyConversion(std::vector<PageReference>& refs) const noexcept;

        static Result<IndexHeader> readIndexHeader(std::span<const uint8_t> data);

        struct IndexArrays
        {
            std::vector<uint32_t> length, prefix, suffix, other;
        };

        static Result<IndexArrays> readAllIndices(std::span<const uint8_t> data, const IndexHeader& header);

        /**
         * Read a single index array.
         * Format: uint32_le count | uint32_le[count] offsets
         *
         * @param data   Buffer starting at the index section
         * @param start  Byte offset of this index within `data`
         * @param end    Byte offset of the next index (or section end)
         * @return Offset array (count element removed), or error
         */
        static Result<std::vector<uint32_t>> readSingleIndex(std::span<const uint8_t> data, uint32_t start, uint32_t end);

        static Result<std::span<const ConversionEntry>> parseConversionTable(std::span<const uint8_t> fileData, size_t offset, size_t fileSize);

        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit Keystore(std::unique_ptr<Impl> impl) noexcept;
    };
}
