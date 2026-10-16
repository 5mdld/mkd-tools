//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"

#include <CLI/CLI.hpp>

#include <expected>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <variant>

namespace MKDCLI
{
    struct GlobalOptions
    {
        std::filesystem::path directory;
    };

    struct ListCommand
    {
    };

    struct InfoCommand
    {
        std::string productId;
    };

    struct ExportCommand
    {
        std::string productId;
        MKD::ExportOptions options;
    };

    struct Invocation
    {
        GlobalOptions global;
        std::variant<ListCommand, InfoCommand, ExportCommand> command;
    };

    class CLIApp
    {
    public:
        CLIApp();

        /**
         * Parse without opening products or requesting access. Help, version and
         * invalid arguments return an exit status after writing to the supplied streams.
         */
        std::expected<Invocation, int> parse(int argc, char* argv[],
                                             std::ostream& out, std::ostream& err);

    private:
        void addInfoCommand();
        void addExportCommand();
        void addResourceOptions(CLI::App& command);

        CLI::App app_{"mkd-tools"};
        Invocation invocation_;
        InfoCommand info_;
        ExportCommand export_;
    };
}
