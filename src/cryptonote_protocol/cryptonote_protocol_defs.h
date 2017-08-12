// Copyright (c) 2014-2017, The Monero Project
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
#include "cryptonote_protocol/blobdatatype.h"
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

    std::string address;
    std::string host;
    std::string ip;
    std::string port;

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

	boost::uuids::uuid connection_id;

    uint64_t height;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(incoming)
      KV_SERIALIZE(localhost)
      KV_SERIALIZE(local_ip)
      KV_SERIALIZE(address)
      KV_SERIALIZE(host)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
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
      KV_SERIALIZE_VAL_POD_AS_BLOB(connection_id)
      KV_SERIALIZE(height)
    END_KV_SERIALIZE_MAP()
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct block_complete_entry
  {
    blobdata block;
    std::list<blobdata> txs;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(block)
      KV_SERIALIZE(txs)
    END_KV_SERIALIZE_MAP()
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 1;

    struct request
    {
      block_complete_entry b;
      uint64_t current_blockchain_height;
      uint32_t hop;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
        KV_SERIALIZE(hop)
      END_KV_SERIALIZE_MAP()
    };
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_TRANSACTIONS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 2;

    struct request
    {
      std::list<blobdata>   txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
      END_KV_SERIALIZE_MAP()
    };
  };
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 3;

    struct request
    {
      std::list<crypto::hash>    txs;
      std::list<crypto::hash>    blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(txs)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(blocks)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct NOTIFY_RESPONSE_GET_OBJECTS
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 4;

    struct request
    {
      std::list<blobdata>              txs;
      std::list<block_complete_entry>  blocks;
      std::list<crypto::hash>               missed_ids;
      uint64_t                         current_blockchain_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs)
        KV_SERIALIZE(blocks)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missed_ids)
        KV_SERIALIZE(current_blockchain_height)
      END_KV_SERIALIZE_MAP()
    };
  };


  struct CORE_SYNC_DATA
  {
    uint64_t current_height;
    uint64_t cumulative_difficulty;
    crypto::hash  top_id;
    uint8_t top_version;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(current_height)
      KV_SERIALIZE(cumulative_difficulty)
      KV_SERIALIZE_VAL_POD_AS_BLOB(top_id)
      KV_SERIALIZE_OPT(top_version, (uint8_t)0)
    END_KV_SERIALIZE_MAP()
  };

  struct NOTIFY_REQUEST_CHAIN
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 6;

    struct request
    {
      std::list<crypto::hash> block_ids; /*IDs of the first 10 blocks are sequential, next goes with pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct NOTIFY_RESPONSE_CHAIN_ENTRY
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 7;

    struct request
    {
      uint64_t start_height;
      uint64_t total_height;
      uint64_t cumulative_difficulty;
      std::list<crypto::hash> m_block_ids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(total_height)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_NEW_FLUFFY_BLOCK
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 8;

    struct request
    {
      block_complete_entry b;
      uint64_t current_blockchain_height;
      uint32_t hop;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(b)
        KV_SERIALIZE(current_blockchain_height)
        KV_SERIALIZE(hop)
      END_KV_SERIALIZE_MAP()
    };
  };  

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  struct NOTIFY_REQUEST_FLUFFY_MISSING_TX
  {
    const static int ID = BC_COMMANDS_POOL_BASE + 9;

    struct request
    {
      crypto::hash block_hash;
      uint64_t current_blockchain_height;      
      std::vector<size_t> missing_tx_indices;
      uint32_t hop;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(block_hash)
        KV_SERIALIZE(current_blockchain_height)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(missing_tx_indices)
        KV_SERIALIZE(hop)
      END_KV_SERIALIZE_MAP()
    };
  }; 
    
}
