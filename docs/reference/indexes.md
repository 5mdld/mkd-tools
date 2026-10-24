# Inspect stored indexes

Use these APIs to inspect stored data. Normal dictionary queries belong in [Searching](../searching/queries.md).

## Inspect headlines and search keys

=== "Python"

    ```python
    for store in dictionary.headlines:
        for headline in store:
            print(headline.headline, headline.entry_id)

    for store in dictionary.keystores:
        index = mkd.KeystoreIndex.PREFIX
        for i in range(store.index_size(index)):
            key = store.key_at(index, i)
            targets = store.entry_ids_at(index, i)
    ```

=== "C++"

    ```cpp
    for (const auto& store : dictionary->headlines()) {
        for (auto headline : store.entries()) {
            auto components = headline.value();
            auto target = components.entryId;
        }
    }
    for (const auto& store : dictionary->keystores()) {
        auto index = MKD::KeystoreIndex::Prefix;
        for (size_t i = 0; i < store.indexSize(index); ++i) {
            auto key = store.keyAt(index, i).value();
            auto targets = store.entryIdsAt(index, i).value();
        }
    }
    ```

Headline records own their component strings. Python keys are owning strings.
C++ key views borrow their store. Targets default to resolved entry IDs. Request
`KeystoreIdMapping::Stored` / `KeystoreIdMapping.STORED` only when inspecting the
underlying conversion scheme. `keysForEntry()` / `keys_for_entry()` builds an
inverse index on first use and therefore uses additional memory.

Use [Searching](../searching/queries.md) for query normalisation and dictionary-specific search rules.
Reading an index directly does not apply those rules. Stored keys have already
been normalised. For example, an authored `すずめ` key may be stored as `スズメ`.

## Decode a headline

=== "Python"

    ```python
    encoded = dictionary.headline_for_entry_id(target)
    text = mkd.InlineText.from_headline(encoded)
    plain = text.plain_text()
    ```

=== "C++"

    ```cpp
    auto components = dictionary->headlines().front().componentsForEntryId(target).value();
    auto text = MKD::InlineText::fromHeadline(components.full()).value();
    auto plain = text.plainText().value();
    ```

Uninterpreted headline commands are preserved as `HeadlineMarkup` for headline round trips.
Converting them to XML or plain text returns an error instead of guessing a
representation. `elements()` / `elements` lets you inspect recognised elements.

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
expects a stored, normalised key and does not apply dictionary-specific search rules. Returned
key views borrow the keystore.

The low-level keystore inspection API above is currently C++ only.
