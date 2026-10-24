//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "analysis_support.hpp"
#include <algorithm>

namespace MKD::detail
{
    SourceMap::SourceMap(std::string_view input) : size_(input.size())
    {
        for (size_t i = 0; i < input.size(); ++i)
            if (input[i] == '\r' || input[i] == '\n')
            {
                if (input[i] == '\r' && i + 1 < input.size() && input[i + 1] == '\n')
                    ++i;
                lines_.push_back(i + 1);
            }
    }

    std::string SourceMap::describe(std::string_view origin,
                                    std::optional<content::SourceRange> range) const
    {
        std::string result(origin);
        if (!range)
            return result;

        if (!result.empty())
            result += " at ";

        result += "byte " + std::to_string(range->offset);
        if (range->offset <= size_)
        {
            const auto line = std::ranges::upper_bound(lines_, range->offset) - lines_.begin() - 1;
            result += " (line " + std::to_string(line + 1) + ", byte column " +
                std::to_string(range->offset - lines_[line] + 1) + ")";
        }
        return result;
    }
}


namespace MKD::content
{
    std::string SourceError::describe(std::string_view origin, std::string_view input) const
    {
        const auto location = detail::SourceMap(input).describe(origin, source);
        return location.empty() ? message : location + ": " + message;
    }
}
