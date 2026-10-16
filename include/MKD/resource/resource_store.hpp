//
// kiwakiwaaにより 2026/01/30 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/retained_span.hpp"

#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace MKD
{
    struct ResourceStoreItem
    {
        uint32_t itemId;
        RetainedSpan data;
    };

    class ResourceStore
    {
    public:
        ~ResourceStore() noexcept;
        ResourceStore(ResourceStore&&) noexcept;
        ResourceStore& operator=(ResourceStore&&) noexcept;

        /** The dictionary ID is required when the map declares encryption. */
        static Result<ResourceStore> open(const std::filesystem::path& directory, std::string_view dictId = {});
        [[nodiscard]] Result<RetainedSpan> get(uint32_t id) const;
        [[nodiscard]] Result<ResourceStoreItem> itemAt(size_t index) const;
        [[nodiscard]] Result<uint32_t> idAt(size_t index) const;
        [[nodiscard]] Result<size_t> indexOf(uint32_t id) const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        /** Each element is a Result because reading or decompressing a payload may fail. */
        [[nodiscard]] auto items() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return itemAt(index); });
        }

        auto items() const && = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit ResourceStore(std::unique_ptr<Impl> impl) noexcept;
    };
}
