//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "monokakido/resource/rsc/rsc.hpp"

#include <string>
#include <vector>

namespace monokakido
{
    class Font
    {
    public:
        Font(std::string name, Rsc data);

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        Font(Font&&) noexcept = default;
        Font& operator=(Font&&) noexcept = default;

        [[nodiscard]] const std::string& name() const noexcept;

        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string> getData() const;

        [[nodiscard]] std::optional<std::string> detectType() const;

        [[nodiscard]] bool isEmpty() const noexcept;

    private:
        [[nodiscard]] std::expected<std::span<const uint8_t>, std::string> loadFontData() const;

        std::string name_;
        Rsc rsc_;
        mutable std::vector<uint8_t> buffer_;
    };
}
