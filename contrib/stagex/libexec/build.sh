#!/usr/bin/env bash
export LC_ALL=C.UTF-8
set -e -o pipefail
export TZ=UTC
umask 0022

if [ -n "$V" ]; then
    # Print both unexpanded (-v) and expanded (-x) forms of commands as they are
    # read from this file.
    set -vx
    # Set VERBOSE for CMake-based builds
    export VERBOSE="$V"
fi

# Check that required environment variables are set
cat << EOF
Required environment variables as seen inside the container:
    STAGEX_ENVIRONMENT: ${STAGEX_ENVIRONMENT:?not set}
    VERSION: ${VERSION:?not set}
    HOST: ${HOST:?not set}
    JOBS: ${JOBS:?not set}
EOF

#####################
# Environment Setup #
#####################

STAGEX_DIR="/monero/stagex"

DEPENDS_SOURCES="${STAGEX_DIR}/depends-sources/"
mkdir -p "${DEPENDS_SOURCES}"
DEPENDS_CACHE="${STAGEX_DIR}/depends-cache/${HOST}-${STAGEX_ENVIRONMENT:0:12}"
mkdir -p "${DEPENDS_CACHE}"

BUILD_DIR="${STAGEX_DIR}/build"
VERSION_DIR="${BUILD_DIR}/${VERSION}"
mkdir -p "${VERSION_DIR}"
(
   cd "${BUILD_DIR}"
   rm -f latest
   ln -sf "${VERSION}" "latest"
)

LOGDIR="${VERSION_DIR}/logs/${HOST}"
mkdir -p "${LOGDIR}"
DISTSRC="${VERSION_DIR}/src/${HOST}"
rm -rf "${DISTSRC}"
mkdir -p "${DISTSRC}"
OUTDIR="${VERSION_DIR}/output/${HOST}"
rm -rf "${OUTDIR}"
DIST_ARCHIVE_BASE="${VERSION_DIR}/output/dist-archive"
mkdir -p "${DIST_ARCHIVE_BASE}"

DISTNAME="monero-${HOST}-${VERSION}"

# Use a fixed timestamp for depends builds so hashes match across commits that
# don't make changes to the build system. This timestamp is only used for depends
# packages. Source archive and binary tarballs use the commit date.
export SOURCE_DATE_EPOCH=1397818193

# Collect some information about the build environment to help debug potential reproducibility issues
printenv | sort > ${LOGDIR}/stagex-env.txt

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
BASEPREFIX="${PWD}/contrib/depends"
rm -rf "${BASEPREFIX}/work"

[ -e /usr/bin/sh ]  || ln -s --no-dereference /usr/bin/bash  /usr/bin/sh

# Environment variables for determinism
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"

case "$HOST" in
    *linux-musl*)
      ;;
    *mingw*)
      ;;
    *)
      build_CC=clang
      build_CXX=clang++
      ;;
esac

####################
# Depends Building #
####################

# Log the depends build ids
make -C contrib/depends --no-print-directory HOST="$HOST" print-final_build_id_long | tr ':' '\n' > ${LOGDIR}/depends-hashes.txt

# Build the depends tree, overriding variables that assume multilib gcc
make -C contrib/depends --jobs="${JOBS}" \
                        HOST="$HOST" \
                        ${V:+V=1} \
                        SOURCES_PATH="${DEPENDS_SOURCES}" \
                        BASE_CACHE_HOST="${DEPENDS_CACHE}" \
                        ${build_CC+build_CC="$build_CC"} \
                        ${build_CXX+build_CXX="$build_CXX"} \
                        android_AR=/usr/bin/llvm-ar \
                        android_RANLIB=/usr/bin/llvm-ranlib \
                        android_NM=/usr/bin/llvm-nm \
                        android_STRIP=/usr/bin/llvm-strip

# Log the depends package hashes
DEPENDS_PACKAGES="$(make -C contrib/depends --no-print-directory HOST="$HOST" print-all_packages)"

# Keep a record of the depends packages and their hashes that will be used for
# our build. If there is a reproducibility issue, comparing this log file could
# help narrow down which package is responsible for the defect.
{
    for package in ${DEPENDS_PACKAGES}; do
        cat "${DEPENDS_CACHE}/${package}"/*.hash
    done
} | sort -k2 > "${LOGDIR}/depends-packages.txt"

###########################
# Source Tarball Building #
###########################

git config --global --add safe.directory /monero

export SOURCE_DATE_EPOCH="$(git -c log.showSignature=false log --format=%at -1)"
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"

GIT_ARCHIVE="${DIST_ARCHIVE_BASE}/monero-source-${VERSION}.tar.gz"

# Create the source tarball if not already there
# This uses `git ls-files --recurse-submodules` instead of `git archive` to make
# sure submodules are included in the source archive.
if [ ! -e "$GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$GIT_ARCHIVE")"
    git ls-files --recurse-submodules \
    | sort \
    | tar --create --transform "s,^,monero-source-${VERSION}/," --mode='u+rw,go+r-w,a+X' --files-from=- \
    | gzip -9n > "${GIT_ARCHIVE}"
    sha256sum "$GIT_ARCHIVE"
fi

###########################
# Binary Tarball Building #
###########################

# CFLAGS
case "$HOST" in
    *linux-musl*)
        HOST_CFLAGS+=" -ffile-prefix-map=${PWD}=." ;;
esac

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"

# LDFLAGS
case "$HOST" in
    *linux-musl)  HOST_LDFLAGS="-Wl,-z,stack-size=5242880" ;; # THREAD_STACK_SIZE
    *mingw*)  HOST_LDFLAGS="-Wl,--no-insert-timestamp" ;;
esac

# STATIC_FLAGS
case "$HOST" in
    *linux-musl)  STATIC_FLAGS="-static-pie" ;;
esac

export RUSTC_BOOTSTRAP=1

# See: https://rust-lang.github.io/rust-project-goals/2025h1/build-std.html
CARGO_OPTIONS="-Zbuild-std=std,panic_abort;"

export GIT_DISCOVERY_ACROSS_FILESYSTEM=1
# Force Trezor support for release binaries
export USE_DEVICE_TREZOR_MANDATORY=1

# Make $HOST-specific native binaries from depends available in $PATH
export PATH="${BASEPREFIX}/${HOST}/native/bin:${PATH}"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

    (
        cd src/fcmp_pp/fcmp_pp_rust
        mkdir -p .cargo
        cp /config.toml .cargo/
    )

    # Remove blobs
    rm -rf tests/data

    # Setup the directory where our Monero build for HOST will be
    # installed. This directory will also later serve as the input for our
    # binary tarballs.
    INSTALLPATH="${DISTSRC}/installed/${DISTNAME}"
    mkdir -p "${INSTALLPATH}"

    # Ensure rpath in the resulting binaries is empty
    CMAKEFLAGS="-DCMAKE_SKIP_RPATH=ON"

    # We can't check if submodules are checked out because we're building in an
    # extracted source archive. The stagex-build script makes sure submodules are
    # checked out before starting a build.
    CMAKEFLAGS+=" -DMANUAL_SUBMODULES=1"

    # Turn off unused default options
    CMAKEFLAGS+=" -DCOMPILER_CACHE=none -DBUILD_DOCUMENTATION=OFF"

    # Configure this DISTSRC for $HOST
    # shellcheck disable=SC2086
    env CFLAGS="${HOST_CFLAGS}" CXXFLAGS="${HOST_CXXFLAGS}" \
    cmake --toolchain "${BASEPREFIX}/${HOST}/share/toolchain.cmake" -S . -B build \
      -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
      -DCMAKE_EXE_LINKER_FLAGS="${HOST_LDFLAGS}" \
      -DCMAKE_SHARED_LINKER_FLAGS="${HOST_LDFLAGS}" \
      -DCARGO_OPTIONS="${CARGO_OPTIONS}" \
      ${STATIC_FLAGS+-DSTATIC_FLAGS="${STATIC_FLAGS}"} \
      ${CMAKEFLAGS}

    make -C build --jobs="${JOBS}"

    # Copy docs
    cp README.md LICENSE docs/ANONYMITY_NETWORKS.md "${INSTALLPATH}"

    # Copy binaries
    cp -a build/bin/* "${INSTALLPATH}"

    (
        cd installed

        mkdir -p "${OUTDIR}"

        # Finally, deterministically produce binary tarballs ready for release
        case "$HOST" in
            *mingw*)
                find "${DISTNAME}/" -print0 \
                    | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
                find "${DISTNAME}/" \
                    | sort \
                    | zip -X@ "${OUTDIR}/${DISTNAME}.zip" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}.zip" && exit 1 )
                ;;
            *)
                find "${DISTNAME}/" -print0 \
                    | xargs -0r touch --no-dereference --date="@${SOURCE_DATE_EPOCH}"
                find "${DISTNAME}/" \
                    | sort \
                    | tar --no-recursion -c -T - \
                    | bzip2 -9 > "${OUTDIR}/${DISTNAME}.tar.bz2" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}.tar.bz2" && exit 1 )
                ;;
        esac
    )
)  # $DISTSRC
