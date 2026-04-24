//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#include "keystore_compare.hpp"
#include "unicode/unicode.hpp"

#include <algorithm>

namespace MKD::detail::keystore
{
    std::u32string normalizeKeyToUTF32(std::string_view s, const bool reverse)
    {
        std::u32string result;
        size_t offset = 0;
        while (offset < s.size())
        {
            const char32_t cp = unicode::nextCodepoint(s, offset);
            if (isIgnorable(cp)) continue;
            result.push_back(unicode::keystoreFold(cp));
        }

        if (reverse)
            std::ranges::reverse(result);

        return result;
    }


    size_t normalizedLength(std::string_view s)
    {
        size_t offset = 0;
        size_t len = 0;
        while (offset < s.size())
        {
            if (const char32_t cp = unicode::nextCodepoint(s, offset); !isIgnorable(cp))
                ++len;
        }
        return len;
    }


    int compare(std::string_view a, std::string_view b, CompareMode mode)
    {
        if (mode == CompareMode::Length) // first by codepoint len then forward (prefix) with fold case
        {
            const size_t lenA = normalizedLength(a);
            const size_t lenB = normalizedLength(b);
            if (lenA < lenB) return -1;
            if (lenA > lenB) return 1;

            mode = CompareMode::Forward;
        }

        if (mode == CompareMode::Backward) // for suffix search
        {
            size_t pa = a.size(); // reverse both strings
            size_t pb = b.size();

            while (true)
            {
                char32_t ca{};
                char32_t cb{};
                bool hasA = false;
                bool hasB = false;

                while (pa > 0)
                {
                    if (const char32_t cp = unicode::previousCodepoint(a, pa); !isIgnorable(cp))
                    {
                        ca = unicode::keystoreFold(cp);
                        hasA = true;
                        break;
                    }
                }

                while (pb > 0)
                {
                    if (const char32_t cp = unicode::previousCodepoint(b, pb); !isIgnorable(cp))
                    {
                        cb = unicode::keystoreFold(cp);
                        hasB = true;
                        break;
                    }
                }

                if (!hasA && !hasB) return 0;
                if (!hasA) return -1;
                if (!hasB) return 1;
                if (ca < cb) return -1;
                if (ca > cb) return 1;
            }
        }
        else // Forward
        {
            size_t pa = 0;
            size_t pb = 0;

            while (true)
            {
                char32_t ca{};
                char32_t cb{};
                bool hasA = false;
                bool hasB = false;

                while (pa < a.size())
                {
                    if (const char32_t cp = unicode::nextCodepoint(a, pa); !isIgnorable(cp))
                    {
                        ca = unicode::keystoreFold(cp);
                        hasA = true;
                        break;
                    }
                }

                while (pb < b.size())
                {
                    if (const char32_t cp = unicode::nextCodepoint(b, pb); !isIgnorable(cp))
                    {
                        cb = unicode::keystoreFold(cp);
                        hasB = true;
                        break;
                    }
                }

                if (!hasA && !hasB) return 0;
                if (!hasA) return -1;
                if (!hasB) return 1;
                if (ca < cb) return -1;
                if (ca > cb) return 1;
            }
        }
    }
}
