//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#pragma once

#include "event.hpp"
#include "MKD/resource/resource_type.hpp"

#include <filesystem>
#include <functional>
#include <variant>

namespace fs = std::filesystem;

namespace MKD
{
    enum class KeystoreExportMode : uint8_t
    {
        Forward = 1 << 0, // key → page references
        Inverse = 1 << 1, // page reference → keys
        Both = Forward | Inverse,
    };

    using ExportEvent = std::variant<PhaseBeginEvent, ProgressEvent, PhaseEndEvent>;
    using ExportCallback = std::function<void(const ExportEvent&)>;

    struct ExportOptions
    {
        fs::path outputDirectory;
        bool overwriteExisting = false;
        bool createSubdirectories = true;
        bool prettyPrintXml = false;
        KeystoreExportMode keystoreExportMode = KeystoreExportMode::Inverse;
        std::vector<ResourceType> resources;
        ExportCallback progressCallback = nullptr;
    };
}