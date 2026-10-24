//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "key_analysis.hpp"
#include "unicode/unicode.hpp"
#include <algorithm>

namespace MKD::detail::search
{
    KeyComponents keyComponents(std::span<const std::string> keys)
    {
        KeyComponents result;
        for (const auto& key : keys)
        {
            const auto clusters = unicode::graphemeClusters(key);
            for (size_t i = 0; i < clusters.size(); ++i)
            {
                auto value = unicode::normalizeDictionaryKey(clusters[i]);
                if (value.empty())
                    continue;

                result.filterHeadlines = result.filterHeadlines || i != 0;
                if (std::ranges::find(result.values, value) == result.values.end())
                    result.values.push_back(std::move(value));
            }
        }
        return result;
    }
}
