//
// kiwakiwaaにより 2026/01/29 に作成されました。
//

#include "resource_store_map.hpp"

namespace MKD
{
    void ResourceStoreMapHeader::swapEndianness() noexcept
    {
        version = std::byteswap(version);
        recordCount = std::byteswap(recordCount);
    }

    void ResourceStoreMapRecord::swapEndianness() noexcept
    {
        chunkGlobalOffset = std::byteswap(chunkGlobalOffset);
        itemOffset = std::byteswap(itemOffset);
    }
}