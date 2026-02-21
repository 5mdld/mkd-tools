//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#pragma once

#include "MKD/output/export_options.hpp"
#include "MKD/output/event.hpp"

#include <indicators.h>

namespace MKDCLI
{
    class ExportProgress
    {
    public:
        void onEvent(const MKD::ExportEvent& event);
        void finish();

    private:
        void onExportBegin(const MKD::ExportBeginEvent& e);
        void onPhaseBegin(const MKD::PhaseBeginEvent& e);
        void onProgress(const MKD::ProgressEvent& e) const;
        void onPhaseEnd(const MKD::PhaseEndEvent& e);

        size_t grandTotal_ = 0;
        size_t phaseOffset_ = 0; // cumulative items from completed phases
        MKD::ResourceType currentPhase_{};

        std::unique_ptr<indicators::ProgressBar> bar_;
    };
}