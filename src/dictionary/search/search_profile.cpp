//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "search_profile.hpp"
#include "dictionary/package/native_format.hpp"

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

        mapNativeStores();
        for (const auto& [scope, stores] : nativeStores_)
            if (!stores.empty())
                nativeScopes_.push_back(scope);

        for (const auto& [scope, stores] : directStores_)
            if (!stores.empty())
                directScopes_.push_back(scope);
    }

    void SearchProfile::selectDefinitions()
    {
        const auto name = dictionary_.searchConfiguration().productSearchClass.value_or(
            native::search::defaultClass.name.data());
        const auto definition =
            std::ranges::find(native::search::classes, name, &native::search::ClassDefinition::name);
        if (definition == native::search::classes.end())
            unsupported_ = "Unsupported native search class: " + name;
        else
            class_ = *definition;

        if (const auto& delegate = dictionary_.searchConfiguration().productDelegateClass)
        {
            const auto found = std::ranges::find(native::search::delegates, *delegate,
                                                 &native::search::DelegateDefinition::name);
            if (found != native::search::delegates.end())
                delegate_ = *found;
            else if (!unsupported_)
                unsupported_ = "Unsupported native search delegate: " + *delegate;
        }
    }

    void SearchProfile::selectNormalization()
    {
        const auto name = dictionary_.content().keyNormalization.value_or(
            native::search::defaultNormalization.name.data());
        const auto definition = std::ranges::find(native::search::normalizations, name,
                                                  &native::search::NormalizationDefinition::name);
        normalizationKnown_ = definition != native::search::normalizations.end();
        if (normalizationKnown_)
            normalization_ = definition->options | delegate_.normalization;
        else
            unsupported_ = "Unsupported key normalisation: " + name;
    }

    void SearchProfile::addStores(const SearchScope destination, const SearchScope source)
    {
        if (const auto found = directStores_.find(source); found != directStores_.end())
        {
            auto& stores = nativeStores_[destination];
            for (const auto* store : found->second)
                if (std::ranges::find(stores, store) == stores.end())
                    stores.push_back(store);
        }
    }

    void SearchProfile::mapNativeStores()
    {
        if (class_.family == native::search::Family::Kanwa)
        {
            addStores(SearchScope::Headword, SearchScope::Headword);
            addStores(SearchScope::Headword, SearchScope::Idiom);
            addStores(SearchScope::Idiom, SearchScope::Idiom);
            addStores(SearchScope::Kanji, SearchScope::Headword);
            addStores(SearchScope::Example, SearchScope::Example);
            addStores(SearchScope::Sense, SearchScope::Sense);
        }
        else if (class_.family == native::search::Family::Kogo)
        {
            for (const auto scope : {
                     SearchScope::Headword, SearchScope::Example, SearchScope::Sense, SearchScope::Modern
                 })
                addStores(scope, scope);
        }
        else
        {
            nativeStores_ = directStores_;
            if (delegate_.includesSupplementaryHeadwords)
                for (const auto& store : dictionary_.keystores())
                    if (store.filename() == native::paths::supplementaryHeadwords)
                        nativeStores_[SearchScope::Headword].push_back(&store);
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
        if (class_.family == native::search::Family::Kanwa && scope == SearchScope::Idiom) return false;
        return hasHeadlineKeyIdentifiers() || compoundSearch(scope)
            || (class_.family == native::search::Family::Kogo && scope == SearchScope::Modern);
    }

    bool SearchProfile::complexSearch(const SearchScope scope) const noexcept
    {
        if (class_.family == native::search::Family::Kanwa || class_.family == native::search::Family::Complex)
            return scope == SearchScope::Example || scope == SearchScope::Sense;

        const auto id = dictionary_.content().dictionaryId;
        return class_.family == native::search::Family::Kogo && id
            && std::ranges::find(native::search::classicalComplexDictionaries, *id) !=
            native::search::classicalComplexDictionaries.end()
            && (scope == SearchScope::Example || scope == SearchScope::Sense || scope == SearchScope::Modern);
    }

    std::span<const Keystore* const> SearchProfile::stores(const SearchScope scope, const SearchFallback fallback) const
    {
        const auto& map = fallback == SearchFallback::Native ? nativeStores_ : directStores_;
        const auto found = map.find(scope);
        return found == map.end() ? std::span<const Keystore* const>{} : found->second;
    }

    std::span<const SearchScope> SearchProfile::scopes(const SearchFallback fallback) const noexcept
    {
        if (fallback == SearchFallback::None)
            return directScopes_;
        return unsupported_ ? std::span<const SearchScope>{} : nativeScopes_;
    }

    uint16_t SearchProfile::headlineOffset(const SearchScope scope) const noexcept
    {
        return class_.family == native::search::Family::Kogo && scope == SearchScope::Modern
                   ? native::search::modernHeadlineOffset
                   : native::search::normalizedHeadlineOffset;
    }

    unicode::DictionaryKeyNormalizeOption SearchProfile::modernNormalization() const noexcept
    {
        return delegate_.modernNormalization;
    }
}
