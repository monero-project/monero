#!/usr/bin/env sh
set -e

cd "$(dirname "$0")"

# Revision the five long-standing protos are vendored from.
COMMON_REF="bc28c316d05bf1e9ebfe3d7df1ab25831d98d168"
# messages-thp.proto does not exist at COMMON_REF, so it is taken from a
# later revision of the same repository rather than from a second upstream.
# Bumping COMMON_REF instead would also rewrite the five protos above,
# which is unrelated to Trezor Host Protocol v2 support.
THP_REF="ff5f1525632ab09b286f765e5583df5207e222b7"

if [ ! -d "trezor-common" ]; then
  git clone https://github.com/trezor/trezor-common.git
fi

cd trezor-common
git fetch
git reset --hard "${COMMON_REF}"
cd ..

mkdir -p protob
rm -f protob/*.proto protob/COPYING

proto_files="messages.proto messages-common.proto messages-monero.proto messages-management.proto messages-debug.proto"

for file in ${proto_files}
do
  cp "trezor-common/protob/${file}" protob/
done
cp "trezor-common/COPYING" protob/

# messages-thp.proto is read out of the object database at THP_REF, so both
# pins are verified by git the same way. Two edits are applied and they are
# the only difference between protob/messages-thp.proto and upstream:
#
#   - the import is redirected from options.proto to messages.proto. This
#     tree does not vendor options.proto; the custom options the file needs
#     (wire_in, wire_out, bitcoin_only, include_in_bitcoin_only) are all
#     declared in messages.proto at COMMON_REF. Vendoring options.proto as
#     well is not an alternative, because it declares the same extension
#     numbers as messages.proto and protoc rejects the duplicates.
#   - the (wire_enum) and (internal_only) options are dropped. Both are
#     declared only in options.proto and only at revisions newer than
#     COMMON_REF, so protoc cannot resolve them here. They are metadata for
#     trezor-firmware's own code generator and affect neither the wire
#     format nor the generated C++.
#
# See protob/README.md for provenance and licensing.
git -C trezor-common show "${THP_REF}:protob/messages-thp.proto" > protob/messages-thp.proto
# POSIX sed has no portable -i; edit via a temp file instead.
sed -e 's|^import "options.proto";|import "messages.proto";|' \
    -e '/option (wire_enum) = true;/d' \
    -e '/option (internal_only) = true;/d' \
    protob/messages-thp.proto > protob/messages-thp.proto.tmp
mv protob/messages-thp.proto.tmp protob/messages-thp.proto

cd protob
echo "Checksums:"
find . -type f -print0 | env LC_ALL=C sort -z | xargs -r0 sha256sum
