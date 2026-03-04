//
// kiwakiwaaにより 2026/02/14 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"
#include "MKD/output/export_result.hpp"
#include "MKD/output/base_exporter.hpp"
#include "MKD/resource/headline_store.hpp"

namespace MKD
{
    /**
     * Export headline data to TSV format.
     */
    class HeadlineExporter final : public BaseExporter
    {
    public:
        /**
         * Export headlines from a HeadlineStore to TSV.
         *
         * @param store The loaded headline store
         * @param options Export options
         * @return ExportResult with statistics or error
         */
        static Result<ExportResult> exportHeadlines(const HeadlineStore& store, const ExportOptions& options);


    private:

        static Result<void> writeRow(std::ofstream& out, const HeadlineComponents& components);
    };
}