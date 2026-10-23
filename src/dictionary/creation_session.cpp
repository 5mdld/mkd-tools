//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "creation_session.hpp"
#include "creation_metadata.hpp"
#include "native_format.hpp"
#include "creation_page.hpp"
#include "appendix_writer.hpp"
#include "io/write_validation.hpp"
#include "binary/binary_buffer.hpp"

#include <algorithm>
#include <format>
#include <numeric>

namespace MKD::detail
{
    ProductBuild::ProductBuild(std::unique_ptr<BuildDirectory> output, DictionaryMetadata metadata, std::string name)
        : output(std::move(output)), metadata(std::move(metadata)), name(std::move(name))
    {
    }

    Result<void> ProductBuild::writeAsset(std::string_view relative, std::span<const uint8_t> bytes) const
    {
        if (auto state = output->check(); !state)
            return state;

        if (auto valid = WriteValidation::relativePath(relative); !valid)
            return output->fail(valid.error());

        if (relative == name + ".json" || relative == native::paths::appendixInfo || relative ==
            native::paths::moreInfo)
            return output->fail("Asset path is reserved for generated product metadata");

        const auto path = WriteValidation::fromUtf8(relative);
        if (*path.begin() == native::paths::appendixMenus)
            return output->fail("Asset path is reserved for generated appendix menus");

        for (const auto& dictionary : dictionaries)
            if (*path.begin() == WriteValidation::fromUtf8(dictionary->metadata.directory))
                return output->fail("Use the dictionary writer for content assets");

        DirectoryOutput directory(output->path() / native::paths::productContents, false, false);
        if (auto prepared = directory.prepare(); !prepared)
            return output->fail(prepared.error());

        auto written = directory.writeFile(path, [bytes](OutputFile& file)
        {
            return file.write(bytes);
        });
        if (!written)
            return output->fail(written.error());

        if (written->skipped)
            return output->fail("Duplicate product asset");

        return {};
    }

    Result<void> ProductBuild::writeAppendices() const
    {
        for (const auto& reference : appendixReferences)
            if (auto valid = AppendixWriter::validateEntry(*this, reference); !valid) return valid;
        if (appendices) return AppendixWriter::write(*this, *appendices);
        AppendixMenu menu;
        for (const auto& dictionary : dictionaries)
        {
            if (dictionary->appendices.empty()) continue;
            AppendixSection section{
                dictionary->metadata.title ? dictionary->metadata.title->resolve() : dictionary->metadata.directory, {}
            };
            section.items = dictionary->appendices;
            menu.sections.push_back(std::move(section));
        }
        return menu.sections.empty() ? Result<void>{} : AppendixWriter::write(*this, menu);
    }

    DictionaryBuild::DictionaryBuild(std::shared_ptr<ProductBuild> product, DictionaryContent metadata,
                                     const DictionaryWriteOptions options)
        : product(product), metadata(std::move(metadata)), options(options),
          root(product->output->path() / native::paths::productContents / WriteValidation::fromUtf8(
              this->metadata.directory))
    {
    }

    Result<std::shared_ptr<ProductBuild>> DictionaryBuild::owner() const
    {
        auto owner = product.lock();
        if (!owner) return std::unexpected("Product writer no longer exists");
        if (auto state = owner->output->check(); !state) return std::unexpected(state.error());
        if (finished) return std::unexpected("Dictionary has already finished");
        return owner;
    }

    Result<void> DictionaryBuild::fail(std::string error) const
    {
        const auto owner = product.lock();
        return owner ? owner->output->fail(std::move(error)) : Result<void>(std::unexpected(std::move(error)));
    }

    void DictionaryBuild::recordReferences(const ContentReferences& found, std::string origin)
    {
        references.insert(found.targets.begin(), found.targets.end());
        for (const auto target : found.targets)
            targetOrigins.try_emplace(target, origin);

        assetReferences.insert(found.assets.begin(), found.assets.end());
        gaijiReferences.insert(found.gaijiNames.begin(), found.gaijiNames.end());
        for (const auto& asset : found.assets)
            assetOrigins.try_emplace(asset, origin);

        for (const auto& name : found.gaijiNames)
            assetOrigins.try_emplace(name, origin);
    }

    Result<void> DictionaryBuild::scanHeadline(const HeadlineComponents& headline)
    {
        const auto origin = "headline " + headline.entryId.toString();
        auto found = ContentReferences::headline(headline.full());
        if (!found)
            return std::unexpected(origin + ": " + found.error());

        recordReferences(*found, origin);
        return {};
    }

    Result<void> DictionaryBuild::scanStylesheet(std::string_view path, std::span<const uint8_t> bytes)
    {
        const std::string_view css(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        auto found = CssReferences::read(path, css);
        if (!found)
            return std::unexpected(std::string(path) + ": " + found.error());

        recordReferences({.assets = std::move(*found)}, std::string(path));
        return {};
    }

    Result<void> DictionaryBuild::scanPage(const uint32_t pageId, std::string_view xhtml)
    {
        if (auto valid = WriteValidation::entryId({pageId, 0}); !valid)
            return valid;

        if (!pageIds.insert(pageId).second)
            return std::unexpected("Duplicate page ID");

        auto scanned = PageReferences::read(pageId, xhtml);
        if (!scanned)
            return std::unexpected(scanned.error());

        anchors.insert(scanned->anchors.begin(), scanned->anchors.end());
        recordReferences({.targets = std::move(scanned->targets), .assets = std::move(scanned->assets)},
                         "page " + std::to_string(pageId));
        return {};
    }

    Result<void> DictionaryBuild::validateReferences() const
    {
        if (pageIds.empty())
            return std::unexpected("Dictionary has no pages");

        if (primaryHeadlines.empty())
            return std::unexpected("Dictionary has no primary headlines");

        if (!keys.contains(SearchScope::Headword) || keys.at(SearchScope::Headword).empty())
            return std::unexpected("Dictionary has no headword search keys");

        if (auto valid = validateEntryReferences(); !valid)
            return valid;

        if (auto valid = validateAssetReferences(); !valid)
            return valid;

        return validateOrder();
    }

    Result<void> DictionaryBuild::validateEntryReferences() const
    {
        for (const auto& page : reservedEntries | std::views::keys)
            if (!pageIds.contains(page))
                return std::unexpected("Reserved entry has no XML: page " + std::to_string(page));

        for (const auto target : references)
        {
            if (!anchors.contains(target))
            {
                const auto origin = targetOrigins.find(target);
                return std::unexpected(
                    "Missing entry target " + target.toString() +
                    (origin == targetOrigins.end() ? "" : " referenced by " + origin->second));
            }
        }

        for (const auto page : pageIds)
            if (!primaryHeadlines.contains({page, 0}))
                return std::unexpected("Each page requires a primary headline");

        return {};
    }

    Result<void> DictionaryBuild::validateAssetReferences() const
    {
        for (const auto& path : assetReferences)
        {
            if (assets.contains(path))
                continue;
            if (path.starts_with("graphics/") && graphicNames.contains(path.substr(9)))
                continue;
            if (path.starts_with("audio/") && audioNames.contains(path.substr(6)))
                continue;
            const auto origin = assetOrigins.find(path);
            return std::unexpected(
                "Missing resource " + path + (origin == assetOrigins.end() ? "" : " referenced by " + origin->second));
        }

        for (const auto& name : gaijiReferences)
        {
            const auto base = std::string(native::label::gaijiDirectory) + name;
            if (!assets.contains(base + ".png") && !assets.contains(base + ".svg"))
                return std::unexpected("Missing gaiji " + name + " referenced by " + assetOrigins.at(name));
        }
        if (metadata.presentation.fullRubyStylesheet && !assets.contains(*metadata.presentation.fullRubyStylesheet))
            return std::unexpected("Missing full-ruby stylesheet: " + *metadata.presentation.fullRubyStylesheet);

        return {};
    }

    Result<void> DictionaryBuild::validateOrder() const
    {
        if (!order.empty())
        {
            std::unordered_set ordered(order.begin(), order.end());
            if (ordered != pageIds || ordered.size() != order.size())
                return std::unexpected("Entry order must contain every page exactly once");
        }

        return {};
    }

    Result<void> DictionaryBuild::writeOrder()
    {
        if (order.empty())
        {
            order.assign(pageIds.begin(), pageIds.end());
            std::ranges::sort(order);
        }

        const auto maximum = *std::ranges::max_element(order);
        std::vector positions(maximum, UINT32_MAX);
        for (uint32_t i = 0; i < order.size(); ++i)
            positions[order[i] - 1] = i;

        BinaryBuffer bytes(8 + uint64_t{4} * (positions.size() + order.size()));
        bytes.writeLittle(static_cast<uint32_t>(order.size()));
        bytes.writeLittle(maximum);

        for (const auto position : positions)
            bytes.writeLittle(position);
        for (const auto page : order)
            bytes.writeLittle(page);

        return OutputFile::writeAll(root / native::paths::contents / native::paths::entryOrder, bytes.data());
    }

    Result<void> DictionaryBuild::writeAsset(std::string_view relative, std::span<const uint8_t> bytes)
    {
        if (auto valid = WriteValidation::relativePath(relative); !valid)
            return valid;

        const auto path = WriteValidation::fromUtf8(relative);
        const auto first = *path.begin();

        if (first == native::paths::contents || first == native::paths::keys || first == native::paths::secondaryKeys ||
            first == native::paths::headlines ||
            first == native::paths::audio || first == native::paths::graphics || first == native::paths::fonts || first
            == native::paths::contentInfo)
            return std::unexpected("Asset path is reserved for generated dictionary resources");

        if (relative.ends_with(".css"))
            if (auto scanned = scanStylesheet(relative, bytes); !scanned) return scanned;

        if (!assets.emplace(relative).second)
            return std::unexpected("Duplicate dictionary asset");

        DirectoryOutput directory(root, false, false);
        if (auto prepared = directory.prepare(); !prepared)
            return prepared;

        auto written = directory.writeFile(path, [bytes](OutputFile& file)
        {
            return file.write(bytes);
        });
        if (!written)
            return std::unexpected(written.error());

        if (written->skipped)
            return std::unexpected("Duplicate dictionary asset");

        return {};
    }

    Result<void> DictionaryBuild::writeMedia(std::string_view name, std::span<const uint8_t> bytes, bool isAudio)
    {
        if (auto valid = WriteValidation::relativePath(name); !valid)
            return valid;

        if (name.contains('/'))
            return std::unexpected("Packed media name must be a single path component");

        auto& writer = isAudio ? audio : graphics;
        if (!writer)
        {
            auto created = NamedResourceStoreWriter::create(
                root / (isAudio ? native::paths::audio : native::paths::graphics));
            if (!created)
                return std::unexpected(created.error());

            writer = std::move(*created);
        }

        if (auto written = writer->add(name, bytes); !written)
            return written;

        (isAudio ? audioNames : graphicNames).emplace(name);
        return {};
    }

    Result<void> DictionaryBuild::finish()
    {
        if (auto state = owner(); !state)
            return std::unexpected(state.error());

        if (auto valid = validateReferences(); !valid)
            return fail(valid.error());

        if (auto result = writePassageHeadlines(); !result)
            return fail(result.error());

        if (auto result = pages->finish(); !result)
            return fail(result.error());

        for (auto& writer : headlines | std::views::values)
            if (auto result = writer.finish(); !result)
                return fail(result.error());

        for (auto& writer : keys | std::views::values)
            if (auto result = writer.finish(); !result)
                return fail(result.error());

        if (graphics)
            if (auto result = graphics->finish(); !result)
                return fail(result.error());

        if (audio)
            if (auto result = audio->finish(); !result)
                return fail(result.error());

        if (auto result = writeOrder(); !result)
            return fail(result.error());

        if (auto result = CreationMetadata::writeContent(root, metadata, primaryHeadlines.size(), audioNames.size()); !
            result)
            return fail(result.error());

        finished = true;
        pageIds = decltype(pageIds){};
        reservedEntries = decltype(reservedEntries){};
        passageIndexes.clear();
        anchors = decltype(anchors){};
        references = decltype(references){};
        primaryHeadlines = decltype(primaryHeadlines){};
        assetReferences = decltype(assetReferences){};
        assets = decltype(assets){};
        graphicNames = decltype(graphicNames){};
        audioNames = decltype(audioNames){};
        order = decltype(order){};
        return {};
    }
}
