#include "search_folding.hpp"

#include "case.hpp"
#include "utf8.hpp"

namespace MKD::detail::unicode
{
    Codepoint hiraganaToKatakana(const Codepoint cp) noexcept
    {
        if (kHiraganaLetterRange.contains(cp))
            return cp + kHiraganaToKatakanaOffset;

        return cp;
    }


    Codepoint keywordSearchFold(const Codepoint cp) noexcept
    {
        if (cp == kAsciiApostrophe || cp == kRightSingleQuote)
            return kCombiningAcute;

        return toLowercase(hiraganaToKatakana(cp));
    }


    bool isJapaneseScript(const Codepoint cp) noexcept
    {
        return kHiraganaRange.contains(cp)
               || kKatakanaRange.contains(cp)
               || kCjkUnifiedIdeographsRange.contains(cp)
               || kCjkUnifiedIdeographsExtensionARange.contains(cp);
    }


    bool containsJapaneseScript(const std::string_view text)
    {
        size_t offset = 0;
        while (offset < text.size())
        {
            if (isJapaneseScript(nextCodepoint(text, offset)))
                return true;
        }

        return false;
    }
}
