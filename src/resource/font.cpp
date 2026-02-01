//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/resource/font.hpp"


namespace monokakido
{
    Font::Font(std::string name, Rsc data)
        : name_(std::move(name)), rsc_(std::move(data))
    {
    }


    const std::string& Font::name() const noexcept
    {
        return name_;
    }


    std::expected<std::span<const uint8_t>, std::string> Font::getData() const
    {
        if (!buffer_.empty())
            return std::span<const uint8_t>(buffer_);

        return loadFontData();
    }


    std::optional<std::string> Font::detectType() const
    {
        if (buffer_.empty() || buffer_.size() < 8)
            return std::nullopt;

        const uint32_t magic = (buffer_[0] << 24 |
                                buffer_[1] << 16 |
                                buffer_[2] << 8 |
                                buffer_[3]);

        if (magic == 0x4F54544F) // "OTTO"
            return "otf"; // OpenType

        return std::nullopt;
    }


    bool Font::isEmpty() const noexcept
    {
        return rsc_.empty();
    }


    std::expected<std::span<const uint8_t>, std::string> Font::loadFontData() const
    {
        buffer_.clear();

        for (const auto& [itemId, data] : rsc_)
        {
            buffer_.insert(buffer_.end(), data.begin(), data.end());
        }

        if (buffer_.empty())
            return std::unexpected("Font data is empty: {}");

        return std::span<const uint8_t>(buffer_);
    }
}
