# Gaiji, ruby and links

These snippets use a dictionary writer created as in
[Creating a dictionary](minimal_dictionary.md), before adding its first page.

## Add ruby

=== "C++"

    ```cpp
    MKD::InlineText label;
    label.append(MKD::Ruby{"鶫", "つぐみ"});
    auto fragment = label.toXhtml().value();
    auto headline = label.toHeadline().value();
    ```

=== "Python"

    ```python
    label = mkd.InlineText()
    label.append(mkd.Ruby("鶫", "つぐみ"))
    fragment = label.to_xml()
    headline = label.to_headline()
    ```

The XML fragment contains `<ruby><rb>鶫</rb><rt>つぐみ</rt></ruby>`.
The headline encoder produces the native headline syntax. Use the appropriate
encoder for each destination.

## Register a gaiji image

Use gaiji for a glyph that needs a supplied image. Ordinary Unicode text needs
no registration. `rare.svg` below is your own glyph artwork.

=== "C++"

    ```cpp
    auto image = MKD::RetainedSpan::map("rare.svg").value();
    auto glyph = dictionary.addGaiji("rare.svg", image.span()).value();
    label.append(" ");
    label.append(MKD::Gaiji{glyph, "鶫"});
    ```

=== "Python"

    ```python
    image = mkd.Buffer.map("rare.svg")
    glyph = dictionary.add_gaiji("rare.svg", image)
    label.append(" ")
    label.append(mkd.Gaiji(glyph, replacement="鶫"))
    ```

Registration writes PNG or SVG bytes under `gaiji/` and returns a reusable
content-relative reference. Replacement text is used by `plainText()`. The default rendering
styles the image as text; choose `GaijiRendering::Original` /
`GaijiRendering.ORIGINAL` to preserve the original image presentation.

## Link to an entry

=== "C++"

    ```cpp
    label.append(" → ");
    label.append(MKD::EntryLink{{1, 0}, "鶫"});
    dictionary.addPage(1, "<html><head/><body>" + label.toXhtml().value() + "</body></html>").value();
    dictionary.addHeadline({
        .headline = label.toHeadline().value(),
        .sortingHeadline = u"つぐみ",
        .entryId = {1, 0},
    }).value();
    dictionary.addSearchKey(MKD::SearchScope::Headword, "つぐみ", {1, 0}).value();
    ```

=== "Python"

    ```python
    label.append(" → ")
    label.append(mkd.EntryLink(mkd.EntryId(1), "鶫"))
    dictionary.add_page(1, "<html><head/><body>" + label.to_xml() + "</body></html>")
    dictionary.add_headline(mkd.HeadlineComponents(
        label.to_headline(), mkd.EntryId(1), sorting_headline="つぐみ",
    ))
    dictionary.add_search_key(mkd.SearchScope.HEADWORD, "つぐみ", mkd.EntryId(1))
    ```

The target may be a page or an anchor on that page. Links can point to pages
added later, but must resolve when the dictionary is finished. `InlineText`
escapes text for XML and owns its elements.

## Configure full-ruby presentation

=== "C++"

    ```cpp
    auto css = MKD::RetainedSpan::map("ruby.css").value();
    dictionary.addAsset("ruby.css", css.span()).value();
    dictionary.setPresentation({.fullRubyStylesheet = "ruby.css"}).value();
    ```

=== "Python"

    ```python
    dictionary.add_asset("ruby.css", mkd.Buffer.map("ruby.css"))
    dictionary.set_presentation(mkd.DictionaryPresentation(full_ruby_stylesheet="ruby.css"))
    ```

This records the stylesheet for the native full-ruby presentation. Supply valid CSS for your article markup. Finish the
dictionary and product as in the minimal guide.

## Decode an existing headline

=== "C++"

    ```cpp
    auto text = MKD::InlineText::fromHeadline(headline).value();
    auto plain = text.plainText().value();
    ```

=== "Python"

    ```python
    text = mkd.InlineText.from_headline(headline)
    plain = text.plain_text()
    ```

Unknown native tokens are preserved as `NativeMarkup` for headline round trips.
Converting them to XML or plain text returns an error instead of guessing a
representation. `elements()` / `elements` lets you inspect recognised elements.
