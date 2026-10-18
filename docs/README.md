# mkd documentation

Read, search and create Monokakido dictionaries with C++ or Python.

## Building

Requires cmake 3.31 or newer and a C++23 compiler.

```sh
git clone --recurse-submodules https://github.com/kiwakiwaa/mkd-tools.git
cd mkd-tools
```

=== "macOS"

    Use Xcode's command line tools and install cmake and ninja.

    ```sh
    cmake --preset release -G Ninja
    cmake --build --preset release --parallel
    ```

    CLI: `build/release/cli/mkd-tools`.

=== "Linux"

    Install cmake, ninja, openssl and a C++23 compiler.

    ```sh
    CC=gcc-14 CXX=g++-14 cmake --preset release -G Ninja
    cmake --build --preset release --parallel
    ```

    ```sh
    CC=clang-20 CXX=clang++-20 CXXFLAGS=-stdlib=libc++ cmake --preset release -G Ninja
    cmake --build --preset release --parallel
    ```

    CLI: `build/release/cli/mkd-tools`.

=== "Windows"

    Install Visual Studio with the Desktop development with C++ and
    cmake. Run from a Developer PowerShell:

    ```powershell
    cmake --preset release
    cmake --build --preset release --parallel
    ```

    CLI: `build/release/cli/Release/mkd-tools.exe`.

## Python bindings

Requires Python 3.10 or newer. From the repository root:

```sh
python -m pip install .
```

This builds the bindings and installs the `mkd` module.

## CLI usage

```sh
mkd-tools list
mkd-tools info DAIJISEN2
mkd-tools export DAIJISEN2
```

On macOS, installed dictionaries are found automatically. Elsewhere, provide the directory containing product folders:

```sh
mkd-tools -d /path/to/dictionaries list
mkd-tools -d /path/to/dictionaries export DAIJISEN2
```

Exports go to `./mkd-export/<product-id>` by default. Use `-o` to select an output
directory.

```sh
mkd-tools export DAIJISEN2 -o ./output
mkd-tools export DAIJISEN2 --only entries --pretty
mkd-tools export DAIJISEN2 --only graphics --only audio
mkd-tools export DAIJISEN2 --only keystores --keystore-mode both
```

`--only` accepts `entries`, `graphics`, `audio`, `fonts`, `headlines`, `keystores`
and `appendix-entries`.
`--pretty` formats XML.
Use `mkd-tools --help` or `mkd-tools export --help` for all options.

## macOS receipt app

Intended only for activating custom third-party dictionaries you are authorised
to use. For store purchases, use the Dictionaries app's purchase restoration.

The SwiftUI receipt app requires Xcode:

```sh
cmake -S . -B build-xcode -G Xcode -DMKD_BUILD_CLI=OFF -DMKD_BUILD_RECEIPT_APP=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3
cmake --build build-xcode --target MKDReceiptApp --config Release
```

App: `build-xcode/apps/monokakido-receipt-ui/Release/MKDReceiptApp.app`.
