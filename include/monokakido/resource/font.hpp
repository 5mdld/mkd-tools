//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "monokakido/resource/rsc/rsc.hpp"

#include <string>


namespace monokakido
{
    class Font
    {
    public:
        Font(std::string name, Rsc data)
            : name_(std::move(name)), data_(std::move(data))
        {}

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        Font(Font&&) noexcept = default;
        Font& operator=(Font&&) noexcept = default;

        [[nodiscard]] const std::string& name() const noexcept { return name_; }

        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string>
        getData() const { return data_.getSequential(); }

        [[nodiscard]] std::optional<std::string>
        detectType() const { return data_.detectFontType(); }

        [[nodiscard]] bool isEmpty() const noexcept { return data_.empty(); }

    private:
        std::string name_;
        Rsc data_;
    };
}