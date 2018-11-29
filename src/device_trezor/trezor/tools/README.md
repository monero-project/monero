# Trezor

## Messages rebuild

Install `protoc` for your distribution. Requirements:

- `protobuf-compiler`
- `libprotobuf-dev`
- `python`


Soft requirement: Python 3, can be easily installed with [pyenv].
If Python 3 is used there are no additional python dependencies.

Since Cmake 3.12 the `FindPython` module is used to locate the Python 
interpreter in your system. It preferably searches for Python 3, if none
is found, it searches for Python 2. 

Lower version of the cmake uses another module which does not guarantee 
ordering. If you want to override the selected python you can do it in 
the following way:

```bash
export TREZOR_PYTHON=`which python3`
``` 
 

### Python 2.7+

Python 3 has `tempfile.TemporaryDirectory` available but Python 2 lacks
this class so the message generation code uses `backports.tempfile` package
bundled in the repository.

The minimal Python versions are 2.7 and 3.4

### Regenerate messages

```bash
cd src/device_trezor/trezor
python tools/build_protob.py
```

The messages regeneration is done also automatically via cmake.

[pyenv]: https://github.com/pyenv/pyenv
