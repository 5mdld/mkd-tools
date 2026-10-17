# mkd library documentation

Read, search, and create Monokakido dictionaries with the C++ library and its Python bindings.

| Task | Guide |
| --- | --- |
| Understand products, dictionaries, and entries | [Core concepts](concepts.md) |
| Discover products and read articles, media and indexes | [Reading dictionaries](reading/browsing.md) |
| Choose a search policy, scope, and matching mode | [Searching](reading/searching.md) |
| Extract articles, media and index tables | [Exporting dictionaries](reading/exporting.md) |
| Write an article and make it searchable | [Creating a dictionary](creating/minimal_dictionary.md) |
| Add gaiji, ruby and entry links | [Inline content](creating/inline_content.md) |
| Add images, audio, fonts and stylesheets | [Resources](creating/resources.md) |
| Add appendices and choose browsing order | [Navigation](creating/navigation.md) |

## Using the snippets

Use `#include <MKD/mkd.hpp>` in C++ or `import mkd` in Python.
Snippets on each page build on the preceding ones.

Fallible C++ operations return `MKD::Result<T>`, an alias for
`std::expected<T, std::string>`. Python operations raise `mkd.Error`.

=== "C++"

    Snippets use `.value()` to keep API calls visible. In application code,
    handle errors before accessing the value:

    ```cpp
    auto opened = MKD::DictionaryProduct::openAtPath(productPath);
    if (!opened)
        return std::unexpected(opened.error());

    auto product = std::move(*opened);
    ```

    This error-propagation example belongs in a function returning `MKD::Result<T>`.
    Calling `.value()` on an error throws `std::bad_expected_access<std::string>`.
    Borrowed dictionaries and search objects require their owners to stay alive.

=== "Python"

    ```python
    try:
        product = mkd.DictionaryProduct.open(product_path)
    except mkd.Error as error:
        print(error)
    ```

    Missing content identifiers return `None`. Searches with no matches return
    an empty result. Paths accept strings and `pathlib.Path`. Metadata read from
    products and dictionaries is an editable copy, so editing it does not change
    the open dictionary or files.

    Dictionaries retain their product, search objects retain their dictionary,
    and dictionary writers retain their product writer. Readers need no explicit
    `close()`. Writers still require explicit `finish()` calls to publish.

Build instructions and CLI usage are in the
[project README](https://github.com/kiwakiwaa/mkd-tools#building).

## Python resource buffers

`entry_xml(page_id)` returns an owning Python string. Use `entry_data(page_id)`
or `resource(path)` to retain bytes without an extra payload copy:

```python
data = dictionary.entry_data(entry.page_id)
view = memoryview(data)
owned = bytes(data)
```

`view` is read-only and keeps its storage alive even after the dictionary and
`data` are deleted. `bytes(data)` explicitly copies. Compressed resources still
require decompression. Retained views can keep entire decompressed chunks alive.
`data.utf8()` decodes text, and `data.slice(offset, length)` retains a byte slice.
`mkd.Buffer.map(path)` maps an ordinary file.

Writer byte inputs accept contiguous one-dimensional byte buffers, including
`bytes`, `bytearray`, `memoryview`, and `mkd.Buffer`. Inputs are borrowed for the
call. Incompatible layouts are rejected. Opening, searching, and reading release
the GIL during native work. Writer calls retain the GIL and should be used serially.

## API map

| Work with | C++ and Python types |
| --- | --- |
| Discover products | `DirectoryDictionarySource`, `DictionaryInfo` |
| Open products and select dictionaries | `DictionaryProduct`, `Dictionary` |
| Describe content | `DictionaryMetadata`, `DictionaryContent`, `LocalizedString`, `DictionaryPresentation` |
| Search | `DictionarySearch`, `SearchResult`, `EntryId` |
| Read packed data | `ResourceStore`, `NamedResourceStore`, `RetainedSpan` (Python: `Buffer`) |
| Inspect labels and keys | `HeadlineStore`, `HeadlineComponents`, `Keystore` |
| Read presentation resources | `Font`, `Stylesheet`, `AppendixEntryList` |
| Export files | `exportToDirectory` (Python: `export_to_directory`), `ExportOptions`, `ExportResult` |
| Build products | `ProductWriter`, `DictionaryWriter` |
| Compose inline content | `InlineText`, `Ruby`, `Gaiji`, `GaijiResource`, `EntryLink` |

Python exposes resource stores through an open dictionary. Standalone store
opening/writing and purchase receipts remain C++ APIs. Python search has result
limits but no cancellation token. Creation currently supports the default native
search and key-normalisation profile, not every specialised installed-dictionary
configuration. See the creation guide for validation and supported scopes.

Python type stubs provide signatures for editors. `help(mkd.Dictionary)` and
`help(mkd.export_to_directory)` describe the main operations and constraints.
