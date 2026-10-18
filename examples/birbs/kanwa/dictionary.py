"""Ten parent characters, each containing two searchable compound subentries."""

import json
from pathlib import Path

import mkd

from article import ROOT, article, inline, search_keys

SOURCE = Path(__file__).resolve().parent


def build(destination, *, encrypt_contents=False):
    metadata = mkd.DictionaryMetadata(
        display_name=mkd.LocalizedString(ja="鳥の漢字辞典", en="Bird Kanji"),
        product_title=mkd.LocalizedString(ja="鳥の漢字辞典"),
        description=mkd.LocalizedString(ja="鳥にまつわる漢字と熟語を読む小辞典。"),
        genre=mkd.LocalizedString(ja="漢和辞典", en="Kanji"),
        category="Japanese-Kanji",
        version="1.0",
        vertical_layout=True,
    )
    product = mkd.ProductWriter.create(
        destination,
        metadata,
        profile=mkd.CreationProfile.KANWA,
    )
    product.set_icon(mkd.Buffer.map(ROOT / "assets/kanji-icon.png"))

    font_license = (ROOT / "assets/font-OFL.txt").read_text(encoding="utf-8")
    information = mkd.ProductInformation(
        copyright=mkd.LocalizedString(
            ja="© 2026 kiwakiwaa。本文と図はオリジナルです。",
        ),
        licenses=[mkd.ProductLicense("Zen Old Mincho フォントライセンス", font_license)],
    )
    product.set_information(information)

    content = mkd.DictionaryContent(
        identifier="example.birdkanji.ja",
        directory="kanji",
        dictionary_id=61002,
        title=mkd.LocalizedString(ja="鳥の漢字", en="Bird Kanji"),
        publisher=mkd.LocalizedString(ja="kiwakiwaa"),
        edition=mkd.LocalizedString(ja="作例版"),
        version="1",
        languages=["ja"],
    )
    dictionary = product.add_dictionary(content, encrypt_contents=encrypt_contents)
    dictionary.add_font("BirdMincho.ttf", mkd.Buffer.map(ROOT / "assets/BirdMincho.ttf"))

    for filename in ("dictionary.css", "dictionary-nightmode.css", "full-ruby.css"):
        dictionary.add_asset(filename, mkd.Buffer.map(SOURCE / "assets" / filename))

    presentation = mkd.DictionaryPresentation(full_ruby_stylesheet="full-ruby.css")
    dictionary.set_presentation(presentation)

    # Both gaiji depict 鶫 using the bundled font.
    glyph = dictionary.add_gaiji(
        "tsugumi.svg",
        mkd.Buffer.map(ROOT / "assets/tsugumi-glyph.svg"),
    )
    original = dictionary.add_gaiji(
        "tsugumi.png",
        mkd.Buffer.map(ROOT / "assets/tsugumi-glyph.png"),
    )
    glyph_label = inline(mkd.Gaiji(glyph, replacement="鶫"))
    glyph_picture = inline(
        mkd.Gaiji(original, replacement="鶫", rendering=mkd.GaijiRendering.ORIGINAL),
    )

    entries = json.loads((SOURCE / "entries.json").read_text(encoding="utf-8"))
    targets = add_entries(dictionary, entries, glyph_label, glyph_picture)
    menu = add_appendices(dictionary, entries, targets, content.dictionary_id)
    product.set_appendices(menu)

    dictionary.finish()
    product.finish()


def add_entries(dictionary, entries, glyph_label, glyph_picture):
    writers = {entry["character"]: dictionary.create_entry() for entry in entries}
    targets = {}
    for character, writer in writers.items():
        targets[character] = {
            "main": writer.target,
            "meaning": writer.add_target(mkd.EntryTargetKind.SENSE),
            "meaning_section": writer.add_target(mkd.EntryTargetKind.SECTION),
            "origin_section": writer.add_target(mkd.EntryTargetKind.SECTION),
            "compounds_section": writer.add_target(mkd.EntryTargetKind.SECTION),
        }
        for index in (1, 2):
            targets[character][f"compound_{index}"] = writer.add_target(mkd.EntryTargetKind.COMPOUND)
            targets[character][f"example_{index}"] = writer.add_target(mkd.EntryTargetKind.EXAMPLE)

    related = inline(
        "関連：",
        mkd.EntryLink(targets["鳥"]["main"], "鳥"),
        " · ",
        mkd.EntryLink(targets["鳴"]["main"], "鳴"),
    )

    for entry in entries:
        entry_targets = targets[entry["character"]]
        target = entry_targets["main"]
        label = glyph_label if entry["character"] == "鶫" else inline(entry["character"])
        xml = article(
            SOURCE,
            entry["article"],
            entry["character"],
            target=entry_targets["main"],
            targets=entry_targets,
            glyph=label.to_xml(),
            related=related.to_xml(),
        )
        if entry["character"] == "鶫":
            # ORIGINAL retains the illustration's dimensions instead of sizing it as text.
            sample = "<p>字形の見本：</p>" + glyph_picture.to_xml()
            xml = xml.replace("</body>", sample + "</body>")
        writers[entry["character"]].write_xml(xml)

        headline = mkd.HeadlineComponents(
            label.to_headline(),
            target,
            sorting_headline=entry["reading"],
            prefix=entry["radical"] + "部 ",
            suffix=f"〈{entry['strokes']}画〉",
        )
        dictionary.add_headline(headline)
        dictionary.add_headline(
            mkd.HeadlineComponents(entry["character"], target),
            role=mkd.HeadlineRole.SHORT,
        )
        dictionary.add_headline(
            mkd.HeadlineComponents(entry["reading"], target),
            role=mkd.HeadlineRole.SECONDARY,
        )
        dictionary.add_headline(
            mkd.HeadlineComponents(entry["character"] + "の意味", entry_targets["meaning"]),
        )

        for index, compound in enumerate(entry["compounds"], start=1):
            compound_label = inline(mkd.Ruby(compound["word"], compound["reading"]))
            dictionary.add_headline(
                mkd.HeadlineComponents(
                    compound_label.to_headline(),
                    entry_targets[f"compound_{index}"],
                    sorting_headline=compound["reading"],
                ),
            )
            dictionary.add_headline(
                mkd.HeadlineComponents(
                    compound["word"] + "の用例",
                    entry_targets[f"example_{index}"],
                ),
            )

        references = entry_targets
        for scope, key, reference, exact_only in search_keys(entry, references):
            dictionary.add_search_key(scope, key, reference, exact_only=exact_only)

    return targets


def add_appendices(dictionary, entries, targets, dictionary_id):
    radicals = []
    # These lists need no product-specific kanwa delegate.
    for radical in ("鳥", "羽", "隹"):
        members = sorted(
            (entry for entry in entries if entry["radical"] == radical),
            key=lambda entry: (entry["residual_strokes"], entry["character"]),
        )
        items = [
            mkd.AppendixEntry(
                targets[entry["character"]]["main"],
                label=f"{entry['character']}　部首外{entry['residual_strokes']}画",
                first_character=radical,
            )
            for entry in members
        ]
        target = dictionary.add_appendix_entries("radical-" + str(ord(radical)), radical + "部", items)
        radicals.append(mkd.AppendixItem(radical + "部", target))

    strokes = sorted(entries, key=lambda entry: (entry["strokes"], entry["character"]))
    dictionary.set_entry_order([targets[entry["character"]]["main"].page_id for entry in strokes])
    items = [
        mkd.AppendixEntry(
            targets[entry["character"]]["main"],
            label=f"{entry['character']}　{entry['strokes']}画",
        )
        for entry in strokes
    ]
    by_strokes = dictionary.add_appendix_entries("strokes", "総画順", items, rows=5, columns=2)

    dictionary.add_asset("appendix/about.html", mkd.Buffer.map(SOURCE / "assets/about.html"))
    about = dictionary.add_appendix("appendix/about.html", "この辞典の見方")
    compound_entries = [
        mkd.AppendixEntry(targets[entry["character"]][f"compound_{index}"], label=compound["word"])
        for entry in entries
        for index, compound in enumerate(entry["compounds"], start=1)
    ]
    compound_list = dictionary.add_appendix_entries("compounds", "熟語一覧", compound_entries)
    compounds = mkd.AppendixList(compound_list.path, single_section=True, using_anchor_id=True)
    compound = mkd.AppendixEntryLink(targets["鳥"]["compound_1"], dictionary_id)
    return mkd.AppendixMenu([
        mkd.AppendixSection("漢字を引く", [
            mkd.AppendixItem("部首から引く", mkd.AppendixMenu([
                mkd.AppendixSection("", radicals),
            ])),
            mkd.AppendixItem("総画順", by_strokes),
            mkd.AppendixItem("熟語一覧", compounds),
        ]),
        mkd.AppendixSection("この辞典について", [
            mkd.AppendixItem("この辞典の見方", about),
            mkd.AppendixItem("外字について", mkd.AppendixDocument(about.path, fragment="gaiji")),
            mkd.AppendixItem("熟語の例：鳥類", compound),
        ]),
    ])
