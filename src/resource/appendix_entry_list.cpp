//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "MKD/resource/appendix_entry_list.hpp"
#include "MKD/platform/mmap_file.hpp"
#include "binary/binary_view.hpp"

#include <format>
#include <vector>

namespace MKD
{
    struct AppendixEntryList::Impl
    {
        struct Layout
        {
            detail::BinaryView data;
            detail::BinaryView entries;
            std::optional<detail::BinaryView> labels;
            std::optional<detail::BinaryView> characters;
            uint32_t rows;
            uint32_t columns;
            size_t entriesEnd;
        };

        static Result<std::optional<detail::BinaryView>> readOptionalTable(
            detail::BinaryView data, uint32_t offset, uint32_t count, size_t entriesEnd);
        static Result<Layout> readLayout(detail::BinaryView data);
        [[nodiscard]] Result<AppendixEntry> readEntry(size_t index) const;
        static Result<void> validateLabels(const Layout& layout);
        static Result<std::unique_ptr<Impl>> read(MappedFile file, std::string filename, std::string title);

        MappedFile file;
        std::string filename;
        std::string title;
        Layout layout;

        Impl(MappedFile file, std::string filename, std::string title, const Layout& layout)
            : file(std::move(file)), filename(std::move(filename)), title(std::move(title)), layout(layout)
        {
        }
    };

    Result<std::optional<detail::BinaryView>> AppendixEntryList::Impl::readOptionalTable(
        detail::BinaryView data, uint32_t offset, uint32_t count, size_t entriesEnd)
    {
        if (offset == 0)
            return std::optional<detail::BinaryView>{};
        if (offset < entriesEnd)
            return std::unexpected("Appendix table overlaps entry records");

        auto table = data.table(offset, count, 4);
        if (!table)
            return std::unexpected(table.error());

        return std::optional(*table);
    }

    Result<AppendixEntryList::Impl::Layout> AppendixEntryList::Impl::readLayout(detail::BinaryView data)
    {
        auto header = data.slice(0, 36);
        if (!header)
            return std::unexpected(header.error());

        const auto count = *header->readLittle<uint32_t>(0);
        auto entries = data.table(36, count, 8);
        if (!entries)
            return std::unexpected(entries.error());

        const size_t entriesEnd = 36 + entries->bytes().size();
        auto labels = readOptionalTable(data, *header->readLittle<uint32_t>(28), count, entriesEnd);
        if (!labels)
            return std::unexpected(labels.error());

        auto characters = readOptionalTable(data, *header->readLittle<uint32_t>(32), count, entriesEnd);
        if (!characters)
            return std::unexpected(characters.error());

        return Layout{
            .data = data,
            .entries = *entries,
            .labels = *labels,
            .characters = *characters,
            .rows = *header->readLittle<uint32_t>(4),
            .columns = *header->readLittle<uint32_t>(8),
            .entriesEnd = entriesEnd,
        };
    }

    Result<void> AppendixEntryList::Impl::validateLabels(const Layout& layout)
    {
        if (!layout.labels)
            return {};

        for (size_t offset = 0; offset < layout.labels->bytes().size(); offset += 4)
        {
            const auto labelOffset = *layout.labels->readLittle<uint32_t>(offset);
            if (labelOffset < layout.entriesEnd)
                return std::unexpected("Appendix label overlaps entry records");
            if (auto label = layout.data.nullTerminatedString(labelOffset); !label)
                return std::unexpected(label.error());
        }
        return {};
    }

    Result<AppendixEntry> AppendixEntryList::Impl::readEntry(size_t index) const
    {
        AppendixEntry entry{
            .entryId = {
                *layout.entries.readLittle<uint32_t>(index * 8),
                *layout.entries.readLittle<uint16_t>(index * 8 + 4)
            },
            .label = std::nullopt,
            .firstCharacter = std::nullopt,
        };

        if (layout.labels)
        {
            const auto offset = *layout.labels->readLittle<uint32_t>(index * 4);
            auto label = layout.data.nullTerminatedString(offset);
            if (!label)
                return std::unexpected(label.error());
            entry.label = *label;
        }

        if (layout.characters)
            entry.firstCharacter = static_cast<char16_t>(*layout.characters->readLittle<uint16_t>(index * 4));
        return entry;
    }

    Result<std::unique_ptr<AppendixEntryList::Impl>> AppendixEntryList::Impl::read(
        MappedFile file, std::string filename, std::string title)
    {
        auto layout = readLayout(detail::BinaryView(file.data()));
        if (!layout)
            return std::unexpected(layout.error());
        if (auto valid = validateLabels(*layout); !valid)
            return std::unexpected(valid.error());

        return std::make_unique<Impl>(std::move(file), std::move(filename), std::move(title), *layout);
    }

    Result<AppendixEntryList> AppendixEntryList::open(const std::filesystem::path& path, std::string title)
    {
        auto file = MappedFile::open(path);
        if (!file)
            return std::unexpected(file.error());
        auto impl = Impl::read(std::move(*file), path.filename().string(), std::move(title));
        if (!impl)
            return std::unexpected(impl.error());

        return AppendixEntryList(std::move(*impl));
    }

    AppendixEntryList::AppendixEntryList(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    AppendixEntryList::~AppendixEntryList() = default;
    AppendixEntryList::AppendixEntryList(AppendixEntryList&&) noexcept = default;
    AppendixEntryList& AppendixEntryList::operator=(AppendixEntryList&&) noexcept = default;
    size_t AppendixEntryList::size() const noexcept { return impl_->layout.entries.bytes().size() / 8; }
    bool AppendixEntryList::empty() const noexcept { return size() == 0; }
    uint32_t AppendixEntryList::rows() const noexcept { return impl_->layout.rows; }
    uint32_t AppendixEntryList::columns() const noexcept { return impl_->layout.columns; }
    bool AppendixEntryList::hasLabels() const noexcept { return impl_->layout.labels.has_value(); }
    bool AppendixEntryList::hasFirstCharacters() const noexcept { return impl_->layout.characters.has_value(); }
    std::string_view AppendixEntryList::filename() const noexcept { return impl_->filename; }
    std::string_view AppendixEntryList::title() const noexcept { return impl_->title; }

    Result<AppendixEntry> AppendixEntryList::entryAt(size_t index) const
    {
        if (index >= size())
            return std::unexpected(std::format("Appendix entry index {} out of range", index));
        return impl_->readEntry(index);
    }
}
