# Write article XML

[![大辞泉’s 鶫 article is an XML document rendered by the app.](../assets/guide/daijisen-article.png)](../assets/guide/daijisen-article.png)

大辞泉’s 鶫 article is an XML document rendered by the app.

The article pane renders XML independently of the headline shown in the result list.
Use the unfinished `dictionary` from [the first dictionary](first_dictionary.md).

## Add an article

=== "Python"

    ```python
    article_xml = """<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>"""

    entry = dictionary.create_entry()
    target = entry.target
    entry.write_xml(article_xml)
    ```

=== "C++"

    ```cpp
    constexpr std::string_view articleXml = R"(<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
    <head><title>鶫</title></head>
    <body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
    </html>)";

    auto entry = dictionary.createEntry().value();
    const auto target = entry.target();
    entry.writeXml(articleXml).value();
    ```

Supply a complete UTF-8 XML document using the article XML markup shown above.
The namespace is part of that markup. `createEntry()` / `create_entry()` allocates
a page ID. `target` refers to the whole article.

## Structure the document

Use UTF-8 XML with the XHTML namespace and `hmhtml="5"`. Put the title and
stylesheet links in `<head>`, then the displayed article in `<body>`.

```xml
<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
  <head>
    <title>鶫</title>
    <link rel="stylesheet" href="dictionary.css"/>
  </head>
  <body>
    <headword>鶫</headword>
    <meaning>冬に日本で見られる鳥。</meaning>
  </body>
</html>
```

You may choose your own element names. Custom elements begin inline, so give
sections `display: block` in [the stylesheet](styling.md). Add referenced assets
before finishing the dictionary. Escape XML special characters in imported text.
[InlineText](ruby.md) handles escaping when constructing fragments.

Each article also needs a [headline](headlines.md) and [search keys](search_keys.md).
Allocate any [subentry targets](subentries.md) before writing its XML.

Next: [Define headlines](headlines.md).
