// Copyright (c) 2014, The Monero Project
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

#include "daemon_ipc_handlers.h"

#include <iostream>

namespace
{
  cryptonote::core *core; /*!< Pointer to the core */
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
    /*!< Pointer to p2p node server */
  zactor_t *server;
  bool testnet;
  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return false;
    }
    return true;
  }
}

namespace IPC
{
  namespace Daemon
  {
    void init(cryptonote::core *p_core,
      nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
      bool p_testnet)
    {
      p2p = p_p2p;
      core = p_core;
      testnet = p_testnet;
      server = zactor_new (wap_server, NULL);
      zsock_send (server, "ss", "BIND", "ipc://@/monero");
      zsock_send (server, "sss", "SET", "server/timeout", "5000");
    }

    void start_mining(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      cryptonote::account_public_address adr;
      const char *address = wap_proto_address(message);
      if (!get_account_address_from_str(adr, testnet, std::string(address)))
      {
        wap_proto_set_status(message, STATUS_WRONG_ADDRESS);
        return;
      }

      boost::thread::attributes attrs;
      attrs.set_stack_size(THREAD_STACK_SIZE);

      uint64_t threads_count = wap_proto_thread_count(message);
      if (!core->get_miner().start(adr, static_cast<size_t>(threads_count), attrs))
      {
        wap_proto_set_status(message, STATUS_MINING_NOT_STARTED);
        return;
      }
      wap_proto_set_status(message, STATUS_OK);
    }

    void retrieve_blocks(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      uint64_t start_height = wap_proto_start_height(message);
      zlist_t *z_block_ids = wap_proto_block_ids(message);

      std::list<crypto::hash> block_ids;
      char *block_id = (char*)zlist_first(z_block_ids);
      while (block_id) {
        crypto::hash hash;
        memcpy(hash.data, block_id + 1, crypto::HASH_SIZE);
        block_ids.push_back(hash);
        block_id = (char*)zlist_next(z_block_ids);
      }

      std::list<std::pair<cryptonote::block, std::list<cryptonote::transaction> > > bs;
      uint64_t result_current_height = 0;
      uint64_t result_start_height = 0;
      if (!core->find_blockchain_supplement(start_height, block_ids, bs, result_current_height,
        result_start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }

      rapidjson::Document result_json;
      result_json.SetObject();
      rapidjson::Document::AllocatorType &allocator = result_json.GetAllocator();
      rapidjson::Value block_json(rapidjson::kArrayType);
      std::string blob;
      BOOST_FOREACH(auto &b, bs)
      {
        rapidjson::Value this_block(rapidjson::kObjectType);
        blob = block_to_blob(b.first);
        rapidjson::Value string_value(rapidjson::kStringType);
        string_value.SetString(blob.c_str(), blob.length(), allocator);
        this_block.AddMember("block", string_value.Move(), allocator);
        rapidjson::Value txs_blocks(rapidjson::kArrayType);
        BOOST_FOREACH(auto &t, b.second)
        {
          rapidjson::Value string_value(rapidjson::kStringType);
          blob = cryptonote::tx_to_blob(t);
          string_value.SetString(blob.c_str(), blob.length(), allocator);
          txs_blocks.PushBack(string_value.Move(), allocator);
        }
        this_block.AddMember("txs", txs_blocks, allocator);
        block_json.PushBack(this_block, allocator);
      }

      result_json.AddMember("blocks", block_json, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      result_json.Accept(writer);
      std::string block_string = buffer.GetString();
      zmsg_t *block_data = zmsg_new();
      zframe_t *frame = zframe_new(block_string.c_str(), block_string.length());
      zmsg_prepend(block_data, &frame);
      wap_proto_set_start_height(message, result_start_height);
      wap_proto_set_curr_height(message, result_current_height);
      wap_proto_set_status(message, STATUS_OK);
      wap_proto_set_block_data(message, &block_data);

    }

    void send_raw_transaction(wap_proto_t *message)
    {

    }

    void get_output_indexes(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      const char *tx_id = wap_proto_tx_id(message);
      crypto::hash hash;
      memcpy(hash.data, tx_id + 1, crypto::HASH_SIZE);
      std::vector<uint64_t> output_indexes;
      bool r = core->get_tx_outputs_gindexs(hash, output_indexes);
      if (!r)
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }
      // Spec guarantees that vector elements are contiguous. So coversion to uint64_t[] is easy.
      uint64_t *indexes = &output_indexes[0];
      zframe_t *frame = zframe_new(indexes, sizeof(uint64_t) * output_indexes.size());
      wap_proto_set_o_indexes(message, &frame);
      wap_proto_set_status(message, STATUS_OK);
    }
  }
}
