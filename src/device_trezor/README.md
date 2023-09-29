# Trezor hardware wallet support

This module adds [Trezor] hardware support to Monero.


## Basic information

Trezor integration is based on the following original proposal: https://github.com/ph4r05/monero-trezor-doc

A custom high-level transaction signing protocol uses Trezor in a similar way a cold wallet is used. 
Transaction is build incrementally on the device. 

Trezor implements the signing protocol in [trezor-firmware] repository, in the [monero](https://github.com/trezor/trezor-firmware/tree/master/core/src/apps/monero) application.
Please, refer to [monero readme](https://github.com/trezor/trezor-firmware/blob/master/core/src/apps/monero/README.md) for more information.

## Dependencies

Trezor uses [Protobuf](https://protobuf.dev/) library. As Monero is compiled with C++14, the newest Protobuf library version cannot be compiled because it requires C++17 (through its dependency Abseil library).
This can result in a compilation failure.

Protobuf v21 is the latest compatible protobuf version.

If you want to compile Monero with Trezor support, please make sure the Protobuf v21 is installed.

More about this limitation: [PR #8752](https://github.com/monero-project/monero/pull/8752), 
[1](https://github.com/monero-project/monero/pull/8752#discussion_r1246174755), [2](https://github.com/monero-project/monero/pull/8752#discussion_r1246480393)

### OSX

To build with installed, but not linked protobuf:

```bash
CMAKE_PREFIX_PATH=$(find /opt/homebrew/Cellar/protobuf@21 -maxdepth 1 -type d -name "21.*" -print -quit) \
make release
```

or to install and link as a default protobuf version:
```bash
# Either install all requirements as
brew update && brew bundle --file=contrib/brew/Brewfile

# or install protobufv21 specifically
brew install protobuf@21 && brew link protobuf@21
```

### MSYS32

```bash
curl -O https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-protobuf-c-1.4.1-1-any.pkg.tar.zst
curl -O https://repo.msys2.org/mingw/mingw64/mingw-w64-x86_64-protobuf-21.9-1-any.pkg.tar.zst
pacman --noconfirm -U mingw-w64-x86_64-protobuf-c-1.4.1-1-any.pkg.tar.zst mingw-w64-x86_64-protobuf-21.9-1-any.pkg.tar.zst
```

### Other systems

- install protobufv21
- point `CMAKE_PREFIX_PATH` environment variable to Protobuf v21 installation.

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