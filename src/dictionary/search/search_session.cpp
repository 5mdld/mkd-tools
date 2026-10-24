//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "search_session.hpp"
#include "key_analysis.hpp"
#include "MKD/resource/keystore_search.hpp"
#include "dictionary/search/text_normalize.hpp"
#include "unicode/unicode.hpp"
#include "platform/search_lemma.hpp"
#include "dictionary/package/package_format.hpp"
#include "resource/keystore/keystore_format.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_set>

namespace MKD::detail::search
{
    void SearchSession::Matches::sort()
    {
        std::ranges::sort(entries);
        entries.erase(std::ranges::unique(entries).begin(), entries.end());
        sortKeys();
    }

    void SearchSession::Matches::sortKeys()
    {
        std::ranges::sort(keys);
        keys.erase(std::ranges::unique(keys).begin(), keys.end());
    }

    void SearchSession::Matches::unite(Matches other)
    {
        std::vector<EntryId> merged;
        merged.reserve(entries.size() + other.entries.size());
        std::ranges::set_union(entries, other.entries, std::back_inserter(merged));
        entries = std::move(merged);
        keys.insert(keys.end(), std::make_move_iterator(other.keys.begin()), std::make_move_iterator(other.keys.end()));
        sortKeys();
    }

    void SearchSession::Matches::intersect(Matches other)
    {
        std::vector<EntryId> common;
        common.reserve(std::min(entries.size(), other.entries.size()));
        std::ranges::set_intersection(entries, other.entries, std::back_inserter(common));
        entries = std::move(common);

        if (empty())
            keys.clear();
        else
        {
            keys.insert(keys.end(), std::make_move_iterator(other.keys.begin()),
                        std::make_move_iterator(other.keys.end()));
            sortKeys();
        }
    }

    SearchSession::SearchSession(const SearchProfile& profile, std::string_view query,
                                 const SearchOptions& options, std::stop_token stop)
        : profile_(profile), text_(query), options_(options), stop_(stop), query_(profile, query)
    {
    }

    Result<SearchResult> SearchSession::run() const
    {
        if (auto valid = validate(); !valid)
            return std::unexpected(valid.error());

        auto result = options_.fallback == SearchFallback::DictionaryRules
                          ? dictionaryQuery()
                          : terms(query_.keys(options_.scope, SearchFallback::None), options_.scope);
        if (!result)
            return std::unexpected(result.error());

        return finish(std::move(*result));
    }

    Result<void> SearchSession::validate() const
    {
        if (cancelled())
            return std::unexpected("Search cancelled");

        if (unicode::firstInvalidUtf8Offset(text_) || text_.contains('\0'))
            return std::unexpected("Search query must be valid UTF-8 without embedded nulls");

        if (options_.match != SearchMode::Prefix && options_.match != SearchMode::Exact && options_.match !=
            SearchMode::Suffix)
            return std::unexpected("Invalid search mode");

        if (options_.fallback != SearchFallback::DictionaryRules && options_.fallback != SearchFallback::None)
            return std::unexpected("Invalid search fallback policy");

        const auto scope = options_.scope;
        if (std::ranges::none_of(searchScopes, [scope](const auto& names) { return names.scope == scope; }))
            return std::unexpected("Invalid search scope");

        if ((options_.fallback == SearchFallback::DictionaryRules || !profile_.normalizationKnown()) && profile_.
            unsupportedReason())
            return std::unexpected(*profile_.unsupportedReason());

        return {};
    }

    Result<SearchResult> SearchSession::finish(Matches result) const
    {
        if (result.scope == SearchScope::Headword)
        {
            // Monokakido's search keeps key order through intersection and resolves IDs only at the end.
            const auto* store = profile_.headwordStore();
            if (store && store->resolveEntryIds(result.entries))
            {
                std::unordered_set<EntryId> seen;
                std::erase_if(result.entries, [&](const EntryId id)
                {
                    return !seen.insert(id).second;
                });
            }
            if (options_.fallback == SearchFallback::DictionaryRules)
                if (auto ordered = orderModernEntries(result); !ordered)
                    return std::unexpected(ordered.error());
        }

        if (options_.fallback == SearchFallback::DictionaryRules && profile_.pageEntriesFirst()
            && (result.scope == SearchScope::CompoundNoun || result.scope == SearchScope::Numeral))
            std::ranges::stable_partition(result.entries, [](const EntryId id)
            {
                return id.itemId == 0;
            });

        if (cancelled())
            return std::unexpected("Search cancelled");

        if (result.empty())
        {
            result.keys.clear();
            result.scope = options_.scope;
        }

        const bool truncated = options_.limit != 0 && result.entries.size() > options_.limit;
        if (truncated)
            result.entries.resize(options_.limit);

        return SearchResult{
            std::move(result.entries),
            std::move(result.keys),
            result.scope,
            truncated
        };
    }

    Result<SearchSession::Matches> SearchSession::dictionaryQuery() const
    {
        const auto keys = query_.keys(options_.scope, SearchFallback::DictionaryRules);
        if (keys.empty())
            return Matches{.scope = options_.scope};

        if (profile_.complexSearch(options_.scope))
            return complex(keys, options_.scope);

        auto result = terms(keys, options_.scope);
        if (!result || !result->empty())
            return result;

        if (options_.scope == SearchScope::Headword && profile_.hasHeadlineKeyIdentifiers())
        {
            result = joinedQuery(keys);
            if (!result || !result->empty())
                return result;
        }

        result = fallbackScopes(keys);
        if (!result || !result->empty())
            return result;

        if (options_.scope == SearchScope::Headword && profile_.hasHeadlineKeyIdentifiers()
            && !profile_.allowsScopeFallback())
        {
            result = headlineFallback(keys);
            if (!result || !result->empty())
                return result;
        }
        return joinedQuery(keys);
    }

    Result<SearchSession::Matches> SearchSession::joinedQuery(const std::vector<std::string>& keys) const
    {
        if (keys.size() <= 1)
            return Matches{.scope = options_.scope};

        auto result = key(SearchQuery::joined(keys), options_.scope);
        if (result)
            for (auto& match : result->keys)
                match.origin = SearchOrigin::JoinedQuery;

        return result;
    }

    Result<SearchSession::Matches> SearchSession::headlineFallback(const std::vector<std::string>& keys) const
    {
        const auto filtered = query_.withoutHeadlineStopWords(keys);
        if (filtered == keys)
            return Matches{.scope = options_.scope};

        auto result = terms(filtered, options_.scope);
        if (!result || !result->empty())
            return result;

        return joinedQuery(filtered);
    }

    Result<SearchSession::Matches> SearchSession::fallbackScopes(const std::vector<std::string>& keys) const
    {
        auto scope = options_.scope;
        auto fallbackKeys = keys;
        Matches empty{.scope = scope};
        if (scope == SearchScope::Headword && profile_.allowsScopeFallback())
        {
            scope = SearchScope::Idiom;
            fallbackKeys = query_.keys(scope, SearchFallback::DictionaryRules);
            auto result = terms(fallbackKeys, scope);
            if (!result || !result->empty())
                return result;
        }

        if (scope == SearchScope::Idiom)
        {
            const auto filtered = query_.withoutStopWords(fallbackKeys);
            if (filtered != fallbackKeys)
            {
                auto result = terms(filtered, scope);
                if (!result || !result->empty())
                    return result;
            }
        }

        if (options_.scope <= SearchScope::Idiom && profile_.allowsScopeFallback()
            && fallbackKeys.size() == 1 && profile_.japanese()
            && unicode::containsJapaneseScript(fallbackKeys.front()))
            return terms(fallbackKeys, SearchScope::Example);

        return empty;
    }

    Result<SearchSession::Matches> SearchSession::terms(const std::vector<std::string>& keys, SearchScope scope) const
    {
        std::optional<Matches> accumulated;
        for (const auto& text : keys)
        {
            auto found = key(text, scope);
            if (!found)
                return std::unexpected(found.error());

            if (options_.fallback == SearchFallback::DictionaryRules && scope == SearchScope::Idiom
                && options_.match != SearchMode::Suffix && !profile_.japanese())
            {
                if (auto lemma = searchLemma(text, profile_.language()); lemma && *lemma != text)
                {
                    auto stem = key(*lemma, scope);
                    if (!stem)
                        return std::unexpected(stem.error());

                    for (auto& match : stem->keys)
                        match.origin = SearchOrigin::Inflection;

                    found->unite(std::move(*stem));
                }
            }

            if (!accumulated)
                accumulated = std::move(*found);
            else if (options_.fallback == SearchFallback::DictionaryRules && scope == SearchScope::Headword
                && profile_.hasHeadlineKeyIdentifiers())
            {
                if (auto common = intersectHeadwords(*accumulated, std::move(*found)); !common)
                    return std::unexpected(common.error());
            }
            else
                accumulated->intersect(std::move(*found));

            if (accumulated->empty())
                break;
        }
        return accumulated ? std::move(*accumulated) : Matches{.scope = scope};
    }

    Result<void> SearchSession::intersectHeadwords(Matches& result, Matches other) const
    {
        const auto* store = profile_.headwordStore();
        if (!store || result.empty() || other.empty())
        {
            result.entries.clear();
            result.keys.clear();
            return {};
        }

        // Monokakido's search retains keys from the smaller set, choosing the newer set on a tie.
        auto& smaller = result.entries.size() < other.entries.size() ? result.entries : other.entries;
        auto& larger = result.entries.size() < other.entries.size() ? other.entries : result.entries;
        auto mapped = mappedHeadwords(smaller, *store);
        if (!mapped)
            return std::unexpected(mapped.error());

        auto compared = mappedHeadwords(larger, *store);
        if (!compared)
            return std::unexpected(compared.error());

        std::ranges::sort(*compared, {}, &HeadlineKey::entry);
        std::vector<EntryId> common;
        common.reserve(mapped->size());
        for (const auto& pair : *mapped)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            if (std::ranges::binary_search(*compared, pair.entry, {}, &HeadlineKey::entry))
                common.push_back(pair.key);
        }
        // Membership uses mapped targets, while retained keys stay in stored ID order.
        common.erase(std::ranges::unique(common).begin(), common.end());
        result.entries = std::move(common);
        if (result.empty())
            result.keys.clear();
        else
        {
            result.keys.insert(result.keys.end(), std::make_move_iterator(other.keys.begin()),
                               std::make_move_iterator(other.keys.end()));
            result.sortKeys();
        }
        return {};
    }

    Result<std::vector<SearchSession::HeadlineKey>> SearchSession::mappedHeadwords(
        std::span<const EntryId> keys, const Keystore& store) const
    {
        std::vector<HeadlineKey> result;
        result.reserve(keys.size());
        for (const auto key : keys)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            auto entry = store.entryIdForKeyId(key.pageId);
            if (!entry)
                return std::unexpected(entry.error());

            if (!*entry)
                return std::vector<HeadlineKey>{};

            result.push_back({{key.pageId, 0}, **entry});
        }
        return result;
    }

    Result<SearchSession::Matches> SearchSession::lookup(std::string_view text, const SearchScope scope,
                                                         const SearchMode mode, const SearchOrigin origin) const
    {
        Matches result{.scope = scope};
        for (const auto* store : profile_.stores(scope, options_.fallback))
            if (auto appended = appendStore(result, *store, text, mode, origin); !appended)
                return std::unexpected(appended.error());

        result.sort();
        return result;
    }

    Result<void> SearchSession::appendStore(Matches& result, const Keystore& store,
                                            std::string_view text, SearchMode mode, const SearchOrigin origin) const
    {
        auto range = keystoreSearch(store, text, mode, stop_);
        if (!range)
            return std::unexpected(range.error());

        if (range->empty() && mode == SearchMode::Exact && options_.fallback == SearchFallback::DictionaryRules)
        {
            mode = SearchMode::Prefix;
            range = keystoreSearch(store, text, mode, stop_);
            if (!range)
                return std::unexpected(range.error());
        }

        const auto before = result.entries.size();
        for (auto i = range->begin; i < range->end; ++i)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            auto record = store.entryAt(range->indexType, i);
            if (!record)
                return std::unexpected(record.error());

            if (mode != SearchMode::Exact && (record->flags & keystore::word::exactOnly))
                continue;

            auto ids = store.entryIdsAt(range->indexType, i, KeystoreIdMapping::Stored, stop_);
            if (!ids)
                return std::unexpected(ids.error());

            result.entries.insert(result.entries.end(), ids->begin(), ids->end());
        }

        if (result.entries.size() != before)
            result.keys.push_back({std::string(text), mode, origin});

        return {};
    }

    Result<SearchSession::Matches> SearchSession::simple(std::string_view text, const SearchScope scope,
                                                         const SearchMode mode) const
    {
        auto result = lookup(text, scope, mode);
        if (!result || !result->empty() || options_.fallback == SearchFallback::None || mode == SearchMode::Suffix)
            return result;

        for (const auto& variation : query_.variations(text, scope))
        {
            auto found = lookup(variation, scope, SearchMode::Exact, SearchOrigin::Inflection);
            if (!found)
                return std::unexpected(found.error());

            result->unite(std::move(*found));
        }
        return result;
    }

    Result<SearchSession::Matches> SearchSession::key(std::string_view text, const SearchScope scope) const
    {
        if (cancelled())
            return std::unexpected("Search cancelled");

        if (options_.fallback == SearchFallback::DictionaryRules && SearchProfile::compoundSearch(scope)
            && !(profile_.family() == monokakido::search::Family::Kanwa && scope == SearchScope::Idiom)
            && unicode::containsJapaneseScript(text))
            return compound(text, scope);
        return simple(text, scope, options_.match);
    }

    Result<SearchSession::Matches> SearchSession::compound(std::string_view text, const SearchScope scope) const
    {
        auto found = compoundSuffix(text, scope);
        if (!found)
            return std::unexpected(found.error());

        if (found->matches.empty() || found->prefix.empty())
            return std::move(found->matches);

        if (auto prefix = intersectCompoundPrefix(found->matches, found->prefix); !prefix)
            return std::unexpected(prefix.error());

        if (auto filtered = filterCompoundHeadlines(found->matches, text); !filtered)
            return std::unexpected(filtered.error());

        return std::move(found->matches);
    }

    Result<SearchSession::CompoundMatch> SearchSession::compoundSuffix(std::string_view text,
                                                                       const SearchScope scope) const
    {
        for (auto cluster : unicode::graphemeClusters(text))
        {
            const auto offset = static_cast<size_t>(cluster.data() - text.data());
            auto found = lookup(text.substr(offset), scope, options_.match, SearchOrigin::Compound);
            if (!found)
                return std::unexpected(found.error());

            if (!found->empty())
                return CompoundMatch{std::move(*found), text.substr(0, offset)};
        }
        return CompoundMatch{Matches{.scope = scope}, {}};
    }

    Result<void> SearchSession::intersectCompoundPrefix(Matches& result, std::string_view prefix) const
    {
        const auto clusters = unicode::graphemeClusters(prefix);
        size_t consumed = 0;
        while (consumed < clusters.size())
        {
            size_t matched = consumed;
            for (size_t end = clusters.size(); end > consumed; --end)
            {
                const auto beginOffset = static_cast<size_t>(clusters[consumed].data() - prefix.data());
                const auto endOffset = end == clusters.size()
                                           ? prefix.size()
                                           : static_cast<size_t>(clusters[end].data() - prefix.data());
                auto found = lookup(prefix.substr(beginOffset, endOffset - beginOffset), result.scope,
                                    SearchMode::Exact, SearchOrigin::Compound);
                if (!found)
                    return std::unexpected(found.error());

                if (found->empty())
                    continue;

                result.intersect(std::move(*found));
                if (result.empty())
                    return {};

                matched = end;
                break;
            }

            if (matched == consumed)
                break;

            consumed = matched;
        }
        return {};
    }

    Result<void> SearchSession::filterCompoundHeadlines(Matches& result, std::string_view text) const
    {
        size_t retained = 0;
        for (const auto entry : result.entries)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            auto value = headline(entry, false);
            if (!value)
                return std::unexpected(value.error());

            if (!*value || normalizedContains(**value, text))
                result.entries[retained++] = entry;
        }

        result.entries.resize(retained);
        if (result.empty())
            result.keys.clear();

        return {};
    }

    Result<SearchSession::Matches> SearchSession::complex(const std::vector<std::string>& keys,
                                                          const SearchScope scope) const
    {
        const auto prepared = keyComponents(keys);
        auto result = intersectComponents(prepared.values, scope);
        if (!result)
            return std::unexpected(result.error());

        if (prepared.filterHeadlines)
            if (auto filtered = filterSortingHeadlines(*result, keys); !filtered)
                return std::unexpected(filtered.error());

        result->keys.clear();
        if (!result->empty())
            for (const auto& key : keys)
                result->keys.push_back({key, SearchMode::Prefix, SearchOrigin::Compound});

        return result;
    }

    Result<SearchSession::Matches> SearchSession::intersectComponents(const std::vector<std::string>& components,
                                                                      const SearchScope scope) const
    {
        std::optional<Matches> result;
        for (const auto& component : components)
        {
            auto found = lookup(component, scope, SearchMode::Prefix, SearchOrigin::Compound);
            if (!found)
                return std::unexpected(found.error());

            if (!result)
                result = std::move(*found);
            else
                result->intersect(std::move(*found));

            if (result->empty())
                break;
        }
        return result ? std::move(*result) : Matches{.scope = scope};
    }

    Result<void> SearchSession::filterSortingHeadlines(Matches& result, const std::vector<std::string>& keys) const
    {
        size_t retained = 0;
        for (const auto entry : result.entries)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            const auto headlineId = companionTarget(entry, profile_.headlineOffset(result.scope));

            auto value = headline(headlineId, true);
            if (!value)
                return std::unexpected(value.error());

            if (*value && std::ranges::all_of(keys, [&](const auto& key) { return (**value).contains(key); }))
                result.entries[retained++] = entry;
        }

        result.entries.resize(retained);
        return {};
    }

    Result<std::optional<std::string>> SearchSession::headline(const EntryId id, bool sorting) const
    {
        for (const auto& store : profile_.dictionary().headlines())
        {
            auto value = store.findComponentsForEntryId(id, !sorting);
            if (!value) return std::unexpected(value.error());
            if (!*value) continue;
            return unicode::toUtf8(sorting ? (**value).sortingHeadline : (**value).full());
        }
        return std::optional<std::string>{};
    }

    Result<void> SearchSession::orderModernEntries(Matches& result) const
    {
        const auto& range = profile_.dictionary().searchConfiguration().modernEntryRange;
        if (!range || profile_.family() != monokakido::search::Family::Kogo)
            return {};

        const auto normalized = unicode::normalizeDictionaryKey(text_, profile_.modernNormalization());
        std::vector<EntryId> ordered;
        ordered.reserve(result.entries.size());
        for (auto entry : result.entries)
        {
            if (cancelled())
                return std::unexpected("Search cancelled");

            if (entry.pageId < range->minimum || entry.pageId > range->maximum)
                continue;

            auto value = headline(entry, false);
            if (!value)
                return std::unexpected(value.error());

            if (!*value)
                continue;

            size_t offset = 0;
            for (size_t i = 0; i < monokakido::search::modernHeadlinePrefixLength && offset < (**value).size(); ++i)
                static_cast<void>(unicode::nextCodepoint(**value, offset));

            if (unicode::normalizeDictionaryKey(std::string_view(**value).substr(offset),
                                                profile_.modernNormalization()) == normalized)
                ordered.push_back(entry);
        }

        for (auto entry : result.entries)
            if (entry.pageId < range->minimum || entry.pageId > range->maximum)
                ordered.push_back(entry);

        result.entries = std::move(ordered);
        return {};
    }
}
