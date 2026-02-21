//
// kiwakiwaaにより 2026/02/21 に作成されました。
//

#include "MKDCLI/format.hpp"
#include "MKDCLI/progress.hpp"

namespace MKDCLI
{
    void ExportProgress::onEvent(const MKD::ExportEvent& event)
    {
        std::visit([this](const auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MKD::PhaseBeginEvent>)
                onPhaseBegin(e);
            else if constexpr (std::is_same_v<T, MKD::ProgressEvent>)
                onProgress(e);
            else if constexpr (std::is_same_v<T, MKD::PhaseEndEvent>)
                onPhaseEnd(e);
        }, event);
    }

    void ExportProgress::onPhaseBegin(const MKD::PhaseBeginEvent& e)
    {
        currentType_ = e.type;

        if (e.totalItems == 0)
            return;

        bar_ = std::make_unique<indicators::ProgressBar>(
            indicators::option::BarWidth{40},
            indicators::option::Start{"["},
            indicators::option::Fill{"█"},
            indicators::option::Lead{"█"},
            indicators::option::Remainder{"░"},
            indicators::option::End{"]"},
            indicators::option::MaxProgress{e.totalItems},
            indicators::option::ShowPercentage{true},
            indicators::option::PrefixText{std::format("{:<10}", MKD::resourceTypeName(e.type))},
            indicators::option::ShowElapsedTime{true}
        );
    }


    void ExportProgress::onProgress(const MKD::ProgressEvent& e) const
    {
        if (!bar_)
            return;

        bar_->set_option(indicators::option::PostfixText{
            std::format("{}/{}", e.completedItems, e.totalItems)
        });
        bar_->set_progress(e.completedItems);
    }

    void ExportProgress::onPhaseEnd(const MKD::PhaseEndEvent& e)
    {
        if (bar_ && !bar_->is_completed())
            bar_->mark_as_completed();

        bar_.reset();

        for (const auto& err : e.result.errors)
            std::cerr << Colour::yellow("  warning: ") << err << "\n";
    }
}
