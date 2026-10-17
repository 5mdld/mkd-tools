//
// kiwakiwaaにより 2026/02/11 に作成されました。
//

#include "entry_id_codec.hpp"
#include "binary/binary_view.hpp"
#include "binary/binary_buffer.hpp"

#include <format>

namespace MKD::detail::keystore
{
    void encodeKeystoreEntry(BinaryBuffer& buffer, const EntryId id, const uint16_t dictionaryId,
                             const std::optional<uint8_t> type)
    {
        const unsigned pageBytes = id.pageId == 0 ? 0 : id.pageId <= 0xFF ? 1 : id.pageId <= 0xFFFF ? 2 : 3;
        const unsigned itemBytes = id.itemId == 0 ? 0 : id.itemId <= 0xFF ? 1 : 2;
        const unsigned dictionaryBytes = dictionaryId == 0 ? 0 : dictionaryId <= 0xFF ? 1 : 2;
        const auto flags = static_cast<uint8_t>((pageBytes == 3 ? 4 : pageBytes) |
            (itemBytes << 4) | (dictionaryBytes << 6) | (type ? 8 : 0));

        buffer.writeByte(flags);
        if (pageBytes == 1)
            buffer.writeByte(static_cast<uint8_t>(id.pageId));
        else if (pageBytes == 2)
            buffer.writeBig<uint32_t, 2>(id.pageId);
        else if (pageBytes == 3)
            buffer.writeBig<uint32_t, 3>(id.pageId);

        if (itemBytes == 1)
            buffer.writeByte(static_cast<uint8_t>(id.itemId));
        else if (itemBytes == 2)
            buffer.writeBig(id.itemId);

        if (dictionaryBytes == 1)
            buffer.writeByte(static_cast<uint8_t>(dictionaryId));
        else if (dictionaryBytes == 2)
            buffer.writeBig(dictionaryId);

        if (type) buffer.writeByte(*type);
    }

    Result<DecodedEntry> decodeKeystoreEntry(std::span<const uint8_t> data)
    {
        const BinaryView input(data);
        auto flags = input.readLittle<uint8_t>(0);
        if (!flags)
            return std::unexpected(flags.error());

        size_t offset = 1;
        DecodedEntry entry{};
        // The flags select byte widths, while multibyte values are big endian.
        const auto field = [&](const uint8_t bits, bool page) -> Result<uint32_t>
        {
            Result<uint32_t> value = uint32_t{0};
            size_t width = 0;
            if (bits & 1) width = 1;
            else if (bits & 2) width = 2;
            else if (page && (bits & 4)) width = 3;
            switch (width)
            {
            case 1: value = input.readBig<uint32_t, 1>(offset);
                break;
            case 2: value = input.readBig<uint32_t, 2>(offset);
                break;
            case 3: value = input.readBig<uint32_t, 3>(offset);
                break;
            default: break;
            }
            if (value)
                offset += width;
            return value;
        };

        auto page = field(*flags & 0x07, true);
        if (!page)
            return std::unexpected(page.error());

        auto item = field((*flags >> 4) & 0x03, false);
        if (!item)
            return std::unexpected(item.error());

        auto dictionaryId = field((*flags >> 6) & 0x03, false);
        if (!dictionaryId)
            return std::unexpected(dictionaryId.error());

        entry.page = *page;
        entry.item = static_cast<uint16_t>(*item);
        entry.dictionaryId = static_cast<uint16_t>(*dictionaryId);
        if (*flags & 0x08)
        {
            auto type = input.readLittle<uint8_t>(offset++);
            if (!type) return std::unexpected(type.error());
            entry.type = *type;
            entry.hasType = true;
        }
        entry.bytesConsumed = offset;
        return entry;
    }

    Result<std::vector<EntryId>> decodeEntryIds(std::span<const uint8_t> data, bool wideCount)
    {
        const BinaryView input(data);
        auto count = wideCount ? input.readLittle<uint32_t>(0) : input.readLittle<uint32_t, 2>(0);
        if (!count)
            return std::unexpected(count.error());

        auto remaining = input.tail(wideCount ? 4 : 2);
        if (!remaining)
            return std::unexpected(remaining.error());
        if (*count > remaining->bytes().size())
            return std::unexpected("Entry count exceeds the encoded data size");

        std::vector<EntryId> result;
        result.reserve(*count);
        for (uint32_t i = 0; i < *count; ++i)
        {
            auto decoded = decodeKeystoreEntry(remaining->bytes());
            if (!decoded)
                return std::unexpected(std::format("Entry {}: {}", i, decoded.error()));
            result.push_back({decoded->page, decoded->item});
            remaining = remaining->tail(decoded->bytesConsumed);
        }
        return result;
    }
}
