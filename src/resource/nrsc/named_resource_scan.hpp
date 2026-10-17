//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/resource/named_resource_store.hpp"
#include "named_resource_store_index_record.hpp"

#include <span>
#include <vector>

namespace MKD
{
    class NamedResourceStoreContents;
}

namespace MKD::detail
{
    struct NamedScanRecord
    {
        std::string_view id;
        NamedResourceStoreIndexRecord location;
    };

    class NamedResourceBatch
    {
    public:
        [[nodiscard]] size_t size() const noexcept { return records_.size(); }
        [[nodiscard]] std::string_view idAt(const size_t index) const { return records_[index].id; }
        [[nodiscard]] Result<RetainedSpan> dataAt(size_t index) const;

    private:
        friend class NamedResourceScan;
        NamedResourceBatch(const NamedResourceStoreContents& contents, std::span<const NamedScanRecord> records);
        const NamedResourceStoreContents* contents_;
        std::span<const NamedScanRecord> records_;
    };

    /** Metadata is ordered by file and offset. Payloads are read only when requested. */
    class NamedResourceScan
    {
    public:
        static Result<NamedResourceScan> open(const NamedResourceStore& store);
        [[nodiscard]] size_t size() const noexcept;
        [[nodiscard]] NamedResourceBatch batchAt(size_t index) const;

    private:
        static constexpr size_t batchSize = 64;
        const NamedResourceStoreContents* contents_;
        std::vector<NamedScanRecord> records_;
    };
}
