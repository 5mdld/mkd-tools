# Build from source

Use these instructions when developing mkd or building your own binaries.

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

## Link the C++ library

```cmake
set(MKD_BUILD_CLI OFF)
add_subdirectory(external/mkd-tools EXCLUDE_FROM_ALL)
target_link_libraries(my_app PRIVATE MKD::mkd)
```

Include `<MKD/mkd.hpp>` in your source.

## Build the Python bindings

Requires Python 3.10 or newer. From the repository root:

```sh
python -m pip install .
```

This builds the bindings and installs the `mkd` module.

For CLI usage, return to [the starting page](../README.md#cli-usage).
