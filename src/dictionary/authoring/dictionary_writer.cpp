//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/dictionary/dictionary_writer.hpp"
#include "writer_handles.hpp"

namespace MKD
{
    DictionaryWriter::DictionaryWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    DictionaryWriter::~DictionaryWriter() = default;
    DictionaryWriter::DictionaryWriter(DictionaryWriter&&) noexcept = default;
    DictionaryWriter& DictionaryWriter::operator=(DictionaryWriter&&) noexcept = default;

    Result<EntryWriter> DictionaryWriter::createEntry(const std::optional<uint32_t> pageId) const
    {
        auto page = impl_->dictionary->reserveEntry(pageId);
        if (!page)
            return std::unexpected(page.error());

        return EntryWriter(std::make_unique<EntryWriter::Impl>(EntryWriter::Impl{impl_->dictionary, *page}));
    }

    Result<void> DictionaryWriter::addPage(const uint32_t pageId, std::string_view xhtml) const
    {
        return impl_->dictionary->addPage(pageId, xhtml);
    }

    Result<void> DictionaryWriter::addHeadline(HeadlineComponents headline, const HeadlineRole role) const
    {
        return impl_->dictionary->addHeadline(std::move(headline), role);
    }

    Result<void> DictionaryWriter::addSearchKey(const SearchScope scope, std::string_view key, const EntryId target,
                                                const SearchKeyOptions options) const
    {
        return impl_->dictionary->addSearchKey(scope, key, target, options);
    }

    Result<GaijiResource> DictionaryWriter::addGaiji(std::string_view filename, std::span<const uint8_t> bytes) const
    {
        return impl_->dictionary->addGaiji(filename, bytes);
    }

    Result<void> DictionaryWriter::setPresentation(DictionaryPresentation presentation) const
    {
        return impl_->dictionary->setPresentation(std::move(presentation));
    }

    Result<void> DictionaryWriter::addGraphic(std::string_view name, std::span<const uint8_t> bytes) const
    {
        return impl_->dictionary->writeMedia(name, bytes, detail::MediaKind::Graphic);
    }

    Result<void> DictionaryWriter::addAudio(std::string_view name, std::span<const uint8_t> bytes) const
    {
        return impl_->dictionary->writeMedia(name, bytes, detail::MediaKind::Audio);
    }

    Result<void> DictionaryWriter::addAsset(std::string_view path, std::span<const uint8_t> bytes) const
    {
        return impl_->dictionary->writeAsset(path, bytes);
    }

    Result<void> DictionaryWriter::addFont(std::string_view filename, std::span<const uint8_t> bytes) const
    {
        return impl_->dictionary->addFont(filename, bytes);
    }

    Result<AppendixDocument> DictionaryWriter::addAppendix(std::string_view relative, std::string_view title) const
    {
        return impl_->dictionary->addAppendixDocument(relative, title);
    }

    Result<AppendixList> DictionaryWriter::addAppendix(std::string_view name, std::string_view title,
                                                       std::span<const AppendixEntry> entries,
                                                       const AppendixListWriteOptions options) const
    {
        return impl_->dictionary->addAppendixList(name, title, entries, options);
    }

    Result<void> DictionaryWriter::setEntryOrder(std::span<const uint32_t> pages) const
    {
        return impl_->dictionary->setEntryOrder(pages);
    }

    Result<void> DictionaryWriter::finish() const
    {
        return impl_->dictionary->finish();
    }
}
