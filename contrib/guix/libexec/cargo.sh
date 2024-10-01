#!/usr/bin/env bash
set -ex -o pipefail

# Environment variables for determinism
export LC_ALL=C
export SOURCE_DATE_EPOCH=1397818193
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"

allowed_pattern=$(awk '{print "^./" $1 "$" }' /monero/contrib/guix/libexec/allowed_deps.txt | paste -sd '|' -)

cd /
wget --no-check-certificate https://static.rust-lang.org/dist/rustc-1.77.1-src.tar.gz
echo "ee106e4c569f52dba3b5b282b105820f86bd8f6b3d09c06b8dce82fb1bb3a4a1 rustc-1.77.1-src.tar.gz" | sha256sum -c
tar xf rustc-1.77.1-src.tar.gz
cp rustc-1.77.1-src/Cargo.lock rustc-1.77.1-src/library/std/

# Vendor fcmp_pp_rust deps
cd /monero/src/fcmp_pp/fcmp_pp_rust
RUSTC_BOOTSTRAP=1 cargo vendor --locked --sync /rustc-1.77.1-src/library/std/Cargo.toml /vendor

# Create deterministic dependency archive
cd /vendor

find . -mindepth 1 -maxdepth 1 -type d | grep -Ev "($allowed_pattern)" | xargs rm -rf

find . -type f -regex ".*\.\(a\|dll\|exe\|lib\)$" -delete
find . -print0 \
  | sort --zero-terminated \
  | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
  | gzip -9n > "/fcmp_pp_rust-0.0.0-deps.tar.gz"

mv /fcmp_pp_rust-0.0.0-deps.tar.gz /monero/
