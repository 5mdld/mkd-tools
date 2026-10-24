//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/dictionary.hpp"

namespace MKD::detail
{
    struct ContentResources
    {
        static Result<RetainedSpan> read(const std::filesystem::path& directory,
                                         const NamedResourceStore* graphics,
                                         const std::variant<ResourceStore, NamedResourceStore>* audio,
                                         std::string_view path);
        static Result<void> validateGaiji(std::string_view path, std::span<const uint8_t> bytes);
    };
}
