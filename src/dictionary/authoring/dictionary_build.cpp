//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "dictionary_build.hpp"
#include "product_build.hpp"
#include "MKD/dictionary/content_analysis.hpp"
#include "unicode/utf8.hpp"
#include "dictionary/content/native_content.hpp"
#include "dictionary/authoring/metadata_preparation.hpp"
#include "dictionary/package/metadata_writer.hpp"
#include "io/write_validation.hpp"
#include "resource/detail/font_format.hpp"
#include "output/directory_output.hpp"
#include <algorithm>

namespace MKD::detail
{
    DictionaryBuild::DictionaryBuild(std::shared_ptr<ProductBuild> product, DictionaryContent metadata,
                                     DictionaryWriteOptions options)
        : product_(product), metadata_(std::move(metadata)),
          root_(product->path() / native::paths::productContents / WriteValidation::fromUtf8(metadata_.directory)),
          state_(std::in_place_type<Working>, root_, product->profile(), options)
    {
    }

    Result<std::shared_ptr<ProductBuild>> DictionaryBuild::owner() const
    {
        auto product = product_.lock();
        if (!product)
            return std::unexpected("Product writer no longer exists");

        if (auto state = product->check(); !state)
            return std::unexpected(state.error());

        if (finished())
            return std::unexpected("Dictionary has already finished");

        return product;
    }

    void DictionaryBuild::fail(std::string error) const
    {
        if (auto product = product_.lock())
            (void)product->fail(std::move(error));
    }

    Result<uint32_t> DictionaryBuild::reserveEntry(std::optional<uint32_t> page)
    {
        return mutate([&]() -> Result<uint32_t>
        {
            return working().entries.reserve(page);
        });
    }

    Result<EntryId> DictionaryBuild::addTarget(const uint32_t page, const EntryTargetKind kind)
    {
        return mutate([&]() -> Result<EntryId>
        {
            auto target = working().entries.addTarget(page, kind);
            if (target)
                working().references.requireTarget(*target, "allocated target " + target->toString());

            return target;
        });
    }

    Result<void> DictionaryBuild::addPage(const uint32_t page, std::string_view xml)
    {
        return mutate([&]() -> Result<void>
        {
            if (working().entries.reserved(page))
                return std::unexpected("Page is reserved. Write its XML through the entry writer");

            return writePage(page, xml);
        });
    }

    Result<void> DictionaryBuild::writePage(const uint32_t page, std::string_view xml)
    {
        return mutate([&]() -> Result<void>
        {
            auto& work = working();
            if (auto valid = work.entries.recordPage(page); !valid)
                return valid;

            auto analysis = content::analyzeArticle(page, xml);
            if (!analysis)
                return std::unexpected(analysis.error().describe("page " + std::to_string(page), xml));

            work.references.define(analysis->anchors);
            if (auto recorded = work.references.record(analysis->references, "page " + std::to_string(page),
                                                       "page.html", xml);
                !recorded)
                return recorded;

            if (!work.pages)
            {
                auto writer = ResourceStoreWriter::create(root_ / native::paths::contents, metadata_.identifier,
                                                          {.encrypt = work.options.encryptContents});
                if (!writer)
                    return std::unexpected(writer.error());

                work.pages = std::move(*writer);
            }

            auto result = work.pages->add(page, {reinterpret_cast<const uint8_t*>(xml.data()), xml.size()});
            if (result)
                work.entries.written(page);

            return result;
        });
    }

    Result<void> DictionaryBuild::addHeadline(HeadlineComponents headline, const HeadlineRole role)
    {
        return mutate([&]() -> Result<void>
        {
            if (auto valid = WriteValidation::entryId(headline.entryId); !valid)
                return valid;

            const auto origin = "headline " + headline.entryId.toString();
            const auto text = headline.full();
            auto analysis = content::analyzeHeadline(text);
            if (!analysis)
                return std::unexpected(analysis.error().describe(origin, unicode::toUtf8(text)));

            if (!analysis->references.empty())
            {
                const auto utf8 = unicode::toUtf8(text);
                if (auto recorded = working().references.record(analysis->references, origin, {}, utf8); !recorded)
                    return recorded;
            }

            working().references.requireTarget(headline.entryId, origin);
            return working().indexes.addHeadline(std::move(headline), role);
        });
    }

    Result<void> DictionaryBuild::addSearchKey(const SearchScope scope, std::string_view key, const EntryId target,
                                               const SearchKeyOptions options)
    {
        return mutate([&]() -> Result<void>
        {
            auto result = working().indexes.addSearchKey(scope, key, target, options);
            if (result)
                working().references.requireTarget(target, "search key '" + std::string(key) + "'");

            return result;
        });
    }

    Result<void> DictionaryBuild::writeAsset(std::string_view path, std::span<const uint8_t> bytes)
    {
        return mutate([&]() -> Result<void>
        {
            if (auto valid = AssetWriter::validateAssetPath(path); !valid)
                return valid;

            if (path.ends_with(".css"))
            {
                const std::string_view css(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                auto analysis = content::analyzeCss(css);
                if (!analysis)
                    return std::unexpected(analysis.error().describe(path, css));

                if (auto recorded = working().references.record(analysis->references, path, path, css); !recorded)
                    return recorded;
            }
            return working().assets.writeAsset(path, bytes);
        });
    }

    Result<void> DictionaryBuild::writeMedia(std::string_view name, std::span<const uint8_t> bytes,
                                             const MediaKind kind)
    {
        return mutate([&]() -> Result<void>
        {
            return working().assets.writeMedia(name, bytes, kind);
        });
    }

    Result<GaijiResource> DictionaryBuild::addGaiji(std::string_view filename, std::span<const uint8_t> bytes)
    {
        return mutate([&]() -> Result<GaijiResource>
        {
            if (filename.contains('/'))
                return std::unexpected("Gaiji filename must be a single path component");

            const auto path = std::string(native::label::gaijiDirectory) + std::string(filename);
            if (auto valid = AssetWriter::validateGaiji(path, bytes); !valid)
                return std::unexpected(valid.error());

            if (auto written = writeAsset(path, bytes); !written)
                return std::unexpected(written.error());

            return GaijiResource{path};
        });
    }

    Result<void> DictionaryBuild::setPresentation(DictionaryPresentation presentation)
    {
        return mutate([&]() -> Result<void>
        {
            if (auto valid = metadata_preparation::validatePresentation(presentation); !valid)
                return valid;

            metadata_.presentation = std::move(presentation);
            return {};
        });
    }

    Result<void> DictionaryBuild::setEntryOrder(std::span<const uint32_t> pages)
    {
        return mutate([&]() -> Result<void>
        {
            return working().entries.setOrder(pages);
        });
    }

    Result<void> DictionaryBuild::validate() const
    {
        const auto& work = working();
        if (work.entries.pages().empty())
            return std::unexpected("Dictionary has no pages");

        if (!work.indexes.primaryCount())
            return std::unexpected("Dictionary has no primary headlines");

        if (!work.indexes.hasHeadwords())
            return std::unexpected("Dictionary has no headword search keys");

        if (auto valid = work.entries.validateReservations(); !valid)
            return valid;

        if (auto valid = work.references.validateTargets(); !valid)
            return valid;

        for (auto page : work.entries.pages())
            if (!work.indexes.hasPrimary({page, 0}))
                return std::unexpected("Each page requires a primary headline");

        for (const auto& [path, origin] : work.references.requiredAssets())
            if (!work.assets.contains(path))
                return std::unexpected("Missing resource " + path + " referenced by " + origin);

        for (const auto& [name, origin] : work.references.requiredGaiji())
            if (!work.assets.contains("gaiji/" + name + ".png") && !work.assets.contains("gaiji/" + name + ".svg"))
                return std::unexpected("Missing gaiji " + name + " referenced by " + origin);

        const auto& stylesheet = metadata_.presentation.fullRubyStylesheet;
        if (stylesheet && !work.assets.containsLoose(*stylesheet))
            return std::unexpected("Missing full-ruby stylesheet: " + *stylesheet);

        return work.entries.validateOrder();
    }

    Result<void> DictionaryBuild::finish()
    {
        auto product = owner();
        if (!product)
            return std::unexpected(product.error());

        auto result = [&]() -> Result<void>
        {
            auto& work = working();
            if (auto valid = validate(); !valid)
                return valid;
            if (auto written = work.indexes.finish(work.references); !written)
                return written;
            if (auto written = work.pages->finish(); !written)
                return written;
            if (auto written = work.assets.finish(); !written)
                return written;
            if (auto written = work.entries.writeOrder(root_); !written)
                return written;
            if (auto written = metadata_writer::writeContent(root_, metadata_, work.indexes.primaryCount(),
                                                             work.assets.audioCount());
                !written)
                return written;

            Finished completed{
                work.indexes.scopes(), std::move(work.references).takeTargets(),
                std::move(work.appendices)
            };
            // Product-level links need only the sealed target catalogue, not the parsing and writing state.
            state_.emplace<Finished>(std::move(completed));
            return {};
        }();

        if (!result)
            return (*product)->fail(result.error());

        return result;
    }

    Result<void> DictionaryBuild::validateTarget(const EntryId target) const
    {
        if (!finished())
            return std::unexpected("Dictionary has not finished");

        if (auto valid = WriteValidation::entryId(target); !valid)
            return valid;

        if (!std::ranges::binary_search(std::get<Finished>(state_).targets, target))
            return std::unexpected("Missing entry target " + target.toString());

        return {};
    }

    Result<void> DictionaryBuild::addFont(std::string_view filename, std::span<const uint8_t> bytes)
    {
        return mutate([&]() -> Result<void>
        {
            if (auto valid = WriteValidation::relativePath(filename); !valid)
                return valid;

            if (filename.contains('/'))
                return std::unexpected("Font filename must be a single path component");

            const auto extension = WriteValidation::fromUtf8(filename).extension().string();
            if (!FontFormat::supportsExtension(extension))
                return std::unexpected("Font filename must use .ttf, .otf or .ttc");

            const auto format = FontFormat::detect(bytes);
            if (!format || *format != extension)
                return std::unexpected("Font signature must match its filename extension");

            auto product = product_.lock();
            if (product->hasFont(filename))
                return std::unexpected("Font filename must be unique within the product");

            DirectoryOutput fonts(root_ / native::paths::fonts, false, false);
            if (auto ready = fonts.prepare(); !ready)
                return ready;

            auto written = fonts.writeFile(WriteValidation::fromUtf8(filename),
                                           [bytes](OutputFile& file) { return file.write(bytes); });
            if (!written)
                return std::unexpected(written.error());

            if (written->skipped)
                return std::unexpected("Duplicate font filename");

            working().assets.registerLoose(std::string(native::paths::fonts) + "/" + std::string(filename));
            product->registerFont(std::string(filename));
            return {};
        });
    }

    Result<AppendixDocument> DictionaryBuild::addAppendixDocument(std::string_view relative, std::string_view title)
    {
        return mutate([&]() -> Result<AppendixDocument>
        {
            if (auto valid = WriteValidation::relativePath(relative); !valid)
                return std::unexpected(valid.error());

            if (auto valid = WriteValidation::xmlText(title); !valid)
                return std::unexpected(valid.error());

            const auto path = relative.substr(0, relative.find('#'));
            const auto extension = WriteValidation::fromUtf8(path).extension();
            if (extension != ".html" && extension != ".pdf")
                return std::unexpected("Document appendices must be HTML or PDF assets");

            working().references.requireAsset(std::string(path), "appendix " + std::string(title));
            const auto hash = relative.find('#');
            AppendixDocument document{
                metadata_.directory + "/" + std::string(path),
                hash == std::string_view::npos ? std::nullopt : std::optional(std::string(relative.substr(hash + 1)))
            };
            working().appendices.push_back({.name = std::string(title), .target = document});
            return document;
        });
    }

    Result<AppendixList> DictionaryBuild::addAppendixList(std::string_view name, std::string_view title,
                                                          std::span<const AppendixEntry> entries,
                                                          const AppendixListWriteOptions options)
    {
        return mutate([&]() -> Result<AppendixList>
        {
            if (auto valid = WriteValidation::relativePath(name); !valid)
                return std::unexpected(valid.error());

            if (auto valid = WriteValidation::xmlText(title); !valid)
                return std::unexpected(valid.error());

            if (auto valid = WriteValidation::xmlText(name); !valid)
                return std::unexpected(valid.error());

            if (name.contains('/'))
                return std::unexpected("Appendix name must be a single path component");

            const auto relative = "appendix/" + std::string(name) + (name.ends_with(".entries") ? "" : ".entries");
            auto writer = AppendixEntryListWriter::create(root_ / WriteValidation::fromUtf8(relative), options);
            if (!writer)
                return std::unexpected(writer.error());

            for (const auto& entry : entries)
            {
                if (entry.entryId.pageId || entry.entryId.itemId)
                    if (auto valid = WriteValidation::entryId(entry.entryId); !valid)
                        return std::unexpected(valid.error());

                auto target = entry;
                if (!target.dictionaryId && target.entryId.pageId)
                    target.dictionaryId = *metadata_.dictionaryId;

                if (entry.entryId.pageId)
                {
                    if (target.dictionaryId == *metadata_.dictionaryId)
                        working().references.requireTarget(entry.entryId, "appendix " + std::string(title));
                    else
                    {
                        auto product = owner();
                        if (!product)
                            return std::unexpected(product.error());

                        (*product)->referenceAppendix({entry.entryId, target.dictionaryId});
                    }
                }
                if (auto result = writer->add(std::move(target)); !result)
                    return std::unexpected(result.error());
            }
            if (auto result = writer->finish(); !result)
                return std::unexpected(result.error());

            AppendixList list{metadata_.directory + "/" + relative};
            working().appendices.push_back({.name = std::string(title), .target = list});
            working().assets.registerLoose(relative);
            return list;
        });
    }
}
