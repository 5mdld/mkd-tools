//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/resource/appendix_entry_list_writer.hpp"
#include "binary/binary_buffer.hpp"
#include "io/build_directory.hpp"
#include "io/output_file.hpp"
#include "io/write_validation.hpp"

#include <algorithm>

namespace MKD
{
    struct AppendixEntryListWriter::Impl
    {
        struct Entry
        {
            EntryId id;
            std::optional<std::string> label;
            std::optional<char16_t> firstCharacter;
        };

        std::unique_ptr<detail::BuildDirectory> build;
        AppendixListWriteOptions options;
        std::vector<Entry> entries;

        Impl(std::unique_ptr<detail::BuildDirectory> build, const AppendixListWriteOptions options)
            : build(std::move(build)), options(options)
        {
        }

        Result<void> write();
    };

    AppendixEntryListWriter::AppendixEntryListWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    AppendixEntryListWriter::~AppendixEntryListWriter() = default;
    AppendixEntryListWriter::AppendixEntryListWriter(AppendixEntryListWriter&&) noexcept = default;
    AppendixEntryListWriter& AppendixEntryListWriter::operator=(AppendixEntryListWriter&&) noexcept = default;

    Result<AppendixEntryListWriter> AppendixEntryListWriter::create(const std::filesystem::path& path,
                                                                    AppendixListWriteOptions options)
    {
        if (!options.columns)
            return std::unexpected("Appendix columns must be nonzero");

        auto build = detail::BuildDirectory::create(path);
        if (!build)
            return std::unexpected(build.error());

        return AppendixEntryListWriter(std::make_unique<Impl>(std::move(*build), options));
    }

    Result<void> AppendixEntryListWriter::add(const AppendixEntry& entry) const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (entry.label)
            if (auto valid = detail::WriteValidation::text(*entry.label, true); !valid)
                return impl_->build->fail(
                    valid.error());

        if (entry.firstCharacter)
            if (auto valid = detail::WriteValidation::text(std::u16string_view(&*entry.firstCharacter, 1), true); !
                valid)
                return impl_->build->fail(valid.error());

        impl_->entries.push_back({
            entry.entryId, entry.label ? std::optional(std::string(*entry.label)) : std::nullopt,
            entry.firstCharacter
        });
        return {};
    }

    Result<void> AppendixEntryListWriter::Impl::write()
    {
        const auto count = entries.size();
        const uint64_t rows = options.rows ? options.rows : count / options.columns;
        if (rows * options.columns != count)
            return std::unexpected("Appendix dimensions do not match the entry count");

        const bool labels = std::ranges::any_of(entries, [](const auto& entry)
        {
            return entry.label.has_value();
        });
        const bool characters = std::ranges::any_of(entries, [](const auto& entry)
        {
            return entry.firstCharacter.has_value();
        });

        const uint64_t labelsOffset = 36 + uint64_t{8} * count;
        const uint64_t charactersOffset = labelsOffset + (labels ? uint64_t{4} * count : 0);
        uint64_t stringOffset = charactersOffset + (characters ? uint64_t{4} * count : 0);
        uint64_t total = stringOffset;

        if (labels)
            for (const auto& entry : entries)
                total += entry.label ? entry.label->size() + 1 : 1;

        if (total > UINT32_MAX)
            return std::unexpected("Appendix list exceeds format limits");

        detail::BinaryBuffer output(static_cast<size_t>(total));
        output.writeLittle(static_cast<uint32_t>(count));
        output.writeLittle(static_cast<uint32_t>(rows));
        output.writeLittle(options.columns);
        output.writePadding(16);
        output.writeLittle(static_cast<uint32_t>(labels ? labelsOffset : 0));
        output.writeLittle(static_cast<uint32_t>(characters ? charactersOffset : 0));

        for (const auto& entry : entries)
        {
            output.writeLittle(entry.id.pageId);
            output.writeLittle(entry.id.itemId);
            output.writeLittle(uint16_t{0});
        }

        if (labels)
        {
            for (const auto& entry : entries)
            {
                output.writeLittle(static_cast<uint32_t>(stringOffset));
                stringOffset += entry.label ? entry.label->size() + 1 : 1;
            }
        }

        if (characters)
            for (const auto& entry : entries)
                output.writeLittle(static_cast<uint32_t>(entry.firstCharacter.value_or(0)));

        if (labels)
        {
            for (const auto& entry : entries)
            {
                if (entry.label)
                    output.writeBytes(std::span(reinterpret_cast<const uint8_t*>(entry.label->data()),
                                                entry.label->size()));
                output.writeByte(0);
            }
        }

        return detail::OutputFile::writeAll(build->path() / "data", output.data());
    }

    Result<void> AppendixEntryListWriter::finish() const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto result = impl_->write(); !result)
            return impl_->build->fail(result.error());

        return impl_->build->publishFile("data");
    }

    size_t AppendixEntryListWriter::size() const noexcept { return impl_->entries.size(); }
    bool AppendixEntryListWriter::empty() const noexcept { return size() == 0; }
}
