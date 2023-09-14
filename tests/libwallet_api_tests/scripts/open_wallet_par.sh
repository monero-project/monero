#!/bin/bash -e

. ./conf.sh

WALLET_NAME=$1

if [ -z $WALLET_NAME ]; then
    echo "Please provide the wallet name as the 1st parameter"
    exit 1
fi
echo "Opening wallet: $WALLET_NAME"


rlwrap "$WALLET_CLI" --wallet-file "$WALLETS_ROOT_DIR/$WALLET_NAME.bin" --password "" --testnet --trusted-daemon --daemon-address $DAEMON_HOST:$DAEMON_PORT  --log-file "$WALLETS_ROOT_DIR/$WALLET_NAME.log"

