//
// kiwakiwaaにより 2026/02/28 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace MKD
{
    namespace detail
    {
        class ResourceFiles;
    }

    /** A byte view that retains its buffer or file mapping, including across store destruction. */
    class RetainedSpan
    {
    public:
        RetainedSpan() = default;
        RetainedSpan(const RetainedSpan&) = default;
        RetainedSpan& operator=(const RetainedSpan&) = default;
        RetainedSpan(RetainedSpan&& other) noexcept;
        RetainedSpan& operator=(RetainedSpan&& other) noexcept;
        explicit RetainedSpan(std::vector<uint8_t> bytes);
        explicit RetainedSpan(std::shared_ptr<const std::vector<uint8_t>> buffer);
        [[nodiscard]] static RetainedSpan copy(std::span<const uint8_t> bytes);
        [[nodiscard]] static Result<RetainedSpan> map(const std::filesystem::path& path);

        [[nodiscard]] Result<RetainedSpan> slice(size_t offset, size_t length) const;
        /** The text view borrows these bytes and remains valid while a retaining owner is alive. */
        [[nodiscard]] Result<std::string_view> utf8() const;
        [[nodiscard]] const uint8_t* data() const noexcept { return span_.data(); }
        [[nodiscard]] size_t size() const noexcept { return span_.size(); }
        [[nodiscard]] bool empty() const noexcept { return span_.empty(); }
        [[nodiscard]] std::span<const uint8_t> span() const noexcept { return span_; }
        explicit operator std::span<const uint8_t>() const noexcept { return span_; }
        [[nodiscard]] auto begin() const noexcept { return span_.begin(); }
        [[nodiscard]] auto end() const noexcept { return span_.end(); }
        [[nodiscard]] uint8_t operator[](const size_t index) const { return span_[index]; }

    private:
        friend class detail::ResourceFiles;
        RetainedSpan(std::shared_ptr<const void> owner, std::span<const uint8_t> bytes);

        std::shared_ptr<const void> owner_;
        std::span<const uint8_t> span_;
    };
}
