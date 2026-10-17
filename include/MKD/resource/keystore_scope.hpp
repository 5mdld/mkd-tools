//
// kiwakiwaaにより 2026/04/22 に作成されました。
//

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace MKD
{
    enum class KeystoreScope : uint8_t
    {
        Headword = 0,
        Idiom = 1,
        Example = 2,
        English = 3,
        Gogi = 4,
        Metadata = 5,
        Kanji = 6,
        Collocation = 7,
        CJ = 8,
        JC = 9,
        Fulltext = 10,
        Group = 11,
        CompoundNoun = 100,
        Numeral = 101,

        Sense = Gogi,
        Modern = Metadata,
        Category = Group,
        Compound = CompoundNoun
    };

    namespace detail
    {
        struct KeystoreScopeNames
        {
            KeystoreScope scope;
            std::string_view name;
            std::string_view filename;
            std::string_view legacyFilename;
            bool generic;
        };

        inline constexpr std::array keystoreScopes{
            KeystoreScopeNames{KeystoreScope::Headword, "Headword", "headword", "headword", true},
            KeystoreScopeNames{KeystoreScope::Idiom, "Idiom", "idiom", "idiom", true},
            KeystoreScopeNames{KeystoreScope::Example, "Example", "example", "example", true},
            KeystoreScopeNames{KeystoreScope::English, "English", "english", "english", true},
            KeystoreScopeNames{KeystoreScope::Gogi, "Gogi", "sense", "gogi", true},
            KeystoreScopeNames{KeystoreScope::Metadata, "Modern", "metadata", "metadata", false},
            KeystoreScopeNames{KeystoreScope::Kanji, "Kanji", "kanji", "kanji", true},
            KeystoreScopeNames{KeystoreScope::Collocation, "Collocation", "collocation", "collocation", true},
            KeystoreScopeNames{KeystoreScope::CJ, "CJ", "cj", "cj", false},
            KeystoreScopeNames{KeystoreScope::JC, "JC", "jc", "jc", false},
            KeystoreScopeNames{KeystoreScope::Fulltext, "Full-Text", "fulltext", "fulltext", true},
            KeystoreScopeNames{KeystoreScope::Group, "Group", "category", "group", true},
            KeystoreScopeNames{KeystoreScope::CompoundNoun, "Compound Noun", "compound", "compound_noun", true},
            KeystoreScopeNames{KeystoreScope::Numeral, "Numeral", "numeral", "numeral", true}
        };
    }

    inline constexpr std::string_view KEYSTORE_SCOPE_FILENAME_EXTENSION = ".keystore";

    [[nodiscard]] constexpr std::optional<std::string_view> formatKeystoreScope(const KeystoreScope scope) noexcept
    {
        for (const auto& names : detail::keystoreScopes)
            if (names.scope == scope) return names.legacyFilename;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<std::string_view> keystoreScopeName(const KeystoreScope scope) noexcept
    {
        return formatKeystoreScope(scope);
    }

    [[nodiscard]] constexpr std::optional<KeystoreScope> parseKeystoreScope(const std::string_view value) noexcept
    {
        for (const auto& names : detail::keystoreScopes)
            if (value == names.filename || value == names.legacyFilename) return names.scope;
        if (value == "index" || value == "vocabulary") return KeystoreScope::Headword;
        if (value == "idiom/phrasal verb" || value == "jyukugo" || value == "kanyoku" || value == "phrase")
            return KeystoreScope::Idiom;
        if (value == "yakugo") return KeystoreScope::Gogi;
        if (value == "modern") return KeystoreScope::Metadata;
        if (value == "oyaji") return KeystoreScope::Kanji;
        if (value == "full-text") return KeystoreScope::Fulltext;
        if (value == "compound noun") return KeystoreScope::CompoundNoun;
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<KeystoreScope> parseKeystoreScopeFilename(
        std::string_view filename) noexcept
    {
        if (const auto sep = filename.find_last_of("/\\"); sep != std::string_view::npos)
            filename.remove_prefix(sep + 1);

        if (!filename.ends_with(KEYSTORE_SCOPE_FILENAME_EXTENSION))
            return std::nullopt;

        filename.remove_suffix(KEYSTORE_SCOPE_FILENAME_EXTENSION.size());
        return parseKeystoreScope(filename);
    }

    [[nodiscard]] inline std::optional<std::string> formatKeystoreScopeFilename(const KeystoreScope scope)
    {
        const auto value = formatKeystoreScope(scope);
        if (!value)
            return std::nullopt;

        std::string filename;
        filename.reserve(value->size() + KEYSTORE_SCOPE_FILENAME_EXTENSION.size());
        filename.append(*value);
        filename.append(KEYSTORE_SCOPE_FILENAME_EXTENSION);
        return filename;
    }
}
