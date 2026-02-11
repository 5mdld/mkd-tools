//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "page_reference.hpp"

#include <string_view>

namespace monokakido
{
    // A single word entry from the keystore
    struct KeystoreWordEntry
    {
        std::string_view key;   // The search term
        size_t pagesOffset;     // Offset to page reference data (words-section-relative)
    };

    // Result of a keystore lookup
    struct KeystoreLookupResult
    {
        std::string_view key;
        size_t index;  // Position in the keystore

        [[nodiscard]] PageReferenceIterator begin() const { return PageReferenceIterator{pageData_, count_}; }
        [[nodiscard]] PageReferenceIterator end() const { return PageReferenceIterator{{}, 0}; }

        // Helper to get count without iterating
        [[nodiscard]] size_t size() const noexcept { return count_; }

        [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

    private:
        friend class Keystore;
        std::span<const uint8_t> pageData_;
        uint16_t count_ = 0;
    };
}