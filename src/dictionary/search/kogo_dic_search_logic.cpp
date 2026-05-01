//
// kiwakiwaaにより 2026/05/01 に作成されました。
//

#include "kogo_dic_search_logic.hpp"

#include "unicode/unicode.hpp"

#include <utility>

namespace MKD::detail::search
{
    bool KogoDicSearchLogic::EntryIdRange::contains(const EntryId& entryId) const noexcept
    {
        return entryId.pageId >= minimumEntryId && entryId.pageId <= maximumEntryId;
    }


    std::optional<KogoDicSearchLogic::EntryIdRange> KogoDicSearchLogic::gendaiEntryRangeForContent(
        const std::string_view contentIdentifier) noexcept
    {
        if (contentIdentifier == "SKOGO.J")
        {
            return EntryIdRange{
                .minimumEntryId = 19249,
                .maximumEntryId = 25337,
            };
        }

        if (contentIdentifier == "KOGO3.J")
        {
            return EntryIdRange{
                .minimumEntryId = 21173,
                .maximumEntryId = 27793,
            };
        }

        return std::nullopt;
    }


    Result<SearchResult> KogoDicSearchLogic::search(
        const std::string_view query,
        const SearchOptions& options)
    {
        auto result = DicComplexSearchLogic::search(query, options);
        if (!result)
            return result;

        if (!isCancelled())
        {
            if (scope_ == SearchScope::Headword)
                sortGendaiHeadwordResults(*result, query);

            if (options.scope == SearchScope::Modern)
                result->flags |= 0x10000;
        }

        return result;
    }


    bool KogoDicSearchLogic::usesComplexSearchForScope(
        const SearchScope scope,
        const Dictionary& dictionary) const
    {
        if (!gendaiEntryRangeForContent(dictionary.content().identifier))
            return false;

        return scope == SearchScope::Example
               || scope == SearchScope::Gogi
               || scope == SearchScope::Modern;
    }


    uint32_t KogoDicSearchLogic::normalizedHeadlineAnchorOffset() const noexcept
    {
        return scope_ == SearchScope::Modern ? 2304 : 2048;
    }


    std::vector<std::string> KogoDicSearchLogic::normalizedKeysForQuery(const std::string_view query) const
    {
        const auto key = gendaiEntryRangeForContent(dictionary_.content().identifier)
                             ? unicode::normalizeKanjiNumberString(query)
                             : std::string(query);
        const auto normalized = unicode::normalizeDictionaryKey(
            key,
            unicode::DictionaryKeyNormalizeOption::CollapseSeparators
        );
        return splitNormalizedKeys(normalized);
    }


    std::string_view KogoDicSearchLogic::gendaiHeadlineKeyText(const std::string_view headline) noexcept
    {
        size_t offset = 0;
        for (size_t i = 0; i < 3 && offset < headline.size(); ++i)
            static_cast<void>(unicode::nextCodepoint(headline, offset));

        return headline.substr(offset);
    }


    void KogoDicSearchLogic::sortGendaiHeadwordResults(
        SearchResult& result,
        const std::string_view query) const
    {
        const auto& contentIdentifier = dictionary_.content().identifier;
        const auto range = gendaiEntryRangeForContent(contentIdentifier);
        if (!range || result.entries.empty())
            return;

        const auto normalizeOptions = contentIdentifier == "SKOGO.J"
                                          ? unicode::DictionaryKeyNormalizeOption::CollapseSeparators
                                          : unicode::DictionaryKeyNormalizeOption::KeepTrailingProlongedSoundMark;
        const auto normalizedQuery = unicode::normalizeDictionaryKey(query, normalizeOptions);

        std::vector<EntryId> matchingGendaiEntries;
        std::vector<EntryId> nonGendaiEntries;
        matchingGendaiEntries.reserve(result.entries.size());
        nonGendaiEntries.reserve(result.entries.size());

        for (const auto& entry : result.entries)
        {
            if (!range->contains(entry))
            {
                nonGendaiEntries.push_back(entry);
                continue;
            }

            const auto headline = dictionary_.headlineForEntryId(entry);
            if (!headline)
                continue;

            const auto normalizedHeadline = unicode::normalizeDictionaryKey(
                gendaiHeadlineKeyText(*headline),
                normalizeOptions
            );

            if (normalizedHeadline == normalizedQuery)
                matchingGendaiEntries.push_back(entry);
        }

        if (nonGendaiEntries.size() == result.entries.size())
            return;

        matchingGendaiEntries.insert(
            matchingGendaiEntries.end(),
            nonGendaiEntries.begin(),
            nonGendaiEntries.end()
        );
        result.entries = std::move(matchingGendaiEntries);
    }
}
