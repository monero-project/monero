# URI Formatting

URIs are useful for easy payment solutions, especially for the use of merchandising purposes by generating QR codes from the specification-regulated protocol scheme described below.

Its syntax follows RFC 3986. Text is percent-encoded; spaces are `%20`, and a literal `+` is not a space. Text is decoded exactly once.

## TX Scheme
```
monero: [address <string>]
```

### Existing single-recipient format

The existing `make_uri` and `parse_uri` RPC calls retain their scalar fields and behavior. Their RPC `amount` is an unsigned integer in atomic XMR units, while the URI's `tx_amount` is a decimal XMR value. For example, `11000000000` atomic units become `0.011000000000` in the URI.

The following query parameters can be optionally appended to the resulting URI:

| Parameter        | Type   | Kind | Description |
|:-----------------|:------:|:----:|:------------|
| `address`        | String | hierarch.| One Monero address after `monero:`. |
| `recipient_name` | String | query | The proposed contact name of the recipient. |
| `tx_amount`      | String | query | The proposed amount in decimal XMR units. |
| `tx_description` | String | query | A description of the payment request. |


The previously documented semicolon-separated lists are not supported by the scalar RPC calls. Use the experimental multi-recipient interface below for more than one recipient.

Standalone `tx_payment_id` is deprecated. The new interface rejects it; the existing scalar parser's behavior is unchanged. Integrated addresses remain addresses and do not require a separate payment-ID parameter.


Thus the resulting URI may look something like this:
```
monero:46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em?tx_amount=239.39014&tx_description=donation
```

### Experimental multi-recipient format

This interface implements the [repeated-address proposal](https://github.com/monero-project/monero/issues/7731#issuecomment-3239824513). `version=2.0` must be the first query parameter. The address after `monero:` is the first recipient. Each subsequent `address=` starts another recipient; following `amount=` and `label=` parameters belong to that recipient. The order of recipients is significant.

For readability, `A` and `B` below stand for valid Monero addresses on the selected network:

```text
monero:A?version=2.0&amount=0.011XMR&label=Buyer&address=B&label=Seller&tx_description=Order%2042
```

This requests `0.011 XMR` for A and leaves B's amount unspecified. Omission differs from `amount=0XMR`. `tx_description` is global and may appear at most once. The two per-recipient attributes may each appear at most once, in either order. `tx_amount` and `recipient_name` are accepted as aliases in version 2, but an alias and its canonical name cannot both occur for one recipient.

Amounts contain ASCII decimal digits, at most one decimal point, and an optional uppercase unit: `XMR`, `BTC`, `ETH`, `USD` or `EUR`. An omitted unit means XMR. At least one digit is required; `.5`, `1.`, and `001.500` are accepted and their spelling is retained. Signs, exponents, commas and unsupported units are rejected. The generator emits an explicit unit for every amount. These are exact decimal strings, without floating-point conversion, scale assumptions, exchange-rate lookup or automatic conversion to atomic XMR. Applications must separately interpret and confirm a requested amount before using it for a transaction.

The new parser also accepts ordinary unversioned single-recipient `tx_amount`, `recipient_name`, and `tx_description` inputs. New `address`, `amount` and `label` keys require version 2. Unsupported versions, misplaced versions, duplicate attributes within a recipient, and malformed percent escapes are rejected. Unknown query parameters are returned as raw `name=value` strings. Their names must be URI-unreserved ASCII characters, and duplicate unknown names are rejected. The existing scalar parser's more permissive handling of unknown fields is unchanged.

Addresses are validated for checksum and network by the public C++ API and RPC handlers. They must be actual addresses; these functions do not resolve DNS or OpenAlias. Integrated addresses are preserved independently for each recipient. URI parsing does not imply that arbitrary combinations of integrated addresses can be used in one transaction.

**Compatibility:** `version=2.0` does not make old binaries reject the URI. Existing parsers may ignore new query parameters and interpret only the first address. Only send multi-recipient links to clients known to support this interface. Use the original `make_uri` for links intended for old single-recipient clients.

### Experimental RPC interface

The additive methods are `make_uri_multi` and `parse_uri_multi`. Neither needs an opened wallet. Both accept `network_type`, one of `mainnet` (default), `testnet` or `stagenet`; validation uses this explicit value, independently of any opened wallet.

`make_uri_multi` parameters:

```json
{
  "network_type": "mainnet",
  "recipients": [
    {"address": "A", "amount": "0.011", "currency": "XMR", "label": "Buyer"},
    {"address": "B"}
  ],
  "tx_description": "Order 42"
}
```

`A` and `B` are placeholders and must be replaced with valid addresses. `recipients` must be nonempty. Each entry requires `address`; `amount` defaults to the empty string (unspecified), `currency` to `XMR`, and `label` to the empty string. An unspecified amount cannot carry a nondefault currency, because the URI has no separate currency parameter. `tx_description` is optional. The result is `{"uri": "monero:..."}`.

`parse_uri_multi` accepts `{"uri": "monero:...", "network_type": "mainnet"}` and returns:

```json
{
  "uri": {
    "recipients": [
      {"address": "A", "amount": "0.011", "currency": "XMR", "label": "Buyer"},
      {"address": "B", "amount": "", "currency": "XMR", "label": ""}
    ],
    "tx_description": "Order 42"
  },
  "unknown_parameters": []
}
```

The new `amount` string is in decimal units. It is intentionally separate from the existing scalar RPC `amount` integer in atomic units; do not pass it directly to an existing transfer method.

## Wallet Definition Scheme
The following scheme is proposed as a means of describing wallets. This scheme may be used for restoring.

```
monero-wallet: [address <string>]
```

Only one of `seed` and `(spend_key,view_key)` may/must be specified.

| Parameter      | Type       | Requires | Description |
|:---------------|:------:    |:-------- |:------------|
| `address`      | String     | `view_key` |Raw (95-character) address                          |
| `spend_key`    | Hex String | `address`, `view_key` | Private spend key of a wallet.                      |
| `view_key`     | Hex String | `address`, absence of `seed` | Private view key of a wallet.                       |
| `seed`         | String     | Absence of `view_key` | URL encoded mnemonic seed to restore a deterministic wallet. |
| `height`       | Long       | Absence of `txid` | Block height when the wallet was created.           |
| `txid`         | String     | Absence of `height` | Transaction ID(s) to scan. Multiple transaction IDs can be provided if separated by a semicolon (`;`). |

>*Note:* The `seed` parameter was named `mnemonic_seed` in earlier versions of this specification.

The resulting URI for a wallet may look like this:
```
monero-wallet:467iotZU5tvG26k2xdZWkJ7gwATFVhfbuV3yDoWx5jHoPwxEi4f5BuJQwkP6GpCb1sZvUVB7nbSkgEuW8NKrh9KKRRga5qz?spend_key=029c559cd7669f14e91fd835144916009f8697ab5ac5c7f7c06e1ff869c17b0b&view_key=afaf646edbff3d3bcee8efd3383ffe5d20c947040f74e1110b70ca0fbb0ef90d
```
or
```
monero-wallet:467iotZU5tvG26k2xdZWkJ7gwATFVhfbuV3yDoWx5jHoPwxEi4f5BuJQwkP6GpCb1sZvUVB7nbSkgEuW8NKrh9KKRRga5qz?seed=python%20runway%20gossip%20lymph%20hills%20karate%20ruined%20innocent%20ought%20dual%20shipped%20shipped%20sushi%20pyramid%20guys%20entrance%20obedient%20natural%20kiwi%20wobbly%20vixen%20wipeout%20template%20typist%20innocent&height=12345676
```
