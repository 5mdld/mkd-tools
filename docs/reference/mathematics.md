# Mathematical notation

Use formula text inside an element with `display: math`. This is a small math language with TeX-like syntax.

```xml
<math>\frac{a}{b}</math>
```

```css
math { display: math; text-combine-horizontal: all; }
```

Every result below is cropped from Monokakido 1.4.6.

## Formatting

| Feature | Formula text | Result in Dictionaries |
| --- | --- | --- |
| Fraction | `\frac{a}{b}` | ![a over b](../assets/reference/math-fraction.png) |
| Subscript and superscript | `x_{i}^{2}` | ![x with subscript i and superscript 2](../assets/reference/math-scripts.png) |
| Square and indexed roots | `\sqrt{x}+\sqrt[3]{y}` | ![Square root of x plus cube root of y](../assets/reference/math-root.png) |
| Large operators | `\prod_{i=1}^{n}x_i+\sum_{i=1}^{n}i` | ![Product and sum with limits](../assets/reference/math-operators.png) |
| Integrals | `\int_{0}^{1}x^2` | ![Integral of x squared from zero to one](../assets/reference/math-integral.png) |
| Accents | `\hat{x}+\bar{x}+\dot{x}` | ![Hat, bar and dot accents](../assets/reference/math-accents.png) |
| Vector arrows | `\vec{a}+\overleftarrow{AB}` | ![Arrow above a and left arrow above AB](../assets/reference/math-vectors.png) |
| Font faces | `\mathrm{ABC}+\boldsymbol{ABC}+\text{birb}` | ![Roman, bold and text faces](../assets/reference/math-faces.png) |
| Formula style | `\displaystyle\frac{a}{b}+{\scriptstyle\frac{a}{b}}` | ![Larger and smaller fractions](../assets/reference/math-styles.png) |
| Symbols and relations | `\alpha+\beta\leq\infty` | ![alpha plus beta less than or equal to infinity](../assets/reference/math-symbols.png) |
| Spacing | `a\!b+a\,b+a\;b` | ![Negative, thin and thick spacing](../assets/reference/math-spaces.png) |
| Aligned lines | `\begin{split}a&=b+c\\d&=e\end{split}` | ![Two equations aligned at their equals signs](../assets/reference/math-alignment.png) |
| Local macro | `\newcommand{\birb}[1]{\frac{#1}{2}}\birb{x}` | ![Macro expands to x over two](../assets/reference/math-macro.png) |

Braces group an argument. `_` and `^` attach to the preceding expression.
Inside `split`, `&` marks alignment and `\\` or `\cr` starts another line.
Escape `&` as `&amp;` in article XML.

The renderer also recognises `\textstyle` and `\scriptscriptstyle`, additional
integrals such as `\iint`, and accents such as `\acute`, `\grave`, `\breve`,
`\check`, `\tilde`, `\ddot`, `\dddot`, `\ddddot`, `\widehat` and `\widetilde`.
Vector forms include `\overrightarrow` and `\overleftrightarrow`.

[Symbol command list](../assets/reference/math-symbols.txt) lists the named
symbols with glyph definitions in the supported math command families.
It is a lookup aid, not a list of TeX packages or environments.

## Limits

Unknown commands can disappear silently while their arguments remain visible.
For example, `\binom{n}{k}+\frac{n}{k}` renders as:

![nk plus n over k, with binom silently missing](../assets/reference/math-unsupported.png)

Keep `text-combine-horizontal: all`on the formula when it should remain upright in vertical articles.
