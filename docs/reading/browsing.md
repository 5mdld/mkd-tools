# Reading dictionaries

## Find and open a product

A source scans a directory containing product folders. Discovery checks for the
expected product files; opening a product loads its metadata and resource indexes,
with payloads read on demand.

=== "C++"

    ```cpp
    auto source = MKD::DirectoryDictionarySource(dictionariesPath);
    for (const auto& info : source.findAllAvailable().value()) {
        std::cout << info.id << ": " << info.path << '\n';
    }

    auto info = source.findById("DAIJISEN2").value();
    auto product = MKD::DictionaryProduct::openAtPath(info.path).value();
    const auto* dictionary = product.findContent("DAIJISEN.J");
    ```

=== "Python"

    ```python
    source = mkd.DirectoryDictionarySource(dictionaries_path)
    for info in source.find_all_available():
        print(info.id, info.path)

    info = source.find_by_id("DAIJISEN2")
    product = mkd.DictionaryProduct.open(info.path)
    dictionary = product.find_content("DAIJISEN.J")
    ```

Product IDs identify folders; content identifiers select a dictionary within a
product. A missing product in `findById` / `find_by_id` is an error. A missing
content identifier returns a null pointer / `None`.

On macOS, obtain the expected installed location with:

=== "C++"

    ```cpp
    auto dataPath = MKD::macOS::monokakidoDataDirectory().value();
    auto dictionariesPath = MKD::macOS::dictionariesDirectory(dataPath);
    ```

=== "Python"

    ```python
    data_path = mkd.monokakido_data_directory()
    dictionaries_path = mkd.dictionaries_directory(data_path)
    ```

These helpers return expected paths; they do not check existence or grant access.
For dictionaries elsewhere, supply your own path on any platform.
`loadMetadata` / `load_metadata` opens metadata only, so use the full open operation
when you need articles or resources.

## Walk through articles

=== "C++"

    ```cpp
    for (auto item : dictionary->entryItems()) {
        const auto& page = item.value();
        uint32_t pageId = page.itemId;
        auto xml = page.data.utf8().value();
        // xml borrows page.data. Process it before page leaves scope.
    }
    ```

=== "Python"

    ```python
    if dictionary.entries is not None:
        for page in dictionary.entries:
            page_id = page.id
            xml = page.data.utf8()
    ```

Each step reads or decompresses one requested page. Page IDs may have gaps;
`entry_count` is a count, not the largest ID. Storage order is not necessarily
native browsing order or search-result order.

=== "C++"

    ```cpp
    auto firstId = dictionary->entries()->idAt(0).value();
    auto page = dictionary->entryById(firstId).value();
    auto text = dictionary->entryUtf8ById(firstId).value();
    ```

=== "Python"

    ```python
    first_id = dictionary.entries.id_at(0)  # No payload read.
    page = dictionary.entries[0]           # Position, not page ID.
    data = dictionary.entries.get(first_id)
    text = dictionary.entry_xml(first_id)
    ```

Python stores support `len()`, negative positional indexes and lazy iteration.
`get(id)` reads by ID. Returned data is a retained `Buffer`; `memoryview(data)`
adds no payload copy. Iteration does not retain previously visited pages unless
you keep them yourself. Decoding to text allocates an owning Python string.

## Read graphics and audio

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

Graphics have string IDs. Audio may have string IDs or numeric IDs in legacy
stores. Both Python store types use the same sequence and `get(id)` interface.
Absent stores are null pointers / `None`.

For a known content-relative path, use `resource("gaiji/rare.svg")` or
`resource("graphics/bird.svg")`. This also maps loose assets. It is not a recursive
file listing and does not resolve numeric legacy audio IDs.

## Read fonts and stylesheets

=== "C++"

    ```cpp
    for (const auto& font : dictionary->fonts())
        font.writeTo(outputDirectory / font.filename()).value();
    if (const auto* css = dictionary->stylesheet()) {
        auto text = css->data();
    }
    ```

=== "Python"

    ```python
    for font in dictionary.fonts:
        font.write_to(output_directory / font.filename)

    if dictionary.stylesheet is not None:
        text = dictionary.stylesheet.data
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

## Inspect headlines and search keys

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

Headline records own their component strings. Python keys are owning strings.
C++ key views borrow their store. Targets default to resolved entry IDs; request
`KeystoreIdMapping::Stored` / `KeystoreIdMapping.STORED` only when inspecting the
underlying conversion scheme. `keysForEntry()` / `keys_for_entry()` builds an
inverse index on first use and therefore uses additional memory.

Use [Searching](searching.md) for query normalisation and native search rules.
Reading an index directly does not apply those rules. Stored keys have already
been normalised; for example, an authored `すずめ` key may be stored as `スズメ`.
