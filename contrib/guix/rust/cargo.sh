#!/usr/bin/env bash
set -e -o pipefail

# Environment variables for determinism
export LC_ALL=C
export SOURCE_DATE_EPOCH=1397818193
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"

# Given a package name and an output name, return the path of that output in our
# current guix environment
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|\x29*$||' \
              --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}

echo "Fetching rust dependencies.."

cd /monero/src/fcmp_pp/fcmp_pp_rust

# error: the cargo feature `public-dependency` requires a nightly version of Cargo, but this is the `stable` channel
#
# https://doc.rust-lang.org/cargo/reference/unstable.html#public-dependency
export RUSTC_BOOTSTRAP=1

# Assert that `Cargo.lock` will remain unchanged
CARGO_OPTIONS="--locked"

# https://github.com/rust-lang/wg-cargo-std-aware/issues/23#issuecomment-1445119470
# If we don't vendor std, we'll run into: 'error: no matching package named `compiler_builtins` found' during build.
CARGO_OPTIONS+=" --sync $(store_path rust-std)/library/Cargo.toml"

# Vendor fcmp_pp_rust + std library deps
cargo vendor ${CARGO_OPTIONS} /rust/vendor

cp -r "$(store_path rust-std)/library" /rust/library

cd /rust/vendor

# `cargo vendor` includes dozens of packages that aren't needed to build the standard library.
# We can't simply remove these packages, because cargo expects them to be there.
# Instead, we replace the packages with a stub, which is sufficient to pass cargo's checks.
while IFS= read -r line; do
    cd "$line"
    find . -not -path "." -not -name "Cargo.toml" -not -name ".cargo-checksum.json" -delete
    mkdir src
    touch src/lib.rs

    # Cargo.toml must remain unaltered.
    # src/lib.rs must exist, but may be empty.
    # 'e3b0...b855' is equivalent to `echo -n "" | sha256sum -`
    # We can't set 'package' to the empty hash, as this might conflict with fcmp_pp_rust's Cargo.lock, resulting
    # in the same error.
    cat .cargo-checksum.json \
      | jq '{files: {"Cargo.toml": .files["Cargo.toml"]}, package}' \
      | jq '.files += {"src/lib.rs": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"}' \
      > .cargo-checksum.json.tmp
    mv .cargo-checksum.json.tmp .cargo-checksum.json
    cd ..
done < "/monero/contrib/guix/rust/stubs"

cd /rust

# Create deterministic archive
find . -print0 \
  | sort --zero-terminated \
  | tar --create --no-recursion --mode='u+rw,go+r-w,a+X' --null --files-from=- \
  | gzip -9n > "/monero/$RUST_DEPS_ARCHIVE"
