//
// kiwakiwaaにより 2026/04/05 に作成されました。
//

#pragma once

#include <cstdint>

namespace MKD
{
    /*
     * Prefix -> Index B: sorted by normalised key, forward comparison
     * Exact -> Index B: keys equal under the keystore comparator
     * Suffix -> Index C: sorted by reversed normalised key, backward comparison
     */
    enum class SearchMode : uint8_t
    {
        Prefix,
        Exact,
        Suffix
    };
}
