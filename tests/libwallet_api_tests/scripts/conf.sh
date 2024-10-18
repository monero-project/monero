#!/bin/bash -e

# path where created wallets will reside
# By default: /var/monero/testnet_pvt
export WALLETS_ROOT_DIR="/var/monero/testnet_pvt"
# If the wallets are created in a different directory, please uncomment and adjust the following export:
#export WALLETS_ROOT_DIR=.

# path to monero-wallet-cli
WALLET_CLI_DIR=.
WALLET_CLI="$WALLET_CLI_DIR/monero-wallet-cli"

# Testnet daemon defaults:
DAEMON_PORT=38081
DAEMON_HOST=localhost

# If the daemon defaults are to be changed, please uncomment the following export:
#export TESTNET_DAEMON_ADDRESS=$DAEMON_HOST:$DAEMON_PORT


# Mainnet daemon defaults:
#DAEMON_PORT_MAINNET=18081
#DAEMON_HOST_MAINNET=$DAEMON_HOST
#export MAINNET_DAEMON_ADDRESS=$DAEMON_HOST_MAINNET:$DAEMON_PORT_MAINNET

