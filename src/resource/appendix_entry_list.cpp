//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "MKD/resource/common.hpp"

#include <cstring>
#include <format>
#include <limits>

namespace MKD
{
    namespace
    {
        constexpr size_t HEADER_SIZE = 36;
        constexpr size_t ENTRY_SIZE = 8;
        constexpr size_t LABEL_OFFSET_SIZE = 4;
        constexpr size_t FIRST_CHARACTER_SLOT_SIZE = 4;

        uint16_t readU16(const std::span<const uint8_t> data, const size_t offset) noexcept
        {
            uint16_t value = 0;
            std::memcpy(&value, data.data() + offset, sizeof(value));
            return value;
        }

        uint32_t readU32(const std::span<const uint8_t> data, const size_t offset) noexcept
        {
            uint32_t value = 0;
            std::memcpy(&value, data.data() + offset, sizeof(value));
            return value;
        }

        bool hasNullTerminator(const std::span<const uint8_t> data, const size_t offset) noexcept
        {
            for (size_t i = offset; i < data.size(); ++i)
            {
                if (data[i] == 0)
                    return true;
            }
            return false;
        }

        Result<void> validateLabelTable(
            const std::span<const uint8_t> data,
            const uint32_t count,
            const size_t entriesEnd,
            const uint32_t offset)
        {
            if (offset == 0)
                return {};

            if (offset < entriesEnd || offset > data.size())
                return std::unexpected(std::format("Label table offset {} out of bounds", offset));

            const size_t tableSize = static_cast<size_t>(count) * LABEL_OFFSET_SIZE;
            if (tableSize > data.size() - offset)
                return std::unexpected("Label table exceeds file size");

            for (uint32_t i = 0; i < count; ++i)
            {
                const uint32_t stringOffset = readU32(data, offset + static_cast<size_t>(i) * LABEL_OFFSET_SIZE);
                if (stringOffset < entriesEnd || stringOffset >= data.size())
                    return std::unexpected(std::format("Label {} offset {} out of bounds", i, stringOffset));
                if (!hasNullTerminator(data, stringOffset))
                    return std::unexpected(std::format("Label {} is not null-terminated", i));
            }

            return {};
        }

        Result<void> validateFirstCharacterTable(
            const std::span<const uint8_t> data,
            const uint32_t count,
            const size_t entriesEnd,
            const uint32_t offset)
        {
            if (offset == 0)
                return {};

            if (offset < entriesEnd || offset > data.size())
                return std::unexpected(std::format("First-character table offset {} out of bounds", offset));

            const size_t tableSize = static_cast<size_t>(count) * FIRST_CHARACTER_SLOT_SIZE;
            if (tableSize > data.size() - offset)
                return std::unexpected("First-character table exceeds file size");

            return {};
        }
    }

    struct AppendixEntryList::Impl
    {
        Impl(
            MappedFile&& fileData,
            std::string filename,
            std::string title,
            const uint32_t count,
            const uint32_t rows,
            const uint32_t columns,
            const uint32_t labelTableOffset,
            const uint32_t firstCharacterTableOffset)
            : fileData(std::move(fileData))
              , filename(std::move(filename))
              , title(std::move(title))
              , count(count)
              , rows(rows)
              , columns(columns)
              , labelTableOffset(labelTableOffset)
              , firstCharacterTableOffset(firstCharacterTableOffset)
        {
        }

        [[nodiscard]] EntryId entryIdAt(const size_t index) const noexcept
        {
            const size_t offset = HEADER_SIZE + index * ENTRY_SIZE;
            const auto data = fileData.data();

            return EntryId{
                .pageId = readU32(data, offset),
                .itemId = readU16(data, offset + sizeof(uint32_t)),
            };
        }

        [[nodiscard]] std::optional<std::string_view> labelAt(const size_t index) const noexcept
        {
            if (labelTableOffset == 0)
                return std::nullopt;

            const auto data = fileData.data();
            const uint32_t labelOffset = readU32(data, labelTableOffset + index * LABEL_OFFSET_SIZE);
            const auto* label = reinterpret_cast<const char*>(data.data() + labelOffset);
            return std::string_view(label);
        }

        [[nodiscard]] std::optional<char16_t> firstCharacterAt(const size_t index) const noexcept
        {
            if (firstCharacterTableOffset == 0)
                return std::nullopt;

            return static_cast<char16_t>(
                readU16(fileData.data(), firstCharacterTableOffset + index * FIRST_CHARACTER_SLOT_SIZE));
        }

        MappedFile fileData;
        std::string filename;
        std::string title;
        uint32_t count;
        uint32_t rows;
        uint32_t columns;
        uint32_t labelTableOffset;
        uint32_t firstCharacterTableOffset;
    };


    Result<AppendixEntryList> AppendixEntryList::load(const fs::path& filePath, std::string title)
    {
        auto mapped = MappedFile::open(filePath);
        if (!mapped)
            return std::unexpected(mapped.error());

        const auto data = mapped->data();
        if (data.size() < HEADER_SIZE)
            return std::unexpected("Appendix entry list is smaller than its header");

        const uint32_t count = readU32(data, 0);
        const uint32_t rows = readU32(data, 4);
        const uint32_t columns = readU32(data, 8);
        const uint32_t labelTableOffset = readU32(data, 28);
        const uint32_t firstCharacterTableOffset = readU32(data, 32);

        const size_t entryCount = count;
        if (entryCount > (std::numeric_limits<size_t>::max() - HEADER_SIZE) / ENTRY_SIZE)
            return std::unexpected("Appendix entry count overflows addressable size");

        const size_t entriesEnd = HEADER_SIZE + entryCount * ENTRY_SIZE;
        if (entriesEnd > data.size())
            return std::unexpected("Appendix entry records exceed file size");

        if (auto result = validateLabelTable(data, count, entriesEnd, labelTableOffset); !result)
            return std::unexpected(result.error());

        if (auto result = validateFirstCharacterTable(data, count, entriesEnd, firstCharacterTableOffset); !result)
            return std::unexpected(result.error());

        return AppendixEntryList(std::make_unique<Impl>(
            std::move(*mapped),
            filePath.filename().string(),
            std::move(title),
            count,
            rows,
            columns,
            labelTableOffset,
            firstCharacterTableOffset));
    }


    AppendixEntryList::~AppendixEntryList() = default;


    AppendixEntryList::AppendixEntryList(AppendixEntryList&&) noexcept = default;


    AppendixEntryList& AppendixEntryList::operator=(AppendixEntryList&&) noexcept = default;


    AppendixEntryList::AppendixEntryList(std::unique_ptr<Impl> impl)
        : impl_(std::move(impl))
    {
    }


    size_t AppendixEntryList::size() const noexcept
    {
        return impl_->count;
    }


    bool AppendixEntryList::empty() const noexcept
    {
        return impl_->count == 0;
    }


    uint32_t AppendixEntryList::rows() const noexcept
    {
        return impl_->rows;
    }


    uint32_t AppendixEntryList::columns() const noexcept
    {
        return impl_->columns;
    }


    bool AppendixEntryList::hasLabels() const noexcept
    {
        return impl_->labelTableOffset != 0;
    }


    bool AppendixEntryList::hasFirstCharacters() const noexcept
    {
        return impl_->firstCharacterTableOffset != 0;
    }


    std::string_view AppendixEntryList::filename() const noexcept
    {
        return impl_->filename;
    }


    std::string_view AppendixEntryList::title() const noexcept
    {
        return impl_->title;
    }


    Result<AppendixEntry> AppendixEntryList::entryAt(const size_t index) const
    {
        if (index >= impl_->count)
            return std::unexpected(std::format(
                "Appendix entry index {} out of range (size = {})", index, impl_->count));

        return AppendixEntry{
            .entryId = impl_->entryIdAt(index),
            .label = impl_->labelAt(index),
            .firstCharacter = impl_->firstCharacterAt(index),
        };
    }
}
