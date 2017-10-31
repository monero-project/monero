#!/bin/bash

rlwrap electroneum-wallet-cli --wallet-file wallet_m --password "" --testnet --trusted-daemon --daemon-address localhost:38081  --log-file wallet_miner.log stop_mining

