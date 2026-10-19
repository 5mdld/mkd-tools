# mkd-tools

A C++23 library for reading, searching and creating 物書堂 dictionaries, with Python bindings and a CLI.

[Library documentation](https://kiwakiwaa.github.io/mkd-tools/) · C++ and Python guides

## Installation

Python library:

```sh
python -m pip install mkd-tools
```

Import the library as `mkd`.

CLI on macOS, with [Homebrew](https://brew.sh/):

```sh
brew install kiwakiwaa/tap/mkd-tools
```

## Acknowledgements

Dictionary-format research used [Hopper](https://www.hopperapp.com/) and [Frida](https://frida.re/).
Python bindings use [nanobind](https://github.com/wjakob/nanobind).
Some macOS bindings are adapted from Apple's
[metal-cpp](https://developer.apple.com/metal/cpp/).

## Notice

An independent project based on reverse engineering, not affiliated with
Monokakido. Use it only with dictionaries and content you are authorised to use.
