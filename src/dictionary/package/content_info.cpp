//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "dictionary/package/content_info.hpp"
#include "dictionary/package/package_format.hpp"
#include "dictionary/search/search_definitions.hpp"
#include "dictionary/package/property_list.hpp"

#include <algorithm>

namespace MKD::detail
{
    Result<void> ContentInfo::loadContent(const std::filesystem::path& directory, DictionaryContent& content,
                                          DictionarySearchConfiguration& search)
    {
        std::error_code error;
        const auto path = directory / native::paths::contentInfo;
        if (!std::filesystem::exists(path, error))
            return error ? Result<void>(std::unexpected(error.message())) : Result<void>{};

        auto plist = PropertyList::open(path, "dict");
        if (!plist)
            return std::unexpected(plist.error());

        if (const auto node = plist->dictionary().value(metadata::content::fullRubyStylesheet))
        {
            if (std::string_view(node.name()) != "string")
                return std::unexpected("Invalid full-ruby stylesheet path");
            content.presentation.fullRubyStylesheet = node.child_value();
        }
        if (const auto node = plist->dictionary().value(metadata::content::type))
        {
            if (std::string_view(node.name()) != "string")
                return std::unexpected("Invalid dictionary content type");

            search.allowsScopeFallback = std::string_view(node.child_value()) == native::contentTypes::dictionary;
        }
        if (const auto node = plist->dictionary().value(metadata::content::keyNormalization))
        {
            if (std::string_view(node.name()) != "string")
                return std::unexpected("Invalid key normalisation profile");

            content.keyNormalization = node.child_value();
        }
        if (const auto node = plist->dictionary().value(metadata::content::hasHeadlineKeyIdentifier))
        {
            const std::string_view type = node.name();
            if (type != "true" && type != "false")
                return std::unexpected("Invalid headline-key identifier flag");

            content.hasHeadlineKeyIdentifier = type == "true";
        }
        return {};
    }

    Result<void> ContentInfo::loadModernRange(const std::filesystem::path& directory,
                                              DictionarySearchConfiguration& search)
    {
        std::error_code error;
        if (!std::filesystem::is_directory(directory, error))
            return error && error != std::errc::no_such_file_or_directory
                       ? Result<void>(std::unexpected(error.message()))
                       : Result<void>{};

        for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
        {
            if (!it->path().filename().generic_u8string().ends_with(native::modernEntries::filenameSuffix))
                continue;

            if (search.modernEntryRange)
                return std::unexpected("Ambiguous modern-entry range property lists");

            auto plist = PropertyList::open(it->path(), "dict");
            if (!plist)
                return std::unexpected(plist.error());

            auto minimum = plist->dictionary().integer(native::modernEntries::minimumKey);
            auto maximum = plist->dictionary().integer(native::modernEntries::maximumKey);
            if (!minimum || !maximum || *minimum > *maximum)
                return std::unexpected("Invalid modern-entry range");

            search.modernEntryRange = DictionarySearchConfiguration::EntryRange{*minimum, *maximum};
        }

        if (error)
            return std::unexpected(error.message());

        return {};
    }

    Result<void> ContentInfo::load(const std::filesystem::path& directory, DictionaryContent& content,
                                   DictionarySearchConfiguration& search)
    {
        if (auto loaded = loadContent(directory, content, search); !loaded)
            return loaded;

        const auto name = search.productSearchClass.value_or(native::search::defaultClass.name.data());
        const auto definition =
            std::ranges::find(native::search::classes, name, &native::search::ClassDefinition::name);
        if (definition != native::search::classes.end() && definition->usesModernEntryRange)
            return loadModernRange(directory, search);

        return {};
    }
}
