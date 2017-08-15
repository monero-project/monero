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
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

namespace tools
{
namespace wallet_rpc
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"

  struct COMMAND_RPC_GET_BALANCE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t 	 balance;
      uint64_t 	 unlocked_balance;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
      END_KV_SERIALIZE_MAP()
    };
  };

    struct COMMAND_RPC_GET_ADDRESS
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string   address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
  };

    struct COMMAND_RPC_GET_HEIGHT
    {
      struct request
      {
        BEGIN_KV_SERIALIZE_MAP()
        END_KV_SERIALIZE_MAP()
      };

      struct response
      {
        uint64_t  height;
        BEGIN_KV_SERIALIZE_MAP()
          KV_SERIALIZE(height)
        END_KV_SERIALIZE_MAP()
      };
    };

  struct transfer_destination
  {
    uint64_t amount;
    std::string address;
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_TRANSFER
  {
    struct request
    {
      std::list<transfer_destination> destinations;
      uint32_t priority;
      uint64_t mixin;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_key;
      bool do_not_relay;
      bool get_tx_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(priority)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string tx_hash;
      std::string tx_key;
      std::list<std::string> amount_keys;
      uint64_t fee;
      std::string tx_blob;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount_keys)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_TRANSFER_SPLIT
  {
    struct request
    {
      std::list<transfer_destination> destinations;
      uint32_t priority;
      uint64_t mixin;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_keys;
      bool do_not_relay;
      bool get_tx_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(priority)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> amount_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_DUST
  {
    struct request
    {
      bool get_tx_keys;
      bool do_not_relay;
      bool get_tx_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SWEEP_ALL
  {
    struct request
    {
      std::string address;
      uint32_t priority;
      uint64_t mixin;
      uint64_t unlock_time;
      std::string payment_id;
      bool get_tx_keys;
      uint64_t below_amount;
      bool do_not_relay;
      bool get_tx_hex;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(priority)
        KV_SERIALIZE(mixin)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE(below_amount)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
      END_KV_SERIALIZE_MAP()
    };

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<std::string> tx_hash_list;
      std::list<std::string> tx_key_list;
      std::list<uint64_t> fee_list;
      std::list<std::string> tx_blob_list;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STORE
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct payment_details
  {
    std::string payment_id;
    std::string tx_hash;
    uint64_t amount;
    uint64_t block_height;
    uint64_t unlock_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(block_height)
      KV_SERIALIZE(unlock_time)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_PAYMENTS
  {
    struct request
    {
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    struct request
    {
      std::vector<std::string> payment_ids;
      uint64_t min_block_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)
        KV_SERIALIZE(min_block_height)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<payment_details> payments;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
  };
  
  struct transfer_details
  {
    uint64_t amount;
    bool spent;
    uint64_t global_index;
    std::string tx_hash;
    uint64_t tx_size;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(spent)
      KV_SERIALIZE(global_index)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(tx_size)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_INCOMING_TRANSFERS
  {
    struct request
    {
      std::string transfer_type;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer_type)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<transfer_details> transfers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfers)
      END_KV_SERIALIZE_MAP()
    };
  };

  //JSON RPC V2
  struct COMMAND_RPC_QUERY_KEY
  {
    struct request
    {
      std::string key_type;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_type)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string key;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_MAKE_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string integrated_address;
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS
  {
    struct request
    {
      std::string integrated_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string standard_address;
      std::string payment_id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STOP_WALLET
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RESCAN_BLOCKCHAIN
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SET_TX_NOTES
  {
    struct request
    {
      std::list<std::string> txids;
      std::list<std::string> notes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TX_NOTES
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
      std::list<std::string> notes;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct transfer_entry
  {
    std::string txid;
    std::string payment_id;
    uint64_t height;
    uint64_t timestamp;
    uint64_t amount;
    uint64_t fee;
    std::string note;
    std::list<transfer_destination> destinations;
    std::string type;
    uint64_t unlock_time;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(txid);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(height);
      KV_SERIALIZE(timestamp);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(fee);
      KV_SERIALIZE(note);
      KV_SERIALIZE(destinations);
      KV_SERIALIZE(type);
      KV_SERIALIZE(unlock_time)
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_GET_TRANSFERS
  {
    struct request
    {
      bool in;
      bool out;
      bool pending;
      bool failed;
      bool pool;

      bool filter_by_height;
      uint64_t min_height;
      uint64_t max_height;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
        KV_SERIALIZE(filter_by_height);
        KV_SERIALIZE(min_height);
        KV_SERIALIZE(max_height);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::list<transfer_entry> in;
      std::list<transfer_entry> out;
      std::list<transfer_entry> pending;
      std::list<transfer_entry> failed;
      std::list<transfer_entry> pool;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_TRANSFER_BY_TXID
  {
    struct request
    {
      std::string txid;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      transfer_entry transfer;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_SIGN
  {
    struct request
    {
      std::string data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_VERIFY
  {
    struct request
    {
      std::string data;
      std::string address;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
        KV_SERIALIZE(address);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      bool good;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_EXPORT_KEY_IMAGES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct signed_key_image
    {
      std::string key_image;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<signed_key_image> signed_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_IMPORT_KEY_IMAGES
  {
    struct signed_key_image
    {
      std::string key_image;
      std::string signature;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct request
    {
      std::vector<signed_key_image> signed_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t height;
      uint64_t spent;
      uint64_t unspent;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(spent)
        KV_SERIALIZE(unspent)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct uri_spec
  {
    std::string address;
    std::string payment_id;
    uint64_t amount;
    std::string tx_description;
    std::string recipient_name;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(tx_description);
      KV_SERIALIZE(recipient_name);
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MAKE_URI
  {
    struct request: public uri_spec
    {
    };

    struct response
    {
      std::string uri;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_PARSE_URI
  {
    struct request
    {
      std::string uri;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uri_spec uri;
      std::vector<std::string> unknown_parameters;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri);
        KV_SERIALIZE(unknown_parameters);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      std::string address;
      std::string payment_id;
      std::string description;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      uint64_t index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      std::list<uint64_t> entries;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };

    struct entry
    {
      uint64_t index;
      std::string address;
      std::string payment_id;
      std::string description;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      std::vector<entry> entries;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY
  {
    struct request
    {
      uint64_t index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_RESCAN_SPENT
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_START_MINING
  {
    struct request
    {
      uint64_t    threads_count;
      bool        do_background_mining;
      bool        ignore_battery;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)        
        KV_SERIALIZE(ignore_battery)        
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_STOP_MINING
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };

    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_GET_LANGUAGES
  {
    struct request
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      std::vector<std::string> languages;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(languages)
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_CREATE_WALLET
  {
    struct request
    {
      std::string filename;
      std::string password;
      std::string language;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
        KV_SERIALIZE(language)
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };

  struct COMMAND_RPC_OPEN_WALLET
  {
    struct request
    {
      std::string filename;
      std::string password;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
      END_KV_SERIALIZE_MAP()
    };
    struct response
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
  };
}
}
