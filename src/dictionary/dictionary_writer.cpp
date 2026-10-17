//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "MKD/dictionary/dictionary_writer.hpp"
#include "native_format.hpp"
#include "creation_session.hpp"
#include "creation_metadata.hpp"
#include "io/write_validation.hpp"
#include "resource/detail/font_format.hpp"
#include "unicode/normalization.hpp"

#include <algorithm>

namespace MKD
{
    DictionaryWriter::DictionaryWriter(std::unique_ptr<Impl> impl) : impl_(std::move(impl))
    {
    }

    DictionaryWriter::~DictionaryWriter() = default;
    DictionaryWriter::DictionaryWriter(DictionaryWriter&&) noexcept = default;
    DictionaryWriter& DictionaryWriter::operator=(DictionaryWriter&&) noexcept = default;

    Result<void> DictionaryWriter::addPage(const uint32_t pageId, std::string_view xhtml) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto scanned = build.scanPage(pageId, xhtml); !scanned)
                return scanned;

            if (!build.pages)
            {
                auto writer = ResourceStoreWriter::create(
                    build.root / detail::native::paths::contents, build.metadata.identifier,
                    {.encrypt = build.options.encryptContents});
                if (!writer)
                    return std::unexpected(writer.error());

                build.pages = std::move(*writer);
            }
            return build.pages->add(pageId, std::span(reinterpret_cast<const uint8_t*>(xhtml.data()), xhtml.size()));
        });
    }

    Result<void> DictionaryWriter::addHeadline(HeadlineComponents headline, HeadlineRole role) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto valid = detail::WriteValidation::entryId(headline.entryId); !valid)
                return valid;

            const auto& names = detail::native::paths::headlineFiles;
            const auto index = static_cast<size_t>(role);
            if (index >= names.size())
                return std::unexpected("Invalid headline role");

            auto it = build.headlines.find(role);
            if (it == build.headlines.end())
            {
                auto writer = HeadlineStoreWriter::create(build.root / detail::native::paths::headlines / names[index]);
                if (!writer)
                    return std::unexpected(writer.error());

                it = build.headlines.emplace(role, std::move(*writer)).first;
            }

            build.references.insert(headline.entryId);
            if (role == HeadlineRole::Primary)
                build.primaryHeadlines.insert(headline.entryId);

            return it->second.add(std::move(headline));
        });
    }

    Result<void> DictionaryWriter::addSearchKey(SearchScope scope, std::string_view key, const EntryId target,
                                                const SearchKeyOptions options) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto valid = detail::WriteValidation::entryId(target); !valid)
                return valid;

            if (auto valid = detail::WriteValidation::text(key); !valid)
                return valid;

            const auto normalized = detail::unicode::normalizeDictionaryKey(key);
            auto info = detail::NativeSearchScope::forScope(scope);
            if (!info)
                return std::unexpected(info.error());

            auto it = build.keys.find(scope);
            if (it == build.keys.end())
            {
                const auto filename = std::string(info->filename).append(KEYSTORE_SCOPE_FILENAME_EXTENSION);
                auto writer = KeystoreWriter::create(build.root / detail::native::paths::keys / filename);
                if (!writer)
                    return std::unexpected(writer.error());

                it = build.keys.emplace(scope, std::move(*writer)).first;
            }
            build.references.insert(target);
            return it->second.add(normalized, target, options);
        });
    }

    Result<void> DictionaryWriter::addGraphic(std::string_view name, std::span<const uint8_t> bytes) const
    {
        return impl_->write([&](detail::DictionaryBuild& build)
        {
            return build.writeMedia(name, bytes, false);
        });
    }

    Result<void> DictionaryWriter::addAudio(std::string_view name, std::span<const uint8_t> bytes) const
    {
        return impl_->write([&](detail::DictionaryBuild& build)
        {
            return build.writeMedia(name, bytes, true);
        });
    }

    Result<void> DictionaryWriter::addAsset(std::string_view relative, std::span<const uint8_t> bytes) const
    {
        return impl_->write([&](detail::DictionaryBuild& build)
        {
            return build.writeAsset(relative, bytes);
        });
    }

    Result<void> DictionaryWriter::addFont(std::string_view filename, std::span<const uint8_t> bytes) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto valid = detail::WriteValidation::relativePath(filename); !valid)
                return valid;

            if (filename.contains('/'))
                return std::unexpected("Font filename must be a single path component");

            const auto extension = detail::WriteValidation::fromUtf8(filename).extension().string();
            if (!detail::FontFormat::supportsExtension(extension))
                return std::unexpected("Font filename must use .ttf, .otf or .ttc");

            const auto format = detail::FontFormat::detect(bytes);
            if (!format || *format != extension)
                return std::unexpected("Font signature must match its filename extension");

            auto product = build.product.lock();
            if (std::ranges::find(product->metadata.fonts, filename) != product->metadata.fonts.end())
                return std::unexpected("Font filename must be unique within the product");

            detail::DirectoryOutput fonts(build.root / detail::native::paths::fonts, false, false);
            if (auto ready = fonts.prepare(); !ready)
                return ready;

            auto written = fonts.writeFile(detail::WriteValidation::fromUtf8(filename),
                                           [bytes](detail::OutputFile& file) { return file.write(bytes); });
            if (!written)
                return std::unexpected(written.error());

            if (written->skipped)
                return std::unexpected("Duplicate font filename");

            product->metadata.fonts.emplace_back(filename);
            return {};
        });
    }

    Result<void> DictionaryWriter::addAppendix(std::string_view relative, std::string_view title) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto valid = detail::WriteValidation::relativePath(relative); !valid)
                return valid;

            if (auto valid = detail::WriteValidation::xmlText(title); !valid)
                return valid;

            const auto extension = detail::WriteValidation::fromUtf8(relative).extension();
            if (extension != ".html" && extension != ".xhtml" && extension != ".pdf")
                return std::unexpected("Document appendices must be HTML, XHTML or PDF assets");

            build.assetReferences.emplace(relative);
            build.appendices.push_back({std::string(relative), std::string(title)});
            return {};
        });
    }

    Result<void> DictionaryWriter::addAppendix(std::string_view name, std::string_view title,
                                               std::span<const AppendixEntry> entries,
                                               const AppendixListWriteOptions options) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (auto valid = detail::WriteValidation::relativePath(name); !valid)
                return valid;

            if (auto valid = detail::WriteValidation::xmlText(title); !valid)
                return valid;

            if (auto valid = detail::WriteValidation::xmlText(name); !valid)
                return valid;

            if (name.contains('/'))
                return std::unexpected("Appendix name must be a single path component");

            const auto relative = "appendix/" + std::string(name) + (name.ends_with(".entries") ? "" : ".entries");
            auto writer = AppendixEntryListWriter::create(build.root / detail::WriteValidation::fromUtf8(relative),
                                                          options);
            if (!writer)
                return std::unexpected(writer.error());

            for (const auto& entry : entries)
            {
                if (entry.entryId.pageId || entry.entryId.itemId)
                    if (auto valid = detail::WriteValidation::entryId(entry.entryId); !valid)
                        return valid;

                if (auto result = writer->add(entry); !result)
                    return result;

                if (entry.entryId.pageId) build.references.insert(entry.entryId);
            }
            if (auto result = writer->finish(); !result)
                return result;

            build.appendices.push_back({relative, std::string(title)});
            build.assets.insert(relative);
            return {};
        });
    }

    Result<void> DictionaryWriter::setEntryOrder(std::span<const uint32_t> pageIds) const
    {
        return impl_->write([&](detail::DictionaryBuild& build) -> Result<void>
        {
            if (pageIds.empty())
                return std::unexpected("Explicit entry order must not be empty");

            build.order.assign(pageIds.begin(), pageIds.end());
            return {};
        });
    }

    Result<void> DictionaryWriter::finish() const
    {
        return impl_->dictionary->finish();
    }
}
