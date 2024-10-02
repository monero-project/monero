#!/usr/bin/env sh
set -e

cd "$(dirname "$0")"

if [ ! -d "trezor-common" ]; then
  git clone https://github.com/trezor/trezor-common.git
fi

cd trezor-common
git fetch
git reset --hard bc28c316d05bf1e9ebfe3d7df1ab25831d98d168
cd ..

rm -rf protob
mkdir protob

proto_files="messages.proto messages-common.proto messages-monero.proto messages-management.proto messages-debug.proto"

for file in ${proto_files}
do
  cp "trezor-common/protob/${file}" protob/
done
cp "trezor-common/COPYING" protob/

cd protob
echo "Checksums:"
find . -type f -print0 | env LC_ALL=C sort -z | xargs -r0 sha256sum
