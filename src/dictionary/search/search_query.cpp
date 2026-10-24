//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "search_query.hpp"
#include "inflection_rules.hpp"
#include "unicode/unicode.hpp"
#include "resource/keystore/keystore_compare.hpp"

#include <algorithm>
#include <ranges>

namespace MKD::detail::search
{
    std::vector<std::string> SearchQuery::keys(const SearchScope scope, const SearchFallback fallback) const
    {
        const auto text = profile_.foldsKanjiNumbers()
                              ? unicode::normalizeKanjiNumberString(text_)
                              : std::string(text_);
        auto options = profile_.normalization();
        if (fallback == SearchFallback::None || !profile_.splitsQuery(scope))
            options |= unicode::DictionaryKeyNormalizeOption::CollapseSeparators;

        std::vector<std::string> keys;
        const auto append = [&](std::string_view part)
        {
            auto key = unicode::normalizeDictionaryKey(part, options);
            const auto first = key.find_first_not_of(' ');
            if (first == std::string::npos)
                return;

            key = key.substr(first, key.find_last_not_of(' ') - first + 1);
            if (!key.empty() && std::ranges::find(keys, key) == keys.end())
                keys.push_back(std::move(key));
        };

        if (fallback == SearchFallback::None || !profile_.splitsQuery(scope))
            append(text);
        else
        {
            size_t begin = 0;
            size_t offset = 0;
            while (offset < text.size())
            {
                const auto position = offset;
                const auto cp = unicode::nextCodepoint(text, offset);
                if (cp == U'\t' || cp == U' ' || cp == U'/' || cp == U'\u3000' /* 全角スペース */ || cp == U'\uFF0F' /* ／ */)
                {
                    append(std::string_view(text).substr(begin, position - begin));
                    begin = offset;
                }
            }
            append(std::string_view(text).substr(begin));
        }

        return keys;
    }

    std::vector<std::string> SearchQuery::withoutStopWords(const std::vector<std::string>& keys) const
    {
        if (!profile_.hasEnglish())
            return keys;

        std::vector<std::string> filtered;
        for (const auto& key : keys)
            if (std::ranges::find(monokakido::search::idiomStopWords, key) == monokakido::search::idiomStopWords.end())
                filtered.push_back(key);

        return filtered;
    }

    std::vector<std::string> SearchQuery::withoutHeadlineStopWords(const std::vector<std::string>& keys) const
    {
        if (!profile_.hasEnglish())
            return keys;

        std::vector<std::string> result;
        for (const auto& key : keys)
            if (std::ranges::find(monokakido::search::headlineSkippedWords, key) == monokakido::search::headlineSkippedWords.
                end())
                result.push_back(key);

        if (result.empty() && !keys.empty())
            result.push_back(keys.front());

        return result;
    }

    std::string SearchQuery::joined(const std::vector<std::string>& keys)
    {
        std::string text;
        for (const auto& key : keys)
            text += key;
        return text;
    }

    std::vector<std::string> SearchQuery::variations(std::string_view key, const SearchScope scope) const
    {
        std::vector<std::string_view> languages;
        if (scope == SearchScope::English)
            languages.push_back(metadata::language::english);
        else if (const auto& configured = profile_.dictionary().content().languages)
            for (const auto& language : *configured)
                languages.push_back(language);

        const auto lower = unicode::toUtf8(keystore::normalizeKeyToUTF32(key, false));
        key = lower;
        std::vector<std::string> result;
        auto rules = languages | std::views::transform(inflections::forLanguage)
            | std::views::join | std::views::join;
        for (const auto& [ending, replacement] : rules)
        {
            if (key.size() <= ending.size() || !key.ends_with(ending))
                continue;
            if (ending == "s" && replacement.empty() && key.ends_with("ass"))
                continue;

            auto variant = std::string(key.substr(0, key.size() - ending.size())).append(replacement);
            if (variant != "as" && unicode::utf16CodeUnitCount(variant) >= 2
                && std::ranges::find(result, variant) == result.end())
                result.push_back(std::move(variant));
        }
        return result;
    }
}
