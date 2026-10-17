//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "search_lemma.hpp"

namespace MKD::detail
{
    std::optional<std::string> searchLemma(std::string_view, std::string_view)
    {
        // Linguistic tags are only available on macOS.
        return std::nullopt;
    }
}
