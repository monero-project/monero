// Copyright (c) 2014-2024, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "crypto/hash.h"
#include "cryptonote_basic/blobdatatype.h"
#include "misc_language.h"
#include "serialization/keyvalue_serialization.h"

#include <list>

namespace cryptonote
{

/**
 * Number to start at for protocol message IDs.
 *
 * Every message below names its contents `request_t`, even the replies,
 * because they are all one-way notifies at the Levin level.
 */
#define BC_COMMANDS_POOL_BASE 2000

  /************************************************************************/
  /* P2P connection info, serializable to json                            */
  /************************************************************************/
  struct connection_info
  {
    bool incoming;
    bool localhost;
    bool local_ip;
    bool ssl;

    std::string address;
    std::string host;
    std::string ip;
    std::string port;
    uint16_t rpc_port;
    uint32_t rpc_credits_per_hash;

    std::string peer_id;

    uint64_t recv_count;
    uint64_t recv_idle_time;

    uint64_t send_count;
    uint64_t send_idle_time;

    std::string state;

    uint64_t live_time;

	uint64_t avg_download;
	uint64_t current_download;
	
	uint64_t avg_upload;
	uint64_t current_upload;
  
	uint32_t support_flags;

	std::string connection_id;

    uint64_t height;

    uint32_t pruning_seed;

    uint8_t address_type;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(incoming)
      KV_SERIALIZE(localhost)
      KV_SERIALIZE(local_ip)
      KV_SERIALIZE(address)
      KV_SERIALIZE(host)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
      KV_SERIALIZE(rpc_port)
      KV_SERIALIZE(rpc_credits_per_hash)
      KV_SERIALIZE(peer_id)
      KV_SERIALIZE(recv_count)
      KV_SERIALIZE(recv_idle_time)
      KV_SERIALIZE(send_count)
      KV_SERIALIZE(send_idle_time)
      KV_SERIALIZE(state)
      KV_SERIALIZE(live_time)
      KV_SERIALIZE(avg_download)
      KV_SERIALIZE(current_download)
      KV_SERIALIZE(avg_upload)
      KV_SERIALIZE(current_upload)
      KV_SERIALIZE(support_flags)
      KV_SERIALIZE(connection_id)
      KV_SERIALIZE(height)
      KV_SERIALIZE(pruning_seed)
      KV_SERIALIZE(address_type)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct tx_blob_entry
  {
    blobdata blob;
    crypto::hash prunable_hash;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(blob)
      KV_SERIALIZE_VAL_POD_AS_BLOB(prunable_hash)
    END_KV_SERIALIZE_MAP()

    tx_blob_entry(const blobdata &bd = {}, const crypto::hash &h = crypto::null_hash): blob(bd), prunable_hash(h) {}
  };
  struct block_complete_entry
  {
    bool pruned;
    blobdata block;
    uint64_t block_weight;
    std::vector<tx_blob_entry> txs;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_OPT(pruned, false)
      KV_SERIALIZE(block)
      KV_SERIALIZE_OPT(block_weight, (uint64_t)0)
      if (this_ref.pruned)
      {
        KV_SERIALIZE(txs)
      }
      else
      {
        std::vector<blobdata> txs;
        if (is_store)
        {
          txs.reserve(this_ref.txs.size());
          for (const auto &e: this_ref.txs) txs.push_back(e.blob);
        }
        epee::serialization::selector<is_store>::serialize(txs, stg, hparent_section, "txs");
        if (!is_store)
        {
          block_complete_entry &self = const_cast<block_complete_entry&>(this_ref);
          self.txs.clear();
          self.txs.reserve(txs.size());
          for (auto &e: txs) self.txs.push_back({std::move(e), crypto::null_hash});
        }
      }
    END_KV_SERIALIZE_MAP()

    block_complete_entry(): pruned(false), block_weight(0) {}
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Legacy message type providing a new block. It is receive-only, as nothing
   * in the codebase sends it anymore, and is just converted into a fluffy
   * block.
   */
  struct NOTIFY_NEW_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 1;

    struct request_t
    {
      block_complete_entry b;
      uint64_t current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Message type relaying transactions, in either the Dandelion++ stem or
   * fluff phase. It is also the reply to `NOTIFY_GET_TXPOOL_COMPLEMENT`. This
   * is ignored unless the peer is in `state_normal` and we are fully synced,
   * and a peer sending a duplicate or invalid transaction will be dropped.
   */
  struct NOTIFY_NEW_TRANSACTIONS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 2;

    struct request_t
    {
      //! Transaction blobs.
      std::vector<blobdata>   txs;

      /**
       * Padding, rounding the message size up to a multiple of 1 KiB when the
       * `--pad-transactions` option is set, as an attempt to prevent traffic
       * analysis.
       */
      std::string _;

      /**
       * True for fluff, false for stem. If missing on the wire, this defaults
       * to true, as older peers always fluff; zero initialization defaults to
       * stem.
       */
      bool dandelionpp_fluff;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
        KV_SERIALIZE(_)
        KV_SERIALIZE_OPT(dandelionpp_fluff, true) // backwards compatible mode is fluff
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Message type asking a peer for the full blocks in one span, with the reply
   * being `NOTIFY_RESPONSE_GET_OBJECTS`. A peer asking for more than
   * `CURRENCY_PROTOCOL_MAX_OBJECT_REQUEST_COUNT` (100) blocks, or sending this
   * before the handshake, will be dropped.
   */
  struct NOTIFY_REQUEST_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 3;

    struct request_t
    {
      //! Hashes of the blocks being requested.
      std::vector<crypto::hash> blocks;

      //! Whether pruned transaction data is wanted for these blocks.
      bool prune;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(blocks)
        KV_SERIALIZE_OPT(prune, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /**
   * Message type providing the blocks (plus their transactions) asked for in
   * `NOTIFY_REQUEST_GET_OBJECTS`. A peer will be dropped if it sends this
   * unrequested, sends no blocks, sends a block that wasn't requested, leaves
   * out a requested block, or sends pruned data that wasn't asked for.
   */
  struct NOTIFY_RESPONSE_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 4;

    struct request_t
    {
      //! The requested blocks; `pruned` and `block_weight` are set when pruned.
      std::vector<block_complete_entry>  blocks;

      //! Hashes of requested blocks that the peer doesn't have.
      std::vector<crypto::hash>          missed_ids;

      //! The peer's current chain height.
      uint64_t                         current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missed_ids)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /**
   * Summary of our chain tip, exchanged with every peer. It is not a message
   * itself, but the `payload_data` carried inside the P2P `COMMAND_HANDSHAKE`
   * and `COMMAND_TIMED_SYNC` commands (1001 and 1002). It is built by
   * `get_payload_sync_data` and read by `process_payload_sync_data`.
   */
  struct CORE_SYNC_DATA
  {
    //! Top block height + 1 (i.e. the number of blocks in the chain).
    uint64_t current_height;

    /**
     * Low and high 64 bits of the 128-bit cumulative difficulty. The high part
     * defaults to 0 when an older peer leaves it out.
     */
    uint64_t cumulative_difficulty;

    //! High 64 bits of the cumulative difficulty; see `cumulative_difficulty`.
    uint64_t cumulative_difficulty_top64;

    crypto::hash  top_id;
    uint8_t top_version;

    //! Our pruning seed, or 0 if not pruned.
    uint32_t pruning_seed;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(current_height)
      KV_SERIALIZE(cumulative_difficulty)
      if (is_store)
        KV_SERIALIZE(cumulative_difficulty_top64)
      else
        KV_SERIALIZE_OPT(cumulative_difficulty_top64, (uint64_t)0)
      KV_SERIALIZE_VAL_POD_AS_BLOB(top_id)
      KV_SERIALIZE_OPT(top_version, (uint8_t)0)
      KV_SERIALIZE_OPT(pruning_seed, (uint32_t)0)
    END_KV_SERIALIZE_MAP()
  };

  /**
   * Message type asking a peer where our chains diverge, by sending a sample of
   * our chain. The peer finds the first hash it knows and replies with
   * `NOTIFY_RESPONSE_CHAIN_ENTRY`, listing hashes starting from it. A peer
   * sending this before the handshake will be dropped.
   */
  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request_t
    {
      std::list<crypto::hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

      /**
       * Whether we accept pruned blocks. If false, the peer only lists hashes
       * up to its next pruned block, so that it can serve all of them in full.
       */
      bool prune;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE_OPT(prune, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /**
   * Message type providing the starting height, total height, and block hashes
   * (plus weights), in reply to `NOTIFY_REQUEST_CHAIN`. Senders include up to
   * `BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT` (10,000) hashes, and receivers
   * accept up to `BLOCKS_IDS_SYNCHRONIZING_MAX_COUNT` (25,000).
   */
  struct NOTIFY_RESPONSE_CHAIN_ENTRY
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 7;

    struct request_t
    {
      uint64_t start_height;
      uint64_t total_height;
      uint64_t cumulative_difficulty;
      uint64_t cumulative_difficulty_top64;
      std::vector<crypto::hash> m_block_ids;
      std::vector<uint64_t> m_block_weights;
      cryptonote::blobdata first_block;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(total_height)
        KV_SERIALIZE(cumulative_difficulty)
        if (is_store)
          KV_SERIALIZE(cumulative_difficulty_top64)
        else
          KV_SERIALIZE_OPT(cumulative_difficulty_top64, (uint64_t)0)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_weights)
        KV_SERIALIZE(first_block)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };
  
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Message type issuing a new (fluffy) block. When relayed, its transactions
   * are stripped; the receiver asks for any it lacks with
   * `NOTIFY_REQUEST_FLUFFY_MISSING_TX`, and the reply is another
   * `NOTIFY_NEW_FLUFFY_BLOCK` with those transactions attached.
   */
  struct NOTIFY_NEW_FLUFFY_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 8;

    struct request_t
    {
      block_complete_entry b;
      uint64_t current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };  

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Message type asking a peer for the transactions we are missing from a
   * fluffy block. The reply is a `NOTIFY_NEW_FLUFFY_BLOCK` message with those
   * transactions attached. A peer will be dropped if it sends this before the
   * handshake, asks about an unknown block or a duplicate/out-of-range index,
   * or transactions that can't be found.
   */
  struct NOTIFY_REQUEST_FLUFFY_MISSING_TX
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 9;

    struct request_t
    {
      //! Hash of the fluffy block these transactions belong to.
      crypto::hash block_hash;

      /**
       * Set by the requester to the block's height + 1, and copied by the peer
       * into its reply.
       */
      uint64_t current_blockchain_height;

      /**
       * Indices into the block's `tx_hashes` of the missing transactions. These
       * are used instead of hashes, as they are smaller.
       */
      std::vector<uint64_t> missing_tx_indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(block_hash)
        KV_SERIALIZE(current_blockchain_height)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missing_tx_indices)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  }; 

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  /**
   * Message type asking a peer for the pool transactions we don't have. The
   * reply is a `NOTIFY_NEW_TRANSACTIONS` message. This is sent once, to one
   * peer, right after we are fully synced, and is ignored unless the peer is
   * in `state_normal`.
   *
   * The peer only returns transactions that were already 'fluffed' or came
   * from blocks, so stem-phase transactions still under embargo are never
   * leaked.
   */
  struct NOTIFY_GET_TXPOOL_COMPLEMENT
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 10;

    struct request_t
    {
      //! Hashes of the transactions already in our pool.
      std::vector<crypto::hash> hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(hashes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };
    
}
