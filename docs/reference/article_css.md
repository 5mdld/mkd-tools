# Article CSS

For examples in context, see [Styling articles](../authoring/styling.md).
Rendered results are screenshots from Monokakido 1.4.6.

## Layout and appearance

| Feature | Syntax and use                                                                                                                                                   | Rendered |
| --- |------------------------------------------------------------------------------------------------------------------------------------------------------------------| --- |
| Writing direction | `body { writing-mode: vertical-rl; }`. The parser also accepts `horizontal-tb` and `vertical-lr`. Set the article direction on `body`.                           | ![The same text in horizontal-tb on the left and vertical-rl on the right](../assets/reference/writing-direction.png) |
| Horizontal group | `text-combine-horizontal: all` keeps a short span together. `none` restores normal orientation. Apply `all` to the number or label, not the whole paragraph.     | ![12 with all on the left and none on the right](../assets/reference/horizontal-group.png) |
| Upright characters | `text-combine-horizontal: full-width` makes individual Latin letters and digits upright. It does not rewrite them as full-width Unicode characters.              | ![12 upright, one digit per position](../assets/reference/combine-full.png) |
| Warichu | `display: warichu; font-size: 0.5em` divides an annotation into two small lines.                                                                                 | [![Two-line reading ソウケツ・ソウキツ](../assets/css/kjt-warichu.png)](layout_examples.md#compact-readings-with-warichu) |
| Ruby | `display: ruby`, `ruby-base`, `ruby-text` style the container, base and reading. Use [ruby markup](../authoring/ruby.md#add-ruby).                      | ![Ruby readings in 例解学習国語](../assets/guide/rgko-ruby.png) |
| Gaiji images | `display: gaiji; height: 1.1em` gives an image text-like placement and colouring. See [gaiji registration](../authoring/gaiji.md#register-and-encode-the-glyph). | [![Inline gaiji beside the reading](../assets/css/kjt-gaiji.png)](layout_examples.md#images-that-behave-like-text) |
| Mathematics | `display: math` interprets formula text. See the [notation reference](mathematics.md).                                                                           | [![a over b](../assets/reference/math-fraction.png)](mathematics.md) |
| Hanging marker | `display: hang` places a marker outside the paragraph's text edge.                                                                                               | [![Triangle markers outside aligned text](../assets/css/okg-hang.png)](layout_examples.md#hanging-markers) |
| Arrow border | `border-shape: arrow-box` or `arrow-box-lr` points one way, `arrow-box-rl` the other. `default` restores an ordinary border.                                     | ![Right- and left-pointing 組成 labels](../assets/reference/arrows.png) [Official example](layout_examples.md#arrow-shaped-labels) |
| Pitch accent | `text-decoration: accent` draws a high-pitch line and final drop.                                                                                                | [![Pitch-accent lines over readings](../assets/css/daijirin-accent.png)](layout_examples.md#pitch-accent-marks) |
| Midpoint dot | `text-decoration: midpoint` puts one dot at the middle of the decorated span, not one dot per character.                                                         | ![One dot below the middle of つぐみ](../assets/reference/midpoint.png) |
| Decoration colour and width | `text-decoration-color: #175C80; text-decoration-width: 0.12em` adjusts a decoration.                                                                            | ![Thick blue accent line](../assets/reference/decoration.png) |
| Font glyph | `glyph: 11307` selects a glyph ID in the current font. It is not a Unicode code point.                                                                           | [![Font-specific boxed zero accent mark](../assets/css/smk-glyph.png)](layout_examples.md#select-a-glyph) |
| Font feature | `glyph-form: ital` selects one four-character OpenType feature tag. Other tags, such as `jp90`, depend on the font.                                              | [![Italic x within Japanese text](../assets/css/oko-glyph-form.png)](layout_examples.md#select-a-font-feature) |
| Named and night colours | `@color birb-blue = #175C80;` defines a name for `color`, `background-color` and borders. Override the name in the sibling `-nightmode.css` stylesheet.          | [![Distinct named colours at night](../assets/css/daijisen-night.png)](../authoring/styling.md#colours-and-night-presentation) |
| Image scale | `img { zoom: 60%; }` scales the image's natural dimensions before fitting it to the available space. `normal` restores 100%. This does not scale ordinary text.  | ![Saberlion at normal scale on the left and 60 percent on the right](../assets/reference/image-zoom.png) |

## Selection & copying

These controls only affect interaction.

| Property | Behaviour | Rendered |
| --- | --- | --- |
| `word-select: word` | Expands a word selection. | ![woodland selected as one word](../assets/reference/word.png) |
| `word-select: character` | Suppresses word expansion. Double-clicking no longer selects the whole word. | ![Narrow selection instead of the whole word](../assets/reference/character.png) |
| `user-select: none` | Prevents selection of the span. `text` allows it, `auto` uses the default. The `-webkit-user-select` is also accepted. | ![Unselectable grey span beside selectable text](../assets/reference/unselectable.png) |
| `user-select: nocopy` | Leaves text visible but excludes it from copied text. Copying this paragraph yields only `Copied birb.` | ![Visible Not copied span beside Copied birb](../assets/reference/nocopy.png) |
| `speech: disabled` / `enabled` | Excludes or includes text in native read-aloud. Here playback begins with `The`, skipping `Silent birb.` | ![Native speech highlight starts after the disabled span](../assets/reference/speech.png) |
