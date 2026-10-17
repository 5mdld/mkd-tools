//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "binary/binary_view.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace MKD::detail
{
    class FontFormat
    {
    public:
        [[nodiscard]] static std::optional<std::string_view> detect(std::span<const uint8_t> bytes)
        {
            const auto magic = BinaryView(bytes).readBig<uint32_t>(0);
            if (!magic)
                return std::nullopt;

            for (const auto& signature : signatures)
                if (signature.magic == *magic) return signature.extension;

            return std::nullopt;
        }

        [[nodiscard]] static bool supportsExtension(std::string_view extension)
        {
            return std::ranges::any_of(signatures, [extension](const Signature& signature)
            {
                return signature.extension == extension;
            });
        }

    private:
        struct Signature
        {
            uint32_t magic;
            std::string_view extension;
        };

        inline static constexpr std::array signatures{
            Signature{0x00010000, ".ttf"},
            Signature{0x74727565, ".ttf"},
            Signature{0x4F54544F, ".otf"},
            Signature{0x74746366, ".ttc"}
        };
    };
}
