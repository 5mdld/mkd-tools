//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

namespace MKD
{
    struct ResourceStoreWriteOptions
    {
        uint32_t fileBytes = 3 * 1024 * 1024;
        uint32_t chunkBytes = 256 * 1024;
        /** Zlib level -1 through 9. Level 0 still writes a zlib stream. */
        int compressionLevel = 6;
        bool encrypt = false;
    };

    /** Writes an indexed RSC store. Input bytes are consumed by add(). */
    class ResourceStoreWriter
    {
    public:
        /** Encryption derives its key from the content identifier, which must then be nonempty. */
        [[nodiscard]] static Result<ResourceStoreWriter> create(const std::filesystem::path& directory,
                                                                std::string_view contentIdentifier = {},
                                                                const ResourceStoreWriteOptions& options = {});
        ~ResourceStoreWriter();
        ResourceStoreWriter(ResourceStoreWriter&&) noexcept;
        ResourceStoreWriter& operator=(ResourceStoreWriter&&) noexcept;
        [[nodiscard]] Result<void> add(uint32_t id, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> finish() const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit ResourceStoreWriter(std::unique_ptr<Impl> impl);
    };
}
