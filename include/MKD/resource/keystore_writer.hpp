//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace MKD
{
    struct KeystorePosting
    {
        EntryId entryId;
        /** Encoded dictionary ID. Generic native search replaces it with the current dictionary's ID. */
        uint16_t dictionaryId = 0;
        /** Optional trailing byte with profile-specific meaning. Leave absent for ordinary keys. */
        std::optional<uint8_t> type;
        auto operator<=>(const KeystorePosting&) const = default;
    };

    struct SearchKeyOptions
    {
        bool exactOnly = false;
        auto operator<=>(const SearchKeyOptions&) const = default;
    };

    enum class KeystoreVersion : uint32_t { Version1 = 0x10000, Version2 = 0x20000 };

    struct KeystoreWriteOptions
    {
        KeystoreVersion version = KeystoreVersion::Version2;
        /** Converted targets retain the full 32-bit page ID. */
        std::vector<EntryId> conversions;
    };

    class KeystoreWriter
    {
    public:
        [[nodiscard]] static Result<KeystoreWriter> create(const std::filesystem::path& path,
                                                           const KeystoreWriteOptions& options = {});
        ~KeystoreWriter();
        KeystoreWriter(KeystoreWriter&&) noexcept;
        KeystoreWriter& operator=(KeystoreWriter&&) noexcept;
        [[nodiscard]] Result<void> add(std::string_view key, EntryId target, SearchKeyOptions options = {}) const;
        [[nodiscard]] Result<void> add(std::string_view key, std::span<const KeystorePosting> targets,
                                       SearchKeyOptions options = {}) const;
        [[nodiscard]] Result<void> finish() const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit KeystoreWriter(std::unique_ptr<Impl> impl);
    };
}
