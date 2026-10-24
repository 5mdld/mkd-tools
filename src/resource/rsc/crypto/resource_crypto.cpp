//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "resource_crypto.hpp"

#include <algorithm>

namespace MKD::detail::resource_crypto
{
    XorSchedule::XorSchedule(const std::array<uint8_t, 32>& key, const uint32_t outputLength)
    {
        const size_t start = outputLength & 31;
        std::array<uint8_t, 32> period;
        for (size_t i = 0; i < period.size(); ++i)
            period[i] = key[i] ^ kData1[i];

        std::ranges::rotate_copy(period, period.begin() + start, bytes.begin() + 32);
        std::copy_n(bytes.begin() + 32, 32, bytes.begin());

        // Before the first table wrap, the key starts at zero rather than at the table offset.
        for (size_t i = 0; i < 32 - start; ++i)
            bytes[i] = key[i] ^ kData1[start + i];
    }
}
