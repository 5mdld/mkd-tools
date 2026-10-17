[Documentation](README.md)

# Core concepts

## A product contains dictionaries

A **product** is an installed package, such as 大辞泉. A **dictionary** is one
searchable collection within that package, with its own articles, headlines,
indexes, and resources. A product can contain more than one dictionary.

These are the relevant parts of an installed 大辞泉 product:

```text
DAIJISEN2/                      # Product root passed to DictionaryProduct::openAtPath()
├── ContentInfo.plist           # Identifies the installed product
└── Contents/                   # Contains product metadata and dictionary directories
    ├── DAIJISEN2.json          # Product metadata, including its dictionaries
    └── Shogakukan-DJS/         # One dictionary's content directory
        ├── DSContentInfo.plist # Metadata for this dictionary
        ├── contents/           # Article XML stored in resource files
        ├── headline/           # Display labels and sorting text
        ├── key/                # Search indexes connecting keys to entry IDs
        ├── graphics/           # Images referenced by the content
        └── gaiji/              # Character images
```

`Contents/` is a container inside the product. It is not itself a dictionary.
`Shogakukan-DJS` is a directory name, while `DAIJISEN.J` is the dictionary's
content identifier used by the API:

=== "C++"

    ```cpp
    auto product = MKD::DictionaryProduct::openAtPath(
        dictionariesDirectory / "DAIJISEN2").value();

    const auto* dictionary = product.findContent("DAIJISEN.J");
    ```

=== "Python"

    ```python
    product = mkd.DictionaryProduct.open(dictionaries_directory / "DAIJISEN2")
    dictionary = product.find_content("DAIJISEN.J")
    ```

`findContent()` returns `nullptr` if the identifier is absent. For other products,
inspect `product.dictionaries()` and each dictionary's `id()` and `content()`.
The writer generates the on-disk layout for you.

## An entry connects search keys, a headline, and an article

For a small dictionary we create ourselves:

| Search key | Target | Headline | Article |
| --- | --- | --- | --- |
| `つぐみ` | `EntryId{1, 0}` | 鶫 | A definition of the bird |
| `鶫` | `EntryId{1, 0}` | 鶫 | The same article |

The **article** is the XML document to read. The **headline** is its label in a
result list. A **search key** is an indexed spelling or reading that locates it.
Multiple keys can point to the same entry, and one key can find multiple entries.
For example, 大辞泉's `つぐみ` also finds a book title.

When writing a dictionary, supply each part explicitly:

=== "C++"

    ```cpp
    dictionary.addPage(1, articleXml).value();
    dictionary.addHeadline({
        .headline = u"鶫",
        .sortingHeadline = u"つぐみ",
        .entryId = {1, 0},
    }).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "つぐみ", {1, 0}).value();
    ```

=== "Python"

    ```python
    target = mkd.EntryId(1, 0)
    dictionary.add_page(1, article_xml)
    dictionary.add_headline(mkd.HeadlineComponents(
        headline="鶫", sorting_headline="つぐみ", entry_id=target,
    ))
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "つぐみ", target)
    ```

`sortingHeadline` is separate sorting text. It does not add a searchable reading.
C++ article XML and search keys use UTF-8, and headline fields use UTF-16.
Python accepts Unicode strings for all text fields.

## A target can be a page or an anchor

=== "C++"

    ```cpp
    MKD::EntryId article{1, 0};
    MKD::EntryId exampleInsideArticle{1, 10};
    ```

=== "Python"

    ```python
    article = mkd.EntryId(1, 0)
    example_inside_article = mkd.EntryId(1, 10)
    ```

`pageId` identifies the XML document. `itemId == 0` selects the whole page.
A nonzero `itemId` selects an anchor in that page, such as an individual example
or sense. The XML anchor for `{1, 10}` is `id="1-000A"`: decimal page ID,
hexadecimal anchor ID.

Reading by `pageId` retrieves the complete XML document, even when the search
result points to an anchor. Keep `itemId` if your application needs to locate
that passage.

## Resources accompany the text

Articles and headlines can reference images, audio, CSS, fonts, and **gaiji**
(character images used in place of ordinary text glyphs). These resources belong
to the dictionary, alongside its articles and indexes.

Continue with [searching](reading/searching.md) or
[creating a dictionary](creating/minimal_dictionary.md).
