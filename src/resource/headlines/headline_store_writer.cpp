//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/resource/headline_store_writer.hpp"
#include "binary/binary_buffer.hpp"
#include "io/build_directory.hpp"
#include "io/output_file.hpp"
#include "io/write_validation.hpp"

#include <array>
#include <map>

namespace MKD
{
    struct HeadlineStoreWriter::Impl
    {
        std::unique_ptr<detail::BuildDirectory> build;
        std::map<EntryId, HeadlineComponents> entries;
        size_t count = 0;

        explicit Impl(std::unique_ptr<detail::BuildDirectory> build) : build(std::move(build))
        {
        }

        Result<void> write();
    };

    HeadlineStoreWriter::HeadlineStoreWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    HeadlineStoreWriter::~HeadlineStoreWriter() = default;
    HeadlineStoreWriter::HeadlineStoreWriter(HeadlineStoreWriter&&) noexcept = default;
    HeadlineStoreWriter& HeadlineStoreWriter::operator=(HeadlineStoreWriter&&) noexcept = default;

    Result<HeadlineStoreWriter> HeadlineStoreWriter::create(const std::filesystem::path& path)
    {
        auto build = detail::BuildDirectory::create(path);
        if (!build)
            return std::unexpected(build.error());

        return HeadlineStoreWriter(std::make_unique<Impl>(std::move(*build)));
    }

    Result<void> HeadlineStoreWriter::add(HeadlineComponents headline) const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        for (const auto* text : {&headline.headline, &headline.prefix, &headline.suffix, &headline.sortingHeadline})
            if (auto valid = detail::WriteValidation::text(*text, text != &headline.headline); !valid)
                return impl_->build->fail(valid.error());

        const auto id = headline.entryId;
        if (!impl_->entries.emplace(id, std::move(headline)).second)
            return impl_->build->fail("Duplicate headline entry ID");

        ++impl_->count;
        return {};
    }

    Result<void> HeadlineStoreWriter::Impl::write()
    {
        const uint64_t stringsStart = 24 + uint64_t{24} * entries.size();
        if (stringsStart > UINT32_MAX)
            return std::unexpected("Headline records exceed format limits");

        detail::BinaryBuffer records(stringsStart);
        detail::BinaryBuffer strings;
        strings.writeLittle(uint16_t{0});
        std::map<std::u16string_view, uint32_t, std::less<>> pool;
        records.writePadding(8);
        records.writeLittle(static_cast<uint32_t>(entries.size()));
        records.writeLittle(uint32_t{24});
        records.writeLittle(static_cast<uint32_t>(stringsStart));
        records.writeLittle(uint32_t{24});

        for (const auto& [id, entry] : entries)
        {
            records.writeLittle(id.pageId);
            records.writeLittle(id.itemId);
            records.writeLittle(uint16_t{0});
            for (const auto* text : {&entry.headline, &entry.prefix, &entry.suffix, &entry.sortingHeadline})
            {
                uint32_t offset = 0;
                if (!text->empty())
                {
                    auto [it, inserted] = pool.try_emplace(*text, static_cast<uint32_t>(strings.position()));
                    if (inserted)
                    {
                        if (stringsStart + strings.position() + uint64_t{2} * (text->size() + 1) > UINT32_MAX)
                            return std::unexpected("Headline strings exceed format limits");

                        for (auto unit : *text)
                            strings.writeLittle(static_cast<uint16_t>(unit));

                        strings.writeLittle(uint16_t{0});
                    }
                    offset = it->second;
                }
                records.writeLittle(offset);
            }
        }

        detail::OutputFile file(build->path() / "data", false);
        if (!file.isOpen())
            return std::unexpected("Cannot create headline store");

        if (auto result = file.write(records.data()); !result)
            return result;

        if (auto result = file.write(strings.data()); !result)
            return result;

        return file.finish().transform([](uint64_t)
        {
        });
    }

    Result<void> HeadlineStoreWriter::finish() const
    {
        if (auto state = impl_->build->check(); !state)
            return state;

        if (auto result = impl_->write(); !result)
            return impl_->build->fail(result.error());

        auto published = impl_->build->publishFile("data");
        if (published)
            impl_->entries = decltype(impl_->entries){};

        return published;
    }

    size_t HeadlineStoreWriter::size() const noexcept { return impl_->count; }
    bool HeadlineStoreWriter::empty() const noexcept { return size() == 0; }
}
