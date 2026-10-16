//
// kiwakiwaaにより 2026/10/16 に作成されました。
//

#include "MKD/platform/macos/paths.hpp"

#include <Foundation/Foundation.hpp>

namespace MKD::macOS
{
    Result<std::filesystem::path> monokakidoDataDirectory()
    {
        NS::ScopedAutoreleasePool pool;
        const auto* group = NS::String::stringWithUTF8String("group.jp.monokakido.Dictionaries");
        const auto* url = NS::FileManager::defaultManager()->containerURLForSecurityApplicationGroupIdentifier(group);
        const auto* path = url ? url->path() : nullptr;
        const auto* text = path ? path->utf8String() : nullptr;
        return text && *text
                   ? Result<std::filesystem::path>(std::filesystem::path(text))
                   : std::unexpected("Unable to locate the Monokakido data directory");
    }

    std::filesystem::path dictionariesDirectory(const std::filesystem::path& dataDirectory)
    {
        return dataDirectory / "Library/Application Support/com.dictionarystore/dictionaries";
    }

    std::filesystem::path purchasesPath(const std::filesystem::path& dataDirectory)
    {
        return dataDirectory / "Library/Application Support/com.dictionarystore/receipt/purchases";
    }
}
