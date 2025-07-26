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

#include <list>
#include "serialization/keyvalue_serialization.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/blobdatatype.h"
#include "byte_slice.h"
#include "storages/portable_storage_template_helper.h"

namespace cryptonote
{


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
  struct NOTIFY_NEW_TRANSACTIONS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 2;

    struct request_t
    {
      std::vector<blobdata>   txs;
      std::string _; // padding
      bool dandelionpp_fluff; //zero initialization defaults to stem mode

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
        KV_SERIALIZE(_)
        KV_SERIALIZE_OPT(dandelionpp_fluff, true) // backwards compatible mode is fluff
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  struct tx_request_padding
  {
    void operator()(NOTIFY_NEW_TRANSACTIONS::request& request) const
    {
      size_t bytes = 9 /* header */ + 4 /* 1 + 'txs' */ + tools::get_varint_data(request.txs.size()).size();
      for(auto tx_blob_it = request.txs.begin(); tx_blob_it!=request.txs.end(); ++tx_blob_it)
        bytes += tools::get_varint_data(tx_blob_it->size()).size() + tx_blob_it->size();

      // stuff some dummy bytes in to stay safe from traffic volume analysis
      static constexpr const size_t granularity = 1024;
      size_t padding = granularity - bytes % granularity;
      const size_t overhead = 2 /* 1 + '_' */ + tools::get_varint_data(padding).size();
      if (overhead > padding)
        padding = 0;
      else
        padding -= overhead;
      request._ = std::string(padding, ' ');

      epee::byte_slice arg_buff;
      epee::serialization::store_t_to_binary(request, arg_buff);

      // we probably lowballed the payload size a bit, so added a but too much. Fix this now.
      size_t remove = arg_buff.size() % granularity;
      if (remove > request._.size())
        request._.clear();
      else
        request._.resize(request._.size() - remove);
      // if the size of _ moved enough, we might lose byte in size encoding, we don't care
    }
  };
  inline constexpr tx_request_padding pad_tx_request{};

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 3;

    struct request_t
    {
      std::vector<crypto::hash> blocks;
      bool prune;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(blocks)
        KV_SERIALIZE_OPT(prune, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  struct NOTIFY_RESPONSE_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 4;

    struct request_t
    {
      std::vector<block_complete_entry>  blocks;
      std::vector<crypto::hash>          missed_ids;
      uint64_t                         current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missed_ids)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };


  struct CORE_SYNC_DATA
  {
    uint64_t current_height;
    uint64_t cumulative_difficulty;
    uint64_t cumulative_difficulty_top64;
    crypto::hash  top_id;
    uint8_t top_version;
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

  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request_t
    {
      std::list<crypto::hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      bool prune;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE_OPT(prune, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

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
  struct NOTIFY_REQUEST_FLUFFY_MISSING_TX
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 9;

    struct request_t
    {
      crypto::hash block_hash;
      uint64_t current_blockchain_height;      
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
  struct NOTIFY_GET_TXPOOL_COMPLEMENT
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 10;

    struct request_t
    {
      std::vector<crypto::hash> hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(hashes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /************************************************************************
  * Announces new transaction hashes that                                 *
  * the sender believes the receiver may not have.                        *
  * The receiver can pull which hashes are missing locally                *
  * and optionally request the actual serialized transactions for them.   *
  *************************************************************************/
  struct NOTIFY_TX_POOL_INV
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 11;

    struct request_t
    {
      std::vector<crypto::hash> t;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(t)
      END_KV_SERIALIZE_MAP()
    };

    typedef epee::misc_utils::struct_init<request_t> request;
  };

  /************************************************************************
  * Requests the actual transaction data corresponding                    *
  * to a set of transaction hashes.                                       *
  * The receiver should look up each transaction in its own pool          *
  * or memory and respond with the serialized version, if known.          *
  *************************************************************************/
  struct NOTIFY_REQUEST_TX_POOL_TXS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 12;

    struct request_t
    {
      std::vector<crypto::hash> t;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(t)
      END_KV_SERIALIZE_MAP()
    };

    typedef epee::misc_utils::struct_init<request_t> request;
  };
    
}
