//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "resource_crypto.hpp"

#include <tmmintrin.h>

namespace MKD::detail::resource_crypto
{
    void decryptBlocks(const std::span<const uint8_t> src, std::span<uint8_t> dst,
                       const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < src.size(); offset += 16)
        {
            const auto indices = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kInverseData2[table].data()));
            const auto input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src.data() + offset));
            const auto mask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(schedule.block(offset)));
            const auto block = _mm_xor_si128(_mm_shuffle_epi8(input, indices), mask);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst.data() + offset), block);

            if (++table == kData2.size())
                table = 0;
        }
    }

    void encryptBlocks(std::span<uint8_t> data, const XorSchedule& schedule, const uint32_t outputLength)
    {
        size_t table = outputLength % kData2.size();
        for (size_t offset = 0; offset < data.size(); offset += 16)
        {
            const auto indices = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kData2[table].data()));
            const auto input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data.data() + offset));
            const auto mask = _mm_loadu_si128(reinterpret_cast<const __m128i*>(schedule.block(offset)));
            const auto block = _mm_shuffle_epi8(_mm_xor_si128(input, mask), indices);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(data.data() + offset), block);

            if (++table == kData2.size())
                table = 0;
        }
    }
}
