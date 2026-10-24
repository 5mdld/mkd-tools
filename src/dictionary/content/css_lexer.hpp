//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "MKD/dictionary/content_analysis.hpp"
#include "MKD/result.hpp"
#include <string_view>

namespace MKD::detail::css
{
    enum class CssTokenKind
    {
        End,
        Identifier,
        AtKeyword,
        String,
        Url,
        Delimiter
    };

    struct CssToken
    {
        CssTokenKind kind = CssTokenKind::End;
        content::SourceRange source;
        std::string value;
    };

    class CssLexer
    {
    public:
        explicit CssLexer(std::string_view source) : source_(source)
        {
        }

        content::AnalysisResult<CssToken> next();

    private:
        Result<void> whitespace();
        size_t errorOffset_ = 0;
        Result<void> escape(std::string& value);
        Result<std::string> string(char quote);
        Result<std::string> url();
        std::string_view source_;
        size_t position_ = 0;
    };
}
