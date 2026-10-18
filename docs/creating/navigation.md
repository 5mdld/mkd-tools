# Appendices and entry order

These snippets extend the writer in [Creating a dictionary](minimal_dictionary.md),
before either `finish()` call.

## Add an entry list

=== "Python"

    ```python
    entries = [mkd.AppendixEntry(target, label="鶫", first_character="つ")]
    dictionary.add_appendix_entries("birds", "鳥の一覧", entries)
    ```

=== "C++"

    ```cpp
    const std::array<MKD::AppendixEntry, 1> entries{{
        {.entryId = target, .label = "鶫", .firstCharacter = u'つ'},
    }};
    dictionary.addAppendix("birds", "鳥の一覧", entries).value();
    ```

This writes `appendix/birds.entries` and registers the list. Each item has an
entry target and optional display label and first-character grouping value.
The first character must fit in one UTF-16 code unit. Supply `rows` and `columns`
when you need a grid. The default is one column with the row count inferred.

After finishing and reopening the product, select its dictionary as
`openedDictionary` / `opened_dictionary`. Lists are available through `appendixEntryLists()` /
`appendix_entry_lists`. Read individual items with `entryAt(i)` / `entry_at(i)`.
C++ returns an `AppendixEntryView` whose label borrows the list. Call `copy()`
to obtain an owning `AppendixEntry` that can outlive it. Python returns the
owning value automatically.

=== "Python"

    ```python
    appendix = opened_dictionary.appendix_entry_lists[0]
    owned = appendix.entry_at(0)
    ```

=== "C++"

    ```cpp
    const auto& list = openedDictionary.appendixEntryLists().front();
    const auto view = list.entryAt(0).value();
    auto owned = view.copy();
    ```

## Register a document appendix

=== "Python"

    ```python
    dictionary.add_asset("appendix/Birds.pdf", mkd.Buffer.map("Birds.pdf"))
    dictionary.add_appendix("appendix/Birds.pdf", "この辞典について")
    ```

=== "C++"

    ```cpp
    auto document = MKD::RetainedSpan::map("Birds.pdf").value();
    dictionary.addAsset("appendix/Birds.pdf", document.span()).value();
    dictionary.addAppendix("appendix/Birds.pdf", "この辞典について").value();
    ```

Registration points to an existing or subsequently added asset. Supported document
extensions are `.html`, `.xhtml` and `.pdf`. Document appendices are registered in
the product's generated appendix listing. `appendix_entry_lists` exposes binary
entry lists, not document appendices.

## Add an alternative headline

=== "Python"

    ```python
    dictionary.add_headline(
        mkd.HeadlineComponents("つぐみ", target), role=mkd.HeadlineRole.SHORT,
    )
    ```

=== "C++"

    ```cpp
    dictionary.addHeadline({.headline = u"つぐみ", .entryId = target}, MKD::HeadlineRole::Short).value();
    ```

Primary, secondary and short headlines are stored separately for native consumers
to select. An alternative label does not replace the required primary headline or
add a search key. Use `HeadlineRole::Secondary` / `HeadlineRole.SECONDARY` for the
secondary store.

## Choose browsing order

In the two-article dictionary below, show 雀 before 鶫.

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
ascending page ID. This writes native browsing order. It does not reorder search
results or the resource-store iteration sequence.

Finish the dictionary and product after adding the desired content.
