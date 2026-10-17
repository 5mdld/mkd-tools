//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <filesystem>
#include <cstdint>
#include "MKD/resource/entry_id.hpp"
#include "MKD/resource/keystore_scope.hpp"

#include <expected>
#include <ranges>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include <span>
#include <stop_token>

namespace MKD
{
    enum class KeystoreIndex : size_t
    {
        Length = 0, // Index A — sorted by key length
        Prefix = 1, // Index B — sorted by key (prefix search)
        Suffix = 2, // Index C — sorted by key suffix
        Other = 3, // Index D — other
    };

    enum class KeystoreIdMapping
    {
        Stored,
        Resolved
    };

    struct KeystoreEntry
    {
        std::string_view key;
        size_t index = 0;
        uint8_t flags = 0; // Word entry flags byte (bit 2 = uint32 count, bit 3 = exact-only)

        [[nodiscard]] bool empty() const noexcept { return key.empty(); }
    };

    class Keystore
    {
    public:
        static Result<Keystore> open(const std::filesystem::path& path);

        ~Keystore();
        Keystore(Keystore&&) noexcept;
        Keystore& operator=(Keystore&&) noexcept;
        Keystore(const Keystore&) = delete;
        Keystore& operator=(const Keystore&) = delete;

        /**
         * Look up an entry by its position within an index.
         *
         * @param type Which index to query
         * @param index Position within that index
         * @return Lookup result with key metadata, or an error
         */
        [[nodiscard]] Result<KeystoreEntry> entryAt(KeystoreIndex type, size_t index) const;

        /**
         * Lookup key only, without decoding entry ids.
         *
         * @param type Which index to query
         * @param index Position within that index
         * @return Lookup key with its index, or an error
         */
        [[nodiscard]] Result<std::string_view> keyAt(KeystoreIndex type, size_t index) const;

        /**
         * Decode entry ids for a previously located keystore position.
         * Resolved IDs apply the embedded conversion table. Stored IDs retain
         * native key order and can be resolved after search intersections.
         *
         * @param type Which index to query
         * @param index Position within that index
         * @return Entry ids for a keystore position, or an error
         */
        [[nodiscard]] Result<std::vector<EntryId>> entryIdsAt(KeystoreIndex type, size_t index,
                                                              KeystoreIdMapping mapping = KeystoreIdMapping::Resolved,
                                                              std::stop_token stop = {}) const;

        /** No value means there is no conversion table. An out-of-range key ID is an error. */
        [[nodiscard]] Result<std::optional<EntryId>> entryIdForKeyId(uint32_t keyId) const;

        /** Resolve stored key IDs in place, preserving order. Returns whether a mapping exists. */
        bool resolveEntryIds(std::span<EntryId> ids) const noexcept;

        /// Number of entries in the given index (0 if absent).
        [[nodiscard]] size_t indexSize(KeystoreIndex type) const noexcept;

        [[nodiscard]] std::string_view filename() const noexcept;
        [[nodiscard]] std::optional<KeystoreScope> scope() const noexcept;

        /**
         * Look up all keys in the prefix index that map to the given entry ID.
         * Returns an empty vector if no keys exist for the entry.
         */
        [[nodiscard]] Result<std::vector<std::string_view>> keysForEntry(const EntryId& id) const;

        /** Sorted unique IDs from the lazily built prefix inverse index. */
        [[nodiscard]] Result<std::vector<EntryId>> indexedEntryIds() const;

        /** Keys are borrowed from this store. Each range element carries its read error. */
        [[nodiscard]] auto entries(KeystoreIndex type) const &
        {
            return std::views::iota(size_t{0}, indexSize(type))
                | std::views::transform([this, type](const size_t index) { return entryAt(type, index); });
        }

        auto entries(KeystoreIndex) const && = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit Keystore(std::unique_ptr<Impl> impl) noexcept;
    };
}
