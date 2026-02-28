//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>


namespace MKD
{
    class ZlibDecompressor
    {
    public:

        ZlibDecompressor() = default;
        ~ZlibDecompressor() = default;

        // not copyable
        ZlibDecompressor(const ZlibDecompressor&) = delete;
        ZlibDecompressor& operator=(const ZlibDecompressor&) = delete;

        // movable
        ZlibDecompressor(ZlibDecompressor&&) = default;
        ZlibDecompressor& operator=(ZlibDecompressor&&) = default;

        // span is valid until next 'decompress' call or object destruction
        [[nodiscard]] Result<std::span<const uint8_t>> decompress(std::span<const uint8_t> compressed, size_t expectedSize) const;

        [[nodiscard]] std::vector<uint8_t> takeBuffer() const;

        static bool isZlibCompressed(std::span<const uint8_t> data);

    private:
        mutable std::vector<uint8_t> decompressBuffer_;
    };
}
