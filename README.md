# mkd-tools

A C++23 toolkit for reading and exporting 物書堂 dictionary resources.

### Building

Requires a C++23 compiler and CMake.

```
cmake --preset release
cmake --build --preset release
```

### Usage

```
mkd-tools <command> [options]
```

**Commands:** `list`, `info <product-id>`, `export <product-id>`

Use `--help` for usage information, including command-specific help such as `mkd-tools export --help`. Commands must be explicit; bare IDs and the former `-l`, `--list`, `-e`, and `--export` aliases are no longer supported.

On macOS, installed dictionaries are detected automatically.\
You can also point to a dictionary directory with `-d <path>`.
```
mkd-tools export <product-id>
mkd-tools -d /path/to/dicts export <product-id>
mkd-tools export <product-id> --only entries --pretty
mkd-tools export <product-id> --only appendix-entries
```

Use `--only` to limit export to specific resource types (`audio`, `entries`, `appendix-entries`, `graphics`, `fonts`, `keystores`, `headlines`). `--pretty` will format XML entry content.

Output goes to `./mkd-export/<product-id>` by default, configurable with `-o`.

### macOS receipt app

A lightweight SwiftUI app is included at `apps/monokakido-receipt-ui`.  Build it with the Xcode generator:

```
cmake -S . -B build-xcode -G Xcode -DMKD_BUILD_CLI=OFF -DMKD_BUILD_RECEIPT_APP=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3
cmake --build build-xcode --target MKDReceiptApp --config Release
```

The built app bundle will be under:

```
build-xcode/apps/monokakido-receipt-ui/Release/MKDReceiptApp.app
```


### Acknowledgements
This work benefited greatly from the use of [Hopper Disassembler](https://www.hopperapp.com) and [Frida](https://frida.re). 
Their capabilities made the analysis of the dictionary formats substantially more tractable.

> ### Notice
>
> This software was developed by means of independent reverse engineering. No source code, decompiled output, or other copyrightable material has been incorporated. The implementation is an original work, informed solely by observation and analysis of the binary formats. \
> This tool is intended for use exclusively with legitimately purchased dictionaries. It is neither designed nor offered as a means of obtaining or accessing content without proper licence. The author accepts no responsibility for any use of this software that may infringe upon the rights of any third party. \
> Users are solely responsible for satisfying themselves that their use of this software complies with all applicable laws in their jurisdiction, including without limitation those governing reverse engineering, copyright, and the circumvention of technological protection measures.
