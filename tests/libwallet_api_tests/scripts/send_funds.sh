#!/bin/bash -e

. ./conf.sh

function send_funds {
    local amount=$1
    local dest=$(cat "$WALLETS_ROOT_DIR/$2.address.txt")

    "$WALLET_CLI" --wallet-file "$WALLETS_ROOT_DIR/wallet_m.bin" --password "" \
        --testnet --trusted-daemon --daemon-address $DAEMON_HOST:$DAEMON_PORT --log-file "$WALLETS_ROOT_DIR/wallet_m.log" \
        --command transfer $dest $amount
}


function seed_wallets {
    local amount=$1
    send_funds $amount wallet_01.bin
    send_funds $amount wallet_02.bin
    send_funds $amount wallet_03.bin
    send_funds $amount wallet_04.bin
    send_funds $amount wallet_05.bin
    send_funds $amount wallet_06.bin
}

seed_wallets 1
seed_wallets 2
seed_wallets 5
seed_wallets 10
seed_wallets 20
seed_wallets 50
seed_wallets 100
