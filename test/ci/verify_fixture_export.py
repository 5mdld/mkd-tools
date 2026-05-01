#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


BIRBS = [
    ("000001-0000", "鸛", "コウノトリ", "鸛【コウノトリ】"),
    ("000002-0000", "鴇", "トキ", "鴇【トキ】"),
    ("000003-0000", "鶚", "ミサゴ", "鶚【ミサゴ】"),
    ("000004-0000", "鶫", "ツグミ", "鶫【ツグミ】"),
    ("000005-0000", "鷭", "バン", "鷭【バン】"),
]

SABERLION_SHA256 = "17245ed1614466dbd972d1c421edcf428b31f2bb7443ba27d8dc915e8059a6ba"

def read_lines(path: Path) -> list[str]:
    if not path.is_file():
        raise AssertionError(f"Missing expected exported file: {path}")
    return path.read_text(encoding="utf-8").splitlines()


def verify_graphics(output: Path) -> None:
    exported = output / "Graphics" / "saberlion.png"
    if not exported.is_file():
        raise AssertionError(f"Missing exported nrsc payload: {exported}")

    data = exported.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SABERLION_SHA256:
        raise AssertionError(f"Unexpected saberlion export hash: {digest}")


def verify_keystore(output: Path) -> None:
    forward = read_lines(output / "Keystore" / "headword.keystore.tsv")
    inverse = read_lines(output / "Keystore" / "headword.keystore_inverse.tsv")

    if len(forward) != 10:
        raise AssertionError(f"Expected 10 forward keystore rows, got {len(forward)}")
    if len(inverse) != 5:
        raise AssertionError(f"Expected 5 inverse keystore rows, got {len(inverse)}")

    forward_set = set(forward)
    inverse_set = set(inverse)

    for index, (entry_id, kanji, reading, _headline) in enumerate(BIRBS, start=1):
        raw_id = f"{index}-0"
        if f"{kanji}\t{raw_id}" not in forward_set:
            raise AssertionError(f"Missing forward keystore row for {kanji}")
        if f"{reading}\t{raw_id}" not in forward_set:
            raise AssertionError(f"Missing forward keystore row for {reading}")

        expected = f"{entry_id}\t{kanji}\t{reading}"
        reverse_expected = f"{entry_id}\t{reading}\t{kanji}"
        if expected not in inverse_set and reverse_expected not in inverse_set:
            raise AssertionError(f"Missing inverse keystore row for {entry_id}")


def verify_headlines(output: Path) -> None:
    lines = read_lines(output / "Headline" / "headline.headlinestore.tsv")
    if len(lines) != 5:
        raise AssertionError(f"Expected 5 headline rows, got {len(lines)}")

    line_set = set(lines)
    for entry_id, _kanji, _reading, headline in BIRBS:
        if f"{entry_id}\t{headline}\t" not in line_set:
            raise AssertionError(f"Missing headline row for {headline}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    verify_graphics(args.output)
    verify_keystore(args.output)
    verify_headlines(args.output)


if __name__ == "__main__":
    main()
