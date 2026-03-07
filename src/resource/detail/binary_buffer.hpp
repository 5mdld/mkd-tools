//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#pragma once

#include <bit>
#include <cstdint>
#include <span>
#include <vector>

namespace MKD::detail
{
    class BinaryBuffer
    {
    public:
        explicit BinaryBuffer(size_t reserveHint = 4096);

        template<typename T>
        void writeStruct(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
            buffer_.insert(buffer_.end(), bytes, bytes + sizeof(T));
        }

        // write an integer in little endian
        template<std::integral T>
        void writeLE(T value)
        {
            if constexpr (std::endian::native == std::endian::big)
                value = std::byteswap(value);

            writeStruct(value);
        }

        // write raw bytes
        void writeBytes(std::span<const uint8_t> data);

        // write a null-terminated utf-8 string including the terminator
        void writeStringNul(std::string_view str);

        // write N zero bytes
        void writePadding(size_t count);

        // Overwrite bytes at a previous position
        template<std::integral T>
        void patchLE(const size_t offset, T value)
        {
            if constexpr (std::endian::native == std::endian::big)
                value = std::byteswap(value);

            std::memcpy(buffer_.data() + offset, &value, sizeof(T));
        }

        // Reserve a slot for a value to be patched later, returns the offset
        template<std::integral T>
        size_t reserveSlot()
        {
            const size_t offset = buffer_.size();
            buffer_.resize(buffer_.size() + sizeof(T), 0);
            return offset;
        }

        // Align the write position to the given boundary with zero padding
        void alignTo(size_t alignment);

        // Current write position
        [[nodiscard]] size_t position() const noexcept;

        // Access the accumulated buffer
        [[nodiscard]] std::span<const uint8_t> data() const noexcept;

        // Take ownership of the buffer (leaves writer empty)
        [[nodiscard]] std::vector<uint8_t> take();

    private:
        std::vector<uint8_t> buffer_;
    };
}
