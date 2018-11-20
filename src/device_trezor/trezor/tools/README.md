# Trezor

## Messages rebuild

Install `protoc` for your distribution.

- `protobuf-compiler`
- `libprotobuf-dev`
- `libprotoc-dev`
- `python-protobuf`

Python 3 is required. If you don't have python 3 quite an easy way is
to use [pyenv].

It is also advised to create own python virtual environment so dependencies
are installed in this project-related virtual environment.

```bash
python -m venv /
```

Make sure your python has `protobuf` package installed

```bash
pip install protobuf
```

Regenerate messages:

```
./venv/bin/python3 src/device_trezor/trezor/tools/build_protob.py
```

The messages regeneration is done also automatically via cmake.

[pyenv]: https://github.com/pyenv/pyenv
