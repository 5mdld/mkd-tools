//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary.hpp"

namespace MKD::detail
{
    struct ContentInfo
    {
        static Result<void> load(const std::filesystem::path& directory, DictionaryContent& content,
                                 DictionarySearchConfiguration& search);

    private:
        static Result<void> loadContent(const std::filesystem::path& directory, DictionaryContent& content, DictionarySearchConfiguration& search);
        static Result<void> loadModernRange(const std::filesystem::path& directory,
                                            DictionarySearchConfiguration& search);
    };
}
