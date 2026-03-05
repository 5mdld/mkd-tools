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
            if constexpr (std::is_same_v<T, MKD::ExportBeginEvent>)
                onExportBegin(e);
            else if constexpr (std::is_same_v<T, MKD::PhaseBeginEvent>)
                onPhaseBegin(e);
            else if constexpr (std::is_same_v<T, MKD::ProgressEvent>)
                onProgress(e);
            else if constexpr (std::is_same_v<T, MKD::PhaseEndEvent>)
                onPhaseEnd(e);
        }, event);
    }

    void ExportProgress::finish()
    {
        bar_->set_option(indicators::option::PostfixText{"│ Complete"});
        if (bar_ && !bar_->is_completed())
            bar_->mark_as_completed();
        bar_.reset();
        indicators::show_console_cursor(true);
    }


    void ExportProgress::onExportBegin(const MKD::ExportBeginEvent& e)
    {
        grandTotal_ = e.totalItems;
        phaseOffset_ = 0;
        indicators::show_console_cursor(false);
        bar_ = std::make_unique<indicators::ProgressBar>(
            indicators::option::BarWidth{40},
            indicators::option::Start{"⟨"},
            indicators::option::Fill{"▓"},
            indicators::option::Lead{"▒"},
            indicators::option::Remainder{"─"},
            indicators::option::End{"⟩"},
            indicators::option::MaxProgress{grandTotal_},
            indicators::option::ShowPercentage{true},
            indicators::option::ShowElapsedTime{true},
            indicators::option::PrefixText{"EXPORTING  "}
        );

        if (Colour::isEnabled())
        {
            bar_->set_option(indicators::option::ForegroundColor{indicators::Color::red});
            bar_->set_option(indicators::option::FontStyles{std::vector{indicators::FontStyle::bold}
            });
        }
    }


    void ExportProgress::onPhaseBegin(const MKD::PhaseBeginEvent& e)
    {
        if (MKD::isHeavyResource(e.type))
            currentPhase_ = e.type;
    }


    void ExportProgress::onProgress(const MKD::ProgressEvent& e) const
    {
        if (!bar_ || !MKD::isHeavyResource(e.type))
            return;

        const size_t globalCompleted = phaseOffset_ + e.completedItems;
        auto name = MKD::resourceTypeName(e.type);

        bar_->set_option(indicators::option::PostfixText{
            std::format("│ {} {}/{}", name, e.completedItems, e.totalItems)
        });
        bar_->set_progress(globalCompleted);
    }


    void ExportProgress::onPhaseEnd(const MKD::PhaseEndEvent& e)
    {
        if (MKD::isHeavyResource(e.type))
            phaseOffset_ += e.result.totalResources;

        for (const auto& err : e.result.errors)
            std::cerr << Colour::yellow("  ⚠ warning: ") << err << "\n";
    }
}
