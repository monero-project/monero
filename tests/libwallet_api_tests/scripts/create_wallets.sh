#!/bin/bash -e

. ./conf.sh

function create_wallet {
    wallet_name=$1
    echo 0 | "$WALLET_CLI" --testnet --trusted-daemon --daemon-address $DAEMON_HOST:$DAEMON_PORT --generate-new-wallet "${WALLETS_ROOT_DIR}/${wallet_name}.bin" --restore-height=1 --password ""
}

function create_wallet_if_not_exists {
    wallet_name=$1
    if [ ! -f "$WALLETS_ROOT_DIR/$wallet_name" ]; then
        create_wallet $wallet_name
    fi
}


create_wallet wallet_01
create_wallet wallet_02
create_wallet wallet_03
create_wallet wallet_04
create_wallet wallet_05
create_wallet wallet_06

create_wallet_if_not_exists wallet_m
# In case you want to recreate it anyway, just uncomment the next line:
#create_wallet wallet_m
