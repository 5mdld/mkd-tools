//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/metadata.hpp"
#include "MKD/dictionary/inline_text.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include "MKD/resource/headline_store.hpp"
#include "MKD/resource/keystore_writer.hpp"
#include "MKD/resource/appendix_entry_list_writer.hpp"

namespace MKD
{
    enum class HeadlineRole { Primary, Secondary, Short };

    struct DictionaryWriteOptions
    {
        bool encryptContents = false;
    };

    /** A dictionary build handle. Its ProductWriter must remain alive until finish(). */
    class DictionaryWriter
    {
    public:
        ~DictionaryWriter();
        DictionaryWriter(DictionaryWriter&&) noexcept;
        DictionaryWriter& operator=(DictionaryWriter&&) noexcept;

        [[nodiscard]] Result<void> addPage(uint32_t pageId, std::string_view xhtml) const;
        [[nodiscard]] Result<void> addHeadline(HeadlineComponents headline, HeadlineRole role = HeadlineRole::Primary) const;
        [[nodiscard]] Result<void> addSearchKey(SearchScope scope, std::string_view key, EntryId target,
                                                SearchKeyOptions options = {}) const;
        /** Registers PNG or SVG bytes under gaiji/. No image decoding is performed. */
        [[nodiscard]] Result<GaijiResource> addGaiji(std::string_view filename, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> addGraphic(std::string_view name, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> addAudio(std::string_view name, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> addAsset(std::string_view relativePath, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> addFont(std::string_view filename, std::span<const uint8_t> bytes) const;
        [[nodiscard]] Result<void> addAppendix(std::string_view relativePath, std::string_view title) const;
        [[nodiscard]] Result<void> addAppendix(std::string_view name, std::string_view title,
                                               std::span<const AppendixEntry> entries,
                                               AppendixListWriteOptions options = {}) const;
        [[nodiscard]] Result<void> setEntryOrder(std::span<const uint32_t> pageIds) const;
        [[nodiscard]] Result<void> finish() const;

    private:
        friend class ProductWriter;
        struct Impl;
        std::unique_ptr<Impl> impl_;
        explicit DictionaryWriter(std::unique_ptr<Impl> impl);
    };
}
