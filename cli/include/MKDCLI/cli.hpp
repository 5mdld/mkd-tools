//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "MKD/platform/dictionary_source.hpp"
#include "MKD/resource/resource_type.hpp"
#include "MKD/output/export_options.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace MKDCLI
{
    std::optional<MKD::ResourceType> parseResourceType(std::string_view str) noexcept;

    enum class Command
    {
        List,
        Info,
        Export,
        Help,
        Version,
    };

    struct CLIOptions
    {
        Command command = Command::Help;
        std::optional<fs::path> dirOverride;
        bool noColour = false;
        std::string dictId;
        fs::path outputDir;
        bool overwrite = false;
        bool prettyPrintXml = false;
        MKD::KeystoreExportMode keystoreMode = MKD::KeystoreExportMode::Inverse;
        std::vector<MKD::ResourceType> onlyResources;
    };


    class CLIApp
    {
    public:
        /**
         * Parse command-line arguments.  Returns an error string on invalid input
         */
        static std::expected<CLIApp, std::string> parse(int argc, char* argv[]);

        /**
         * Create DictionarySource from the parsed options
         * On macOS this handles the permission prompt for access to security scoped dictionary folder
         * On other platforms, --dir is required
         */
        [[nodiscard]] std::expected<std::unique_ptr<MKD::DictionarySource>, std::string> createSource() const;

        /**
         * Construct MKD::ExportOptions from the parsed CLI options
         */
        [[nodiscard]] MKD::ExportOptions buildExportOptions() const;


        /**
         * Get the parsed CLI options
         */
        [[nodiscard]] const CLIOptions& options() const noexcept;

        /**
         * Prints usage help, listing commands with examples
         *
         * @param program cli executable file name
         */
        static void printHelp(std::string_view program);

        /**
         * Prints the current version of the CLI
         */
        static void printVersion();

    private:
        // Private constructor - use parse()
        explicit CLIApp(CLIOptions options, std::string program);

        CLIOptions options_;
        std::string program_;
    };
}
