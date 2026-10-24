//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#pragma once
#include "MKD/resource/entry_id.hpp"
#include <span>
#include <string>
#include <vector>

namespace MKD::detail::search
{
    struct KeyComponents
    {
        std::vector<std::string> values;
        bool filterHeadlines = false;
    };

    constexpr EntryId companionTarget(const EntryId target, const uint16_t offset)
    {
        return {target.pageId, static_cast<uint16_t>(target.itemId + offset)};
    }

    KeyComponents keyComponents(std::span<const std::string> keys);
}
