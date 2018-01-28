// Copyright (c) 2014-2018, The Monero Project
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
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/difficulty.h"
#include "crypto/hash.h"

namespace cryptonote
{
  //-----------------------------------------------
#define CORE_RPC_STATUS_OK   "OK"
#define CORE_RPC_STATUS_BUSY   "BUSY"
#define CORE_RPC_STATUS_NOT_MINING "NOT MINING"

// When making *any* change here, bump minor
// If the change is incompatible, then bump major and set minor to 0
// This ensures CORE_RPC_VERSION always increases, that every change
// has its own version, and that clients can just test major to see
// whether they can talk to a given daemon without having to know in
// advance which version they will stop working with
// Don't go over 32767 for any of these
#define CORE_RPC_VERSION_MAJOR 1
#define CORE_RPC_VERSION_MINOR 17
#define MAKE_CORE_RPC_VERSION(major,minor) (((major)<<16)|(minor))
#define CORE_RPC_VERSION MAKE_CORE_RPC_VERSION(CORE_RPC_VERSION_MAJOR, CORE_RPC_VERSION_MINOR)

  struct COMMAND_RPC_GET_HEIGHT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BLOCKS_FAST
  {

    struct request
    {
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      bool        prune;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(prune)
      END_KV_SERIALIZE_MAP()
    };

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

    struct response
    {
      std::list<block_complete_entry> blocks;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;
      std::vector<block_output_indices> output_indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE(status)
        KV_SERIALIZE(output_indices)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BLOCKS_BY_HEIGHT
  {
    struct request
    {
      std::vector<uint64_t> heights;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(heights)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<block_complete_entry> blocks;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

    struct COMMAND_RPC_GET_ALT_BLOCKS_HASHES
    {
        struct request
        {
            BEGIN_KV_SERIALIZE_MAP()
            END_KV_SERIALIZE_MAP()
        };

        struct response
        {
            std::vector<std::string> blks_hashes;
            std::string status;

            BEGIN_KV_SERIALIZE_MAP()
                KV_SERIALIZE(blks_hashes)
                KV_SERIALIZE(status)
            END_KV_SERIALIZE_MAP()
        };
    };
  struct COMMAND_RPC_GET_HASHES_FAST
  {

    struct request
    {
      std::list<crypto::hash> block_ids; //*first 10 blocks id goes sequential, next goes in pow(2,n) offset, like 2, 4, 8, 16, 32, 64 and so on, and the last one is always genesis block */
      uint64_t    start_height;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(block_ids)
        KV_SERIALIZE(start_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<crypto::hash> m_block_ids;
      uint64_t    start_height;
      uint64_t    current_height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(m_block_ids)
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(current_height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_GET_ADDRESS_TXS
  {
      struct request
      {
        std::string address;
        std::string view_key;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };
      
      struct spent_output {
        uint64_t amount;
        std::string key_image;
        std::string tx_pub_key;
        uint64_t out_index;
        uint32_t mixin;
        
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(key_image)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(out_index)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };
      
      struct transaction
      {
        uint64_t id;
        std::string hash;
        uint64_t timestamp;
        uint64_t total_received;
        uint64_t total_sent;
        uint64_t unlock_time;
        uint64_t height;
        std::list<spent_output> spent_outputs;
        std::string payment_id;
        bool coinbase;
        bool mempool;
        uint32_t mixin;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(id)
          KV_SERIALIZE(hash)
          KV_SERIALIZE(timestamp)
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_sent)
          KV_SERIALIZE(unlock_time)
          KV_SERIALIZE(height)
          KV_SERIALIZE(spent_outputs)
          KV_SERIALIZE(payment_id)
          KV_SERIALIZE(coinbase)
          KV_SERIALIZE(mempool)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };
      
      
      struct response
      {
        //std::list<std::string> txs_as_json;
        uint64_t total_received;
        uint64_t total_received_unlocked = 0; // OpenMonero only
        uint64_t scanned_height;
        std::list<transaction> transactions;
        uint64_t blockchain_height;
        uint64_t scanned_block_height;
        std::string status;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_received_unlocked)
          KV_SERIALIZE(scanned_height)
          KV_SERIALIZE(transactions)
          KV_SERIALIZE(blockchain_height)
          KV_SERIALIZE(scanned_block_height)
          KV_SERIALIZE(status)
        END_KV_SERIALIZE_MAP()
      };
  };
  
  //-----------------------------------------------
  struct COMMAND_RPC_GET_ADDRESS_INFO
  {
      struct request
      {
        std::string address;
        std::string view_key;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };
      
      struct spent_output 
      {
        uint64_t amount;
        std::string key_image;
        std::string tx_pub_key;
        uint64_t  out_index;
        uint32_t  mixin;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(key_image)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(out_index)
          KV_SERIALIZE(mixin)
        END_KV_SERIALIZE_MAP()
      };
  
      
      
      struct response
      { 
        uint64_t locked_funds;
        uint64_t total_received;
        uint64_t total_sent;
        uint64_t scanned_height;
        uint64_t scanned_block_height;
        uint64_t start_height;
        uint64_t transaction_height;
        uint64_t blockchain_height;
        std::list<spent_output> spent_outputs;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(locked_funds)
          KV_SERIALIZE(total_received)
          KV_SERIALIZE(total_sent)        
          KV_SERIALIZE(scanned_height)
          KV_SERIALIZE(scanned_block_height)
          KV_SERIALIZE(start_height)
          KV_SERIALIZE(transaction_height)
          KV_SERIALIZE(blockchain_height)
          KV_SERIALIZE(spent_outputs)
        END_KV_SERIALIZE_MAP()
      };
  };
  
  //-----------------------------------------------
  struct COMMAND_RPC_GET_UNSPENT_OUTS
  {
      struct request
      {
        std::string amount;
        std::string address;
        std::string view_key;
        // OpenMonero specific
        uint64_t mixin;
        bool use_dust;
        std::string dust_threshold;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
          KV_SERIALIZE(mixin)
          KV_SERIALIZE(use_dust)
          KV_SERIALIZE(dust_threshold)
        END_KV_SERIALIZE_MAP()
      };
    
      
      struct output {
        uint64_t amount;
        std::string public_key;
        uint64_t  index;
        uint64_t global_index;
        std::string rct;
        std::string tx_hash;
        std::string tx_pub_key;
        std::string tx_prefix_hash;
        std::vector<std::string> spend_key_images;
        uint64_t timestamp;
        uint64_t height;                


        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(public_key)
          KV_SERIALIZE(index)
          KV_SERIALIZE(global_index)
          KV_SERIALIZE(rct)
          KV_SERIALIZE(tx_hash)
          KV_SERIALIZE(tx_pub_key)
          KV_SERIALIZE(tx_prefix_hash)
          KV_SERIALIZE(spend_key_images)  
          KV_SERIALIZE(timestamp)  
          KV_SERIALIZE(height)                                    
        END_KV_SERIALIZE_MAP()
      };
      
      struct response
      {
        uint64_t amount;
        std::list<output> outputs;
        uint64_t per_kb_fee;
        std::string status;
        std::string reason;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(outputs)
          KV_SERIALIZE(per_kb_fee)
          KV_SERIALIZE(status)
          KV_SERIALIZE(reason)
        END_KV_SERIALIZE_MAP()
      };
  };
  
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTS
  {
      struct request
      {
        std::vector<std::string> amounts;
        uint32_t count;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amounts)
          KV_SERIALIZE(count)
        END_KV_SERIALIZE_MAP()
      };
    
      
      struct output {
        std::string public_key;
        uint64_t global_index;
        std::string rct; // 64+64+64 characters long (<rct commit> + <encrypted mask> + <rct amount>)

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(public_key)
          KV_SERIALIZE(global_index)
          KV_SERIALIZE(rct)                                 
        END_KV_SERIALIZE_MAP()
      };
      
      struct amount_out {
        uint64_t amount;
        std::vector<output> outputs;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount)
          KV_SERIALIZE(outputs)                                
        END_KV_SERIALIZE_MAP()
        
      };
      
      struct response
      {
        std::vector<amount_out> amount_outs;
        std::string Error;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(amount_outs)
          KV_SERIALIZE(Error)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_SUBMIT_RAW_TX
  {
      struct request
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
    
      
      struct response
      {
        std::string status;
        std::string error;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_SERIALIZE(error)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_LOGIN
  {
      struct request
      {
        std::string address;
        std::string view_key;
        bool create_account;

        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key) 
          KV_SERIALIZE(create_account) 
        END_KV_SERIALIZE_MAP()
      };
    
      
      struct response
      {
        std::string status;
        std::string reason;
        bool new_address;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(status)
          KV_SERIALIZE(reason)
          KV_SERIALIZE(new_address)
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_IMPORT_WALLET_REQUEST
  {
      struct request
      {
        std::string address;
        std::string view_key;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(address)
          KV_SERIALIZE(view_key)
        END_KV_SERIALIZE_MAP()
      };
    
      
      struct response
      {
        std::string payment_id;
        uint64_t import_fee;
        bool new_request;
        bool request_fulfilled;
        std::string payment_address;
        std::string status;
        
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(payment_id)
          KV_SERIALIZE(import_fee)
          KV_SERIALIZE(new_request)
          KV_SERIALIZE(request_fulfilled)
          KV_SERIALIZE(payment_address)
          KV_SERIALIZE(status)            
        END_KV_SERIALIZE_MAP()
      };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_TRANSACTIONS
  {
    struct request
    {
      std::list<std::string> txs_hashes;
      bool decode_as_json;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_hashes)
        KV_SERIALIZE(decode_as_json)
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      std::string tx_hash;
      std::string as_hex;
      std::string as_json;
      bool in_pool;
      bool double_spend_seen;
      uint64_t block_height;
      uint64_t block_timestamp;
      std::vector<uint64_t> output_indices;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(as_hex)
        KV_SERIALIZE(as_json)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(double_spend_seen)
        KV_SERIALIZE(block_height)
        KV_SERIALIZE(block_timestamp)
        KV_SERIALIZE(output_indices)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      // older compatibility stuff
      std::list<std::string> txs_as_hex;  //transactions blobs as hex (old compat)
      std::list<std::string> txs_as_json; //transactions decoded as json (old compat)

      // in both old and new
      std::list<std::string> missed_tx;   //not found transactions

      // new style
      std::vector<entry> txs;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txs_as_hex)
        KV_SERIALIZE(txs_as_json)
        KV_SERIALIZE(txs)
        KV_SERIALIZE(missed_tx)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_IS_KEY_IMAGE_SPENT
  {
    enum STATUS {
      UNSPENT = 0,
      SPENT_IN_BLOCKCHAIN = 1,
      SPENT_IN_POOL = 2,
    };

    struct request
    {
      std::vector<std::string> key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_images)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<int> spent_status;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(spent_status)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_GET_TX_GLOBAL_OUTPUTS_INDEXES
  {
    struct request
    {
      crypto::hash txid;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_VAL_POD_AS_BLOB(txid)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::vector<uint64_t> o_indexes;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(o_indexes)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS
  {
    struct request
    {
      std::vector<uint64_t> amounts;
      uint64_t              outs_count;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts)
        KV_SERIALIZE(outs_count)
      END_KV_SERIALIZE_MAP()
    };

#pragma pack (push, 1)
    struct out_entry
    {
      uint64_t global_amount_index;
      crypto::public_key out_key;
    };
#pragma pack(pop)

    struct outs_for_amount
    {
      uint64_t amount;
      std::list<out_entry> outs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<outs_for_amount> outs;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
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
    struct request
    {
      std::vector<get_outputs_out> outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs)
      END_KV_SERIALIZE_MAP()
    };

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

    struct response
    {
      std::vector<outkey> outs;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_OUTPUTS
  {
    struct request
    {
      std::vector<get_outputs_out> outputs;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs)
      END_KV_SERIALIZE_MAP()
    };

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

    struct response
    {
      std::vector<outkey> outs;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS
  {
    struct request
    {
      uint64_t outs_count;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outs_count)
      END_KV_SERIALIZE_MAP()
    };

#pragma pack (push, 1)
    struct out_entry
    {
      uint64_t amount;
      uint64_t global_amount_index;
      crypto::public_key out_key;
      rct::key commitment;
    };
#pragma pack(pop)

    struct response
    {
      std::list<out_entry> outs;
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(outs)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_SEND_RAW_TX
  {
    struct request
    {
      std::string tx_as_hex;
      bool do_not_relay;

      request() {}
      explicit request(const transaction &);

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_as_hex)
        KV_SERIALIZE(do_not_relay)
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      std::string reason;
      bool not_relayed;
      bool low_mixin;
      bool double_spend;
      bool invalid_input;
      bool invalid_output;
      bool too_big;
      bool overspend;
      bool fee_too_low;
      bool not_rct;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(reason)
        KV_SERIALIZE(not_relayed)
        KV_SERIALIZE(low_mixin)
        KV_SERIALIZE(double_spend)
        KV_SERIALIZE(invalid_input)
        KV_SERIALIZE(invalid_output)
        KV_SERIALIZE(too_big)
        KV_SERIALIZE(overspend)
        KV_SERIALIZE(fee_too_low)
        KV_SERIALIZE(not_rct)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_START_MINING
  {
    struct request
    {
      std::string miner_address;
      uint64_t    threads_count;
      bool        do_background_mining;
      bool        ignore_battery;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(miner_address)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)        
        KV_SERIALIZE(ignore_battery)        
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  //-----------------------------------------------
  struct COMMAND_RPC_GET_INFO
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t height;
      uint64_t target_height;
      uint64_t difficulty;
      uint64_t target;
      uint64_t tx_count;
      uint64_t tx_pool_size;
      uint64_t alt_blocks_count;
      uint64_t outgoing_connections_count;
      uint64_t incoming_connections_count;
      uint64_t rpc_connections_count;
      uint64_t white_peerlist_size;
      uint64_t grey_peerlist_size;
      bool testnet;
      std::string top_block_hash;
      uint64_t cumulative_difficulty;
      uint64_t block_size_limit;
      uint64_t start_time;
      uint64_t free_space;
      bool offline;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(target)
        KV_SERIALIZE(tx_count)
        KV_SERIALIZE(tx_pool_size)
        KV_SERIALIZE(alt_blocks_count)
        KV_SERIALIZE(outgoing_connections_count)
        KV_SERIALIZE(incoming_connections_count)
        KV_SERIALIZE(rpc_connections_count)
        KV_SERIALIZE(white_peerlist_size)
        KV_SERIALIZE(grey_peerlist_size)
        KV_SERIALIZE(testnet)
        KV_SERIALIZE(top_block_hash)
        KV_SERIALIZE(cumulative_difficulty)
        KV_SERIALIZE(block_size_limit)
        KV_SERIALIZE(start_time)
        KV_SERIALIZE(free_space)
        KV_SERIALIZE(offline)
      END_KV_SERIALIZE_MAP()
    };
  };

    
  //-----------------------------------------------
  struct COMMAND_RPC_STOP_MINING
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_MINING_STATUS
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;
      bool active;
      uint64_t speed;
      uint32_t threads_count;
      std::string address;
      bool is_background_mining_enabled;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(active)
        KV_SERIALIZE(speed)
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(address)
        KV_SERIALIZE(is_background_mining_enabled)
      END_KV_SERIALIZE_MAP()
    };
  };

  //-----------------------------------------------
  struct COMMAND_RPC_SAVE_BC
  {
    struct request
    {

      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };


    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  //
  struct COMMAND_RPC_GETBLOCKCOUNT
  {
    typedef std::list<std::string> request;

    struct response
    {
      uint64_t count;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(count)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GETBLOCKHASH
  {
    typedef std::vector<uint64_t> request;

    typedef std::string response;
  };


  struct COMMAND_RPC_GETBLOCKTEMPLATE
  {
    struct request
    {
      uint64_t reserve_size;       //max 255 bytes
      std::string wallet_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(reserve_size)
        KV_SERIALIZE(wallet_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t difficulty;
      uint64_t height;
      uint64_t reserved_offset;
      uint64_t expected_reward;
      std::string prev_hash;
      blobdata blocktemplate_blob;
      blobdata blockhashing_blob;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(difficulty)
        KV_SERIALIZE(height)
        KV_SERIALIZE(reserved_offset)
        KV_SERIALIZE(expected_reward)
        KV_SERIALIZE(prev_hash)
        KV_SERIALIZE(blocktemplate_blob)
        KV_SERIALIZE(blockhashing_blob)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SUBMITBLOCK
  {
    typedef std::vector<std::string> request;
    
    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
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
      difficulty_type difficulty;
      uint64_t reward;
      uint64_t block_size;
      uint64_t num_txes;
      
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
        KV_SERIALIZE(reward)
        KV_SERIALIZE(block_size)
        KV_SERIALIZE(num_txes)
      END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_LAST_BLOCK_HEADER
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };
  
  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HASH
  {
    struct request
    {
      std::string hash;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT
  {
    struct request
    {
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GET_BLOCK
  {
    struct request
    {
      std::string hash;
      uint64_t height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hash)
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      block_header_response block_header;
      std::string miner_tx_hash;
      std::vector<std::string> tx_hashes;
      std::string blob;
      std::string json;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_header)
        KV_SERIALIZE(miner_tx_hash)
        KV_SERIALIZE(tx_hashes)
        KV_SERIALIZE(status)
        KV_SERIALIZE(blob)
        KV_SERIALIZE(json)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct peer {
    uint64_t id;
    std::string host;
    uint32_t ip;
    uint16_t port;
    uint64_t last_seen;

    peer() = default;

    peer(uint64_t id, const std::string &host, uint64_t last_seen)
      : id(id), host(host), ip(0), port(0), last_seen(last_seen)
    {}
    peer(uint64_t id, uint32_t ip, uint16_t port, uint64_t last_seen)
      : id(id), host(std::to_string(ip)), ip(ip), port(port), last_seen(last_seen)
    {}

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id)
      KV_SERIALIZE(host)
      KV_SERIALIZE(ip)
      KV_SERIALIZE(port)
      KV_SERIALIZE(last_seen)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PEER_LIST
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<peer> white_list;
      std::vector<peer> gray_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(white_list)
        KV_SERIALIZE(gray_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_LOG_HASH_RATE
  {
    struct request
    {
      bool visible;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(visible)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_LOG_LEVEL
  {
    struct request
    {
      int8_t level;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(level)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_LOG_CATEGORIES
  {
    struct request
    {
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::string categories;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(categories)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct tx_info
  {
    std::string id_hash;
    std::string tx_json; // TODO - expose this data directly
    uint64_t blob_size;
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
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<tx_info> transactions;
      std::vector<spent_key_image_info> spent_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(transactions)
        KV_SERIALIZE(spent_key_images)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_HASHES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<crypto::hash> tx_hashes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(tx_hashes)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct tx_backlog_entry
  {
    uint64_t blob_size;
    uint64_t fee;
    uint64_t time_in_pool;
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_BACKLOG
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<tx_backlog_entry> backlog;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE_CONTAINER_POD_AS_BLOB(backlog)
      END_KV_SERIALIZE_MAP()
    };
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
      KV_SERIALIZE_CONTAINER_POD_AS_BLOB(histo)
      KV_SERIALIZE(num_double_spends)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_TRANSACTION_POOL_STATS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      txpool_stats pool_stats;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(pool_stats)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_CONNECTIONS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<connection_info> connections;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(connections)
      END_KV_SERIALIZE_MAP()
    };

  };

  struct COMMAND_RPC_GET_BLOCK_HEADERS_RANGE
  {
    struct request
    {
      uint64_t start_height;
      uint64_t end_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(start_height)
        KV_SERIALIZE(end_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<block_header_response> headers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(headers)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STOP_DAEMON
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_FAST_EXIT
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
	  std::string status;
	  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_GET_LIMIT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
      std::string status;
      uint64_t limit_up;
      uint64_t limit_down;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_SET_LIMIT
  {
    struct request
    {
      int64_t limit_down;
      int64_t limit_up;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(limit_down)
        KV_SERIALIZE(limit_up)
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
      std::string status;
      int64_t limit_up;
      int64_t limit_down;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(limit_up)
        KV_SERIALIZE(limit_down)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_OUT_PEERS
  {
	struct request
    {
	  uint64_t out_peers;
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(out_peers)
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
	  std::string status;
	  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
    
  struct COMMAND_RPC_START_SAVE_GRAPH
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
	  std::string status;
	  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct COMMAND_RPC_STOP_SAVE_GRAPH
  {
	struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    
    struct response
    {
	  std::string status;
	  
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_HARD_FORK_INFO
  {
    struct request
    {
      uint8_t version;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint8_t version;
      bool enabled;
      uint32_t window;
      uint32_t votes;
      uint32_t threshold;
      uint8_t voting;
      uint32_t state;
      uint64_t earliest_height;
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
        KV_SERIALIZE(enabled)
        KV_SERIALIZE(window)
        KV_SERIALIZE(votes)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(voting)
        KV_SERIALIZE(state)
        KV_SERIALIZE(earliest_height)
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
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

    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };
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

    struct request
    {
      std::vector<ban> bans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(bans)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_FLUSH_TRANSACTION_POOL
  {
    struct request
    {
      std::list<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_OUTPUT_HISTOGRAM
  {
    struct request
    {
      std::vector<uint64_t> amounts;
      uint64_t min_count;
      uint64_t max_count;
      bool unlocked;
      uint64_t recent_cutoff;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amounts);
        KV_SERIALIZE(min_count);
        KV_SERIALIZE(max_count);
        KV_SERIALIZE(unlocked);
        KV_SERIALIZE(recent_cutoff);
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      uint64_t amount;
      uint64_t total_instances;
      uint64_t unlocked_instances;
      uint64_t recent_instances;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount);
        KV_SERIALIZE(total_instances);
        KV_SERIALIZE(unlocked_instances);
        KV_SERIALIZE(recent_instances);
      END_KV_SERIALIZE_MAP()

      entry(uint64_t amount, uint64_t total_instances, uint64_t unlocked_instances, uint64_t recent_instances):
          amount(amount), total_instances(total_instances), unlocked_instances(unlocked_instances), recent_instances(recent_instances) {}
      entry() {}
    };

    struct response
    {
      std::string status;
      std::vector<entry> histogram;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(histogram)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_VERSION
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint32_t version;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_COINBASE_TX_SUM
  {
    struct request
    {
      uint64_t height;
      uint64_t count;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height);
        KV_SERIALIZE(count);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t emission_amount;
      uint64_t fee_amount;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(emission_amount)
        KV_SERIALIZE(fee_amount)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE
  {
    struct request
    {
      uint64_t grace_blocks;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(grace_blocks)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      uint64_t fee;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(fee)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ALTERNATE_CHAINS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct chain_info
    {
      std::string block_hash;
      uint64_t height;
      uint64_t length;
      uint64_t difficulty;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(block_hash)
        KV_SERIALIZE(height)
        KV_SERIALIZE(length)
        KV_SERIALIZE(difficulty)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      std::list<chain_info> chains;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(chains)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_UPDATE
  {
    struct request
    {
      std::string command;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(command);
        KV_SERIALIZE(path);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;
      bool update;
      std::string version;
      std::string user_uri;
      std::string auto_uri;
      std::string hash;
      std::string path;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(update)
        KV_SERIALIZE(version)
        KV_SERIALIZE(user_uri)
        KV_SERIALIZE(auto_uri)
        KV_SERIALIZE(hash)
        KV_SERIALIZE(path)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RELAY_TX
  {
    struct request
    {
      std::list<std::string> txids;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string status;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SYNC_INFO
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

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

    struct response
    {
      std::string status;
      uint64_t height;
      uint64_t target_height;
      std::list<peer> peers;
      std::list<span> spans;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(status)
        KV_SERIALIZE(height)
        KV_SERIALIZE(target_height)
        KV_SERIALIZE(peers)
        KV_SERIALIZE(spans)
      END_KV_SERIALIZE_MAP()
    };
  };
}
