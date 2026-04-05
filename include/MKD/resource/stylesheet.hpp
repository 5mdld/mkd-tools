//
// kiwakiwaaにより 2026/04/05 に作成されました。
//

#pragma once

#include "MKD/result.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace MKD
{
    class Stylesheet
    {
    public:
        /**
         * Loads a CSS file from disk.
         * @param cssPath Path to a .css file
         * @return Stylesheet instance with file name and CSS text data
         */
        static Result<Stylesheet> load(const fs::path& cssPath);

        /**
         * Returns stylesheet file name.
         */
        [[nodiscard]] const std::string& name() const noexcept;

        /**
         * Returns raw stylesheet CSS text.
         */
        [[nodiscard]] const std::string& data() const noexcept;

    private:
        explicit Stylesheet(std::string name, std::string data);

        std::string name_;
        std::string data_;
    };
}

