#!/bin/bash



function send_funds {
    local amount=$1
    local dest=$(cat "$2.address.txt")

    simplewallet --wallet-file wallet_m --password "" \
        --testnet --trusted-daemon --daemon-address localhost:38081  --log-file wallet_m.log \
        --command transfer $dest $amount 
}


send_funds 100 wallet_01.bin
send_funds 100 wallet_02.bin
send_funds 100 wallet_03.bin
send_funds 100 wallet_04.bin
send_funds 100 wallet_05.bin


