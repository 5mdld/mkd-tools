//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "entry_allocator.hpp"
#include "MKD/dictionary/product_writer.hpp"
#include "io/write_validation.hpp"
#include "binary/binary_buffer.hpp"
#include "io/output_file.hpp"
#include <algorithm>
#include <ranges>

namespace MKD::detail
{
    Result<uint32_t> EntryAllocator::reserve(std::optional<uint32_t> requested)
    {
        if (!requested)
            while (pages_.contains(next_) || reserved_.contains(next_))
                ++next_;

        const auto page = requested.value_or(next_);
        if (auto valid = WriteValidation::entryId({page, 0}); !valid)
            return std::unexpected(valid.error());

        if (pages_.contains(page) || reserved_.contains(page))
            return std::unexpected("Page ID is already written or reserved: " + std::to_string(page));

        reserved_.emplace(page, std::array<uint32_t, monokakido::entry::targetRanges.size()>{});
        return page;
    }

    Result<EntryId> EntryAllocator::addTarget(const uint32_t pageId, EntryTargetKind kind)
    {
        if (pages_.contains(pageId))
            return std::unexpected("Cannot allocate targets after writing entry XML");

        const auto index = static_cast<size_t>(kind);
        if (index >= monokakido::entry::targetRanges.size())
            return std::unexpected("Invalid entry target kind");

        auto range = monokakido::entry::targetRanges[index];
        if (profile_ == CreationProfile::Kanwa && (kind == EntryTargetKind::Sense || kind == EntryTargetKind::Example))
            range.last -= monokakido::search::normalizedHeadlineOffset;

        auto& count = reserved_.at(pageId)[index];
        const auto item = range.first + count;
        if (item > range.last)
            return std::unexpected("Entry target range exhausted on page " + std::to_string(pageId));

        ++count;
        const EntryId target{pageId, static_cast<uint16_t>(item)};
        return target;
    }

    Result<void> EntryAllocator::validateOrder() const
    {
        if (!order_.empty())
        {
            std::unordered_set ordered(order_.begin(), order_.end());
            if (ordered != pages_ || ordered.size() != order_.size())
                return std::unexpected("Entry order must contain every page exactly once");
        }

        return {};
    }

    Result<void> EntryAllocator::writeOrder(const std::filesystem::path& root)
    {
        if (order_.empty())
        {
            order_.assign(pages_.begin(), pages_.end());
            std::ranges::sort(order_);
        }

        const auto maximum = *std::ranges::max_element(order_);
        std::vector positions(maximum, UINT32_MAX);
        for (uint32_t i = 0; i < order_.size(); ++i)
            positions[order_[i] - 1] = i;

        BinaryBuffer bytes(8 + uint64_t{4} * (positions.size() + order_.size()));
        bytes.writeLittle(static_cast<uint32_t>(order_.size()));
        bytes.writeLittle(maximum);

        for (const auto position : positions)
            bytes.writeLittle(position);
        for (const auto page : order_)
            bytes.writeLittle(page);

        return OutputFile::writeAll(root / monokakido::paths::contents / monokakido::paths::entryOrder, bytes.data());
    }

    Result<void> EntryAllocator::recordPage(const uint32_t page)
    {
        if (auto valid = WriteValidation::entryId({page, 0}); !valid)
            return valid;

        if (!pages_.insert(page).second)
            return std::unexpected("Duplicate page ID");

        return {};
    }

    Result<void> EntryAllocator::validateReservations() const
    {
        for (const auto& page : reserved_ | std::views::keys)
            if (!pages_.contains(page))
                return std::unexpected("Reserved entry has no XML: page " + std::to_string(page));
        return {};
    }

    Result<void> EntryAllocator::setOrder(std::span<const uint32_t> pages)
    {
        if (pages.empty())
            return std::unexpected("Explicit entry order must not be empty");

        order_.assign(pages.begin(), pages.end());
        return {};
    }
}
