//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "content_references.hpp"
#include "io/write_validation.hpp"
#include "unicode/utf8.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>

namespace MKD::detail
{
    Result<ContentReferences> ContentReferences::headline(std::u16string_view text)
    {
        auto parsed = InlineText::fromHeadline(text);
        if (!parsed)
            return std::unexpected(parsed.error());

        ContentReferences references;
        for (const auto& element : parsed->elements())
        {
            if (const auto gaiji = std::get_if<Gaiji>(&element))
                references.assets.insert(gaiji->resource.path);
            else if (const auto link = std::get_if<EntryLink>(&element))
                references.targets.insert(link->target);
            else if (const auto raw = std::get_if<NativeMarkup>(&element))
            {
                const auto token = NativeLabelToken::read(raw->text);
                const auto name = token->fields.front();
                if (name == native::label::image && token->fields.size() >= 2)
                    references.assets.emplace(token->fields[1]);

                if (name == native::label::reference && token->fields.size() >= 2)
                    if (const auto target = NativeEntryReference::parse(token->fields[1]))
                        references.targets.insert(*target);

                if (std::ranges::find(native::label::commands, name) == native::label::commands.end() && !name.
                    starts_with("CID"))
                {
                    if (name.contains('/'))
                        references.assets.emplace(name);
                    else
                        references.gaijiNames.emplace(name);
                }
                for (const auto field : token->fields)
                    if (field.contains('{'))
                    {
                        auto nested = headline(unicode::toUtf16(field));
                        if (!nested)
                            return std::unexpected(nested.error());

                        references.assets.insert(nested->assets.begin(), nested->assets.end());
                        references.targets.insert(nested->targets.begin(), nested->targets.end());
                        references.gaijiNames.insert(nested->gaijiNames.begin(), nested->gaijiNames.end());
                    }
            }
        }
        return references;
    }

    Result<void> CssReferences::whitespace()
    {
        while (position_ < source_.size())
        {
            if (std::isspace(static_cast<unsigned char>(source_[position_])))
                ++position_;
            else if (source_.substr(position_).starts_with("/*"))
            {
                const auto end = source_.find("*/", position_ + 2);
                if (end == std::string_view::npos)
                    return std::unexpected("Unterminated CSS comment");
                position_ = end + 2;
            }
            else break;
        }
        return {};
    }

    Result<void> CssReferences::escape(std::string& value)
    {
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
            if (!cp || cp > unicode::kUnicodeMax || (cp >= unicode::kHighSurrogateStart && cp <=
                unicode::kLowSurrogateEnd))
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

    Result<std::string> CssReferences::string(const char quote)
    {
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
                return std::unexpected("Newline in CSS string");
            else
                value += c;
        }
        return std::unexpected("Unterminated CSS string");
    }

    Result<void> CssReferences::reference(std::string_view value)
    {
        if (value.empty() || value.front() == '#' || value.contains(':') || value.starts_with("//"))
            return {};

        value = value.substr(0, value.find_first_of("?#"));
        std::string decoded;
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] != '%')
                decoded += value[i];
            else
            {
                if (i + 2 >= value.size())
                    return std::unexpected("Invalid percent escape in CSS resource path");

                unsigned byte{};
                const auto result = std::from_chars(value.data() + i + 1, value.data() + i + 3, byte, 16);
                if (result.ec != std::errc{} || result.ptr != value.data() + i + 3)
                    return std::unexpected("Invalid percent escape in CSS resource path");

                decoded += static_cast<char>(byte);
                i += 2;
            }
        }
        const auto path = (directory_ / WriteValidation::fromUtf8(decoded)).lexically_normal().generic_u8string();
        const std::string utf8(reinterpret_cast<const char*>(path.data()), path.size());
        if (auto valid = WriteValidation::relativePath(utf8); !valid)
            return valid;

        paths_.insert(utf8);
        return {};
    }

    Result<void> CssReferences::url()
    {
        if (auto valid = whitespace(); !valid)
            return valid;

        std::string value;
        if (position_ < source_.size() && (source_[position_] == '\'' || source_[position_] == '"'))
        {
            auto parsed = string(source_[position_++]);
            if (!parsed)
                return std::unexpected(parsed.error());

            value = std::move(*parsed);
            if (auto valid = whitespace(); !valid)
                return valid;
        }
        else
        {
            while (position_ < source_.size() && source_[position_] != ')')
            {
                const auto c = source_[position_++];
                if (c == '\\')
                {
                    if (auto valid = escape(value); !valid)
                        return valid;
                }
                else value += c;
            }
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.pop_back();
        }

        if (position_ == source_.size() || source_[position_++] != ')')
            return std::unexpected("Unterminated CSS url()");

        return reference(value);
    }

    Result<void> CssReferences::scan()
    {
        while (position_ < source_.size())
        {
            if (auto valid = whitespace(); !valid)
                return valid;

            if (position_ == source_.size())
                break;

            const auto c = source_[position_++];
            if (c == '\'' || c == '"')
            {
                if (auto parsed = string(c); !parsed) return std::unexpected(parsed.error());
                continue;
            }

            if (!std::isalpha(static_cast<unsigned char>(c)) && c != '@' && c != '-' && c != '_' && c != '\\')
                continue;

            std::string name;
            if (c == '\\')
            {
                if (auto valid = escape(name); !valid)
                    return valid;
            }
            else
                name += c;

            while (position_ < source_.size())
            {
                const auto next = source_[position_];
                if (std::isalnum(static_cast<unsigned char>(next)) || next == '-' || next == '_')
                {
                    name += next;
                    ++position_;
                }
                else if (next == '\\')
                {
                    ++position_;
                    if (auto valid = escape(name); !valid)
                        return valid;
                }
                else
                    break;
            }

            std::ranges::transform(name, name.begin(), [](const unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

            if (name == "url" && position_ < source_.size() && source_[position_] == '(')
            {
                ++position_;
                if (auto valid = url(); !valid)
                    return valid;
            }
            else if (name == "@import")
            {
                if (auto valid = whitespace(); !valid)
                    return valid;
                if (position_ < source_.size() && (source_[position_] == '\'' || source_[position_] == '"'))
                {
                    auto value = string(source_[position_++]);
                    if (!value)
                        return std::unexpected(value.error());
                    if (auto valid = reference(*value); !valid)
                        return valid;
                }
            }
        }
        return {};
    }

    Result<std::set<std::string>> CssReferences::read(std::string_view path, std::string_view css)
    {
        if (auto valid = WriteValidation::xmlText(css, true); !valid)
            return std::unexpected(valid.error());

        CssReferences reader;
        reader.source_ = css;
        reader.directory_ = WriteValidation::fromUtf8(path).parent_path();
        if (auto valid = reader.scan(); !valid)
            return std::unexpected(valid.error());

        return std::move(reader.paths_);
    }
}
