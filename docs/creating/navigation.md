# Appendices and entry order

These snippets extend the writer in [Creating a dictionary](minimal_dictionary.md),
before either `finish()` call.

## Add an entry list

=== "C++"

    ```cpp
    const std::array<MKD::AppendixEntry, 1> entries{{
        {.entryId = target, .label = "鶫", .firstCharacter = u'つ'},
    }};
    dictionary.addAppendix("birds", "鳥の一覧", entries).value();
    ```

=== "Python"

    ```python
    entries = [mkd.AppendixEntry(target, label="鶫", first_character="つ")]
    dictionary.add_appendix_entries("birds", "鳥の一覧", entries)
    ```

This writes `appendix/birds.entries` and registers the list. Each item has an
entry target and optional display label and first-character grouping value.
The first character must fit in one UTF-16 code unit. Supply `rows` and `columns`
when you need a grid; the default is one column with the row count inferred.

After reopening, lists are available through `appendixEntryLists()` /
`appendix_entry_lists`. Read individual items with `entryAt(i)` / `entry_at(i)`.
C++ returns an `AppendixEntryView` whose label borrows the list. Call `copy()`
to obtain an owning `AppendixEntry` that can outlive it. Python returns the
owning value automatically.

=== "C++"

    ```cpp
    const auto view = list.entryAt(0).value();
    auto owned = view.copy();
    ```

=== "Python"

    ```python
    owned = appendix.entry_at(0)
    ```

## Register a document appendix

=== "C++"

    ```cpp
    auto document = MKD::RetainedSpan::map("Birds.pdf").value();
    dictionary.addAsset("appendix/Birds.pdf", document.span()).value();
    dictionary.addAppendix("appendix/Birds.pdf", "この辞典について").value();
    ```

=== "Python"

    ```python
    dictionary.add_asset("appendix/Birds.pdf", mkd.Buffer.map("Birds.pdf"))
    dictionary.add_appendix("appendix/Birds.pdf", "この辞典について")
    ```

Registration points to an existing or subsequently added asset. Supported document
extensions are `.html`, `.xhtml` and `.pdf`. Document appendices are registered in
the product's generated appendix listing; `appendix_entry_lists` exposes binary
entry lists, not document appendices.

## Add an alternative headline

=== "C++"

    ```cpp
    dictionary.addHeadline({.headline = u"つぐみ", .entryId = {1, 0}}, MKD::HeadlineRole::Short).value();
    ```

=== "Python"

    ```python
    dictionary.add_headline(
        mkd.HeadlineComponents("つぐみ", mkd.EntryId(1)), role=mkd.HeadlineRole.SHORT,
    )
    ```

Primary, secondary and short headlines are stored separately for native consumers
to select. An alternative label does not replace the required primary headline or
add a search key. Use `HeadlineRole::Secondary` / `HeadlineRole.SECONDARY` for the
secondary store.

## Choose browsing order

=== "C++"

    ```cpp
    dictionary.addPage(2, "<html><head/><body><h1>雀</h1></body></html>").value();
    dictionary.addHeadline({.headline = u"雀", .entryId = {2, 0}}).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "すずめ", {2, 0}).value();
    const std::array<uint32_t, 2> order{2, 1};
    dictionary.setEntryOrder(order).value();
    ```

=== "Python"

    ```python
    dictionary.add_page(2, "<html><head/><body><h1>雀</h1></body></html>")
    dictionary.add_headline(mkd.HeadlineComponents("雀", mkd.EntryId(2)))
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "すずめ", mkd.EntryId(2))
    dictionary.set_entry_order([2, 1])
    ```

An explicit order must contain every page ID exactly once. The default is
ascending page ID. This writes native browsing order; it does not reorder search
results or the resource-store iteration sequence.
