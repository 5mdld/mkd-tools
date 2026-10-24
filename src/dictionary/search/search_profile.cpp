//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "search_profile.hpp"
#include "dictionary/package/package_format.hpp"

#include <algorithm>

namespace MKD::detail::search
{
    SearchProfile::SearchProfile(const Dictionary& dictionary) : dictionary_(dictionary)
    {
        selectDefinitions();
        selectNormalization();
        for (const auto& store : dictionary.keystores())
            if (auto scope = store.scope())
                directStores_[*scope].push_back(&store);

        mapDictionaryStores();
        for (const auto& [scope, stores] : dictionaryStores_)
            if (!stores.empty())
                dictionaryScopes_.push_back(scope);

        for (const auto& [scope, stores] : directStores_)
            if (!stores.empty())
                directScopes_.push_back(scope);
    }

    void SearchProfile::selectDefinitions()
    {
        const auto name = dictionary_.searchConfiguration().productSearchClass.value_or(
            monokakido::search::defaultClass.name.data());
        const auto definition =
            std::ranges::find(monokakido::search::classes, name, &monokakido::search::ClassDefinition::name);
        if (definition == monokakido::search::classes.end())
            unsupported_ = "Unsupported product search class: " + name;
        else
            class_ = *definition;

        if (const auto& delegate = dictionary_.searchConfiguration().productDelegateClass)
        {
            const auto found = std::ranges::find(monokakido::search::delegates, *delegate,
                                                 &monokakido::search::DelegateDefinition::name);
            if (found != monokakido::search::delegates.end())
                delegate_ = *found;
            else if (!unsupported_)
                unsupported_ = "Unsupported product search delegate: " + *delegate;
        }
    }

    void SearchProfile::selectNormalization()
    {
        const auto name = dictionary_.content().keyNormalization.value_or(
            monokakido::search::defaultNormalization.name.data());
        const auto definition = std::ranges::find(monokakido::search::normalizations, name,
                                                  &monokakido::search::NormalizationDefinition::name);
        normalizationKnown_ = definition != monokakido::search::normalizations.end();
        if (normalizationKnown_)
            normalization_ = definition->options | delegate_.normalization;
        else
            unsupported_ = "Unsupported key normalisation: " + name;
    }

    void SearchProfile::addStores(const SearchScope destination, const SearchScope source)
    {
        if (const auto found = directStores_.find(source); found != directStores_.end())
        {
            auto& stores = dictionaryStores_[destination];
            for (const auto* store : found->second)
                if (std::ranges::find(stores, store) == stores.end())
                    stores.push_back(store);
        }
    }

    void SearchProfile::mapDictionaryStores()
    {
        if (class_.family == monokakido::search::Family::Kanwa)
        {
            addStores(SearchScope::Headword, SearchScope::Headword);
            addStores(SearchScope::Headword, SearchScope::Idiom);
            addStores(SearchScope::Idiom, SearchScope::Idiom);
            addStores(SearchScope::Kanji, SearchScope::Headword);
            addStores(SearchScope::Example, SearchScope::Example);
            addStores(SearchScope::Sense, SearchScope::Sense);
        }
        else if (class_.family == monokakido::search::Family::Kogo)
        {
            for (const auto scope : {
                     SearchScope::Headword, SearchScope::Example, SearchScope::Sense, SearchScope::Modern
                 })
                addStores(scope, scope);
        }
        else
        {
            dictionaryStores_ = directStores_;
            if (delegate_.includesSupplementaryHeadwords)
                for (const auto& store : dictionary_.keystores())
                    if (store.filename() == monokakido::paths::supplementaryHeadwords)
                        dictionaryStores_[SearchScope::Headword].push_back(&store);
        }
    }

    std::string_view SearchProfile::language() const noexcept
    {
        const auto& languages = dictionary_.content().languages;
        return languages && !languages->empty() ? std::string_view(languages->front()) : std::string_view{};
    }

    bool SearchProfile::japanese() const noexcept { return language().starts_with(metadata::language::japanese); }

    bool SearchProfile::hasEnglish() const noexcept
    {
        const auto& languages = dictionary_.content().languages;
        return languages && std::ranges::any_of(*languages, [](const auto& language)
        {
            return language.starts_with(metadata::language::english);
        });
    }

    bool SearchProfile::hasHeadlineKeyIdentifiers() const noexcept
    {
        return dictionary_.content().hasHeadlineKeyIdentifier.value_or(false);
    }

    const Keystore* SearchProfile::headwordStore() const noexcept
    {
        const auto found = directStores_.find(SearchScope::Headword);
        return found == directStores_.end() || found->second.empty() ? nullptr : found->second.front();
    }

    bool SearchProfile::compoundSearch(const SearchScope scope) noexcept
    {
        switch (scope)
        {
        case SearchScope::Idiom:
        case SearchScope::Example:
        case SearchScope::Sense:
        case SearchScope::Collocation:
        case SearchScope::CJ:
        case SearchScope::Fulltext: return true;
        default: return false;
        }
    }

    bool SearchProfile::splitsQuery(const SearchScope scope) const noexcept
    {
        if (class_.family == monokakido::search::Family::Kanwa && scope == SearchScope::Idiom) return false;
        return hasHeadlineKeyIdentifiers() || compoundSearch(scope)
            || (class_.family == monokakido::search::Family::Kogo && scope == SearchScope::Modern);
    }

    bool SearchProfile::complexSearch(const SearchScope scope) const noexcept
    {
        if (class_.family == monokakido::search::Family::Kanwa || class_.family == monokakido::search::Family::Complex)
            return scope == SearchScope::Example || scope == SearchScope::Sense;

        const auto id = dictionary_.content().dictionaryId;
        return class_.family == monokakido::search::Family::Kogo && id
            && std::ranges::find(monokakido::search::classicalComplexDictionaries, *id) !=
            monokakido::search::classicalComplexDictionaries.end()
            && (scope == SearchScope::Example || scope == SearchScope::Sense || scope == SearchScope::Modern);
    }

    std::span<const Keystore* const> SearchProfile::stores(const SearchScope scope, const SearchFallback fallback) const
    {
        const auto& map = fallback == SearchFallback::DictionaryRules ? dictionaryStores_ : directStores_;
        const auto found = map.find(scope);
        return found == map.end() ? std::span<const Keystore* const>{} : found->second;
    }

    std::span<const SearchScope> SearchProfile::scopes(const SearchFallback fallback) const noexcept
    {
        if (fallback == SearchFallback::None)
            return directScopes_;
        return unsupported_ ? std::span<const SearchScope>{} : dictionaryScopes_;
    }

    uint16_t SearchProfile::headlineOffset(const SearchScope scope) const noexcept
    {
        return class_.family == monokakido::search::Family::Kogo && scope == SearchScope::Modern
                   ? monokakido::search::modernHeadlineOffset
                   : monokakido::search::normalizedHeadlineOffset;
    }

    unicode::DictionaryKeyNormalizeOption SearchProfile::modernNormalization() const noexcept
    {
        return delegate_.modernNormalization;
    }
}
