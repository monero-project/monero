# Anonymity Networks with Monero


Currently Tor/I2P is available. Anonymity networks (AN) wrap traffic and proxy it over several hops to the final destination, which can include some exit back into clearnet or be a hidden service inside the AN itself. The support is experimental and while anonymity networks conceal node information there will be corner-cases that could leak information.


## Table of Contents

  - [Outbound connections](#outbound-connections)
     - [Proxy node traffic](#proxy-node-traffic)
    - [Connect to hidden services](#connect-to-hidden-service)
    - [Peer handling](#peer-handling)
  - [Inbound connections](#inbound-connections)
    - [Allow inbound RPC (Server)](#allow-inbound-rpc)
    - [Connect to remote daemon (Client)](#connect-to-remote-daemon)
  - [Configuration examples](#configuration-examples)
  - [Privacy Limitations](#privacy-limitations)
  - [Using Tor on tails (whonix)](#using-tor-on-tails)
  - [Legacy options](#legacy-options)
    - [Torsocks and monerod](#wrapping-monerod-with-torsocks)
    - [Torsocks and Tails](#torsocks-and-tails)

Three major options configure `monerod` and differ in connection, traffic type and route:

| Connection | Type   | Clearnet | Exit-Relay/I2PTunnel | .onions/.i2p        |
|------------|--------|----------|----------------------|---------------------|
| incoming   | tx     | default  |           -          | --anonymous-inbound |
|            | blocks | default  |           -          | N/A                 |
| outgoing   | tx     | default  | --proxy              | --tx-proxy          |
|            | blocks | default  | "                    | N/A                 | 

Clearnet (Ip4) is the default. Connections are hopened both ways and peers sync blocks and tx (handshakes, peer timed syncs and transactions). Setting up an AN with `--proxy`, relays blocks and peer transactions (p2p) over relays. Introducing hidden services for transactions increases anonymity. You can provide a service yourself `--anonymous-inbound` or connect to the hidden service of a peer.

If any anonymity network is enabled, transactions being broadcast that lack a valid "context" (i.e. the transaction did not come from a p2p connection), will only be sent to peers on anonymity networks. 

Block synchronisation via hidden services (.onions, .i2p) is currently not supported, while handshakes, peer timed syncs and local transactions are, see [--tx-proxy](##Conceal-local-rpc-transactions) for outgoing tx or option [--anonymous-inbound](##inbound-connections) for incoming connections to your own service. 

To connect your local node to the AN, you need a running instance for a socket to enter.
```
apt info tor # apt install
apt info i2p
```


## Outbound connections

### Proxy node traffic

The option `--proxy` allows for outgoing connections to go over a local socket as an entry point into the AN
```
./monerod --proxy 127.0.0.1:5050 #tor
./monerod --proxy 127.0.0.1:7657 #i2p
``` 
The `monerod` will use the proxy _and_ clearnet, unless you bind the p2p to localhost. You can close clearnet by binding p2p ip `0.0.0.0` to localhost.
```
# monero.conf 
proxy 127.0.0.1:5050    # tor out(going)
p2p-bind-ip 127.0.0.1   # no p2p outside ANs; default is INADDR_ANY
```

The option `--proxy` will also redirect DNS over AN, unless you set `--proxy-allow-dns-leaks` to use the default dns resolver, see [#7616 (comment)](https://github.com/monero-project/monero/pull/7616#issuecomment-892736115).


### Connect to hidden service

Hidden services are used for relaying transactions not blocks, be it from peers or local RPC. This adds the benefit of reaching a peer without revealing tx into clearnet (no exit relay).

Add option `--tx-proxy` to connect to a hidden service,
```
proxy 127.0.0.1:5050           # tor in & out
tx-proxy=tor,127.0.0.1:9050,16 # onions out; [,disable_noise]
tx-proxy=i2p,127.0.0.1:9000    # i2p
```
which tells `monerod` that `.onion` p2p addresses can be forwarded to a socks
proxy at IP 127.0.0.1 port 9050 with a max of 16 outgoing connections and
".b32.i2p" p2p addresses can be forwarded to a socks proxy at IP 127.0.0.1 port
9000 with the default max outgoing connections. The local tx is now exclusively sent over onions and withholds until an onion peer is found.

Adding `disable_noise` will tell the peer to relay the tx over tor instead of onions. It disables Dandelion++ and makes network propagation faster, but degrades anonymity, because the tx will propagate outside the AN. 

You can contribute to the network by running your own hidden service to allow inbound connections, see [Inbound connections](###-Inbound-connections).


#### Peer handling

If desired, outgoing peers can be specified manually:
```
--add-peer 136.XXX.121.74
--add-peer rveahdfho.onion:18083
--add-peer cmeua5767.b32.i2p:5000

--add-exclusive-node 136.XXX.121.74
--add-exclusive-node rveahdfho.onion:18083
--add-exclusive-node cmeua5767.b32.i2p:5000
```
The option can be repeated and recognizes Tor, I2P, and IPv4 addresses. If you add a hidden service peer, the option `--tx-proxy` is mandatory. Using `--add-exclusive-node` will prevent the usage of seed nodes on _all_ networks, which will typically be undesirable.


## Inbound connections

An anonymity network can be configured to forward incoming connections to `monerod`. Add the option
```
--anonymous-inbound rveahdfho.onion:18083,127.0.0.1:18083,25
--anonymous-inbound cmeua5767.b32.i2p:5000,127.0.0.1:30000
```
which tells `monerod` that a max of 25 inbound Tor connections are being
received at address `rveahdfho.onion:18083` and forwarded to `monerod`
localhost port `18083`.  
In this case for I2P, the default max. connections are being received at address `cmeua5767.b32.i2p:5000` and forwarded to `monerod` localhost port `30000`. The connecting hidden service address will be seeded to other peers on the AN.

Incoming connections require a hidden service. Regards Tor, add
```
# file /etc/tor/torrc
HiddenServiceDir /var/lib/tor/data/monero
# HiddenServicePort 18081 127.0.0.1:18081 # rpc port
HiddenServicePort 18083 127.0.0.1:18083   # tx port
```
and restart `sudo systemctl restart tor.service`. This will generate your `.onion` address in `/var/lib/tor/data/monero/hostname`. 

I2P must be configured with a standard server tunnel. Configuration differs by I2P implementation.


### Allow inbound RPC

The clients `monero-wallet-cli` and `monero-wallet-rpc` can connect to a daemon over AN. The daemon must provide a hidden service on free port that redirects the request to your local rpc port `--rpc-bind-ip`.
```
HiddenServicePort 18081 127.0.0.1:18081 # rpc port
```
Now not only localhost rpcs, but remote rpcs incoming from AN via port 18081 can use your service. Use with caution.

If you wish to run a public rpc service, further options for `monerod` are availble that restrict available commands, add rpc-ssl or require authentication. You can also require e.g. [Client Authoriztion](https://community.torproject.org/onion-services/advanced/) on the AN side.


### Connect to remote daemon

You can use a client to do rpc calls against a remote `monerod` peer. The wallet will be configured to use a onion/i2p address:
```
--proxy 127.0.0.1:9050             # tor or i2p entry port
--daemon-address rveahdfho.onion   # tor or
--daemon-address cmeua5767.b32.i2p # i2p
```
The onion and i2p addresses provide the information necessary to authenticate and encrypt the connection end-to-end, which is inherent to the address design. A _remote_ daemon is untrusted by default, unless `--trusted-daemon` is set. A _local_ daemon is trusted unless option `--untrusted-daemon` is set.

If desired, SSL can also be applied to the connection with `--daemon-address https://rveahdfho.onion` which requires a server certificate that is signed by a "root" certificate on the machine running the wallet. Alternatively, `--daemon-cert-file` can be used to specify a certificate to authenticate the server.

Proxies can also be used to connect to "clearnet" (ipv4 addresses or ICANN
domains), but `--daemon-cert-file` _must_ be used for authentication and
encryption.


## Configuration examples

These [terminal commands](https://www.debian.org/doc/manuals/debian-reference/ch05.en.html#_the_low_level_network_configuration) help you inspect your configuration results (interface/ip/port).

### Example A: 
This configuration puts together mentioned options.
```
proxy=127.0.0.1:5050        # tor out
tx-proxy=tor,127.0.0.1:9050,16 # onions out
anonymous-inbound=123.onion:18084,127.0.0.1:18084,8 # onions in
p2p-bind-ip=127.0.0.1  # no clearnet

out-peers=32 # reduce outpeers
in-peers=0  # meaningless, proxy is out and clearnet pulled localhost
# add-peer=mybuddy.onion:18084

# other
# check-updates=disabled
no-igd=1 # UPnP port forwa., pointless with tor
no-zmq=1 # if not needed
```
It proxies p2p traffic over tor port 5050 via exit-relays (outgoing connections). Local tx are sent to onions with max 16 outgoing peers. It also runs a local onion service to allow incoming peers to send tx. DNS goes over proxy. The p2p interface is bound to localhost and prevents clearnet traffic. Since proxy is outgoing only, in-peers is set to 0. The option `in-peers` does not affect # of services.


## Privacy Limitations

There are currently some techniques that could be used to _possibly_ identify
the machine that broadcast a transaction over an anonymity network.

### Timestamps

The peer timed sync command sends the current time in the message. This value
can be used to link an onion address to an IPv4/IPv6 address. If a peer first
sees a transaction over Tor, it could _assume_ (possibly incorrectly) that the
transaction originated from the peer. If both the Tor connection and an
IPv4/IPv6 connection have timestamps that are approximately close in value they
could be used to link the two connections. This is less likely to happen if the
system clock is fairly accurate - many peers on the Monero network should have
similar timestamps.

#### Mitigation

Keep the system clock accurate so that fingerprinting is more difficult. In
the future a random offset might be applied to anonymity networks so that if
the system clock is noticeably off (and therefore more fingerprintable),
linking the public IPv4/IPv6 connections with the anonymity networks will be
more difficult.

### Intermittent Monero Syncing

If a user only runs `monerod` to send a transaction then quit, this can also
be used by an ISP to link a user to a transaction.

#### Mitigation

Run `monerod` as often as possible to conceal when transactions are being sent.
Future versions will also have peers that first receive a transaction over an
anonymity network delay the broadcast to public peers by a randomized amount.
This will not completely mitigate a user who syncs up sends then quits, in
part because this rule is not enforceable, so this mitigation strategy is
simply a best effort attempt.

### Active Bandwidth Shaping

An attacker could attempt to bandwidth shape traffic in an attempt to determine
the source of a Tor/I2P connection. There isn't great mitigation against
this, but I2P should provide better protection against this attack since
the connections are not circuit based.

#### Mitigation

The best mitigation is to use I2P instead of Tor. However, I2P
has a smaller set of users (less cover traffic) and academic reviews, so there
is a trade off in potential issues. Also, anyone attempting this strategy really
wants to uncover a user, it seems unlikely that this would be performed against
every Tor/I2P user.

### I2P/Tor Stream Used Twice

If a single I2P/Tor stream is used 2+ times for transmitting a transaction, the
operator of the hidden service can conclude that both transactions came from the
same source. If the subsequent transactions spend a change output from the
earlier transactions, this will also reveal the "real" spend in the ring
signature. This issue was (primarily) raised by @secparam on Twitter.

#### Mitigation

`monerod` currently selects two outgoing connections every 5 minutes for
transmitting transactions over I2P/Tor. Using outgoing connections prevents an
adversary from making many incoming connections to obtain information (this
technique was taken from Dandelion). Outgoing connections also do not have a
persistent public key identity - the creation of a new circuit will generate
a new public key identity. The lock time on a change address is ~20 minutes, so
`monerod` will have rotated its selected outgoing connections several times in
most cases. However, the number of outgoing connections is typically a small
fixed number, so there is a decent probability of re-use with the same public
key identity.

@secparam (twitter) recommended changing circuits (Tor) as an additional
precaution. This is likely not a good idea - forcibly requesting Tor to change
circuits is observable by the ISP. Instead, `monerod` should likely disconnect
from peers occasionally. Tor will rotate circuits every ~10 minutes, so
establishing new connections will use a new public key identity and make it
more difficult for the hidden service to link information. This process will
have to be done carefully because closing/reconnecting connections can also
leak information to hidden services if done improperly.

At the current time, if users need to frequently make transactions, I2P/Tor
will improve privacy from ISPs and other common adversaries, but still have
some metadata leakages to unknown hidden service operators.



## Using Tor on tails

If you wish to have an additional privacy layer, you can encapsulate the node in a short-lived environment that prevents personal data being visible in case a process breaches. The project (Whonix)[https://www.whonix.org/wiki/Monero] uses a virtual-machine approach (needs host OS), but has monero preinstalled.  
In contrast, [Tails](https://tails.boum.org/) is a self-contained OS that can be run from an usb stick. It torifies as well but does not include monero binaries by default. Run the node with `--proxy 127.0.0.1:9050` and add an iptable rule if you wish to allow local rpc:
```
sudo iptables -I OUTPUT 2 -p tcp -dest 127.0.0.1 --dport 18081 -j ACCEPT
```
The kernel now allows tcp packages originating from local processes to reach the localhost rpc port. The `monero-wallet-cli` now connects as usual.


## Legacy options

Introducing new options can help get rid of previous setups.

### Wrapping monerod with torsocks

Monerod can be used wrapped with torsocks, by setting the following configuration parameters and environment variables:

* `--p2p-bind-ip 127.0.0.1` on the command line or `p2p-bind-ip=127.0.0.1` in
  monerod.conf to disable listening for connections on external interfaces.
* `--no-igd` on the command line or `no-igd=1` in monerod.conf to disable IGD
  (UPnP port forwarding negotiation), which is pointless with Tor.
* `DNS_PUBLIC=tcp` or `DNS_PUBLIC=tcp://x.x.x.x` where x.x.x.x is the IP of the
  desired DNS server, for DNS requests to go over TCP, so that they are routed
  through Tor. When IP is not specified, monerod uses the default list of
  servers defined in [src/common/dns_utils.cpp](src/common/dns_utils.cpp).
* `TORSOCKS_ALLOW_INBOUND=1` to tell torsocks to allow monerod to bind to interfaces
   to accept connections from the wallet. On some Linux systems, torsocks
   allows binding to localhost by default, so setting this variable is only
   necessary to allow binding to local LAN/VPN interfaces to allow wallets to
   connect from remote hosts. On other systems, it may be needed for local wallets
   as well.
* Do NOT pass `--detach` when running through torsocks with systemd, (see
  [utils/systemd/monerod.service](utils/systemd/monerod.service) for details).
* If you use the wallet with a Tor daemon via the loopback IP (eg, 127.0.0.1:9050),
  then use `--untrusted-daemon` unless it is your own hidden service.

Example command line to start monerod through Tor:

```bash
DNS_PUBLIC=tcp torsocks monerod --p2p-bind-ip 127.0.0.1 --no-igd
```

A helper script is in contrib/tor/monero-over-tor.sh. It assumes Tor is installed already, and runs Tor and Monero with the right configuration. More notes in the [TorifyHOWTO](https://gitlab.torproject.org/legacy/trac/-/wikis/doc/TorifyHOWTO#Howtotorifyspecificprograms) of torproject.

#### Torsocks and tails

TAILS ships with a very restrictive set of firewall rules. Therefore, you need
to add a rule to allow this connection too, in addition to telling torsocks to
allow inbound connections. Full example:

```bash
sudo iptables -I OUTPUT 2 -p tcp -d 127.0.0.1 -m tcp --dport 18081 -j ACCEPT
DNS_PUBLIC=tcp torsocks ./monerod --p2p-bind-ip 127.0.0.1 --no-igd --rpc-bind-ip 127.0.0.1 \
    --data-dir /home/amnesia/Persistent/your/directory/to/the/blockchain
```
