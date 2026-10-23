//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "MKD/dictionary/entry_writer.hpp"
#include "creation_session.hpp"
#include "native_content.hpp"
#include "io/write_validation.hpp"

namespace MKD
{
    EntryWriter::EntryWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    EntryWriter::~EntryWriter() = default;
    EntryWriter::EntryWriter(EntryWriter&&) noexcept = default;
    EntryWriter& EntryWriter::operator=(EntryWriter&&) noexcept = default;

    EntryId EntryWriter::target() const noexcept
    {
        return {impl_->pageId, 0};
    }

    Result<EntryId> EntryWriter::addTarget(const EntryTargetKind kind) const
    {
        return impl_->writer.write([&](detail::DictionaryBuild& build)
        {
            return build.addTarget(impl_->pageId, kind);
        });
    }

    Result<void> EntryWriter::writeXml(std::string_view xml) const
    {
        return impl_->writer.write([&](detail::DictionaryBuild& build)
        {
            return build.writePage(impl_->pageId, xml);
        });
    }
}

namespace MKD::detail
{
    Result<uint32_t> DictionaryBuild::reserveEntry(std::optional<uint32_t> requested)
    {
        if (!requested)
            while (pageIds.contains(nextPageId) || reservedEntries.contains(nextPageId))
                ++nextPageId;

        const auto page = requested.value_or(nextPageId);
        if (auto valid = WriteValidation::entryId({page, 0}); !valid)
            return std::unexpected(valid.error());

        if (pageIds.contains(page) || reservedEntries.contains(page))
            return std::unexpected("Page ID is already written or reserved: " + std::to_string(page));

        reservedEntries.emplace(page, std::array<uint32_t, native::entry::targetRanges.size()>{});
        return page;
    }

    Result<EntryId> DictionaryBuild::addTarget(const uint32_t pageId, EntryTargetKind kind)
    {
        if (pageIds.contains(pageId))
            return std::unexpected("Cannot allocate targets after writing entry XML");

        const auto index = static_cast<size_t>(kind);
        if (index >= native::entry::targetRanges.size())
            return std::unexpected("Invalid entry target kind");

        auto range = native::entry::targetRanges[index];
        if (product.lock()->profile == CreationProfile::Kanwa &&
            (kind == EntryTargetKind::Sense || kind == EntryTargetKind::Example))
            range.last -= native::search::normalizedHeadlineOffset;

        auto& count = reservedEntries.at(pageId)[index];
        const auto item = range.first + count;
        if (item > range.last)
            return std::unexpected("Entry target range exhausted on page " + std::to_string(pageId));

        ++count;
        const EntryId target{pageId, static_cast<uint16_t>(item)};
        references.insert(target);
        return target;
    }

    Result<void> DictionaryBuild::writePage(const uint32_t pageId, std::string_view xml)
    {
        if (auto scanned = scanPage(pageId, xml); !scanned)
            return scanned;

        if (!pages)
        {
            auto writer = ResourceStoreWriter::create(
                root / native::paths::contents, metadata.identifier,
                {.encrypt = options.encryptContents});
            if (!writer)
                return std::unexpected(writer.error());

            pages = std::move(*writer);
        }
        auto result = pages->add(pageId, std::span(reinterpret_cast<const uint8_t*>(xml.data()), xml.size()));
        if (result)
            reservedEntries.erase(pageId);
        return result;
    }
}
