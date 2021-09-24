# The Current/Future Status of ZMQ in Monero

## ZMQ Pub/Sub
Client `ZMQ_SUB` sockets must "subscribe" to topics before it receives any data.
This allows filtering on the server side, so network traffic is reduced. Monero
allows for filtering on: (1) format, (2) context, and (3) event.

 * **format** refers to the _wire_ format (i.e. JSON) used to send event
   information.
 * **context** allows for a reduction in fields for the event, so the
   daemon doesn't waste cycles serializing fields that get ignored.
 * **event** refers to status changes occurring within the daemon (i.e. new
   block to main chain).

 * Formats:
   * `json`
 * Contexts:
   * `full` - the entire block or transaction is transmitted (the hash can be
     computed remotely).
   * `minimal` - the bare minimum for a remote client to react to an event is
     sent.
 * Events:
   * `chain_main` - changes to the primary/main blockchain.
   * `txpool_add` - new _publicly visible_ transactions in the mempool.
     Includes previously unseen transactions in a block but _not_ the
     `miner_tx`. Does not "re-publish" after a reorg. Includes `do_not_relay`
     transactions.
   * `miner_data` - provides the necessary data to create a custom block template
     Available only in the `full` context.

The subscription topics are formatted as `format-context-event`, with prefix
matching supported by both Monero and ZMQ. The `format`, `context` and `event`
will _never_ have hyphens or colons in their name. For example, subscribing to
`json-minimal-chain_main` will send minimal information in JSON when changes
to the main/primary blockchain occur. Whereas, subscribing to `json-minimal`
will send minimal information in JSON on all available events supported by the
daemon.

The Monero daemon will ensure that events prefixed by `chain` will be sent in
"chain-order" - the `prev_id` (hash) field will _always_ refer to a previous
block. On rollbacks/reorgs, the event will reference an earlier block in the
chain instead of the last block. The Monero daemon also ensures that
`txpool_add` events are sent before `chain_*` events - the `chain_*` messages
will only serialize miner transactions since the other transactions were
previously published via `txpool_add`. This prevents transactions from being
serialized twice, even when the transaction was first observed in a block.

ZMQ Pub/Sub will drop messages if the network is congested, so the above rules
for send order are used for detecting lost messages. A missing gap in `height`
or `prev_id` for `chain_*` events indicates a lost pub message. Missing
`txpool_add` messages can only be detected at the next `chain_` message.

Since blockchain events can be dropped, clients will likely want to have a
timeout against `chain_main` events. The `GetLastBlockHeader` RPC is useful
for checking the current chain state. Dropped messages should be rare in most
conditions.

The Monero daemon will send a `txpool_add` pub exactly once for each
transaction, even after a reorg or restarts. Clients should use the
`GetTransactionPool` after a reorg to get all transactions that have been put
back into the tx pool or been invalidated due to a double-spend.


