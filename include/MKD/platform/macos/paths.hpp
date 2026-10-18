//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <filesystem>

namespace MKD::macOS
{
    /** Returns the expected location without checking its existence or access permissions. */
    [[nodiscard]] Result<std::filesystem::path> monokakidoDataDirectory();
    [[nodiscard]] std::filesystem::path dictionariesDirectory(const std::filesystem::path& dataDirectory);
}
