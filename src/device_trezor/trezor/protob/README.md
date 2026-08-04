# Vendored Trezor protobuf definitions

The `.proto` files in this directory come from Trezor's `trezor-common`
repository. They are refreshed by `../fetch_protob.sh`, which is a hand-run
developer tool: the build compiles the committed files with `protoc` at
configure time (see `cmake/CheckTrezor.cmake`) and never runs the script.

Upstream: <https://github.com/trezor/trezor-common>

| File | Pinned revision |
| ---- | --------------- |
| `messages.proto`, `messages-common.proto`, `messages-monero.proto`, `messages-management.proto`, `messages-debug.proto`, `COPYING` | `bc28c316d05bf1e9ebfe3d7df1ab25831d98d168` |
| `messages-thp.proto` | `ff5f1525632ab09b286f765e5583df5207e222b7` |

`messages-thp.proto` carries the Trezor Host Protocol v2 message set, which
did not exist at the older pin. Taking it from a later revision of the same
repository keeps the vendoring to a single upstream. Bumping the older pin
instead would rewrite all five files above, including semantic changes to
`messages-monero.proto` that have nothing to do with THP.

## Licence

`COPYING` in this directory is a verbatim copy of `COPYING` at the root of
`trezor-common`. It is the GNU Lesser General Public License version 3.
SPDX identifier: `LGPL-3.0-only`. Upstream ships no per-file licence headers
and makes no "or any later version" election, so the bare LGPL-3.0 text is
the whole grant.

`trezor-common` is a read-only export of the `common/` directory of the
`trezor-firmware` monorepo. The same files inside that monorepo are governed
by `common/COPYING`, which is the same LGPL-3.0 text. They are not governed
by `trezor-firmware`'s repository-root `COPYING`, which is GPL-3.0 and
covers the firmware itself.

## Local modifications

`messages-thp.proto` is not a byte-for-byte copy of upstream.
`fetch_protob.sh` applies exactly two edits, and nothing else in the file is
touched:

  1. `import "options.proto";` becomes `import "messages.proto";`. This tree
     does not vendor `options.proto`. Every custom option the file uses
     (`wire_in`, `wire_out`, `bitcoin_only`, `include_in_bitcoin_only`) is
     declared in `messages.proto` at the pin above. Vendoring
     `options.proto` as well is not an alternative: it declares the same
     extension numbers as `messages.proto`, and protoc rejects the
     duplicates.
  2. `option (wire_enum) = true;` and `option (internal_only) = true;` are
     deleted, six lines in total. Both extensions are declared in
     `options.proto` only, so protoc cannot resolve them here. They are
     metadata for trezor-firmware's own code generator and change neither
     the wire format nor the generated C++.

To reproduce the vendored file:

```shell
git clone https://github.com/trezor/trezor-common.git
git -C trezor-common show ff5f1525632ab09b286f765e5583df5207e222b7:protob/messages-thp.proto \
  | sed -e 's|^import "options.proto";|import "messages.proto";|' \
        -e '/option (wire_enum) = true;/d' \
        -e '/option (internal_only) = true;/d'
```

The other files are verbatim copies of upstream at the pin above, with one
exception: `messages-management.proto` carries a one-word comment typo fix
made in this tree by commit `2c327aa32` ("fix spelling in comments and
docs").
