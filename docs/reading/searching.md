[Documentation](../README.md) · [Core concepts](../concepts.md)

# Searching

Choose three things: **policy** (dictionary-specific rules or direct lookup),
**scope** (which kind of index), and **match** (prefix, exact, or suffix).

## Open a dictionary

```cpp
auto product = MKD::DictionaryProduct::openAtPath(
    dictionariesDirectory / "DAIJISEN2").value();

const auto* dictionary = product.findContent("DAIJISEN.J");
if (!dictionary)
    return std::unexpected("DAIJISEN.J is not present in this product.");

MKD::DictionarySearch search(*dictionary);
```

This example selects 大辞泉 by content identifier. Keep the dictionary alive
at the same address while `search` exists. Reuse the search object for subsequent
queries.

## Search using the dictionary's rules

```cpp
auto result = search.search("つぐみ").value();
```

The defaults are `Headword`, `Prefix`, `Native`, with no result limit. For the
installed 大辞泉 edition, this finds both the bird 鶫 and a book titled つぐみ.
Search operates on stored keys, not the displayed headline text.

`Native` selects the supported search rules described by the product's metadata.
You do not select an algorithm class yourself. Depending on the dictionary and
scope, those rules can change which indexes are consulted, split a query into
terms and intersect their results, try inflections, or retry another scope.

| Dictionary / search | Behaviour under `Native` |
| --- | --- |
| 大辞泉 headwords | Includes supplementary headword indexes when present, as well as the main headword index. |
| 角川新字源 headwords | Searches both headword and idiom indexes. `Kanji` selects its headword index, while `Idiom` selects its idiom index. |
| 古語 dictionaries such as 全訳古語辞典 | Use their configured kana and numeral normalisation. Some scopes use character-component matching and sorting-headline filtering. |
| 詩語集 example and sense searches | Intersect character-component matches, with sorting-headline filtering for multi-character terms. |

The algorithm is not identical across these dictionaries. A scope's name alone
does not determine the lookup steps. Native rules are implemented for recognised
profiles, rather than by calling the native app. Linguistic support can vary by platform.

```cpp
bool recognised = search.supportsNativeSearch();
auto availableScopes = search.scopes(MKD::SearchFallback::Native);
```

An unrecognised native profile returns an error on native search. The support
check covers the class, delegate, and normalisation profile, not availability of
every scope or platform linguistic feature.

## Request an exact lookup without broadening

```cpp
auto exact = search.search("つぐみ", {
    .scope = MKD::SearchScope::Headword,
    .match = MKD::SearchMode::Exact,
    .fallback = MKD::SearchFallback::None,
}).value();
```

`None` treats the query as one normalised key and searches the selected scope's
indexes. It skips native term splitting, inflection retries, supplementary-index
selection, and scope fallbacks. Query normalisation still follows the dictionary's
known profile, so exact matching is not byte-for-byte equality.

**Use `None` when the requested match and scope must stay fixed.** Under `Native`,
an unsuccessful exact lookup can retry as a prefix lookup. An unsuccessful
headword search can also try idioms and, for applicable Japanese queries, examples.
These retries depend on the dictionary and query.

Direct lookup can still work when the native class or delegate is unrecognised,
provided the normalisation profile is known. It is not a bypass for unknown
normalisation.

## Prefix and suffix matching

```cpp
auto beginning = search.search("つぐ", {
    .match = MKD::SearchMode::Prefix,
    .fallback = MKD::SearchFallback::None,
}).value();

auto ending = search.search("ぐみ", {
    .match = MKD::SearchMode::Suffix,
    .fallback = MKD::SearchFallback::None,
}).value();
```

Both queries can find `つぐみ` when that key is present. Prefix matching compares
the start of a key, suffix matching its end. Records marked exact-only are
excluded from prefix and suffix results. An absent required index yields no matches.

## Select a scope

```cpp
auto directScopes = search.scopes(MKD::SearchFallback::None);

auto examples = search.search("鳥", {
    .scope = MKD::SearchScope::Example,
    .match = MKD::SearchMode::Prefix,
    .fallback = MKD::SearchFallback::None,
}).value();
```

| Scope | Indexed material |
| --- | --- |
| `Headword` | Entry spellings and readings |
| `Idiom` | Idioms and phrases |
| `Example` | Examples |
| `Sense` | Sense / definition keys |
| `Kanji` | Kanji lookup |
| `Fulltext` | The dictionary's full-text index, if supplied |

This is not a scan of article XML. A dictionary must supply the corresponding
index. `scopes(policy)` reports available scopes for that policy. Native and
direct scope lists can differ. A valid scope without an index produces no matches,
subject to native retries when enabled.

## Read results and understand fallbacks

```cpp
for (const auto& entry : result.entries)
{
    auto headline = dictionary->headlineForEntryId(entry).value();
    auto xml = dictionary->entryUtf8ById(entry.pageId).value();
}

auto effectiveScope = result.scope;
for (const auto& match : result.matches)
{
    auto key = match.key;
    auto mode = match.match;
    auto origin = match.origin;
}
```

`entries` contains ordered page/anchor targets. A headline can contain native
formatting tokens. `entryUtf8ById()` returns the complete article XML as an owning
UTF-8 string, including when the target refers to an anchor within it.

`scope` is the scope that produced the results. `matches` describes the normalised
query terms or variations that contributed to the search, including their actual
match mode and origin (`Query`, `Inflection`, `Compound`, or `JoinedQuery`). It is
search-level information, not a list of matching substrings for each entry.

Results follow stored index order and applicable native ordering rules, not a
relevance score. No matches is a successful empty result.

## Limit or cancel a search

```cpp
std::stop_source cancellation;
auto limited = search.search("つ", {.limit = 20}, cancellation.get_token());
```

`limit = 0` means unlimited. Limits apply after filtering and ordering, and
`result.truncated` reports omitted results. This is a result-size limit, not a
bound on work performed. Another thread can call `cancellation.request_stop()`.
Cancellation returns an error. Calls on the same search object may run concurrently
and have independent cancellation tokens.

## Inspect an index directly

Use `DictionarySearch` for dictionary-aware queries. To inspect the keys and
postings of one particular index, use the lower-level keystore API:

```cpp
const auto& store = dictionary->keystores().front();
auto hits = MKD::keystoreSearchResults(store, "つぐみ", MKD::SearchMode::Exact).value();
```

Select the intended store by its `scope()` or `filename()` rather than assuming
the first one is the headword index. This API compares keys in that store without
`DictionarySearch`'s query normalisation or native rules. Supply a key already
normalised for that index. Returned key views borrow the keystore.
