# Tests

Install nanobind in the Python environment used by CMake:

```sh
python -m pip install "nanobind>=3.1,<4"
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug --parallel 4
```

Testing builds the Python bindings and generates two small dictionaries under
`build/debug/test/dictionaries/Debug`.

Use `ctest --preset debug -R Search --output-on-failure` to run a smaller group.
