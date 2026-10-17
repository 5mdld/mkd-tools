//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/resource_store.hpp"
#include "resource_store_map.hpp"

#include <optional>
#include <span>
#include <vector>

namespace MKD
{
    class ResourceStoreContents;
}

namespace MKD::detail
{
    struct ScanRecord
    {
        uint32_t id;
        ResourceStoreMapRecord location;
    };

    /** A borrowed scan batch retains its decoded chunk until all its items have been consumed. */
    class ResourceBatch
    {
    public:
        [[nodiscard]] size_t size() const noexcept { return records_.size(); }
        [[nodiscard]] uint32_t idAt(const size_t index) const { return records_[index].id; }
        [[nodiscard]] Result<RetainedSpan> dataAt(size_t index);

    private:
        friend class ResourceScan;
        ResourceBatch(const ResourceStoreContents& contents, std::span<const ScanRecord> records);
        const ResourceStoreContents* contents_;
        std::span<const ScanRecord> records_;
        std::optional<RetainedSpan> chunk_;
        std::optional<std::string> error_;
    };

    /** The store must outlive the scan, and the scan must outlive its batches. */
    class ResourceScan
    {
    public:
        static Result<ResourceScan> open(const ResourceStore& store);
        [[nodiscard]] size_t size() const noexcept { return groups_.size(); }
        [[nodiscard]] ResourceBatch batchAt(size_t index) const;

    private:
        const ResourceStoreContents* contents_;
        std::vector<ScanRecord> records_;
        std::vector<size_t> groups_;
    };
}
