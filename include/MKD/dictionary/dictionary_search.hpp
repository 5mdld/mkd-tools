//
// Public dictionary search API.
//

#pragma once

#include "MKD/result.hpp"
#include "MKD/resource/entry_id.hpp"
#include "MKD/resource/search_mode.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace MKD
{
    class Dictionary;

    enum class SearchScope : uint8_t
    {
        Headword = 0,
        Idiom = 1,
        Example = 2,
        English = 3,
        Gogi = 4,
        Kanji = 6,
        Collocation = 7,
        CJ = 8,
        JC = 9,
        Fulltext = 10,
        Group = 11,
        CompoundNoun = 100,
        Numeral = 101,

        Sense = Gogi,
        Category = Group,
        Compound = CompoundNoun
    };

    [[nodiscard]] constexpr std::optional<SearchScope> searchScopeFromName(const std::string_view name) noexcept
    {
        if (name == "Headword" || name == "Index" || name == "Vocabulary") return SearchScope::Headword;
        if (name == "Idiom" || name == "Idiom/Phrasal verb" || name == "Jyukugo"
            || name == "Kanyoku" || name == "Phrase") return SearchScope::Idiom;
        if (name == "Example") return SearchScope::Example;
        if (name == "English") return SearchScope::English;
        if (name == "Gogi" || name == "Yakugo") return SearchScope::Gogi;
        if (name == "Kanji" || name == "Oyaji") return SearchScope::Kanji;
        if (name == "Collocation") return SearchScope::Collocation;
        if (name == "CJ") return SearchScope::CJ;
        if (name == "JC") return SearchScope::JC;
        if (name == "Full-Text") return SearchScope::Fulltext;
        if (name == "Group") return SearchScope::Group;
        if (name == "Compound Noun") return SearchScope::CompoundNoun;
        if (name == "Numeral") return SearchScope::Numeral;
        return std::nullopt;
    }


    struct SearchOptions
    {
        SearchScope scope = SearchScope::Headword;
        SearchMode type = SearchMode::Prefix;
        size_t limit = 0; // 0 = no limit

        bool enableScopeFallback = true;
        bool enableStopWords = true;
        bool enableJapaneseCompound = true;
    };


    struct SearchResult
    {
        std::vector<EntryId> entries;
        //   0 = direct match
        //   4 = scope fallback
        //   8 = jp fallback
        uint32_t flags = 0;
        std::vector<std::string> matchedKeys;

        [[nodiscard]] size_t size() const noexcept { return entries.size(); }
        [[nodiscard]] bool empty() const noexcept { return entries.empty(); }

        [[nodiscard]] auto begin() const noexcept { return entries.begin(); }
        [[nodiscard]] auto end() const noexcept { return entries.end(); }

        void unionWith(const SearchResult& other);
        void intersectWith(const SearchResult& other);
    };


    class DictionarySearch
    {
    public:
        explicit DictionarySearch(const Dictionary& dict);
        ~DictionarySearch();

        DictionarySearch(DictionarySearch&&) noexcept;
        DictionarySearch& operator=(DictionarySearch&&) noexcept;

        DictionarySearch(const DictionarySearch&) = delete;
        DictionarySearch& operator=(const DictionarySearch&) = delete;

        [[nodiscard]] Result<SearchResult> search(std::string_view query,
                                                  const SearchOptions& options = {}) const;

        // cancel in progress search
        void cancel() const noexcept;

        // reset cancellation
        void reset() const noexcept;

        [[nodiscard]] bool isCancelled() const noexcept;

    private:
        // search keys at current scope with cascading scope fallback
        [[nodiscard]] Result<SearchResult> searchWithKeys(const std::vector<std::string>& keys,
                                                          size_t limit) const;

        // iterate keys, search each, intersect results. sets flags on result.
        [[nodiscard]] Result<SearchResult> searchWithKeysAndFlags(const std::vector<std::string>& keys,
                                                                  size_t limit,
                                                                  uint32_t flags) const;

        // dispatch compound search for Japanese keys at eligible scopes, otherwise simple
        [[nodiscard]] Result<SearchResult> searchSingleKey(std::string_view key, size_t limit) const;

        // compound word decomposition with front/back stripping
        [[nodiscard]] Result<SearchResult> japaneseCompoundSearch(std::string_view key) const;

        // binary search across keystores for the current scope
        [[nodiscard]] Result<SearchResult> simpleSearch(std::string_view key, SearchMode type) const;

        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
