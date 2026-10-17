//
// kiwakiwaaにより 2026/10/17 に作成されました。
//

#include "reading.hpp"
#include <MKD/dictionary/dictionary_search.hpp>
#include <MKD/dictionary/dictionary.hpp>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace MKD::Python
{
    EntryId searchResultAt(const SearchResult& result, Py_ssize_t index)
    {
        return result.entries[sequenceIndex(index, result.size())];
    }

    nb::typed<nb::tuple, SearchScope, nb::ellipsis> searchScopes(const DictionarySearch& search,
                                                                 const SearchFallback fallback)
    {
        nb::list scopes;
        for (const auto scope : search.scopes(fallback))
            scopes.append(nb::cast(scope));

        return nb::tuple(scopes);
    }

    void bindSearchEnums(nb::module_& module)
    {
        nb::enum_<SearchScope>(module, "SearchScope")
            .value("HEADWORD", SearchScope::Headword)
            .value("IDIOM", SearchScope::Idiom)
            .value("EXAMPLE", SearchScope::Example)
            .value("ENGLISH", SearchScope::English)
            .value("SENSE", SearchScope::Sense)
            .value("MODERN", SearchScope::Modern)
            .value("KANJI", SearchScope::Kanji)
            .value("COLLOCATION", SearchScope::Collocation)
            .value("CJ", SearchScope::CJ)
            .value("JC", SearchScope::JC)
            .value("FULLTEXT", SearchScope::Fulltext)
            .value("CATEGORY", SearchScope::Category)
            .value("COMPOUND", SearchScope::Compound)
            .value("NUMERAL", SearchScope::Numeral);
        nb::enum_<SearchMode>(module, "SearchMode")
            .value("PREFIX", SearchMode::Prefix)
            .value("EXACT", SearchMode::Exact)
            .value("SUFFIX", SearchMode::Suffix);
        nb::enum_<SearchFallback>(module, "SearchFallback")
            .value("NONE", SearchFallback::None)
            .value("NATIVE", SearchFallback::Native);
        nb::enum_<SearchOrigin>(module, "SearchOrigin")
            .value("QUERY", SearchOrigin::Query)
            .value("INFLECTION", SearchOrigin::Inflection)
            .value("COMPOUND", SearchOrigin::Compound)
            .value("JOINED_QUERY", SearchOrigin::JoinedQuery);
    }

    void bindSearchMatch(nb::module_& module)
    {
        auto type = nb::class_<SearchMatch>(module, "SearchMatch");
        type.def_ro("key", &SearchMatch::key)
            .def_ro("match", &SearchMatch::match)
            .def_ro("origin", &SearchMatch::origin);
    }

    void bindSearchResult(nb::module_& module)
    {
        auto type = nb::class_<SearchResult>(module, "SearchResult");
        type.def("__len__", &SearchResult::size);

        type.def("__getitem__", &searchResultAt, "index"_a);

        type.def(
            "__iter__",
            [](const SearchResult& result)
            {
                return nb::make_iterator<nb::rv_policy::copy>(nb::type<SearchResult>(), "EntryIterator",
                                                              result.begin(), result.end());
            },
            nb::keep_alive<0, 1>());

        type.def_ro("scope", &SearchResult::scope);
        type.def_ro("truncated", &SearchResult::truncated);
        type.def_prop_ro("matches",
                         [](const SearchResult& result) -> nb::typed<nb::tuple, SearchMatch, nb::ellipsis>
                         {
                             return nb::tuple(nb::cast(result.matches));
                         });
    }

    void bindSearch(nb::module_& module)
    {
        auto type = nb::class_<DictionarySearch>(module, "DictionarySearch");
        type.def(nb::init<const Dictionary&>(), "dictionary"_a, nb::keep_alive<1, 2>());

        type.def_prop_ro("supports_native_search", &DictionarySearch::supportsNativeSearch);

        type.def("scopes", &searchScopes, nb::kw_only(), "fallback"_a = SearchFallback::Native);

        type.def(
            "search",
            [](const DictionarySearch& search, const std::string& query, SearchScope scope, SearchMode match,
               SearchFallback fallback, size_t limit)
            {
                return unwrap(search.search(
                    query, {.scope = scope, .match = match, .fallback = fallback, .limit = limit}));
            },
            "query"_a, nb::kw_only(), "scope"_a = SearchScope::Headword, "match"_a = SearchMode::Prefix,
            "fallback"_a = SearchFallback::Native, "limit"_a = 0, nb::call_guard<nb::gil_scoped_release>());
    }
}
