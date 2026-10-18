//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"
#include <span>
#include <vector>
#include <optional>
#include <stop_token>

namespace MKD::detail
{
    class BinaryBuffer;
}

namespace MKD::detail::keystore
{
    // Variable-length encoding
    //
    // Each entry is bit-packed with a leading flags byte:
    //   0x01: page is 1 byte
    //   0x02: page is 2 bytes (big-endian)
    //   0x04: page is 3 bytes (big-endian)
    //   0x08: optional profile-specific byte present (not part of the entry ID)
    //   0x10: item is 1 byte
    //   0x20: item is 2 bytes (big-endian)
    //   0x40: dictionaryId is 1 byte
    //   0x80: dictionaryId is 2 bytes (big-endian)
    struct DecodedEntry
    {
        uint32_t page;
        uint16_t item;
        uint16_t dictionaryId;
        uint8_t type;
        bool hasType;
        size_t bytesConsumed;
    };

    /**
     * Decode a single variable-length entry from raw bytes.
     */
    [[nodiscard]] Result<DecodedEntry> decodeKeystoreEntry(std::span<const uint8_t> data);
    void encodeKeystoreEntry(BinaryBuffer& buffer, EntryId id, uint16_t dictionaryId = 0,
                             std::optional<uint8_t> type = std::nullopt);

    /**
     * Decode all entry ids from a counted block.
     *
     * Block format: uint16_le count | encoded entries...
     * When wideCount is true: uint32_le count | encoded entries...
     *
     * @param data  Span starting at the count field
     * @param wideCount  If true, count field is uint32 instead of uint16
     * @return Decoded entry ids, or an error
     */
    [[nodiscard]] Result<std::vector<EntryId>> decodeEntryIds(std::span<const uint8_t> data, bool wideCount = false,
                                                              std::stop_token stop = {});
}
