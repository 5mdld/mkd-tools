//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once
#include "MKD/dictionary/metadata.hpp"

namespace MKD
{
    enum class CreationProfile;
}

namespace MKD::detail::metadata_preparation
{
    Result<DictionaryMetadata> prepareProduct(DictionaryMetadata metadata, const std::filesystem::path& destination,
                                              CreationProfile profile);
    Result<DictionaryContent> prepareContent(DictionaryContent content);
    Result<void> validatePresentation(const DictionaryPresentation& presentation);
}
