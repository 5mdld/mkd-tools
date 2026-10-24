//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/resource/retained_span.hpp"

#include <optional>
#include <ranges>

namespace MKD
{
    class ResourceStore;

    /** Article browsing order, independent of storage order and search-result order. */
    class EntryOrder
    {
    public:
        /** Maps an order file. When absent, Dictionaries assumes page IDs 1 through the store count. */
        [[nodiscard]] static Result<EntryOrder> open(const std::filesystem::path& path, const ResourceStore& entries);

        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] Result<uint32_t> pageAt(size_t index) const;
        [[nodiscard]] Result<size_t> indexOf(uint32_t pageId) const;
        /** Returns no page at the boundary. An unknown page ID is an error. */
        [[nodiscard]] Result<std::optional<uint32_t>> previous(uint32_t pageId) const;
        [[nodiscard]] Result<std::optional<uint32_t>> next(uint32_t pageId) const;

        [[nodiscard]] auto pages() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return page(index); });
        }

        [[nodiscard]] auto pages() const && = delete;

    private:
        friend class Dictionary;
        explicit EntryOrder(uint32_t pageCount) noexcept;
        [[nodiscard]] Result<void> validate() const;
        [[nodiscard]] uint32_t position(uint32_t pageId) const;
        [[nodiscard]] uint32_t page(size_t index) const;

        RetainedSpan data_;
        uint32_t count_ = 0;
        uint32_t maximum_ = 0;
    };
}
