# Birb dictionaries

One Python example with two dictionaries:

- **BirdWords:** 18 entries and a three-page field guide.
- **BirdKanji:** 10 kanji entries with 20 compound subentries, ruby and gaiji.

## Build

From the repository root:

```sh
python -m pip install .
python examples/birbs/build.py build/birbs --encrypt
```

The products demonstrate search scopes, headline variants, ruby, gaiji, media,
fonts, native CSS, information pages, icons, appendix menus and browsing order.
