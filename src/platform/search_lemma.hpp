//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace MKD::detail
{
    [[nodiscard]] std::optional<std::string> searchLemma(std::string_view text, std::string_view language);
}
