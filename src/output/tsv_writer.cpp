//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "tsv_writer.hpp"
#include "unicode/unicode.hpp"

#include <format>

namespace MKD::detail
{
    TsvWriter::TsvWriter(OutputFile& output) : output_(output)
    {
        buffer_.reserve(64 * 1024);
    }

    void TsvWriter::field(std::string_view value)
    {
        if (!firstField_)
            buffer_.push_back('\t');

        firstField_ = false;
        for (const char character : value)
        {
            switch (character)
            {
            case '\t': buffer_ += "\\t";
                break;
            case '\n': buffer_ += "\\n";
                break;
            case '\r': buffer_ += "\\r";
                break;
            case '\\': buffer_ += "\\\\";
                break;
            default: buffer_.push_back(character);
                break;
            }
        }
    }

    void TsvWriter::field(std::u16string_view value)
    {
        if (error_)
            return;

        try
        {
            field(unicode::toUtf8(value));
        }
        catch (const std::runtime_error& error)
        {
            error_ = error.what();
        }
    }

    void TsvWriter::field(EntryId value)
    {
        field(std::format("{:06}-{:04X}", value.pageId, value.itemId));
    }

    Result<void> TsvWriter::endRow()
    {
        if (error_)
            return std::unexpected(*error_);

        buffer_.push_back('\n');
        firstField_ = true;
        return buffer_.size() >= 64 * 1024 ? finish() : Result<void>{};
    }

    Result<void> TsvWriter::finish()
    {
        if (error_)
            return std::unexpected(*error_);

        auto written = output_.write(std::span(reinterpret_cast<const uint8_t*>(buffer_.data()), buffer_.size()));
        buffer_.clear();
        return written;
    }
}
