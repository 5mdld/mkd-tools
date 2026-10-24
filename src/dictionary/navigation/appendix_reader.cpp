//
// kiwakiwaaにより 2026/10/18 に作成されました。
//

#include "dictionary/navigation/appendix_reader.hpp"
#include "dictionary/package/native_format.hpp"
#include "io/write_validation.hpp"
#include "dictionary/content/native_content.hpp"
#include "dictionary/package/property_list.hpp"
#include <charconv>

namespace MKD::detail
{
    Result<std::filesystem::path> AppendixReader::resolve(const std::filesystem::path& contents, std::string_view path)
    {
        if (auto valid = WriteValidation::relativePath(path); !valid)
            return std::unexpected(valid.error());

        if (path.find_first_of("?#") != std::string_view::npos)
            return std::unexpected("Appendix paths must not contain query strings or fragments");

        std::error_code error;
        auto resolved = std::filesystem::weakly_canonical(contents / WriteValidation::fromUtf8(path), error);
        if (error)
            return std::unexpected(error.message());

        const auto base = std::filesystem::weakly_canonical(contents, error);
        if (error)
            return std::unexpected(error.message());

        const auto relative = resolved.lexically_relative(base);
        if (relative.empty() || *relative.begin() == "..")
            return std::unexpected("Appendix path escapes the product");

        return resolved;
    }

    Result<AppendixMenu> AppendixReader::open(const std::filesystem::path& contents)
    {
        std::error_code error;
        const auto exists = std::filesystem::exists(contents / native::paths::appendixInfo, error);
        if (error)
            return std::unexpected(error.message());

        if (!exists)
            return AppendixMenu{};

        return AppendixReader(contents).read(native::paths::appendixInfo);
    }

    Result<AppendixMenu> AppendixReader::read(std::string_view relative)
    {
        auto path = resolve(contents_, relative);
        if (!path)
            return std::unexpected(path.error());

        if (reading_.size() >= maximumAppendixMenuDepth || !reading_.insert(*path).second)
            return std::unexpected("Cyclic or excessively nested appendix menu: " + std::string(relative));

        auto document = PropertyList::open(*path, "array");
        if (!document)
            return std::unexpected(document.error());

        const auto array = document->root();

        AppendixMenu result;
        for (auto node : array.children())
        {
            if (std::string_view(node.name()) != "dict")
                return std::unexpected("Invalid appendix section");

            const PropertyListDictionary properties(node);
            AppendixSection section{properties.text(metadata::appendix::name).value_or(""), {}};
            const auto children = properties.value(metadata::appendix::children);
            if (std::string_view(children.name()) != "array")
                return std::unexpected("Appendix section must contain an array of children");

            for (auto child : children.children())
            {
                auto entry = item(child);
                if (!entry)
                    return std::unexpected(entry.error());
                section.items.push_back(std::move(*entry));
            }
            result.sections.push_back(std::move(section));
        }
        reading_.erase(*path);
        return result;
    }

    Result<AppendixItem> AppendixReader::item(pugi::xml_node node)
    {
        if (std::string_view(node.name()) != "dict")
            return std::unexpected("Invalid appendix item");

        auto destination = target(node);
        if (!destination)
            return std::unexpected(destination.error());

        const PropertyListDictionary properties(node);
        return AppendixItem{
            .name = properties.text(metadata::appendix::name).value_or(""),
            .target = std::move(*destination),
            .title = properties.text(metadata::appendix::title),
            .number = properties.text(metadata::appendix::number),
            .description = properties.text(metadata::appendix::description),
            .image = properties.text(metadata::appendix::image),
            .indentation = properties.value(metadata::appendix::indentation).text().as_uint(),
        };
    }

    Result<AppendixTarget> AppendixReader::target(pugi::xml_node node)
    {
        const PropertyListDictionary properties(node);
        if (auto presenter = properties.text(metadata::appendix::presenter))
            return AppendixCustom{
                *presenter, properties.text(metadata::appendix::contents),
                properties.text(metadata::appendix::file),
                properties.flag(metadata::appendix::detail)
            };

        if (auto entry = properties.text(metadata::appendix::entry))
        {
            const auto separator = entry->rfind('-');
            const auto id = NativeEntryReference::parse(entry->substr(0, separator));
            uint16_t dictionary = 0;
            if (!id || separator == std::string::npos)
                return std::unexpected("Invalid appendix entry target: " + *entry);

            const auto parsed =
                std::from_chars(entry->data() + separator + 1, entry->data() + entry->size(), dictionary);
            if (parsed.ec != std::errc{} || parsed.ptr != entry->data() + entry->size() || !dictionary)
                return std::unexpected("Invalid appendix target dictionary: " + *entry);

            return AppendixEntryLink{*id, dictionary};
        }
        const auto contents = properties.text(metadata::appendix::contents);
        if (!contents)
            return std::unexpected("Appendix item has no target");

        const auto hash = contents->find('#');
        const auto path = contents->substr(0, hash);
        const auto fragment = hash == std::string::npos ? std::nullopt : std::optional(contents->substr(hash + 1));
        if (auto resolved = resolve(contents_, path); !resolved)
            return std::unexpected(resolved.error());

        const auto extension = WriteValidation::fromUtf8(path).extension();
        if (extension == ".entries")
            return AppendixList{
                path, properties.flag(metadata::appendix::singleSection),
                properties.flag(metadata::appendix::usingAnchorId),
                properties.flag(metadata::appendix::skipThe)
            };

        if (extension == ".plist" && WriteValidation::fromUtf8(path).filename() != native::paths::graphicsInfo)
        {
            auto menu = read(path);
            if (!menu)
                return std::unexpected(menu.error());

            return std::move(*menu);
        }

        if (extension == ".plist")
        {
            auto images = gallery(path);
            if (!images)
                return std::unexpected(images.error());

            return std::move(*images);
        }

        if (extension == ".html" || extension == ".pdf")
            return AppendixDocument{path, fragment};

        return AppendixCustom{"", *contents, std::nullopt, false};
    }

    Result<AppendixGallery> AppendixReader::gallery(std::string_view relative) const
    {
        auto path = resolve(contents_, relative);
        if (!path)
            return std::unexpected(path.error());

        auto document = PropertyList::open(*path, "array");
        if (!document)
            return std::unexpected(document.error());

        const auto array = document->root();
        AppendixGallery result;
        for (auto node : array.children())
        {
            auto image = PropertyListDictionary(node).text(metadata::appendix::file);
            if (!image)
                return std::unexpected("Appendix gallery item has no image file");

            if (auto resolved = resolve(contents_, *image); !resolved)
                return std::unexpected(resolved.error());

            result.images.push_back(std::move(*image));
        }
        return result;
    }

    std::unordered_map<std::string, std::string> AppendixReader::listTitles(const AppendixMenu& menu)
    {
        std::unordered_map<std::string, std::string> titles;
        for (const auto& section : menu.sections)
            for (const auto& item : section.items)
            {
                if (const auto* list = std::get_if<AppendixList>(&item.target))
                    titles.try_emplace(list->path, item.name);
                else if (const auto* submenu = std::get_if<AppendixMenu>(&item.target))
                    titles.merge(listTitles(*submenu));
            }
        return titles;
    }
}
