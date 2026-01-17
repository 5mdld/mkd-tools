//
// Caoimheにより 2026/01/17 に作成されました。
//

#pragma once

#include "monokakido/resource/nrsc/nrsc.hpp"
#include "export_options.hpp"

#include <expected>
#include <ranges>
#include <utility>


namespace monokakido::resource
{

    class ResourceExporter
    {
    public:

        explicit ResourceExporter(ExportOptions&& options) : options_(std::move(options)){}


        [[nodiscard]] std::expected<void, std::string> exportAll(const Nrsc& nrsc) const
        {
            fs::create_directories(options_.outputDirectory);

            for (auto [id, data] : nrsc)
            {
                const auto outputPath = options_.outputDirectory / id;

                std::ofstream file(outputPath, std::ios::out | std::ios::binary);
                if (!file)
                {
                    return std::unexpected(std::format("Failed to open file for writing: {}", outputPath.string()));
                }

                file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));

                if (!file)
                {
                    return std::unexpected(std::format("Failed to write data to: {}", outputPath.string()));
                }
            }

            return {};
        }


    private:
        ExportOptions options_;

    };

}