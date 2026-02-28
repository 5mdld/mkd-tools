//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/output/export_result.hpp"

#include <expected>
#include <span>


namespace MKD
{
    class BaseExporter
    {
    protected:
        static Result<void> writeData(std::span<const uint8_t> data, const fs::path& path);

        static bool shouldSkipExisting(const fs::path& path, bool overwrite);
    };
}
