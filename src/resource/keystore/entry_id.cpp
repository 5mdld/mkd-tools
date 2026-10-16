//
// kiwakiwaaにより 2026/02/11 に作成されました。
//

#include "entry_id_codec.hpp"
#include "binary/binary_view.hpp"

#include <format>

namespace MKD::detail::keystore
{
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
        if (!page) return std::unexpected(page.error());
        auto item = field((*flags >> 4) & 0x03, false);
        if (!item) return std::unexpected(item.error());
        auto extra = field((*flags >> 6) & 0x03, false);
        if (!extra) return std::unexpected(extra.error());
        entry.page = *page;
        entry.item = static_cast<uint16_t>(*item);
        entry.extra = static_cast<uint16_t>(*extra);
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
