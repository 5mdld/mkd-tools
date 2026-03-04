//
// kiwakiwaaにより 2026/01/16 に作成されました。
//

#include "MKD/resource/nrsc.hpp"
#include "nrsc_index.hpp"
#include "nrsc_data.hpp"

#include <cassert>
#include <format>

namespace MKD
{
    struct Nrsc::Impl
    {
        NrscIndex index;
        NrscData data;

        Impl(NrscIndex&& index, NrscData&& data)
            : index(std::move(index)), data(std::move(data))
        {
        }
    };


    Nrsc::Nrsc(std::unique_ptr<Impl> impl) noexcept
        : impl_(std::move(impl))
    {
    }


    Nrsc::~Nrsc() = default;
    Nrsc::Nrsc(Nrsc&&) noexcept = default;
    Nrsc& Nrsc::operator=(Nrsc&&) noexcept = default;


    Result<Nrsc> Nrsc::open(const fs::path& directoryPath)
    {
        auto indexResult = NrscIndex::load(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load nrsc index: {}", indexResult.error()));

        auto dataResult = NrscData::load(directoryPath);
        if (!dataResult)
            return std::unexpected(std::format("Failed to load nrsc data: {}", dataResult.error()));

        return Nrsc{std::make_unique<Impl>(std::move(*indexResult), std::move(*dataResult))};
    }


    Result<RetainedSpan> Nrsc::get(std::string_view id) const
    {
        auto record = impl_->index.findById(id);
        if (!record)
            return std::unexpected(record.error());

        return impl_->data.get(*record);
    }


    Result<NrscItem> Nrsc::getByIndex(const size_t index) const
    {
        auto result = impl_->index.getByIndex(index);
        if (!result)
            return std::unexpected(result.error());

        auto [id, record] = *result;
        auto dataResult = impl_->data.get(record);
        if (!dataResult)
            return std::unexpected(dataResult.error());

        return NrscItem{id, *dataResult};
    }


    size_t Nrsc::size() const noexcept
    {
        return impl_->index.size();
    }


    bool Nrsc::empty() const noexcept
    {
        return impl_->index.empty();
    }


    Nrsc::Iterator::Iterator(const Nrsc* nrsc, const size_t index)
        : nrsc_(nrsc), index_(index)
    {
    }

    Nrsc::Iterator::value_type Nrsc::Iterator::operator*() const
    {
        assert(nrsc_ != nullptr && "Dereferencing invalid iterator");
        assert(index_ < nrsc_->size() && "Dereferencing end iterator");

        auto result = nrsc_->getByIndex(index_);
        if (!result)
        {
            throw std::runtime_error(
                std::format("Nrsc iteration failed at position {}: {}",
                    index_, result.error()));
        }

        return *result;
    }

    Nrsc::Iterator& Nrsc::Iterator::operator++()
    {
        ++index_;
        return *this;
    }

    Nrsc::Iterator Nrsc::Iterator::operator++(int)
    {
        const Iterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool Nrsc::Iterator::operator==(const Iterator& other) const
    {
        return nrsc_ == other.nrsc_ && index_ == other.index_ ;
    }

    bool Nrsc::Iterator::operator!=(const Iterator& other) const
    {
        return !(*this == other);
    }


    Nrsc::Iterator Nrsc::begin() const
    {
        return Iterator{this, 0};
    }


    Nrsc::Iterator Nrsc::end() const
    {
        return Iterator{this, size()};
    }
}
