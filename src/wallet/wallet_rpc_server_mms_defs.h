// Copyright (c) 2019, The Monero Project
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
// #include "wallet/message_store.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

// When making *any* change here, bump versions in "wallet_rpc_server_commands_defs.h"
// Check WALLET_RPC_VERSION_MAJOR and WALLET_RPC_VERSION_MINOR there early in the file


// The following public methods of "message_store.h" don't have any equivalent here,
// in an attempt to avoid unnecessary bloat:
// - get_sanitized_message_text: If you display message text, e.g. notes, "sanitize"
// yourself as needed in accordance with the environment you are in
// - stop: Internal use by "wallet2"; use its 'stop' method if necessary
// - read_file, write_file: Internal use by "wallet2"
// - get_signer_index_by_monero_address, get_signer_index_by_label: Should be quite
// trivial to implement in a client, with the help of "get_all_signers"

namespace tools
{
namespace wallet_rpc
{

  struct mms_message
  {
    uint32_t id;
    std::string type;       // mms::message_type
    std::string direction;  // mms::message_direction
    std::string content;
    uint64_t created;
    uint64_t modified;
    uint64_t sent;
    uint32_t signer_index;
    std::string hash;
    std::string state;      // mms::message_state
    uint32_t wallet_height;
    uint32_t round;
    uint32_t signature_count;
    std::string transport_id;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(id);
      KV_SERIALIZE(type);
      KV_SERIALIZE(direction);
      KV_SERIALIZE(content);
      KV_SERIALIZE(created);
      KV_SERIALIZE(modified);
      KV_SERIALIZE(sent);
      KV_SERIALIZE(signer_index);
      KV_SERIALIZE(hash);
      KV_SERIALIZE(state);
      KV_SERIALIZE(wallet_height);
      KV_SERIALIZE(round);
      KV_SERIALIZE(signature_count);
      KV_SERIALIZE(transport_id);
    END_KV_SERIALIZE_MAP()
  };

  struct mms_processing_data
  {
    std::string processing;   // mms::message_processing
    std::vector<uint32_t> message_ids;
    uint32_t receiving_signer_index;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(processing);
      KV_SERIALIZE(message_ids);
      KV_SERIALIZE(receiving_signer_index);
    END_KV_SERIALIZE_MAP()
  };

  struct mms_authorized_signer
  {
    std::string label;
    std::string transport_address;
    std::string monero_address;
    bool me;
    uint32_t index;
    std::string auto_config_token;
    bool auto_config_running;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(label);
      KV_SERIALIZE(transport_address);
      KV_SERIALIZE(monero_address);
      KV_SERIALIZE(me);
      KV_SERIALIZE(index);
      KV_SERIALIZE(auto_config_token);
      KV_SERIALIZE(auto_config_running);
    END_KV_SERIALIZE_MAP()
  };

  struct mms_multisig_wallet_state
  {
    std::string address;
    std::string nettype;
    bool multisig;
    bool multisig_is_ready;
    bool has_multisig_partial_key_images;
    uint32_t multisig_rounds_passed;
    uint64_t num_transfer_details;

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address);
      KV_SERIALIZE(nettype);
      KV_SERIALIZE(multisig);
      KV_SERIALIZE(multisig_is_ready);
      KV_SERIALIZE(has_multisig_partial_key_images);
      KV_SERIALIZE(multisig_rounds_passed);
      KV_SERIALIZE(num_transfer_details);
    END_KV_SERIALIZE_MAP()
  };

  struct COMMAND_RPC_MMS_INIT
  {
    struct request_t
    {
      std::string own_label;
      std::string own_transport_address;
      uint32_t num_authorized_signers;
      uint32_t num_required_signers;
      
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(own_label)
        KV_SERIALIZE(own_transport_address)
        KV_SERIALIZE(num_authorized_signers)
        KV_SERIALIZE(num_required_signers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_INFO
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool active;
      uint32_t num_authorized_signers;
      uint32_t num_required_signers;
      bool signer_config_complete;
      bool signer_labels_complete;
      bool auto_send;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(active)
        KV_SERIALIZE(num_authorized_signers)
        KV_SERIALIZE(num_required_signers)
        KV_SERIALIZE(signer_config_complete)
        KV_SERIALIZE(signer_labels_complete)
        KV_SERIALIZE(auto_send)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_MULTISIG_WALLET_STATE
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      mms_multisig_wallet_state state;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(state)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_SET_OPTIONS
  {
    struct request_t
    {
      bool set_active;
      bool active;
      bool set_auto_send;
      bool auto_send;
      bool set_bitmessage_options;
      std::string bitmessage_address;
      std::string bitmessage_login;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(set_active)
        KV_SERIALIZE(active)
        KV_SERIALIZE(set_auto_send)
        KV_SERIALIZE(auto_send)
        KV_SERIALIZE(set_bitmessage_options)
        KV_SERIALIZE(bitmessage_address)
        KV_SERIALIZE(bitmessage_login)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_SET_SIGNER
  {
    struct request_t
    {
      int32_t index;
      bool set_label;
      std::string label;
      bool set_transport_address;
      std::string transport_address;
      bool set_monero_address;
      std::string monero_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(set_label)
        KV_SERIALIZE(label)
        KV_SERIALIZE(set_transport_address)
        KV_SERIALIZE(transport_address)
        KV_SERIALIZE(set_monero_address)
        KV_SERIALIZE(monero_address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_SIGNER
  {
    struct request_t
    {
      int32_t index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      mms_authorized_signer signer;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signer)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_ALL_SIGNERS
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::vector<mms_authorized_signer> signers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_SIGNER_CONFIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signer_config;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signer_config)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_UNPACK_SIGNER_CONFIG
  {
    struct request_t
    {
      std::string signer_config;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signer_config)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::vector<mms_authorized_signer> signers;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_PROCESS_SIGNER_CONFIG
  {
    struct request_t
    {
      std::string signer_config;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signer_config)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_START_AUTO_CONFIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_CHECK_AUTO_CONFIG_TOKEN
  {
    struct request_t
    {
      std::string raw_token;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(raw_token)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool token_valid;
      std::string adjusted_token;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(token_valid)
        KV_SERIALIZE(adjusted_token)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_ADD_AUTO_CONFIG_DATA_MESSAGE
  {
    struct request_t
    {
      std::string auto_config_token;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(auto_config_token)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_PROCESS_AUTO_CONFIG_DATA_MESSAGE
  {
    struct request_t
    {
      uint32_t id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_STOP_AUTO_CONFIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_PROCESS_WALLET_CREATED_DATA
  {
    struct request_t
    {
      std::string type;
      std::string content;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(type)
        KV_SERIALIZE(content)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_PROCESSABLE_MESSAGES
  {
    struct request_t
    {
      bool force_sync;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(force_sync)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool available;
      std::vector<mms_processing_data> data_list;
      std::string wait_reason;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(available)
        KV_SERIALIZE(data_list)
        KV_SERIALIZE(wait_reason)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_SET_MESSAGES_PROCESSED
  {
    struct request_t
    {
      mms_processing_data data;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_ADD_MESSAGE
  {
    struct request_t
    {
      uint32_t signer_index;
      std::string type;
      std::string direction;
      std::string content;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signer_index)
        KV_SERIALIZE(type)
        KV_SERIALIZE(direction)
        KV_SERIALIZE(content)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_ALL_MESSAGES
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::vector<mms_message> messages;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(messages)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_GET_MESSAGE_BY_ID
  {
    struct request_t
    {
      uint32_t id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool found;
      mms_message message;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(found)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_SET_MESSAGE_PROCESSED_OR_SENT
  {
    struct request_t
    {
      uint32_t id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_DELETE_MESSAGE
  {
    struct request_t
    {
      uint32_t id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_DELETE_ALL_MESSAGES
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_SEND_MESSAGE
  {
    struct request_t
    {
      uint32_t id;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct COMMAND_RPC_MMS_CHECK_FOR_MESSAGES
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool received;
      std::vector<mms_message> messages;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(received)
        KV_SERIALIZE(messages)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

}
}
