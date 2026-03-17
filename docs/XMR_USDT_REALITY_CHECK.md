# XMR to USDT Reality Check

This utility turns a quick operational rule into an explicit probe:

- confirm which Monero network each RPC endpoint is on
- refuse to treat testnet or stagenet balances as economically convertible
- point to the valid next branch for either testing or real funds

## Utility location

The checker lives at `utils/python-rpc/monero_swap_reality_check.py`.

## Example

```bash
python3 utils/python-rpc/monero_swap_reality_check.py \
  --source-rpc dev2.monerodevs.org:28089 \
  --target-rpc dev2.monerodevs.org:18089 \
  --amount-atomic 3532212 \
  --target-asset USDT \
  --target-chain TRC20 \
  --format markdown
```

## Expected outcome for the original request

If `dev2.monerodevs.org:28089` reports `testnet` and `dev2.monerodevs.org:18089` reports `mainnet`, the request is not possible as stated:

- the `28089` balance is not spendable mainnet XMR
- the `18089` node is infrastructure, not a bridge into economic value
- the valid dev branch is an all-stagenet proof
- the valid real branch starts with mainnet XMR and a venue that explicitly supports XMR deposits plus the intended USDT withdrawal chain

## Modes

- `--mode auto`: infer the recommendation from the detected source network
- `--mode dev`: force the output toward a stagenet proof workflow
- `--mode real`: force the output toward the mainnet real-funds workflow

## Optional inputs

- `--source-address`: decode the address prefix byte and cross-check the wallet network
- `--rpc-user` / `--rpc-password`: use digest auth if the endpoint requires it
- `--insecure`: skip TLS verification for a development endpoint with a self-signed certificate
- `--report-file`: save the rendered output to a file

## Valid branches

### Dev proof

Use stagenet end to end:

1. fund a stagenet wallet
2. connect to a stagenet daemon or restricted RPC endpoint
3. run the test trade on Haveno stagenet
4. choose `USDT-ERC20` or `USDT-TRC20` before trading

### Real swap

Use mainnet end to end:

1. source real XMR on mainnet
2. connect to a mainnet daemon or trusted remote node
3. choose a venue that supports both XMR deposits and the exact USDT withdrawal chain you need

## References

- [Monero network docs](https://docs.getmonero.org/infrastructure/networks/)
- [Monero standard address docs](https://docs.getmonero.org/public-address/standard-address/)
- [Haveno stagenet install guide](https://docs.haveno.exchange/development/installing/)
- [Haveno supported assets](https://docs.haveno.exchange/the-project/assets/)
