//
// kiwakiwaaにより 2026/01/14 に作成されました。
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/dictionary/detail/metadata_keys.hpp"

#include <glaze/glaze.hpp>

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>

namespace fs = std::filesystem;

namespace MKD
{
    enum class LocalizedLanguage
    {
        Japanese,
        English
    };


    struct LocalizedString
    {
        std::optional<std::string> en;
        std::optional<std::string> ja;

        [[nodiscard]] std::string resolve() const;
        [[nodiscard]] std::string resolve(LocalizedLanguage preferred) const;
        [[nodiscard]] bool empty() const noexcept;
    };

    struct DictionaryContent
    {
        std::string identifier;
        std::string directory;
        std::optional<uint16_t> dictionaryId;
        std::optional<std::string> version;
        std::optional<LocalizedString> title;
        std::optional<LocalizedString> publisher;
        std::optional<LocalizedString> edition;
        std::optional<std::vector<std::string>> languages;
        std::optional<std::string> keyNormalization;
        std::optional<bool> hasHeadlineKeyIdentifier;
    };

    struct DictionaryMetadata
    {
        static Result<DictionaryMetadata> loadFromPath(const fs::path& path);

        std::optional<LocalizedString> displayName;
        std::optional<LocalizedString> description;
        std::optional<LocalizedString> genre;
        std::optional<LocalizedString> productTitle;
        std::string productIdentifier;
        std::optional<std::string> category;
        std::optional<std::string> version;
        std::optional<std::string> productSearchClass;
        std::vector<DictionaryContent> contents;
        std::vector<std::string> searchScopeNames;
        std::vector<std::string> fonts;
        std::optional<bool> verticalLayout;
        std::optional<std::string> requiredDicKitVersion;

    private:
        static std::expected<std::string, std::error_code> readTextFile(const fs::path& path);
    };
}

template <>
struct glz::meta<MKD::LocalizedString>
{
    using T = MKD::LocalizedString;
    static constexpr auto value = object(
        MKD::detail::metadata::language::english, &T::en,
        MKD::detail::metadata::language::japanese, &T::ja
    );
};

template <>
struct glz::meta<MKD::DictionaryContent>
{
    using T = MKD::DictionaryContent;
    static constexpr auto value = object(
        MKD::detail::metadata::content::identifier, &T::identifier,
        MKD::detail::metadata::content::directory, &T::directory,
        MKD::detail::metadata::content::dictionaryId, &T::dictionaryId,
        MKD::detail::metadata::content::version, &T::version,
        MKD::detail::metadata::content::title, &T::title,
        MKD::detail::metadata::content::publisher, &T::publisher,
        MKD::detail::metadata::content::edition, &T::edition,
        MKD::detail::metadata::content::languages, &T::languages,
        MKD::detail::metadata::content::keyNormalization, &T::keyNormalization,
        MKD::detail::metadata::content::hasHeadlineKeyIdentifier, &T::hasHeadlineKeyIdentifier
    );
};

template <>
struct glz::meta<MKD::DictionaryMetadata>
{
    using T = MKD::DictionaryMetadata;
    static constexpr auto value = object(
        MKD::detail::metadata::product::displayName, &T::displayName,
        MKD::detail::metadata::product::description, &T::description,
        MKD::detail::metadata::product::genre, &T::genre,
        MKD::detail::metadata::product::title, &T::productTitle,
        MKD::detail::metadata::product::identifier, &T::productIdentifier,
        MKD::detail::metadata::product::category, &T::category,
        MKD::detail::metadata::product::version, &T::version,
        MKD::detail::metadata::product::searchClass, &T::productSearchClass,
        MKD::detail::metadata::product::contents, &T::contents,
        MKD::detail::metadata::product::searchScopeNames, &T::searchScopeNames,
        MKD::detail::metadata::product::fonts, &T::fonts,
        MKD::detail::metadata::product::verticalLayout, &T::verticalLayout,
        MKD::detail::metadata::product::requiredDicKitVersion, &T::requiredDicKitVersion
    );
};
