#!/bin/bash -e

. ./conf.sh

rlwrap "$WALLET_CLI" --wallet-file "$WALLETS_ROOT_DIR/wallet_m.bin" --password "" --testnet --trusted-daemon --daemon-address $DAEMON_HOST:$DAEMON_PORT --log-file "$WALLETS_ROOT_DIR/wallet_m.log" start_mining

