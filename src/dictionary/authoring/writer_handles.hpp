//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once

#include "dictionary_build.hpp"

namespace MKD
{
    struct DictionaryWriter::Impl
    {
        std::shared_ptr<detail::DictionaryBuild> dictionary;

        explicit Impl(std::shared_ptr<detail::DictionaryBuild> value) : dictionary(std::move(value))
        {
        }
    };

    struct EntryWriter::Impl
    {
        std::shared_ptr<detail::DictionaryBuild> dictionary;
        uint32_t pageId;
        SourceContext context;
    };
}
