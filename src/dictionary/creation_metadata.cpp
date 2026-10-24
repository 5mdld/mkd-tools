//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "creation_metadata.hpp"
#include "dictionary/authoring/metadata_preparation.hpp"
#include "dictionary/package/metadata_writer.hpp"
#include "dictionary/package/property_list.hpp"
#include "dictionary/search/native_definitions.hpp"

namespace MKD::detail
{

    Result<ScopeDefinition> CreationMetadata::scopeDefinition(SearchScope scope, CreationProfile profile)
    {
        return search::scopeDefinition(scope, profile);
    }
    Result<void> CreationMetadata::validateProduct(DictionaryMetadata& metadata, const std::filesystem::path& destination,
                                                  CreationProfile profile)
    {
        auto prepared = metadata_preparation::prepareProduct(metadata, destination, profile);
        if (!prepared) return std::unexpected(prepared.error());
        metadata = std::move(*prepared);
        return {};
    }
    Result<void> CreationMetadata::validateContent(DictionaryContent& content)
    {
        auto prepared = metadata_preparation::prepareContent(content);
        if (!prepared) return std::unexpected(prepared.error());
        content = std::move(*prepared);
        return {};
    }
    Result<void> CreationMetadata::validatePresentation(const DictionaryPresentation& presentation)
    {
        return metadata_preparation::validatePresentation(presentation);
    }
    Result<void> CreationMetadata::writeProduct(const std::filesystem::path& root, std::string_view name,
                                               const DictionaryMetadata& metadata)
    {
        return metadata_writer::writeProduct(root, name, metadata);
    }
    Result<void> CreationMetadata::writeContent(const std::filesystem::path& root, const DictionaryContent& metadata,
                                               size_t headwords, size_t audio)
    {
        return metadata_writer::writeContent(root, metadata, headwords, audio);
    }
    Result<void> CreationMetadata::writePlist(const std::filesystem::path& path, pugi::xml_document& document)
    {
        return property_list::write(path, document);
    }
    pugi::xml_node CreationMetadata::plistRoot(pugi::xml_document& document, std::string_view kind)
    {
        return property_list::root(document, kind);
    }
    void CreationMetadata::string(pugi::xml_node dictionary, std::string_view key, std::string_view value)
    {
        property_list::string(dictionary, key, value);
    }
    void CreationMetadata::integer(pugi::xml_node dictionary, std::string_view key, uint64_t value)
    {
        property_list::integer(dictionary, key, value);
    }
    void CreationMetadata::boolean(pugi::xml_node dictionary, std::string_view key, bool value)
    {
        property_list::boolean(dictionary, key, value);
    }
    void CreationMetadata::localized(pugi::xml_node dictionary, std::string_view key,
                                    const std::optional<LocalizedString>& value)
    {
        property_list::localized(dictionary, key, value);
    }
}
