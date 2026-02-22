# Proxy usage in the Monero ecosystem
The CLI/RPC wallets and daemon both support proxies and use the same parameters
to configure them. Currently socks 4, 4a, and 5 are supported and can be
selected with command-line options.

## Wallet
The CLI and RPC wallets support proxies via the `--proxy` option. The format
for usage is `[socks5://[user:pass]]host:port`. The square brackets indicate
an optional portion. This option can only be specified once. Examples:

```
--proxy 192.168.0.10:1050
--proxy socks5://192.168.0.10:1050
--proxy socks5://username:password@192.168.0.10:1050
--proxy [::1]:1050
--proxy socks5://[::1]:1050
--proxy socks5://username:password@[::1]:1050
```

The first connects to `192.168.0.10` on port `1050` using socks 4a. The second
connects to the same location using socks 5. The third uses socks 5 at the same
location and sends user authentication if prompted by the proxy server. The
last three are identical to the first 3, except an IPv6 address is used
instead. While IPv6 connections are invalid for Socks 4 and 4a, the proxy
server itself can be connected using IPv6.

The username and password fields both support "percent-encoding" for special
character support. As an example, `%40` gets converted to `@`, such that
`username:p%40ssword` gets converted to `username:p@ssword`. This allows that
specific character to be used; specifying the character directly will
incorrectly change the specification of the hostname.

> NOTE: The username+password will show up in the process list and can be read
> by other programs. It is recommended that `--config-file` be used to store
> username+password options. The format for a config file is `option=value`,
> so in this example the file would contain:
> `proxy=socks5://username:password@192.168.0.10:1080`.

The CLI and RPC wallets currently reject hosts that do **NOT** end in`.onion`
or `.i2p` **unless** `--daemon-ssl-ca-certificates`,
`--daemon-ssl-allow-any-cert`, or `--daemon-ssl-allowed-fingerprints` is used.
If an onion or i2p address is used, the hostname contains the certificate
verification, providing decent security against man-in-the-middle (MitM)
attacks. The two `--daemon-ssl-*` options support specifying exact
certificates, also preventing MitM attacks.

> Perhaps the wallets should be relaxed to allow system-CA checks, but for now
> certificates must be strictly provided.

## Daemon
The daemon has two options for proxies `--proxy` and `--tx-proxy` which can be
used in isolation or together. The `--proxy` option controls how
IPv4/IPv6/hostname connections are performed, whereas `--tx-proxy` controls
how local transactions are relayed. Both options support Socks 4, 4a, and 5.

### `--proxy`
This option should be used when outbound connections to IPv4/IPv6 addresses and
hostnames  (other than `.onion` `.i2p`) need to be proxied. Common examples
include using Tor exit nodes or a VPN to conceal your local IP. This option
will **not** use Tor or I2P hidden services for P2P connections; this is
primarily used for proxying standard IPv4 or IPv6 connections to some remote
host. Hidden services are not used because this is designed to be more general
purpose (i.e. a standard socks VPN can be used). 

> An additional option for hidden services (separate from `--tx-proxy`) could
> arguably be added, which could optionally turn off IPv4/IPv6 connections for
> P2P.

The format for `--proxy` usage: `[socks5://[user:pass]]@127.0.0.1`. The square
bracket indicate optional portion. See [wallet](#wallet) section above for
examples and other information on the format. The option can only be specified
once. The restrictions for MitM attacks apply only to the wallet usage, and not
to the daemon.

> When using `--proxy`, inbound connections will be impossible unless the
> proxy server is somehow setup to forward connections. This setup is a
> difficult because each outgoing socks connections can have a unique binding
> port. Such a setup is currently out-of-scope for this document.

### `--tx-proxy`
This option should be used to specify a proxy that can resolve hidden service
hostnames, so that local transactions can be forwarded over a privacy
preserving network. Currently only Tor or I2P hidden services are supported.
This option be specified multiple times, but only once per network (see below).

The format for `--tx-proxy` is
`network,[socks5://[user:pass@]]ip:port[,max_connections][,disable_noise]`.
Examples:

```
--tx-proxy tor,127.0.0.1:1050
--tx-proxy tor,127.0.0.1:1050,100
--tx-proxy tor,127.0.0.1:1050,disable_noise
--tx-proxy tor,127.0.0.1:1050,100,disable_noise
--tx-proxy tor,socks5://127.0.0.1:1050
--tx-proxy tor,socks5://127.0.0.1:1050,100
--tx-proxy tor,socks5://127.0.0.1:1050,disable_noise
--tx-proxy tor,socks5://127.0.0.1:1050,100,disable_noise
--tx-proxy tor,socks5://username:password@127.0.0.1:1050
--tx-proxy tor,socks5://username:password@127.0.0.1:1050,100
--tx-proxy tor,socks5://username:password@127.0.0.1:1050,disable_noise
--tx-proxy tor,socks5://username:password@127.0.0.1:1050,100,disable_noise
--tx-proxy tor,[::1]:1050
--tx-proxy tor,[::1]:1050,100
--tx-proxy tor,[::1]:1050,disable_noise
--tx-proxy tor,[::1]:1050,100,disable_noise
--tx-proxy tor,socks5://[::1]:1050
--tx-proxy tor,socks5://[::1]:1050,100
--tx-proxy tor,socks5://[::1]:1050,disable_noise
--tx-proxy tor,socks5://[::1]:1050,100,disable_noise
--tx-proxy tor,socks5://username:password@[::1]:1050
--tx-proxy tor,socks5://username:password@[::1]:1050,100
--tx-proxy tor,socks5://username:password@[::1]:1050,disable_noise
--tx-proxy tor,socks5://username:password@[::1]:1050,100,disable_noise
--tx-proxy i2p,127.0.0.1:1050
--tx-proxy i2p,127.0.0.1:1050,100
--tx-proxy i2p,127.0.0.1:1050,disable_noise
--tx-proxy i2p,127.0.0.1:1050,100,disable_noise
--tx-proxy i2p,socks5://127.0.0.1:1050
--tx-proxy i2p,socks5://127.0.0.1:1050,100
--tx-proxy i2p,socks5://127.0.0.1:1050,disable_noise
--tx-proxy i2p,socks5://127.0.0.1:1050,100,disable_noise
--tx-proxy i2p,socks5://username:password@127.0.0.1:1050
--tx-proxy i2p,socks5://username:password@127.0.0.1:1050,100
--tx-proxy i2p,socks5://username:password@127.0.0.1:1050,disable_noise
--tx-proxy i2p,socks5://username:password@127.0.0.1:1050,100,disable_noise
--tx-proxy i2p,[::1]:1050
--tx-proxy i2p,[::1]:1050,100
--tx-proxy i2p,[::1]:1050,disable_noise
--tx-proxy i2p,[::1]:1050,100,disable_noise
--tx-proxy i2p,socks5://[::1]:1050
--tx-proxy i2p,socks5://[::1]:1050,100
--tx-proxy i2p,socks5://[::1]:1050,disable_noise
--tx-proxy i2p,socks5://[::1]:1050,100,disable_noise
--tx-proxy i2p,socks5://username:password@[::1]:1050
--tx-proxy i2p,socks5://username:password@[::1]:1050,100
--tx-proxy i2p,socks5://username:password@[::1]:1050,disable_noise
--tx-proxy i2p,socks5://username:password@[::1]:1050,100,disable_noise
```

The above examples are fairly exhaustive of all the possible option scenarios
that will be incurred by the typical user. 

#### The `network` portion of the option
The first section (before the first `,`) indicates the network - only `tor` or
`i2p` are valid here.

This portion of the option tells `--add-node`, `--add-priority-node`, and
`--add-exclusive-node` options to use the specified proxy for those nodes. In
other words, command-line specified hidden services are forwarded to their
corresponding `--tx-proxy` server. Hidden services do **NOT** have to be
specified on the command-line, there are built-in seed nodes for each network.

#### The `ip:port` portion of the option
The second portion of the option (after the first `,` and _optionally_ ending
in the next `,`) indicates the location of the socks server. The location
**must** include an IPv4/IPv6 AND port. The location can optionally include the
socks version  - `socks4`, `socks4a`, and `socks5` are all valid here. If
the socks version is not specified, `socks4a` is assumed.

An optional username and password can also be included. These fields support
percent-encoding, see [wallet](#wallet) section for more information.

#### The last portion of the option
After the ip:port section two options can be specified: the number of max
connections and `disable_noise`. They can be specified in either order, but
must be after the ip:port section.

The max connections does exactly as advertised, it limits the number of
outgoing connections to the proxy. The `disable_noise` feature lowers the
bandwidth requirements, and decreases the tx-relay time. When **NOT**
specified, dummy P2P packets are sent periodically to connections (via the
proxy) to conceal when a transaction is forwarded over the connection. When
the option is specified, P2P links only send data for peerlist information and
local outgoing transactions.

### `--anonymous-inbound`
Currently the daemon cannot configure incoming hidden services connections.
Instead, the user must manually configure Tor or I2P to accept inbound
connections. Then, `--anonymous-inbound` must be used to tell the daemon where
to listen for incoming connections, and the incoming hidden service address.
The option can be specified once for each network type. The format for usage
is: `hidden-service-address,[bind-ip:]port[,max_connections]`. Examples:

```
--anonymous-inbound rveahdfho7wo4b2m.onion:18083,18083
--anonymous-inbound rveahdfho7wo4b2m.onion:18083,18083,100
--anonymous-inbound rveahdfho7wo4b2m.onion:18083,127.0.0.1:18083
--anonymous-inbound rveahdfho7wo4b2m.onion:18083,127.0.0.1:18083,100
--anonymous-inbound udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p,18083
--anonymous-inbound udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p,18083,100
--anonymous-inbound udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p,127.0.0.1:18083
--anonymous-inbound udhdrtrcetjm5sxzskjyr5ztpeszydbh4dpl3pl4utgqqw2v4jna.b32.i2p,127.0.0.1:18083,100
```

Everything before the first `,` is the hidden service hostname. This must be
a valid Tor or I2P address. This tells the daemon the **inbound** hidden
service as configured for the local Tor or I2P daemons.

Everything between `,`s specify the bind ip and bind port. The IP address is
optional, and defaults to `127.0.0.1`. The Tor and I2P daemons must be
configured to forward incoming hidden service connections to this IP/Port pair.

Everything after the second `,` is used to specify the number of max inbound
connections. The field is optional.
