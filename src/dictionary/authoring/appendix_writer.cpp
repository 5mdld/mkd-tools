//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "dictionary/package/paths.hpp"
#include "dictionary/authoring/appendix_writer.hpp"
#include "dictionary/navigation/appendix_reader.hpp"
#include "dictionary/creation_metadata.hpp"
#include "dictionary/creation_session.hpp"
#include "dictionary/creation_page.hpp"
#include "MKD/resource/resource_store.hpp"
#include "io/write_validation.hpp"
#include <algorithm>
#include <format>

namespace MKD::detail
{
    Result<void> AppendixWriter::write(const ProductBuild& product, const AppendixMenu& menu)
    {
        return AppendixWriter(product).menu(menu, native::paths::appendixInfo);
    }

    Result<void> AppendixWriter::menu(const AppendixMenu& value, std::string_view path)
    {
        if (++depth_ > maximumAppendixMenuDepth)
            return std::unexpected("Appendix menus are excessively nested");

        pugi::xml_document document;
        auto sections = CreationMetadata::plistRoot(document, "array");
        for (const auto& section : value.sections)
        {
            if (auto valid = WriteValidation::xmlText(section.name, true); !valid)
                return valid;

            auto node = sections.append_child("dict");
            CreationMetadata::string(node, metadata::appendix::name, section.name);
            node.append_child("key").text().set(metadata::appendix::children);
            auto children = node.append_child("array");
            for (const auto& entry : section.items)
                if (auto written = item(children.append_child("dict"), entry); !written)
                    return written;
        }
        const auto output = product_.output->path() / native::paths::productContents / WriteValidation::fromUtf8(path);
        std::error_code error;
        if (std::filesystem::exists(output, error))
            return std::unexpected("Duplicate appendix menu path: " + std::string(path));

        if (error)
            return std::unexpected(error.message());

        std::filesystem::create_directories(output.parent_path(), error);
        if (error)
            return std::unexpected(error.message());

        --depth_;
        return CreationMetadata::writePlist(output, document);
    }

    Result<void> AppendixWriter::item(pugi::xml_node node, const AppendixItem& value)
    {
        if (auto valid = WriteValidation::xmlText(value.name, value.title.has_value() && value.number.has_value());
            !valid)
            return valid;

        CreationMetadata::string(node, metadata::appendix::name, value.name);
        const std::array fields{
            std::pair{metadata::appendix::title, &value.title},
            std::pair{metadata::appendix::number, &value.number},
            std::pair{metadata::appendix::description, &value.description},
        };

        for (const auto& [key, text] : fields)
            if (*text)
            {
                if (auto valid = WriteValidation::xmlText(**text); !valid)
                    return valid;

                CreationMetadata::string(node, key, **text);
            }

        if (value.image)
        {
            const auto extension = WriteValidation::fromUtf8(*value.image).extension().string();
            if (auto valid = file(*value.image, extension); !valid)
                return valid;

            CreationMetadata::string(node, metadata::appendix::image, *value.image);
        }

        if (value.indentation)
            CreationMetadata::integer(node, metadata::appendix::indentation, value.indentation);

        return std::visit(
            [&](const auto& destination)
            {
                return target(node, destination);
            },
            value.target);
    }

    Result<void> AppendixWriter::file(std::string_view path, std::string_view extension) const
    {
        if (auto valid = WriteValidation::xmlText(path); !valid)
            return valid;

        auto resolved = resolveProductPath(product_.output->path() / native::paths::productContents, path);
        if (!resolved)
            return std::unexpected(resolved.error());

        if (resolved->extension() != extension)
            return std::unexpected("Invalid appendix file extension: " + std::string(path));

        std::error_code error;
        const auto regular = std::filesystem::is_regular_file(*resolved, error);
        if (error || !regular)
            return std::unexpected("Missing appendix file: " + std::string(path));

        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node node, const AppendixList& value) const
    {
        if (auto valid = file(value.path, ".entries"); !valid)
            return valid;

        const auto path =
            product_.output->path() / native::paths::productContents / WriteValidation::fromUtf8(value.path);
        if (auto list = AppendixEntryList::open(path); !list)
            return std::unexpected(list.error());

        CreationMetadata::string(node, metadata::appendix::contents, value.path);
        CreationMetadata::boolean(node, metadata::appendix::singleSection, value.singleSection);
        CreationMetadata::boolean(node, metadata::appendix::usingAnchorId, value.usingAnchorId);
        CreationMetadata::boolean(node, metadata::appendix::skipThe, value.skipThe);
        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node node, const AppendixDocument& value) const
    {
        const auto extension = WriteValidation::fromUtf8(value.path).extension().string();
        if (extension != ".html" && extension != ".pdf")
            return std::unexpected("Document appendices must be HTML or PDF assets");

        if (auto valid = file(value.path, extension); !valid)
            return valid;

        if (value.fragment)
        {
            if (extension != ".html")
                return std::unexpected("Only HTML appendix documents support fragments");
            if (auto valid = WriteValidation::xmlText(*value.fragment); !valid)
                return valid;
            if (value.fragment->contains('#'))
                return std::unexpected("Invalid appendix document fragment");
        }
        CreationMetadata::string(node, metadata::appendix::contents,
                                 value.path + (value.fragment ? "#" + *value.fragment : ""));
        return {};
    }

    Result<void> AppendixWriter::validateEntry(const ProductBuild& product, const AppendixEntryLink& value)
    {
        const auto dictionary = std::ranges::find_if(product.dictionaries,
                                                     [&](const auto& candidate)
                                                     {
                                                         return candidate->metadata.dictionaryId == value.dictionaryId;
                                                     });
        if (dictionary == product.dictionaries.end())
            return std::unexpected("Appendix entry targets an unknown dictionary");

        if (auto valid = WriteValidation::entryId(value.entryId); !valid)
            return valid;

        auto store =
            ResourceStore::open((*dictionary)->root / native::paths::contents, (*dictionary)->metadata.identifier);
        if (!store)
            return std::unexpected(store.error());

        return validateEntry(*store, value.entryId);
    }

    Result<void> AppendixWriter::validateEntry(const ResourceStore& store, EntryId target)
    {
        if (auto index = store.indexOf(target.pageId); !index)
            return std::unexpected(index.error());

        if (!target.itemId)
            return {};

        auto entry = store.get(target.pageId);
        if (!entry)
            return std::unexpected(entry.error());

        auto xml = entry->utf8();
        if (!xml)
            return std::unexpected(xml.error());

        auto references = PageReferences::read(target.pageId, *xml);
        if (!references)
            return std::unexpected(references.error());

        if (!references->anchors.contains(target))
            return std::unexpected("Missing appendix entry anchor");

        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node node, const AppendixEntryLink& value) const
    {
        if (auto valid = validateEntry(product_, value); !valid)
            return valid;

        CreationMetadata::string(node, metadata::appendix::entry,
                                 value.entryId.toString() + "-" + std::to_string(value.dictionaryId));
        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node node, const AppendixMenu& value)
    {
        const auto path = std::format("{}/menu-{}.plist", native::paths::appendixMenus, ++nextMenu_);
        if (auto written = menu(value, path); !written)
            return written;

        CreationMetadata::string(node, metadata::appendix::contents, path);
        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node node, const AppendixGallery& value)
    {
        pugi::xml_document document;
        auto images = CreationMetadata::plistRoot(document, "array");
        for (const auto& image : value.images)
        {
            const auto extension = WriteValidation::fromUtf8(image).extension().string();
            if (auto valid = file(image, extension); !valid)
                return valid;

            CreationMetadata::string(images.append_child("dict"), metadata::appendix::file, image);
        }
        const auto relative =
            std::format("{}/gallery-{}/{}", native::paths::appendixMenus, ++nextMenu_, native::paths::graphicsInfo);
        const auto path = product_.output->path() / native::paths::productContents / relative;
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error)
            return std::unexpected(error.message());

        if (auto written = CreationMetadata::writePlist(path, document); !written)
            return written;

        CreationMetadata::string(node, metadata::appendix::contents, relative);
        return {};
    }

    Result<void> AppendixWriter::target(pugi::xml_node, const AppendixCustom&)
    {
        return std::unexpected("App-specific appendix presenters cannot be authored");
    }
}
