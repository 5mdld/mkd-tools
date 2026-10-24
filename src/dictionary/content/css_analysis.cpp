//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "MKD/dictionary/content_analysis.hpp"
#include "css_lexer.hpp"
#include "analysis_support.hpp"
#include "io/write_validation.hpp"
#include <algorithm>
#include <cctype>

namespace MKD::content
{
    using detail::css::CssLexer;
    using detail::css::CssTokenKind;
    using detail::WriteValidation;

    AnalysisResult<Analysis> analyzeCss(std::string_view css, CssInput input)
    {
        if (auto valid = WriteValidation::xmlText(css, true); !valid)
            return std::unexpected(SourceError{valid.error(), {}});

        CssLexer lexer(css);
        Analysis result;
        bool import = false;
        size_t braces = 0, parentheses = 0, brackets = 0;
        bool statementStart = true;
        for (;;)
        {
            auto token = lexer.next();
            if (!token)
                return std::unexpected(token.error());
            if (token->kind == CssTokenKind::End)
                return result;

            // Packaging checks every local mention, including syntax ignored by the native renderer.
            if (token->kind == CssTokenKind::Url || (import && token->kind == CssTokenKind::String))
                if (auto reference = detail::resourceReference(token->value, ResourceSyntax::CssUrl, token->source))
                    result.references.push_back(std::move(*reference));

            import = token->kind == CssTokenKind::AtKeyword &&
                std::ranges::equal(token->value, std::string_view("@import"), {},
                                   [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (input == CssInput::Stylesheet && statementStart && !braces && !parentheses && !brackets &&
                token->kind == CssTokenKind::AtKeyword &&
                css.substr(token->source.offset, token->source.length) != "@color")
                result.diagnostics.push_back(
                    {
                        DiagnosticKind::IgnoredAtRule, token->source,
                        "Native article styles ignore " + std::string(
                            css.substr(token->source.offset, token->source.length))
                    });

            if (token->kind == CssTokenKind::Delimiter)
            {
                if (token->value == "(") ++parentheses;
                else if (token->value == ")" && parentheses) --parentheses;
                else if (token->value == "[") ++brackets;
                else if (token->value == "]" && brackets) --brackets;
                else if (!parentheses && !brackets)
                {
                    if (token->value == "{") ++braces;
                    else if (token->value == "}" && braces) --braces;
                }
            }

            statementStart = !braces && !parentheses && !brackets &&
                token->kind == CssTokenKind::Delimiter &&
                (token->value == ";" || token->value == "}");
        }
    }
}
