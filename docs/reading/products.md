# Open a dictionary

[![Each icon in the collection opens a dictionary product.](../assets/guide/collection.png)](../assets/guide/collection.png)

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
    auto dictionariesDirectory = MKD::macOS::dictionariesDirectory(dataPath);
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

Next: [Read articles and headlines](articles.md).
