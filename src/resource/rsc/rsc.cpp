//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "MKD/resource/rsc.hpp"
#include "rsc_index.hpp"
#include "rsc_data.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace MKD
{
    struct Rsc::Impl
    {
        RscIndex index;
        RscData data;

        Impl(RscIndex&& index, RscData&& data)
            : index(std::move(index)), data(std::move(data))
        {
        }
    };


    Rsc::Rsc(std::unique_ptr<Impl> impl) noexcept
        : impl_(std::move(impl))
    {
    }


    Rsc::~Rsc() noexcept = default;
    Rsc::Rsc(Rsc&&) noexcept = default;
    Rsc& Rsc::operator=(Rsc&&) noexcept = default;


    Result<Rsc> Rsc::open(const fs::path& directoryPath, std::string_view dictId)
    {
        auto indexResult = RscIndex::load(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load rsc index: {}", indexResult.error()));

        auto dataResult = RscData::load(directoryPath, dictId, indexResult->mapVersion());
        if (!dataResult)
            return std::unexpected(std::format("Failed to load rsc data: {}", dataResult.error()));

        return Rsc(std::make_unique<Impl>(std::move(*indexResult), std::move(*dataResult)));
    }


    Result<RetainedSpan> Rsc::get(const uint32_t itemId) const
    {
        const auto record = impl_->index.findById(itemId);
        if (!record)
            return std::unexpected(std::format("Failed to get record from rsc index: {}", itemId));

        return impl_->data.get(*record);
    }


    Result<RscItem> Rsc::getByIndex(const size_t index) const
    {
        auto result = impl_->index.getByIndex(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return RscItem{id, *dataResult};
    }


    size_t Rsc::size() const noexcept
    {
        return impl_->index.size();
    }


    bool Rsc::empty() const noexcept
    {
        return impl_->index.empty();
    }


    Rsc::Iterator::Iterator(const Rsc* rsc, const size_t index)
        : rsc_(rsc), index_(index)
    {
    }

    Rsc::Iterator::value_type Rsc::Iterator::operator*() const
    {
        assert(rsc_ != nullptr && "Dereferencing invalid iterator");
        assert(index_ < rsc_->size() && "Dereferencing end iterator");

        auto result = rsc_->getByIndex(index_);
        if (!result)
        {
            throw std::runtime_error(
                std::format("Rsc iteration failed at position {}: {}",
                    index_, result.error()));
        }

        return *result;
    }

    Rsc::Iterator& Rsc::Iterator::operator++()
    {
        ++index_;
        return *this;
    }

    Rsc::Iterator Rsc::Iterator::operator++(int)
    {
        const Iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool Rsc::Iterator::operator==(const Iterator& other) const
    {
        return rsc_ == other.rsc_ && index_ == other.index_;
    }

    bool Rsc::Iterator::operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }

    Rsc::Iterator Rsc::begin() const
    {
        return Iterator{this, 0};
    }

    Rsc::Iterator Rsc::end() const
    {
        return Iterator{this, size()};
    }
}
