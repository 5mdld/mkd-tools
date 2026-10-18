# Gaiji and ruby

Use `InlineText` to build XML fragments and headline text with ruby and gaiji.
These examples use an unfinished dictionary writer from the preceding guides.

## Add ruby

=== "C++"

    ```cpp
    MKD::InlineText label;
    label.append(MKD::Ruby{"鶫", "つぐみ"});
    ```

=== "Python"

    ```python
    label = mkd.InlineText()
    label.append(mkd.Ruby("鶫", "つぐみ"))
    ```

The ruby pairs the written form 鶫 with its reading つぐみ.

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
styles the image as text. Choose `GaijiRendering::Original` /
`GaijiRendering.ORIGINAL` to preserve the original image presentation.

## Use the formatted text

After assembling the label, encode it for the intended destination:

=== "C++"

    ```cpp
    auto fragment = label.toXhtml().value();
    auto headline = label.toHeadline().value();
    ```

=== "Python"

    ```python
    fragment = label.to_xml()
    headline = label.to_headline()
    ```

Insert `fragment` into article XML before writing it. Use `headline` as a
`HeadlineComponents` headline. XML ruby uses `<ruby><rb>鶫</rb><rt>つぐみ</rt></ruby>`.
Headlines use a separate native encoding.

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

Next: [Images, audio, fonts and CSS](resources.md).
