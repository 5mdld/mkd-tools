//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"

#include <span>
#include <string>
#include <variant>
#include <vector>

namespace MKD
{
    /** A content-relative resource reference. Registration writes its bytes immediately. */
    struct GaijiResource
    {
        std::string path;
        auto operator<=>(const GaijiResource&) const = default;
    };

    enum class GaijiRendering { Text, Original };

    struct Ruby
    {
        std::string base;
        std::string reading;
        auto operator<=>(const Ruby&) const = default;
    };

    struct Gaiji
    {
        GaijiResource resource;
        std::string replacement;
        GaijiRendering rendering = GaijiRendering::Text;
        auto operator<=>(const Gaiji&) const = default;
    };

    struct EntryLink
    {
        EntryId target;
        std::string text;
        auto operator<=>(const EntryLink&) const = default;
    };

    /** Preserves native tokens which have no portable content representation. */
    struct NativeMarkup
    {
        std::string text;
        auto operator<=>(const NativeMarkup&) const = default;
    };

    using InlineElement = std::variant<std::string, Ruby, Gaiji, EntryLink, NativeMarkup>;

    /** Owns its text. Encoders reject content which their target syntax cannot represent. */
    class InlineText
    {
    public:
        void append(InlineElement element);
        [[nodiscard]] std::span<const InlineElement> elements() const noexcept;
        [[nodiscard]] static Result<InlineText> fromHeadline(std::u16string_view text);
        [[nodiscard]] Result<std::u16string> toHeadline() const;
        [[nodiscard]] Result<std::string> toXhtml() const;
        /** Uses ruby base text and gaiji replacement text. Unknown native tokens are errors. */
        [[nodiscard]] Result<std::string> plainText() const;

    private:
        std::vector<InlineElement> elements_;
    };
}
