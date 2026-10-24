//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/dictionary/inline_text.hpp"
#include "dictionary/content/content_markup.hpp"
#include "io/write_validation.hpp"
#include "unicode/utf8.hpp"

namespace MKD
{
    void InlineText::append(InlineElement element)
    {
        elements_.push_back(std::move(element));
    }

    std::span<const InlineElement> InlineText::elements() const noexcept { return elements_; }

    Result<InlineText> InlineText::fromHeadline(std::u16string_view text)
    {
        if (auto valid = detail::WriteValidation::text(text, true); !valid)
            return std::unexpected(valid.error());

        const auto utf8 = detail::unicode::toUtf8(text);
        InlineText result;
        size_t position = 0;
        while (position < utf8.size())
        {
            const auto begin = utf8.find('{', position);
            const auto endText = begin == std::string::npos ? utf8.size() : begin;
            if (endText != position)
                result.append(utf8.substr(position, endText - position));

            if (begin == std::string::npos)
                break;

            auto token = detail::HeadlineToken::readLeading(std::string_view(utf8).substr(begin));
            if (!token)
                return std::unexpected(token.error());

            result.append(token->element());
            position = begin + token->source.size();
        }
        return result;
    }

    Result<std::u16string> InlineText::toHeadline() const
    {
        std::string result;
        for (const auto& element : elements_)
        {
            auto encoded = std::visit(detail::HeadlineEncoder{}, element);
            if (!encoded)
                return std::unexpected(encoded.error());

            result += *encoded;
        }
        return detail::unicode::toUtf16(result);
    }

    Result<std::string> InlineText::toXhtml() const
    {
        std::string result;
        for (const auto& element : elements_)
        {
            auto encoded = std::visit(detail::XhtmlEncoder{}, element);
            if (!encoded)
                return std::unexpected(encoded.error());

            result += *encoded;
        }
        return result;
    }

    Result<std::string> InlineText::plainText() const
    {
        std::string result;
        for (const auto& element : elements_)
        {
            auto text = std::visit(detail::PlainTextEncoder{}, element);
            if (!text)
                return std::unexpected(text.error());

            if (auto valid = detail::WriteValidation::text(*text, true); !valid)
                return std::unexpected(valid.error());

            result += *text;
        }
        return result;
    }
}
