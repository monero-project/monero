#!/usr/bin/env bash
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Copyright (c) 2022-2024 The Monero Project
# Distributed under the MIT software license, see the accompanying
# file ../LICENSE.txt or http://www.opensource.org/licenses/mit-license.php.
export LC_ALL=C
set -e -o pipefail
export TZ=UTC

# shellcheck source=contrib/shell/git-utils.bash
source contrib/shell/git-utils.bash

# Although Guix _does_ set umask when building its own packages (in our case,
# this is all packages in manifest.scm), it does not set it for `guix
# environment`. It does make sense for at least `guix environment --container`
# to set umask, so if that change gets merged upstream and we bump the
# time-machine to a commit which includes the aforementioned change, we can
# remove this line.
#
# This line should be placed before any commands which creates files.
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
ls -1 /gnu/store | sort > ${LOGDIR}/guix-hashes.txt
printenv | sort | grep -v '^\(BASE_CACHE=\|DISTNAME=\|DISTSRC=\|OUTDIR=\|LOGDIR=\|SOURCES_PATH=\|JOBS=\|OPTIONS=\|DEPENDS_ONLY=\)' > ${LOGDIR}/guix-env.txt

# The depends folder also serves as a base-prefix for depends packages for
# $HOSTs after successfully building.
BASEPREFIX="${PWD}/contrib/depends"

# Given a package name and an output name, return the path of that output in our
# current guix environment
store_path() {
    grep --extended-regexp "/[^-]{32}-${1}-[^-]+${2:+-${2}}" "${GUIX_ENVIRONMENT}/manifest" \
        | head --lines=1 \
        | sed --expression='s|\x29*$||' \
              --expression='s|^[[:space:]]*"||' \
              --expression='s|"[[:space:]]*$||'
}

# These environment variables are automatically set by Guix, but don't
# necessarily point to the correct toolchain paths. This is fixed below.
unset LIBRARY_PATH
unset CPATH
unset C_INCLUDE_PATH
unset CPLUS_INCLUDE_PATH
unset OBJC_INCLUDE_PATH
unset OBJCPLUS_INCLUDE_PATH

NATIVE_GCC="$(store_path gcc-toolchain)"

export C_INCLUDE_PATH="${NATIVE_GCC}/include"
export CPLUS_INCLUDE_PATH="${NATIVE_GCC}/include/c++:${NATIVE_GCC}/include"
export OBJC_INCLUDE_PATH="${NATIVE_GCC}/include"
export OBJCPLUS_INCLUDE_PATH="${NATIVE_GCC}/include/c++:${NATIVE_GCC}/include"

case "$HOST" in
    *darwin*) export LIBRARY_PATH="${NATIVE_GCC}/lib" ;;
    *mingw*) export LIBRARY_PATH="${NATIVE_GCC}/lib" ;;
    *)
        NATIVE_GCC_STATIC="$(store_path gcc-toolchain static)"
        export LIBRARY_PATH="${NATIVE_GCC}/lib:${NATIVE_GCC_STATIC}/lib"
        ;;
esac

prepend_to_search_env_var() {
    export "${1}=${2}${!1:+:}${!1}"
}

# Set environment variables to point the CROSS toolchain to the right
# includes/libs for $HOST
case "$HOST" in
    *mingw*)
        # Determine output paths to use in CROSS_* environment variables
        case "$HOST" in
            i686-*)    CROSS_GLIBC="$(store_path "mingw-w64-i686-winpthreads")" ;;
            x86_64-*)  CROSS_GLIBC="$(store_path "mingw-w64-x86_64-winpthreads")" ;;
            *)         exit 1 ;;
        esac

        CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
        CROSS_GCC_LIB_STORE="$(store_path "gcc-cross-${HOST}" lib)"
        CROSS_GCC_LIBS=( "${CROSS_GCC_LIB_STORE}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
        CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

        # The search path ordering is generally:
        #    1. gcc-related search paths
        #    2. libc-related search paths
        #    2. kernel-header-related search paths (not applicable to mingw-w64 hosts)
        export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include"
        export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
        export CROSS_LIBRARY_PATH="${CROSS_GCC_LIB_STORE}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib"
        ;;
    *darwin*)
        # The CROSS toolchain for darwin uses the SDK and ignores environment variables.
        # See depends/hosts/darwin.mk for more details.
        ;;
    *android*)
        export LD_LIBRARY_PATH="$(find /gnu/store -maxdepth 1 -name "*zlib*" | sort | head -n 1)/lib:$(find /gnu/store -maxdepth 1 -name "*gcc-11*-lib" | sort | head -n 1)/lib"
        ;;
    *linux-gnu*)
        CROSS_GLIBC="$(store_path "glibc-cross-${HOST}")"
        CROSS_GLIBC_STATIC="$(store_path "glibc-cross-${HOST}" static)"
        CROSS_KERNEL="$(store_path "linux-libre-headers-cross-${HOST}")"
        CROSS_GCC="$(store_path "gcc-cross-${HOST}")"
        CROSS_GCC_LIB_STORE="$(store_path "gcc-cross-${HOST}" lib)"
        CROSS_GCC_LIBS=( "${CROSS_GCC_LIB_STORE}/lib/gcc/${HOST}"/* ) # This expands to an array of directories...
        CROSS_GCC_LIB="${CROSS_GCC_LIBS[0]}" # ...we just want the first one (there should only be one)

        export CROSS_C_INCLUDE_PATH="${CROSS_GCC_LIB}/include:${CROSS_GCC_LIB}/include-fixed:${CROSS_GLIBC}/include:${CROSS_KERNEL}/include"
        export CROSS_CPLUS_INCLUDE_PATH="${CROSS_GCC}/include/c++:${CROSS_GCC}/include/c++/${HOST}:${CROSS_GCC}/include/c++/backward:${CROSS_C_INCLUDE_PATH}"
        export CROSS_LIBRARY_PATH="${CROSS_GCC_LIB_STORE}/lib:${CROSS_GCC_LIB}:${CROSS_GLIBC}/lib:${CROSS_GLIBC_STATIC}/lib"
        ;;
    *freebsd*)
        ;;
    *)
        exit 1 ;;
esac


# Sanity check CROSS_*_PATH directories
IFS=':' read -ra PATHS <<< "${CROSS_C_INCLUDE_PATH}:${CROSS_CPLUS_INCLUDE_PATH}:${CROSS_LIBRARY_PATH}"
for p in "${PATHS[@]}"; do
    if [ -n "$p" ] && [ ! -d "$p" ]; then
        echo "'$p' doesn't exist or isn't a directory... Aborting..."
        exit 1
    fi
done

# Disable Guix ld auto-rpath behavior
case "$HOST" in
    *darwin*)
        # The auto-rpath behavior is necessary for darwin builds as some native
        # tools built by depends refer to and depend on Guix-built native
        # libraries
        #
        # After the native packages in depends are built, the ld wrapper should
        # no longer affect our build, as clang would instead reach for
        # x86_64-apple-darwin-ld from cctools
        ;;
    *android*)
        ;;
    *) export GUIX_LD_WRAPPER_DISABLE_RPATH=yes ;;
esac

# Make /usr/bin if it doesn't exist
[ -e /usr/bin ] || mkdir -p /usr/bin
[ -e /lib64 ] || mkdir /lib64

# Symlink file and env to a conventional path
[ -e /usr/bin/file ] || ln -s --no-dereference "$(command -v file)" /usr/bin/file
[ -e /usr/bin/env ]  || ln -s --no-dereference "$(command -v env)"  /usr/bin/env
[ -e /bin/bash ]  || ln -s --no-dereference "$(command -v bash)"  /bin/bash
[ -e /bin/sh ]  || ln -s --no-dereference "$(command -v sh)"  /bin/sh

# The Android NDK toolchain cannot (yet) be bootstrapped. The compiler binaries
# included in the NDK have their dynamic interpreter set to the standard x86_64
# interpreter path, which does not exist in this location in the Guix environment.
# The alternative was patchelf-ing all binaries included in the NDK, but this is
# more hacky and adds a dependency on patchelf for non-Guix builders.
[ -e /lib64/ld-linux-x86-64.so.2 ]  || ln -s --no-dereference "${NATIVE_GCC}/lib/ld-linux-x86-64.so.2"  /lib64/ld-linux-x86-64.so.2

# Determine the correct value for -Wl,--dynamic-linker for the current $HOST
#
# We need to do this because the dynamic linker does not exist at a standard path
# in the Guix container. Binaries wouldn't be able to start in other environments.
case "$HOST" in
    *linux-gnu*)
        glibc_dynamic_linker=$(
            case "$HOST" in
                x86_64-linux-gnu)      echo /lib64/ld-linux-x86-64.so.2 ;;
                arm-linux-gnueabihf)   echo /lib/ld-linux-armhf.so.3 ;;
                aarch64-linux-gnu)     echo /lib/ld-linux-aarch64.so.1 ;;
                riscv64-linux-gnu)     echo /lib/ld-linux-riscv64-lp64d.so.1 ;;
                i686-linux-gnu)        echo /lib/ld-linux.so.2 ;;
                *)                     exit 1 ;;
            esac
        )
        ;;
esac

export GLIBC_DYNAMIC_LINKER=${glibc_dynamic_linker}

# Environment variables for determinism
export TAR_OPTIONS="--owner=0 --group=0 --numeric-owner --mtime='@${SOURCE_DATE_EPOCH}' --sort=name"
export TZ="UTC"
case "$HOST" in
    *darwin*)
        # cctools AR, unlike GNU binutils AR, does not have a deterministic mode
        # or a configure flag to enable determinism by default, it only
        # understands if this env-var is set or not. See:
        #
        # https://github.com/tpoechtrager/cctools-port/blob/55562e4073dea0fbfd0b20e0bf69ffe6390c7f97/cctools/ar/archive.c#L334
        export ZERO_AR_DATE=yes
        ;;
esac

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
                                   LOGDIR="$LOGDIR" \
                                   x86_64_linux_CC=x86_64-linux-gnu-gcc \
                                   x86_64_linux_CXX=x86_64-linux-gnu-g++ \
                                   x86_64_linux_AR=x86_64-linux-gnu-gcc-ar \
                                   x86_64_linux_RANLIB=x86_64-linux-gnu-gcc-ranlib \
                                   x86_64_linux_NM=x86_64-linux-gnu-gcc-nm \
                                   x86_64_linux_STRIP=x86_64-linux-gnu-strip

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
    | gzip -9n > ${GIT_ARCHIVE}
    sha256sum "$GIT_ARCHIVE"
fi

###########################
# Binary Tarball Building #
###########################

# CFLAGS
case "$HOST" in
    *linux-gnu*)
        HOST_CFLAGS=$(find /gnu/store -maxdepth 1 -mindepth 1 -type d -exec echo -n " -ffile-prefix-map={}=/usr" \;)
        HOST_CFLAGS+=" -ffile-prefix-map=${PWD}=." ;;
esac

# CXXFLAGS
HOST_CXXFLAGS="$HOST_CFLAGS"
case "$HOST" in
    arm-linux-gnueabihf) HOST_CXXFLAGS+=" -Wno-psabi" ;;
esac

# LDFLAGS
case "$HOST" in
    *linux-gnu*)  HOST_LDFLAGS="-Wl,--as-needed -Wl,--dynamic-linker=$glibc_dynamic_linker -static-libstdc++" ;;
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
    tar --strip-components=1 -xf "${GIT_ARCHIVE}"

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

    # Binaries should not contain references to the store path
    for binary in "build/bin"/*; do
        if strings "$binary" | grep -q "/gnu/store"; then
            echo "ERR: ${binary} contains unexpected string: /gnu/store"
            exit 1
        fi
    done

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
                    | tar --no-recursion --owner=0 --group=0 -c -T - \
                    | bzip2 -9 > "${OUTDIR}/${DISTNAME}.tar.bz2" \
                    || ( rm -f "${OUTDIR}/${DISTNAME}.tar.bz2" && exit 1 )
                ;;
        esac
    )
)  # $DISTSRC

rm -rf "$ACTUAL_OUTDIR"
mv --no-target-directory "$OUTDIR" "$ACTUAL_OUTDIR" \
    || ( rm -rf "$ACTUAL_OUTDIR" && exit 1 )

(
    cd /outdir-base
    {
        echo "$GIT_ARCHIVE"
        find "$ACTUAL_OUTDIR" -type f
    } | xargs realpath --relative-base="$PWD" \
      | xargs sha256sum \
      | sort -k2 -o "$LOGDIR"/SHA256SUMS.part
)
