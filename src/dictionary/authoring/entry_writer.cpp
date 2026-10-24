//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "MKD/dictionary/entry_writer.hpp"
#include "writer_handles.hpp"

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

    Result<EntryId> EntryWriter::addTarget(const EntryTargetKind kind, std::optional<SourceContext> context) const
    {
        return impl_->dictionary->addTarget(impl_->pageId, kind, context.value_or(impl_->context));
    }

    Result<void> EntryWriter::writeXml(std::string_view xml, std::optional<SourceContext> context) const
    {
        return impl_->dictionary->writePage(impl_->pageId, xml, context.value_or(impl_->context));
    }
}
