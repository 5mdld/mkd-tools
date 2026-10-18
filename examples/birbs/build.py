import argparse
from pathlib import Path

from general.dictionary import build as build_general
from kanwa.dictionary import build as build_kanwa


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    parser.add_argument(
        "--encrypt",
        action="store_true",
        help="encrypt contents",
    )
    args = parser.parse_args()
    output = args.destination.resolve()
    for name in ("BirdWords", "BirdKanji"):
        if (output / name).exists():
            parser.error(f"destination already exists: {output / name}")

    output.mkdir(parents=True, exist_ok=True)
    for name, build in (("BirdWords", build_general), ("BirdKanji", build_kanwa)):
        build(output / name, encrypt_contents=args.encrypt)
        print(f"Created {output / name}")


if __name__ == "__main__":
    main()
