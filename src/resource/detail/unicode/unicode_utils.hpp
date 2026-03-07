//
// kiwakiwaaにより 2026/03/07 に作成されました。
//

#pragma once

#include "utf8.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace MKD::detail::unicode
{
    inline size_t utf8CodepointCount(const std::string& s)
    {
        return utf8::distance(s.begin(), s.end());
    }

    inline std::u16string toUtf16(const std::string& s)
    {
        std::u16string result;
        utf8::utf8to16(s.begin(), s.end(), std::back_inserter(result));
        return result;
    }

    inline std::u32string toUtf32(const std::string& s)
    {
        std::u32string result;
        utf8::utf8to32(s.begin(), s.end(), std::back_inserter(result));
        return result;
    }

    inline std::string toUtf8(const std::u16string& s)
    {
        std::string result;
        utf8::utf16to8(s.begin(), s.end(), std::back_inserter(result));
        return result;
    }

    inline std::string toUtf8(const std::u32string& s)
    {
        std::string result;
        utf8::utf32to8(s.begin(), s.end(), std::back_inserter(result));
        return result;
    }

    // Reverse string at codepoint level
    inline std::u16string utf16Reversed(const std::string& s)
    {
        std::vector<uint32_t> cps;
        utf8::utf8to32(s.begin(), s.end(), std::back_inserter(cps));
        std::ranges::reverse(cps);

        std::string utf8_temp;
        utf8::utf32to8(cps.begin(), cps.end(), std::back_inserter(utf8_temp));

        std::u16string result;
        utf8::utf8to16(utf8_temp.begin(), utf8_temp.end(), std::back_inserter(result));
        return result;
    }
}