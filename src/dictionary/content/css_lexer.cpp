//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "css_lexer.hpp"
#include "unicode/utf8.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>

namespace MKD::detail::css
{
    Result<void> CssLexer::whitespace()
    {
        while (position_ < source_.size())
        {
            if (std::isspace(static_cast<unsigned char>(source_[position_])))
                ++position_;
            else if (source_.substr(position_).starts_with("/*"))
            {
                const auto end = source_.find("*/", position_ + 2);
                if (end == std::string_view::npos)
                {
                    errorOffset_ = position_;
                    return std::unexpected("Unterminated CSS comment");
                }
                position_ = end + 2;
            }
            else
                break;
        }
        return {};
    }

    Result<void> CssLexer::escape(std::string& value)
    {
        errorOffset_ = position_ - 1;
        if (position_ == source_.size())
            return std::unexpected("Unterminated CSS escape");

        if (source_[position_] == '\n' || source_[position_] == '\r')
        {
            const auto c = source_[position_++];
            if (c == '\r' && position_ < source_.size() && source_[position_] == '\n')
                ++position_;
            return {};
        }

        const auto start = position_;
        while (position_ < source_.size() && position_ - start < 6 &&
            std::isxdigit(static_cast<unsigned char>(source_[position_])))
            ++position_;

        if (position_ == start)
            value += source_[position_++];
        else
        {
            uint32_t cp{};
            std::from_chars(source_.data() + start, source_.data() + position_, cp, 16);
            if (!cp || cp > unicode::kUnicodeMax ||
                (cp >= unicode::kHighSurrogateStart && cp <= unicode::kLowSurrogateEnd))
                return std::unexpected("Invalid Unicode CSS escape");

            unicode::appendUtf8(value, cp);
            if (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_])))
            {
                const auto c = source_[position_++];
                if (c == '\r' && position_ < source_.size() && source_[position_] == '\n')
                    ++position_;
            }
        }
        return {};
    }

    Result<std::string> CssLexer::string(const char quote)
    {
        const auto begin = position_ - 1;
        std::string value;
        while (position_ < source_.size())
        {
            const auto c = source_[position_++];
            if (c == quote)
                return value;

            if (c == '\\')
            {
                if (auto valid = escape(value); !valid)
                    return std::unexpected(valid.error());
            }
            else if (c == '\n' || c == '\r')
            {
                errorOffset_ = position_ - 1;
                return std::unexpected("Newline in CSS string");
            }
            else
                value += c;
        }
        errorOffset_ = begin;
        return std::unexpected("Unterminated CSS string");
    }

    Result<std::string> CssLexer::url()
    {
        const auto begin = position_ - 1;
        if (auto valid = whitespace(); !valid)
            return std::unexpected(valid.error());

        std::string value;
        if (position_ < source_.size() && (source_[position_] == '\'' || source_[position_] == '"'))
        {
            auto parsed = string(source_[position_++]);
            if (!parsed)
                return std::unexpected(parsed.error());

            value = std::move(*parsed);
            if (auto valid = whitespace(); !valid)
                return std::unexpected(valid.error());
        }
        else
        {
            while (position_ < source_.size() && source_[position_] != ')')
            {
                const auto c = source_[position_++];
                if (c == '\\')
                {
                    if (auto valid = escape(value); !valid)
                        return std::unexpected(valid.error());
                }
                else
                    value += c;
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.pop_back();
        }

        if (position_ == source_.size() || source_[position_++] != ')')
        {
            errorOffset_ = begin;
            return std::unexpected("Unterminated CSS url()");
        }

        return value;
    }

    content::AnalysisResult<CssToken> CssLexer::next()
    {
        if (auto valid = whitespace(); !valid)
            return std::unexpected(content::SourceError{valid.error(), content::SourceRange{errorOffset_, 0}});

        const auto begin = position_;
        if (begin == source_.size())
            return CssToken{};

        const auto c = source_[position_++];
        CssToken token{.source = {begin, 1}};
        if (c == '\'' || c == '"')
        {
            auto value = string(c);
            if (!value)
                return std::unexpected(content::SourceError{value.error(), content::SourceRange{errorOffset_, 0}});

            token.kind = CssTokenKind::String;
            token.value = std::move(*value);
        }
        else if (std::isalpha(static_cast<unsigned char>(c)) || c == '@' || c == '-' || c == '_' || c == '\\')
        {
            token.kind = c == '@' ? CssTokenKind::AtKeyword : CssTokenKind::Identifier;
            if (c == '\\')
            {
                if (auto valid = escape(token.value); !valid)
                    return std::unexpected(content::SourceError{valid.error(), content::SourceRange{errorOffset_, 0}});
            }
            else
                token.value += c;

            while (position_ < source_.size())
            {
                const auto next = source_[position_];
                if (std::isalnum(static_cast<unsigned char>(next)) || next == '-' || next == '_')
                {
                    token.value += next;
                    ++position_;
                }
                else if (next == '\\')
                {
                    ++position_;
                    if (auto valid = escape(token.value); !valid)
                        return std::unexpected(content::SourceError{
                            valid.error(), content::SourceRange{errorOffset_, 0}
                        });
                }
                else
                    break;
            }
            if (std::ranges::equal(token.value, std::string_view("url"), {},
                                   [](const unsigned char c) { return static_cast<char>(std::tolower(c)); }) &&
                position_ < source_.size() && source_[position_] == '(')
            {
                ++position_;
                auto value = url();
                if (!value)
                    return std::unexpected(content::SourceError{value.error(), content::SourceRange{errorOffset_, 0}});

                token.kind = CssTokenKind::Url;
                token.value = std::move(*value);
            }
        }
        else
        {
            token.kind = CssTokenKind::Delimiter;
            token.value = c;
        }
        token.source.length = position_ - begin;
        return token;
    }
}
