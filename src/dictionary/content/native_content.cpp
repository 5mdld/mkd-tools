//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "dictionary/content/native_content.hpp"
#include "io/write_validation.hpp"

#include <charconv>
#include <format>

namespace MKD::detail
{
    std::optional<EntryId> NativeEntryReference::parse(std::string_view text)
    {
        const auto dash = text.find('-');
        if (dash == std::string_view::npos)
            return std::nullopt;

        EntryId id{};
        const auto page = std::from_chars(text.data(), text.data() + dash, id.pageId);
        const auto item = std::from_chars(text.data() + dash + 1, text.data() + text.size(), id.itemId, 16);
        if (page.ec != std::errc{} || page.ptr != text.data() + dash || item.ec != std::errc{} ||
            item.ptr != text.data() + text.size())
            return std::nullopt;

        return id;
    }

    Result<NativeLabelToken> NativeLabelToken::read(std::string_view source)
    {
        if (source.size() < 2 || source.front() != '{' || source.back() != '}')
            return std::unexpected("Expected a complete native label token");

        NativeLabelToken token{source, {}};
        size_t start = 1;
        size_t depth = 0;
        for (size_t i = 1; i < source.size() - 1; ++i)
        {
            const auto c = source[i];
            if (c == '{')
            {
                if (++depth > 64) return std::unexpected("Native label nesting exceeds 64 levels");
            }
            else if (c == '}')
            {
                if (!depth) return std::unexpected("Unbalanced native label token");
                --depth;
            }
            else if (c == ':' && !depth)
            {
                token.fields.push_back(source.substr(start, i - start));
                start = i + 1;
            }
        }

        if (depth)
            return std::unexpected("Unbalanced native label token");

        token.fields.push_back(source.substr(start, source.size() - 1 - start));
        return token;
    }

    Result<NativeLabelToken> NativeLabelToken::readLeading(std::string_view source)
    {
        if (source.empty() || source.front() != '{')
            return std::unexpected("Expected a native label token");

        size_t end = 0, depth = 0;
        do
        {
            if (source[end] == '{') ++depth;
            else if (source[end] == '}') --depth;
            ++end;
        }
        while (end < source.size() && depth);
        if (depth)
            return std::unexpected("Unterminated native label token");
        return read(source.substr(0, end));
    }

    InlineElement NativeLabelToken::element() const
    {
        // Nested formatter expressions retain their original spelling until we can model them faithfully.
        if (source.substr(1, source.size() - 2).contains('{'))
            return NativeMarkup{std::string(source)};

        if (fields.size() == 3 && fields[0] == native::label::ruby && !fields[1].empty() && !fields[2].empty())
            return Ruby{std::string(fields[1]), std::string(fields[2])};

        if (fields.size() == 3 && fields[0] == native::label::reference)
            if (const auto id = NativeEntryReference::parse(fields[1]))
                return EntryLink{*id, std::string(fields[2])};

        if (fields.size() == 3 && fields[0] == native::label::image && !fields[2].empty() &&
            NativeContent::gaijiPath(fields[1]))
            return Gaiji{{std::string(fields[1])}, std::string(fields[2]), GaijiRendering::Original};

        // Explicit paths cannot collide with formatter names such as RB, IMG or CID.
        if (fields.size() == 2 && fields[0].contains('/') && !fields[1].empty() && NativeContent::gaijiPath(fields[0]))
            return Gaiji{{std::string(fields[0])}, std::string(fields[1])};

        return NativeMarkup{std::string(source)};
    }

    Result<void> NativeContent::labelText(std::string_view text, bool field)
    {
        if (auto valid = WriteValidation::text(text, true); !valid)
            return valid;

        if (text.find_first_of(field ? "{}:" : "{") != std::string_view::npos)
            return std::unexpected("Text cannot be represented literally in native label markup");

        return {};
    }

    Result<void> NativeContent::gaijiPath(std::string_view path)
    {
        if (auto valid = WriteValidation::relativePath(path); !valid)
            return valid;

        if (auto valid = labelText(path, true); !valid)
            return valid;

        if (auto valid = WriteValidation::xmlText(path); !valid)
            return valid;

        if (path.find_first_of("?#%\n\r\t") != std::string_view::npos)
            return std::unexpected("Gaiji paths must not contain URL delimiters or escapes");

        if (!path.contains('/') || (!path.ends_with(".svg") && !path.ends_with(".png")))
            return std::unexpected("Gaiji must use an explicit relative PNG or SVG path");

        return {};
    }

    std::string NativeContent::escapeXml(std::string_view text)
    {
        std::string result;
        for (const auto c : text)
            switch (c)
            {
            case '&': result += "&amp;";
                break;
            case '<': result += "&lt;";
                break;
            case '>': result += "&gt;";
                break;
            case '"': result += "&quot;";
                break;
            case '\'': result += "&apos;";
                break;
            default: result += c;
            }
        return result;
    }

    Result<std::string> HeadlineEncoder::operator()(const std::string& text) const
    {
        if (auto valid = NativeContent::labelText(text); !valid)
            return std::unexpected(valid.error());
        return text;
    }

    Result<std::string> HeadlineEncoder::operator()(const Ruby& ruby) const
    {
        for (const auto& field : {ruby.base, ruby.reading})
            if (auto valid = NativeContent::labelText(field, true); !valid)
                return std::unexpected(valid.error());

        if (ruby.base.empty() || ruby.reading.empty())
            return std::unexpected("Ruby needs base text and a reading");

        return std::format("{{{}:{}:{}}}", native::label::ruby, ruby.base, ruby.reading);
    }

    Result<std::string> HeadlineEncoder::operator()(const Gaiji& gaiji) const
    {
        if (auto valid = NativeContent::gaijiPath(gaiji.resource.path); !valid)
            return std::unexpected(valid.error());

        if (auto valid = NativeContent::labelText(gaiji.replacement, true); !valid)
            return std::unexpected(valid.error());

        if (gaiji.replacement.empty())
            return std::unexpected("Gaiji needs replacement text");

        switch (gaiji.rendering)
        {
        case GaijiRendering::Text:
            return std::format("{{{}:{}}}", gaiji.resource.path, gaiji.replacement);
        case GaijiRendering::Original:
            return std::format("{{{}:{}:{}}}", native::label::image, gaiji.resource.path,
                               gaiji.replacement);
        }
        return std::unexpected("Invalid gaiji rendering mode");
    }

    Result<std::string> HeadlineEncoder::operator()(const EntryLink& link) const
    {
        if (auto valid = WriteValidation::entryId(link.target); !valid)
            return std::unexpected(valid.error());

        if (auto valid = NativeContent::labelText(link.text, true); !valid)
            return std::unexpected(valid.error());

        return std::format("{{{}:{}:{}}}", native::label::reference, link.target.toString(),
                           link.text);
    }

    Result<std::string> HeadlineEncoder::operator()(const NativeMarkup& markup) const
    {
        if (auto valid = WriteValidation::text(markup.text); !valid)
            return std::unexpected(valid.error());

        if (auto token = NativeLabelToken::read(markup.text); !token)
            return std::unexpected(token.error());

        return markup.text;
    }

    Result<std::string_view> PlainTextEncoder::operator()(const std::string& text) const
    {
        return std::string_view(text);
    }

    Result<std::string_view> PlainTextEncoder::operator()(const Ruby& ruby) const
    {
        return std::string_view(ruby.base);
    }

    Result<std::string_view> PlainTextEncoder::operator()(const Gaiji& gaiji) const
    {
        return std::string_view(gaiji.replacement);
    }

    Result<std::string_view> PlainTextEncoder::operator()(const EntryLink& link) const
    {
        return std::string_view(link.text);
    }

    Result<std::string_view> PlainTextEncoder::operator()(const NativeMarkup&) const
    {
        return std::unexpected("Unknown native markup has no plain-text representation");
    }

    Result<std::string> XhtmlEncoder::operator()(const std::string& text) const
    {
        if (auto valid = WriteValidation::xmlText(text, true); !valid)
            return std::unexpected(valid.error());
        return NativeContent::escapeXml(text);
    }

    Result<std::string> XhtmlEncoder::operator()(const Ruby& ruby) const
    {
        for (const auto& text : {ruby.base, ruby.reading})
            if (auto valid = WriteValidation::xmlText(text); !valid)
                return std::unexpected(valid.error());

        return std::format("<ruby><rb>{}</rb><rt>{}</rt></ruby>", NativeContent::escapeXml(ruby.base),
                           NativeContent::escapeXml(ruby.reading));
    }

    Result<std::string> XhtmlEncoder::operator()(const Gaiji& gaiji) const
    {
        if (auto valid = NativeContent::gaijiPath(gaiji.resource.path); !valid)
            return std::unexpected(valid.error());

        if (auto valid = WriteValidation::xmlText(gaiji.replacement); !valid)
            return std::unexpected(valid.error());

        if (gaiji.rendering != GaijiRendering::Text && gaiji.rendering != GaijiRendering::Original)
            return std::unexpected("Invalid gaiji rendering mode");

        return std::format("<img src=\"{}\" alt=\"{}\" style=\"{}\"/>", NativeContent::escapeXml(gaiji.resource.path),
                           NativeContent::escapeXml(gaiji.replacement), gaiji.rendering == GaijiRendering::Text
                                                                            ? native::label::gaijiStyle
                                                                            : native::label::imageStyle);
    }

    Result<std::string> XhtmlEncoder::operator()(const EntryLink& link) const
    {
        if (auto valid = WriteValidation::entryId(link.target); !valid)
            return std::unexpected(valid.error());

        if (auto valid = WriteValidation::xmlText(link.text, true); !valid)
            return std::unexpected(valid.error());

        return std::format("<a href=\"{}\">{}</a>", link.target.toString(),
                           NativeContent::escapeXml(link.text));
    }

    Result<std::string> XhtmlEncoder::operator()(const NativeMarkup&) const
    {
        return std::unexpected("Unknown native markup has no XHTML representation");
    }
}
