//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <filesystem>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace MKD
{
    enum class NamedResourceStorage { Raw, Zlib };

    struct NamedResourceWriteOptions
    {
        NamedResourceStorage storage = NamedResourceStorage::Raw;
        /** Zlib level -1 through 9, including a stored zlib stream at level 0. */
        int compressionLevel = 6;
    };

    struct NamedResourceStoreWriteOptions
    {
        uint32_t fileBytes = 33 * 1024 * 1024;
        NamedResourceWriteOptions resources;
    };

    /** Writes an NRSC store. Input bytes are consumed by add(). */
    class NamedResourceStoreWriter
    {
    public:
        [[nodiscard]] static Result<NamedResourceStoreWriter> create(const std::filesystem::path& directory,
                                                                     const NamedResourceStoreWriteOptions& options =
                                                                         {});
        ~NamedResourceStoreWriter();
        NamedResourceStoreWriter(NamedResourceStoreWriter&&) noexcept;
        NamedResourceStoreWriter& operator=(NamedResourceStoreWriter&&) noexcept;
        /** Supplied per-resource options replace the store defaults. */
        [[nodiscard]] Result<void> add(std::string_view id, std::span<const uint8_t> bytes,
                                       std::optional<NamedResourceWriteOptions> options = std::nullopt) const;
        [[nodiscard]] Result<void> finish() const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit NamedResourceStoreWriter(std::unique_ptr<Impl> impl);
    };
}
