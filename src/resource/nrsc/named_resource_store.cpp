//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#include "MKD/resource/named_resource_store.hpp"
#include "named_resource_store_index.hpp"
#include "named_resource_store_contents.hpp"

#include <cassert>
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


    Result<NamedResourceStore> NamedResourceStore::open(const fs::path& directoryPath)
    {
        auto indexResult = NamedResourceStoreIndex::load(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load nrsc index: {}", indexResult.error()));

        auto dataResult = NamedResourceStoreContents::load(directoryPath);
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


    Result<NamedResourceStoreItem> NamedResourceStore::getByIndex(const size_t index) const
    {
        auto result = impl_->index.getByIndex(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return NamedResourceStoreItem{id, *dataResult};
    }


    size_t NamedResourceStore::size() const noexcept
    {
        return impl_->index.size();
    }


    bool NamedResourceStore::empty() const noexcept
    {
        return impl_->index.empty();
    }


    NamedResourceStore::Iterator::Iterator(const NamedResourceStore* nrsc, const size_t index)
        : store_(nrsc), index_(index)
    {
    }

    NamedResourceStore::Iterator::value_type NamedResourceStore::Iterator::operator*() const
    {
        assert(nrsc_ != nullptr && "Dereferencing invalid iterator");
        assert(index_ < nrsc_->size() && "Dereferencing end iterator");

        auto result = store_->getByIndex(index_);
        if (!result)
        {
            throw std::runtime_error(
                std::format("Nrsc iteration failed at position {}: {}",
                    index_, result.error()));
        }

        return *result;
    }

    NamedResourceStore::Iterator& NamedResourceStore::Iterator::operator++()
    {
        ++index_;
        return *this;
    }

    NamedResourceStore::Iterator NamedResourceStore::Iterator::operator++(int)
    {
        const Iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool NamedResourceStore::Iterator::operator==(const Iterator& other) const
    {
        return store_ == other.store_ && index_ == other.index_ ;
    }

    bool NamedResourceStore::Iterator::operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }


    NamedResourceStore::Iterator NamedResourceStore::begin() const
    {
        return Iterator{this, 0};
    }


    NamedResourceStore::Iterator NamedResourceStore::end() const
    {
        return Iterator{this, size()};
    }
}
