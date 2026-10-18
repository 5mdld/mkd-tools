[Documentation](../README.md)

# Reading dictionaries

## Open a product and select a dictionary

Use the path to a product folder. Here `dictionariesDirectory` /
`dictionaries_directory` is the directory containing installed products.
C++ snippets use `.value()` for readability. Error handling is covered below.

=== "Python"

    ```python
    import mkd

    product = mkd.DictionaryProduct.open(dictionaries_directory / "DAIJISEN2")
    dictionary = product.find_content("DAIJISEN.J")
    if dictionary is None:
        raise LookupError("DAIJISEN.J is not present in this product")
    ```

=== "C++"

    ```cpp
    auto product = MKD::DictionaryProduct::openAtPath(
        dictionariesDirectory / "DAIJISEN2").value();
    const auto* dictionary = product.findContent("DAIJISEN.J");
    if (!dictionary)
        return std::unexpected("DAIJISEN.J is not present in this product.");
    ```

For another product, inspect `product.dictionaries()` / `product.dictionaries`
and each dictionary's `id()` / `id`. Opening loads metadata and resource indexes.
Article and media payloads are read on demand.

## Read an article

Read the first stored article, or use a page ID obtained from a search result:

=== "Python"

    ```python
    first_id = dictionary.entries.id_at(0)  # No payload read.
    text = dictionary.entry_xml(first_id)
    ```

=== "C++"

    ```cpp
    auto firstId = dictionary->entries()->idAt(0).value();
    auto text = dictionary->entryUtf8ById(firstId).value();
    ```

The returned text is an owning string. This example assumes the selected
dictionary contains articles.

## Walk through articles

=== "Python"

    ```python
    if dictionary.entries is not None:
        for page in dictionary.entries:
            page_id = page.id
            xml = page.data.utf8()
    ```

=== "C++"

    ```cpp
    for (auto item : dictionary->entryItems()) {
        const auto& page = item.value();
        uint32_t pageId = page.itemId;
        auto xml = page.data.utf8().value();
        // xml borrows page.data. Process it before page leaves scope.
    }
    ```

Each step reads or decompresses one page. Earlier pages are not kept unless you
retain them. Page IDs may have gaps. Storage order can differ from browsing
order in the app.

## Read graphics and audio

=== "Python"

    ```python
    if dictionary.graphics is not None:
        graphics = dictionary.graphics
        name = graphics.id_at(0)
        data = graphics.get(name)

    if dictionary.audio is not None:
        for clip in dictionary.audio:
            print(clip.id, len(clip.data))
    ```

=== "C++"

    ```cpp
    if (const auto* graphics = dictionary->graphics()) {
        for (size_t i = 0; i < graphics->size(); ++i) {
            auto name = graphics->idAt(i).value();
            auto data = graphics->get(name).value();
        }
    }
    if (const auto* audio = dictionary->audio()) {
        std::visit([](const auto& store) {
            for (auto item : store.items()) {
                auto data = item.value().data;
            }
        }, *audio);
    }
    ```

Graphics have string IDs. Audio may have string IDs or numeric IDs in legacy
stores. Both Python store types use the same sequence and `get(id)` interface.
Absent stores are null pointers / `None`.

For a known content-relative path, use `resource("gaiji/rare.svg")` or
`resource("graphics/bird.svg")`. This also maps loose assets. It is not a recursive
file listing and does not resolve numeric legacy audio IDs.

## Read fonts and stylesheets

=== "Python"

    ```python
    for font in dictionary.fonts:
        font.write_to(output_directory / font.filename)

    if dictionary.stylesheet is not None:
        text = dictionary.stylesheet.data
    ```

=== "C++"

    ```cpp
    for (const auto& font : dictionary->fonts())
        font.writeTo(outputDirectory / font.filename()).value();
    if (const auto* css = dictionary->stylesheet()) {
        auto text = css->data();
    }
    ```

A font can be an ordinary mapped file or several packed fragments. Opening
identifies its format and resolves `filename()` / `filename`,
including `.ttf`, `.otf` or `.ttc`. `name()` / `name` is the resource name without
that extension.

Path writes stream the fragments, return the byte count and refuse existing
files. The output directory must already exist. Partial new files are removed
if writing fails. To use an already open Python file, retain direct buffer access:

```python
with open(output_directory / font.filename, "wb") as output:
    for fragment in font:
        output.write(fragment)
```

The C++ stream overload and `fragments()` remain available. Both approaches avoid
assembling a contiguous font buffer. Night-mode CSS is available through
`nightmodeStylesheet()` / `nightmode_stylesheet`.

## Find installed products

On macOS, the library provides the expected installed location:

=== "Python"

    ```python
    data_path = mkd.monokakido_data_directory()
    dictionaries_directory = mkd.dictionaries_directory(data_path)
    ```

=== "C++"

    ```cpp
    auto dataPath = MKD::macOS::monokakidoDataDirectory().value();
    dictionariesDirectory = MKD::macOS::dictionariesDirectory(dataPath);
    ```

Set this directory before opening a product. These helpers return paths without
checking existence or granting access.
On any platform, you can supply a directory of your own.

=== "Python"

    ```python
    source = mkd.DirectoryDictionarySource(dictionaries_directory)
    for info in source.find_all_available():
        print(info.id, info.path)

    info = source.find_by_id("DAIJISEN2")
    ```

=== "C++"

    ```cpp
    MKD::DirectoryDictionarySource source(dictionariesDirectory);
    for (const auto& info : source.findAllAvailable().value())
        std::cout << info.id << ": " << info.path << '\n';

    auto info = source.findById("DAIJISEN2").value();
    ```

Discovery checks for product files. A missing product in `findById()` /
`find_by_id()` is an error. Use `loadMetadata()` / `load_metadata()` instead of
opening the full product when you only need metadata.

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

Use [Searching](searching.md) for query normalisation and native search rules.
Reading an index directly does not apply those rules. Stored keys have already
been normalised. For example, an authored `すずめ` key may be stored as `スズメ`.

## Errors and return values

Fallible C++ operations return `MKD::Result<T>`, an alias for
`std::expected<T, std::string>`. Python operations raise `mkd.Error`.

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
    `close()`.

=== "C++"

    Snippets use `.value()` to keep API calls visible. In application code,
    handle errors before accessing the value:

    ```cpp
    auto opened = MKD::DictionaryProduct::openAtPath(productPath);
    if (!opened)
        return std::unexpected(opened.error());

    auto product = std::move(*opened);
    ```

    Return errors from a function returning `MKD::Result<T>`. Keep the product
    alive while using its dictionaries, and the dictionary alive at the same
    address while using its search object.

Next: [Searching](searching.md).
