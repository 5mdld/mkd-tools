//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/retained_span.hpp"

#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

namespace MKD::detail
{
    class NamedResourceScan;
}

namespace MKD
{
    struct NamedResourceStoreItem
    {
        std::string id;
        RetainedSpan data;
    };

    class NamedResourceStore
    {
    public:
        ~NamedResourceStore();
        NamedResourceStore(NamedResourceStore&&) noexcept;
        NamedResourceStore& operator=(NamedResourceStore&&) noexcept;

        static Result<NamedResourceStore> open(const std::filesystem::path& directory);
        [[nodiscard]] Result<RetainedSpan> get(std::string_view id) const;
        [[nodiscard]] Result<NamedResourceStoreItem> itemAt(size_t index) const;
        /** ID views are borrowed from the store. Returned items own their IDs. */
        [[nodiscard]] Result<std::string_view> idAt(size_t index) const;
        [[nodiscard]] Result<size_t> indexOf(std::string_view id) const;
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] auto items() const &
        {
            return std::views::iota(size_t{0}, size())
                | std::views::transform([this](const size_t index) { return itemAt(index); });
        }

        auto items() const && = delete;

    private:
        friend class detail::NamedResourceScan;
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit NamedResourceStore(std::unique_ptr<Impl> impl) noexcept;
    };
}
