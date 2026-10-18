"""Build the small dictionaries used by C++ and Python tests."""

import argparse
from pathlib import Path
import shutil

import birbs
import kanji


def build(destination):
    destination.mkdir(parents=True, exist_ok=True)
    for name, author in (('Birbs', birbs), ('BirbKanji', kanji)):
        path = destination / name
        if path.exists():
            shutil.rmtree(path)
        author.build(path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=Path)
    build(parser.parse_args().destination)
