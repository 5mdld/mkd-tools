//
// Public keystore scope helpers.
//

#pragma once

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
        Sense = 4,
        Kanji = 6,
        Collocation = 7,
        Fulltext = 10,
        Category = 11,
        Compound = 100,
        Numeral = 101
    };

    inline constexpr std::string_view KEYSTORE_SCOPE_FILENAME_EXTENSION = ".keystore";

    [[nodiscard]] constexpr std::optional<std::string_view> formatKeystoreScope(const KeystoreScope scope) noexcept
    {
        switch (scope)
        {
            case KeystoreScope::Headword: return "headword";
            case KeystoreScope::Idiom: return "idiom";
            case KeystoreScope::Example: return "example";
            case KeystoreScope::English: return "english";
            case KeystoreScope::Sense: return "sense";
            case KeystoreScope::Kanji: return "kanji";
            case KeystoreScope::Collocation: return "collocation";
            case KeystoreScope::Fulltext: return "fulltext";
            case KeystoreScope::Category: return "category";
            case KeystoreScope::Compound: return "compound";
            case KeystoreScope::Numeral: return "numeral";
        }
        return std::nullopt;
    }

    [[nodiscard]] constexpr std::optional<std::string_view> keystoreScopeName(const KeystoreScope scope) noexcept
    {
        return formatKeystoreScope(scope);
    }

    [[nodiscard]] constexpr std::optional<KeystoreScope> parseKeystoreScope(const std::string_view value) noexcept
    {
        if (value == "headword") return KeystoreScope::Headword;
        if (value == "idiom") return KeystoreScope::Idiom;
        if (value == "example") return KeystoreScope::Example;
        if (value == "english") return KeystoreScope::English;
        if (value == "sense") return KeystoreScope::Sense;
        if (value == "kanji") return KeystoreScope::Kanji;
        if (value == "collocation") return KeystoreScope::Collocation;
        if (value == "fulltext") return KeystoreScope::Fulltext;
        if (value == "category") return KeystoreScope::Category;
        if (value == "compound") return KeystoreScope::Compound;
        if (value == "numeral") return KeystoreScope::Numeral;
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
