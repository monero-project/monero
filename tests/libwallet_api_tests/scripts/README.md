# Running libwallet_api tests

## Environment for the tests
* Running monero node, linked to private/public testnet. 
  By default, tests expect daemon running at ```localhost:38081```,
  can be overridden with environment variable ```TESTNET_DAEMON_ADDRESS=<your_daemon_address>```
  [Manual](https://github.com/moneroexamples/private-testnet) explaining how to run private testnet.
  It is benefitial to run the node with the `--disable-rpc-ban` option, because the test will be abusing the node.

* Running monero node, linked to mainnet.
  By default, tests expect daemon running at ```localhost:18081```,
  can be overridden with environment variable ```MAINNET_DAEMON_ADDRESS=<your_daemon_address>```

* Directory with pre-generated wallets
  (wallet_01.bin, wallet_02.bin,...,wallet_06.bin, some of these wallets might not be used in the tests currently). 
  By default, tests expect these wallets to be in ```/var/monero/testnet_pvt```. 
  Directory can be overriden with environment variable ```WALLETS_ROOT_DIR=<your_directory_with_wallets>```.
  Directory and files should be writable for the user running tests.

* The above environment variables can be conviniently modified and exported via the `conf.sh` script.

## Preparation of WALLETS_ROOT_DIR
Ideally copy all the scripts and symlink the test executable into the directory pointed by `WALLETS_ROOT_DIR` variable
and adjust your choices via the `conf.sh` script. In such scenario, uncomment the `export WALLETS_ROOT_DIR=.` line.
From there, run the below scripts:

## Generating test wallets
* ```create_wallets.sh``` - this script will create wallets (wallet_01.bin, wallet_02.bin,...,wallet_06.bin) in current directory. 
  when running first time, the script will create a special wallet_m.bin miner wallet as well.
  This wallet should be used for mining and all test wallets supposed to be seed from this miner wallet.

* ```mining_start.sh``` and ```mining_stop.sh``` - helper scripts to start and stop mining on miner wallet.

* ```send_funds.sh``` - script for seeding test wallets. Please run this script when you have enough money on miner wallet.

## Running the tests
* Before running the tests, you have to source the `conf.sh` script with: `source conf.sh` or just: `. conf.sh` within the same terminal.

* The particular tests can be executed using a Regex filter, for example: `./libwallet_api_tests --gtest_filter=WalletTest1.WalletShowsBalance`.

* Execute `./libwallet_api_tests --gtest_list_tests` to obtain the full list of available tests.
