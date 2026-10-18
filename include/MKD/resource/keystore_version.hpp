//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include <cstdint>

namespace MKD
{
    enum class KeystoreVersion : uint32_t
    {
        Version1 = 0x10000,
        Version2 = 0x20000
    };
}
