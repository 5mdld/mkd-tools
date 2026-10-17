[Documentation](../README.md) · [Core concepts](../concepts.md)

# Creating a dictionary

Create a product with one article about 鶫（つぐみ）, searchable by its spelling
and reading.

## Create the product and its dictionary

=== "C++"

    ```cpp
    auto product = MKD::ProductWriter::create(destination, {
        .displayName = MKD::LocalizedString{.en = "Birds", .ja = "鳥の辞典"},
        .version = "1.0",
    }).value();

    auto dictionary = product.addDictionary({
        .identifier = "example.birds.ja",
        .directory = "ja",
        .dictionaryId = 1000,
        .title = MKD::LocalizedString{.ja = "鳥の辞典"},
        .languages = std::vector<std::string>{"ja"},
    }).value();
    ```

    Keep `product` alive while using its dictionary writer.

=== "Python"

    ```python
    product = mkd.ProductWriter.create(
        destination,
        mkd.DictionaryMetadata(
            display_name=mkd.LocalizedString(en="Birds", ja="鳥の辞典"),
            version="1.0",
        ),
    )

    dictionary = product.add_dictionary(mkd.DictionaryContent(
        identifier="example.birds.ja",
        directory="ja",
        dictionary_id=1000,
        title=mkd.LocalizedString(ja="鳥の辞典"),
        languages=["ja"],
    ))
    ```

    The dictionary writer retains its product writer automatically.

`destination` is a new product directory, for example `/path/to/Birds`. The writer
refuses an existing destination. This creates one dictionary under `Contents/ja`.

| Value | Purpose |
| --- | --- |
| `Birds` in the destination path | Determines the product identifier `jp.monokakido.Dictionaries.Birds`. |
| `displayName` | Human-readable product name. |
| `identifier` | Stable textual identity for this dictionary's content. |
| `directory` | Content directory name inside `Contents/`. |
| `dictionaryId` | Nonzero 16-bit native dictionary ID. |

Content identifiers, directory names, and numeric dictionary IDs must be unique
within the product. Choose numeric IDs that do not collide with products you
intend to use together. `1000` is an example, not a reserved custom ID.

## Add an article

=== "C++"

    ```cpp
    constexpr std::string_view articleXml = R"(<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>)";

    dictionary.addPage(1, articleXml).value();
    ```

=== "Python"

    ```python
    article_xml = """<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>"""

    dictionary.add_page(1, article_xml)
    ```

Supply a complete UTF-8 XML document using the native article markup shown above.
The namespace is part of that markup. Page ID `1` is chosen by the caller and
must be nonzero. `EntryId{1, 0}` refers to this whole article.

## Give the article a headline

=== "C++"

    ```cpp
    dictionary.addHeadline({
        .headline = u"鶫",
        .sortingHeadline = u"つぐみ",
        .entryId = {1, 0},
    }).value();
    ```

    Headline fields use UTF-16.

=== "Python"

    ```python
    target = mkd.EntryId(1, 0)
    dictionary.add_headline(mkd.HeadlineComponents(
        headline="鶫",
        sorting_headline="つぐみ",
        entry_id=target,
    ))
    ```

    Headline fields accept Python strings and convert them automatically.

Every page needs a primary headline. The displayed label and sorting text are
independent of the article's `<h1>` element.

## Make both spellings searchable

=== "C++"

    ```cpp
    dictionary.addSearchKey(MKD::SearchScope::Headword, "鶫", {1, 0}).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "つぐみ", {1, 0}).value();
    ```

=== "Python"

    ```python
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "鶫", target)
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "つぐみ", target)
    ```

Search keys use UTF-8. Both keys point to the same article. Neither the headline
nor `sortingHeadline` automatically adds search keys. The writer normalises
these keys using its supported default native profile.

## Restrict a key to exact matching

=== "C++"

    ```cpp
    dictionary.addSearchKey(MKD::SearchScope::Headword, "とり", {1, 0}, {.exactOnly = true}).value();
    ```

=== "Python"

    ```python
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "とり", mkd.EntryId(1), exact_only=True)
    ```

This key participates in exact lookups and is excluded from prefix and suffix
matching.

## Creation options and validation

To encrypt article contents, pass `{.encryptContents = true}` as the second
argument to `product.addDictionary(content, options)` in C++, or
`encrypt_contents=True` to `product.add_dictionary(content, ...)` in Python.

The default profile is general search with default key normalisation. Supported
general creation scopes are headword, idiom, example, English,
sense, kanji, collocation, full text, category, compound and numeral. Specialised
modern, CJ and JC authoring profiles are not supported. Full-text keys, like other
keys, must be supplied explicitly; article text is not indexed automatically.

Completion checks that:

- Every page has a primary headline and the dictionary has headword search keys.
- Search, headline, link and appendix targets resolve to existing pages or anchors.
- Referenced local assets and configured presentation stylesheets exist.
- Any explicit browsing order contains every page exactly once.

XML is parsed and supported references are validated. This is not a complete
validator for every native presentation feature. Choose a creation profile instead of supplying native search-class strings.
Product-specific delegates and headline-key identifier authoring remain unsupported.

See [Gaiji, ruby and links](inline_content.md), [Resources](resources.md) and
[Appendices and entry order](navigation.md) for richer content.

## Create a kanwa product

=== "C++"

    ```cpp
    auto product = MKD::ProductWriter::create(
        destination, metadata, MKD::CreationProfile::Kanwa).value();
    ```

=== "Python"

    ```python
    product = mkd.ProductWriter.create(
        destination, metadata, profile=mkd.CreationProfile.KANWA,
    )
    ```

Kanwa uses the generic native search class without a product-specific delegate.
Its scopes are parent characters (`Headword` or `Kanji` keys), compounds
(`Idiom`), meanings (`Sense`) and examples (`Example`). The writer generates the
native scope names and index mappings. The general profile remains the default.

## Publish the files

=== "C++"

    ```cpp
    dictionary.finish().value();
    product.finish().value();
    ```

=== "Python"

    ```python
    dictionary.finish()
    product.finish()
    ```

Finish each dictionary before finishing the product. Dictionary completion checks
required content and references. Product completion publishes the staged files
at `destination`. The generated product can then be opened with
`DictionaryProduct::openAtPath(destination)`.

Destruction does not publish. Abandoning a build removes its staging files. After
a mutating operation or completion fails, start a fresh build. Writers are used
serially, and input strings and byte spans are borrowed only for the call.

## Search what you created

=== "C++"

    ```cpp
    auto opened = MKD::DictionaryProduct::openAtPath(destination).value();
    const auto* birds = opened.findContent("example.birds.ja");
    MKD::DictionarySearch search(*birds);

    auto result = search.search("つぐみ", {
        .match = MKD::SearchMode::Exact,
        .fallback = MKD::SearchFallback::None,
    }).value();
    ```

    `result.entries` contains `EntryId{1, 0}`.

=== "Python"

    ```python
    opened = mkd.DictionaryProduct.open(destination)
    birds = opened.find_content("example.birds.ja")
    search = mkd.DictionarySearch(birds)

    result = search.search(
        "つぐみ", match=mkd.SearchMode.EXACT, fallback=mkd.SearchFallback.NONE,
    )
    ```

    `result[0]` is `mkd.EntryId(1, 0)`.

Searching for `鶫` returns the same target.
See [Searching](../reading/searching.md) for scope selection, match modes, and
native dictionary rules.
