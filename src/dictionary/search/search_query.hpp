//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "search_profile.hpp"

namespace MKD::detail::search
{
    class SearchQuery
    {
    public:
        SearchQuery(const SearchProfile& profile, std::string_view text) : profile_(profile), text_(text)
        {
        }

        [[nodiscard]] std::vector<std::string> keys(SearchScope scope, SearchFallback fallback) const;
        [[nodiscard]] std::vector<std::string> withoutStopWords(const std::vector<std::string>& keys) const;
        [[nodiscard]] std::vector<std::string> withoutHeadlineStopWords(const std::vector<std::string>& keys) const;
        [[nodiscard]] std::vector<std::string> variations(std::string_view key, SearchScope scope) const;

        [[nodiscard]] static std::string joined(const std::vector<std::string>& keys);

    private:
        const SearchProfile& profile_;
        std::string_view text_;
    };
}
