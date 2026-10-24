//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/resource_store.hpp"
#include <filesystem>
#include <variant>

namespace MKD::detail
{
    Result<RetainedSpan> readDictionaryResource(const std::filesystem::path& directory,
                                                const NamedResourceStore* graphics,
                                                const std::variant<ResourceStore, NamedResourceStore>* audio,
                                                std::string_view path);
}
