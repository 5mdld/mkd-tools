//
// kiwakiwaaにより 2026/02/20 に作成されました。
//

#include "MKDCLI/cli.hpp"
#include "MKDCLI/commands.hpp"
#include "MKDCLI/source.hpp"

#include <exception>
#include <iostream>
#include <type_traits>

int main(int argc, char* argv[])
{
    try
    {
        MKDCLI::CLIApp app;
        auto invocation = app.parse(argc, argv, std::cout, std::cerr);
        if (!invocation)
            return invocation.error();

        auto source = MKDCLI::createSource(invocation->global);
        if (!source)
        {
            std::cerr << "Error: " << source.error() << "\n";
            return 1;
        }

        return std::visit([&](const auto& command)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype(command)>, MKDCLI::ListCommand>)
                return MKDCLI::runCommand(**source, command);
            else
            {
                auto info = (*source)->findById(command.productId);
                if (!info)
                {
                    std::cerr << "Error: " << info.error() << "\n";
                    return 1;
                }
                auto product = MKD::DictionaryProduct::openAtPath(info->path);
                if (!product)
                {
                    std::cerr << "Error: " << product.error() << "\n";
                    return 1;
                }
                return MKDCLI::runCommand(*product, command);
            }
        }, invocation->command);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
