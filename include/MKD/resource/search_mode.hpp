//
// Created for the public search API.
//

#pragma once

#include <cstdint>

namespace MKD
{
    /*
     * Prefix -> Index B: sorted by normalised key, forward comparison
     * Exact -> Index B: prefix search range + exact key filter
     * Suffix -> Index C: sorted by reversed normalised key, backward comparison
     */
    enum class SearchMode : uint8_t
    {
        Prefix,
        Exact,
        Suffix
    };
}
