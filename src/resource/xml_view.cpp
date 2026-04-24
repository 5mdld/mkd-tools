//
// kiwakiwaaにより 2026/01/29 に作成されました。
//

#include "MKD/resource/xml_view.hpp"
#include "unicode/unicode.hpp"

#include <format>

namespace MKD
{

    XmlView::XmlView(const std::span<const uint8_t> data)
        : data_(data)
    {
    }


    std::expected<std::string_view, std::string> XmlView::asStringView() const
    {
        if (auto validationResult = validate(); !validationResult)
            return std::unexpected(validationResult.error());

        return std::string_view(
            reinterpret_cast<const char*>(data_.data()),
            data_.size()
        );
    }


    std::expected<void, std::string> XmlView::validate() const
    {
        if (const auto invalidOffset = detail::unicode::firstInvalidUtf8Offset(data_))
        {
            return std::unexpected(
                std::format("Invalid UTF-8 sequence at byte offset {}", *invalidOffset)
            );
        }

        return {};
    }


    std::span<const uint8_t> XmlView::data() const noexcept
    {
        return data_;
    }

}
