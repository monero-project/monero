# Levin Protocol
This is a document explaining the current design of the Levin protocol, as
used by Monero. The protocol is largely inherited from cryptonote, but has
undergone some changes.

This document also may differ from the `struct bucket_head2` in Monero's
code slightly - the spec here is slightly more strict to allow for
extensibility.

One of the goals of this document is to clearly indicate what is being sent
"on the wire" to identify metadata that could de-anonymize users over I2P/Tor.
These issues will be addressed as they are found. See `ANONYMITY_NETWORKS.md` in
the `docs` folder for any outstanding issues.

> This document does not currently list all data being sent by the monero
> protocol, that portion is a work-in-progress. Please take the time to do it
> if interested in learning about Monero p2p traffic!


## Encryption
Monero P2P encryption conceals the contents of P2P messages, and uses several
strategies to make deep-packet inspection (DPI) difficult:

  * Noise_IK handshake is used, and all data sent is uniformly random
  * Verifying the first poly1305 tag requires knowledge of the responders long
    term static x25519 pubkey (unique per node).
  * Randomized padding amounts are selected - 0-8192 bytes are added to every
    message

The goal is **not** to defend against all packet analysis (too difficult),
instead the goal is to make the basic forms more difficult. Regex matching
should be unworkable because all bytes of the stream are uniformly random and
the size is randomly varied. The major identifiable information is the default
port, which may be addressed in future modifications to the protocol. This
forces a spy to monitor the P2P network for IP/port peerlist information,
which is difficult to defend against since the information is public.

There is no packet timing obfuscation, perhaps in a future version (this should
need a client side rule(s) only). This is fairly advanced tracking compared
to regex or port number.

> Usage of a custom Noise protocol is somewhat controversial - it prevents an
> ISP from blocking SSL traffic to block Monero. Instead the ISP has to monitor
> the specific network to block specific IP/ports it sees in peerlist
> communication, OR has to aggressively block all traffic it cannot identify
> easily.


### Potential Concerns
One major concern with the design is denial of service (DoS) attacks due to the
expensive operations needed during a connection handshake. This is partially
mitigated by the incoming P2P connection limit, and is hardened by some rules:

  * An IP address can only make an inbound connection once every 10 seconds
  * An IP address can only send an IP/port once during peerlist sharing;
    "additional" pubkey/versions cannot be sent (i.e. as per description below,
    a node will store multiple pubkeys/versions for the same ip/port, but will
    only accept one pubkey/version from a peer).


### Design
Monero P2P connections are encrypted if both sender and receiver support the
mode. The protocol is designed to be backwards+forwards compatible - older
nodes will reject incoming encryption requests and newer nodes must know in
advance if a peer does not accept incoming encryption requests. If a node
makes an unencrypted request to a node that accepts encrypted connections, the
responding peer must indicate that it supports encryption so the initiator can
restart the connection with encryption. The only time a peer should incorrectly
make an encrypted connection is when a node disables encryption after previously
enabling it OR if the IP address was re-assigned. New versions of the encryption
protocol are optional until at least one hardfork has passed - after which the
prior version(s) _may_ be dropped. Version 0 of the encryption protocol will
become mandatory through this technique.

When IP/port peerlists are shared, additional information for a pubkey is now
transferred. The field name will indicate the type of key and version
(i.e. `v0_x25519` in a peerlist entry indicates the peer accepts `v0` encryption
mode and the pubkey for authentication is an x25519 key). This same field is
transferred in handshake messages so that peers can upgrade to an encrypted
stream when necessary.

The current version is based on the Noise_IK protocol with x25519 for key
exchange, Keccak for hashing, and Chacha20-Poly1305 for AEAD encryption. The
rationale for Noise_IK instead of SSL:

  * **Privacy:** The _entire_ payload, including the initial 32-byte pubkey,
    appears as random noise. A regex based DPI engine cannot identify the
    traffic; a custom processing engine to identify the stream must be done
    (once default port numbers are addressed).
  * **Efficiency:** The initial handshake message and client authentication can
    be sent immediately (0-RTT) with some reduced security (no forward-secrecy
    for initial 1 message). TLS 1.3 requires 2-RTT for client authentication
    before data, and 1-RTT for server only authentication before data.
  * **Simpler:** There is only one key exchange/hash/encryption algorithm in
    use per version, and any key re-negotiations occur at fixed points.


#### Specification
Version 0 is the Noise_IK handshake, with x25519 for key exchanges, Keccak for
hashing, and Chacha20-Poly1305 for AEAD encryption. The Noise_IK `protocol_name`
used in the chaining key input is `Monero_P2P_V0` - when/if a newer version is
rolled out, the responder will set `protocol_name` to the newest version
supported and compute authentication tags until a match is found.

An overview of the Noise_IK specification:
```
      <- s
      ...
      -> e, es, s, ss
      <- e, ee, se
```


##### Initiator_0
This is the first packet from the initiator (client):
```
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          random bytes NOT matching levin signature            |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                    initiator x25519 pubkey `e`                |
|                 encrypted with responder `s` key              |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|             poly1305 tag of initiator `e` pubkey              |
|                        using `s` as key                       |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                  initiator x25519 pubkey `s`                  |
|                    encrypted with `es` key                    |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|             poly1305 tag of initiator `s` pubkey              |
|                        using `es` as key                      |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 payload (handshake or ping)                   |
|              encrypted+poly1305 using `ss` as key             |
                               ...
```

The first 8 bytes are randomized by the initiator, with byte string
0x0121010101010101 being rejected since it matches the levin signature. Older
clients that do not support encryption will check for the levin signature and
close the connection.

The next 32 bytes is an ephermal x25519 pubkey generated by the
client/initiator that is encrypted using the HKDF process described by the Noise
protocol. The `protocol_name` is `MONERO_P2P_V0` and the first chaining input is
the static x25519 pubkey of the responder. This provides the responder with a
unique AEAD decryption key for the first ephermal pubkey - successful decryption
indicates correct protocol version and correct destination.

The `s` pubkey from the initiator is then decrypted and included in the
handshake as per Noise protocol rules. Successful decryption of the first
payload (handshake or ping) proves that the initiator knows the secret to pubkey
`s` and `e`.

The first payload sent uses the modified levin protocol.


#### Responder_0
This is the first packet from the responder (server):
```
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                   responder x25519 pubkey `e`                 |
|                     encrypted with `ss` key                   |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|             poly1305 tag of responder `e` pubkey              |
|                        using `ss` as key                      |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            payload                            |
|              encrypted+poly1305 using `se` as key             |
                               ...
```
The responder completes the forward-secrecy key exchange, and every payload
continues with the modified levin protocol.


#### Modified Levin Protocol
The v0 encryption protocol uses the same [levin header](#unencrypted-payload-header)
from the unencrypted protocol but removes the levin signature. Instead, poly1305
tags indicate whether the stream is valid.

The size field from the levin header is also encrypted AND poly1305 tagged. The
receiver should use `crypto_stream_chacha20_xor` to decrypt _just the length
field_ (at first). The length field indicates the entire payload size (including
randomized padding) but does not include the header size. The header+payload is
then decrypted using `crypto_aead_chachapoly1305_decrypt` where each call to the
function uses the minimum of 65535 bytes OR remaining bytes in the
header+payload. The same nonce is re-used in `crypto_stream_chacha20_xor` and
the first call to `crypto_aead_chachapoly1305_decrypt` because the the sender
encrypted in one shot (conceptually the nonce is **NOT** re-used; the first call
is simply decrypting without authentication). Some protocols send the length
unencrypted, but this makes the stream identifiable into packets.


#### Nonce/key
The nonce is a 64-bit counter in little-endian. A counter is kept in each
direction, and incremented everytime a call to
`crypto_aead_chachapoly1305_decrypt` is performed. This requires that the key
be unique per-stream and direction, and the Noise_IK handshake rules guarantees
this.

If a sender is going to exceed the 64-bit counter, it should immediately close
the stream. This should never happen in practice, unless the sender is
intentionally sending bogus data to overload the receiver.

> The initiator of the connection can technically "leak" the first ping or
> handshake message by re-using the same ephermal key to the same responder.
> However, the initiator is only leaking their private information, and the
> responder immediately provides their own ephermal key to protect the
> encrypted contents.


#### Authentication
Authentication is only enforced for seed nodes and pubkeys listed on the command
line with `--add-node`, `--add-priority-node`, or `--add-exclusive-node`. All
other situations are technically not authenticated. If an IP/port pair meets one
of these criteria, then only one pubkey is stored locally and this must be the
static pubkey used by the IP/port during the Noise_IK handshake. Connections are
immediately closed if the static pubkey does not match the expected version.

If a node is not one of the aforementioned cases, then each unique
IP/port/pubkey/version is stored. A IP/port/pubkey/version is only whitelisted
and transmitted to peers if a successful connection is made. Due to the design of
Noise_IK, a successful handshake or ping response indicates that the responder
has knowledge of the long-term static, but is not strictly authentication
because another pubkey for that IP/port can be cached+attempted. The difficulty
in this situation is authority - only a small subset of cases have verifiable
identity.

The selection process for IP/port/pubkey/version remains virtually the same as
before, except IP/port can be overloaded in the same manner that IP could be
overloaded with port values. The rules for limiting IP addresses or IP address
"classes" are used without change.


#### `--proxy` mode privacy
When `--proxy` mode is enabled, the default behavior is to send an ephermal
pubkey per connection for `s`. This prevents tracking over VPN/Tor. A switch
`--static-pubkey` overrides this behavior.


## Unencrypted Payload Header
This header is sent for every Monero p2p message.

```
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x01     |      0x21     |      0x01     |      0x01     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x01     |      0x01     |      0x01     |      0x01     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             Length                            |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  E. Response  |                   Command
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                |                 Return Code
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                |Q|S|B|E|               Reserved
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                |      0x01     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     0x00      |
+-+-+-+-+-+-+-+-+
```

### Signature
The first 8 bytes are the "signature" which helps identify the protocol (in
case someone connected to the wrong port, etc). The comments indicate that byte
sequence is from "benders nightmare".

This also can be used by deep packet inspection (DPI) engines to identify
Monero when the link is not encrypted. SSL has been proposed as a means to
mitigate this issue, but BIP-151 or the Noise protocol should also be considered.

### Length
The length is an unsigned 64-bit little endian integer. The length does _not_
include the header.

The implementation currently rejects received messages that exceed 100 MB
(base 10) by default.

### Expect Response
A zero-byte if no response is expected from the peer, and a non-zero byte if a
response is expected from the peer. Peers must respond to requests with this
flag in the same order that they were received, however, other messages can be
sent between responses.

There are some commands in the
[cryptonote protocol](#cryptonote-protocol-commands) where a response is
expected from the peer, but this flag is not set. Those responses are returned
as notify messages and can be sent in any order by the peer.

### Command
An unsigned 32-bit little endian integer representing the Monero specific
command being invoked.

### Return Code
A signed 32-bit little endian integer representing the response from the peer
from the last command that was invoked. This is `0` for request messages.

### Flags
 * `Q` - Bit is set if the message is a request.
 * `S` - Bit is set if the message is a response.
 * `B` - Bit is set if this is a the beginning of a [fragmented message](#fragmented-messages).
 * `E` - Bit is set if this is the end of a [fragmented message](#fragmented-messages).

### Version
A fixed value of `1` as an unsigned 32-bit little endian integer.


## Message Flow
The protocol can be subdivided into: (1) notifications, (2) requests,
(3) responses, (4) fragmented messages, and (5) dummy messages. Response
messages must be sent in the same order that a peer issued a request message.
A peer does not have to send a response immediately following a request - any
other message type can be sent instead.

### Notifications
Notifications are one-way messages that can be sent at any time without
an expectation of a response from the peer. The `Q` bit must be set, the `S`,
`B` and `E` bits must be unset, and the `Expect Response` field must be zeroed.

Some notifications must be in response to other notifications. This is not
part of the levin messaging layer, and is described in the
[commands](#commands) section.

### Requests
Requests are the basis of the admin protocol for Monero. The `Q` bit must be
set, the `S`, `B` and `E` bits must be unset, and the `Expect Response` field
must be non-zero. The peer is expected to send a response message with the same
`command` number.

### Responses
Response message can only be sent after a peer first issues a request message.
Responses must have the `S` bit set, the `Q`, `B` and `E` bits unset, and have
a zeroed `Expect Response` field. The `Command` field must be the same value
that was sent in the request message. The `Return Code` is specific to the
`Command` being issued (see [commands])(#commands)).

### Fragmented
Fragmented messages were introduced for the "white noise" feature for i2p/tor.
A transaction can be sent in fragments to conceal when "real" data is being
sent instead of dummy messages. Only one fragmented message can be sent at a
time, and bits `B` and `E` are never set at the same time
(see [dummy messages](#dummy)). The re-constructed message must contain a
levin header for a different (non-fragment) message type.

The `Q` and `S` bits are never set and the `Expect Response` field must always
be zero. The first fragment has the `B` bit set, neither `B` nor `E` is set for
"middle" fragments, and `E` is set for the last fragment.

### Dummy
Dummy messages have the `B` and `E` bits set, the `Q` and `S` bits unset, and
the `Expect Response` field zeroed. When a message of this type is received, the
contents can be safely ignored.


## Commands
### P2P (Admin) Commands

#### (`1001` Request) Handshake
#### (`1001` Response) Handshake
#### (`1002` Request) Timed Sync
#### (`1002` Response) Timed Sync
#### (`1003` Request) Ping
#### (`1003` Response) Ping
#### (`1004` Request) Stat Info
#### (`1004` Response) Stat Info
#### (`1005` Request) Network State
#### (`1005` Response) Network State
#### (`1006` Request) Peer ID
#### (`1006` Response) Peer ID
#### (`1007` Request) Support Flags
#### (`1007` Response) Support Flags

### Cryptonote Protocol Commands

#### (`2001` Notification) New Block
#### (`2002` Notification) New Transactions
#### (`2003` Notification) Request Get Objects
#### (`2004` Notification) Response Get Objects
#### (`2006` Notification) Request Chain
#### (`2007` Notification) Response Chain Entry
#### (`2008` Notification) New Fluffy Block
#### (`2009` Notification) Request Fluffy Missing TX
