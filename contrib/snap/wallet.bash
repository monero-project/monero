#!/bin/bash -e

export LD_LIBRARY_PATH=${SNAP_LIBRARY_PATH}:${SNAP}/usr/lib/x86_64-linux-gnu
export HOME=${SNAP_USER_DATA}
cd ${SNAP_USER_DATA}

exec ${SNAP}/usr/bin/rlwrap ${SNAP}/bin/monero-wallet-cli "$@"
