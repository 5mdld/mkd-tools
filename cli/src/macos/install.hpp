//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include <filesystem>

namespace MKDCLI::macOS
{
    MKD::Result<void> requireDictionariesClosed();
    MKD::Result<std::filesystem::path> installPackage(const std::filesystem::path& package,
                                                      const std::filesystem::path& dataDirectory, bool replace);
}
