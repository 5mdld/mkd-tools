//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "MKDCLI/source.hpp"
#include "MKD/platform/directory_dictionary_source.hpp"

#ifdef __APPLE__
#include "MKD/platform/macos/macos_dictionary_source.hpp"
#endif

#include <iostream>

namespace MKDCLI
{
    MKD::Result<std::unique_ptr<MKD::DictionarySource>> createSource(const GlobalOptions& options)
    {
        if (!options.directory.empty())
            return std::make_unique<MKD::DirectoryDictionarySource>(options.directory);

#ifdef __APPLE__
        auto source = std::make_unique<MKD::MacOSDictionarySource>();
        if (!source->checkAccess())
        {
            std::cerr << "Requesting access to macOS dictionaries folder...\n";
            if (!source->requestAccess(true))
                return std::unexpected("Permission denied. Use --dir <path> to specify a directory manually.");
            std::cerr << "Access granted.\n";
        }
        return source;
#else
        return std::unexpected("No dictionary source specified.\n"
            "Use --dir <path> to point to a dictionaries directory.");
#endif
    }
}
