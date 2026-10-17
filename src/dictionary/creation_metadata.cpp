//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "creation_metadata.hpp"
#include "native_format.hpp"
#include "native_search.hpp"
#include "io/output_file.hpp"
#include "io/write_validation.hpp"

namespace MKD::detail
{
    Result<NativeSearchScope> NativeSearchScope::forScope(SearchScope scope)
    {
        for (const auto& names : keystoreScopes)
            if (names.generic && names.scope == static_cast<KeystoreScope>(scope))
                return NativeSearchScope{names.name, names.filename};

        return std::unexpected("Search scope requires a specialised native search profile");
    }

    Result<void> CreationMetadata::validateProduct(DictionaryMetadata& metadata,
                                                   const std::filesystem::path& destination)
    {
        const auto nameBytes = destination.stem().generic_u8string();
        const std::string name(reinterpret_cast<const char*>(nameBytes.data()), nameBytes.size());
        if (auto valid = WriteValidation::relativePath(name); !valid)
            return valid;

        if (name.contains('/') || name.starts_with('.'))
            return std::unexpected("Invalid product name");

        const std::string identifier = std::string(native::defaults::productIdentifierPrefix) + name;
        if (auto valid = WriteValidation::xmlText(identifier); !valid)
            return valid;

        if (metadata.productIdentifier.empty())
            metadata.productIdentifier = identifier;

        if (metadata.productIdentifier != identifier)
            return std::unexpected("Native product identifier must be " + identifier);

        if (!metadata.contents.empty())
            return std::unexpected("Add content through ProductWriter::addDictionary()");

        if (!metadata.fonts.empty() || !metadata.searchScopeNames.empty())
            return std::unexpected("Fonts and search scopes are generated from dictionary resources");

        if (metadata.productSearchClass && *metadata.productSearchClass != native::search::defaultClass.name.data())
            return std::unexpected("Specialised native search classes need an authoring profile");

        if (metadata.productDelegateClass)
            return std::unexpected("Specialised native delegates need an authoring profile");

        metadata.productSearchClass = native::search::defaultClass.name.data();
        for (const auto* text : {&metadata.displayName, &metadata.description, &metadata.genre, &metadata.productTitle})
            if (*text)
                for (const auto* language : {&(*text)->en, &(*text)->ja})
                    if (*language)
                        if (auto valid = WriteValidation::xmlText(**language, true); !valid)
                            return valid;

        for (const auto* text : {&metadata.category, &metadata.version, &metadata.requiredDicKitVersion})
            if (*text)
                if (auto valid = WriteValidation::xmlText(**text); !valid)
                    return valid;

        return {};
    }

    Result<void> CreationMetadata::validatePresentation(const DictionaryPresentation& presentation)
    {
        if (presentation.fullRubyStylesheet)
        {
            const auto& path = *presentation.fullRubyStylesheet;
            if (auto valid = WriteValidation::relativePath(path); !valid)
                return valid;
            if (auto valid = WriteValidation::xmlText(path); !valid)
                return valid;
            if (!path.ends_with(".css") || path.find_first_of("?#") != std::string::npos)
                return std::unexpected("Full-ruby stylesheet must be a relative CSS path");
        }
        return {};
    }

    Result<void> CreationMetadata::validateContent(DictionaryContent& content)
    {
        if (auto valid = WriteValidation::xmlText(content.identifier); !valid)
            return valid;

        if (auto valid = WriteValidation::relativePath(content.directory); !valid)
            return valid;

        if (auto valid = WriteValidation::xmlText(content.directory); !valid)
            return valid;

        if (content.directory.contains('/') || content.directory.starts_with('.'))
            return std::unexpected("Content directory must be a single visible path component");

        if (!content.dictionaryId || !*content.dictionaryId)
            return std::unexpected("A nonzero numeric dictionary ID is required");

        if (!content.languages || content.languages->empty())
            return std::unexpected("At least one content language is required");

        for (const auto& language : *content.languages)
            if (auto valid = WriteValidation::xmlText(language); !valid) return valid;

        for (const auto* text : {&content.title, &content.publisher, &content.edition})
            if (*text)
                for (const auto* language : {&(*text)->en, &(*text)->ja})
                    if (*language)
                        if (auto valid = WriteValidation::xmlText(**language, true); !valid)
                            return valid;

        if (content.version)
            if (auto valid = WriteValidation::xmlText(*content.version); !valid)
                return valid;

        if (content.keyNormalization && *content.keyNormalization != native::search::defaultNormalization.name.data())
            return std::unexpected("Creation currently supports the default native key normalisation");

        if (content.hasHeadlineKeyIdentifier.value_or(false))
            return std::unexpected("Headline key identifiers require a dedicated authoring profile");

        if (auto valid = validatePresentation(content.presentation); !valid)
            return valid;

        content.keyNormalization = native::search::defaultNormalization.name.data();
        content.hasHeadlineKeyIdentifier = false;
        return {};
    }

    pugi::xml_node CreationMetadata::plistRoot(pugi::xml_document& document, std::string_view kind)
    {
        auto declaration = document.append_child(pugi::node_declaration);
        declaration.append_attribute("version") = "1.0";
        declaration.append_attribute("encoding") = "UTF-8";
        auto plist = document.append_child("plist");
        plist.append_attribute("version") = "1.0";
        return plist.append_child(std::string(kind).c_str());
    }

    void CreationMetadata::string(pugi::xml_node dictionary, std::string_view key, std::string_view value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child("string").text().set(std::string(value).c_str());
    }

    void CreationMetadata::integer(pugi::xml_node dictionary, std::string_view key, const uint64_t value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child("integer").text().set(std::to_string(value).c_str());
    }

    void CreationMetadata::boolean(pugi::xml_node dictionary, std::string_view key, bool value)
    {
        dictionary.append_child("key").text().set(std::string(key).c_str());
        dictionary.append_child(value ? "true" : "false");
    }

    void CreationMetadata::localized(pugi::xml_node dictionary, std::string_view key,
                                     const std::optional<LocalizedString>& value)
    {
        if (!value)
            return;

        dictionary.append_child("key").text().set(std::string(key).c_str());

        auto languages = dictionary.append_child("dict");
        if (value->en)
            string(languages, metadata::language::english, *value->en);
        if (value->ja)
            string(languages, metadata::language::japanese, *value->ja);
    }

    Result<void> CreationMetadata::writePlist(const std::filesystem::path& path, pugi::xml_document& document)
    {
        OutputFile file(path, false);
        if (!file.isOpen())
            return std::unexpected("Cannot create property list: " + path.string());

        document.save(file.stream(), "  ", pugi::format_default, pugi::encoding_utf8);
        return file.finish().transform([](uint64_t)
        {
        });
    }

    Result<void> CreationMetadata::writeProduct(const std::filesystem::path& root, std::string_view name,
                                                const DictionaryMetadata& metadata)
    {
        std::string json;
        if (const auto error = glz::write_json(metadata, json); error)
            return std::unexpected("Cannot serialise product metadata");

        const auto bytes = std::span(reinterpret_cast<const uint8_t*>(json.data()), json.size());
        const auto path = root / native::paths::productContents / (std::string(name) + ".json");
        if (auto written = OutputFile::writeAll(path, bytes); !written)
            return written;

        pugi::xml_document document;
        auto dictionary = plistRoot(document, "dict");
        string(dictionary, metadata::package::identifier, metadata.productIdentifier);
        string(dictionary, metadata::package::version, metadata.version.value_or(native::defaults::productVersion));
        string(dictionary, metadata::package::platform, native::defaults::platform);
        string(dictionary, metadata::package::releaseType, native::defaults::releaseType);
        string(dictionary, metadata::package::buildVersion, native::defaults::buildVersion);
        return writePlist(root / native::paths::productInfo, document);
    }

    Result<void> CreationMetadata::writeContent(const std::filesystem::path& root, const DictionaryContent& metadata,
                                                const size_t headwords, const size_t audio)
    {
        pugi::xml_document document;
        auto dictionary = plistRoot(document, "dict");
        string(dictionary, metadata::content::identifier, metadata.identifier);
        string(dictionary, metadata::content::directory, metadata.directory);
        integer(dictionary, metadata::content::dictionaryId, *metadata.dictionaryId);
        string(dictionary, metadata::content::version, metadata.version.value_or(native::defaults::contentVersion));
        string(dictionary, metadata::content::keyNormalization,
               metadata.keyNormalization.value_or(native::search::defaultNormalization.name.data()));
        boolean(dictionary, metadata::content::hasHeadlineKeyIdentifier,
                metadata.hasHeadlineKeyIdentifier.value_or(false));

        if (metadata.presentation.fullRubyStylesheet)
            string(dictionary, metadata::content::fullRubyStylesheet, *metadata.presentation.fullRubyStylesheet);

        integer(dictionary, metadata::content::headwordCount, headwords);
        integer(dictionary, metadata::content::audioCount, audio);
        localized(dictionary, metadata::content::title, metadata.title);
        localized(dictionary, metadata::content::publisher, metadata.publisher);
        localized(dictionary, metadata::content::edition, metadata.edition);
        dictionary.append_child("key").text().set(metadata::content::languages);

        auto languages = dictionary.append_child("array");
        for (const auto& language : *metadata.languages)
            languages.append_child("string").text().set(language.c_str());

        return writePlist(root / native::paths::contentInfo, document);
    }
}
