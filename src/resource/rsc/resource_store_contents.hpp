//
// kiwakiwaaにより 2026/01/22 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/retained_span.hpp"
#include "resource/detail/resource_files.hpp"
#include "resource_store_crypto.hpp"
#include "resource_store_index.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>


namespace MKD::detail
{
    class ResourceBatch;
}

namespace MKD
{
    /**
    * Resource Store Contents file format (.rsc)
    *
    * The .rsc format is the content storage system for XML entry data, audio and fonts.
    * .rsc files contain compressed (and also optionally encrypted) dictionary entries in XML format.
    *  - also used in `/index` for dict RUIGO (Apple binary .plist files - not implemented yet)
    *
    * Overview:
    * ┌─────────────────────────────────────────────────────────────────┐
    * │ Index Layer (contents.idx + contents.map)                       │
    * │  ├─ contents.idx: Maps item IDs to map indices                  │
    * │  └─ contents.map: Maps indices to chunk locations               │
    * ├─────────────────────────────────────────────────────────────────┤
    * │ Data Layer (0.rsc, 1.rsc, 2.rsc, ...)                           │
    * │  ├─ Multiple numbered files for size management                 │
    * │  ├─ Each file contains multiple compressed "chunks"             │
    * │  └─ Each chunk contains multiple dictionary entries             │
    * └─────────────────────────────────────────────────────────────────┘
    *
    * CHUNKS: Compressed blocks containing multiple dictionary entries
    * - Each chunk can be zlib-compressed (optionally encrypted+compressed)
    * - Chunks are variable-sized
    * - Multiple entries sharing a chunk reduces decompression overhead
    *
    * GLOBAL OFFSETS: Unified addressing across all .rsc files
    * - Treats all numbered files (0.rsc, 1.rsc, ...) as one virtual file
    * - Offsets in MapRecords are relative to the start of 0.rsc
    * - Example: offset 5MB in a 3MB+4MB file setup points to byte 2MB of 1.rsc
    *
    * Chunk Structure (per chunk in .rsc files):
    *
    * Two formats exist, distinguished by the first 4 bytes:
    *
    * OLD FORMAT (unencrypted):
    * ┌──────────────────────────────────────┐
    * │ Compressed Length    (4 bytes, > 0)  │
    * ├──────────────────────────────────────┤
    * │ Zlib Data            (variable)      │
    * └──────────────────────────────────────┘
    *
    * NEW FORMAT (encrypted):
    * ┌──────────────────────────────────────┐
    * │ Zero Marker          (4 bytes, = 0)  │
    * ├──────────────────────────────────────┤
    * │ Encrypted Length     (4 bytes)       │
    * ├──────────────────────────────────────┤
    * │ Encrypted Data       (variable)      │
    * └──────────────────────────────────────┘
    *
    * Encryption Details:
    * - Algorithm: XOR based algorithm 'HMDicEncoder::Decode'
    * - Key: Dictionary-specific 32-byte key
    * - Format marker: First 4 bytes are 0x00000000
    * - Process: Decrypt → Decompress → Parse items
    */
    class ResourceStoreContents
    {
    public:
        static Result<ResourceStoreContents> open(const std::filesystem::path& directory, std::string_view dictId = {},
                                                  uint32_t mapVersion = 0);
        [[nodiscard]] Result<RetainedSpan> get(const ResourceStoreMapRecord& record) const;

    private:
        friend class detail::ResourceBatch;
        explicit ResourceStoreContents(detail::ResourceFiles files,
                                       std::optional<std::array<uint8_t, 32>> key);
        [[nodiscard]] Result<RetainedSpan> readDirectData(size_t offset) const;
        [[nodiscard]] Result<RetainedSpan> getOrLoadChunk(size_t offset) const;
        [[nodiscard]] Result<RetainedSpan> decodeChunkAt(size_t offset) const;
        static Result<RetainedSpan>
        parseItemFromChunk(RetainedSpan chunk, size_t offset);

        struct ChunkCache
        {
            std::mutex mutex;
            std::unordered_map<size_t, RetainedSpan> entries;
        };

        detail::ResourceFiles files_;
        std::optional<std::array<uint8_t, 32>> decryptionKey_;
        std::unique_ptr<ChunkCache> cache_;
        static constexpr size_t MAX_CACHED_CHUNKS = 64;
    };
}
