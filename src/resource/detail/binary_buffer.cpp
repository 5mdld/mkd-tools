//
// kiwakiwaaにより 2026/03/06 に作成されました。
//

#include "binary_buffer.hpp"

namespace MKD::detail
{

    BinaryBuffer::BinaryBuffer(const size_t reserveHint)
    {
        buffer_.reserve(reserveHint);
    }


    void BinaryBuffer::writeBytes(std::span<const uint8_t> data)
    {
        buffer_.insert(buffer_.end(), data.begin(), data.end());
    }


    void BinaryBuffer::writeStringNul(std::string_view str)
    {
        const auto* bytes = reinterpret_cast<const uint8_t*>(str.data());
        buffer_.insert(buffer_.end(), bytes, bytes + str.size());
        buffer_.push_back(0);
    }


    void BinaryBuffer::writePadding(const size_t count)
    {
        buffer_.resize(buffer_.size() + count, 0);
    }


    void BinaryBuffer::alignTo(const size_t alignment)
    {
        if (const size_t remainder = buffer_.size() % alignment; remainder != 0)
            writePadding(alignment - remainder);
    }


    size_t BinaryBuffer::position() const noexcept
    {
        return buffer_.size();
    }


    std::span<const uint8_t> BinaryBuffer::data() const noexcept
    {
        return buffer_;
    }


    std::vector<uint8_t> BinaryBuffer::take()
    {
        return std::move(buffer_);
    }

}