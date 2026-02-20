//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/cli.hpp"
#include "MKDCLI/commands.hpp"
#include "MKDCLI/format.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    auto app = MKDCLI::CLIApp::parse(argc, argv);
    if (!app)
    {
        std::cerr << app.error() << "\n";
        return 1;
    }

    const auto& opts = app->options();

    if (opts.noColour)
        MKDCLI::Colour::setEnabled(false);
    else
        MKDCLI::Colour::autoDetect();

    if (opts.command == MKDCLI::Command::Help)
    {
        MKDCLI::CLIApp::printHelp(argv[0]);
        return 0;
    }
    if (opts.command == MKDCLI::Command::Version)
    {
        MKDCLI::CLIApp::printVersion();
        return 0;
    }

    auto source = app->createSource();
    if (!source)
    {
        std::cerr << MKDCLI::Colour::red("Error: ") << source.error() << "\n";
        return 1;
    }

    switch (opts.command)
    {
        case MKDCLI::Command::List:
            return MKDCLI::runList(**source, opts);

        case MKDCLI::Command::Info:
            return MKDCLI::runInfo(**source, opts);

        case MKDCLI::Command::Export:
            return MKDCLI::runExport(**source, opts, app->buildExportOptions());

        default:
            MKDCLI::CLIApp::printHelp(argv[0]);
            return 1;
    }
}
