//
// kiwakiwaaにより 2026/01/15 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/dictionary/metadata.hpp"
#include "MKD/dictionary/paths.hpp"
#include "MKD/platform/dictionary_source.hpp"
#include "MKD/output/export_options.hpp"
#include "MKD/output/resource_exporter.hpp"
#include "MKD/resource/named_resource_store.hpp"
#include "MKD/resource/resource_store.hpp"
#include "MKD/resource/font.hpp"
#include "MKD/resource/keystore.hpp"
#include "MKD/resource/headline_store.hpp"
#include "MKD/resource/stylesheet.hpp"

#include <cstdint>
#include <variant>

namespace MKD
{
    struct DictionaryResources
    {
        std::optional<ResourceStore> entries;
        std::optional<NamedResourceStore> graphics;
        std::optional<std::variant<ResourceStore, NamedResourceStore>> audio;
        std::optional<Stylesheet> stylesheet;
        std::optional<Stylesheet> nightmodeStylesheet;
        std::vector<Font> fonts;
        std::vector<Keystore> keystores;
        std::vector<HeadlineStore> headlines;
    };


    class Dictionary
    {
    public:
        explicit Dictionary(DictionaryContent content, DictionaryResources resources);

        [[nodiscard]] const std::string& id() const noexcept;
        [[nodiscard]] const DictionaryContent& content() const noexcept;

        // returns nullptr if no content resource store is available
        [[nodiscard]] ResourceStore* entries() noexcept;
        [[nodiscard]] const ResourceStore* entries() const noexcept;
        [[nodiscard]] bool hasEntries() const noexcept;

        [[nodiscard]] Result<ResourceStoreItem> entryByIndex(size_t index) const;
        [[nodiscard]] Result<ResourceStoreItem> entryById(uint32_t itemId) const;
        [[nodiscard]] Result<std::string> entryUtf8ByIndex(size_t index) const;
        [[nodiscard]] Result<std::string> entryUtf8ById(uint32_t itemId) const;

        [[nodiscard]] ResourceStore::Iterator entryBegin() const;
        [[nodiscard]] ResourceStore::Iterator entryEnd() const;

        // returns nullptr if no nrsc resources available
        [[nodiscard]] NamedResourceStore* graphics() noexcept;
        [[nodiscard]] const NamedResourceStore* graphics() const noexcept;
        [[nodiscard]] Stylesheet* stylesheet() noexcept;
        [[nodiscard]] const Stylesheet* stylesheet() const noexcept;
        [[nodiscard]] Stylesheet* nightmodeStylesheet() noexcept;
        [[nodiscard]] const Stylesheet* nightmodeStylesheet() const noexcept;
        [[nodiscard]] std::vector<Font>& fonts() noexcept;
        [[nodiscard]] const std::vector<Font>& fonts() const noexcept;
        [[nodiscard]] const std::vector<Keystore>& keystores() const noexcept;

        [[nodiscard]] size_t resourceCount(ResourceType type) const noexcept;

        [[nodiscard]] ExportResult exportWithOptions(const ExportOptions& options) const;

        [[nodiscard]] Result<std::string> headlineForEntryId(const EntryId& entryId) const;

    private:
        [[nodiscard]] Result<ExportResult> exportAudio(const ExportOptions& options) const;
        [[nodiscard]] Result<ExportResult> exportFonts(const ExportOptions& options) const;
        [[nodiscard]] Result<ExportResult> exportKeystores(const ExportOptions& options) const;
        [[nodiscard]] Result<ExportResult> exportHeadlines(const ExportOptions& options) const;

        DictionaryContent content_;
        std::optional<ResourceStore> entries_;
        std::optional<NamedResourceStore> graphics_;
        std::optional<std::variant<ResourceStore, NamedResourceStore>> audio_;
        std::optional<Stylesheet> stylesheet_;
        std::optional<Stylesheet> nightmodeStylesheet_;
        std::vector<Font> fonts_;
        std::vector<Keystore> keystores_;
        std::vector<HeadlineStore> headlines_;
    };
}
