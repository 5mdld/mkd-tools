# Register fonts

![旺文社国語辞典 uses its font’s italic form for the variable x.](../assets/css/oko-glyph-form.png)

旺文社国語辞典 uses its font’s italic form for the variable x.

Register a font, then select its internal family name in your stylesheet.

## Register a font

=== "Python"

    ```python
    dictionary.add_font("Birds.otf", mkd.Buffer.map("Birds.otf"))
    ```

=== "C++"

    ```cpp
    auto font = MKD::RetainedSpan::map("Birds.otf").value();
    dictionary.addFont("Birds.otf", font.span()).value();
    ```

The writer stores the font under `fonts/` and registers its filename in product
metadata. Use `.ttf`, `.otf` or `.ttc` with a matching signature. Font filenames
must be unique across the product.

Select the registered font by its internal family or PostScript name, which may
differ from the filename. For a font named `Birds`:

```css
body { font-family: Birds; }
```

Read back registered fonts through [media and fonts](../reading/resources.md).

Include any required [font license](information.md) in the product.

Next: [Style articles](styling.md).
