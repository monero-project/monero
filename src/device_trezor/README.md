# Trezor hardware wallet support

This module adds [Trezor] hardware support to Monero.


## Basic information

Trezor integration is based on the following original proposal: https://github.com/ph4r05/monero-trezor-doc

A custom high-level transaction signing protocol uses Trezor in a similar way a cold wallet is used. 
Transaction is build incrementally on the device. 

Trezor implements the signing protocol in [trezor-firmware] repository, in the [monero](https://github.com/trezor/trezor-firmware/tree/master/core/src/apps/monero) application.
Please, refer to [monero readme](https://github.com/trezor/trezor-firmware/blob/master/core/src/apps/monero/README.md) for more information.

## Wire protocols

The signing protocol above is carried over one of two wire protocols.

Protocol v1 is the long-standing plaintext framing used by the Trezor One,
Model T, Safe 3 and Safe 5. It is implemented by `ProtocolV1` in
`trezor/protocol.cpp` and is unchanged.

Protocol v2, the Trezor Host Protocol (THP), is an end-to-end-encrypted
protocol built on the Noise Protocol Framework. The Trezor Safe 7 speaks it
exclusively and has no v1 support at all, so THP is required to use a Safe 7
with Monero. It is implemented under `trezor/thp/`; see
[trezor/thp/README.md](trezor/thp/README.md) for the wire format, the
handshake and the pairing flow.

Which protocol a device speaks is detected rather than configured.
`ProtocolAutoDetect` (`trezor/thp/auto_detect.cpp`) is installed by default;
on the first `open()` it sends a THP channel-allocation request on the
broadcast channel. A Safe 7 answers with a channel-allocation response and
the connection continues over THP on the channel just allocated. A v1 device
answers with a v1 `Failure` report, which is not a valid THP frame, and the
probe falls back to `ProtocolV1`. The result is cached per device path, so
one probe per attach is enough. Setting `TREZOR_FORCE_THP=1` or
`TREZOR_FORCE_V1=1` in the environment skips the probe and forces one
protocol, which is only intended for diagnostics.

THP authenticates the device the first time it is seen: the user confirms a
6-digit code shown on the Trezor, and the device then issues a long-lived
credential the host presents on later connections. That credential, together
with the host's own static key, is kept in `.trezor/thp_store.bin` under the
Monero data directory; the store creates that directory and file with
owner-only access. If the file is deleted, the next connection simply asks
for the pairing code again.

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