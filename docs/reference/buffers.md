# Buffers and resource inputs

Use retained data when processing articles or resources without an extra payload copy.

## Retain article bytes

Use retained buffers when you need bytes without an extra payload copy.

=== "Python"

    ```python
    page = dictionary.entries[0]
    data = dictionary.entries.get(first_id)
    view = memoryview(data)
    owned = bytes(data)
    ```

    `entries[0]` reads by position. `get(first_id)` reads by page ID.
    Python stores support `len()`, negative positional indexes and lazy iteration.
    `entry_data(first_id)` is another way to obtain the article's buffer.

    `view` is read-only and keeps its storage alive. `bytes(data)` explicitly
    copies. `data.utf8()` returns an owning string, while
    `data.slice(offset, length)` retains a byte slice.

=== "C++"

    ```cpp
    auto page = dictionary->entryById(firstId).value();
    auto bytes = page.data.span();
    auto xml = page.data.utf8().value();
    ```

    The byte span and text view borrow `page.data`. Keep the owning data alive
    while using them.

Compressed resources require decompression. Retained buffers can keep an entire
decompressed chunk alive. Python releases the GIL during reading and searching.

## Resource inputs

Python writer methods accept `mkd.Buffer`, `bytes`, `bytearray` and contiguous
one-dimensional byte `memoryview` inputs. `mkd.Buffer.map(path)` maps a file.
Inputs are consumed during the call. Writer calls retain the GIL and should be
used serially.
