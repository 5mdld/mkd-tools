//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"
#include "MKD/result.hpp"

#include <filesystem>

namespace MKD
{
    class Dictionary;

    /**
     * Synchronously exports the selected resources. An empty selection includes all resource types.
     * Failed, partially written files are removed.
     */
    [[nodiscard]] Result<ExportResult> exportToDirectory(
        const Dictionary& dictionary,
        const std::filesystem::path& destination,
        const ExportOptions& options = {},
        ExportCallback progress = {});
}
