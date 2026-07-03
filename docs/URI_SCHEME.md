# URI Formatting

URIs are useful for easy payment solutions, especially for the use of merchandising purposes by generating QR codes from the specification-regulated protocol scheme described below.

Its syntax follows RFC 3986. See [URI scheme](https://en.wikipedia.org/wiki/URI_scheme) for explanation. Spaces must be [x-www-urlencoded](https://en.wikipedia.org/wiki/Application/x-www-form-urlencoded) as `%20`.

## TX Scheme
```
monero: [address <string>]
```

The following parameters can be optionally appended to the resulting URI:

| Parameter        | Type   | Kind | Description |
|:-----------------|:------:|:----:|:------------|
| `address`        | String | hierarch.| The raw address (or the openalias?). Multiple addresses can be provided if separated by a semicolon (`;`).|
| `recipient_name` | String | ? | The proposed contact name of the recipient. Multiple names can be provided if separated by a semicolon (`;`).|
| `tx_amount`      | String | query | The proposed amount of the transaction in decimal units. Multiple amounts can be provided if separated by a semicolon (`;`).|
| `tx_description` | String | fragment | Describes the transaction which should be initiated. |


>*Note:* If multiple `address` parameters are supplied, there must be a matching number of `tx_amount` parameters.
>*Note:* The `tx_payment_id` parameter was supported in earlier versions of this specification but is now deprecated and no longer allowed for manual inclusion.


Thus the resulting URI may look something like this:
```
monero:46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em?tx_amount=239.39014&tx_description=donation
```
or
```
monero:46BeWrHpwXmHDpDEUmZBWZfoQpdc6HaERCNmx1pEYL2rAcuwufPN9rXHHtyUA4QVy66qeFQkn6sfK8aHYjA3jk3o1Bv16em;888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H?tx_amount=239.39014;132.44&tx_description=donations
```

## Wallet Definition Scheme
The following scheme is proposed as a means of describing wallets. This scheme may be used for restoring.

```
monero-wallet: [address <string>]
```

Only one of `seed` and `(spend_key,view_key)` may/must be specified.

| Parameter      | Type       | Kind |Requires | Description |
|:---------------|:------:    |:-----|:-------- |:------------|
| `address`      | String     | hierarchy | `view_key` (for view only and non deterministic wallets) |Raw (95-character) address. Only required for view only wallet and non deterministic wallets.                         |
| `spend_key`    | Hex String | query string | `address`, `view_key` (for non deterministic wallets), absence of `seed` | Private spend key of a wallet. Omitted for view only wallets.                     |
| `view_key`     | Hex String | query string | `address` (for view only and non deterministic wallets), absence of `seed` | Private view key of a wallet. Only required for view only and non deterministic wallets.                      |
| `seed`         | String     | query string | Absence of `view_key` and `spend_key` | URL encoded mnemonic seed to restore a deterministic wallet. |
| `height`       | Long       | query string | Absence of `txid` | Block height or ISO 8601 timestamp at which the wallet should be restored. |
| `txid`         | String     | query string | Absence of `height` | Transaction ID(s) to scan. Multiple transaction IDs can be provided if separated by a semicolon (`;`). |
| `label` | String | query string | None | Optional name to assign to the wallet. |

>*Note:* The `seed` parameter was named `mnemonic_seed` in earlier versions of this specification.

The resulting URI for a wallet may look like this:
```
monero-wallet:467iotZU5tvG26k2xdZWkJ7gwATFVhfbuV3yDoWx5jHoPwxEi4f5BuJQwkP6GpCb1sZvUVB7nbSkgEuW8NKrh9KKRRga5qz?spend_key=029c559cd7669f14e91fd835144916009f8697ab5ac5c7f7c06e1ff869c17b0b&view_key=afaf646edbff3d3bcee8efd3383ffe5d20c947040f74e1110b70ca0fbb0ef90d&label=Test%20Wallet&height=2020-01-01
```
or
```
monero-wallet:467iotZU5tvG26k2xdZWkJ7gwATFVhfbuV3yDoWx5jHoPwxEi4f5BuJQwkP6GpCb1sZvUVB7nbSkgEuW8NKrh9KKRRga5qz?seed=python%20runway%20gossip%20lymph%20hills%20karate%20ruined%20innocent%20ought%20dual%20shipped%20shipped%20sushi%20pyramid%20guys%20entrance%20obedient%20natural%20kiwi%20wobbly%20vixen%20wipeout%20template%20typist%20innocent&height=12345676
```

View Only Wallet

```
monero-wallet:47PN2JMatPHNRXbdhhurNSKrZLXD5U5FjBkCuZ1Zyo2VLvBe8Gqj66mGTFRCMfW9pCFqG47Z9EsGWYwNV5hfDLRGRowEGWx?view_key=ac9fed8f56fe1b7e00c3b58dff826d9573231d30f1070ec177e096739b05600b&label=MS-2201-aD3490b8-18-V&height=3690371
```

![View Only Wallet](monero-wallet-V.png)

Full Wallet

```
monero-wallet:47PN2JMatPHNRXbdhhurNSKrZLXD5U5FjBkCuZ1Zyo2VLvBe8Gqj66mGTFRCMfW9pCFqG47Z9EsGWYwNV5hfDLRGRowEGWx?view_key=ac9fed8f56fe1b7e00c3b58dff826d9573231d30f1070ec177e096739b05600b&spend_key=a7a919903097e25c4c8567fd1cc8aa2be00cc9e445f353b93c75a9bca8374a03&label=MS-2201-aD3490b8-18-F&height=2026-06-01
```
![Full Wallet](monero-wallet-F.png)


