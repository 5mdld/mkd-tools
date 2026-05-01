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

    inline constexpr std::string_view KEYSTORE_SCOPE_FILENAME_EXTENSION = ".keystore";

    [[nodiscard]] constexpr std::optional<std::string_view> formatKeystoreScope(const KeystoreScope scope) noexcept
    {
        switch (scope)
        {
            case KeystoreScope::Headword: return "headword";
            case KeystoreScope::Idiom: return "idiom";
            case KeystoreScope::Example: return "example";
            case KeystoreScope::English: return "english";
            case KeystoreScope::Gogi: return "gogi";
            case KeystoreScope::Metadata: return "metadata";
            case KeystoreScope::Kanji: return "kanji";
            case KeystoreScope::Collocation: return "collocation";
            case KeystoreScope::CJ: return "cj";
            case KeystoreScope::JC: return "jc";
            case KeystoreScope::Fulltext: return "fulltext";
            case KeystoreScope::Group: return "group";
            case KeystoreScope::CompoundNoun: return "compound_noun";
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
        if (value == "headword" || value == "index" || value == "vocabulary") return KeystoreScope::Headword;
        if (value == "idiom" || value == "idiom/phrasal verb" || value == "jyukugo"
            || value == "kanyoku" || value == "phrase") return KeystoreScope::Idiom;
        if (value == "example") return KeystoreScope::Example;
        if (value == "english") return KeystoreScope::English;
        if (value == "gogi" || value == "yakugo" || value == "sense") return KeystoreScope::Gogi;
        if (value == "metadata" || value == "modern") return KeystoreScope::Metadata;
        if (value == "kanji" || value == "oyaji") return KeystoreScope::Kanji;
        if (value == "collocation") return KeystoreScope::Collocation;
        if (value == "cj") return KeystoreScope::CJ;
        if (value == "jc") return KeystoreScope::JC;
        if (value == "fulltext" || value == "full-text") return KeystoreScope::Fulltext;
        if (value == "group" || value == "category") return KeystoreScope::Group;
        if (value == "compound noun" || value == "compound_noun" || value == "compound") return KeystoreScope::CompoundNoun;
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
