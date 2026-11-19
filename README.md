# Numista Coins

Numista Coins is an experimental blockchain project intended to provide the
blockchain layer for a UK-based plot-purchase game. This repository is a
Monero-derived codebase being adapted for Numista; it is not the Monero network
and is not ready for production use.

## Project vision

The game is planned around purchasing plots and encountering game events, such
as discovering a building. Some buildings may allow the game to issue specific
in-game assets, including NFT-like stocks. The long-term goal is to represent
and settle supported game assets on the Numista Coins blockchain, alongside
game-backend services.

The first protocol prototype adds a versioned asset-operation payload to
transaction extra, with operations for issuer-authorized username registration,
asset creation and minting, plus owner-key-authorized transfers. It supports
collectible and fungible supply rules and signs a canonical, domain-separated
encoding. The public payload uses game usernames and dedicated asset keys,
not Monero wallet addresses.

Consensus validation is wired for a future hard-fork activation at block
version 17. A separate Numista testnet profile now uses an isolated network ID,
address prefixes, ports, data directory, genesis nonce, and hard-fork schedule;
version 17 activates at block height 1 on that network only. The profile uses
the existing testnet genesis transaction with a distinct nonce. The Numista
testnet has a dedicated issuer public key configured for test operations; its
matching private scalar is stored outside the repository in the local session
files directory. It is for testnet only and must never be reused on production.
Asset state is reconstructed from canonical blocks at startup and
updated/reverted as blocks are added, popped, and reorganized; it is not stored
in a separate database.
There is not yet a Numista-specific asset RPC endpoint or wallet command. Do
not use this prototype as proof of ownership or submit asset operations to a
production network.

Run the daemon and wallet with `--numista-testnet` to select this isolated
network. The daemon and wallet CLI now default to Numista testnet when no
network flag is supplied; Monero mainnet requires the explicit `--mainnet`
flag:

```sh
monerod
monero-wallet-cli --daemon-address 127.0.0.1:48081
```

The `--numista-testnet` flag may also be used explicitly. Do not combine
network flags. Existing Monero testnet and stagenet remain explicitly
selectable with `--testnet` and `--stagenet`; regtest remains selectable with
`--regtest`. Numista testnet has no Monero DNS or fallback seed peers configured;
it will not automatically connect to Monero peers. Connect testnet nodes
explicitly with `--add-peer` until Numista seed infrastructure is set up. The
Numista testnet issuer key is fixed by consensus in
`config::numista_testnet::NUMISTA_ASSET_ISSUER_PUBLIC_KEY_HEX`. Keep the
corresponding private scalar outside the source tree; never reuse it on
production.

## Future production network parameters

An isolated production parameter profile is reserved in
`src/cryptonote_config_numista_production.h`. It is deliberately marked
unconfigured and is not selectable by the daemon or wallet. Production address
prefixes, ports, network ID, genesis data, asset activation height, and issuer
key are unset; do not copy Monero or testnet values into this profile as final
production settings.

Only complete and review this profile after the Numista testnet is operational
and has passed its consensus, upgrade, restart, and reorganization testing.
Until then, the existing Monero network configurations and activation schedules
remain unchanged.

## Project status

This project is under active development. Do not use it to store value or run
production game services. A distinct network, coin parameters, game-asset
consensus enforcement, and integration with the game backend still need to be
designed and implemented.

## Roadmap

1. Define the asset payload and issuer/owner authorization rules. The initial
   payload and signature rules are now in the codebase.
2. Integrate asset validation with transaction-pool and block consensus, and
   make asset state consistent across startup, block application, and chain
   reorganizations. The code path is implemented; Codespaces compilation and
   end-to-end consensus tests remain pending.
3. Configure and validate the isolated Numista testnet. Its network profile,
   version-17 activation, and test issuer key are in place; run consensus,
   restart, and reorganization tests before calling it operational.
   Production parameters must not be inferred from Monero or testnet.
4. Add node and wallet RPC methods and CLI commands for asset registration,
   creation, minting, transfer, and querying.
5. Connect the game backend and test the full protocol on the devnet before
   considering a production launch.

## Build from source

The project uses CMake and the top-level Makefile. Dependencies and platform
specific build notes are documented in the
[Monero build guide](https://github.com/monero-project/monero#compiling-monero-from-source);
Numista-specific build requirements may change as the fork develops.

Clone this repository with its required submodules:

```sh
git clone --recursive https://github.com/danielmw3210-sketch/numstacoin.git
cd numstacoin
make
```

The build output is placed in `build/release/bin`. To build and run the test
suite:

```sh
make release-test
```

## Code and licensing

Numista Coins is based on the Monero codebase and includes code originally
copyrighted by the Monero Project and the CryptoNote developers. See
[LICENSE](LICENSE) and the copyright notices in individual files for the
applicable terms and attributions.
