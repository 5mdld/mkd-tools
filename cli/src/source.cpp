//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "MKDCLI/source.hpp"
#include "MKD/platform/directory_dictionary_source.hpp"

#ifdef __APPLE__
#include "MKD/platform/macos/paths.hpp"
#endif

namespace MKDCLI
{
    MKD::Result<std::unique_ptr<MKD::DictionarySource>> createSource(const GlobalOptions& options)
    {
        if (!options.directory.empty())
            return std::make_unique<MKD::DirectoryDictionarySource>(options.directory);

#ifdef __APPLE__
        auto directory = MKD::macOS::monokakidoDataDirectory();
        if (!directory)
            return std::unexpected(directory.error() + ". Use --dir <path> to specify a directory manually.");
        return std::make_unique<MKD::DirectoryDictionarySource>(MKD::macOS::dictionariesDirectory(*directory));
#else
        return std::unexpected("No dictionary source specified.\n"
            "Use --dir <path> to point to a dictionaries directory.");
#endif
    }
}
