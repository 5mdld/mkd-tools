//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/inline_text.hpp"
#include <optional>
#include <array>

namespace MKD::detail
{
    struct NativeEntryReference
    {
        static std::optional<EntryId> parse(std::string_view text);
    };

    struct NativeLabelToken
    {
        std::string_view source;
        std::vector<std::string_view> fields;
        static Result<NativeLabelToken> read(std::string_view source);
        [[nodiscard]] InlineElement element() const;
    };

    struct NativeContent
    {
        static Result<void> labelText(std::string_view text, bool field = false);
        static Result<void> gaijiPath(std::string_view path);
        static std::string escapeXml(std::string_view text);
    };

    struct HeadlineEncoder
    {
        Result<std::string> operator()(const std::string& text) const;
        Result<std::string> operator()(const Ruby& ruby) const;
        Result<std::string> operator()(const Gaiji& gaiji) const;
        Result<std::string> operator()(const EntryLink& link) const;
        Result<std::string> operator()(const NativeMarkup& markup) const;
    };

    struct PlainTextEncoder
    {
        Result<std::string_view> operator()(const std::string& text) const;
        Result<std::string_view> operator()(const Ruby& ruby) const;
        Result<std::string_view> operator()(const Gaiji& gaiji) const;
        Result<std::string_view> operator()(const EntryLink& link) const;
        Result<std::string_view> operator()(const NativeMarkup& markup) const;
    };

    struct XhtmlEncoder
    {
        Result<std::string> operator()(const std::string& text) const;
        Result<std::string> operator()(const Ruby& ruby) const;
        Result<std::string> operator()(const Gaiji& gaiji) const;
        Result<std::string> operator()(const EntryLink& link) const;
        Result<std::string> operator()(const NativeMarkup& markup) const;
    };
}

namespace MKD::detail::native::label
{
    inline constexpr char ruby[] = "RB";
    inline constexpr char image[] = "IMG";
    inline constexpr char reference[] = "REF";
    inline constexpr char gaijiDirectory[] = "gaiji/";
    inline constexpr char gaijiStyle[] = "display: gaiji; height: 1em;";
    inline constexpr char imageStyle[] = "height: 1em;";
    inline constexpr auto commands = std::to_array<std::string_view>({
        "AR", "B", "BI", "BR", "C", "C1", "CD", "CID", "CL", "CR", "GSUB", "HI", "I", image,
        "KY", "L", "MATH", "N", "NS", "R", ruby, reference, "RR", "S", "SUB", "SUP", "T", "TI", "UD", "ZS"
    });
}
