"""Kanwa navigation indexes built from the example's kanji metadata."""

from itertools import groupby

import mkd

RADICALS = (("羽", "はね", 6), ("隹", "ふるとり", 8), ("鳥", "とり・とりへん", 11))


def radical_index(entries, targets):
    sections = []
    for character, reading, strokes in RADICALS:
        members = sorted(
            (entry for entry in entries if entry["radical"] == character),
            key=lambda entry: entry["residual_strokes"],
        )
        groups = []
        for count, group in groupby(members, key=lambda entry: entry["residual_strokes"]):
            items = [
                mkd.NavigationItem(label=entry["character"], target=targets[entry["character"]]["main"])
                for entry in group
            ]
            groups.append(mkd.NavigationSection(f"部首 ＋ {count}画", items))
        radical = mkd.NavigationItem(
            label=mkd.RadicalLabel(character, reading=reading),
            target=mkd.NavigationIndex(sections=groups, section_index=False),
        )
        sections.append(mkd.NavigationSection(f"{strokes}画", [radical]))
    return mkd.NavigationIndex(kind=mkd.NavigationKind.RADICALS, sections=sections)


def stroke_index(entries, targets):
    items = []
    ordered = sorted(entries, key=lambda entry: entry["strokes"])
    for count, group in groupby(ordered, key=lambda entry: entry["strokes"]):
        members = [
            mkd.NavigationItem(label=entry["character"], target=targets[entry["character"]]["main"])
            for entry in group
        ]
        items.append(mkd.NavigationItem(
            label=str(count),
            target=mkd.NavigationIndex(sections=[mkd.NavigationSection(items=members)]),
        ))
    return mkd.NavigationIndex(
        kind=mkd.NavigationKind.TOTAL_STROKES,
        sections=[mkd.NavigationSection(items=items)],
    )


def reading_index(entries, targets):
    groups = []
    for initial, group in groupby(sorted(entries, key=lambda entry: entry["reading"]),
                                  key=lambda entry: entry["reading"][0]):
        items = [
            mkd.NavigationItem(label=entry["reading"], target=targets[entry["character"]]["main"])
            for entry in group
        ]
        groups.append(mkd.NavigationSection(initial, items))
    return mkd.NavigationIndex(kind=mkd.NavigationKind.READINGS, sections=groups)


def build_navigation(entries, targets):
    compounds = [
        mkd.NavigationItem(label=compound["word"], target=targets[entry["character"]][f"compound_{index}"])
        for entry in entries
        for index, compound in enumerate(entry["compounds"], start=1)
    ]
    return mkd.NavigationIndex(sections=[
        mkd.NavigationSection("親字", [
            mkd.NavigationItem(label="部首", target=radical_index(entries, targets)),
            mkd.NavigationItem(label="総画", target=stroke_index(entries, targets)),
            mkd.NavigationItem(label="音訓", target=reading_index(entries, targets)),
        ]),
        mkd.NavigationSection("熟語", [
            mkd.NavigationItem(label="熟語一覧", target=mkd.NavigationIndex(sections=[
                mkd.NavigationSection(items=compounds),
            ])),
        ]),
    ])
