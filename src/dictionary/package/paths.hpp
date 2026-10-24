//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once
#include "MKD/result.hpp"
#include <filesystem>

namespace MKD::detail
{
    Result<std::filesystem::path> resolveProductPath(const std::filesystem::path& contents, std::string_view path);
}
