//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "MKD/resource/resource_store.hpp"
#include "resource_store_index.hpp"
#include "resource_store_contents.hpp"

#include <format>

namespace MKD
{
    struct ResourceStore::Impl
    {
        ResourceStoreIndex index;
        ResourceStoreContents data;

        Impl(ResourceStoreIndex&& index, ResourceStoreContents&& data)
            : index(std::move(index)), data(std::move(data))
        {
        }
    };


    ResourceStore::ResourceStore(std::unique_ptr<Impl> impl) noexcept
        : impl_(std::move(impl))
    {
    }


    ResourceStore::~ResourceStore() noexcept = default;
    ResourceStore::ResourceStore(ResourceStore&&) noexcept = default;
    ResourceStore& ResourceStore::operator=(ResourceStore&&) noexcept = default;


    Result<ResourceStore> ResourceStore::open(const std::filesystem::path& directoryPath, std::string_view dictId)
    {
        auto indexResult = ResourceStoreIndex::open(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load rsc index: {}", indexResult.error()));

        auto dataResult = ResourceStoreContents::open(directoryPath, dictId, indexResult->mapVersion());
        if (!dataResult)
            return std::unexpected(std::format("Failed to load rsc data: {}", dataResult.error()));

        return ResourceStore(std::make_unique<Impl>(std::move(*indexResult), std::move(*dataResult)));
    }


    Result<RetainedSpan> ResourceStore::get(const uint32_t itemId) const
    {
        const auto record = impl_->index.findById(itemId);
        if (!record)
            return std::unexpected(record.error());

        return impl_->data.get(*record);
    }


    Result<ResourceStoreItem> ResourceStore::itemAt(const size_t index) const
    {
        auto result = impl_->index.itemAt(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return ResourceStoreItem{id, *dataResult};
    }


    Result<uint32_t> ResourceStore::idAt(const size_t index) const
    {
        auto result = impl_->index.itemAt(index);
        if (!result)
            return std::unexpected(result.error());

        return result->first;
    }

    Result<size_t> ResourceStore::indexOf(const uint32_t itemId) const
    {
        return impl_->index.indexOf(itemId);
    }


    size_t ResourceStore::size() const noexcept
    {
        return impl_->index.size();
    }


    bool ResourceStore::empty() const noexcept
    {
        return impl_->index.empty();
    }
}
