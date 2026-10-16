//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#pragma once

#include "resource/detail/resource_files.hpp"
#include "named_resource_store_index_record.hpp"

namespace MKD
{
    class NamedResourceStoreContents
    {
    public:
        static Result<NamedResourceStoreContents> open(const std::filesystem::path& directory);
        [[nodiscard]] Result<RetainedSpan> get(const NamedResourceStoreIndexRecord& record) const;

    private:
        explicit NamedResourceStoreContents(detail::ResourceFiles files) : files_(std::move(files))
        {
        }

        detail::ResourceFiles files_;
    };
}
