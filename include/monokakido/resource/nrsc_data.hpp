//
// Caoimheにより 2026/01/16 に作成されました。
//

#pragma once

#include "nrsc_index.hpp"
#include "zlib_decompressor.hpp"

#include <expected>
#include <fstream>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace fs = std::filesystem;

namespace monokakido::resource
{
    /**
    * NRSC Resource File Format (.nrsc)
    * ==================================
    *
    * Resource files store the actual data (images, audio, etc.) referenced by
    * the index. Multiple numbered files (0.nrsc, 1.nrsc, 2.nrsc, ...) are used
    * to split large resource collections into chunks.
    *
    * Virtual File Space:
    * ┌─────────────────────────────────────────────────────────────┐
    * │ 0.nrsc (1.2 MB)                                             │
    * │  globalOffset: 0                                            │
    * │  Contains: Resources at offsets 0 to 1,258,291              │
    * ├─────────────────────────────────────────────────────────────┤
    * │ 1.nrsc (800 KB)                                             │
    * │  globalOffset: 1,258,292                                    │
    * │  Contains: Resources at offsets 1,258,292 to 2,077,043      │
    * ├─────────────────────────────────────────────────────────────┤
    * │ 2.nrsc (1.5 MB)                                             │
    * │  globalOffset: 2,077,044                                    │
    * │  Contains: Resources at offsets 2,077,044 to 3,648,819      │
    * └─────────────────────────────────────────────────────────────┘
    *
    * The files are treated as a single virtual concatenated file space.
    * Index records store global offsets that span across all files.
    *
    * Workflow
    * - Load NrscIndex to get the table of contents
    * - Load NrscData to access the numbered resource files
    * - Look up a resource ID in the index to get an NrscIndexRecord
    * - Pass the record to NrscData::get() to retrieve the data
    *
    * Example
    * auto index = NrscIndex::load("dict/graphics");
    * auto data = NrscData::load("dict/graphics");
    *
    * auto record = index->findById("icon_search.png");
    * auto bytes = data->get(*record);  // Retrieves and decompresses if needed
    */

    // Represents a single .nrsc data file
    struct ResourceFile
    {
        uint32_t sequenceNumber;
        size_t fileSize;
        size_t globalOffset; // Offset in the virtual concatenated file space
        fs::path filePath;
    };


    class NrscData
    {
    public:

        static std::expected<NrscData, std::string> load(const fs::path& directoryPath);

        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string> get(const NrscIndexRecord& record) const;


    private:

        explicit NrscData(std::vector<ResourceFile>&& files);

        [[nodiscard]] std::optional<std::reference_wrapper<const ResourceFile>> findFileForOffset(uint64_t offset) const;

        // makes sense to have a dedicated ZlibDecompressor rather than doing that here since .rsc files also can be zlib compressed.
        // ill implement .rsc handling later
        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string> readAndDecompress(const NrscIndexRecord& record) const;

        static std::optional<uint32_t> parseSequenceNumber(const fs::path& filename);

        std::vector<ResourceFile> files_;
        std::unique_ptr<ZlibDecompressor> decompressor_;
        mutable std::vector<uint8_t> readBuffer_;

    };

}