# Images, audio, fonts and CSS

Use these operations before finishing the dictionary writer from
[Creating a dictionary](minimal_dictionary.md). Files below are your own assets. Media is stored as supplied, without transcoding.

## Add packed media

=== "C++"

    ```cpp
    auto image = MKD::RetainedSpan::map("bird.svg").value();
    auto sound = MKD::RetainedSpan::map("bird.aac").value();
    dictionary.addGraphic("bird.svg", image.span()).value();
    dictionary.addAudio("bird.aac", sound.span()).value();
    ```

=== "Python"

    ```python
    dictionary.add_graphic("bird.svg", mkd.Buffer.map("bird.svg"))
    dictionary.add_audio("bird.aac", mkd.Buffer.map("bird.aac"))
    ```

Names must be single filenames. Reference these assets with the content-relative
paths `graphics/bird.svg` and `audio/bird.aac` in article XML. For example, an image
uses `<img src="graphics/bird.svg" alt="鶫"/>`. The writer packs graphics and audio
into named stores. `add_asset` cannot write into those generated directories.
Native playback still depends on a supported media format.

## Add stylesheets and loose assets

=== "C++"

    ```cpp
    auto css = MKD::RetainedSpan::map("Birds.css").value();
    dictionary.addAsset("Birds.css", css.span()).value();
    auto nightCss = MKD::RetainedSpan::map("Birds-nightmode.css").value();
    dictionary.addAsset("Birds-nightmode.css", nightCss.span()).value();
    ```

=== "Python"

    ```python
    dictionary.add_asset("Birds.css", mkd.Buffer.map("Birds.css"))
    dictionary.add_asset("Birds-nightmode.css", mkd.Buffer.map("Birds-nightmode.css"))
    ```

Reference `Birds.css` from an article's `<head>` with
`<link rel="stylesheet" href="Birds.css"/>`. The reader recognises the corresponding
`-nightmode.css` sibling. CSS resource references are checked when finishing.
Use relative paths within the dictionary for other loose files.

`ProductWriter::addAsset` / `ProductWriter.add_asset` instead writes paths relative
to the product root. Use dictionary assets for content-relative references.

## Register a font

=== "C++"

    ```cpp
    auto font = MKD::RetainedSpan::map("Birds.otf").value();
    dictionary.addFont("Birds.otf", font.span()).value();
    ```

=== "Python"

    ```python
    dictionary.add_font("Birds.otf", mkd.Buffer.map("Birds.otf"))
    ```

The writer stores the font under `fonts/` and registers its filename in product
metadata. Use `.ttf`, `.otf` or `.ttc` with a matching signature. Font filenames
must be unique across the product.

A stylesheet can reference the registered font:

```css
@font-face {
    font-family: Birds;
    src: url("fonts/Birds.otf");
}
body { font-family: Birds; }
```

See [Reading dictionaries](../reading/browsing.md) to inspect media, stream fonts
and read stylesheets after reopening the product.

## Product information and licenses

The product generates its information page from its title, description, version
and content publishers. Supply copyright and license text separately:

=== "C++"

    ```cpp
    product.setInformation({
        .copyright = MKD::LocalizedString{.ja = "© 2026 鳥の辞典編集室"},
        .licenses = {{.title = "Font license", .text = licenseText}},
    }).value();
    ```

=== "Python"

    ```python
    product.set_information(mkd.ProductInformation(
        copyright=mkd.LocalizedString(ja="© 2026 鳥の辞典編集室"),
        licenses=[mkd.ProductLicense("Font license", license_text)],
    ))
    ```

The native その他 tab includes copyright, license pages and settings.
For a custom information page, set `ProductInformation.page` to the path of a
product asset added with `addAsset` / `add_asset`. Each license also accepts a
`page` override. Paths are relative to the product's `Contents/` directory.
Custom pages must exist before `finish()`.

## Product icon

=== "C++"

    ```cpp
    auto icon = MKD::RetainedSpan::map("icon.png").value();
    product.setIcon(icon.span()).value();
    ```

=== "Python"

    ```python
    product.set_icon(mkd.Buffer.map("icon.png"))
    ```

Supply one square PNG or JPEG, up to 4096 pixels wide. The writer generates all
13 native filename/scale variants. Supply the icon once per product. The input
bytes are consumed during the call.

## Resource inputs

Python writer methods accept `mkd.Buffer`, `bytes`, `bytearray` and contiguous
one-dimensional byte `memoryview` inputs. `mkd.Buffer.map(path)` maps a file.
Inputs are consumed during the call. Writer calls retain the GIL and should be
used serially.

Next: [Appendices and entry order](navigation.md).
