//
// Caoimheにより 2026/01/16 に作成されました。
//

#pragma once

#include <cstdint>


namespace monokakido::resource
{
    enum class CompressionFormat : uint16_t
    {
        Uncompressed = 0,
        Zlib = 1
    };
}