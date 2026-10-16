//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#include "MKD/resource/named_resource_store.hpp"
#include "named_resource_store_index.hpp"
#include "named_resource_store_contents.hpp"

#include <format>

namespace MKD
{
    struct NamedResourceStore::Impl
    {
        NamedResourceStoreIndex index;
        NamedResourceStoreContents data;

        Impl(NamedResourceStoreIndex&& index, NamedResourceStoreContents&& data)
            : index(std::move(index)), data(std::move(data))
        {
        }
    };


    NamedResourceStore::NamedResourceStore(std::unique_ptr<Impl> impl) noexcept
        : impl_(std::move(impl))
    {
    }


    NamedResourceStore::~NamedResourceStore() = default;
    NamedResourceStore::NamedResourceStore(NamedResourceStore&&) noexcept = default;
    NamedResourceStore& NamedResourceStore::operator=(NamedResourceStore&&) noexcept = default;


    Result<NamedResourceStore> NamedResourceStore::open(const std::filesystem::path& directoryPath)
    {
        auto indexResult = NamedResourceStoreIndex::open(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load nrsc index: {}", indexResult.error()));

        auto dataResult = NamedResourceStoreContents::open(directoryPath);
        if (!dataResult)
            return std::unexpected(std::format("Failed to load nrsc data: {}", dataResult.error()));

        return NamedResourceStore{std::make_unique<Impl>(std::move(*indexResult), std::move(*dataResult))};
    }


    Result<RetainedSpan> NamedResourceStore::get(std::string_view id) const
    {
        auto record = impl_->index.findById(id);
        if (!record)
            return std::unexpected(record.error());

        return impl_->data.get(*record);
    }


    Result<NamedResourceStoreItem> NamedResourceStore::itemAt(const size_t index) const
    {
        auto result = impl_->index.itemAt(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return NamedResourceStoreItem{std::string(id), std::move(*dataResult)};
    }


    size_t NamedResourceStore::size() const noexcept
    {
        return impl_->index.size();
    }


    bool NamedResourceStore::empty() const noexcept
    {
        return impl_->index.empty();
    }


    Result<std::string_view> NamedResourceStore::idAt(size_t index) const
    {
        return impl_->index.idAt(index);
    }

    Result<size_t> NamedResourceStore::indexOf(std::string_view id) const
    {
        return impl_->index.indexOf(id);
    }
}
