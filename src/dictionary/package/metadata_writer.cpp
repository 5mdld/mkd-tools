//
// kiwakiwaaにより 2026/10/24 に作成されました。
//

#include "metadata_writer.hpp"
#include "property_list.hpp"
#include "package_format.hpp"
#include "dictionary/search/search_definitions.hpp"
#include "io/output_file.hpp"

namespace MKD::detail::metadata_writer
{
    Result<void> writeProduct(const std::filesystem::path& root, std::string_view name,
                              const DictionaryMetadata& metadata)
    {
        std::string json;
        if (const auto error = glz::write_json(metadata, json); error)
            return std::unexpected("Cannot serialise product metadata");

        const auto bytes = std::span(reinterpret_cast<const uint8_t*>(json.data()), json.size());
        const auto path = root / monokakido::paths::productContents / (std::string(name) + ".json");
        if (auto written = OutputFile::writeAll(path, bytes); !written)
            return written;

        pugi::xml_document document;
        auto dictionary = property_list::root(document, "dict");
        property_list::string(dictionary, metadata::package::identifier, metadata.productIdentifier);
        property_list::string(dictionary, metadata::package::version,
                              metadata.version.value_or(monokakido::defaults::productVersion));
        property_list::string(dictionary, metadata::package::platform, monokakido::defaults::platform);
        property_list::string(dictionary, metadata::package::releaseType, monokakido::defaults::releaseType);
        property_list::string(dictionary, metadata::package::buildVersion, monokakido::defaults::buildVersion);
        property_list::integer(dictionary, metadata::package::mkdPackageVersion, monokakido::defaults::mkdPackageVersion);
        return property_list::write(root / monokakido::paths::productInfo, document);
    }

    Result<void> writeContent(const std::filesystem::path& root, const DictionaryContent& metadata,
                              const size_t headwords, const size_t audio)
    {
        pugi::xml_document document;
        auto dictionary = property_list::root(document, "dict");
        property_list::string(dictionary, metadata::content::identifier, metadata.identifier);
        property_list::string(dictionary, metadata::content::directory, metadata.directory);
        property_list::integer(dictionary, metadata::content::dictionaryId, *metadata.dictionaryId);
        property_list::string(dictionary, metadata::content::version,
                              metadata.version.value_or(monokakido::defaults::contentVersion));
        property_list::string(dictionary, metadata::content::keyNormalization,
                              metadata.keyNormalization.value_or(monokakido::search::defaultNormalization.name.data()));
        property_list::boolean(dictionary, metadata::content::hasHeadlineKeyIdentifier,
                               metadata.hasHeadlineKeyIdentifier.value_or(false));

        if (metadata.presentation.fullRubyStylesheet)
            property_list::string(dictionary, metadata::content::fullRubyStylesheet,
                                  *metadata.presentation.fullRubyStylesheet);

        property_list::integer(dictionary, metadata::content::headwordCount, headwords);
        property_list::integer(dictionary, metadata::content::audioCount, audio);
        property_list::localized(dictionary, metadata::content::title, metadata.title);
        property_list::localized(dictionary, metadata::content::publisher, metadata.publisher);
        property_list::localized(dictionary, metadata::content::edition, metadata.edition);
        dictionary.append_child("key").text().set(metadata::content::languages);

        auto languages = dictionary.append_child("array");
        for (const auto& language : *metadata.languages)
            languages.append_child("string").text().set(language.c_str());

        return property_list::write(root / monokakido::paths::contentInfo, document);
    }
}
