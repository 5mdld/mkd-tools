[Documentation](../README.md)

# Creating a dictionary

Create a product with one article about 鶫（つぐみ）, searchable by its spelling
and reading. Use a new directory for `destination`.

For a more complete reference, see [Birb dictionaries](https://github.com/kiwakiwaa/mkd-tools/tree/main/examples/birbs).
It has two dictionary products (general and kanwa profiles) with subentries, search
keys, ruby, gaiji, media, styling and appendix menus.

## Create the product and its dictionary

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

=== "Python"

    ```python
    article_xml = """<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>"""

    entry = dictionary.create_entry()
    target = entry.target
    entry.write_xml(article_xml)
    ```

=== "C++"

    ```cpp
    constexpr std::string_view articleXml = R"(<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>)";

    auto entry = dictionary.createEntry().value();
    const auto target = entry.target();
    entry.writeXml(articleXml).value();
    ```

Supply a complete UTF-8 XML document using the native article markup shown above.
The namespace is part of that markup. `createEntry()` / `create_entry()` allocates
a page ID. `target` refers to the whole article.

## Give the article a headline

=== "Python"

    ```python
    dictionary.add_headline(mkd.HeadlineComponents(
        headline="鶫",
        sorting_headline="つぐみ",
        entry_id=target,
    ))
    ```

    Headline fields accept Python strings and convert them automatically.

=== "C++"

    ```cpp
    dictionary.addHeadline({
        .headline = u"鶫",
        .sortingHeadline = u"つぐみ",
        .entryId = target,
    }).value();
    ```

    Headline fields use UTF-16.

Every page needs a primary headline. `sortingHeadline` / `sorting_headline`
controls headline ordering. Neither field comes from the XML heading.

## Add spelling and reading keys

=== "Python"

    ```python
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "鶫", target)
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "つぐみ", target)
    ```

=== "C++"

    ```cpp
    dictionary.addSearchKey(MKD::SearchScope::Headword, "鶫", target).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "つぐみ", target).value();
    ```

Search keys use UTF-8. Both keys point to the same article. Neither the headline
nor `sortingHeadline` automatically adds search keys. The writer normalises
these keys using its supported default native profile.

## Publish the files

=== "Python"

    ```python
    dictionary.finish()
    product.finish()
    ```

=== "C++"

    ```cpp
    dictionary.finish().value();
    product.finish().value();
    ```

Finish each dictionary before finishing the product. Dictionary completion checks
required content and references. Product completion publishes the staged files
at `destination`. The generated product can then be opened with
`DictionaryProduct::openAtPath(destination)`.

Destruction does not publish. Abandoning a build removes its staging files. After
a mutating operation or completion fails, start a fresh build. Writers are used
serially, and input strings and byte spans are borrowed only for the call.

## Search the finished dictionary

=== "Python"

    ```python
    opened = mkd.DictionaryProduct.open(destination)
    birds = opened.find_content("example.birds.ja")
    search = mkd.DictionarySearch(birds)

    result = search.search(
        "つぐみ", match=mkd.SearchMode.EXACT, fallback=mkd.SearchFallback.NONE,
    )
    ```

    `result[0]` equals the allocated `target`.

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

    `result.entries` contains the allocated `target`.

Searching for `鶫` returns the same target.
See [Searching](../reading/searching.md) for scope selection, match modes, and
native dictionary rules.

## Optional build settings

Apply these changes to the earlier steps before finishing the writers.

### Restrict a key to exact matching

=== "Python"

    ```python
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "とり", target, exact_only=True)
    ```

=== "C++"

    ```cpp
    dictionary.addSearchKey(MKD::SearchScope::Headword, "とり", target, {.exactOnly = true}).value();
    ```

This key participates in exact lookups and is excluded from prefix and suffix
matching.

### Encryption and validation

To encrypt article contents, pass `{.encryptContents = true}` as the second
argument to `product.addDictionary(content, options)` in C++, or
`encrypt_contents=True` to `product.add_dictionary(content, ...)` in Python.

The default creation profile is general search. The next guide shows the kanwa
profile. Full-text keys must also be supplied explicitly.

Completion checks that:

- Every page has a primary headline and the dictionary has headword search keys.
- Search, headline, link and appendix targets resolve to existing pages or anchors.
- Referenced local assets and configured presentation stylesheets exist.
- Any explicit browsing order contains every page exactly once.

Validation covers supported references and resources, rather than every native
presentation feature. Product-specific delegates and headline-key identifier
authoring are not supported.

### Import explicit IDs

Automatic allocation is enough for ordinary authoring. When importing existing
content or preserving IDs across reordered builds, pass a page ID to
`createEntry(10)` / `create_entry(page_id=10)`. `addPage()` / `add_page()` also
accepts complete XML pages with explicit IDs.

Next: [Subentries and links](subentries.md).
