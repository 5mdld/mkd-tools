//
// kiwakiwaaにより 2026/02/27 に作成されました。
//

#include "read_sequence.hpp"

namespace MKD
{
    ReadSequence::ReadSequence(BinaryFileReader& reader)
        : reader_(reader)
    {
    }


    std::vector<uint8_t> ReadSequence::readBytes(const size_t count)
    {
        if (failed_) return {};

        auto result = reader_.readBytes(count);
        if (!result)
        {
            captureError(result.error());
            return {};
        }
        return std::move(*result);
    }


    std::string ReadSequence::readString(const size_t size)
    {
        if (failed_) return {};

        auto result = reader_.readBytesIntoString(size);
        if (!result)
        {
            captureError(result.error());
            return {};
        }
        return std::move(*result);
    }


    size_t ReadSequence::remaining() const
    {
        if (failed_) return 0;
        return reader_.remainingBytes();
    }


    void ReadSequence::seek(const size_t offset)
    {
        if (failed_) return;

        if (auto result = reader_.seek(offset); !result)
            captureError(result.error());
    }


    ReadSequence::operator bool() const { return !failed_; }


    const std::string& ReadSequence::error() const { return error_; }


    void ReadSequence::captureError(const std::string& err)
    {
        failed_ = true;
        error_ = err;
    }
}
