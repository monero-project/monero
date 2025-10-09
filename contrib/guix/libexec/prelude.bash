#!/usr/bin/env bash
export LC_ALL=C
set -e -o pipefail

# shellcheck source=contrib/shell/git-utils.bash
source contrib/shell/git-utils.bash

################
# Required non-builtin commands should be invocable
################

check_tools() {
    for cmd in "$@"; do
        if ! command -v "$cmd" > /dev/null 2>&1; then
            echo "ERR: This script requires that '$cmd' is installed and available in your \$PATH"
            exit 1
        fi
    done
}

check_tools cat env git realpath

################
# We should be at the top directory of the repository
################

same_dir() {
    local resolved1 resolved2
    resolved1="$(realpath -e "${1}")"
    resolved2="$(realpath -e "${2}")"
    [ "$resolved1" = "$resolved2" ]
}

if ! same_dir "${PWD}" "$(git_root)"; then
cat << EOF
ERR: This script must be invoked from the top level of the git repository

Hint: This may look something like:
    env FOO=BAR ./contrib/guix/guix-<blah>

EOF
exit 1
fi

################
# Execute "$@" in a pinned, possibly older version of Guix, for reproducibility
# across time.
#
# For more information on guix time-machine, see:
# https://guix.gnu.org/manual/en/html_node/Invoking-guix-time_002dmachine.html
#
# Before updating the pinned hash:
#
# - Make sure a bootstrapped build works with the new commit using a fresh Guix install:
#   $ export ADDITIONAL_GUIX_COMMON_FLAGS='--no-substitutes'
#
# - Check how the update affects our build graph and which packages have been updated.
time-machine() {
    # shellcheck disable=SC2086
    guix time-machine --url=https://codeberg.org/guix/guix.git \
                      --commit=2a5297fd4ac9c3f4a7f869097f47ba0029c196ec \
                      --cores="$JOBS" \
                      --keep-failed \
                      --fallback \
                      ${SUBSTITUTE_URLS:+--substitute-urls="$SUBSTITUTE_URLS"} \
                      ${ADDITIONAL_GUIX_COMMON_FLAGS} ${ADDITIONAL_GUIX_TIMEMACHINE_FLAGS} \
                      -- "$@"
}


################
# Set common variables
################

VERSION="${FORCE_VERSION:-$(git_head_version)}"

VERSION_BASE_DIR="${VERSION_BASE_DIR:-${PWD}}"
version_base_prefix="${VERSION_BASE_DIR}/guix/guix-build-"
VERSION_BASE="${version_base_prefix}${VERSION}"  # TOP

DISTSRC_BASE="${DISTSRC_BASE:-${VERSION_BASE}}"

OUTDIR_BASE="${OUTDIR_BASE:-${VERSION_BASE}/output}"

LOGDIR_BASE="${LOGDIR_BASE:-${VERSION_BASE}/logs}"

var_base_basename="var"
VAR_BASE="${VAR_BASE:-${VERSION_BASE}/${var_base_basename}}"

profiles_base_basename="profiles"
PROFILES_BASE="${PROFILES_BASE:-${VAR_BASE}/${profiles_base_basename}}"
