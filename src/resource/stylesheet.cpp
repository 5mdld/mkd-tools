//
// kiwakiwaaにより 2026/04/05 に作成されました。
//

#include "MKD/resource/stylesheet.hpp"

#include <fstream>
#include <format>

namespace MKD
{
    Result<Stylesheet> Stylesheet::load(const fs::path& cssPath)
    {
        if (!fs::is_regular_file(cssPath))
            return std::unexpected(std::format("Stylesheet file not found: {}", cssPath.string()));

        std::ifstream file(cssPath, std::ios::in | std::ios::binary);
        if (!file)
            return std::unexpected(std::format("Failed to open stylesheet: {}", cssPath.string()));

        file.seekg(0, std::ios::end);
        const auto length = file.tellg();
        if (length < 0)
            return std::unexpected(std::format("Failed to read stylesheet length: {}", cssPath.string()));
        file.seekg(0, std::ios::beg);

        std::string data;
        data.resize(static_cast<size_t>(length));
        if (!file.read(data.data(), static_cast<std::streamsize>(length)))
            return std::unexpected(std::format("Failed to read stylesheet: {}", cssPath.string()));

        return Stylesheet(cssPath.filename().string(), std::move(data));
    }


    const std::string& Stylesheet::name() const noexcept
    {
        return name_;
    }


    const std::string& Stylesheet::data() const noexcept
    {
        return data_;
    }


    Stylesheet::Stylesheet(std::string name, std::string data)
        : name_(std::move(name)), data_(std::move(data))
    {
    }
}
