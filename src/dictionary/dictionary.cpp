//
// kiwakiwaaにより 2026/01/15 に作成されました。
//


#include "MKD/dictionary/dictionary.hpp"
#include "MKD/resource/resource_loader.hpp"
#include "MKD/output/keystore_exporter.hpp"
#include "MKD/output/headline_exporter.hpp"

#include <utility>

namespace MKD
{
    Dictionary::Dictionary(DictionaryContent content,
                           std::optional<ResourceStore> entries,
                           std::optional<NamedResourceStore> graphics,
                           std::optional<std::variant<ResourceStore, NamedResourceStore>> audio,
                           std::optional<Stylesheet> stylesheet,
                           std::optional<Stylesheet> nightmodeStylesheet,
                           std::vector<Font> fonts,
                           std::vector<Keystore> keystores,
                           std::vector<HeadlineStore> headlines)
        : content_(std::move(content)),
          entries_(std::move(entries)),
          graphics_(std::move(graphics)),
          audio_(std::move(audio)),
          stylesheet_(std::move(stylesheet)),
          nightmodeStylesheet_(std::move(nightmodeStylesheet)),
          fonts_(std::move(fonts)),
          keystores_(std::move(keystores)),
          headlines_(std::move(headlines))
    {
    }


    const std::string& Dictionary::id() const noexcept
    {
        return content_.identifier;
    }


    const DictionaryContent& Dictionary::content() const noexcept
    {
        return content_;
    }


    NamedResourceStore* Dictionary::graphics() noexcept
    {
        if (!graphics_ || graphics_->empty())
            return nullptr;

        return &*graphics_;
    }


    const NamedResourceStore* Dictionary::graphics() const noexcept
    {
        if (!graphics_ || graphics_->empty())
            return nullptr;

        return &*graphics_;
    }

    Stylesheet* Dictionary::stylesheet() noexcept
    {
        return stylesheet_ ? &*stylesheet_ : nullptr;
    }


    const Stylesheet* Dictionary::stylesheet() const noexcept
    {
        return stylesheet_ ? &*stylesheet_ : nullptr;
    }


    Stylesheet* Dictionary::nightmodeStylesheet() noexcept
    {
        return nightmodeStylesheet_ ? &*nightmodeStylesheet_ : nullptr;
    }


    const Stylesheet* Dictionary::nightmodeStylesheet() const noexcept
    {
        return nightmodeStylesheet_ ? &*nightmodeStylesheet_ : nullptr;
    }


    std::vector<Font>& Dictionary::fonts() noexcept
    {
        return fonts_;
    }


    const std::vector<Font>& Dictionary::fonts() const noexcept
    {
        return fonts_;
    }


    size_t Dictionary::resourceCount(const ResourceType type) const noexcept
    {
        switch (type)
        {
            case ResourceType::Audio:
            case ResourceType::AudioLegacy:
                return audio_ ? std::visit([](const auto& r) { return r.size(); }, *audio_) : 0;
            case ResourceType::Contents:
                return entries_ ? entries_->size() : 0;
            case ResourceType::Graphics:
                return graphics_ ? graphics_->size() : 0;
            case ResourceType::Fonts:
                return fonts_.size();
            case ResourceType::Keystores:
                return keystores_.size();
            case ResourceType::Headlines:
            {
                size_t total = 0;
                for (const auto& store : headlines_)
                    total += store.size();
                return total;
            }
        }
        std::unreachable();
    }


    Result<std::string> Dictionary::headlineForEntryId(const EntryId& entryId) const
    {
        if (headlines_.empty())
            return std::unexpected("no headlines");

        const EntryId id{
            .pageId = entryId.pageId,
            .itemId = entryId.itemId,
        };

        const auto compareEntryId = [](const EntryId& lhs, const EntryId& rhs) {
            if (lhs.pageId != rhs.pageId)
                return lhs.pageId < rhs.pageId;
            return lhs.itemId < rhs.itemId;
        };

        const auto equalEntryId = [](const EntryId& lhs, const EntryId& rhs) {
            return lhs.pageId == rhs.pageId && lhs.itemId == rhs.itemId;
        };

        for (const auto& store : headlines_)
        {
            auto it = std::lower_bound(store.begin(), store.end(), id,
                                       [&](const HeadlineComponents& components, const EntryId& target) {
                                           return compareEntryId(components.entryId, target);
                                       });

            if (it != store.end() && equalEntryId((*it).entryId, id))
                return (*it).fullUtf8();
        }

        return std::unexpected("headline not found");
    }


    const std::vector<Keystore>& Dictionary::keystores() const noexcept
    {
        return keystores_;
    }


    ExportResult Dictionary::exportWithOptions(const ExportOptions& options) const
    {
        ExportResult combinedResult;

        const auto shouldExport = [&](const ResourceType type) {
            return options.resources.empty()
                   || std::ranges::find(options.resources, type) != options.resources.end();
        };

        const auto notify = [&](const ExportEvent& event) {
            if (options.progressCallback)
                options.progressCallback(event);
        };

        // Pre-count heavy resource totals for the unified progress bar
        size_t heavyTotal = 0;
        for (const auto type : {ResourceType::Audio, ResourceType::Contents, ResourceType::Graphics})
        {
            if (shouldExport(type))
                heavyTotal += resourceCount(type);
        }

        notify(ExportBeginEvent{.totalItems = heavyTotal});

        const auto runPhase = [&](const ResourceType type, auto exportFn) {
            if (!shouldExport(type))
                return;

            const size_t totalItems = resourceCount(type);
            if (totalItems == 0)
                return;

            notify(PhaseBeginEvent{type, totalItems});

            if (auto result = exportFn())
            {
                notify(PhaseEndEvent{type, *result});
                combinedResult += *result;
            }
            else
            {
                ExportResult failed;
                failed.failed = 1;
                failed.errors.push_back(std::format("Failed to export {}: {}",
                                                    resourceTypeName(type), result.error()));
                notify(PhaseEndEvent{type, failed});
                combinedResult += failed;
            }
        };

        runPhase(ResourceType::Audio, [&] { return exportAudio(options); });
        runPhase(ResourceType::Contents, [&] {
            return ResourceExporter::exportAll(*entries_, options, ResourceType::Contents);
        });
        runPhase(ResourceType::Fonts, [&] { return exportFonts(options); });
        runPhase(ResourceType::Graphics, [&] {
            return ResourceExporter::exportAll(*graphics_, options, ResourceType::Graphics);
        });
        runPhase(ResourceType::Headlines, [&] { return exportHeadlines(options); });
        runPhase(ResourceType::Keystores, [&] { return exportKeystores(options); });

        return combinedResult;
    }


    Result<ExportResult> Dictionary::exportAudio(const ExportOptions& options) const
    {
        return std::visit([&options](const auto& audioResource) {
            return ResourceExporter::exportAll(audioResource, options, ResourceType::Audio);
        }, *audio_);
    }


    Result<ExportResult> Dictionary::exportFonts(const ExportOptions& options) const
    {
        ExportResult combinedResult;
        for (auto& font : fonts_)
        {
            auto result = ResourceExporter::exportFont(font, options);
            if (!result)
                return result;

            combinedResult += *result;
        }
        return combinedResult;
    }


    Result<ExportResult> Dictionary::exportKeystores(const ExportOptions& options) const
    {
        ExportResult combined;
        for (const auto& keystore : keystores_)
        {
            auto result = KeystoreExporter::exportKeystore(keystore, options);
            if (!result)
                return result;

            combined += *result;
        }
        return combined;
    }


    Result<ExportResult> Dictionary::exportHeadlines(const ExportOptions& options) const
    {
        ExportResult combined;
        for (const auto& store : headlines_)
        {
            auto result = HeadlineExporter::exportHeadlines(store, options);
            if (!result)
                return result;

            combined += *result;
        }
        return combined;
    }
}
