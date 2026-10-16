//
// kiwakiwaaにより 2026/02/07 に作成されました。
//

#pragma once

#include <compare>
#include <cstdint>
#include <functional>

namespace MKD
{
    struct EntryId
    {
        uint32_t pageId;
        uint16_t itemId;
        auto operator<=>(const EntryId&) const = default;
    };
}

template <>
struct std::hash<MKD::EntryId>
{
    size_t operator()(const MKD::EntryId& id) const noexcept
    {
        const auto value = (static_cast<uint64_t>(id.pageId) << 16) | id.itemId;
        return std::hash<uint64_t>{}(value);
    }
};
