# Battleground

Battleground is an ASIC-resistant, CPU-mineable cryptocurrency designed to
serve as in-game currency. It is a fork of [Monero](https://github.com/monero-project/monero)
and inherits Monero's privacy properties (ring signatures, stealth
addresses, RingCT, Bulletproofs+) and — critically for this project —
[RandomX](https://github.com/tevador/RandomX) as its proof-of-work algorithm.

## Why RandomX (ASIC resistance + CPU preference)

RandomX is a proof-of-work function designed to be efficient on general
purpose CPUs. Its design goals directly match the requirements for this
coin:

- **ASIC resistance**: RandomX uses random code execution, a large
  2 GiB scratchpad, and floating-point/integer operations that exploit
  features present in modern CPUs (out-of-order execution, branch
  prediction, caches). Specialized hardware loses most of its advantage.
- **Strong on CPU**: ordinary CPUs — the same ones players already have
  in their gaming rigs — are the first-class mining target.
- **Degraded on GPU**: GPUs lack the general-purpose capabilities
  RandomX leans on, so GPU miners run at a significant disadvantage
  versus CPUs of comparable cost. Public benchmarks have consistently
  shown GPUs mining RandomX at a small fraction of comparably priced
  CPU hardware.

This is the algorithm Monero adopted in 2019 for exactly the same
anti-centralization reasons, and it is why this project forks the
Monero codebase instead of reimplementing the mining pipeline from
scratch.

## Coin parameters

| Parameter                       | Value                                     |
|---------------------------------|-------------------------------------------|
| Coin name                       | Battleground                              |
| Ticker (display unit)           | `battleground` (atomic unit: `picoground`)|
| Proof of work                   | RandomX (CPU-optimized)                   |
| Target block time               | 120 seconds                               |
| Smallest unit                   | 1e-12 battleground                        |
| Emission                        | Monero-style tail emission                |
| Address prefix (mainnet)        | 66                                        |
| Integrated address prefix       | 67                                        |
| Subaddress prefix               | 68                                        |
| Default P2P port (mainnet)      | 19090                                     |
| Default RPC port (mainnet)      | 19091                                     |
| Default ZMQ port (mainnet)      | 19092                                     |
| Default P2P port (testnet)      | 29090                                     |
| Default P2P port (stagenet)     | 39090                                     |

Mainnet addresses use base58 prefix 66, so they do not collide with
Monero (prefix 18), testnet Monero (prefix 53) or stagenet Monero
(prefix 24). A wallet that tries to send Monero to a Battleground
address will reject it at parse time, and vice versa.

## Building

Battleground uses the same build system as upstream Monero. The
dependencies, build steps, and platform notes below all carry over.

```
make release-static
```

Build artifacts appear under `build/<platform>/_/release/bin`. The
binaries currently retain their upstream names (`monerod`,
`monero-wallet-cli`, `monero-wallet-rpc`); they speak the Battleground
protocol because every coin-identity value (network ID, ports, address
prefixes, genesis nonce, message-signing prefix) has been changed.
Binary renaming is tracked as a follow-up.

See the `docs/` directory for detailed build instructions; they are
unchanged from upstream Monero.

## Running a node

```
./monerod --p2p-bind-port 19090 --rpc-bind-port 19091
```

Battleground is a brand-new network and ships with no canonical seed
nodes. To bootstrap the network you (or another operator) must connect
nodes to each other explicitly:

```
./monerod --seed-node <first-node-ip>:19090
./monerod --add-peer <peer-ip>:19090
```

## Mining

RandomX mining is built into the daemon. To mine to an address:

```
./monerod --start-mining <your-battleground-address> --mining-threads <N>
```

Recommended: set `N` to the number of physical CPU cores, leaving one
core free for gameplay if mining on a player rig.

## In-game currency usage

Battleground is intended as in-game currency. A game client typically
integrates by running (or connecting to) a `monero-wallet-rpc` instance
on the default RPC port and issuing `transfer`, `get_balance`, and
`get_transfers` calls over JSON-RPC. The wallet-RPC interface is fully
compatible with the upstream Monero RPC, so existing client libraries
work unchanged once pointed at the Battleground node.

## License

See [LICENSE](LICENSE). Battleground is distributed under the same
BSD-3-clause-style license as upstream Monero.

## Upstream credit

Battleground is a direct fork of Monero. The cryptography, consensus
rules, and RandomX implementation are the Monero Project's work, and
this project inherits their copyright notices throughout the source
tree. This fork is not affiliated with or endorsed by the Monero
Project.
