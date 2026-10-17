# mkd library documentation

Read, search, and create Monokakido dictionaries with the C++ library.

| Task | Guide |
| --- | --- |
| Understand products, dictionaries, and entries | [Core concepts](concepts.md) |
| Choose a search policy, scope, and matching mode | [Searching](reading/searching.md) |
| Write an article and make it searchable | [Creating a dictionary](creating/minimal_dictionary.md) |

## Using the snippets

Include `<MKD/mkd.hpp>`. Snippets on each page build on the preceding ones.

Fallible operations return `MKD::Result<T>`, an alias for
`std::expected<T, std::string>`. Snippets use `.value()` to keep the API calls
visible. In application code, handle errors before accessing the value:

```cpp
auto opened = MKD::DictionaryProduct::openAtPath(productPath);
if (!opened)
    return std::unexpected(opened.error());

auto product = std::move(*opened);
```

This error-propagation example belongs in a function returning `MKD::Result<T>`.
Calling `.value()` on an error instead throws `std::bad_expected_access<std::string>`.

Build instructions and CLI usage are in the [project README](../README.md).
