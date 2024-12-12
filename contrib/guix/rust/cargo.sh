#!/usr/bin/env bash
set -e -o pipefail

# Environment variables for determinism
export LC_ALL=C
export SOURCE_DATE_EPOCH=1397818193
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"

echo "Fetching rust dependencies.."

cd /monero/src/fcmp_pp/fcmp_pp_rust

# Vendor fcmp_pp_rust deps
cargo vendor --locked /rust/vendor

cd /rust

# Create deterministic archive
find . -print0 \
  | sort --zero-terminated \
  | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
  | gzip -9n > "/monero/$RUST_DEPS_ARCHIVE"
