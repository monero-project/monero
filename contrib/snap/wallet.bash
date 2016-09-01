#!/bin/bash -e

export HOME=${SNAP_USER_DATA}
cd ${SNAP_USER_DATA}

exec ${SNAP}/usr/bin/rlwrap ${SNAP}/bin/monero-wallet-cli "$@"
