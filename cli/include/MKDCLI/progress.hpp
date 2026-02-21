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

    private:
        void onPhaseBegin(const MKD::PhaseBeginEvent& e);
        void onProgress(const MKD::ProgressEvent& e) const;
        void onPhaseEnd(const MKD::PhaseEndEvent& e);

        std::unique_ptr<indicators::ProgressBar> bar_;
        MKD::ResourceType currentType_{};
    };
}