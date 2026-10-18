//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "MKD/resource/entry_id.hpp"
#include <format>

namespace MKD
{
    std::string EntryId::toString() const
    {
        return std::format("{:05}-{:04X}", pageId, itemId);
    }
}
