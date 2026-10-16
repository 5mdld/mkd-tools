//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <span>
#include <vector>

namespace MKD::detail
{
    class BinaryBuffer
    {
    public:
        explicit BinaryBuffer(size_t reserveHint = 4096);

        template <std::unsigned_integral T, size_t Bytes = sizeof(T)>
            requires (!std::same_as<T, bool> && Bytes > 0 && Bytes <= sizeof(T))
        void writeLittle(T value)
        {
            write<T, Bytes, std::endian::little>(value);
        }

        template <std::unsigned_integral T, size_t Bytes = sizeof(T)>
            requires (!std::same_as<T, bool> && Bytes > 0 && Bytes <= sizeof(T))
        void writeBig(T value)
        {
            write<T, Bytes, std::endian::big>(value);
        }

        void writeBytes(std::span<const uint8_t> data);
        void writeByte(uint8_t value);
        void writePadding(size_t count);
        [[nodiscard]] size_t position() const noexcept;
        [[nodiscard]] std::span<const uint8_t> data() const noexcept;
        [[nodiscard]] std::vector<uint8_t> take();

    private:
        template <typename T, size_t Bytes, std::endian Order>
        void write(T value)
        {
            for (size_t i = 0; i < Bytes; ++i)
            {
                const size_t shift = Order == std::endian::little ? i : Bytes - i - 1;
                buffer_.push_back(static_cast<uint8_t>(value >> (shift * 8)));
            }
        }

        std::vector<uint8_t> buffer_;
    };
}
