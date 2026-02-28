//
// kiwakiwaaにより 2026/02/19 に作成されました。
//

#pragma once

#include "MKD/platform/dictionary_source.hpp"

#include <string_view>

namespace MKD
{
    class DirectoryDictionarySource final : public DictionarySource
    {
    public:
        explicit DirectoryDictionarySource(fs::path root);

        [[nodiscard]] Result<std::vector<DictionaryInfo>> findAllAvailable() const override;

        [[nodiscard]] Result<DictionaryInfo> findById(std::string_view dictId) const override;

    private:

        static bool looksLikeDictionary(const fs::path& path);

        std::filesystem::path root_;
    };
}