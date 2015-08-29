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

/*!
 * \file daemon_ipc_handlers.cpp
 * \brief Implementation of Daemon IPC handlers
 * 
 * Most of this code is borrowed from core_rpc_server.cpp but changed to use 0MQ objects.
 */

//TODO: Recheck memory leaks

#include "daemon_ipc_handlers.h"

#include <iostream>

/*!
 * \namespace IPC
 * \brief Anonymous namepsace to keep things in the scope of this file
 */
namespace
{
  cryptonote::core *core; /*!< Pointer to the core */
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
    /*!< Pointer to p2p node server */
  zactor_t *server; /*!< 0MQ server */
  bool testnet; /*!< testnet mode or not */

  /*!
   * \brief Checks if core is busy
   * 
   * \return true if core is busy
   */
  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return false;
    }
    return true;
  }

  /*!
   * \brief Checks if core is ready
   * 
   * \return true if core is ready
   */
  bool check_core_ready()
  {
    if (p2p->get_payload_object().is_synchronized())
    {
      return false;
    }
    return check_core_busy();
  }

  /*!
   * \brief equivalent of strstr, but with arbitrary bytes (ie, NULs)
   * This does not differentiate between "not found" and "found at offset 0"
   * (taken straight from core_rpc_server.cpp)
   */
  uint64_t slow_memmem(const void *start_buff, size_t buflen, const void *pat, size_t patlen)
  {
    const void *buf = start_buff;
    const void *end = (const char*)buf + buflen;
    if (patlen > buflen || patlen == 0) return 0;
    while (buflen > 0 && (buf = memchr(buf, ((const char*)pat)[0], buflen-patlen + 1)))
    {
      if (memcmp(buf,pat,patlen) == 0)
        return (const char*)buf - (const char*)start_buff;
      buf = (const char*)buf + 1;
      buflen = (const char*)end - (const char*)buf;
    }
    return 0;
  }
}

/*!
 * \namespace IPC
 * \brief Namespace pertaining to IPC.
 */
namespace IPC
{
  /*!
   * \namespace Daemon
   * \brief Namespace pertaining to Daemon IPC.
   */
  namespace Daemon
  {
    /*!
     * \brief initializes it with objects necessary to handle IPC requests and starts
     *        IPC server
     * 
     * \param p_core cryptonote core object
     * \param p_p2p  p2p object
     * \param p_testnet testnet mode or not
     */
    void init(cryptonote::core &p_core,
      nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > &p_p2p,
      bool p_testnet)
    {
      p2p = &p_p2p;
      core = &p_core;
      testnet = p_testnet;
      server = zactor_new (wap_server, NULL);
      zsock_send (server, "ss", "BIND", "ipc://@/monero");
      zsock_send (server, "sss", "SET", "server/timeout", "5000");
    }

    /*!
     * \brief stops the IPC server
     * 
     * \param p_core cryptonote core object
     * \param p_p2p  p2p object
     * \param p_testnet testnet mode or not
     */
    void stop() {
      zactor_destroy(&server);
    }

    /*!
     * \brief start_mining IPC
     * 
     * \param message 0MQ response object to populate
     */
    void start_mining(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      cryptonote::account_public_address adr;
      zchunk_t *address_chunk = wap_proto_address(message);
      char *address = (char*)zchunk_data(address_chunk);
      std::string address_string(address, zchunk_size(address_chunk));

      if (!get_account_address_from_str(adr, testnet, std::string(address_string)))
      {
        wap_proto_set_status(message, STATUS_WRONG_ADDRESS);
        return;
      }

      boost::thread::attributes attrs;
      attrs.set_stack_size(THREAD_STACK_SIZE);

      uint64_t thread_count = wap_proto_thread_count(message);
      if (!core->get_miner().start(adr, static_cast<size_t>(thread_count), attrs))
      {
        wap_proto_set_status(message, STATUS_MINING_NOT_STARTED);
        return;
      }
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief stop_mining IPC
     * 
     * \param message 0MQ response object to populate
     */
    void stop_mining(wap_proto_t *message)
    {
      if (!core->get_miner().stop())
      {
        wap_proto_set_status(message, STATUS_MINING_NOT_STOPPED);
        return;
      }
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_blocks IPC
     * 
     * \param message 0MQ response object to populate
     */
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

      // We are using JSON to encode blocks. The JSON string will sit in a 
      // 0MQ frame which gets sent in a zmsg_t object. One could put each block
      // a different frame too.

      // First create a rapidjson object and then stringify it.
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
      // Put the JSON string in a frame.
      zframe_t *frame = zframe_new(block_string.c_str(), block_string.length());
      zmsg_prepend(block_data, &frame);
      wap_proto_set_start_height(message, result_start_height);
      wap_proto_set_curr_height(message, result_current_height);
      wap_proto_set_status(message, STATUS_OK);
      wap_proto_set_block_data(message, &block_data);

    }

    /*!
     * \brief send_raw_transaction IPC
     * 
     * \param message 0MQ response object to populate
     */
    void send_raw_transaction(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      std::string tx_blob;
      zchunk_t *tx_as_hex_chunk = wap_proto_tx_as_hex(message);
      char *tx_as_hex = (char*)zchunk_data(tx_as_hex_chunk);
      std::string tx_as_hex_string(tx_as_hex, zchunk_size(tx_as_hex_chunk));
      if (!string_tools::parse_hexstr_to_binbuff(tx_as_hex_string, tx_blob))
      {
        LOG_PRINT_L0("[on_send_raw_tx]: Failed to parse tx from hexbuff: " << tx_as_hex_string);
        wap_proto_set_status(message, STATUS_INVALID_TX);
        return;
      }

      cryptonote::cryptonote_connection_context fake_context = AUTO_VAL_INIT(fake_context);
      cryptonote::tx_verification_context tvc = AUTO_VAL_INIT(tvc);
      if (!core->handle_incoming_tx(tx_blob, tvc, false))
      {
        LOG_PRINT_L0("[on_send_raw_tx]: Failed to process tx");
        wap_proto_set_status(message, STATUS_INVALID_TX);
        return;
      }

      if (tvc.m_verifivation_failed)
      {
        LOG_PRINT_L0("[on_send_raw_tx]: tx verification failed");
        wap_proto_set_status(message, STATUS_TX_VERIFICATION_FAILED);
        return;
      }

      if (!tvc.m_should_be_relayed)
      {
        LOG_PRINT_L0("[on_send_raw_tx]: tx accepted, but not relayed");
        wap_proto_set_status(message, STATUS_TX_NOT_RELAYED);
        return;
      }

      cryptonote::NOTIFY_NEW_TRANSACTIONS::request r;
      r.txs.push_back(tx_blob);
      core->get_protocol()->relay_transactions(r, fake_context);
      // TODO: make sure that tx has reached other nodes here, probably wait to receive reflections from other nodes
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_output_indexes IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_output_indexes(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      zchunk_t *tx_id = wap_proto_tx_id(message);
      crypto::hash hash;
      memcpy(hash.data, zchunk_data(tx_id), crypto::HASH_SIZE);
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

    /*!
     * \brief get_random_outputs IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_random_outs(wap_proto_t *message) {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      // The core does its stuff with old style RPC objects.
      // So we construct and read from those objects.
      cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req;
      uint64_t outs_count = wap_proto_outs_count(message);
      req.outs_count = outs_count;
      zframe_t *amounts_frame = wap_proto_amounts(message);
      uint64_t amounts_count = zframe_size(amounts_frame) / sizeof(uint64_t);
      uint64_t *amounts = (uint64_t*)zframe_data(amounts_frame);
      for (unsigned int i = 0; i < amounts_count; i++) {
        req.amounts.push_back(amounts[i]);
      }

      cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res;
      if (!core->get_random_outs_for_amounts(req, res))
      {
        wap_proto_set_status(message, STATUS_RANDOM_OUTS_FAILED);
      }

      // We convert the result into a JSON string and put it into a 0MQ frame.
      rapidjson::Document result_json;
      result_json.SetObject();
      rapidjson::Document::AllocatorType &allocator = result_json.GetAllocator();
      rapidjson::Value outputs_json(rapidjson::kArrayType);

      typedef cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount outs_for_amount;
      typedef cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
      std::stringstream ss;
      std::for_each(res.outs.begin(), res.outs.end(), [&](outs_for_amount& ofa)
      {
        ss << "[" << ofa.amount << "]:";
        CHECK_AND_ASSERT_MES(ofa.outs.size(), ;, "internal error: ofa.outs.size() is empty for amount " << ofa.amount);
        std::for_each(ofa.outs.begin(), ofa.outs.end(), [&](out_entry& oe)
            {
              ss << oe.global_amount_index << " ";
            });
        ss << ENDL;
      });
      std::string s = ss.str();
      LOG_PRINT_L2("COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS: " << ENDL << s);
      for (unsigned int i = 0; i < res.outs.size(); i++) {
        rapidjson::Value output(rapidjson::kObjectType);
        outs_for_amount out = res.outs[i];
        rapidjson::Value output_entries(rapidjson::kArrayType);
        for (std::list<out_entry>::iterator it = out.outs.begin(); it != out.outs.end(); it++) {
          rapidjson::Value output_entry(rapidjson::kObjectType);
          out_entry entry = *it;
          output_entry.AddMember("global_amount_index", entry.global_amount_index, allocator);
          rapidjson::Value string_value(rapidjson::kStringType);
          string_value.SetString(entry.out_key.data, 32, allocator);
          output_entry.AddMember("out_key", string_value.Move(), allocator);
          output_entries.PushBack(output_entry, allocator);
        }
        output.AddMember("amount", out.amount, allocator);
        output.AddMember("outs", output_entries, allocator);
        outputs_json.PushBack(output, allocator);
      }
      result_json.AddMember("outputs", outputs_json, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      result_json.Accept(writer);
      std::string block_string = buffer.GetString();

      zframe_t *frame = zframe_new(block_string.c_str(), block_string.length());
      wap_proto_set_random_outputs(message, &frame);

      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_height IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_height(wap_proto_t *message) {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      wap_proto_set_height(message, core->get_current_blockchain_height());
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief save_bc IPC
     * 
     * \param message 0MQ response object to populate
     */
    void save_bc(wap_proto_t *message) {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      if (!core->get_blockchain_storage().store_blockchain()) {
        wap_proto_set_status(message, STATUS_ERROR_STORING_BLOCKCHAIN);
        return;
      }
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_info IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_info(wap_proto_t *message) {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      uint64_t height = core->get_current_blockchain_height();
      wap_proto_set_height(message, height);
      wap_proto_set_target_height(message, core->get_target_blockchain_height());
      wap_proto_set_difficulty(message, core->get_blockchain_storage().get_difficulty_for_next_block());
      wap_proto_set_tx_count(message, core->get_blockchain_storage().get_total_transactions() - height);
      wap_proto_set_tx_pool_size(message, core->get_pool_transactions_count());
      wap_proto_set_alt_blocks_count(message, core->get_blockchain_storage().get_alternative_blocks_count());
      uint64_t outgoing_connections_count = p2p->get_outgoing_connections_count();
      wap_proto_set_outgoing_connections_count(message, outgoing_connections_count);
      uint64_t total_connections = p2p->get_connections_count();
      wap_proto_set_incoming_connections_count(message, total_connections - outgoing_connections_count);
      wap_proto_set_white_peerlist_size(message, p2p->get_peerlist_manager().get_white_peers_count());
      wap_proto_set_grey_peerlist_size(message, p2p->get_peerlist_manager().get_gray_peers_count());
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_peer_list IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_peer_list(wap_proto_t *message) {
      std::list<nodetool::peerlist_entry> white_list;
      std::list<nodetool::peerlist_entry> gray_list;
      p2p->get_peerlist_manager().get_peerlist_full(white_list, gray_list);

      // The response is of non-trivial type and so is encoded as JSON.
      // Each peer list is going to look like this:
      // {"peers": [{"id": ....}, ...]}

      rapidjson::Document white_list_json;
      white_list_json.SetObject();
      rapidjson::Document::AllocatorType &white_list_allocator = white_list_json.GetAllocator();
      rapidjson::Value white_peers(rapidjson::kArrayType);

      for (auto & entry : white_list) {
        // Each peer object is encoded as JSON
        rapidjson::Value output(rapidjson::kObjectType);
        output.AddMember("id", entry.id, white_list_allocator);
        output.AddMember("ip", entry.adr.ip, white_list_allocator);
        output.AddMember("port", entry.adr.port, white_list_allocator);
        output.AddMember("last_seen", entry.last_seen, white_list_allocator);
        white_peers.PushBack(output, white_list_allocator);
      }
      white_list_json.AddMember("peers", white_peers, white_list_allocator);

      rapidjson::Document gray_list_json;
      gray_list_json.SetObject();
      rapidjson::Document::AllocatorType &gray_list_allocator = gray_list_json.GetAllocator();
      rapidjson::Value gray_peers(rapidjson::kArrayType);

      for (auto & entry : gray_list) {
        // Each peer object is encoded as JSON
        rapidjson::Value output(rapidjson::kObjectType);
        output.AddMember("id", entry.id, gray_list_allocator);
        output.AddMember("ip", entry.adr.ip, gray_list_allocator);
        output.AddMember("port", entry.adr.port, gray_list_allocator);
        output.AddMember("last_seen", entry.last_seen, gray_list_allocator);
        gray_peers.PushBack(output, gray_list_allocator);
      }
      gray_list_json.AddMember("peers", gray_peers, gray_list_allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      white_list_json.Accept(writer);
      std::string white_list_string = buffer.GetString();

      zframe_t *white_list_frame = zframe_new(white_list_string.c_str(), white_list_string.length());

      buffer.Clear();
      gray_list_json.Accept(writer);
      std::string gray_list_string = buffer.GetString();
      zframe_t *gray_list_frame = zframe_new(gray_list_string.c_str(), gray_list_string.length());

      wap_proto_set_white_list(message, &white_list_frame);
      wap_proto_set_gray_list(message, &gray_list_frame);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_mining_status IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_mining_status(wap_proto_t *message) {
      if (!check_core_ready()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      const cryptonote::miner& lMiner = core->get_miner();
      wap_proto_set_active(message, lMiner.is_mining() ? 1 : 0);

      if (lMiner.is_mining()) {
        wap_proto_set_speed(message, lMiner.get_speed());
        wap_proto_set_thread_count(message, lMiner.get_threads_count());
        const cryptonote::account_public_address& lMiningAdr = lMiner.get_mining_address();
        std::string address = get_account_address_as_str(testnet, lMiningAdr);
        zchunk_t *address_chunk = zchunk_new((void*)address.c_str(), address.length());
        wap_proto_set_address(message, &address_chunk);
      }

      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief set_log_hash_rate IPC
     * 
     * \param message 0MQ response object to populate
     */
    void set_log_hash_rate(wap_proto_t *message) {
      if (core->get_miner().is_mining())
      {
        core->get_miner().do_print_hashrate(wap_proto_visible(message));
        wap_proto_set_status(message, STATUS_OK);
      }
      else
      {
        wap_proto_set_status(message, STATUS_NOT_MINING);
      }
    }

    /*!
     * \brief set_log_hash_rate IPC
     * 
     * \param message 0MQ response object to populate
     */
    void set_log_level(wap_proto_t *message) {
      // zproto supports only unsigned integers afaik. so the log level is sent as
      // one and casted to signed int here.
      int8_t level = (int8_t)wap_proto_level(message);
      if (level < LOG_LEVEL_MIN || level > LOG_LEVEL_MAX)
      {
        wap_proto_set_status(message, STATUS_INVALID_LOG_LEVEL);
      }
      else
      {
        epee::log_space::log_singletone::get_set_log_detalisation_level(true, level);
        int otshell_utils_log_level = 100 - (level * 20);
        gCurrentLogger.setDebugLevel(otshell_utils_log_level);
        wap_proto_set_status(message, STATUS_OK);
      }
    }

    /*!
     * \brief start_save_graph IPC
     * 
     * \param message 0MQ response object to populate
     */
    void start_save_graph(wap_proto_t *message) {
      p2p->set_save_graph(true);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief stop_save_graph IPC
     * 
     * \param message 0MQ response object to populate
     */
    void stop_save_graph(wap_proto_t *message) {
      p2p->set_save_graph(false);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_block_hash IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_block_hash(wap_proto_t *message) {
      if (!check_core_busy())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      uint64_t height = wap_proto_height(message);
      if (core->get_current_blockchain_height() <= height)
      {
        wap_proto_set_status(message, STATUS_HEIGHT_TOO_BIG);
        return;
      }
      std::string hash = string_tools::pod_to_hex(core->get_block_id_by_height(height));
      zchunk_t *hash_chunk = zchunk_new((void*)(hash.c_str()), hash.length());
      wap_proto_set_hash(message, &hash_chunk);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_block_template IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_block_template(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      uint64_t reserve_size = wap_proto_reserve_size(message);
      if (reserve_size > 255)
      {
        wap_proto_set_status(message, STATUS_RESERVE_SIZE_TOO_BIG);
        return;
      }

      cryptonote::account_public_address acc = AUTO_VAL_INIT(acc);

      zchunk_t *address_chunk = wap_proto_address(message);
      std::string address((char*)zchunk_data(address_chunk), zchunk_size(address_chunk));
      if (!address.size() || !cryptonote::get_account_address_from_str(acc, testnet, address))
      {
        wap_proto_set_status(message, STATUS_WRONG_ADDRESS);
        return;
      }

      cryptonote::block b = AUTO_VAL_INIT(b);
      cryptonote::blobdata blob_reserve;
      blob_reserve.resize(reserve_size, 0);
      uint64_t difficulty = wap_proto_difficulty(message);
      uint64_t height;
      if (!core->get_block_template(b, acc, difficulty, height, blob_reserve))
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }
      cryptonote::blobdata block_blob = t_serializable_object_to_blob(b);
      crypto::public_key tx_pub_key = cryptonote::get_tx_pub_key_from_extra(b.miner_tx);
      if (tx_pub_key == cryptonote::null_pkey)
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }
      uint64_t reserved_offset = slow_memmem((void*)block_blob.data(), block_blob.size(), &tx_pub_key, sizeof(tx_pub_key));
      if (!reserved_offset)
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }
      reserved_offset += sizeof(tx_pub_key) + 3; // 3 bytes: tag for TX_EXTRA_TAG_PUBKEY(1 byte), tag for TX_EXTRA_NONCE(1 byte), counter in TX_EXTRA_NONCE(1 byte)
      if (reserved_offset + reserve_size > block_blob.size())
      {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }
      wap_proto_set_height(message, height);
      wap_proto_set_difficulty(message, difficulty);
      wap_proto_set_reserved_offset(message, reserved_offset);
      std::string prev_hash = string_tools::pod_to_hex(b.prev_id);
      zchunk_t *prev_hash_chunk = zchunk_new((void*)prev_hash.c_str(), prev_hash.length());
      wap_proto_set_prev_hash(message, &prev_hash_chunk);

      cryptonote::blobdata blocktemplate_blob = string_tools::buff_to_hex_nodelimer(block_blob);
      zchunk_t *blob_chunk = zchunk_new((void*)blocktemplate_blob.c_str(), blocktemplate_blob.length());
      wap_proto_set_block_template_blob(message, &blob_chunk);
      wap_proto_set_status(message, STATUS_OK);
    }
  }
}
