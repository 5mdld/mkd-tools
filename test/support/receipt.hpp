//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "test_dictionary.hpp"
#include "receipt/purchase_receipt.hpp"
#include "platform/macos/chacha_poly.hpp"
#include <array>

namespace MKD::test
{
    inline constexpr std::array<uint8_t, 32> receiptKey{'r', 'H', 'D', 'M', 'g', 'l', '1', 'P', 'S', 'W', 'o',
                                                        'r', 'X', 'j', 'q', 'g', 'j', 'S', 'v', 'o', 'u', 'C',
                                                        'L', 'x', 'w', 'a', 'n', 'W', 's', 'H', 'S', 'A'};

    inline std::vector<uint8_t> encodedReceipt(std::string_view json)
    {
        return require(detail::sealChaChaPoly(bytes(json), receiptKey));
    }
}
