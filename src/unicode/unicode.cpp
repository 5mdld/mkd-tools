#include "unicode.hpp"

#include <utf8proc.h>

#include <array>
#include <format>
#include <stdexcept>

namespace MKD::detail::unicode
{
    namespace
    {
        constexpr Codepoint HIRAGANA_START = 0x3041;
        constexpr Codepoint HIRAGANA_END   = 0x3096;
        constexpr Codepoint HIRAGANA_TO_KATAKANA_OFFSET = 0x60;

        constexpr Codepoint ASCII_APOSTROPHE   = 0x0027;
        constexpr Codepoint RIGHT_SINGLE_QUOTE = 0x2019;
        constexpr Codepoint COMBINING_ACUTE    = 0x0301;

        constexpr Codepoint UNICODE_MAX = 0x10FFFF;
        constexpr Codepoint HIGH_SURROGATE_START = 0xD800;
        constexpr Codepoint HIGH_SURROGATE_END = 0xDBFF;
        constexpr Codepoint LOW_SURROGATE_START = 0xDC00;
        constexpr Codepoint LOW_SURROGATE_END = 0xDFFF;
        constexpr Codepoint SURROGATE_OFFSET = 0x10000;

        [[nodiscard]] bool isContinuationByte(const unsigned char byte) noexcept
        {
            return (byte & 0xC0) == 0x80;
        }

        [[nodiscard]] bool isUnicodeScalarValue(const Codepoint cp) noexcept
        {
            return cp <= UNICODE_MAX && !(cp >= HIGH_SURROGATE_START && cp <= LOW_SURROGATE_END);
        }

        [[nodiscard]] const utf8proc_uint8_t* bytes(const std::string_view text) noexcept
        {
            return reinterpret_cast<const utf8proc_uint8_t*>(text.data());
        }

        [[nodiscard]] const utf8proc_uint8_t* bytes(const std::span<const uint8_t> data) noexcept
        {
            return reinterpret_cast<const utf8proc_uint8_t*>(data.data());
        }

        [[nodiscard]] Codepoint decodeAt(const std::string_view text, const size_t offset, size_t& bytesRead)
        {
            if (offset >= text.size())
                throw std::out_of_range("UTF-8 decode offset is past the end of the string");

            utf8proc_int32_t cp = 0;
            const auto result = utf8proc_iterate(
                bytes(text) + offset,
                static_cast<utf8proc_ssize_t>(text.size() - offset),
                &cp
            );

            if (result < 0)
                throw std::runtime_error(std::format("Invalid UTF-8 sequence at byte offset {}", offset));

            bytesRead = static_cast<size_t>(result);
            return static_cast<Codepoint>(cp);
        }

        [[nodiscard]] Codepoint combineSurrogates(const char16_t high, const char16_t low) noexcept
        {
            return SURROGATE_OFFSET
                   + ((static_cast<Codepoint>(high) - HIGH_SURROGATE_START) << 10)
                   + (static_cast<Codepoint>(low) - LOW_SURROGATE_START);
        }
    }


    std::optional<size_t> firstInvalidUtf8Offset(const std::span<const uint8_t> data) noexcept
    {
        size_t offset = 0;
        while (offset < data.size())
        {
            utf8proc_int32_t cp = 0;
            const auto result = utf8proc_iterate(
                bytes(data) + offset,
                static_cast<utf8proc_ssize_t>(data.size() - offset),
                &cp
            );

            if (result < 0)
                return offset;

            offset += static_cast<size_t>(result);
        }

        return std::nullopt;
    }


    std::optional<size_t> firstInvalidUtf8Offset(const std::string_view text) noexcept
    {
        return firstInvalidUtf8Offset(std::span{
            reinterpret_cast<const uint8_t*>(text.data()),
            text.size()
        });
    }


    Codepoint nextCodepoint(const std::string_view text, size_t& offset)
    {
        size_t bytesRead = 0;
        const Codepoint cp = decodeAt(text, offset, bytesRead);
        offset += bytesRead;
        return cp;
    }


    Codepoint previousCodepoint(const std::string_view text, size_t& offset)
    {
        if (offset == 0)
            throw std::out_of_range("UTF-8 reverse decode offset is at the beginning of the string");

        if (offset > text.size())
            throw std::out_of_range("UTF-8 reverse decode offset is past the end of the string");

        size_t start = offset - 1;
        while (start > 0 && isContinuationByte(static_cast<unsigned char>(text[start])))
            --start;

        size_t bytesRead = 0;
        const Codepoint cp = decodeAt(text, start, bytesRead);
        if (start + bytesRead != offset)
            throw std::runtime_error(std::format("Invalid UTF-8 sequence ending at byte offset {}", offset));

        offset = start;
        return cp;
    }


    size_t codepointCount(const std::string_view text)
    {
        size_t offset = 0;
        size_t count = 0;
        while (offset < text.size())
        {
            static_cast<void>(nextCodepoint(text, offset));
            ++count;
        }
        return count;
    }


    std::u32string toUtf32(const std::string_view text)
    {
        std::u32string result;
        result.reserve(text.size());

        size_t offset = 0;
        while (offset < text.size())
            result.push_back(nextCodepoint(text, offset));

        return result;
    }


    std::string toUtf8(const std::u32string_view text)
    {
        std::string result;
        result.reserve(text.size() * 4);

        for (const Codepoint cp : text)
            appendUtf8(result, cp);

        return result;
    }


    std::string toUtf8(const std::u16string_view text)
    {
        std::string result;
        result.reserve(text.size() * 3);

        for (size_t i = 0; i < text.size(); ++i)
        {
            const Codepoint unit = text[i];
            if (unit >= HIGH_SURROGATE_START && unit <= HIGH_SURROGATE_END)
            {
                if (i + 1 >= text.size())
                    throw std::runtime_error("Invalid UTF-16 sequence: missing low surrogate");

                const Codepoint low = text[++i];
                if (low < LOW_SURROGATE_START || low > LOW_SURROGATE_END)
                    throw std::runtime_error("Invalid UTF-16 sequence: high surrogate not followed by low surrogate");

                appendUtf8(result, combineSurrogates(static_cast<char16_t>(unit), static_cast<char16_t>(low)));
                continue;
            }

            if (unit >= LOW_SURROGATE_START && unit <= LOW_SURROGATE_END)
                throw std::runtime_error("Invalid UTF-16 sequence: unpaired low surrogate");

            appendUtf8(result, unit);
        }

        return result;
    }


    void appendUtf8(std::string& output, const Codepoint cp)
    {
        if (!isUnicodeScalarValue(cp))
            throw std::runtime_error(std::format("Invalid Unicode scalar value U+{:04X}", static_cast<uint32_t>(cp)));

        std::array<utf8proc_uint8_t, 4> encoded{};
        const auto bytesWritten = utf8proc_encode_char(static_cast<utf8proc_int32_t>(cp), encoded.data());
        if (bytesWritten <= 0)
            throw std::runtime_error(std::format("Failed to encode Unicode scalar value U+{:04X}", static_cast<uint32_t>(cp)));

        output.append(reinterpret_cast<const char*>(encoded.data()), static_cast<size_t>(bytesWritten));
    }


    Codepoint hiraganaToKatakana(const Codepoint cp) noexcept
    {
        if (cp >= HIRAGANA_START && cp <= HIRAGANA_END)
            return cp + HIRAGANA_TO_KATAKANA_OFFSET;

        return cp;
    }


    Codepoint keywordSearchFold(const Codepoint cp) noexcept
    {
        if (cp == ASCII_APOSTROPHE || cp == RIGHT_SINGLE_QUOTE)
            return COMBINING_ACUTE;

        return toLowercase(hiraganaToKatakana(cp));
    }


    bool isJapaneseScript(const Codepoint cp) noexcept
    {
        return (cp >= 0x3040 && cp <= 0x309F)
               || (cp >= 0x30A0 && cp <= 0x30FF)
               || (cp >= 0x4E00 && cp <= 0x9FFF)
               || (cp >= 0x3400 && cp <= 0x4DBF);
    }


    bool containsJapaneseScript(const std::string_view text)
    {
        size_t offset = 0;
        while (offset < text.size())
        {
            if (isJapaneseScript(nextCodepoint(text, offset)))
                return true;
        }

        return false;
    }
}
