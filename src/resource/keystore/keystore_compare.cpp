//
// kiwakiwaaにより 2026/03/11 に作成されました。
//

#include "keystore_compare.hpp"
#include "unicode/unicode.hpp"

namespace MKD::detail::keystore
{
    class KeyCursor
    {
    public:
        KeyCursor(std::string_view text, bool reverse)
            : text_(text), reverse_(reverse), offset_(reverse ? text.size() : 0)
        {
        }

        std::optional<char32_t> next()
        {
            while (reverse_ ? offset_ > 0 : offset_ < text_.size())
            {
                const auto cp = reverse_
                                    ? unicode::previousCodepoint(text_, offset_)
                                    : unicode::nextCodepoint(text_, offset_);
                if (!isIgnorable(cp))
                    return foldCase(cp);
            }
            return std::nullopt;
        }

    private:
        std::string_view text_;
        bool reverse_;
        size_t offset_;
    };

    char32_t foldCase(const char32_t cp)
    {
        if (cp >= U'A' && cp <= U'Z')
            return cp | 0x20;
        if (cp >= 0x3000 && cp < 0xA000)
            return cp;
        return unicode::toLowercase(cp);
    }

    std::u32string normalizeKeyToUTF32(std::string_view text, bool reverse)
    {
        std::u32string result;
        KeyCursor cursor(text, reverse);
        while (auto cp = cursor.next())
            result.push_back(*cp);
        return result;
    }

    int comparePrefix(std::string_view key, std::string_view query, const CompareMode mode)
    {
        KeyCursor left(key, mode == CompareMode::Backward);
        KeyCursor right(query, mode == CompareMode::Backward);
        while (auto b = right.next())
        {
            const auto a = left.next();
            if (!a || *a < *b) return -1;
            if (*a > *b) return 1;
        }
        return 0;
    }

    int compare(std::string_view a, std::string_view b, const CompareMode mode)
    {
        if (mode == CompareMode::Length)
        {
            // Index A counts all code points, including spaces and hyphens.
            const auto left = unicode::codepointCount(a);
            const auto right = unicode::codepointCount(b);
            if (left != right)
                return left < right ? -1 : 1;
        }

        KeyCursor left(a, mode == CompareMode::Backward);
        KeyCursor right(b, mode == CompareMode::Backward);
        while (true)
        {
            const auto ca = left.next();
            const auto cb = right.next();
            if (ca == cb)
            {
                if (!ca) return 0;
                continue;
            }
            return ca < cb ? -1 : 1;
        }
    }
}
