//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "search_definitions.hpp"
#include "MKD/dictionary/product_writer.hpp"

namespace MKD::detail::search
{
    Result<ScopeDefinition> scopeDefinition(const SearchScope scope, const CreationProfile profile)
    {
        if (profile == CreationProfile::Kanwa)
        {
            for (const auto& names : native::search::kanwaScopes)
                if (names.scope == scope)
                    return names;

            return std::unexpected("Search scope is unavailable in the kanwa creation profile");
        }
        for (const auto& names : searchScopes)
            if (names.generic && names.scope == scope)
                return names;

        return std::unexpected("Search scope requires a specialised native search profile");
    }
}
