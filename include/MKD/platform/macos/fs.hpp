//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#pragma once

#include <filesystem>
#include <string>

namespace MKD::macOS
{
    [[nodiscard]] std::filesystem::path getContainerPathByGroupIdentifier(const std::string& groupIdentifier);
}
