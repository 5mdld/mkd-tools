"""A general Japanese dictionary and a separate field-guide content."""

import json
from pathlib import Path

import mkd

from article import ROOT, article, inline, search_keys

SOURCE = Path(__file__).resolve().parent


def build(destination, *, encrypt_contents=False):
    metadata = mkd.DictionaryMetadata(
        display_name=mkd.LocalizedString(ja="鳥のことば辞典", en="Bird Words"),
        product_title=mkd.LocalizedString(ja="鳥のことば辞典"),
        description=mkd.LocalizedString(ja="鳥から広がる、小さな日本語辞典。"),
        genre=mkd.LocalizedString(ja="国語辞典", en="Japanese"),
        category="Japanese-Dictionary",
        version="1.0",
        vertical_layout=False,
    )
    product = mkd.ProductWriter.create(destination, metadata)
    product.set_icon(mkd.Buffer.map(ROOT / "assets/words-icon.png"))

    font_license = (ROOT / "assets/font-OFL.txt").read_text(encoding="utf-8")
    information = mkd.ProductInformation(
        copyright=mkd.LocalizedString(
            ja="© 2026 kiwakiwaa。本文と図はオリジナルです。",
        ),
        licenses=[mkd.ProductLicense("Zen Old Mincho フォントライセンス", font_license)],
    )
    product.set_information(information)

    content = mkd.DictionaryContent(
        identifier="example.birdwords.ja",
        directory="words",
        dictionary_id=61000,
        title=mkd.LocalizedString(ja="鳥のことば", en="Bird Words"),
        publisher=mkd.LocalizedString(ja="kiwakiwaa"),
        edition=mkd.LocalizedString(ja="作例版"),
        version="1",
        languages=["ja"],
    )
    dictionary = product.add_dictionary(content)
    add_resources(dictionary)

    entries = json.loads((SOURCE / "entries.json").read_text(encoding="utf-8"))
    targets = add_entries(dictionary, entries)
    sections = add_appendices(dictionary, entries, targets)
    dictionary.finish()

    guide_items = add_guide(product, encrypt_contents=encrypt_contents)
    sections.append(mkd.AppendixSection("鳥を見る手帖", guide_items))
    sections.append(add_field_notes(product))
    product.set_appendices(mkd.AppendixMenu(sections))
    birbs = mkd.NavigationIndex(sections=[mkd.NavigationSection(items=[
        mkd.NavigationItem(label=entry["word"], target=targets[entry["word"]]["main"])
        for entry in entries
    ])])
    product.set_navigation(mkd.NavigationIndex(rows=2, columns=2, sections=[
        mkd.NavigationSection(items=[
            mkd.NavigationItem(label="鳥のことば", target=birbs),
            mkd.NavigationItem(label="鳥なき里の蝙蝠", target=targets["鳥"]["idiom"]),
        ]),
    ]))
    product.finish()


def add_resources(dictionary):
    dictionary.add_font("BirdMincho.ttf", mkd.Buffer.map(ROOT / "assets/BirdMincho.ttf"))

    for filename in ("dictionary.css", "dictionary-nightmode.css", "full-ruby.css"):
        dictionary.add_asset(filename, mkd.Buffer.map(SOURCE / "assets" / filename))

    presentation = mkd.DictionaryPresentation(full_ruby_stylesheet="full-ruby.css")
    dictionary.set_presentation(presentation)
    dictionary.add_asset("speaker.png", mkd.Buffer.map(ROOT / "assets/speaker.png"))

    for filename in ("thrush.png", "sparrow.png", "thrush.svg", "sparrow.svg"):
        dictionary.add_graphic(filename, mkd.Buffer.map(ROOT / "assets" / filename))

    for filename in ("tsugumi.m4a", "suzume.m4a"):
        dictionary.add_audio(filename, mkd.Buffer.map(ROOT / "assets" / filename))


def add_entries(dictionary, entries):
    writers = {entry["word"]: dictionary.create_entry() for entry in entries}
    targets = {
        word: {
            "main": writer.target,
            "example": writer.add_target(mkd.EntryTargetKind.EXAMPLE),
        }
        for word, writer in writers.items()
    }
    targets["鳥"]["idiom"] = writers["鳥"].add_target(mkd.EntryTargetKind.IDIOM)
    related = inline(
        "関連：",
        mkd.EntryLink(targets["渡り鳥"]["main"], "渡り鳥"),
        " · ",
        mkd.EntryLink(targets["鶫"]["example"], "鶫の用例"),
    )

    for entry in entries:
        entry_targets = targets[entry["word"]]
        target = entry_targets["main"]
        title = entry["reading"] + "【" + entry["word"] + "】"

        # The same structured text becomes article XML and an encoded headline.
        if entry["word"] == "鶫":
            label = inline(mkd.Ruby("鶫", "つぐみ"))
        else:
            label = inline(entry["word"])
        xml = article(
            SOURCE,
            entry["article"],
            title,
            target=entry_targets["main"],
            targets=entry_targets,
            bird=str(targets["鳥"]["main"]),
            migration=str(targets["渡り鳥"]["main"]),
            resident=str(targets["留鳥"]["main"]),
            heading="<h1><headword>" + label.to_xml() + "</headword></h1>",
            related=related.to_xml(),
        )
        writers[entry["word"]].write_xml(xml)

        headline = mkd.HeadlineComponents(
            label.to_headline(),
            target,
            prefix=entry["reading"] + "【",
            suffix="】",
            sorting_headline=entry["reading"],
        )
        dictionary.add_headline(headline)
        dictionary.add_headline(
            mkd.HeadlineComponents(entry["word"], target),
            role=mkd.HeadlineRole.SHORT,
        )

        # HeadlineMarkup retains a headline command, not an XML element.
        secondary = inline(mkd.HeadlineMarkup("{B:" + entry["word"] + "}"))
        dictionary.add_headline(
            mkd.HeadlineComponents(secondary.to_headline(), target),
            role=mkd.HeadlineRole.SECONDARY,
        )
        dictionary.add_headline(
            mkd.HeadlineComponents(entry["word"] + "の用例", entry_targets["example"]),
        )

        references = entry_targets
        for scope, key, reference, exact_only in search_keys(entry, references):
            dictionary.add_search_key(scope, key, reference, exact_only=exact_only)
        for alias in entry.get("aliases", []):
            dictionary.add_search_key(mkd.SearchScope.HEADWORD, alias, target)

    idiom = targets["鳥"]["idiom"]
    dictionary.add_headline(mkd.HeadlineComponents("鳥なき里の蝙蝠", idiom))
    dictionary.add_search_key(mkd.SearchScope.IDIOM, "とりなきさとのこうもり", idiom)
    return targets


def add_appendices(dictionary, entries, targets):
    ordered = sorted(entries, key=lambda entry: entry["reading"])
    dictionary.set_entry_order([targets[entry["word"]]["main"].page_id for entry in ordered])
    items = [
        mkd.AppendixEntry(
            targets[entry["word"]]["main"],
            label=entry["reading"] + "【" + entry["word"] + "】",
            first_character=entry["reading"][0],
        )
        for entry in ordered
    ]
    all_words = dictionary.add_appendix_entries("words", "五十音で引く", items)

    winter = [
        mkd.AppendixEntry(targets["鶫"]["main"], label="鶫", first_character="つ"),
        mkd.AppendixEntry(targets["鴨"]["main"], label="鴨", first_character="か"),
    ]
    winter_birbs = dictionary.add_appendix_entries("winter", "冬の鳥", winter, rows=1, columns=2)

    dictionary.add_asset("appendix/about.html", mkd.Buffer.map(SOURCE / "assets/about.html"))
    about = dictionary.add_appendix("appendix/about.html", "この辞典について")
    return [mkd.AppendixSection("鳥のことば", [
        mkd.AppendixItem("五十音で引く", all_words),
        mkd.AppendixItem("冬の鳥", winter_birbs),
        mkd.AppendixItem("この辞典について", about),
    ])]


def add_guide(product, *, encrypt_contents):
    # A product can contain several independently searchable dictionaries.
    content = mkd.DictionaryContent(
        identifier="example.birdwords.guide",
        directory="guide",
        dictionary_id=61001,
        title=mkd.LocalizedString(ja="鳥を見る手帖"),
        languages=["ja"],
    )
    guide = product.add_dictionary(content, encrypt_contents=encrypt_contents)
    guide.add_asset("dictionary.css", mkd.Buffer.map(SOURCE / "assets/guide.css"))
    guide.add_asset(
        "dictionary-nightmode.css",
        mkd.Buffer.map(SOURCE / "assets/dictionary-nightmode.css"),
    )
    guide.add_graphic("thrush.png", mkd.Buffer.map(ROOT / "assets/thrush.png"))

    pages = [
        ("guide-observation.xml", "姿と声を観察する", "観察"),
        ("guide-migration.xml", "季節と渡り", "渡り"),
        ("guide-manners.xml", "鳥との距離", "マナー"),
    ]
    menu_items = []
    for filename, title, key in pages:
        entry = guide.create_entry()
        target = entry.target
        entry.write_xml(article(SOURCE, filename, title, target=entry.target))
        guide.add_headline(mkd.HeadlineComponents(title, target, sorting_headline=key))
        guide.add_search_key(mkd.SearchScope.HEADWORD, key, target)
        menu_items.append(mkd.AppendixItem(title, mkd.AppendixEntryLink(target, content.dictionary_id)))

    guide.finish()
    return menu_items


def add_field_notes(product):
    product.add_asset("field-note.pdf", mkd.Buffer.map(SOURCE / "assets/field-note.pdf"))
    images = []
    for filename in ("thrush.png", "sparrow.png"):
        path = "gallery/" + filename
        product.add_asset(path, mkd.Buffer.map(ROOT / "assets" / filename))
        images.append(path)
    return mkd.AppendixSection("観察メモ", [
        mkd.AppendixItem("小鳥の図", mkd.AppendixGallery(images)),
        mkd.AppendixItem("観察メモ（PDF）", mkd.AppendixDocument("field-note.pdf")),
    ])
