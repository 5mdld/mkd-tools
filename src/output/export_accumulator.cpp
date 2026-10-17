//
// kiwakiwaaにより 2026/02/26 に作成されました。
//

#include "export_accumulator.hpp"

#include <algorithm>


namespace MKD
{
    ExportAccumulator::ExportAccumulator(const size_t total, const ResourceType type, const ExportCallback& callback)
        : totalItems_(total),
          reportInterval_(std::max<size_t>(50, total / 400)),
          type_(type),
          callback_(callback)
    {
    }


    void ExportAccumulator::recordExport(const uint64_t bytes)
    {
        exported_.fetch_add(1, std::memory_order_relaxed);
        totalBytes_.fetch_add(bytes, std::memory_order_relaxed);
        completed_.fetch_add(1, std::memory_order_release);
        maybeReport();
    }


    void ExportAccumulator::recordSkip()
    {
        skipped_.fetch_add(1, std::memory_order_relaxed);
        completed_.fetch_add(1, std::memory_order_release);
        maybeReport();
    }


    void ExportAccumulator::recordFailure(std::string error, const size_t count)
    {
        failed_.fetch_add(count, std::memory_order_relaxed);
        {
            std::lock_guard lock(mutex_);
            errors_.push_back(std::move(error));
        }
        completed_.fetch_add(count, std::memory_order_release);
        maybeReport();
    }


    ExportResult ExportAccumulator::finalize()
    {
        report();

        ExportResult result;
        result.totalFiles = totalItems_;
        result.exported = exported_.load();
        result.skipped = skipped_.load();
        result.failed = failed_.load();
        result.totalBytes = totalBytes_.load();
        result.errors = std::move(errors_);
        return result;
    }


    void ExportAccumulator::maybeReport()
    {
        const auto done = completed();
        if (callback_ && (done == totalItems_ || done % reportInterval_ == 0))
            report();
    }


    void ExportAccumulator::report()
    {
        if (!callback_)
            return;

        std::lock_guard lock(mutex_);
        const auto done = completed();
        if (done == lastReported_ || (done != totalItems_ && done - lastReported_ < reportInterval_))
            return;

        lastReported_ = done;
        callback_(ProgressEvent{
            .type = type_,
            .completedItems = done,
            .totalItems = totalItems_,
            .bytesWritten = totalBytes_.load(std::memory_order_relaxed)
        });
    }


    size_t ExportAccumulator::completed() const
    {
        return completed_.load(std::memory_order_acquire);
    }
}
