//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "monokakido/resource/common.hpp"
#include "monokakido/core/platform/fs.hpp"
#include "page_reference.hpp"
#include "keystore_lookup_result.hpp"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace monokakido
{
    /**
     * Keystore File Format (.keystore)
     * =================================
     *
     * Keystores map search terms (typically Japanese words) to locations
     * within dictionary content files. They enable efficient lookup of
     * dictionary entries by various keys (word form, reading, etc.).
     *
     * File Structure:
     * ┌─────────────────────────────────────────────────────────┐
     * │ File Header (16 or 32 bytes)                            │
     * │  - Version (4 bytes) - 0x10000 or 0x20000               │
     * │  - Magic fields (must be 0)                             │
     * │  - Words offset                                         │
     * │  - Index offset                                         │
     * │  - Next offset (v2 only)                                │
     * ├─────────────────────────────────────────────────────────┤
     * │ Words Section (variable length)                         │
     * │  - uint32 pages_offset (offset to page data)            │
     * │  - uint8 separator (0x00)                               │
     * │  - Null-terminated search term string                   │
     * │  - Page reference data (variable-length encoded)        │
     * ├─────────────────────────────────────────────────────────┤
     * │ Index Header (20 bytes)                                 │
     * │  - Magic (0x04)                                         │
     * │  - Four index offsets (A, B, C, D)                      │
     * ├─────────────────────────────────────────────────────────┤
     * │ Index Sections A, B, C, D (variable length each)        │
     * │  - First uint32: count of entries                       │
     * │  - Remaining uint32s: offsets into words section        │
     * └─────────────────────────────────────────────────────────┘
     */
    constexpr uint32_t KEYSTORE_V1 = 0x10000;
    constexpr uint32_t KEYSTORE_V2 = 0x20000;
    constexpr uint32_t INDEX_MAGIC = 0x04;

    // Which index array to use
    enum class KeystoreIndex : size_t
    {
        Length = 0,   // Index A - sorted by key length
        Prefix = 1,   // Index B - sorted by key (prefix search)
        Suffix = 2,   // Index C - sorted by key suffix
        Other  = 3,   // Index D - conversion table
    };

    struct KeystoreHeader : BinaryStruct<KeystoreHeader>
    {
        uint32_t version;       // 0x10000 or 0x20000
        uint32_t magic1;        // Must be 0
        uint32_t wordsOffset;   // Offset to words section
        uint32_t idxOffset;     // Offset to index section
        uint32_t nextOffset;    // V2 only: offset to next section (or 0)
        uint32_t magic5;        // V2 only: must be 0
        uint32_t magic6;        // V2 only: must be 0
        uint32_t magic7;        // V2 only: must be 0

        [[nodiscard]] size_t headerSize() const noexcept;

        void swapEndianness() noexcept;
    };

    struct IndexHeader : BinaryStruct<IndexHeader>
    {
        uint32_t magic;         // Must be 0x04
        uint32_t indexAOffset;  // Offset to index A (length index)
        uint32_t indexBOffset;  // Offset to index B (prefix index)
        uint32_t indexCOffset;  // Offset to index C (suffix index)
        uint32_t indexDOffset;  // Offset to index D (conversion table)

        void swapEndianness() noexcept;
    };


    class Keystore
    {
    public:
        /**
         * Loads a keystore file from disk.
         *
         * Reads and validates the file, parsing all index arrays.
         * The file data is kept in memory for subsequent lookups.
         *
         * @param path Path to the .keystore file
         * @return The loaded Keystore, or an error message if loading failed
         */
        static std::expected<Keystore, std::string> load(const fs::path& path);

        /**
         * Retrieves an entry by its position within an index.
         *
         * @param indexType Which index array to use
         * @param index position within the index
         * @return The lookup result containing the key and page references,
         *         or an error if the index type is invalid or index is out of bounds
         */
        [[nodiscard]] std::expected<KeystoreLookupResult, std::string> getByIndex(KeystoreIndex indexType, size_t index) const;


        /**
         * Returns the number of entries in an index.
         *
         * @param indexType Which index array to query
         * @return Entry count, or 0 if the index type is invalid or doesn't exist
         */
        [[nodiscard]] size_t indexSize(KeystoreIndex indexType) const noexcept;

    private:
        /*
         * Private constructor - use load(const fs::path& path)
         */
        explicit Keystore(
            std::vector<uint8_t>&& fileData,
            std::vector<uint32_t>&& indexLength,
            std::vector<uint32_t>&& indexPrefix,
            std::vector<uint32_t>&& indexSuffix,
            std::vector<uint32_t>&& indexD,
            size_t wordsOffset,
            bool hasConversionTable
        );

        /**
         * Maps an index type enum to its corresponding vector.
         *
         * @param type The index type to look up
         * @return Pointer to the index vector, or nullptr for invalid types
         */
        [[nodiscard]] const std::vector<uint32_t>* getIndexArray(KeystoreIndex type) const noexcept;


        /**
         * Parses a word entry from the words section.
         *
         * Word entries have the format:
         *   uint32 pages_offset | 0x00 | null-terminated string
         *
         * @param wordOffset Offset from start of words section
         * @return Parsed entry with key string and pages offset, or error
         */
        [[nodiscard]] std::expected<KeystoreWordEntry, std::string> getWordEntry(uint32_t wordOffset) const;


        /**
         * Constructs a complete lookup result from parsed components.
         *
         * @param key The search term
         * @param pagesOffset Offset to page reference data
         * @param index Original index position
         * @return Complete lookup result with page iterator, or error
         */
        [[nodiscard]] std::expected<KeystoreLookupResult, std::string> buildLookupResult(std::string_view key, size_t pagesOffset, size_t index) const;


        /**
         * Creates an iterator over page references at the given offset.
         *
         * Page data format:
         *   uint16 count | variable-length encoded PageReference entries
         *
         * Validates all entries can be decoded before returning.
         *
         * @param pagesOffset Offset from start of words section
         * @return Iterator positioned at first page reference, or error
         */
        [[nodiscard]] std::expected<PageReferenceIterator, std::string> getPageIterator(size_t pagesOffset) const;


        /**
         * Reads and validates the file header.
         *
         * @param reader File reader positioned at start of file
         * @return Parsed header, or error message
         */
        static std::expected<KeystoreHeader, std::string> readHeader(platform::fs::BinaryFileReader& reader);


        /**
         * Reads and validates the index section header.
         *
         * @param data Buffer containing index section
         * @param maxSize Maximum valid offset within the index section
         * @return Parsed index header, or error message
         */
        static std::expected<IndexHeader, std::string> readIndexHeader(std::span<const uint8_t> data,
                                                                       size_t maxSize);

        struct Indices
        {
            std::vector<uint32_t> indexA, indexB, indexC, indexD;
        };


        /**
         * Reads all four index arrays from the index section.
         *
         * Each index has the format:
         *   uint32 count | uint32[] word_offsets
         *
         * @param data Buffer containing index section
         * @param header Parsed index header with offsets
         * @param maxSize Size of the index section in bytes
         * @return All four index arrays, or error message
         */
        static std::expected<Indices, std::string> readIndices(
            std::span<const uint8_t> data,
            const IndexHeader& header,
            size_t maxSize);

        std::vector<uint8_t> fileData_;
        std::vector<uint32_t> indexLength_;  // Index A
        std::vector<uint32_t> indexPrefix_;  // Index B
        std::vector<uint32_t> indexSuffix_;  // Index C
        std::vector<uint32_t> indexD_;       // Index D
        size_t wordsOffset_;
        bool hasConversionTable_;
    };
}
