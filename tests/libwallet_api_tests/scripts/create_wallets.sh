#!/bin/bash

function create_wallet {
    wallet_name=$1
    echo 0 | monero-wallet-cli  --testnet --trusted-daemon --daemon-address localhost:38081 --generate-new-wallet $wallet_name --password "" --restore-height=1
}


create_wallet wallet_01.bin
create_wallet wallet_02.bin
create_wallet wallet_03.bin
create_wallet wallet_04.bin
create_wallet wallet_05.bin
create_wallet wallet_06.bin

# create_wallet wallet_m


