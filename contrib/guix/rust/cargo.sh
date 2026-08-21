#!/usr/bin/env bash
set -e -o pipefail

# Environment variables for determinism
export LC_ALL=C
export SOURCE_DATE_EPOCH=1397818193
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"
umask 0022

# Vendor fcmp_pp_rust deps
echo "Fetching rust dependencies.."
cd /monero/src/fcmp_pp/fcmp_pp_rust
cargo vendor --locked /rust/vendor > /tmp/config.toml

if ! diff --unified /monero/contrib/guix/rust/config.toml /tmp/config.toml; then
  echo ""
  echo "ERR: contrib/guix/rust/config.toml does not match the configuration"
  echo "     required by Cargo.lock. Update it as shown in the diff above."
  exit 1
fi

# Create deterministic archive
cd /rust
find . -print0 \
  | sort --zero-terminated \
  | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
  | gzip -9n > "/monero/$RUST_DEPS_ARCHIVE"
