//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

namespace MKD::detail
{
    class BinaryView
    {
    public:
        BinaryView() noexcept = default;

        explicit BinaryView(std::span<const uint8_t> bytes) noexcept : bytes_(bytes)
        {
        }

        template <std::unsigned_integral T, size_t Bytes = sizeof(T)>
            requires (!std::same_as<T, bool> && Bytes > 0 && Bytes <= sizeof(T))
        [[nodiscard]] Result<T> readLittle(const size_t offset) const
        {
            return read<T, Bytes, std::endian::little>(offset);
        }

        template <std::unsigned_integral T, size_t Bytes = sizeof(T)>
            requires (!std::same_as<T, bool> && Bytes > 0 && Bytes <= sizeof(T))
        [[nodiscard]] Result<T> readBig(const size_t offset) const
        {
            return read<T, Bytes, std::endian::big>(offset);
        }

        [[nodiscard]] Result<BinaryView> slice(size_t offset, size_t length) const;
        [[nodiscard]] Result<BinaryView> tail(size_t offset) const;
        [[nodiscard]] Result<BinaryView> table(size_t offset, size_t count, size_t stride) const;
        [[nodiscard]] Result<std::string_view> nullTerminatedString(size_t offset) const;
        [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }

    private:
        template <typename T, size_t Bytes, std::endian Order>
        Result<T> read(const size_t offset) const
        {
            auto field = slice(offset, Bytes);
            if (!field)
                return std::unexpected(field.error());

            T value = 0;
            if constexpr (Bytes == sizeof(T))
            {
                std::memcpy(&value, field->bytes_.data(), sizeof(T));
                if constexpr (std::endian::native != Order)
                    value = std::byteswap(value);
            }
            else
            {
                for (size_t i = 0; i < Bytes; ++i)
                {
                    const size_t shift = Order == std::endian::little ? i : Bytes - i - 1;
                    value |= static_cast<T>(field->bytes_[i]) << (shift * 8);
                }
            }
            return value;
        }

        std::span<const uint8_t> bytes_;
    };
}
