//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "binary_view.hpp"

#include <algorithm>
#include <format>

namespace MKD::detail
{
    Result<BinaryView> BinaryView::slice(const size_t offset, const size_t length) const
    {
        if (offset > bytes_.size() || length > bytes_.size() - offset)
            return std::unexpected(std::format(
                "Byte range [{}, +{}) exceeds buffer size {}", offset, length, bytes_.size()));
        return BinaryView(bytes_.subspan(offset, length));
    }

    Result<BinaryView> BinaryView::tail(const size_t offset) const
    {
        if (offset > bytes_.size())
            return std::unexpected(std::format("Byte offset {} exceeds buffer size {}", offset, bytes_.size()));
        return BinaryView(bytes_.subspan(offset));
    }

    Result<BinaryView> BinaryView::table(const size_t offset, const size_t count, const size_t stride) const
    {
        auto remaining = tail(offset);
        if (!remaining)
            return std::unexpected(remaining.error());
        if (stride == 0 || count > remaining->bytes_.size() / stride)
            return std::unexpected("Record table exceeds buffer size");
        return remaining->slice(0, count * stride);
    }

    Result<std::string_view> BinaryView::nullTerminatedString(const size_t offset) const
    {
        auto remaining = tail(offset);
        if (!remaining)
            return std::unexpected(remaining.error());
        const auto bytes = remaining->bytes_;
        const auto end = std::ranges::find(bytes, uint8_t{0});
        if (end == bytes.end())
            return std::unexpected("String is not null-terminated");
        return std::string_view(reinterpret_cast<const char*>(bytes.data()),
                                static_cast<size_t>(end - bytes.begin()));
    }
}
