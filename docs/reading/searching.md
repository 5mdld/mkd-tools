[Documentation](../README.md)

# Searching

Continue with 大辞泉 example opened in the previous guide.
Create a search object and reuse it for subsequent queries:

=== "C++"

    ```cpp
    MKD::DictionarySearch search(*dictionary);
    auto result = search.search("つぐみ").value();
    ```

=== "Python"

    ```python
    search = mkd.DictionarySearch(dictionary)
    result = search.search("つぐみ")
    ```

This searches headwords by prefix, using the dictionary's native rules.
In 大辞泉 it finds the bird 鶫 and a book titled つぐみ, among other entries
beginning with that reading. Search uses indexed keys, not the displayed XML text.

## Read the results

=== "C++"

    ```cpp
    for (const auto& entry : result.entries)
    {
        auto headline = dictionary->headlineForEntryId(entry).value();
        auto xml = dictionary->entryUtf8ById(entry.pageId).value();
    }
    ```

    `result.entries` contains ordered page/anchor targets. `entryUtf8ById()`
    returns the complete article XML as an owning UTF-8 string.

=== "Python"

    ```python
    for entry in result:
        headline = dictionary.headline_for_entry_id(entry)
        xml = dictionary.entry_xml(entry.page_id)
    ```

    The result is an iterable sequence of `EntryId` values. Use `len(result)`
    and `result[index]` to inspect it. `entry_xml()` returns the complete article
    XML as a Python string.

A headline can contain native formatting tokens. Article lookup returns the entire
page even when the target refers to an anchor within it.

## Choose a search scope

A scope selects what kind of material to search. These are the common choices:

| Scope | Use it for |
| --- | --- |
| `Headword` | Entry spellings and readings. Kanwa dictionaries use it for parent kanji entries. |
| `Idiom` | Phrases and idioms, or jukugo in kanwa dictionaries |
| `Example` | Example sentences and quotations |
| `Sense` | Keys associated with meanings or definitions |
| `Kanji` | Kanji lookup in dictionaries that provide it |

Names here use C++ spelling. Python uses uppercase names such as `HEADWORD`.
Ask the dictionary which scopes are available:

=== "C++"

    ```cpp
    auto availableScopes = search.scopes(MKD::SearchFallback::Native);
    auto examples = search.search("鳥", {.scope = MKD::SearchScope::Example}).value();
    ```

=== "Python"

    ```python
    available_scopes = search.scopes(fallback=mkd.SearchFallback.NATIVE)
    examples = search.search("鳥", scope=mkd.SearchScope.EXAMPLE)
    ```

Other scopes are available for dictionaries that supply those indexes:

| Scope | Indexed material |
| --- | --- |
| `English` | English keys |
| `Collocation` | Collocations |
| `Fulltext` | Full-text search keys |
| `Category` | Categories or groups |
| `CompoundNoun` | Compound nouns. Python calls this `COMPOUND`. |
| `Numeral` | Counters and numeral expressions |
| `Modern` | Specialised modern-language lookup |
| `CJ`, `JC` | Chinese–Japanese and Japanese–Chinese lookup |

Selecting a scope does not scan article text. The dictionary must contain its
index. When authoring, the general profile supports all scopes above except
`Modern`, `CJ` and `JC`. The kanwa profile supports `Headword`, `Idiom`, `Sense`
and `Example`.

## Prefix and suffix matching

Use `match` to choose prefix, suffix or exact lookup. Set `fallback` to `None`
when you want the match and scope to stay fixed. The default, `Native`, can try
variations according to the dictionary's rules.

=== "C++"

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

=== "Python"

    ```python
    beginning = search.search(
        "つぐ", match=mkd.SearchMode.PREFIX, fallback=mkd.SearchFallback.NONE,
    )
    ending = search.search(
        "ぐみ", match=mkd.SearchMode.SUFFIX, fallback=mkd.SearchFallback.NONE,
    )
    ```

Both queries can find `つぐみ` when that key is present. Prefix matching compares
the start of a key, suffix matching its end. Records marked exact-only are
excluded from prefix and suffix results. An absent required index yields no matches.

## Request an exact lookup without broadening

=== "C++"

    ```cpp
    auto exact = search.search("つぐみ", {
        .scope = MKD::SearchScope::Headword,
        .match = MKD::SearchMode::Exact,
        .fallback = MKD::SearchFallback::None,
    }).value();
    ```

=== "Python"

    ```python
    exact = search.search(
        "つぐみ",
        scope=mkd.SearchScope.HEADWORD,
        match=mkd.SearchMode.EXACT,
        fallback=mkd.SearchFallback.NONE,
    )
    ```

Exact lookup still normalises the query using the dictionary's rules. It does
not require byte-for-byte equality with the stored key.

With `Native`, an unsuccessful exact lookup may retry as a prefix lookup.
A headword query may also try idioms or examples. Use `None` to skip these retries
and native query expansion.

## Dictionary-specific search rules

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

=== "C++"

    ```cpp
    bool recognised = search.supportsNativeSearch();
    ```

=== "Python"

    ```python
    recognised = search.supports_native_search
    ```

An unrecognised native profile returns an error on native search. Direct lookup
with `None` can still work if the dictionary's normalisation is supported.

## Inspect how a query matched

=== "C++"

    ```cpp
    auto effectiveScope = result.scope;
    for (const auto& match : result.matches)
    {
        auto key = match.key;
        auto mode = match.match;
        auto origin = match.origin;
    }
    ```

=== "Python"

    ```python
    effective_scope = result.scope
    for match in result.matches:
        key, mode, origin = match.key, match.match, match.origin
    ```

`scope` is the scope that produced the results. `matches` describes the normalised
query terms or variations that contributed to the search, including their actual
match mode and origin (`Query`, `Inflection`, `Compound`, or `JoinedQuery`). It is
search-level information, not a list of matching substrings for each entry.

Results follow stored index order and applicable native ordering rules, not a
relevance score. No matches is a successful empty result.

## Limit or cancel a search

=== "C++"

    ```cpp
    std::stop_source cancellation;
    auto limited = search.search("つ", {.limit = 20}, cancellation.get_token());
    ```

    Another thread can call `cancellation.request_stop()`. Cancellation returns
    an error. Each call has an independent cancellation token.

=== "Python"

    ```python
    limited = search.search("つ", limit=20)
    ```

    Python searches currently have no cancellation token argument.

`limit = 0` means unlimited. Limits apply after filtering and ordering, and
`result.truncated` reports omitted results. This is a result-size limit, not a
bound on work performed. Calls on the same search object may run concurrently.

## Inspect an index directly

Use `DictionarySearch` for dictionary-aware queries. To inspect the keys and
postings of one particular index, use the lower-level keystore API:

=== "C++"

    ```cpp
    for (const auto& store : dictionary->keystores())
    {
        if (store.scope() != MKD::SearchScope::Headword
            || store.indexSize(MKD::KeystoreIndex::Prefix) == 0)
            continue;

        auto key = store.keyAt(MKD::KeystoreIndex::Prefix, 0).value();
        auto hits = MKD::keystoreSearchResults(store, key, MKD::SearchMode::Exact).value();
    }
    ```

This looks up a key taken directly from a headword index. The lower-level API
expects a stored, normalised key and does not apply native search rules. Returned
key views borrow the keystore.

The low-level keystore inspection API above is currently C++ only.

Next: [Exporting dictionaries](exporting.md).
