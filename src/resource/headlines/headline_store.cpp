//
// kiwakiwaaにより 2026/02/12 に作成されました。
//

#include "MKD/resource/headline_store.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "binary/binary_view.hpp"

#include <algorithm>
#include <optional>

namespace MKD
{
    std::u16string HeadlineComponents::full() const
    {
        std::u16string result;
        result.reserve(prefix.size() + headline.size() + suffix.size());

        result.append(prefix);
        result.append(headline);
        result.append(suffix);

        return result;
    }


    struct HeadlineStore::Impl
    {
        struct Record
        {
            EntryId id;
            uint32_t headline;
            uint32_t prefix;
            uint32_t suffix;
            uint32_t sorting;
        };

        struct Layout
        {
            detail::BinaryView records;
            detail::BinaryView strings;
            size_t stride;
        };

        MappedFile file;
        std::string filename;
        Layout layout;

        Impl(MappedFile file, std::string filename, const Layout& layout)
            : file(std::move(file)), filename(std::move(filename)), layout(layout)
        {
        }

        static Result<Layout> readLayout(detail::BinaryView data);
        static Result<std::unique_ptr<Impl>> read(MappedFile file, std::string filename);
        [[nodiscard]] Result<void> validateRecords() const;
        [[nodiscard]] Result<detail::BinaryView> stringAt(uint32_t offset) const;
        [[nodiscard]] Result<std::u16string> readString(uint32_t offset) const;
        [[nodiscard]] Record recordAt(size_t index) const;
        [[nodiscard]] Result<HeadlineComponents> components(const Record& record) const;
        [[nodiscard]] std::optional<size_t> find(EntryId id) const;
        [[nodiscard]] size_t size() const noexcept { return layout.records.bytes().size() / layout.stride; }
    };

    Result<HeadlineStore::Impl::Layout> HeadlineStore::Impl::readLayout(detail::BinaryView data)
    {
        auto header = data.slice(0, 24);
        if (!header)
            return std::unexpected("Headline file too small for header");

        const auto count = *header->readLittle<uint32_t>(8);
        const auto recordsOffset = *header->readLittle<uint32_t>(12);
        const auto stringsOffset = *header->readLittle<uint32_t>(16);
        const auto storedStride = *header->readLittle<uint32_t>(20);
        const size_t stride = storedStride == 0 ? 20 : storedStride;
        if (stride < 20 || recordsOffset < 24 || stringsOffset < recordsOffset)
            return std::unexpected("Invalid headline record layout");

        auto recordsRegion = data.slice(recordsOffset, stringsOffset - recordsOffset);
        if (!recordsRegion)
            return std::unexpected(recordsRegion.error());

        auto records = recordsRegion->table(0, count, stride);
        if (!records)
            return std::unexpected(records.error());

        auto strings = data.tail(stringsOffset);
        if (!strings)
            return std::unexpected(strings.error());

        if (strings->bytes().size() % 2 != 0)
            return std::unexpected("Truncated UTF-16 headline string region");

        return Layout{.records = *records, .strings = *strings, .stride = stride};
    }

    Result<detail::BinaryView> HeadlineStore::Impl::stringAt(uint32_t offset) const
    {
        if (offset % 2 != 0)
            return std::unexpected("Unaligned UTF-16 headline string offset");

        auto text = layout.strings.tail(offset);
        if (!text)
            return std::unexpected(text.error());

        const auto bytes = text->bytes();
        for (size_t i = 0; i < bytes.size(); i += 2)
            if (bytes[i] == 0 && bytes[i + 1] == 0)
                return text->slice(0, i);

        return std::unexpected("Headline string is not null-terminated");
    }

    Result<std::u16string> HeadlineStore::Impl::readString(uint32_t offset) const
    {
        auto text = stringAt(offset);
        if (!text)
            return std::unexpected(text.error());

        std::u16string result;
        result.reserve(text->bytes().size() / 2);
        for (size_t i = 0; i < text->bytes().size(); i += 2)
            result.push_back(static_cast<char16_t>(*text->readLittle<uint16_t>(i)));

        return result;
    }

    HeadlineStore::Impl::Record HeadlineStore::Impl::recordAt(size_t index) const
    {
        const auto data = *layout.records.slice(index * layout.stride, layout.stride);
        return Record{
            .id = {*data.readLittle<uint32_t>(0), *data.readLittle<uint16_t>(4)},
            .headline = *data.readLittle<uint32_t>(8),
            .prefix = *data.readLittle<uint32_t>(12),
            .suffix = *data.readLittle<uint32_t>(16),
            .sorting = layout.stride >= 24 ? *data.readLittle<uint32_t>(20) : 0,
        };
    }

    Result<void> HeadlineStore::Impl::validateRecords() const
    {
        std::optional<EntryId> previous;
        for (size_t i = 0; i < size(); ++i)
        {
            const auto record = recordAt(i);
            if (previous && *previous > record.id)
                return std::unexpected("Headline entry IDs are not sorted");

            if (auto text = stringAt(record.headline); !text)
                return std::unexpected(text.error());

            for (auto offset : {record.prefix, record.suffix, record.sorting})
                if (offset != 0)
                    if (auto text = stringAt(offset); !text)
                        return std::unexpected(text.error());

            previous = record.id;
        }
        return {};
    }

    Result<HeadlineComponents> HeadlineStore::Impl::components(const Record& record) const
    {
        auto headline = readString(record.headline);
        if (!headline) return std::unexpected(headline.error());
        auto sorting = readString(record.sorting == 0 ? record.headline : record.sorting);
        if (!sorting) return std::unexpected(sorting.error());
        auto prefix = record.prefix == 0 ? Result<std::u16string>(std::u16string{}) : readString(record.prefix);
        if (!prefix) return std::unexpected(prefix.error());
        auto suffix = record.suffix == 0 ? Result<std::u16string>(std::u16string{}) : readString(record.suffix);
        if (!suffix) return std::unexpected(suffix.error());
        return HeadlineComponents{
            .prefix = std::move(*prefix),
            .headline = std::move(*headline),
            .suffix = std::move(*suffix),
            .sortingHeadline = std::move(*sorting),
            .entryId = record.id,
        };
    }

    std::optional<size_t> HeadlineStore::Impl::find(EntryId id) const
    {
        const auto positions = std::views::iota(size_t{0}, size());
        const auto it = std::ranges::lower_bound(positions, id, {}, [this](size_t i) { return recordAt(i).id; });
        if (it == positions.end() || recordAt(*it).id != id)
            return std::nullopt;
        return *it;
    }

    Result<std::unique_ptr<HeadlineStore::Impl>> HeadlineStore::Impl::read(MappedFile file, std::string filename)
    {
        auto layout = readLayout(detail::BinaryView(file.data()));
        if (!layout)
            return std::unexpected(layout.error());

        auto impl = std::make_unique<Impl>(std::move(file), std::move(filename), *layout);
        if (auto valid = impl->validateRecords(); !valid)
            return std::unexpected(valid.error());

        return impl;
    }

    HeadlineStore::HeadlineStore(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl))
    {
    }

    HeadlineStore::~HeadlineStore() = default;
    HeadlineStore::HeadlineStore(HeadlineStore&&) noexcept = default;
    HeadlineStore& HeadlineStore::operator=(HeadlineStore&&) noexcept = default;

    Result<HeadlineStore> HeadlineStore::open(const std::filesystem::path& path)
    {
        auto file = MappedFile::open(path);
        if (!file)
            return std::unexpected(file.error());

        auto impl = Impl::read(std::move(*file), path.filename().string());
        if (!impl)
            return std::unexpected(impl.error());

        return HeadlineStore(std::move(*impl));
    }

    Result<HeadlineComponents> HeadlineStore::entryAt(size_t index) const
    {
        if (index >= size())
            return std::unexpected("Headline index out of range");
        return impl_->components(impl_->recordAt(index));
    }

    Result<EntryId> HeadlineStore::entryIdAt(size_t index) const
    {
        if (index >= size())
            return std::unexpected("Headline index out of range");
        return impl_->recordAt(index).id;
    }

    Result<std::optional<HeadlineComponents>> HeadlineStore::findComponentsForEntryId(const EntryId& id,
                                                                                     bool allowFallback) const
    {
        auto index = impl_->find(id);
        if (!index && allowFallback && id.itemId != 0) index = impl_->find({id.pageId, 0});
        if (!index) return std::optional<HeadlineComponents>{};
        return impl_->components(impl_->recordAt(*index)).transform([](HeadlineComponents value)
        {
            return std::optional(std::move(value));
        });
    }

    Result<std::optional<std::u16string>> HeadlineStore::findSortingHeadlineForEntryId(const EntryId& id) const
    {
        auto index = impl_->find(id);
        if (!index && id.itemId != 0) index = impl_->find({id.pageId, 0});
        if (!index) return std::optional<std::u16string>{};
        const auto record = impl_->recordAt(*index);
        return impl_->readString(record.sorting == 0 ? record.headline : record.sorting).transform([](std::u16string value)
        {
            return std::optional(std::move(value));
        });
    }

    Result<HeadlineComponents> HeadlineStore::componentsForEntryId(const EntryId& id, bool allowFallback) const
    {
        auto found = findComponentsForEntryId(id, allowFallback);
        if (!found) return std::unexpected(found.error());
        if (!*found) return std::unexpected("Headline not found");
        return std::move(**found);
    }

    Result<std::u16string> HeadlineStore::sortingHeadlineForEntryId(const EntryId& id) const
    {
        auto found = findSortingHeadlineForEntryId(id);
        if (!found) return std::unexpected(found.error());
        if (!*found) return std::unexpected("Headline not found");
        return std::move(**found);
    }

    size_t HeadlineStore::size() const noexcept { return impl_->size(); }
    bool HeadlineStore::empty() const noexcept { return size() == 0; }
    std::string_view HeadlineStore::filename() const noexcept { return impl_->filename; }
}
