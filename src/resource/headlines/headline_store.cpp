//
// kiwakiwaaにより 2026/02/12 に作成されました。
//

#include "MKD/resource/headline_store.hpp"
#include "platform/mmap_file.hpp"
#include "unicode/unicode.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <format>
#include <ranges>
#include <span>

namespace MKD
{
    static_assert(std::endian::native == std::endian::little, "HeadlineStore assumes little-endian byte order");

    namespace
    {
        template<typename T>
        const T* ptrAt(const MappedFile& file, const size_t offset)
        {
            if (offset + sizeof(T) > file.size())
                return nullptr;
            return reinterpret_cast<const T*>(file.data().data() + offset);
        }
    }

    uint32_t HeadlineHeader::effectiveStride() const noexcept
    {
        return recordStride != 0 ? recordStride : HEADLINE_DEFAULT_STRIDE;
    }


    std::u16string HeadlineComponents::full() const
    {
        std::u16string result;
        result.reserve(prefix.size() + headline.size() + suffix.size());

        if (!prefix.empty())
            result.append(prefix);

        if (!headline.empty())
            result.append(headline);

        if (!suffix.empty())
            result.append(suffix);

        return result;
    }


    std::string HeadlineComponents::fullUtf8() const
    {
        return prefixUtf8() + headlineUtf8() + suffixUtf8();
    }


    std::string HeadlineComponents::prefixUtf8() const
    {
        return detail::unicode::toUtf8(prefix);
    }


    std::string HeadlineComponents::headlineUtf8() const
    {
        return detail::unicode::toUtf8(headline);
    }


    std::string HeadlineComponents::suffixUtf8() const
    {
        return detail::unicode::toUtf8(suffix);
    }

    std::string HeadlineComponents::sortingHeadlineUtf8() const
    {
        return detail::unicode::toUtf8(sortingHeadline);
    }


    Result<HeadlineStore> HeadlineStore::load(const fs::path& filePath)
    {
        auto mapped = MappedFile::open(filePath);
        if (!mapped)
            return std::unexpected(std::format("Failed to open headline store '{}'", filePath.string()));

        const auto* header = ptrAt<HeadlineHeader>(*mapped, 0);
        if (!header)
            return std::unexpected("Headline store file too small for header");

        const uint32_t stride = header->effectiveStride();
        const uint32_t entryCount = header->entryCount;
        const size_t recordsOffset = header->recordsOffset;
        const size_t stringsOffset = header->stringsOffset;
        const size_t recordsBytes = static_cast<size_t>(entryCount) * stride;

        if (stride < sizeof(HeadlineRecord))
            return std::unexpected(std::format(
                "Headline record stride {} is smaller than record size {}",
                stride, sizeof(HeadlineRecord)));

        if (recordsOffset < sizeof(HeadlineHeader) || recordsOffset > stringsOffset)
            return std::unexpected("Headline records offset out of bounds");

        if (stringsOffset < sizeof(HeadlineHeader) || stringsOffset > mapped->size())
            return std::unexpected("Headline strings offset out of bounds");

        if (recordsOffset > stringsOffset)
            return std::unexpected("Headline records offset must not exceed strings offset");

        if (recordsBytes > mapped->size() - recordsOffset)
            return std::unexpected("Headline records section exceeds file size");

        if (stringsOffset < recordsOffset + recordsBytes)
            return std::unexpected("Headline strings section overlaps records section");

        return HeadlineStore(
            std::move(*mapped),
            filePath.filename().string(),
            entryCount,
            stride,
            recordsOffset,
            stringsOffset);
    }

    Result<HeadlineComponents> HeadlineStore::operator[](const size_t index) const
    {
        if (index >= entryCount_)
            return std::unexpected(std::format(
                "Headline index {} out of range (size = {})", index, entryCount_));

        return componentsFromRecord(recordAt(index));
    }


    Result<EntryId> HeadlineStore::entryIdAt(const size_t index) const
    {
        if (index >= entryCount_)
            return std::unexpected(std::format(
                "Headline index {} out of range (size = {})", index, entryCount_));

        HeadlineRecord rec{};
        std::memcpy(&rec, recordAt(index), sizeof(rec));
        return rec.entryId();
    }


    Result<HeadlineComponents> HeadlineStore::componentsForEntryId(
        const EntryId& entryId,
        const bool allowFallback) const
    {
        const auto* record = recordForEntryId(entryId, allowFallback);
        if (!record)
            return std::unexpected("headline not found");

        return componentsFromRecord(record);
    }


    Result<std::u16string_view> HeadlineStore::sortingHeadlineForEntryId(const EntryId& entryId) const
    {
        const auto* record = recordForEntryId(entryId, true);
        if (!record)
            return std::unexpected("sorting headline not found");

        return sortingHeadlineFromRecord(record);
    }


    size_t HeadlineStore::size() const noexcept
    {
        return entryCount_;
    }


    bool HeadlineStore::empty() const noexcept
    {
        return entryCount_ == 0;
    }


    std::string_view HeadlineStore::filename() const
    {
        return filename_;
    }


    HeadlineStore::HeadlineStore(MappedFile&& fileData,
                                 std::string filename,
                                 const uint32_t entryCount,
                                 const uint32_t stride,
                                 const size_t recordsOffset,
                                 const size_t stringsOffset)
        : fileData_(std::move(fileData))
          , filename_(std::move(filename))
          , entryCount_(entryCount)
          , stride_(stride)
          , recordsOffset_(recordsOffset)
          , stringsOffset_(stringsOffset)
    {
    }


    HeadlineStore::Iterator::Iterator(const HeadlineStore* store, const size_t pos) noexcept
        : store_(store)
          , position_(pos)
    {
    }

    HeadlineStore::Iterator::value_type HeadlineStore::Iterator::operator*() const
    {
        auto result = (*store_)[position_];
        if (!result)
            throw std::out_of_range(result.error());
        return *result;
    }

    HeadlineStore::Iterator::value_type HeadlineStore::Iterator::operator[](const difference_type n) const
    {
        auto result = (*store_)[position_ + n];
        if (!result)
            throw std::out_of_range(result.error());
        return *result;
    }

    HeadlineStore::Iterator& HeadlineStore::Iterator::operator++() noexcept
    {
        ++position_;
        return *this;
    }

    HeadlineStore::Iterator HeadlineStore::Iterator::operator++(int) noexcept
    {
        const auto copy = *this;
        ++position_;
        return copy;
    }

    HeadlineStore::Iterator& HeadlineStore::Iterator::operator--() noexcept
    {
        --position_;
        return *this;
    }

    HeadlineStore::Iterator HeadlineStore::Iterator::operator--(int) noexcept
    {
        const auto copy = *this;
        --position_;
        return copy;
    }

    HeadlineStore::Iterator& HeadlineStore::Iterator::operator+=(const difference_type n) noexcept
    {
        position_ += n;
        return *this;
    }

    HeadlineStore::Iterator& HeadlineStore::Iterator::operator-=(const difference_type n) noexcept
    {
        position_ -= n;
        return *this;
    }

    HeadlineStore::Iterator HeadlineStore::Iterator::operator+(const difference_type n) const noexcept
    {
        return {store_, position_ + n};
    }

    HeadlineStore::Iterator HeadlineStore::Iterator::operator-(const difference_type n) const noexcept
    {
        return {store_, position_ - n};
    }

    HeadlineStore::Iterator operator+(const HeadlineStore::Iterator::difference_type n,
                                      const HeadlineStore::Iterator& it) noexcept
    {
        return it + n;
    }

    HeadlineStore::Iterator::difference_type HeadlineStore::Iterator::operator-(const Iterator& other) const noexcept
    {
        return static_cast<difference_type>(position_) - static_cast<difference_type>(other.position_);
    }

    HeadlineStore::Iterator HeadlineStore::begin() const noexcept
    {
        return {this, 0};
    }

    HeadlineStore::Iterator HeadlineStore::end() const noexcept
    {
        return {this, entryCount_};
    }


    const uint8_t* HeadlineStore::recordAt(const size_t index) const noexcept
    {
        return fileData_.data().data() + recordsOffset_ + index * stride_;
    }


    const uint8_t* HeadlineStore::recordForEntryId(const EntryId& entryId, const bool allowFallback) const noexcept
    {
        const auto compareEntryId = [](const EntryId& lhs, const EntryId& rhs) {
            if (lhs.pageId != rhs.pageId)
                return lhs.pageId < rhs.pageId;
            return lhs.itemId < rhs.itemId;
        };

        const auto equalEntryId = [](const EntryId& lhs, const EntryId& rhs) {
            return lhs.pageId == rhs.pageId && lhs.itemId == rhs.itemId;
        };

        const auto lowerBoundRecord = [&](const EntryId& target) -> const uint8_t* {
            size_t first = 0;
            size_t count = entryCount_;

            while (count > 0)
            {
                const size_t step = count / 2;
                const size_t middle = first + step;

                HeadlineRecord rec{};
                std::memcpy(&rec, recordAt(middle), sizeof(rec));

                if (compareEntryId(rec.entryId(), target))
                {
                    first = middle + 1;
                    count -= step + 1;
                }
                else
                {
                    count = step;
                }
            }

            if (first >= entryCount_)
                return nullptr;

            const uint8_t* record = recordAt(first);
            HeadlineRecord rec{};
            std::memcpy(&rec, record, sizeof(rec));
            return equalEntryId(rec.entryId(), target) ? record : nullptr;
        };

        if (const auto* record = lowerBoundRecord(entryId))
            return record;

        if (!allowFallback || entryId.itemId == 0)
            return nullptr;

        return lowerBoundRecord(EntryId{
            .pageId = entryId.pageId,
            .itemId = 0,
        });
    }

    Result<std::u16string_view> HeadlineStore::stringAt(const uint32_t offset) const
    {
        if (offset % sizeof(char16_t) != 0)
            return std::unexpected(std::format("Headline string offset {} is not UTF-16 aligned", offset));

        const size_t absoluteOffset = stringsOffset_ + offset;
        if (absoluteOffset >= fileData_.size())
            return std::unexpected(std::format("Headline string offset {} out of bounds", offset));

        const auto stringBytes = fileData_.data().subspan(absoluteOffset);
        if (stringBytes.size() < sizeof(char16_t))
            return std::unexpected(std::format("Headline string offset {} has no terminator", offset));

        const auto* str = reinterpret_cast<const char16_t*>(stringBytes.data());
        const size_t units = stringBytes.size() / sizeof(char16_t);

        for (size_t len = 0; len < units; ++len)
        {
            if (str[len] == u'\0')
                return std::u16string_view(str, len);
        }

        return std::unexpected(std::format("Headline string at offset {} is not null-terminated", offset));
    }


    Result<HeadlineComponents> HeadlineStore::componentsFromRecord(const uint8_t* record) const
    {
        HeadlineRecord rec{};
        std::memcpy(&rec, record, sizeof(rec));

        auto headline = stringAt(rec.headlineOffset);
        if (!headline)
            return std::unexpected(headline.error());

        auto sortingHeadline = sortingHeadlineFromRecord(record);
        if (!sortingHeadline)
            return std::unexpected(sortingHeadline.error());

        std::u16string_view prefix;
        if (rec.prefixOffset != 0)
        {
            auto p = stringAt(rec.prefixOffset);
            if (!p) return std::unexpected(p.error());
            prefix = *p;
        }

        std::u16string_view suffix;
        if (rec.suffixOffset != 0)
        {
            auto s = stringAt(rec.suffixOffset);
            if (!s) return std::unexpected(s.error());
            suffix = *s;
        }

        return HeadlineComponents{
            .prefix = prefix,
            .headline = *headline,
            .suffix = suffix,
            .sortingHeadline = *sortingHeadline,
            .entryId = rec.entryId(),
        };
    }


    Result<std::u16string_view> HeadlineStore::sortingHeadlineFromRecord(const uint8_t* record) const
    {
        HeadlineRecord rec{};
        std::memcpy(&rec, record, sizeof(rec));

        uint32_t sortingOffset = 0;
        if (stride_ >= HEADLINE_EXTENDED_STRIDE)
            std::memcpy(&sortingOffset, record + sizeof(HeadlineRecord), sizeof(sortingOffset));

        if (sortingOffset != 0)
            return stringAt(sortingOffset);

        return stringAt(rec.headlineOffset);
    }
}
