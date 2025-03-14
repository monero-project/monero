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

#include "string_tools.h"

#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/difficulty.h"
#include "crypto/hash.h"
#include "fcmp_pp/curve_trees.h"
#include "rpc/rpc_handler.h"
#include "common/varint.h"
#include "common/perf_timer.h"

namespace
{
  template<typename T>
  std::string compress_integer_array(const std::vector<T> &v)
  {
    std::string s;
    s.resize(v.size() * (sizeof(T) * 8 / 7 + 1));
    char *ptr = (char*)s.data();
    for (const T &t: v)
      tools::write_varint(ptr, t);
    s.resize(ptr - s.data());
    return s;
  }

  template<typename T>
  std::vector<T> decompress_integer_array(const std::string &s)
  {
    std::vector<T> v;
    v.reserve(s.size());
    int read = 0;
    const std::string::const_iterator end = s.end();
    for (std::string::const_iterator i = s.begin(); i != end; std::advance(i, read))
    {
      T t;
      read = tools::read_varint(std::string::const_iterator(i), s.end(), t);
      CHECK_AND_ASSERT_THROW_MES(read > 0 && read <= 256, "Error decompressing data");
      v.push_back(t);
    }
    return v;
  }
}

namespace cryptonote
{
  //-----------------------------------------------
#define CORE_RPC_STATUS_OK   "OK"
#define CORE_RPC_STATUS_BUSY   "BUSY"
#define CORE_RPC_STATUS_NOT_MINING "NOT MINING"
#define CORE_RPC_STATUS_PAYMENT_REQUIRED "PAYMENT REQUIRED"

inline const std::string get_rpc_status(const bool trusted_daemon, const std::string &s)
{
  if (trusted_daemon)
    return s;
  if (s == CORE_RPC_STATUS_OK)
    return s;
  if (s == CORE_RPC_STATUS_BUSY)
    return s;
  if (s == CORE_RPC_STATUS_PAYMENT_REQUIRED)
    return s;
  return "<error>";
}

// When making *any* change here, bump minor
// If the change is incompatible, then bump major and set minor to 0
// This ensures CORE_RPC_VERSION always increases, that every change
// has its own version, and that clients can just test major to see
// whether they can talk to a given daemon without having to know in
// advance which version they will stop working with
// Don't go over 32767 for any of these
// TODO: make sure this matches RPC version expected in wallet2::check_version when FCMP++ is ready
#define CORE_RPC_VERSION_MAJOR 3
#define CORE_RPC_VERSION_MINOR 17
#define MAKE_CORE_RPC_VERSION(major,minor) (((major)<<16)|(minor))
#define CORE_RPC_VERSION MAKE_CORE_RPC_VERSION(CORE_RPC_VERSION_MAJOR, CORE_RPC_VERSION_MINOR)

  struct rpc_request_base
  {
    BEGIN_KV_SERIALIZE_MAP()
    END_KV_SERIALIZE_MAP()
  };

  struct rpc_response_base
  {
    std::string status;
    bool untrusted;

    rpc_response_base(): untrusted(false) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(status)
      KV_SERIALIZE(untrusted)
    END_KV_SERIALIZE_MAP()
  };

  struct rpc_access_request_base: public rpc_request_base
  {
    std::string client;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_PARENT(rpc_request_base)
      KV_SERIALIZE(client)
    END_KV_SERIALIZE_MAP()
  };

  struct rpc_access_response_base: public rpc_response_base
  {
    uint64_t credits;
    std::string top_hash;

    rpc_access_response_base(): credits(0) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE_PARENT(rpc_response_base)
      KV_SERIALIZE(credits)
      KV_SERIALIZE(top_hash)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_HEIGHT
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint64_t 	 height;
      std::string hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE(hash)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BLOCKS_FAST
  {

    enum REQUESTED_INFO
    {
      BLOCKS_ONLY = 0,
      BLOCKS_AND_POOL = 1,
      POOL_ONLY = 2
    };

    struct request_t: public rpc_access_request_base
    {
      uint8_t     requested_info;
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      bool        prune;
      bool        no_miner_tx;
      bool        high_height_ok;
      uint64_t    pool_info_since;
      uint64_t    max_block_count;
      bool        init_tree_sync;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE_OPT(requested_info, (uint8_t)0)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(prune)
        KV_SERIALIZE_OPT(no_miner_tx, false)
        KV_SERIALIZE_OPT(high_height_ok, false) // default false maintains backwards compatibility for clients that relied on failure on high height
        KV_SERIALIZE_OPT(pool_info_since, (uint64_t)0)
        KV_SERIALIZE_OPT(max_block_count, (uint64_t)0)
        KV_SERIALIZE_OPT(init_tree_sync, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct tx_output_indices
    {
      std::vector<uint64_t> indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(indices)
      END_KV_SERIALIZE_MAP()
    };

    struct block_output_indices
    {
      std::vector<tx_output_indices> indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(indices)
      END_KV_SERIALIZE_MAP()
    };

    struct pool_tx_info
    {
      crypto::hash tx_hash;
      blobdata tx_blob;
      bool double_spend_seen;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(tx_hash)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(double_spend_seen)
      END_KV_SERIALIZE_MAP()
    };

    enum POOL_INFO_EXTENT
    {
      NONE = 0,
      INCREMENTAL = 1,
      FULL = 2
    };

    struct locked_outputs_t
    {
      uint64_t last_locked_block;
      std::vector<fcmp_pp::curve_trees::OutputContext> outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(last_locked_block)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outputs)
      END_KV_SERIALIZE_MAP()
    };

    struct init_tree_sync_data_t
    {
      uint64_t init_block_idx;
      crypto::hash init_block_hash;
      /// n leaf tuples in the chain when the chain tip was block init_block_idx
      uint64_t n_leaf_tuples;
      /// the last full path in the tree when the chain tip was block init_block_idx
      fcmp_pp::curve_trees::PathBytes last_path;
      /// outputs created before init_block_idx with last locked block >= init_block_idx
      std::vector<locked_outputs_t> locked_outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(init_block_idx)
        KV_SERIALIZE_VAL_POD_AS_BLOB(init_block_hash)
        KV_SERIALIZE(n_leaf_tuples)
        KV_SERIALIZE(last_path)
        KV_SERIALIZE(locked_outputs)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      std::vector<block_complete_entry> blocks;
      uint64_t    start_height;
      uint64_t    current_height;
      crypto::hash top_block_hash;
      std::vector<block_output_indices> output_indices;
      uint64_t    daemon_time;
      uint8_t     pool_info_extent;
      std::vector<pool_tx_info> added_pool_txs;
      std::vector<crypto::hash> remaining_added_pool_txids;
      std::vector<crypto::hash> removed_pool_txids;
      bool included_init_tree_sync_data;
      init_tree_sync_data_t init_tree_sync_data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE_VAL_POD_AS_BLOB_OPT(top_block_hash, crypto::null_hash)
        KV_SERIALIZE(output_indices)
        KV_SERIALIZE_OPT(daemon_time, (uint64_t) 0)
        KV_SERIALIZE_OPT(pool_info_extent, (uint8_t) 0)
        if (pool_info_extent != POOL_INFO_EXTENT::NONE)
        {
          KV_SERIALIZE(added_pool_txs)
          KV_SERIALIZE_CONTAINER_POD_AS_BLOB(remaining_added_pool_txids)
        }
        if (pool_info_extent == POOL_INFO_EXTENT::INCREMENTAL)
        {
          KV_SERIALIZE_CONTAINER_POD_AS_BLOB(removed_pool_txids)
        }
        KV_SERIALIZE_OPT(included_init_tree_sync_data, false)
        if (included_init_tree_sync_data)
        {
          KV_SERIALIZE(init_tree_sync_data)
        }
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BLOCKS_BY_HEIGHT
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<uint64_t> heights;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(heights)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<block_complete_entry> blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(blocks)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

    struct COMMAND_RPC_GET_ALT_BLOCKS_HASHES
    {
        struct request_t: public rpc_access_request_base
        {
            BEGIN_KV_SERIALIZE_MAP()
                KV_SERIALIZE_PARENT(rpc_access_request_base)
            END_KV_SERIALIZE_MAP()
        };
        typedef epee::misc_utils::struct_init<request_t> request;

        struct response_t: public rpc_access_response_base
        {
            std::vector<std::string> blks_hashes;

            BEGIN_KV_SERIALIZE_MAP()
                KV_SERIALIZE_PARENT(rpc_access_response_base)
                KV_SERIALIZE(blks_hashes)
            END_KV_SERIALIZE_MAP()
        };
        typedef epee::misc_utils::struct_init<response_t> response;
    };
  struct COMMAND_RPC_GET_HASHES_FAST
  {

    struct request_t: public rpc_access_request_base
    {
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<crypto::hash> m_block_ids;
      uint64_t    start_height;
      uint64_t    current_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_SUBMIT_RAW_TX
  {
      struct request_t
      {
        std::string address;
        std::string view_key;
        std::string tx;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
          KV_SERIALIZE(tx)  
        END_KV_SERIALIZE_MAP()
      };
      typedef epee::misc_utils::struct_init<request_t> request;
    
      
      struct response_t
      {
        std::string status;
        std::string error;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_SERIALIZE(error)
        END_KV_SERIALIZE_MAP()
      };
      typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TRANSACTIONS
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<std::string> txs_hashes;
      bool decode_as_json;
      bool prune;
      bool split;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(txs_hashes)
        KV_SERIALIZE(decode_as_json)
        KV_SERIALIZE_OPT(prune, false)
        KV_SERIALIZE_OPT(split, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct entry
    {
      std::string tx_hash;
      std::string as_hex;
      std::string pruned_as_hex;
      std::string prunable_as_hex;
      std::string prunable_hash;
      std::string as_json;
      bool in_pool;
      bool double_spend_seen;
      uint64_t block_height;
      uint64_t confirmations;
      uint64_t block_timestamp;
      uint64_t received_timestamp;
      std::vector<uint64_t> output_indices;
      bool relayed;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(as_hex)
        KV_SERIALIZE(pruned_as_hex)
        KV_SERIALIZE(prunable_as_hex)
        KV_SERIALIZE(prunable_hash)
        KV_SERIALIZE(as_json)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(double_spend_seen)
        if (!this_ref.in_pool)
        {
          KV_SERIALIZE(block_height)
          KV_SERIALIZE(confirmations)
          KV_SERIALIZE(block_timestamp)
          KV_SERIALIZE(output_indices)
        }
        else
        {
          KV_SERIALIZE(relayed)
          KV_SERIALIZE(received_timestamp)
        }
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      // older compatibility stuff
      std::vector<std::string> txs_as_hex;  //transactions blobs as hex (old compat)
      std::vector<std::string> txs_as_json; //transactions decoded as json (old compat)

      // in both old and new
      std::vector<std::string> missed_tx;   //not found transactions

      // new style
      std::vector<entry> txs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(txs_as_hex)
        KV_SERIALIZE(txs_as_json)
        KV_SERIALIZE(txs)
        KV_SERIALIZE(missed_tx)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //-----------------------------------------------
  struct COMMAND_RPC_IS_KEY_IMAGE_SPENT
  {
    enum STATUS {
      UNSPENT = 0,
      SPENT_IN_BLOCKCHAIN = 1,
      SPENT_IN_POOL = 2,
    };

    struct request_t: public rpc_access_request_base
    {
      std::vector<std::string> key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(key_images)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_access_response_base
    {
      std::vector<int> spent_status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(spent_status)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES
  {
    struct request_t: public rpc_access_request_base
    {
      crypto::hash txid;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE_VAL_POD_AS_BLOB(txid)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_access_response_base
    {
      std::vector<uint64_t> o_indexes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(o_indexes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct get_outputs_out
  {
    uint64_t amount;
    uint64_t index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(index)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_OUTPUTS_BIN
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<get_outputs_out> outputs;
      bool get_txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(outputs)
        KV_SERIALIZE_OPT(get_txid, true)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct outkey
    {
      crypto::public_key key;
      rct::key mask;
      bool unlocked;
      uint64_t height;
      crypto::hash txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(key)
        KV_SERIALIZE_VAL_POD_AS_BLOB(mask)
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(height)
        KV_SERIALIZE_VAL_POD_AS_BLOB(txid)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      std::vector<outkey> outs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(outs)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_OUTPUTS
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<get_outputs_out> outputs;
      bool get_txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(outputs)
        KV_SERIALIZE(get_txid)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct outkey
    {
      std::string key;
      std::string mask;
      bool unlocked;
      uint64_t height;
      std::string txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
        KV_SERIALIZE(mask)
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(height)
        KV_SERIALIZE(txid)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      std::vector<outkey> outs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(outs)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_PATH_BY_AMOUNT_OUTPUT_ID_BIN
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<get_outputs_out> outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(outputs)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<fcmp_pp::curve_trees::PathBytes> paths;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(paths)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_SEND_RAW_TX
  {
    struct request_t: public rpc_access_request_base
    {
      std::string tx_as_hex;
      bool do_not_relay;
      bool do_sanity_checks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(tx_as_hex)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(do_sanity_checks, true)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_access_response_base
    {
      std::string reason;
      bool not_relayed;
      bool low_mixin;
      bool double_spend;
      bool invalid_input;
      bool invalid_output;
      bool too_big;
      bool overspend;
      bool fee_too_low;
      bool too_few_outputs;
      bool sanity_check_failed;
      bool tx_extra_too_big;
      bool nonzero_unlock_time;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(reason)
        KV_SERIALIZE(not_relayed)
        KV_SERIALIZE(low_mixin)
        KV_SERIALIZE(double_spend)
        KV_SERIALIZE(invalid_input)
        KV_SERIALIZE(invalid_output)
        KV_SERIALIZE(too_big)
        KV_SERIALIZE(overspend)
        KV_SERIALIZE(fee_too_low)
        KV_SERIALIZE(too_few_outputs)
        KV_SERIALIZE(sanity_check_failed)
        KV_SERIALIZE(tx_extra_too_big)
        KV_SERIALIZE(nonzero_unlock_time)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_START_MINING
  {
    struct request_t: public rpc_request_base
    {
      std::string miner_address;
      uint64_t    threads_count;
      bool        do_background_mining;
      bool        ignore_battery;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(miner_address)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)        
        KV_SERIALIZE(ignore_battery)        
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_INFO
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      uint64_t height;
      uint64_t target_height;
      uint64_t difficulty;
      std::string wide_difficulty;
      uint64_t difficulty_top64;
      uint64_t target;
      uint64_t tx_count;
      uint64_t tx_pool_size;
      uint64_t alt_blocks_count;
      uint64_t outgoing_connections_count;
      uint64_t incoming_connections_count;
      uint64_t rpc_connections_count;
      uint64_t white_peerlist_size;
      uint64_t grey_peerlist_size;
      bool mainnet;
      bool testnet;
      bool stagenet;
      std::string nettype;
      std::string top_block_hash;
      uint64_t cumulative_difficulty;
      std::string wide_cumulative_difficulty;
      uint64_t cumulative_difficulty_top64;
      uint64_t block_size_limit;
      uint64_t block_weight_limit;
      uint64_t block_size_median;
      uint64_t block_weight_median;
      uint64_t adjusted_time;
      uint64_t start_time;
      uint64_t free_space;
      bool offline;
      std::string bootstrap_daemon_address;
      uint64_t height_without_bootstrap;
      bool was_bootstrap_ever_used;
      uint64_t database_size;
      bool update_available;
      bool busy_syncing;
      std::string version;
      bool synchronized;
      bool restricted;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(wide_difficulty)
        KV_SERIALIZE(difficulty_top64)
        KV_SERIALIZE(target)
        KV_SERIALIZE(tx_count)
        KV_SERIALIZE(tx_pool_size)
        KV_SERIALIZE(alt_blocks_count)
        KV_SERIALIZE(outgoing_connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(rpc_connections_count)
        KV_SERIALIZE(white_peerlist_size)
        KV_SERIALIZE(grey_peerlist_size)
        KV_SERIALIZE(mainnet)
        KV_SERIALIZE(testnet)
        KV_SERIALIZE(stagenet)
        KV_SERIALIZE(nettype)
        KV_SERIALIZE(top_block_hash)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE(wide_cumulative_difficulty)
        KV_SERIALIZE(cumulative_difficulty_top64)
        KV_SERIALIZE(block_size_limit)
        KV_SERIALIZE_OPT(block_weight_limit, (uint64_t)0)
        KV_SERIALIZE(block_size_median)
        KV_SERIALIZE_OPT(block_weight_median, (uint64_t)0)
        KV_SERIALIZE(adjusted_time)
        KV_SERIALIZE(start_time)
        KV_SERIALIZE(free_space)
        KV_SERIALIZE(offline)
        KV_SERIALIZE(bootstrap_daemon_address)
        KV_SERIALIZE(height_without_bootstrap)
        KV_SERIALIZE(was_bootstrap_ever_used)
        KV_SERIALIZE(database_size)
        KV_SERIALIZE(update_available)
        KV_SERIALIZE(busy_syncing)
        KV_SERIALIZE(version)
        KV_SERIALIZE(synchronized)
        KV_SERIALIZE(restricted)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

    
  //-----------------------------------------------
  struct COMMAND_RPC_GET_NET_STATS
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_response_base
    {
      uint64_t start_time;
      uint64_t total_packets_in;
      uint64_t total_bytes_in;
      uint64_t total_packets_out;
      uint64_t total_bytes_out;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(start_time)
        KV_SERIALIZE(total_packets_in)
        KV_SERIALIZE(total_bytes_in)
        KV_SERIALIZE(total_packets_out)
        KV_SERIALIZE(total_bytes_out)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //-----------------------------------------------
  struct COMMAND_RPC_STOP_MINING
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //-----------------------------------------------
  struct COMMAND_RPC_MINING_STATUS
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_response_base
    {
      bool active;
      uint64_t speed;
      uint32_t threads_count;
      std::string address;
      std::string pow_algorithm;
      bool is_background_mining_enabled;
      uint8_t bg_idle_threshold;
      uint8_t bg_min_idle_seconds;
      bool bg_ignore_battery;
      uint8_t bg_target;
      uint32_t block_target;
      uint64_t block_reward;
      uint64_t difficulty;
      std::string wide_difficulty;
      uint64_t difficulty_top64;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(active)
        KV_SERIALIZE(speed)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(address)
        KV_SERIALIZE(pow_algorithm)
        KV_SERIALIZE(is_background_mining_enabled)
        KV_SERIALIZE(bg_idle_threshold)
        KV_SERIALIZE(bg_min_idle_seconds)
        KV_SERIALIZE(bg_ignore_battery)
        KV_SERIALIZE(bg_target)
        KV_SERIALIZE(block_target)
        KV_SERIALIZE(block_reward)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(wide_difficulty)
        KV_SERIALIZE(difficulty_top64)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //-----------------------------------------------
  struct COMMAND_RPC_SAVE_BC
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;


    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  //
  struct COMMAND_RPC_GETBLOCKCOUNT
  {
    typedef std::list<std::string> request;

    struct response_t: public rpc_response_base
    {
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(count)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GETBLOCKHASH
  {
    typedef std::vector<uint64_t> request;

    typedef std::string response;
  };


  struct COMMAND_RPC_GETBLOCKTEMPLATE
  {
    struct request_t: public rpc_request_base
    {
      uint64_t reserve_size;       //max 255 bytes
      std::string wallet_address;
      std::string prev_block;
      std::string extra_nonce;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(reserve_size)
        KV_SERIALIZE(wallet_address)
        KV_SERIALIZE(prev_block)
        KV_SERIALIZE(extra_nonce)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint64_t difficulty;
      std::string wide_difficulty;
      uint64_t difficulty_top64;
      uint64_t height;
      uint64_t reserved_offset;
      uint64_t expected_reward;
      uint64_t cumulative_weight;
      std::string prev_hash;
      uint64_t seed_height;
      std::string seed_hash;
      std::string next_seed_hash;
      blobdata blocktemplate_blob;
      blobdata blockhashing_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(wide_difficulty)
        KV_SERIALIZE(difficulty_top64)
        KV_SERIALIZE(height)
        KV_SERIALIZE(reserved_offset)
        KV_SERIALIZE(expected_reward)
        KV_SERIALIZE_OPT(cumulative_weight, static_cast<uint64_t>(0))
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(seed_height)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(blockhashing_blob)
        KV_SERIALIZE(seed_hash)
        KV_SERIALIZE(next_seed_hash)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GETMINERDATA
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint8_t major_version;
      uint64_t height;
      std::string prev_id;
      uint8_t fcmp_pp_n_tree_layers;
      std::string fcmp_pp_tree_root;
      std::string seed_hash;
      std::string difficulty;
      uint64_t median_weight;
      uint64_t already_generated_coins;

      struct tx_backlog_entry
      {
        std::string id;
        uint64_t weight;
        uint64_t fee;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(id)
          KV_SERIALIZE(weight)
          KV_SERIALIZE(fee)
        END_KV_SERIALIZE_MAP()
      };

      std::vector<tx_backlog_entry> tx_backlog;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(height)
        KV_SERIALIZE(prev_id)
        if (major_version >= HF_VERSION_FCMP_PLUS_PLUS)
        {
          KV_SERIALIZE(fcmp_pp_n_tree_layers)
          KV_SERIALIZE(fcmp_pp_tree_root)
        }
        KV_SERIALIZE(seed_hash)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(median_weight)
        KV_SERIALIZE(already_generated_coins)
        KV_SERIALIZE(tx_backlog)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_CALCPOW
  {
    struct request_t: public rpc_request_base
    {
      uint8_t major_version;
      uint64_t height;
      blobdata block_blob;
      std::string seed_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(height)
        KV_SERIALIZE(block_blob)
        KV_SERIALIZE(seed_hash)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    typedef std::string response;
  };

  struct COMMAND_RPC_ADD_AUX_POW
  {
    struct aux_pow_t
    {
      std::string id;
      std::string hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
        KV_SERIALIZE(hash)
      END_KV_SERIALIZE_MAP()
    };

    struct request_t: public rpc_request_base
    {
      blobdata blocktemplate_blob;
      std::vector<aux_pow_t> aux_pow;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(aux_pow)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      blobdata blocktemplate_blob;
      blobdata blockhashing_blob;
      std::string merkle_root;
      uint64_t merkle_tree_depth;
      std::vector<aux_pow_t> aux_pow;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(blockhashing_blob)
        KV_SERIALIZE(merkle_root)
        KV_SERIALIZE(merkle_tree_depth)
        KV_SERIALIZE(aux_pow)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SUBMITBLOCK
  {
    typedef std::vector<std::string> request;
    
    struct response_t: public rpc_response_base
    {
      std::string block_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(block_id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GENERATEBLOCKS
  {
    struct request_t: public rpc_request_base
    {
      uint64_t amount_of_blocks;
      std::string wallet_address;
      std::string prev_block;
      uint32_t starting_nonce;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(amount_of_blocks)
        KV_SERIALIZE(wallet_address)
        KV_SERIALIZE(prev_block)
        KV_SERIALIZE_OPT(starting_nonce, (uint32_t)0)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
    
    struct response_t: public rpc_response_base
    {
      uint64_t height;
      std::vector<std::string> blocks;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE(blocks)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  struct block_header_response
  {
      uint8_t major_version;
      uint8_t minor_version;
      uint64_t timestamp;
      std::string prev_hash;
      uint32_t nonce;
      bool orphan_status;
      uint64_t height;
      uint64_t depth;
      std::string hash;
      uint64_t difficulty;
      std::string wide_difficulty;
      uint64_t difficulty_top64;
      uint64_t cumulative_difficulty;
      std::string wide_cumulative_difficulty;
      uint64_t cumulative_difficulty_top64;
      uint64_t reward;
      uint64_t block_size;
      uint64_t block_weight;
      uint64_t num_txes;
      std::string pow_hash;
      uint64_t long_term_weight;
      std::string miner_tx_hash;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(major_version)
        KV_SERIALIZE(minor_version)
        KV_SERIALIZE(timestamp)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(orphan_status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(depth)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(wide_difficulty)
        KV_SERIALIZE(difficulty_top64)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE(wide_cumulative_difficulty)
        KV_SERIALIZE(cumulative_difficulty_top64)
        KV_SERIALIZE(reward)
        KV_SERIALIZE(block_size)
        KV_SERIALIZE_OPT(block_weight, (uint64_t)0)
        KV_SERIALIZE(num_txes)
        KV_SERIALIZE(pow_hash)
        KV_SERIALIZE_OPT(long_term_weight, (uint64_t)0)
        KV_SERIALIZE(miner_tx_hash)
      END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_LAST_BLOCK_HEADER
  {
    struct request_t: public rpc_access_request_base
    {
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE_OPT(fill_pow_hash, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(block_header)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;

  };
  
  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH
  {
    struct request_t: public rpc_access_request_base
    {
      std::string hash;
      std::vector<std::string> hashes;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(hashes)
        KV_SERIALIZE_OPT(fill_pow_hash, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      block_header_response block_header;
      std::vector<block_header_response> block_headers;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(block_headers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT
  {
    struct request_t: public rpc_access_request_base
    {
      uint64_t height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE_OPT(fill_pow_hash, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(block_header)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BLOCK
  {
    struct request_t: public rpc_access_request_base
    {
      std::string hash;
      uint64_t height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(height)
        KV_SERIALIZE_OPT(fill_pow_hash, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      block_header_response block_header;
      std::string miner_tx_hash;
      std::vector<std::string> tx_hashes;
      std::string blob;
      std::string json;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(miner_tx_hash)
        KV_SERIALIZE(tx_hashes)
        KV_SERIALIZE(blob)
        KV_SERIALIZE(json)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct peer {
    uint64_t id;
    std::string host;
    uint32_t ip;
    uint16_t port;
    uint16_t rpc_port;
    uint32_t rpc_credits_per_hash;
    uint64_t last_seen;
    uint32_t pruning_seed;

    peer() = default;

    peer(uint64_t id, const std::string &host, uint64_t last_seen, uint32_t pruning_seed, uint16_t rpc_port, uint32_t rpc_credits_per_hash)
      : id(id), host(host), ip(0), port(0), rpc_port(rpc_port), rpc_credits_per_hash(rpc_credits_per_hash), last_seen(last_seen), pruning_seed(pruning_seed)
    {}
    peer(uint64_t id, const std::string &host, uint16_t port, uint64_t last_seen, uint32_t pruning_seed, uint16_t rpc_port, uint32_t rpc_credits_per_hash)
      : id(id), host(host), ip(0), port(port), rpc_port(rpc_port), rpc_credits_per_hash(rpc_credits_per_hash), last_seen(last_seen), pruning_seed(pruning_seed)
    {}
    peer(uint64_t id, uint32_t ip, uint16_t port, uint64_t last_seen, uint32_t pruning_seed, uint16_t rpc_port, uint32_t rpc_credits_per_hash)
      : id(id), host(epee::string_tools::get_ip_string_from_int32(ip)), ip(ip), port(port), rpc_port(rpc_port), rpc_credits_per_hash(rpc_credits_per_hash), last_seen(last_seen), pruning_seed(pruning_seed)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id)
      KV_SERIALIZE(host)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
      KV_SERIALIZE_OPT(rpc_port, (uint16_t)0)
      KV_SERIALIZE_OPT(rpc_credits_per_hash, (uint32_t)0)
      KV_SERIALIZE(last_seen)
      KV_SERIALIZE_OPT(pruning_seed, (uint32_t)0)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PEER_LIST
  {
    struct request_t: public rpc_request_base
    {
      bool public_only;
      bool include_blocked;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(public_only, true)
        KV_SERIALIZE_OPT(include_blocked, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::vector<peer> white_list;
      std::vector<peer> gray_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(white_list)
        KV_SERIALIZE(gray_list)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct public_node
  {
    std::string host;
    uint64_t last_seen;
    uint16_t rpc_port;
    uint32_t rpc_credits_per_hash;

    public_node(): last_seen(0), rpc_port(0), rpc_credits_per_hash(0) {}

    public_node(const peer &peer)
      : host(peer.host), last_seen(peer.last_seen), rpc_port(peer.rpc_port), rpc_credits_per_hash(peer.rpc_credits_per_hash)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(host)
      KV_SERIALIZE(last_seen)
      KV_SERIALIZE(rpc_port)
      KV_SERIALIZE(rpc_credits_per_hash)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PUBLIC_NODES
  {
    struct request_t: public rpc_request_base
    {
      bool gray;
      bool white;
      bool include_blocked;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(gray, false)
        KV_SERIALIZE_OPT(white, true)
        KV_SERIALIZE_OPT(include_blocked, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::vector<public_node> gray;
      std::vector<public_node> white;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(gray)
        KV_SERIALIZE(white)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SET_LOG_HASH_RATE
  {
    struct request_t: public rpc_request_base
    {
      bool visible;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(visible)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SET_LOG_LEVEL
  {
    struct request_t: public rpc_request_base
    {
      int8_t level;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(level)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SET_LOG_CATEGORIES
  {
    struct request_t: public rpc_request_base
    {
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct tx_info
  {
    std::string id_hash;
    std::string tx_json; // TODO - expose this data directly
    uint64_t blob_size;
    uint64_t weight;
    uint64_t fee;
    std::string max_used_block_id_hash;
    uint64_t max_used_block_height;
    bool kept_by_block;
    uint64_t last_failed_height;
    std::string last_failed_id_hash;
    uint64_t receive_time;
    bool relayed;
    uint64_t last_relayed_time;
    bool do_not_relay;
    bool double_spend_seen;
    std::string tx_blob;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id_hash)
      KV_SERIALIZE(tx_json)
      KV_SERIALIZE(blob_size)
      KV_SERIALIZE_OPT(weight, (uint64_t)0)
      KV_SERIALIZE(fee)
      KV_SERIALIZE(max_used_block_id_hash)
      KV_SERIALIZE(max_used_block_height)
      KV_SERIALIZE(kept_by_block)
      KV_SERIALIZE(last_failed_height)
      KV_SERIALIZE(last_failed_id_hash)
      KV_SERIALIZE(receive_time)
      KV_SERIALIZE(relayed)
      KV_SERIALIZE(last_relayed_time)
      KV_SERIALIZE(do_not_relay)
      KV_SERIALIZE(double_spend_seen)
      KV_SERIALIZE(tx_blob)
    END_KV_SERIALIZE_MAP()
  };

  struct spent_key_image_info
  {
    std::string id_hash;
    std::vector<std::string> txs_hashes;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id_hash)
      KV_SERIALIZE(txs_hashes)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<tx_info> transactions;
      std::vector<spent_key_image_info> spent_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(transactions)
        KV_SERIALIZE(spent_key_images)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_HASHES_BIN
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<crypto::hash> tx_hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(tx_hashes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_HASHES
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<std::string> tx_hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(tx_hashes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct tx_backlog_entry
  {
    uint64_t weight;
    uint64_t fee;
    uint64_t time_in_pool;
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<tx_backlog_entry> backlog;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(backlog)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct txpool_histo
  {
    uint32_t txs;
    uint64_t bytes;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(txs)
      KV_SERIALIZE(bytes)
    END_KV_SERIALIZE_MAP()
  };

  struct txpool_stats
  {
    uint64_t bytes_total;
    uint32_t bytes_min;
    uint32_t bytes_max;
    uint32_t bytes_med;
    uint64_t fee_total;
    uint64_t oldest;
    uint32_t txs_total;
    uint32_t num_failing;
    uint32_t num_10m;
    uint32_t num_not_relayed;
    uint64_t histo_98pc;
    std::vector<txpool_histo> histo;
    uint32_t num_double_spends;

    txpool_stats(): bytes_total(0), bytes_min(0), bytes_max(0), bytes_med(0), fee_total(0), oldest(0), txs_total(0), num_failing(0), num_10m(0), num_not_relayed(0), histo_98pc(0), num_double_spends(0) {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(bytes_total)
      KV_SERIALIZE(bytes_min)
      KV_SERIALIZE(bytes_max)
      KV_SERIALIZE(bytes_med)
      KV_SERIALIZE(fee_total)
      KV_SERIALIZE(oldest)
      KV_SERIALIZE(txs_total)
      KV_SERIALIZE(num_failing)
      KV_SERIALIZE(num_10m)
      KV_SERIALIZE(num_not_relayed)
      KV_SERIALIZE(histo_98pc)
      KV_SERIALIZE(histo)
      KV_SERIALIZE(num_double_spends)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_STATS
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      txpool_stats pool_stats;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(pool_stats)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_CONNECTIONS
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::list<connection_info> connections;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(connections)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BLOCK_HEADERS_RANGE
  {
    struct request_t: public rpc_access_request_base
    {
      uint64_t start_height;
      uint64_t end_height;
      bool fill_pow_hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(end_height)
        KV_SERIALIZE_OPT(fill_pow_hash, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::vector<block_header_response> headers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(headers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SET_BOOTSTRAP_DAEMON
  {
    struct request_t
    {
      std::string address;
      std::string username;
      std::string password;
      std::string proxy;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(username)
        KV_SERIALIZE(password)
        KV_SERIALIZE(proxy)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_STOP_DAEMON
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  struct COMMAND_RPC_GET_LIMIT
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
    
    struct response_t: public rpc_response_base
    {
      uint64_t limit_up;
      uint64_t limit_down;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  struct COMMAND_RPC_SET_LIMIT
  {
    struct request_t: public rpc_request_base
    {
      int64_t limit_down;  // all limits (for get and set) are kB/s
      int64_t limit_up;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(limit_down)
        KV_SERIALIZE(limit_up)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
    
    struct response_t: public rpc_response_base
    {
      int64_t limit_up;
      int64_t limit_down;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  struct COMMAND_RPC_OUT_PEERS
  {
    struct request_t: public rpc_request_base
    {
      bool set;
      uint32_t out_peers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(set, true)
        KV_SERIALIZE(out_peers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;
    
    struct response_t: public rpc_response_base
    {
      uint32_t out_peers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(out_peers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_IN_PEERS
  {
    struct request_t: public rpc_request_base
    {
      bool set;
      uint32_t in_peers;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(set, true)
        KV_SERIALIZE(in_peers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint32_t in_peers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(in_peers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
    
  struct COMMAND_RPC_HARD_FORK_INFO
  {
    struct request_t: public rpc_access_request_base
    {
      uint8_t version;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      uint8_t version;
      bool enabled;
      uint32_t window;
      uint32_t votes;
      uint32_t threshold;
      uint8_t voting;
      uint32_t state;
      uint64_t earliest_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(version)
        KV_SERIALIZE(enabled)
        KV_SERIALIZE(window)
        KV_SERIALIZE(votes)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(voting)
        KV_SERIALIZE(state)
        KV_SERIALIZE(earliest_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GETBANS
  {
    struct ban
    {
      std::string host;
      uint32_t ip;
      uint32_t seconds;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(host)
        KV_SERIALIZE(ip)
        KV_SERIALIZE(seconds)
      END_KV_SERIALIZE_MAP()
    };

    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SETBANS
  {
    struct ban
    {
      std::string host;
      uint32_t ip;
      bool ban;
      uint32_t seconds;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(host)
        KV_SERIALIZE(ip)
        KV_SERIALIZE(ban)
        KV_SERIALIZE(seconds)
      END_KV_SERIALIZE_MAP()
    };

    struct request_t: public rpc_request_base
    {
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_BANNED
  {
    struct request_t
    {
      std::string address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string status;
      bool banned;
      uint32_t seconds;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(banned)
        KV_SERIALIZE(seconds)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_FLUSH_TRANSACTION_POOL
  {
    struct request_t: public rpc_request_base
    {
      std::vector<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_OUTPUT_HISTOGRAM
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<uint64_t> amounts;
      uint64_t min_count;
      uint64_t max_count;
      bool unlocked;
      uint64_t recent_cutoff;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(amounts)
        KV_SERIALIZE(min_count)
        KV_SERIALIZE(max_count)
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(recent_cutoff)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct entry
    {
      uint64_t amount;
      uint64_t total_instances;
      uint64_t unlocked_instances;
      uint64_t recent_instances;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE(total_instances)
        KV_SERIALIZE(unlocked_instances)
        KV_SERIALIZE(recent_instances)
      END_KV_SERIALIZE_MAP()

      entry(uint64_t amount, uint64_t total_instances, uint64_t unlocked_instances, uint64_t recent_instances):
          amount(amount), total_instances(total_instances), unlocked_instances(unlocked_instances), recent_instances(recent_instances) {}
      entry() {}
    };

    struct response_t: public rpc_access_response_base
    {
      std::vector<entry> histogram;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(histogram)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_VERSION
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct hf_entry
    {
      uint8_t hf_version;
      uint64_t height;

      bool operator==(const hf_entry& hfe) const { return hf_version == hfe.hf_version && height == hfe.height; }

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hf_version)
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_response_base
    {
      uint32_t version;
      bool release;
      uint64_t current_height;
      uint64_t target_height;
      std::vector<hf_entry> hard_forks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(version)
        KV_SERIALIZE(release)
        KV_SERIALIZE_OPT(current_height, (uint64_t)0)
        KV_SERIALIZE_OPT(target_height, (uint64_t)0)
        KV_SERIALIZE_OPT(hard_forks, std::vector<hf_entry>())
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_COINBASE_TX_SUM
  {
    struct request_t: public rpc_access_request_base
    {
      uint64_t height;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE(count)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      uint64_t emission_amount;
      std::string wide_emission_amount;
      uint64_t emission_amount_top64;
      uint64_t fee_amount;
      std::string wide_fee_amount;
      uint64_t fee_amount_top64;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(emission_amount)
        KV_SERIALIZE(wide_emission_amount)
        KV_SERIALIZE(emission_amount_top64)
        KV_SERIALIZE(fee_amount)
        KV_SERIALIZE(wide_fee_amount)
        KV_SERIALIZE(fee_amount_top64)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_BASE_FEE_ESTIMATE
  {
    struct request_t: public rpc_access_request_base
    {
      uint64_t grace_blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(grace_blocks)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      uint64_t fee;
      uint64_t quantization_mask;
      std::vector<uint64_t> fees;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(fee)
        KV_SERIALIZE_OPT(quantization_mask, (uint64_t)1)
        KV_SERIALIZE(fees)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_ALTERNATE_CHAINS
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct chain_info
    {
      std::string block_hash;
      uint64_t height;
      uint64_t length;
      uint64_t difficulty;
      std::string wide_difficulty;
      uint64_t difficulty_top64;
      std::vector<std::string> block_hashes;
      std::string main_chain_parent_block;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_hash)
        KV_SERIALIZE(height)
        KV_SERIALIZE(length)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(wide_difficulty)
        KV_SERIALIZE(difficulty_top64)
        KV_SERIALIZE(block_hashes)
        KV_SERIALIZE(main_chain_parent_block)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_response_base
    {
      std::vector<chain_info> chains;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(chains)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_UPDATE
  {
    struct request_t: public rpc_request_base
    {
      std::string command;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(command)
        KV_SERIALIZE(path)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      bool update;
      std::string version;
      std::string user_uri;
      std::string auto_uri;
      std::string hash;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(update)
        KV_SERIALIZE(version)
        KV_SERIALIZE(user_uri)
        KV_SERIALIZE(auto_uri)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(path)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_RELAY_TX
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_SYNC_INFO
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct peer
    {
      connection_info info;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };

    struct span
    {
      uint64_t start_block_height;
      uint64_t nblocks;
      std::string connection_id;
      uint32_t rate;
      uint32_t speed;
      uint64_t size;
      std::string remote_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_block_height)
        KV_SERIALIZE(nblocks)
        KV_SERIALIZE(connection_id)
        KV_SERIALIZE(rate)
        KV_SERIALIZE(speed)
        KV_SERIALIZE(size)
        KV_SERIALIZE(remote_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      uint64_t height;
      uint64_t target_height;
      uint32_t next_needed_pruning_seed;
      std::list<peer> peers;
      std::list<span> spans;
      std::string overview;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(next_needed_pruning_seed)
        KV_SERIALIZE(peers)
        KV_SERIALIZE(spans)
        KV_SERIALIZE(overview)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_OUTPUT_DISTRIBUTION
  {
    struct request_t: public rpc_access_request_base
    {
      std::vector<uint64_t> amounts;
      uint64_t from_height;
      uint64_t to_height;
      bool cumulative;
      bool binary;
      bool compress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(amounts)
        KV_SERIALIZE_OPT(from_height, (uint64_t)0)
        KV_SERIALIZE_OPT(to_height, (uint64_t)0)
        KV_SERIALIZE_OPT(cumulative, false)
        KV_SERIALIZE_OPT(binary, true)
        KV_SERIALIZE_OPT(compress, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct distribution
    {
      rpc::output_distribution_data data;
      uint64_t amount;
      std::string compressed_data;
      bool binary;
      bool compress;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE_N(data.start_height, "start_height")
        KV_SERIALIZE(binary)
        KV_SERIALIZE(compress)
        if (this_ref.binary)
        {
          if (is_store)
          {
            if (this_ref.compress)
            {
              const_cast<std::string&>(this_ref.compressed_data) = compress_integer_array(this_ref.data.distribution);
              KV_SERIALIZE(compressed_data)
            }
            else
              KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(data.distribution, "distribution")
          }
          else
          {
            if (this_ref.compress)
            {
              KV_SERIALIZE(compressed_data)
              const_cast<std::vector<uint64_t>&>(this_ref.data.distribution) = decompress_integer_array<uint64_t>(this_ref.compressed_data);
            }
            else
              KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(data.distribution, "distribution")
          }
        }
        else
          KV_SERIALIZE_N(data.distribution, "distribution")
        KV_SERIALIZE_N(data.base, "base")
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_access_response_base
    {
      std::vector<distribution> distributions;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(distributions)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_INFO
  {
    struct request_t: public rpc_access_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      std::string hashing_blob;
      uint64_t seed_height;
      std::string seed_hash;
      std::string next_seed_hash;
      uint32_t cookie;
      uint64_t diff;
      uint64_t credits_per_hash_found;
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
        KV_SERIALIZE(hashing_blob)
        KV_SERIALIZE(seed_height)
        KV_SERIALIZE(seed_hash)
        KV_SERIALIZE(next_seed_hash)
        KV_SERIALIZE(cookie)
        KV_SERIALIZE(diff)
        KV_SERIALIZE(credits_per_hash_found)
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_SUBMIT_NONCE
  {
    struct request_t: public rpc_access_request_base
    {
      uint32_t nonce;
      uint32_t cookie;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(nonce)
        KV_SERIALIZE(cookie)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_PAY
  {
    struct request_t: public rpc_access_request_base
    {
      std::string paying_for;
      uint64_t payment;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_request_base)
        KV_SERIALIZE(paying_for)
        KV_SERIALIZE(payment)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_access_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_access_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_TRACKING
  {
    struct request_t: public rpc_request_base
    {
      bool clear;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(clear)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct entry
    {
      std::string rpc;
      uint64_t count;
      uint64_t time;
      uint64_t credits;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(rpc)
        KV_SERIALIZE(count)
        KV_SERIALIZE(time)
        KV_SERIALIZE(credits)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_response_base
    {
      std::vector<entry> data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(data)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_DATA
  {
    struct request_t: public rpc_request_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct entry
    {
      std::string client;
      uint64_t balance;
      uint64_t last_update_time;
      uint64_t credits_total;
      uint64_t credits_used;
      uint64_t nonces_good;
      uint64_t nonces_stale;
      uint64_t nonces_bad;
      uint64_t nonces_dupe;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(client)
        KV_SERIALIZE(balance)
        KV_SERIALIZE(last_update_time)
        KV_SERIALIZE(credits_total)
        KV_SERIALIZE(credits_used)
        KV_SERIALIZE(nonces_good)
        KV_SERIALIZE(nonces_stale)
        KV_SERIALIZE(nonces_bad)
        KV_SERIALIZE(nonces_dupe)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t: public rpc_response_base
    {
      std::list<entry> entries;
      uint32_t hashrate;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(entries)
        KV_SERIALIZE(hashrate)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_ACCESS_ACCOUNT
  {
    struct request_t: public rpc_request_base
    {
      std::string client;
      int64_t delta_balance;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(client)
        KV_SERIALIZE_OPT(delta_balance, (int64_t)0)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint64_t credits;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(credits)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_POP_BLOCKS
  {
    struct request_t: public rpc_request_base
    {
      uint64_t nblocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(nblocks)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_PRUNE_BLOCKCHAIN
  {
    struct request_t: public rpc_request_base
    {
      bool check;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(check, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      bool pruned;
      uint32_t pruning_seed;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(pruned)
        KV_SERIALIZE(pruning_seed)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_FLUSH_CACHE
  {
    struct request_t: public rpc_request_base
    {
      bool bad_blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE_OPT(bad_blocks, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_GET_TXIDS_LOOSE
  {
    struct request_t: public rpc_request_base
    {
      std::string txid_template;
      std::uint32_t num_matching_bits;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_request_base)
        KV_SERIALIZE(txid_template)
        KV_SERIALIZE(num_matching_bits)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t: public rpc_response_base
    {
      std::vector<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_PARENT(rpc_response_base)
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

}
