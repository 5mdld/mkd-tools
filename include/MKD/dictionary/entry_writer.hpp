//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#pragma once

#include "MKD/resource/entry_id.hpp"
#include "MKD/result.hpp"
#include "MKD/dictionary/authoring_diagnostics.hpp"
#include <memory>
#include <string_view>

namespace MKD
{
    enum class EntryTargetKind
    {
        Idiom,
        Compound,
        Example,
        Sense,
        Section
    };

    /** Reserves a page and its targets. XML is consumed when writeXml() is called. */
    class EntryWriter
    {
    public:
        ~EntryWriter();
        EntryWriter(EntryWriter&&) noexcept;
        EntryWriter& operator=(EntryWriter&&) noexcept;

        [[nodiscard]] EntryId target() const noexcept;
        /** Kanwa sense and example allocation reserves companion IDs, leaving 2047 targets of each kind per page. */
        [[nodiscard]] Result<EntryId> addTarget(EntryTargetKind kind, std::optional<SourceContext> context = {}) const;
        /** Writes once. All nonzero targets must appear as canonical XML IDs. */
        [[nodiscard]] Result<void> writeXml(std::string_view xml, std::optional<SourceContext> context = {}) const;

    private:
        friend class DictionaryWriter;
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit EntryWriter(std::unique_ptr<Impl> impl);
    };
}
