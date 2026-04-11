//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "MKD/resource/resource_store.hpp"
#include "resource_store_index.hpp"
#include "resource_store_contents.hpp"

#include <algorithm>
#include <cassert>
#include <format>
#include <ranges>
#include <stdexcept>

#include "utf8.h"

namespace MKD
{
    Result<std::string_view> ResourceStoreItem::asUtf8StringView() const
    {
        if (data.empty())
            return std::string_view{};

        const auto* begin = data.data();
        const auto* end = begin + data.size();

        if (const auto* it = utf8::find_invalid(begin, end); it != end)
        {
            const auto offset = static_cast<size_t>(it - begin);
            return std::unexpected(std::format("Invalid UTF-8 sequence at byte offset {}", offset));
        }

        return std::string_view(reinterpret_cast<const char*>(begin), data.size());
    }


    Result<std::string> ResourceStoreItem::asUtf8String() const
    {
        auto view = asUtf8StringView();
        if (!view)
            return std::unexpected(view.error());

        return std::string(*view);
    }


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


    Result<ResourceStore> ResourceStore::open(const fs::path& directoryPath, std::string_view dictId)
    {
        auto indexResult = ResourceStoreIndex::load(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load rsc index: {}", indexResult.error()));

        auto dataResult = ResourceStoreContents::load(directoryPath, dictId, indexResult->mapVersion());
        if (!dataResult)
            return std::unexpected(std::format("Failed to load rsc data: {}", dataResult.error()));

        return ResourceStore(std::make_unique<Impl>(std::move(*indexResult), std::move(*dataResult)));
    }


    Result<RetainedSpan> ResourceStore::get(const uint32_t itemId) const
    {
        const auto record = impl_->index.findById(itemId);
        if (!record)
            return std::unexpected(std::format("Failed to get record from rsc index: {}", itemId));

        return impl_->data.get(*record);
    }


    Result<ResourceStoreItem> ResourceStore::getByIndex(const size_t index) const
    {
        auto result = impl_->index.getByIndex(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return ResourceStoreItem{id, *dataResult};
    }


    Result<uint32_t> ResourceStore::idAtIndex(const size_t index) const
    {
        auto result = impl_->index.getByIndex(index);
        if (!result)
            return std::unexpected(result.error());

        return result->first;
    }


    size_t ResourceStore::size() const noexcept
    {
        return impl_->index.size();
    }


    bool ResourceStore::empty() const noexcept
    {
        return impl_->index.empty();
    }


    ResourceStore::Iterator::Iterator(const ResourceStore* rsc, const size_t index)
        : store_(rsc), index_(index)
    {
    }

    ResourceStore::Iterator::value_type ResourceStore::Iterator::operator*() const
    {
        assert(store_ != nullptr && "Dereferencing invalid iterator");
        assert(index_ < store_->size() && "Dereferencing end iterator");

        auto result = store_->getByIndex(index_);
        if (!result)
        {
            throw std::runtime_error(
                std::format("Rsc iteration failed at position {}: {}",
                    index_, result.error()));
        }

        return *result;
    }

    ResourceStore::Iterator& ResourceStore::Iterator::operator++()
    {
        ++index_;
        return *this;
    }

    ResourceStore::Iterator ResourceStore::Iterator::operator++(int)
    {
        const Iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool ResourceStore::Iterator::operator==(const Iterator& other) const
    {
        return store_ == other.store_ && index_ == other.index_;
    }

    bool ResourceStore::Iterator::operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }

    ResourceStore::Iterator ResourceStore::begin() const
    {
        return Iterator{this, 0};
    }

    ResourceStore::Iterator ResourceStore::end() const
    {
        return Iterator{this, size()};
    }
}
