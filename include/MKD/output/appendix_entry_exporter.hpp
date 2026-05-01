//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#pragma once

#include "MKD/output/base_exporter.hpp"
#include "MKD/output/export_options.hpp"
#include "MKD/output/export_result.hpp"
#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/resource/headline_store.hpp"

#include <span>

namespace MKD
{
    class AppendixEntryExporter final : public BaseExporter
    {
    public:
        static Result<ExportResult> exportEntryLists(
            std::span<const AppendixEntryList> lists,
            std::span<const HeadlineStore> headlines,
            const ExportOptions& options);

    private:
        static Result<ExportResult> exportEntryList(
            const AppendixEntryList& list,
            std::span<const HeadlineStore> headlines,
            const fs::path& path);
    };
}
