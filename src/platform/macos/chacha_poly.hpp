//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace MKD::detail
{
    [[nodiscard]] Result<std::vector<uint8_t>> sealChaChaPoly(
        std::span<const uint8_t> plaintext, std::span<const uint8_t, 32> key);
    [[nodiscard]] Result<std::vector<uint8_t>> openChaChaPoly(
        std::span<const uint8_t> encrypted, std::span<const uint8_t, 32> key);
}
