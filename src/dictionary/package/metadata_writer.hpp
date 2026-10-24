//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once
#include "MKD/dictionary/metadata.hpp"

namespace MKD::detail::metadata_writer
{
    Result<void> writeProduct(const std::filesystem::path& root, std::string_view name,
                              const DictionaryMetadata& metadata);
    Result<void> writeContent(const std::filesystem::path& root, const DictionaryContent& metadata, size_t headwords,
                              size_t audio);
}
