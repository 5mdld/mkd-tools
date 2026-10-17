[Documentation](../README.md) · [Core concepts](../concepts.md)

# Creating a dictionary

Create a product with one article about 鶫（つぐみ）, searchable by its spelling
and reading. The headword occurs in 大辞泉. The short definition below is original
example text.

## Create the product and its dictionary

```cpp
auto product = MKD::ProductWriter::create(destination, {
    .displayName = MKD::LocalizedString{.en = "Birds", .ja = "鳥の辞典"},
    .version = "1.0",
}).value();

auto dictionary = product.addDictionary({
    .identifier = "example.birds.ja",
    .directory = "ja",
    .dictionaryId = 1000,
    .title = MKD::LocalizedString{.ja = "鳥の辞典"},
    .languages = std::vector<std::string>{"ja"},
}).value();
```

`destination` is a new product directory, for example `/path/to/Birds`. The writer
refuses an existing destination. This creates one dictionary under `Contents/ja`.
Keep `product` alive while using `dictionary`.

| Value | Purpose |
| --- | --- |
| `Birds` in the destination path | Determines the product identifier `jp.monokakido.Dictionaries.Birds`. |
| `displayName` | Human-readable product name. |
| `identifier` | Stable textual identity for this dictionary's content. |
| `directory` | Content directory name inside `Contents/`. |
| `dictionaryId` | Nonzero 16-bit native dictionary ID. |

Content identifiers, directory names, and numeric dictionary IDs must be unique
within the product. Choose numeric IDs that do not collide with products you
intend to use together. `1000` is an example, not a reserved custom ID.

## Add an article

```cpp
constexpr std::string_view articleXml = R"(<html xmlns="http://www.w3.org/1999/xhtml" hmhtml="5" lang="ja">
<head><title>鶫</title></head>
<body><h1>鶫（つぐみ）</h1><p>冬に日本で見られる鳥。</p></body>
</html>)";

dictionary.addPage(1, articleXml).value();
```

Supply a complete UTF-8 XML document using the native article markup shown above.
The namespace is part of that markup. Page ID `1` is chosen by the caller and
must be nonzero. `EntryId{1, 0}` refers to this whole article.

## Give the article a headline

```cpp
dictionary.addHeadline({
    .headline = u"鶫",
    .sortingHeadline = u"つぐみ",
    .entryId = {1, 0},
}).value();
```

Every page needs a primary headline. Headline fields use UTF-16. The displayed
label and sorting text are independent of the article's `<h1>` element.

## Make both spellings searchable

```cpp
dictionary.addSearchKey(MKD::SearchScope::Headword, "鶫", {1, 0}).value();
dictionary.addSearchKey(MKD::SearchScope::Headword, "つぐみ", {1, 0}).value();
```

Search keys use UTF-8. Both keys point to the same article. Neither the headline
nor `sortingHeadline` automatically adds search keys. The writer normalises
these keys using its supported default native profile.

## Publish the files

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

## Search what you created

```cpp
auto opened = MKD::DictionaryProduct::openAtPath(destination).value();
const auto* birds = opened.findContent("example.birds.ja");
MKD::DictionarySearch search(*birds);

auto result = search.search("つぐみ", {
    .match = MKD::SearchMode::Exact,
    .fallback = MKD::SearchFallback::None,
}).value();
```

`result.entries` contains `EntryId{1, 0}`. Searching for `鶫` returns the same target.
See [Searching](../reading/searching.md) for scope selection, match modes, and
native dictionary rules.
