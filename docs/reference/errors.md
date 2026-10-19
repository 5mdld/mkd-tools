# Errors and object lifetimes

These conventions apply throughout the Python and C++ APIs.

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
