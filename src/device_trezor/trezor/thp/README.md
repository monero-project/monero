# Trezor Host Protocol v2 (THP) over USB

This directory implements the host side of the Trezor Host Protocol v2, the
end-to-end-encrypted wire protocol that the Trezor Safe 7, and any future
Trezor device that drops the legacy v1 protocol, speaks exclusively. The
transport is USB: THP frames are carried in 64-byte USB-HID packets, the same
carrier the existing v1 transports use. The protocol is specified at:

  - <https://docs.trezor.io/trezor-firmware/common/thp/specification.html>
  - `docs/common/thp/specification.md` in trezor-firmware at commit
    `4c8a38f294d02bf092ec7995f4ae2274317b259d`.

THP is built on the Noise Protocol Framework
(`Noise_XX_25519_AESGCM_SHA256`), with one deviation described under
"Relationship to stock Noise XX" below.

## Layout

| File | Purpose |
| ---- | ------- |
| `framing.{hpp,cpp}` | Wire-level segmentation. `encode_frame` produces 64-byte USB chunks; `FrameAssembler` reassembles them and checks the CRC-32 trailer, computed with `boost::crc_32_type` (CRC-32/ISO-HDLC, the same function the device uses). |
| `noise.{hpp,cpp}` | Cryptographic primitives (X25519, SHA-256, HMAC-SHA-256, THP HKDF, AES-256-GCM, Trezor static-key masking) and the `NoiseXxInitiator` state machine that runs the host side of the handshake (HH0, HH1, HH2/HH3). |
| `pairing.{hpp,cpp}` | CodeEntry pairing FSM (host side). Runs the CPace exchange (IRTF `CPACE-X25519-SHA512` symmetric suite) so the user can confirm the 6-digit code the Trezor displays and the device can issue a long-lived credential. |
| `protocol_v2.{hpp,cpp}` | Channel allocation on the broadcast CID 0xFFFF, and `ProtocolV2 : public Protocol`, a drop-in replacement for `ProtocolV1` that performs allocation and handshake on `session_begin` and AES-GCM-seals or opens application traffic on `write` / `read`. |
| `auto_detect.{hpp,cpp}` | `ProtocolAutoDetect : public Protocol`. Probes the attached device once and selects v1 or THP, then runs first-pairing (CodeEntry) and persists the device credential. |
| `store.{hpp,cpp}` | TOFU device-credential store. Remembers paired devices (trust on first use) so CodeEntry only runs once per device. |

## Protocol selection

`transport.cpp` installs `ProtocolAutoDetect` by default. On the first
`open()` it sends a `ChannelAllocationRequest` on the broadcast CID (a single
64-byte USB packet that `framing.cpp` produces) and inspects the reply: a
`ChannelAllocationResponse` means a THP device (Safe 7) and the
freshly-allocated channel is reused; the legacy v1 magic (or no THP response)
selects `ProtocolV1` for a Model T or earlier device. The decision is cached
per device-path so a single probe per attach suffices.

Two environment variables override the probe, mainly for diagnostics:

  - `TREZOR_FORCE_THP=1` always uses `ProtocolV2` (skip the probe).
  - `TREZOR_FORCE_V1=1` always uses `ProtocolV1` (skip the probe), useful if
    the THP probe misbehaves on a particular USB stack.

## Pairing

The handshake produces a confidential channel but, on its own, only gives
weak (TOFU) authentication of the device. Per the spec, mutual authentication
requires a one-time pairing step in which the user confirms a 6-digit code
displayed on the Trezor, after which Trezor issues a long-lived credential
the host can present on subsequent connections.

`pairing.cpp` implements the CodeEntry method: it runs the CPace exchange
(IRTF `CPACE-X25519-SHA512` symmetric suite) seeded with the user-entered
code, then verifies the device's commitment before accepting the issued
credential. The pairing-code prompt is supplied by the host or GUI through
the `on_pairing_code_request` callback that the device-glue layer registers;
the resulting credential is persisted by `store.cpp` so the CodeEntry flow
runs only once per device.

## Credential store

`store.cpp` keeps the host static keypair and one credential per paired
device in a single file, `<data dir>/.trezor/thp_store.bin`, where the data
directory is chosen by `device_trezor_base.cpp` and is always **per user and
per network**:

  - POSIX: `tools::get_default_data_dir()`, i.e. `~/.bitmonero`, with
    `testnet/`, `stagenet/` or `fake/` appended for the non-mainnet networks.
  - Windows: `CSIDL_APPDATA\<CRYPTONOTE_NAME>` rather than the
    `CSIDL_COMMON_APPDATA` that `get_default_data_dir()` resolves to. The
    common folder is shared by every local account on the machine, and the
    file holds a private key in the clear.

The layout is tag-prefixed records, documented field by field at the top of
`store.hpp`. Unknown tags are skipped on load, so the format can grow without
breaking an older wallet's file.

**The host static private key is written in plaintext.** That matches the
existing Trezor v1 flow, which persists hardware-wallet state without
additional encryption, but it only holds up while the file is unreadable to
other local accounts. So:

  - the `.trezor` directory is created and then `chmod`ed to `S_IRWXU`
    (0700), because `create_directories()` honours the umask and would
    otherwise leave it group- and world-readable;
  - on POSIX the file is written through `tools::private_file::create(...,
    O_EXCL)`, which opens it 0600, exclusively and flock-held, and unlinks it
    from its destructor, so no failure path strands the private key in a temp
    file. It is `fsync`ed and then moved into place with
    `tools::replace_file`, so a reader never sees a half-written store;
  - on Windows `private_file` cannot be used, because it opens with
    `FILE_FLAG_DELETE_ON_CLOSE` and the file has to survive the close that
    precedes the rename. The temp file there goes through
    `epee::file_io_utils::save_string_to_file`, is flushed, and is removed
    explicitly on every failure path. Confidentiality there comes from the
    per-user directory, which is why `default_path()` *requires* one rather
    than setting a DACL of its own;
  - the temp-file suffix is drawn from the CSPRNG on both, so a temp name
    cannot be planted in advance and hijacked.

Deleting the file is safe and costs only a re-pairing: the next connect
regenerates a host static key and runs CodeEntry again.

## Relationship to stock Noise XX

THP instantiates `Noise_XX_25519_AESGCM_SHA256` as the Noise Protocol
Framework defines it, revision 34. There is exactly one deviation, plus two
constructions that read like deviations but are not. Both kinds are recorded
here, because both cost time to establish when reading this code against the
framework.

The one real deviation is **Trezor static-key masking**. In stock XX the
responder's message carries its static public key; in THP it carries
`X25519(SHA-256(static_pub || eph_pub), static_pub)` instead, and the host
uses that masked value directly as the peer static key for the `es` DH and
the HKDF that follows. The device's ephemeral key is fresh on every
connection, so the masked value rotates every session and cannot be used to
recognise a device from the wire. Recognising a previously paired device
therefore means recomputing the mask over each stored static key with this
session's device ephemeral key and comparing, which is what
`NoiseXxInitiator::consume_init_response` does.

Two constructions that are stock Noise and must not be "corrected":

  - **The initial chaining key is the literal protocol name, not its hash.**
    `Noise_XX_25519_AESGCM_SHA256` is 28 ASCII bytes, and with the four
    trailing NUL bytes it is exactly 32 bytes, which is HASHLEN. Section 5.2
    (`InitializeSymmetric`) says to zero-pad rather than hash in that case,
    and then to set `ck = h`. `PROTOCOL_NAME` in `noise.cpp` is that 32-byte
    value and is passed straight into the first HKDF.
  - **The device properties are mixed into `h` before the ephemeral keys.**
    That is `MixHash(prologue)` from section 5.3 (`HandshakeState`), with the
    `DeviceProperties` blob from `ChannelAllocationResponse` as the prologue.
    Binding the allocation response into the handshake hash is what stops a
    device from downgrading the pairing methods it advertised.

Nonce encoding is stock too, at every counter value and not just at 0 and 1.
Section 12.4 (AESGCM) specifies the 96-bit nonce as 32 zero bits followed by
the **big-endian** encoding of `n`; little-endian belongs to ChaChaPoly in
section 12.3. `crypto::iv_for_nonce` writes four zero bytes followed by the
64-bit counter big-endian, which is exactly what the spec's `0^96` (counter
0) and `0^95 || 1` (counter 1) notation denotes, and it matches the firmware:
`full_nonce[4..].copy_from_slice(&nonce_counter.to_be_bytes())` in
`core/embed/rust/src/thp/crypto.rs`.

## Test vectors

The deterministic upstream vectors used by
`tests/unit_tests/device_trezor_thp.cpp` (AES-GCM under THP-derived IVs, the
THP HKDF cascade, the IV layout, CRC-32, and a byte-exact wire fixture) were
taken from trezor-firmware at commit
`bc97a6b2b00068cb36a13da27ddaba8306314870`:

  - `core/tests/test_trezor.wire.thp.crypto.py`
  - `core/tests/test_trezor.wire.thp.checksum.py`
  - `core/tests/test_trezor.wire.thp.writer.py`

Upstream deleted all three in commit
`00ec9a7f8b6388f45e7825f48d6cb5b42d27d795` ("feat(core): switch from Python
THP implementation to Rust-based one"), and the Rust implementation that
replaced them ships no vector tables, so `bc97a6b2b` is the last revision at
which the published vectors can be read. That is why they are cited by commit
rather than by branch.

The remaining vectors come from the RFCs listed below, and the handshake as a
whole is exercised end to end against an in-process simulator that follows
the spec's state machine
(`device_trezor_thp.cpp:thp_handshake.full_handshake_self_test`).

## References

  - THP specification: <https://docs.trezor.io/trezor-firmware/common/thp/>
  - Noise Protocol Framework, revision 34: <https://noiseprotocol.org/noise.html>
  - RFC 7748 (X25519): <https://datatracker.ietf.org/doc/html/rfc7748>
  - RFC 4231 (HMAC-SHA-256 test vectors): <https://datatracker.ietf.org/doc/html/rfc4231>
  - RFC 5869 (HKDF). THP's HKDF is Noise's own two-output HKDF from section
    4.3, which is HKDF-Extract followed by HKDF-Expand with an empty info
    string: <https://datatracker.ietf.org/doc/html/rfc5869>
  - draft-irtf-cfrg-cpace (CPace, used by CodeEntry pairing): <https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-cpace>
  - NIST SP 800-38D (AES-GCM): <https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf>
  - Upstream tracking issue (Monero core): <https://github.com/monero-project/monero/issues/10368>
  - Upstream tracking issue (Monero GUI): <https://github.com/monero-project/monero-gui/issues/4517>
