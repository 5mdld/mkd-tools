//
// Caoimheにより 2026/01/16 に作成されました。
//

#pragma once

#include "common.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace monokakido::resource
{
    /**
     * NRSC Index File Format (.nidx)
     * ==============================
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


    /**
     * Represents a single entry in the .nrsc index
     *
     * This class loads and provides access to the index that maps string IDs
     * to resources stored across multiple numbered .nrsc files. Records are
     * stored sorted by ID string to enable binary search lookups.
     */
    struct NrscIndexRecord
    {
        uint16_t format;            // Compression: 0=uncompressed, 1=zlib
        uint16_t fileSequence;      // Which numbered .nrsc file (0.nrsc, 1.nrsc, etc.)
        uint32_t idStringOffset;    // Byte offset into the ID strings region
        uint32_t fileOffset;        // Byte offset within the target .nrsc file
        uint32_t length;            // Decompressed size of the resource data

        [[nodiscard]] CompressionFormat compressionFormat() const;
        [[nodiscard]] size_t fileSeq() const noexcept;
        [[nodiscard]] size_t idOffset() const noexcept;
        [[nodiscard]] uint64_t offset() const noexcept;
        [[nodiscard]] size_t len() const noexcept;

        void toLittleEndian() noexcept;
    };

    static_assert(sizeof(NrscIndexRecord) == 16, "NrscIndexRecord should be 16 bytes long");

    constexpr size_t HEADER_SIZE = 8;
    constexpr size_t RECORD_SIZE = sizeof(NrscIndexRecord);

    struct IndexHeader
    {
        uint32_t zeroField;
        uint32_t recordCount;
    };


    // Manages the nrsc index file (index.nidx)
    class NrscIndex
    {
    public:
        /**
         * Factory method to load an Nrsc index file from a directory (index.nidx)
         * @param directoryPath Directory containing the index
         * @return NrscIndex or error string if failure
         */
        static std::expected<NrscIndex, std::string> load(const fs::path& directoryPath);


        /**
         * Find a record by string ID (binary search)
         * @return NrscIndexRecord or error string if failure
         */
        [[nodiscard]] std::expected<NrscIndexRecord, std::string> findById(std::string_view id) const;


        /**
         * Get record by index
         * @param index Index of record
         * @return string ID & NrscIndexRecord pair or error string if failure
         */
        [[nodiscard]] std::expected<std::pair<std::string_view, NrscIndexRecord>, std::string> getByIndex(size_t index) const;


        /**
         * Get total number of records
         * @return Number of records
         */
        [[nodiscard]] size_t size() const noexcept;


    private:

        NrscIndex(std::vector<NrscIndexRecord>&& records, std::string&& idStrings, size_t headerSize);


        /**
         * Get ID string at given offset
         * @param offset offset in index file
         * @return ID string or error string if failure
         */
        [[nodiscard]] std::expected<std::string_view, std::string> getIdAt(size_t offset) const;


        /**
         * Read and validate the index file header
         * @param file index filestream
         * @return Indexheader or error string if failure
         */
        static std::expected<IndexHeader, std::string> readHeader(std::ifstream& file);


        /**
         * Read all index records and convert to native endianness
         * @param file index filestream
         * @param count number of records
         * @return Vector of index records or error string if failure
         */
        static std::expected<std::vector<NrscIndexRecord>, std::string> readRecords(std::ifstream& file, uint32_t count);


        /**
         * Read the ID strings region
         * @param file index filestream
         * @param stringRegionSize size of the string region
         * @return null-terminated concatenated strings or error string if failure
         */
        static std::expected<std::string, std::string> readIdStrings(std::ifstream& file, size_t stringRegionSize);


        std::vector<NrscIndexRecord> records_;  // All index records, sorted by ID
        std::string idStrings_;                 // Concatenated null-terminated ID strings
        size_t headerSize_;                     // Header + records size (for offset calculations)
    };

};
