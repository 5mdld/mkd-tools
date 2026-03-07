//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#pragma once

#include "MKD/resource/resource_type.hpp"
#include "MKD/output/export_result.hpp"

#include <cstdint>
#include <functional>
#include <variant>

namespace MKD
{
    struct ExportBeginEvent
    {
        size_t totalItems = 0; // sum of heavy-phase items only
    };

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

    using ExportEvent = std::variant<ExportBeginEvent, PhaseBeginEvent, ProgressEvent, PhaseEndEvent>;
    using ExportCallback = std::function<void(const ExportEvent&)>;

    constexpr bool isHeavyResource(const ResourceType type)
    {
        return type == ResourceType::Audio
            || type == ResourceType::Contents
            || type == ResourceType::Graphics;
    }
}