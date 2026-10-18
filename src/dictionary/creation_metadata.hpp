//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#pragma once

#include "MKD/dictionary/product_writer.hpp"
#include "MKD/dictionary/dictionary_search.hpp"
#include <pugixml.hpp>
#include "resource/keystore/scope_definitions.hpp"

namespace MKD::detail
{
    class CreationMetadata
    {
    public:
        static Result<ScopeDefinition> scopeDefinition(SearchScope scope, CreationProfile profile);
        static Result<void> validateProduct(DictionaryMetadata& metadata, const std::filesystem::path& destination,
                                            CreationProfile profile);
        static Result<void> validatePresentation(const DictionaryPresentation& presentation);
        static Result<void> validateContent(DictionaryContent& content);
        static Result<void> writeProduct(const std::filesystem::path& root, std::string_view name,
                                         const DictionaryMetadata& metadata);
        static Result<void> writeContent(const std::filesystem::path& root, const DictionaryContent& metadata,
                                         size_t headwords, size_t audio);
        static Result<void> writePlist(const std::filesystem::path& path, pugi::xml_document& document);
        static pugi::xml_node plistRoot(pugi::xml_document& document, std::string_view kind);
        static void string(pugi::xml_node dictionary, std::string_view key, std::string_view value);
        static void integer(pugi::xml_node dictionary, std::string_view key, uint64_t value);
        static void boolean(pugi::xml_node dictionary, std::string_view key, bool value);
        static void localized(pugi::xml_node dictionary, std::string_view key,
                              const std::optional<LocalizedString>& value);
    };
}
