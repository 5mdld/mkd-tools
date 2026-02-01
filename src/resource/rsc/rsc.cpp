//
// kiwakiwaaにより 2026/02/01 に作成されました。
//

#include "monokakido/resource/rsc/rsc.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace monokakido::resource
{
    std::expected<Rsc, std::string> Rsc::open(const fs::path& directoryPath)
    {
        auto indexResult = RscIndex::load(directoryPath);
        if (!indexResult)
            return std::unexpected(std::format("Failed to load rsc index: {}", indexResult.error()));

        auto dictId = getDictId(directoryPath);
        if (!dictId)
            return std::unexpected(std::format("Failed to get dict id from path: {}", directoryPath.string()));

        auto dataResult = RscData::load(directoryPath, *dictId);
        if (!dataResult)
            return std::unexpected(std::format("Failed to load rsc data: {}", dataResult.error()));

        return Rsc(std::move(*indexResult), std::move(*dataResult));
    }


    std::expected<std::span<const uint8_t>, std::string> Rsc::get(const uint32_t itemId) const
    {
        const auto record = index_.findById(itemId);
        if (!record)
            return std::unexpected(std::format("Failed to get record from rsc index: {}", itemId));

        return data_.get(*record);
    }


    std::expected<std::span<const uint8_t>, std::string> Rsc::getSequential()
    {
        sequentialBuffer_.clear();

        for (const auto& mapRecord : index_ | std::views::values)
        {
            auto dataResult = data_.get(mapRecord);
            if (!dataResult)
                return std::unexpected(std::format("Failed to read sequential data: {}", dataResult.error()));

            const auto& chunk = *dataResult;
            sequentialBuffer_.insert(sequentialBuffer_.end(), chunk.begin(), chunk.end());
        }

        return std::span<const uint8_t>(sequentialBuffer_);
    }


    std::optional<std::string> Rsc::detectFontType() const
    {
        if (sequentialBuffer_.empty() || sequentialBuffer_.size() < 8)
            return std::nullopt;

        const uint32_t magic = (sequentialBuffer_[0] << 24 |
                                sequentialBuffer_[1] << 16 |
                                sequentialBuffer_[2] << 8 |
                                sequentialBuffer_[3]);

        if (magic == 0x4F54544F) // "OTTO"
            return "otf"; // OpenType

        return std::nullopt;
    }


    size_t Rsc::size() const noexcept
    {
        return index_.size();
    }


    bool Rsc::empty() const noexcept
    {
        return index_.empty();
    }


    Rsc::Iterator::Iterator(const Rsc* rsc, const size_t index)
        : rsc_(rsc), index_(index)
    {
    }

    Rsc::Iterator::value_type Rsc::Iterator::operator*() const
    {
        assert(rsc_ != nullptr && "Dereferencing invalid iterator");
        assert(index_ < rsc_->size() && "Dereferencing end iterator");

        auto result = rsc_->index_.getByIndex(index_);
        if (!result)
        {
            throw std::runtime_error(
                std::format("Rsc iteration failed at position {}: {}",
                            index_, result.error()));
        }

        auto [itemId, mapRecord] = *result;
        auto dataResult = rsc_->data_.get(mapRecord);
        if (!dataResult)
        {
            throw std::runtime_error(
                std::format("Rsc iteration failed to get data at position {}: {}",
                            index_, dataResult.error()));
        }

        return RscItem{itemId, *dataResult};
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


    Rsc::Rsc(RscIndex&& index, RscData&& data)
    : index_(std::move(index)), data_(std::move(data))
    {
    }


    std::optional<std::string> Rsc::getDictId(const fs::path& directoryPath)
    {
        std::vector<std::string> components;
        for (const auto& part : directoryPath)
            components.emplace_back(part.string());

        for (size_t i = 0; i < components.size(); ++i)
        {
            if (components[i] == "dictionaries")
                return components.at(i + 1);
        }

        return std::nullopt;
    }
}
