//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "resource_crypto.hpp"

#include <arm_neon.h>

namespace MKD::detail::resource_crypto
{
    void decryptBlocks(const std::span<const uint8_t> src, std::span<uint8_t> dst,
                       const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < src.size(); offset += 16)
        {
            const auto indices = vld1q_u8(kInverseData2[table].data());
            const auto block = vqtbl1q_u8(vld1q_u8(src.data() + offset), indices);
            vst1q_u8(dst.data() + offset, veorq_u8(block, vld1q_u8(schedule.block(offset))));

            if (++table == kData2.size())
                table = 0;
        }
    }

    void encryptBlocks(std::span<uint8_t> data, const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < data.size(); offset += 16)
        {
            const auto indices = vld1q_u8(kData2[table].data());
            const auto block = veorq_u8(vld1q_u8(data.data() + offset), vld1q_u8(schedule.block(offset)));
            vst1q_u8(data.data() + offset, vqtbl1q_u8(block, indices));

            if (++table == kData2.size())
                table = 0;
        }
    }
}
