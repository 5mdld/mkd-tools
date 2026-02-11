//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include <expected>
#include <span>
#include <string>


namespace monokakido
{
    struct DecodedEntry
    {
        uint32_t page;          // Page ID (HTML file)
        uint16_t item;          // Item within page (HTML element reference)
        uint16_t extra;         // Encoded via flags 0x40/0x80. purpose varies by dictionary;
                                // not stored into the entry ID buffer during standard search

        uint8_t  type;          // Filter byte (flag 0x08), entries with this set may be
                                // skipped for non-type-1 searches
        bool     hasType;
        size_t   bytesConsumed;
    };

    // Reference to a dictionary entry
    struct PageReference
    {
        uint32_t page;    // Page number in dictionary content
        uint16_t item;    // Item ID within page (HTML element id)

        auto operator<=>(const PageReference&) const = default;
    };

    /**
     * Decode a single page reference entry from bit-packed format
     *
     * Format uses first byte as flags:
     *   0x01: page is 1 byte
     *   0x02: page is 2 bytes (big-endian)
     *   0x04: page is 3 bytes (big-endian)
     *   0x08: type byte present
     *   0x10: item is 1 byte
     *   0x20: item is 2 bytes (big-endian)
     *   0x40: subitem is 1 byte
     *   0x80: subitem is 2 bytes (big-endian)
     */
    [[nodiscard]] std::expected<DecodedEntry, std::string> decodeKeystoreEntry(std::span<const uint8_t> data);

    // Iterator over page references for a single key
    class PageReferenceIterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = PageReference;
        using pointer = const value_type*;
        using reference = value_type;

        PageReferenceIterator() noexcept = default;

        value_type operator*() const;

        PageReferenceIterator& operator++();

        PageReferenceIterator operator++(int);

        bool operator==(const PageReferenceIterator& other) const noexcept;

    private:
        friend class Keystore;
        friend struct KeystoreLookupResult;

        PageReferenceIterator(const std::span<const uint8_t> data, const uint16_t count)
            : data_(data), remaining_(count)
        {
        }

        std::span<const uint8_t> data_;
        uint16_t remaining_ = 0;
    };
}
