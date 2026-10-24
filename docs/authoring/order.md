# Choose article order

[![例解学習国語 offers the neighbouring articles すずめのなみだ and すすめ beside 雀.](../assets/guide/rgko-order.png)](../assets/guide/rgko-order.png)

例解学習国語 offers the neighbouring articles すずめのなみだ and すすめ beside 雀.

Article order controls the previous and next article buttons. It does not reorder search results or category indexes.

## Choose browsing order

Extend the first guide’s one-article dictionary. Here `target` is its 鶫 entry.
Add 雀, then place it before 鶫.

=== "Python"

    ```python
    sparrow = dictionary.create_entry()
    sparrow_target = sparrow.target
    sparrow.write_xml("""<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5">
      <head><title>雀</title></head>
      <body><h1>雀</h1></body>
    </html>""")
    dictionary.add_headline(mkd.HeadlineComponents("雀", sparrow_target))
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "すずめ", sparrow_target)
    dictionary.set_entry_order([sparrow_target.page_id, target.page_id])
    ```

=== "C++"

    ```cpp
    auto sparrow = dictionary.createEntry().value();
    const auto sparrowTarget = sparrow.target();
    sparrow.writeXml(R"(<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5">
      <head><title>雀</title></head>
      <body><h1>雀</h1></body>
    </html>)").value();
    dictionary.addHeadline({.headline = u"雀", .entryId = sparrowTarget}).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "すずめ", sparrowTarget).value();
    const std::array order{sparrowTarget.pageId, target.pageId};
    dictionary.setEntryOrder(order).value();
    ```

An explicit order must contain every page ID exactly once. The default is
ascending page ID. This writes the browsing order used by Dictionaries. It does not reorder search
results or the resource-store iteration sequence.

Finish the dictionary and product after adding the desired content.

After reopening, use [browsing order](../reading/order.md#follow-browsing-order)
to retrieve this sequence or find the previous and next article.

Next: [Finish and install](publishing.md).
