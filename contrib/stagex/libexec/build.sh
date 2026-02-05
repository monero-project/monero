#!/usr/bin/env bash
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Copyright (c) 2022-2025 The Monero Project
export LC_ALL=C
set -e -o pipefail
export TZ=UTC
umask 0022

# shellcheck source=contrib/shell/git-utils.bash
source contrib/shell/git-utils.bash

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
    DIST_ARCHIVE_BASE: ${DIST_ARCHIVE_BASE:?not set}
    VERSION: ${VERSION:?not set}
    HOST: ${HOST:?not set}
    COMMIT_TIMESTAMP: ${COMMIT_TIMESTAMP:?not set}
    JOBS: ${JOBS:?not set}
    DISTSRC: ${DISTSRC:?not set}
    OUTDIR: ${OUTDIR:?not set}
    LOGDIR: ${LOGDIR:?not set}
    OPTIONS: ${OPTIONS}
EOF

ACTUAL_OUTDIR="${OUTDIR}"
OUTDIR="${DISTSRC}/output"
DISTNAME="monero-${HOST}-${VERSION}"

# Use a fixed timestamp for depends builds so hashes match across commits that
# don't make changes to the build system. This timestamp is only used for depends
# packages. Source archive and binary tarballs use the commit date.
export SOURCE_DATE_EPOCH=1397818193


#####################
# Environment Setup #
#####################

# Collect some information about the build environment to help debug potential reproducibility issues
mkdir -p "${LOGDIR}"
#printenv | sort | grep -v '^\(BASE_CACHE=\|DISTNAME=\|DISTSRC=\|OUTDIR=\|LOGDIR=\|SOURCES_PATH=\|JOBS=\|OPTIONS=\|DEPENDS_ONLY=\)' > ${LOGDIR}/stagex-env.txt

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
BASEPREFIX="${PWD}/contrib/depends"

prepend_to_search_env_var() {
    export "${1}=${2}${!1:+:}${!1}"
}

[ -e /usr/bin/sh ]  || ln -s --no-dereference /usr/bin/bash  /usr/bin/sh

# Environment variables for determinism
# todo: segfaults
#export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"

####################
# Depends Building #
####################

mkdir -p "${OUTDIR}"

# Log the depends build ids
make -C contrib/depends --no-print-directory HOST="$HOST" print-final_build_id_long | tr ':' '\n' > ${LOGDIR}/depends-hashes.txt

# Build the depends tree, overriding variables that assume multilib gcc
make -C contrib/depends --jobs="$JOBS" HOST="$HOST" \
                                   ${V:+V=1} \
                                   ${SOURCES_PATH+SOURCES_PATH="$SOURCES_PATH"} \
                                   ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} \
                                   ${SDK_PATH+SDK_PATH="$SDK_PATH"} \
                                   OUTDIR="$OUTDIR" \
                                   LOGDIR="$LOGDIR"

# Log the depends package hashes
DEPENDS_PACKAGES="$(make -C contrib/depends --no-print-directory HOST="$HOST" print-all_packages)"
DEPENDS_CACHE="$(make -C contrib/depends --no-print-directory ${BASE_CACHE+BASE_CACHE="$BASE_CACHE"} print-BASE_CACHE)"

# Keep a record of the depends packages and their hashes that will be used for
# our build. If there is a reproducibility issue, comparing this log file could
# help narrow down which package is responsible for the defect.
{
    for package in ${DEPENDS_PACKAGES}; do
        cat "${DEPENDS_CACHE}/${HOST}/${package}"/*.hash
    done
} | sort -k2 > "${LOGDIR}/depends-packages.txt"

# Stop here if we're only building depends packages. This is useful when
# debugging reproducibility issues in depends packages. Skips ahead to the next
# target, so we don't spend time building Monero binaries.
if [[ -n "$DEPENDS_ONLY" ]]; then
    exit 0
fi

###########################
# Source Tarball Building #
###########################

# Use COMMIT_TIMESTAMP for the source and release binary archives
export SOURCE_DATE_EPOCH=${COMMIT_TIMESTAMP}
#export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"

GIT_ARCHIVE="${DIST_ARCHIVE_BASE}/monero-source-${VERSION}.tar.gz"

git config --global --add safe.directory /monero

# Create the source tarball if not already there
# This uses `git ls-files --recurse-submodules` instead of `git archive` to make
# sure submodules are included in the source archive.
if [ ! -e "$GIT_ARCHIVE" ]; then
    mkdir -p "$(dirname "$GIT_ARCHIVE")"
    git ls-files --recurse-submodules \
    | sort \
    | tar --create --files-from=- \
    | gzip -9n > ${GIT_ARCHIVE}
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
    *linux-musl*)  HOST_LDFLAGS="-static-pie" ;;
    *mingw*)  HOST_LDFLAGS="-Wl,--no-insert-timestamp" ;;
esac

export GIT_DISCOVERY_ACROSS_FILESYSTEM=1
# Force Trezor support for release binaries
export USE_DEVICE_TREZOR_MANDATORY=1

# Make $HOST-specific native binaries from depends available in $PATH
export PATH="${BASEPREFIX}/${HOST}/native/bin:${PATH}"
mkdir -p "$DISTSRC"
(
    cd "$DISTSRC"

    # Extract the source tarball
    tar -xf "${GIT_ARCHIVE}"

    # Setup the directory where our Monero build for HOST will be
    # installed. This directory will also later serve as the input for our
    # binary tarballs.
    INSTALLPATH="${DISTSRC}/installed/${DISTNAME}"
    mkdir -p "${INSTALLPATH}"

    # Ensure rpath in the resulting binaries is empty
    CMAKEFLAGS="-DCMAKE_SKIP_RPATH=ON"

    # We can't check if submodules are checked out because we're building in an
    # extracted source archive. The guix-build script makes sure submodules are
    # checked out before starting a build.
    CMAKEFLAGS+=" -DMANUAL_SUBMODULES=1"

    # Enabling stack traces causes a compilation issue on Linux targets.
    # Gitian builds did not enable stack traces either, so this is not a
    # regression.
    case "$HOST" in
        *linux-musl*)  CMAKEFLAGS+=" -DSTACK_TRACE=OFF" ;;
    esac

    # Configure this DISTSRC for $HOST
    # shellcheck disable=SC2086
    env CFLAGS="${HOST_CFLAGS}" CXXFLAGS="${HOST_CXXFLAGS}" \
    cmake --toolchain "${BASEPREFIX}/${HOST}/share/toolchain.cmake" -S . -B build \
      -DCMAKE_INSTALL_PREFIX="${INSTALLPATH}" \
      -DCMAKE_EXE_LINKER_FLAGS="${HOST_LDFLAGS}" \
      -DCMAKE_SHARED_LINKER_FLAGS="${HOST_LDFLAGS}" \
      ${CMAKEFLAGS}

    make -C build --jobs="$JOBS"

    # Copy docs
    cp README.md LICENSE docs/ANONYMITY_NETWORKS.md "${INSTALLPATH}"

    # Copy binaries
    cp -a build/bin/* "${INSTALLPATH}"

    (
        cd installed

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

rm -rf "$ACTUAL_OUTDIR"
mv --no-target-directory "$OUTDIR" "$ACTUAL_OUTDIR" \
    || ( rm -rf "$ACTUAL_OUTDIR" && exit 1 )
