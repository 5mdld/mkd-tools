//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/output/base_exporter.hpp"

#include <format>
#include <fstream>

namespace monokakido
{
    std::expected<void, std::string> BaseExporter::writeData(const std::span<const uint8_t> data, const fs::path& path)
    {
        // Ensure parent directory exists
        if (const auto parent = path.parent_path(); !parent.empty())
        {
            std::error_code ec;
            fs::create_directories(parent, ec);
            if (ec)
                return std::unexpected(
                    std::format("Failed to create directory {}: {}",
                                parent.string(), ec.message()));
        }

        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (!file)
            return std::unexpected(std::format("Failed to open file for writing: {}", path.string()));

        file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

        if (!file)
            return std::unexpected(std::format("Failed to write data to: {}", path.string()));

        return {};
    }


    bool BaseExporter::shouldSkipExisting(const fs::path& path, const bool overwrite)
    {
        return !overwrite && fs::exists(path);
    }
}