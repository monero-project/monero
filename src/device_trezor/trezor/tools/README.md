# Trezor

## Messages rebuild

Install `protoc` for your distribution. Requirements:

- `protobuf-compiler`
- `libprotobuf-dev`
- `python`


Soft requirement: Python 3, can be easily installed with [pyenv].

### Python 2

Workaround if there is no Python3 available:

```bash
pip install backports.tempfile
```

### Regenerate messages

```bash
cd src/device_trezor/trezor
python tools/build_protob.py
```

The messages regeneration is done also automatically via cmake.

[pyenv]: https://github.com/pyenv/pyenv
