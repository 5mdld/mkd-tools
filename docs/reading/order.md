# Follow article order

[![例解学習国語 offers the neighbouring articles すずめのなみだ and すすめ beside 雀.](../assets/guide/rgko-order.png)](../assets/guide/rgko-order.png)

例解学習国語 offers the neighbouring articles すずめのなみだ and すすめ beside 雀.

The previous and next article controls follow dictionary order, not the order of a resource file.

## Follow browsing order

Use `entry_order` to browse articles in the dictionary's intended sequence.
A compound belongs to its parent article's page.

=== "Python"

    ```python
    order = dictionary.entry_order
    first_page = order[0]
    xml = dictionary.entry_xml(first_page)

    next_page = order.next(first_page)
    if next_page is not None:
        xml = dictionary.entry_xml(next_page)
    ```

=== "C++"

    ```cpp
    const auto& order = dictionary->entryOrder();
    auto firstPage = order.pageAt(0).value();
    auto xml = dictionary->entryUtf8ById(firstPage).value();

    auto nextPage = order.next(firstPage).value();
    if (nextPage)
        xml = dictionary->entryUtf8ById(*nextPage).value();
    ```

`previous()` and `next()` stop at the ends without wrapping. An unknown page ID
is an error. Search results keep their own sequence, including compound targets.
Order lookup reads no article XML.

Without an order file, the native fallback is page IDs `1` through the article
count. It assumes consecutive IDs. The writer always generates an order file,
including for dictionaries with gaps in their page IDs.

Next: [Export resources](exporting.md).
