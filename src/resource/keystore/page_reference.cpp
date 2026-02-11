//
// kiwakiwaaにより 2026/02/11 に作成されました。
//

#include "monokakido/resource/keystore/page_reference.hpp"


namespace monokakido
{
    std::expected<DecodedEntry, std::string> decodeKeystoreEntry(
        const std::span<const uint8_t> data)
    {
        if (data.empty())
            return std::unexpected("Empty data for keystore entry");

        const uint8_t flags = data[0];
        size_t offset = 1;
        DecodedEntry entry{};

        if (flags & 0x01)
        {
            if (offset + 1 > data.size())
                return std::unexpected("Truncated page field");
            entry.page = data[offset];
            offset += 1;
        }
        else if (flags & 0x02)
        {
            if (offset + 2 > data.size())
                return std::unexpected("Truncated page field");
            entry.page = (static_cast<uint32_t>(data[offset]) << 8) |
                         data[offset + 1];
            offset += 2;
        }
        else if (flags & 0x04)
        {
            if (offset + 3 > data.size())
                return std::unexpected("Truncated page field");
            entry.page = (static_cast<uint32_t>(data[offset]) << 16) |
                         (static_cast<uint32_t>(data[offset + 1]) << 8) |
                         data[offset + 2];
            offset += 3;
        }

        if (flags & 0x10)
        {
            if (offset + 1 > data.size())
                return std::unexpected("Truncated item field");
            entry.item = data[offset];
            offset += 1;
        }
        else if (flags & 0x20)
        {
            if (offset + 2 > data.size())
                return std::unexpected("Truncated item field");
            entry.item = (static_cast<uint16_t>(data[offset]) << 8) |
                         data[offset + 1];
            offset += 2;
        }

        if (flags & 0x40)
        {
            if (offset + 1 > data.size())
                return std::unexpected("Truncated subitem field");
            entry.extra = data[offset];
            offset += 1;
        }
        else if (flags & 0x80)
        {
            if (offset + 2 > data.size())
                return std::unexpected("Truncated subitem field");
            entry.extra = (static_cast<uint16_t>(data[offset]) << 8) |
                          data[offset + 1];
            offset += 2;
        }

        if (flags & 0x08)
        {
            if (offset + 1 > data.size())
                return std::unexpected("Truncated type field");
            entry.type = data[offset];
            offset += 1;
            entry.hasType = true;
        }

        entry.bytesConsumed = offset;
        return entry;
    }

    PageReferenceIterator::value_type PageReferenceIterator::operator*() const
    {
        if (remaining_ == 0 || data_.empty())
            return PageReference{0, 0};

        // Decode current entry without modifying state
        const auto decoded = decodeKeystoreEntry(data_);
        if (!decoded)
            return PageReference{0, 0};

        return PageReference{
            decoded->page,
            decoded->item
        };
    }

    PageReferenceIterator& PageReferenceIterator::operator++()
    {
        if (remaining_ == 0 || data_.empty())
            return *this;

        // Decode to find how many bytes to skip
        const auto decoded = decodeKeystoreEntry(data_);
        if (!decoded || decoded->bytesConsumed > data_.size())
        {
            remaining_ = 0;
            data_ = {};
            return *this;
        }

        // Advance to next entry
        data_ = data_.subspan(decoded->bytesConsumed);
        --remaining_;
        return *this;
    }

    PageReferenceIterator PageReferenceIterator::operator++(int)
    {
        const auto copy = *this;
        ++*this;
        return copy;
    }


    bool PageReferenceIterator::operator==(const PageReferenceIterator& other) const noexcept
    {
        return remaining_ == other.remaining_;
    }
}
