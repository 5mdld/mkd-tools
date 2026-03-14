//
// kiwakiwaaにより 2026/03/12 に作成されました。
//

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

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


    constexpr const char* keystoreScopeName(const KeystoreScope scope)
    {
        switch (scope)
        {
            case KeystoreScope::Headword: return "headword";
            case KeystoreScope::Idiom: return "idiom";
            case KeystoreScope::Example: return "example";
            case KeystoreScope::English: return "english";
            case KeystoreScope::Sense: return "sense";
            case KeystoreScope::Collocation: return "collocation";
            case KeystoreScope::Fulltext: return "fulltext";
            case KeystoreScope::Category: return "category";
            case KeystoreScope::Compound: return "compound";
            case KeystoreScope::Numeral: return "numeral";
            default: ;
        }
        std::unreachable();
    }
}
