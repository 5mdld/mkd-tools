//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "common.hpp"

#include <expected>
#include <span>
#include <string>


namespace MKD
{
    class BaseExporter
    {
    protected:
        static std::expected<void, std::string> writeData(std::span<const uint8_t> data, const fs::path& path);

        static bool shouldSkipExisting(const fs::path& path, bool overwrite);
    };
}
