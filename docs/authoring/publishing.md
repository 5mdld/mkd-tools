# Finish, install and update

[![Each icon in the collection opens a dictionary product.](../assets/guide/collection.png)](../assets/guide/collection.png)

Each icon in the collection opens a dictionary product.

Finish the product, then install its folder in Dictionaries on macOS.

## Publish the files

=== "Python"

    ```python
    dictionary.finish()
    product.finish()
    ```

=== "C++"

    ```cpp
    dictionary.finish().value();
    product.finish().value();
    ```

Finish each dictionary before finishing the product. Dictionary completion checks
required content and references. Product completion publishes the staged files
at `destination`. The generated product can then be opened with
`DictionaryProduct::openAtPath(destination)`.

Destruction does not publish. Abandoning a build removes its staging files. After
a mutating operation or completion fails, start a fresh build. Writers are used
serially, and input strings and byte spans are borrowed only for the call.

## Encryption and validation

To encrypt article contents, pass `{.encryptContents = true}` as the second
argument to `product.addDictionary(content, options)` in C++, or
`encrypt_contents=True` to `product.add_dictionary(content, ...)` in Python.

The default creation profile is general search. See [profiles](../reference/search_profiles.md)
for kanwa authoring. Full-text keys must also be supplied explicitly.

Completion checks that:

- Every page has a primary headline and the dictionary has headword search keys.
- Search, headline, link and appendix targets resolve to existing pages or anchors.
- Referenced local assets and configured presentation stylesheets exist.
- Any explicit browsing order contains every page exactly once.

Validation covers supported references and resources, rather than every native
presentation feature. Product-specific delegates and headline-key identifier
authoring are not supported.

## Install and replace

Quit Dictionaries before installation. Run the CLI against the finished product:

```sh
mkd-tools install build/Birds
```

Build a replacement in another directory with the same final folder name, then:

```sh
mkd-tools install build/next/Birds --replace
```

Installation copies the package and adds local activation for that custom product.
An existing activation is preserved. It accepts packages created by mkd and is
intended for custom dictionaries you are allowed to use.

Keep product and content identifiers stable across updates. If you need links or
bookmarks to survive changed article order, preserve the affected IDs when
[importing content](../reference/authoring_options.md#import-explicit-ids).

Verify search, links, navigation, media and both article presentations in the
actual app. The writer checks references, but it does not reproduce native layout.
