//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "resource_crypto.hpp"

#include <algorithm>

namespace MKD::detail::resource_crypto
{
    void decryptBlocks(const std::span<const uint8_t> src, std::span<uint8_t> dst,
                       const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < src.size(); offset += 16)
        {
            const auto& permutation = kData2[table];
            const auto* mask = schedule.block(offset);
            for (size_t i = 0; i < 16; ++i)
                dst[offset + permutation[i]] = src[offset + i] ^ mask[permutation[i]];

            if (++table == kData2.size())
                table = 0;
        }
    }

    void encryptBlocks(std::span<uint8_t> data, const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < data.size(); offset += 16)
        {
            const auto& permutation = kData2[table];
            const auto* mask = schedule.block(offset);
            std::array<uint8_t, 16> block;
            std::copy_n(data.data() + offset, block.size(), block.data());
            for (size_t i = 0; i < 16; ++i)
                data[offset + i] = block[permutation[i]] ^ mask[permutation[i]];

            if (++table == kData2.size())
                table = 0;
        }
    }
}
