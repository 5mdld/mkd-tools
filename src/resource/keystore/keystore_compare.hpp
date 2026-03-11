//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace MKD::detail::keystore
{
    char32_t foldCase(char32_t cp);


    constexpr bool isIgnorable(const char32_t cp)
    {
        return cp == 0x20 || cp == 0x2D; // space or hyphen
    }


    std::u32string normalizeKeyToUTF32(std::string_view s, bool reverse);


    size_t normalizedLength(std::string_view s);


    enum class CompareMode
    {
        Forward, // prefix
        Backward, // suffix
        Length
    };


    /**
     * Compare two UTF-8 dictionary keys
     * @return <0 if a < b, 0 if equal, >0 if a > b
     */
    int compare(std::string_view a, std::string_view b, CompareMode mode);
}