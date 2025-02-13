# Trezor hardware wallet support

This module adds [Trezor] hardware support to Monero.


## Basic information

Trezor integration is based on the following original proposal: https://github.com/ph4r05/monero-trezor-doc

A custom high-level transaction signing protocol uses Trezor in a similar way a cold wallet is used. 
Transaction is build incrementally on the device. 

Trezor implements the signing protocol in [trezor-firmware] repository, in the [monero](https://github.com/trezor/trezor-firmware/tree/master/core/src/apps/monero) application.
Please, refer to [monero readme](https://github.com/trezor/trezor-firmware/blob/master/core/src/apps/monero/README.md) for more information.

## Dependencies

Trezor uses [Protobuf](https://protobuf.dev/) library.

Monero is now compiled with C++17 by default. If you are getting Trezor compilation errors, it may be caused by abseil (protobuf dependency) not being compiled with C++17.
To fix this try installing protobuf from sources:

```shell
git clone --recursive git@github.com:protocolbuffers/protobuf.git
cd protobuf
cmake -DABSL_PROPAGATE_CXX_STD=TRUE -DCMAKE_CXX_STANDARD=17 -Dprotobuf_BUILD_SHARED_LIBS=ON -Dprotobuf_BUILD_TESTS=OFF .
cmake --build .
sudo make install
```

### macOS

```bash
brew update && brew bundle --file=contrib/brew/Brewfile
```

### MSYS32

```bash
pacman -S mingw-w64-x86_64-protobuf
```

### Other systems

- install Protobuf
- point `CMAKE_PREFIX_PATH` environment variable to Protobuf installation.

## Troubleshooting

To disable Trezor support, set `USE_DEVICE_TREZOR=OFF`, e.g.:

```shell
USE_DEVICE_TREZOR=OFF make release
```

## Resources:

- First pull request https://github.com/monero-project/monero/pull/4241
- Integration proposal https://github.com/ph4r05/monero-trezor-doc
- Integration readme in trezor-firmware https://github.com/trezor/trezor-firmware/blob/master/core/src/apps/monero/README.md

[Trezor]: https://trezor.io/
[trezor-firmware]: https://github.com/trezor/trezor-firmware/