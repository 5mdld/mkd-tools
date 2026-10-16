//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace MKD::detail::zlib
{
    [[nodiscard]] Result<std::vector<uint8_t>> compress(std::span<const uint8_t> data, int level = -1);
    [[nodiscard]] Result<std::vector<uint8_t>> decompress(std::span<const uint8_t> data, size_t initialSize = 4096);
    [[nodiscard]] bool isCompressed(std::span<const uint8_t> data) noexcept;
}
