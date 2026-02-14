//
// kiwakiwaaにより 2026/02/14 に作成されました。
//

#pragma once

#include "monokakido/output/common.hpp"
#include "monokakido/output/base_exporter.hpp"
#include "monokakido/resource/headline/headline_store.hpp"

#include <expected>
#include <string>


namespace monokakido
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
        static std::expected<ExportResult, std::string> exportHeadlines(
            const HeadlineStore& store,
            const ExportOptions& options);


    private:

        static std::expected<void, std::string> writeRow(std::ofstream& out, const HeadlineComponents& components);
    };
}