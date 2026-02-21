//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#pragma once

#include "MKD/resource/resource_type.hpp"
#include "export_result.hpp"

#include <cstdint>

namespace MKD
{
    struct PhaseBeginEvent
    {
        ResourceType type;
        size_t totalItems = 0;
    };

    struct ProgressEvent
    {
        ResourceType type;
        size_t completedItems = 0;
        size_t totalItems = 0;
        uint64_t bytesWritten = 0;
    };

    struct PhaseEndEvent
    {
        ResourceType type;
        ExportResult result;
    };
}