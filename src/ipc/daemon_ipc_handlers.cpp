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

// for tx_info and spent_key_image_info, maybe they should be moved elsewhere
#include "rpc/core_rpc_server_commands_defs.h"
////#include "serialization/serialization.h"
//#include "serialization/vector.h"

#include <iostream>

/*!
 * \namespace IPC
 * \brief Anonymous namepsace to keep things in the scope of this file
 */
namespace
{
  cryptonote::core *core = NULL; /*!< Pointer to the core */
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p = NULL;
    /*!< Pointer to p2p node server */
  zactor_t *server = NULL; /*!< 0MQ server */
  bool testnet = false; /*!< testnet mode or not */
  bool restricted = false; /*!< only command suitable for public use */

  /*!
   * \brief Checks if core is busy
   * 
   * \return false if core is busy
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
    if (!p2p->get_payload_object().is_synchronized())
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
      bool p_testnet, bool p_restricted)
    {
      if (server)
        stop();
      p2p = &p_p2p;
      core = &p_core;
      testnet = p_testnet;
      restricted = p_restricted;
      server = zactor_new (wap_server, NULL);
      zsock_send (server, "ss", "BIND", "ipc://@/monero");
      zsock_send (server, "sss", "SET", "server/timeout", "5000");
    }

    /*!
     * \brief stops the IPC server
     */
    void stop() {
      zactor_destroy(&server);
      server = NULL;
      p2p = NULL;
      core = NULL;
    }

    /*!
     * \brief start_mining IPC
     * 
     * \param message 0MQ response object to populate
     */
    void start_mining(wap_proto_t *message)
    {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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

      // we need this to be fast, as refreshing from the wallet is something
      // the user waits through, so we don't encode to JSON (and decode on the
      // wallet side). Instead we sent:
      //  - uin64_t nblocks (big endian)
      //  - That many blocks, consisting of:
      //    - string (4 byte length prefixed)
      //    - uint32_t ntransactions (big endian)
      //    - That many transactions, consisting of:
      //      - string (4 byte length prefixed)
      std::string block_string;
      cryptonote::blobdata blob;

      block_string.resize(block_string.size()+8);
      IPC::write64be((uint8_t*)block_string.c_str() + block_string.size() - 8, bs.size());
      for (std::list<std::pair<cryptonote::block, std::list<cryptonote::transaction>>>::const_iterator i = bs.begin(); i != bs.end(); ++i) {
        block_string.resize(block_string.size()+4);
        blob = block_to_blob(i->first);
        IPC::write32be((uint8_t*)block_string.c_str() + block_string.size() - 4, blob.size());
        block_string.append(blob);
        block_string.resize(block_string.size()+4);
        IPC::write32be((uint8_t*)block_string.c_str() + block_string.size() - 4, i->second.size());
        for (std::list<cryptonote::transaction>::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
          block_string.resize(block_string.size()+4);
          blob = cryptonote::tx_to_blob(*j);
          IPC::write32be((uint8_t*)block_string.c_str() + block_string.size() - 4, blob.size());
          block_string.append(blob);
        }
      }

      // put that string in a frame and msg
      zmsg_t *block_data = zmsg_new();
      zframe_t *frame = zframe_new(block_string.c_str(), block_string.length());
      zmsg_prepend(block_data, &frame);
      wap_proto_set_start_height(message, result_start_height);
      wap_proto_set_curr_height(message, result_current_height);
      wap_proto_set_msg_data(message, &block_data);
      wap_proto_set_status(message, STATUS_OK);
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
      if (!core->handle_incoming_tx(tx_blob, tvc, false, false))
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
        return;
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      wap_proto_set_target(message, core->get_blockchain_storage().get_current_hard_fork_version() < 2 ? DIFFICULTY_TARGET_V1 : DIFFICULTY_TARGET);
      wap_proto_set_tx_count(message, core->get_blockchain_storage().get_total_transactions() - height);
      wap_proto_set_tx_pool_size(message, core->get_pool_transactions_count());
      wap_proto_set_alt_blocks_count(message, core->get_blockchain_storage().get_alternative_blocks_count());
      uint64_t outgoing_connections_count = p2p->get_outgoing_connections_count();
      wap_proto_set_outgoing_connections_count(message, outgoing_connections_count);
      uint64_t total_connections = p2p->get_connections_count();
      wap_proto_set_incoming_connections_count(message, total_connections - outgoing_connections_count);
      wap_proto_set_white_peerlist_size(message, p2p->get_peerlist_manager().get_white_peers_count());
      wap_proto_set_grey_peerlist_size(message, p2p->get_peerlist_manager().get_gray_peers_count());
      wap_proto_set_testnet(message, testnet);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_peer_list IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_peer_list(wap_proto_t *message) {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
      p2p->set_save_graph(true);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief stop_save_graph IPC
     * 
     * \param message 0MQ response object to populate
     */
    void stop_save_graph(wap_proto_t *message) {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
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

    /*!
     * \brief get_hard_fork_info IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_hard_fork_info(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      const cryptonote::Blockchain &blockchain = core->get_blockchain_storage();

      uint8_t version = wap_proto_hfversion(message);
      if (version == 0)
      {
        version = blockchain.get_current_hard_fork_version();
      }

      uint32_t window, votes, threshold;
      uint8_t voting;
      bool enabled = blockchain.get_hard_fork_voting_info(version, window, votes, threshold, voting);
      cryptonote::HardFork::State hfstate = blockchain.get_hard_fork_state();

      wap_proto_set_hfversion(message, blockchain.get_current_hard_fork_version());
      wap_proto_set_enabled(message, enabled);
      wap_proto_set_window(message, window);
      wap_proto_set_votes(message, votes);
      wap_proto_set_threshold(message, threshold);
      wap_proto_set_voting(message, voting);
      wap_proto_set_hfstate(message, hfstate);

      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_connections_list IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_connections_list(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      const std::list<cryptonote::connection_info> connections = p2p->get_payload_object().get_connections();

      // We are using JSON to encode blocks. The JSON string will sit in a 
      // 0MQ frame which gets sent in a zmsg_t object. One could put each block
      // a different frame too.

      // First create a rapidjson object and then stringify it.
      rapidjson::Document result_json;
      result_json.SetObject();
      rapidjson::Document::AllocatorType &allocator = result_json.GetAllocator();
      rapidjson::Value connections_json(rapidjson::kArrayType);
      std::string blob;
      BOOST_FOREACH(auto &c, connections)
      {
        rapidjson::Value this_connection(rapidjson::kObjectType);
        rapidjson::Value string_value(rapidjson::kStringType);
        this_connection.AddMember("incoming", c.incoming, allocator);
        this_connection.AddMember("localhost", c.localhost, allocator);
        this_connection.AddMember("local_ip", c.local_ip, allocator);
        string_value.SetString(c.ip.c_str(), c.ip.length(), allocator);
        this_connection.AddMember("ip", string_value.Move(), allocator);
        string_value.SetString(c.port.c_str(), c.port.length(), allocator);
        this_connection.AddMember("port", string_value.Move(), allocator);
        string_value.SetString(c.peer_id.c_str(), c.peer_id.length(), allocator);
        this_connection.AddMember("peer_id", string_value.Move(), allocator);
        this_connection.AddMember("recv_count", c.recv_count, allocator);
        this_connection.AddMember("recv_idle_time", c.recv_idle_time, allocator);
        this_connection.AddMember("send_count", c.send_count, allocator);
        this_connection.AddMember("send_idle_time", c.send_idle_time, allocator);
        string_value.SetString(c.state.c_str(), c.state.length(), allocator);
        this_connection.AddMember("state", string_value.Move(), allocator);
        this_connection.AddMember("live_time", c.live_time, allocator);
        this_connection.AddMember("avg_download", c.avg_download, allocator);
        this_connection.AddMember("current_download", c.current_download, allocator);
        this_connection.AddMember("avg_upload", c.avg_upload, allocator);
        this_connection.AddMember("current_upload", c.current_upload, allocator);

        connections_json.PushBack(this_connection, allocator);
      }
      result_json.AddMember("connections", connections_json, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      result_json.Accept(writer);
      std::string connections_string = buffer.GetString();
      zframe_t *connections_frame = zframe_new(connections_string.c_str(), connections_string.length());
      wap_proto_set_connections(message, &connections_frame);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief stop_daemon IPC
     * 
     * \param message 0MQ response object to populate
     */
    void stop_daemon(wap_proto_t *message) {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      bool fast = wap_proto_fast(message);
      if (fast) {
        cryptonote::core::set_fast_exit();
        p2p->deinit();
        core->deinit();
      }
      else {
        p2p->send_stop_signal();
      }
      wap_proto_set_status(message, STATUS_OK);
    }

    void get_block_by_worker(wap_proto_t *message, const crypto::hash &hash, bool header_only, bool as_json) {
      const cryptonote::Blockchain &blockchain =  core->get_blockchain_storage();
      cryptonote::block block;
      if (!blockchain.get_block_by_hash(hash, block))
      {
        wap_proto_set_status(message, STATUS_BLOCK_NOT_FOUND);
        return;
      }
      zchunk_t *chunk = NULL;
      if (header_only)
      {
        if (as_json)
        {
          std::string repr = cryptonote::obj_to_json_str((cryptonote::block_header&)block);
          chunk = zchunk_new((const char *)repr.c_str(), repr.size());
        }
        else
        {
          cryptonote::blobdata blob = t_serializable_object_to_blob((cryptonote::block_header&)block);
          chunk = zchunk_new((const char *)blob.data(), blob.size());
        }
      }
      else
      {
        if (as_json)
        {
          std::string repr = cryptonote::obj_to_json_str(block);
          chunk = zchunk_new((const char *)repr.c_str(), repr.size());
        }
        else
        {
          cryptonote::blobdata blob = t_serializable_object_to_blob(block);
          chunk = zchunk_new((const char *)blob.data(), blob.size());
        }
      }
      wap_proto_set_block(message, &chunk);
      wap_proto_set_major_version(message, block.major_version);
      wap_proto_set_major_version(message, block.minor_version);
      wap_proto_set_timestamp(message, block.timestamp);
      std::string hash_str = string_tools::pod_to_hex(block.prev_id);
      zchunk_t *hash_chunk = zchunk_new((void*)(hash_str.c_str()), hash_str.length());
      wap_proto_set_prev_hash(message, &hash_chunk);
      wap_proto_set_nonce(message, block.nonce);
      wap_proto_set_orphan(message, false);
      uint64_t block_height = boost::get<cryptonote::txin_gen>(block.miner_tx.vin.front()).height;
      wap_proto_set_height(message, block_height);
      wap_proto_set_depth(message, core->get_current_blockchain_height() - block_height - 1);
      hash_str = string_tools::pod_to_hex(hash);
      hash_chunk = zchunk_new((void*)(hash_str.c_str()), hash_str.length());
      wap_proto_set_hash(message, &hash_chunk);
      wap_proto_set_difficulty(message, blockchain.block_difficulty(block_height));
      wap_proto_set_reward(message, [](const cryptonote::block &b){uint64_t reward = 0; BOOST_FOREACH(const cryptonote::tx_out& out, b.miner_tx.vout) { reward += out.amount; } return reward;}(block));
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_block_by_hash IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_block_by_hash(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      zchunk_t *hash_chunk = wap_proto_hash(message);
      if (zchunk_size(hash_chunk) != sizeof(crypto::hash))
      {
        wap_proto_set_status(message, STATUS_WRONG_BLOCK_ID_LENGTH);
        return;
      }
      const crypto::hash hash = *reinterpret_cast<const crypto::hash*>(zchunk_data(hash_chunk));
      bool header_only = wap_proto_header_only(message);
      bool as_json = wap_proto_as_json(message);
      get_block_by_worker(message, hash, header_only, as_json);
    }

    /*!
     * \brief get_block_by_height IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_block_by_height(wap_proto_t *message) {
      if (!check_core_ready())
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
      bool header_only = wap_proto_header_only(message);
      bool as_json = wap_proto_as_json(message);
      const cryptonote::Blockchain &blockchain =  core->get_blockchain_storage();
      crypto::hash hash = blockchain.get_block_id_by_height(height);
      if (hash == cryptonote::null_hash)
      {
        wap_proto_set_status(message, STATUS_BLOCK_NOT_FOUND);
        return;
      }
      get_block_by_worker(message, hash, header_only, as_json);
    }

    /*!
     * \brief get_transaction IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_transaction(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      zchunk_t *hash_chunk = wap_proto_tx_id(message);
      if (zchunk_size(hash_chunk) != sizeof(crypto::hash))
      {
        wap_proto_set_status(message, STATUS_WRONG_TX_ID_LENGTH);
        return;
      }
      const crypto::hash hash = *reinterpret_cast<const crypto::hash*>(zchunk_data(hash_chunk));
      std::vector<crypto::hash> hashes;
      hashes.push_back(hash);
      std::list<crypto::hash> missed_txs;
      std::list<cryptonote::transaction> txs;
      bool in_pool = false;
      core->get_transactions(hashes, txs, missed_txs);
      if (!missed_txs.empty()) {
        std::list<cryptonote::transaction> pool_txs;
        if (core->get_pool_transactions(pool_txs)) {
          for (std::list<cryptonote::transaction>::const_iterator i = pool_txs.begin(); i != pool_txs.end(); ++i) {
            std::list<crypto::hash>::iterator mi = std::find(missed_txs.begin(), missed_txs.end(), get_transaction_hash(*i));
            if (mi != missed_txs.end())
            {
              missed_txs.erase(mi);
              txs.push_back(*i);
              in_pool = true;
            }
          }
        }
      }
      if (txs.empty()) {
        wap_proto_set_status(message, STATUS_TX_NOT_FOUND);
        return;
      }
      wap_proto_set_in_pool(message, in_pool);
      zchunk_t *chunk = NULL;
      bool as_json = wap_proto_as_json(message);
      if (as_json)
      {
        std::string repr = cryptonote::obj_to_json_str(txs.front());
        chunk = zchunk_new((const char *)repr.c_str(), repr.size());
      }
      else
      {
        cryptonote::blobdata blob = t_serializable_object_to_blob(txs.front());
        chunk = zchunk_new((const char *)blob.data(), blob.size());
      }
      wap_proto_set_tx_data(message, &chunk);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_key_image_status IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_key_image_status(wap_proto_t *message) {
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      zframe_t *frame = wap_proto_key_images(message);
      size_t n_bytes = zframe_size(frame);
      if (n_bytes % sizeof(crypto::key_image)) {
        wap_proto_set_status(message, STATUS_WRONG_KEY_IMAGE_LENGTH);
        return;
      }
      size_t n_key_images = n_bytes / sizeof(crypto::key_image);
      const crypto::key_image *key_image_data = reinterpret_cast<const crypto::key_image*>(zframe_data(frame));

      std::vector<crypto::key_image> key_images;
      key_images.reserve(n_key_images);
      for (size_t n = 0; n < n_key_images; ++n) {
        key_images.push_back(key_image_data[n]);
      }

      std::vector<bool> spent_status;
      bool r = core->are_key_images_spent(key_images, spent_status);
      if (!r || spent_status.size() != n_key_images) {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }

      bool *spent_data = new bool[n_key_images];
      for (size_t n = 0; n < n_key_images; ++n)
        spent_data[n] = spent_status[n];
      frame = zframe_new(spent_data, n_key_images * sizeof(bool));
      wap_proto_set_spent(message, &frame);

      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_tx_pool IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_tx_pool(wap_proto_t *message)
    {
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      std::vector<cryptonote::tx_info> transactions;
      std::vector<cryptonote::spent_key_image_info> key_images;
      if (!core->get_pool_transactions_and_spent_keys_info(transactions, key_images)) {
        wap_proto_set_status(message, STATUS_INTERNAL_ERROR);
        return;
      }

      // We are using JSON to encode transactions. The JSON string will sit in a 
      // 0MQ frame which gets sent in a zmsg_t object. One could put each transaction
      // a different frame too.

      // First create a rapidjson object and then stringify it.
      rapidjson::Document result_json;
      result_json.SetObject();
      rapidjson::Document::AllocatorType &allocator = result_json.GetAllocator();
      rapidjson::Value tx_json(rapidjson::kArrayType), ki_json(rapidjson::kArrayType);
      rapidjson::Value string_value(rapidjson::kStringType);

      BOOST_FOREACH(auto &txi, transactions)
      {
        rapidjson::Value this_tx(rapidjson::kObjectType);

        string_value.SetString(txi.id_hash.c_str(), txi.id_hash.size(), allocator);
        this_tx.AddMember("id_hash", string_value.Move(), allocator);
        string_value.SetString(txi.tx_json.c_str(), txi.tx_json.size(), allocator);
        this_tx.AddMember("tx_json", string_value.Move(), allocator);
        this_tx.AddMember("blob_size", txi.blob_size, allocator);
        this_tx.AddMember("fee", txi.fee, allocator);
        string_value.SetString(txi.max_used_block_id_hash.c_str(), txi.max_used_block_id_hash.size(), allocator);
        this_tx.AddMember("max_used_block_id_hash", string_value.Move(), allocator);
        this_tx.AddMember("max_used_block_height", txi.max_used_block_height, allocator);
        this_tx.AddMember("kept_by_block", txi.kept_by_block, allocator);
        this_tx.AddMember("last_failed_height", txi.last_failed_height, allocator);
        string_value.SetString(txi.last_failed_id_hash.c_str(), txi.last_failed_id_hash.size(), allocator);
        this_tx.AddMember("last_failed_id_hash", string_value.Move(), allocator);
        this_tx.AddMember("receive_time", txi.receive_time, allocator);

        tx_json.PushBack(this_tx, allocator);
      }
      result_json.AddMember("tx_info", tx_json, allocator);

      BOOST_FOREACH(auto &skii, key_images)
      {
        rapidjson::Value this_ki(rapidjson::kObjectType);

        string_value.SetString(skii.id_hash.c_str(), skii.id_hash.size(), allocator);
        this_ki.AddMember("id_hash", string_value.Move(), allocator);

        rapidjson::Value tx_hashes_json(rapidjson::kArrayType);
        BOOST_FOREACH(auto &tx_hash, skii.txs_hashes)
        {
          string_value.SetString(tx_hash.c_str(), tx_hash.size(), allocator);
          tx_hashes_json.PushBack(string_value.Move(), allocator);
        }
        this_ki.AddMember("tx_hashes", tx_hashes_json, allocator);

        ki_json.PushBack(this_ki, allocator);
      }
      result_json.AddMember("spent_key_image_info", ki_json, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      result_json.Accept(writer);
      std::string tx_string = buffer.GetString();
      zmsg_t *tx_data = zmsg_new();
      // Put the JSON string in a frame.
      zframe_t *frame = zframe_new(tx_string.c_str(), tx_string.length());
      zmsg_prepend(tx_data, &frame);
      wap_proto_set_msg_data(message, &tx_data);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief set_out_peers IPC
     * 
     * \param message 0MQ response object to populate
     */
    void set_out_peers(wap_proto_t *message) {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
      if (!check_core_ready())
      {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }
      uint32_t num_out_peers = wap_proto_num_out_peers(message);
      LOG_PRINT_L0("set_out_peers not implemented");
      wap_proto_set_status(message, STATUS_NOT_IMPLEMENTED);
    }

    /*!
     * \brief get_bans IPC
     * 
     * \param message 0MQ response object to populate
     */
    void get_bans(wap_proto_t *message)
    {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      // We are using JSON to encode the bans list. The JSON string will sit in a 
      // 0MQ frame which gets sent in a zmsg_t object.

      // First create a rapidjson object and then stringify it.
      rapidjson::Document result_json;
      result_json.SetObject();
      rapidjson::Document::AllocatorType &allocator = result_json.GetAllocator();
      rapidjson::Value bans_json(rapidjson::kArrayType);
      rapidjson::Value string_value(rapidjson::kStringType);

      const auto now = time(nullptr);
      std::map<uint32_t, time_t> blocked_ips = p2p->get_blocked_ips();
      for (std::map<uint32_t, time_t>::const_iterator i = blocked_ips.begin(); i != blocked_ips.end(); ++i)
      {
        rapidjson::Value this_ban(rapidjson::kObjectType);

        std::string ip = epee::string_tools::get_ip_string_from_int32(i->first);
        string_value.SetString(ip.c_str(), ip.size(), allocator);
        this_ban.AddMember("ip", string_value.Move(), allocator);
        uint64_t seconds = i->second >= now ? i->second - now : 0;
        this_ban.AddMember("seconds", seconds, allocator);

        bans_json.PushBack(this_ban, allocator);
      }
      result_json.AddMember("bans", bans_json, allocator);

      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      result_json.Accept(writer);
      std::string bans_string = buffer.GetString();
      // Put the JSON string in a frame.
      zframe_t *frame = zframe_new(bans_string.c_str(), bans_string.length());
      wap_proto_set_bans(message, &frame);
      wap_proto_set_status(message, STATUS_OK);
    }

    /*!
     * \brief get_bans IPC
     * 
     * \param message 0MQ response object to populate
     */
    void set_bans(wap_proto_t *message)
    {
      if (restricted) {
        wap_proto_set_status(message, STATUS_RESTRICTED_COMMAND);
        return;
      }
      if (!check_core_busy()) {
        wap_proto_set_status(message, STATUS_CORE_BUSY);
        return;
      }

      zframe_t *bans = wap_proto_bans(message);
      const char *data = (const char*)zframe_data(bans);
      size_t size = zframe_size(bans);
      rapidjson::Document bans_json;
      if (bans_json.Parse(data, size).HasParseError()) {
        wap_proto_set_status(message, STATUS_BAD_JSON_SYNTAX);
        return;
      }

      size_t nbans = bans_json["bans"].Size();
      for (size_t n = 0; n < nbans; ++n) {
        uint32_t ip;
        if (!epee::string_tools::get_ip_int32_from_string(ip, bans_json["bans"][n]["ip"].GetString())) {
          wap_proto_set_status(message, STATUS_BAD_IP_ADDRESS);
          return;
        }
        bool ban = bans_json["bans"][n]["ban"].GetBool();
        uint64_t seconds = bans_json["bans"][n]["seconds"].GetUint64(); 

        if (ban)
          p2p->block_ip(ip, seconds);
        else
          p2p->unblock_ip(ip);
      }

      wap_proto_set_status(message, STATUS_OK);
    }

  }
}
