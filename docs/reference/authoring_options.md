# Import IDs and build options

Automatic target allocation is the default for ordinary authoring.

## Import explicit IDs

Automatic allocation is enough for ordinary authoring. When importing existing
content or preserving IDs across reordered builds, pass a page ID to
`createEntry(10)` / `create_entry(page_id=10)`. `addPage()` / `add_page()` also
accepts complete XML pages with explicit IDs.

See [finishing and validation](../authoring/publishing.md#encryption-and-validation) for encryption and completion checks.
