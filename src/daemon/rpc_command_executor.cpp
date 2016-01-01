// Copyright (c) 2014-2015, The Monero Project
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

#include "string_tools.h"
#include "common/scoped_message_writer.h"
#include "daemon/rpc_command_executor.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/hardfork.h"
#include "ipc/include/daemon_ipc_handlers.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <boost/format.hpp>
#include <ctime>
#include <string>

namespace daemonize {

namespace {
  template<typename T>
  void print_peer(const std::string &prefix, T &peer)
  {
    time_t now;
    time(&now);
    time_t last_seen = static_cast<time_t>(peer["last_seen"].GetInt());

    std::string id_str;
    std::string port_str;
    std::string elapsed = epee::misc_utils::get_time_interval_string(now - last_seen);
    std::string ip_str = epee::string_tools::get_ip_string_from_int32(peer["ip"].GetUint());
    epee::string_tools::xtype_to_string(peer["id"].GetUint64(), id_str);
    epee::string_tools::xtype_to_string(peer["port"].GetUint(), port_str);
    std::string addr_str = ip_str + ":" + port_str;
    tools::msg_writer() << boost::format("%-10s %-25s %-25s %s") % prefix % id_str % addr_str % elapsed;
  }

  void print_block_header(wap_client_t *ipc_client)
  {
    zchunk_t *prev_hash = wap_client_prev_hash(ipc_client);
    std::string prev_hash_str((const char *)zchunk_data(prev_hash), zchunk_size(prev_hash));
    zchunk_t *hash = wap_client_hash(ipc_client);
    std::string hash_str((const char *)zchunk_data(hash), zchunk_size(hash));
    tools::success_msg_writer()
      << "timestamp: " << boost::lexical_cast<std::string>(wap_client_timestamp(ipc_client)) << std::endl
      << "previous hash: " << prev_hash_str << std::endl
      << "nonce: " << std::hex << std::setfill('0') << std::setw(8) << wap_client_nonce(ipc_client) << std::dec <<  std::endl
      << "is orphan: " << (wap_client_orphan(ipc_client) ? "true" : "false") << std::endl
      << "height: " << boost::lexical_cast<std::string>(wap_client_height(ipc_client)) << std::endl
      << "depth: " << boost::lexical_cast<std::string>(wap_client_depth(ipc_client)) << std::endl
      << "hash: " << hash_str << std::endl
      << "difficulty: " << boost::lexical_cast<std::string>(wap_client_difficulty(ipc_client)) << std::endl
      << "reward: " << boost::lexical_cast<std::string>(wap_client_reward(ipc_client));
  }
}

t_rpc_command_executor::t_rpc_command_executor()
  : ipc_client(NULL)
{
}

t_rpc_command_executor::~t_rpc_command_executor()
{
  if (ipc_client) {
    wap_client_destroy(&ipc_client);
   }
}

bool t_rpc_command_executor::check_connection_to_daemon() const
{
  return ipc_client && wap_client_connected(ipc_client);
}

bool t_rpc_command_executor::connect_to_daemon()
{
  if (check_connection_to_daemon()) {
    return true;
  }
  ipc_client = wap_client_new();
  wap_client_connect(ipc_client, "ipc://@/monero", 200, "wallet identity");
  if (!check_connection_to_daemon()) {
    wap_client_destroy(&ipc_client); // this sets ipc_client to NULL
    return false;
  }
  return true;
}

bool t_rpc_command_executor::print_peer_list() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_get_peer_list(ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }

  rapidjson::Document response_json;
  rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
  rapidjson::Value result_json;
  result_json.SetObject();

  zframe_t *white_list_frame = wap_client_white_list(ipc_client);
  rapidjson::Document white_list_json;
  const char *data = reinterpret_cast<const char*>(zframe_data(white_list_frame));
  size_t size = zframe_size(white_list_frame);

  if (white_list_json.Parse(data, size).HasParseError()) {
    tools::fail_msg_writer() << "Couldn't parse JSON sent by daemon.";
    return true;
  }

  for (size_t n = 0; n < white_list_json["peers"].Size(); ++n) {
    print_peer("white", white_list_json["peers"][n]);
  }

  zframe_t *gray_list_frame = wap_client_gray_list(ipc_client);
  rapidjson::Document gray_list_json;
  data = reinterpret_cast<const char*>(zframe_data(gray_list_frame));
  size = zframe_size(gray_list_frame);

  if (gray_list_json.Parse(data, size).HasParseError()) {
    tools::fail_msg_writer() << "Couldn't parse JSON sent by daemon.";
    return true;
  }

  for (size_t n = 0; n < gray_list_json["peers"].Size(); ++n) {
    print_peer("gray", gray_list_json["peers"][n]);
  }

  return true;
}

bool t_rpc_command_executor::save_blockchain() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_save_bc (ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }

   tools::success_msg_writer() << "Blockchain saved";

  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_set_log_hash_rate(ipc_client, 1);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  tools::success_msg_writer() << "Hash rate logging is on";
  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_set_log_hash_rate(ipc_client, 0);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  tools::success_msg_writer() << "Hash rate logging is off";
  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_get_info(ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  uint64_t height = wap_client_height(ipc_client);
  uint64_t difficulty = wap_client_difficulty(ipc_client);
  tools::success_msg_writer() <<   "BH: " << height
                              << ", DIFF: " << difficulty
                              << ", HR: " << (int) difficulty / 60L << " H/s";
  return true;
}

bool t_rpc_command_executor::show_status() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }

  int status = wap_client_get_info(ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  uint64_t height = wap_client_height(ipc_client);
  uint64_t target_height = wap_client_target_height(ipc_client);
  uint64_t difficulty = wap_client_difficulty(ipc_client);
  bool testnet = wap_client_testnet(ipc_client);
  uint64_t outgoing_connections_count = wap_client_outgoing_connections_count(ipc_client);
  uint64_t incoming_connections_count = wap_client_incoming_connections_count(ipc_client);

  status = wap_client_get_hard_fork_info(ipc_client, 0);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  uint8_t version = wap_client_hfversion(ipc_client);
  cryptonote::HardFork::State state = (cryptonote::HardFork::State)wap_client_hfstate(ipc_client);

  tools::success_msg_writer() << boost::format("Height: %llu/%llu (%.1f%%) on %s, net hash %s, v%u, %s, %u+%u connections")
    % (unsigned long long)height
    % (unsigned long long)(target_height >= height ? target_height : height)
    % (100.0f * height / (target_height ? target_height < height ? height : target_height : height))
    % (testnet ? "testnet" : "mainnet")
    % [&difficulty]()->std::string {
      float hr = difficulty / 60.0f;
      if (hr>1e9) return (boost::format("%.2f GH/s") % (hr/1e9)).str();
      if (hr>1e6) return (boost::format("%.2f MH/s") % (hr/1e6)).str();
      if (hr>1e3) return (boost::format("%.2f kH/s") % (hr/1e3)).str();
      return (boost::format("%.0f H/s") % hr).str();
    }()
    % (unsigned)version
    % (state == cryptonote::HardFork::Ready ? "up to date" : state == cryptonote::HardFork::UpdateNeeded ? "update needed" : "out of date, likely forked")
    % (unsigned)outgoing_connections_count % (unsigned)incoming_connections_count
  ;

  return true;
}

bool t_rpc_command_executor::print_connections() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }

  int status = wap_client_get_connections_list(ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }

  rapidjson::Document response_json;
  rapidjson::Document::AllocatorType &allocator = response_json.GetAllocator();
  rapidjson::Value result_json;
  result_json.SetObject();

  zframe_t *connections_frame = wap_client_connections(ipc_client);
  if (!connections_frame) {
    tools::fail_msg_writer() << "Failed to get connections";
    return true;
  }
  rapidjson::Document connections_json;
  const char *data = reinterpret_cast<const char*>(zframe_data(connections_frame));
  size_t size = zframe_size(connections_frame);

  if (connections_json.Parse(data, size).HasParseError()) {
    tools::fail_msg_writer() << "Couldn't parse JSON sent by daemon.";
    return true;
  }

  tools::msg_writer() << std::setw(30) << std::left << "Remote Host"
      << std::setw(20) << "Peer id"
      << std::setw(30) << "Recv/Sent (inactive,sec)"
      << std::setw(25) << "State"
      << std::setw(20) << "Livetime(sec)"
      << std::setw(12) << "Down (kB/s)"
      << std::setw(14) << "Down(now)"
      << std::setw(10) << "Up (kB/s)" 
      << std::setw(13) << "Up(now)"
      << std::endl;

  for (size_t n = 0; n < connections_json["connections"].Size(); ++n) {
    const auto &c = connections_json["connections"][n];

    std::string ip_str = epee::string_tools::get_ip_string_from_int32(c["ip"].GetUint());
    std::string port_str;
    epee::string_tools::xtype_to_string(c["port"].GetUint(), port_str);
    std::string address = c["incoming"].GetBool() ? "INC " : "OUT ";
    address += ip_str + ":" + port_str;
    uint64_t recv_count = c["recv_count"].GetUint64();
    uint64_t recv_idle_time = c["recv_idle_time"].GetUint64();
    uint64_t send_count = c["send_count"].GetUint64();
    uint64_t send_idle_time = c["send_idle_time"].GetUint64();
    //std::string in_out = info.incoming ? "INC " : "OUT ";
    tools::msg_writer()
     //<< std::setw(30) << std::left << in_out
     << std::setw(30) << std::left << address
     << std::setw(20) << c["id"].GetUint64()
     << std::setw(30) << std::to_string(recv_count) + "("  + std::to_string(recv_idle_time) + ")/" + std::to_string(send_count) + "(" + std::to_string(send_idle_time) + ")"
     << std::setw(25) << c["state"].GetString()
     << std::setw(20) << c["live_time"].GetUint64()
     << std::setw(12) << c["avg_download"].GetUint64()
     << std::setw(14) << c["current_download"].GetUint64()
     << std::setw(10) << c["avg_upload"].GetUint64()
     << std::setw(13) << c["current_upload"].GetUint64()

     << std::left << (c["localhost"].GetBool() ? "[LOCALHOST]" : "")
     << std::left << (c["local_ip"].GetBool() ? "[LAN]" : "");
    //tools::msg_writer() << boost::format("%-25s peer_id: %-25s %s") % address % info.peer_id % in_out;
  }

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {

// this function appears to not exist in the json rpc api, and so is commented
// until such a time as it does.

/*
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;
  epee::json_rpc::error error_resp;

  req.start_height = start_block_index;
  req.end_height = end_block_index;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblockheadersrange", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_getblockheadersrange(req, res, error_resp))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }
  }

  for (auto & header : res.headers)
  {
    std::cout
      << "major version: " << header.major_version << std::endl
      << "minor version: " << header.minor_version << std::endl
      << "height: " << header.height << ", timestamp: " << header.timestamp << ", difficulty: " << header.difficulty << std::endl
      << "block id: " << header.hash << std::endl
      << "previous block id: " << header.prev_hash << std::endl
      << "difficulty: " << header.difficulty << ", nonce " << header.nonce << std::endl;
  }

*/
  return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_set_log_level(ipc_client, level);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  tools::success_msg_writer() << "Log level is now " << std::to_string(level);
  return true;
}

bool t_rpc_command_executor::print_height() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_get_height(ipc_client);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  uint64_t height = wap_client_height(ipc_client);
  tools::success_msg_writer() << boost::lexical_cast<std::string>(height);
  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash hash) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  zchunk_t *hash_chunk = zchunk_new(hash.data, sizeof(hash));
  int status = wap_client_get_block_by_hash(ipc_client, &hash_chunk, true, false);
  zchunk_destroy(&hash_chunk);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  zchunk_t *chunk = wap_client_block(ipc_client);
  cryptonote::blobdata blob((const char *)zchunk_data(chunk), zchunk_size(chunk));
  cryptonote::block_header block_header;
  if (!cryptonote::parse_and_validate_block_header_from_blob(blob, block_header)) {
    tools::fail_msg_writer() << "Failed to validate block header";
    return true;
  }
  tools::success_msg_writer() << "Header: " << string_tools::pod_to_hex(block_header);
  print_block_header(ipc_client);
  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_get_block_by_height(ipc_client, height, true, false);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  zchunk_t *chunk = wap_client_block(ipc_client);
  cryptonote::blobdata blob((const char *)zchunk_data(chunk), zchunk_size(chunk));
  cryptonote::block_header block_header;
  if (!cryptonote::parse_and_validate_block_header_from_blob(blob, block_header)) {
    tools::fail_msg_writer() << "Failed to validate block header";
    return true;
  }
  tools::success_msg_writer() << "Header: " << string_tools::pod_to_hex(block_header);
  print_block_header(ipc_client);
  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  zchunk_t *txid_chunk = zchunk_new(transaction_hash.data, sizeof(transaction_hash));
  int status = wap_client_get_tx(ipc_client, &txid_chunk, false);
  zchunk_destroy(&txid_chunk);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  zchunk_t *tx_data = wap_client_tx_data(ipc_client);
  cryptonote::blobdata blob((const char *)zchunk_data(tx_data), zchunk_size(tx_data));
  cryptonote::transaction transaction;
  if (!cryptonote::parse_and_validate_tx_from_blob(blob, transaction)) {
    tools::fail_msg_writer() << "Failed to validate transaction";
    return true;
  }
  bool in_pool = wap_client_in_pool(ipc_client);
  tools::success_msg_writer() << "TX found in " << (in_pool ? "pool" : "blockchain");
  tools::success_msg_writer() << cryptonote::obj_to_json_str(transaction);
  return true;
}

bool t_rpc_command_executor::is_key_image_spent(const crypto::key_image &ki) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  zframe_t *frame = zframe_new(&ki, sizeof(ki));
  int status = wap_client_get_key_image_status(ipc_client, &frame);
  zframe_destroy(&frame);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  frame = wap_client_spent(ipc_client);
  if (zframe_size(frame) != 1) {
    // we expect a single reply here, since we went a single key image
    tools::fail_msg_writer() << "Unexpected size reply";
    return true;
  }
  const bool *data = (const bool *)zframe_data(frame);
  tools::success_msg_writer() << ki << ": " << (data[0] ? "spent" : "unspent");
  return true;
}

bool t_rpc_command_executor::print_transaction_pool_long() {
#if 0
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (!m_rpc_server->on_get_transaction_pool(req, res))
  {
    tools::fail_msg_writer() << fail_message.c_str();
    return true;
  }

  if (res.transactions.empty() && res.spent_key_images.empty())
  {
    tools::msg_writer() << "Pool is empty" << std::endl;
  }
  if (! res.transactions.empty())
  {
    tools::msg_writer() << "Transactions: ";
    for (auto & tx_info : res.transactions)
    {
      tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                          << tx_info.tx_json << std::endl
                          << "blob_size: " << tx_info.blob_size << std::endl
                          << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                          << "kept_by_block: " << (tx_info.kept_by_block ? 'T' : 'F') << std::endl
                          << "max_used_block_height: " << tx_info.max_used_block_height << std::endl
                          << "max_used_block_id: " << tx_info.max_used_block_id_hash << std::endl
                          << "last_failed_height: " << tx_info.last_failed_height << std::endl
                          << "last_failed_id: " << tx_info.last_failed_id_hash << std::endl;
    }
    if (res.spent_key_images.empty())
    {
      tools::msg_writer() << "WARNING: Inconsistent pool state - no spent key images";
    }
  }
  if (! res.spent_key_images.empty())
  {
    tools::msg_writer() << ""; // one newline
    tools::msg_writer() << "Spent key images: ";
    for (const cryptonote::spent_key_image_info& kinfo : res.spent_key_images)
    {
      tools::msg_writer() << "key image: " << kinfo.id_hash;
      if (kinfo.txs_hashes.size() == 1)
      {
        tools::msg_writer() << "  tx: " << kinfo.txs_hashes[0];
      }
      else if (kinfo.txs_hashes.size() == 0)
      {
        tools::msg_writer() << "  WARNING: spent key image has no txs associated";
      }
      else
      {
        tools::msg_writer() << "  NOTE: key image for multiple txs: " << kinfo.txs_hashes.size();
        for (const std::string& tx_id : kinfo.txs_hashes)
        {
          tools::msg_writer() << "  tx: " << tx_id;
        }
      }
    }
    if (res.transactions.empty())
    {
      tools::msg_writer() << "WARNING: Inconsistent pool state - no transactions";
    }
  }
#endif

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_short() {
#if 0
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (!m_rpc_server->on_get_transaction_pool(req, res))
  {
    tools::fail_msg_writer() << fail_message.c_str();
    return true;
  }

  if (res.transactions.empty())
  {
    tools::msg_writer() << "Pool is empty" << std::endl;
  }
  for (auto & tx_info : res.transactions)
  {
    tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                        << "blob_size: " << tx_info.blob_size << std::endl
                        << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                        << "kept_by_block: " << (tx_info.kept_by_block ? 'T' : 'F') << std::endl
                        << "max_used_block_height: " << tx_info.max_used_block_height << std::endl
                        << "max_used_block_id: " << tx_info.max_used_block_id_hash << std::endl
                        << "last_failed_height: " << tx_info.last_failed_height << std::endl
                        << "last_failed_id: " << tx_info.last_failed_id_hash << std::endl;
  }
#endif

  return true;
}

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads, bool testnet) {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }

  std::string address_str = cryptonote::get_account_address_as_str(testnet, address);
  zchunk_t *address_chunk = zchunk_new((void*)address_str.c_str(), address_str.length());
  int status = wap_client_start(ipc_client, &address_chunk, num_threads);
  zchunk_destroy(&address_chunk);
  if (status < 0) {
    tools::fail_msg_writer() << "Failed to start mining";
    return true;
  }
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  tools::success_msg_writer() << "Mining started";
  return true;
}

bool t_rpc_command_executor::stop_mining() {
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }

  int status = wap_client_stop(ipc_client);
  if (status < 0) {
    tools::fail_msg_writer() << "Failed to stop mining";
    return true;
  }
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }

  tools::success_msg_writer() << "Mining stopped";
  return true;
}

bool t_rpc_command_executor::stop_daemon()
{
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_stop_daemon(ipc_client);
  if (status < 0) {
    tools::fail_msg_writer() << "Failed to stop daemon";
    return true;
  }
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  tools::success_msg_writer() << "Daemon stopping";
  return true;
}

bool t_rpc_command_executor::print_status()
{
  tools::success_msg_writer() << "print_status makes no sense in interactive mode";
  return true;
}

bool t_rpc_command_executor::get_limit()
{
    int limit_down = epee::net_utils::connection_basic::get_rate_down_limit( );
    int limit_up = epee::net_utils::connection_basic::get_rate_up_limit( );
    std::cout << "limit-down is " << limit_down/1024 << " kB/s" << std::endl;
    std::cout << "limit-up is " << limit_up/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::set_limit(int limit)
{
    epee::net_utils::connection_basic::set_rate_down_limit( limit );
    epee::net_utils::connection_basic::set_rate_up_limit( limit );
    std::cout << "Set limit-down to " << limit/1024 << " kB/s" << std::endl;
    std::cout << "Set limit-up to " << limit/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::get_limit_up()
{
    int limit_up = epee::net_utils::connection_basic::get_rate_up_limit( );
    std::cout << "limit-up is " << limit_up/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::set_limit_up(int limit)
{
    epee::net_utils::connection_basic::set_rate_up_limit( limit );
    std::cout << "Set limit-up to " << limit/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::get_limit_down()
{
    int limit_down = epee::net_utils::connection_basic::get_rate_down_limit( );
    std::cout << "limit-down is " << limit_down/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::set_limit_down(int limit)
{
    epee::net_utils::connection_basic::set_rate_down_limit( limit );
    std::cout << "Set limit-down to " << limit/1024 << " kB/s" << std::endl;
    return true;
}

bool t_rpc_command_executor::fast_exit()
{
#if 0
	cryptonote::COMMAND_RPC_FAST_EXIT::request req;
	cryptonote::COMMAND_RPC_FAST_EXIT::response res;
	
	std::string fail_message = "Daemon did not stop";
	
	if (!m_rpc_server->on_fast_exit(req, res))
	{
		tools::fail_msg_writer() << fail_message.c_str();
		return true;
	}

	tools::success_msg_writer() << "Daemon stopped";
#endif
	return true;
}

bool t_rpc_command_executor::out_peers(uint64_t limit)
{
#if 0
	cryptonote::COMMAND_RPC_OUT_PEERS::request req;
	cryptonote::COMMAND_RPC_OUT_PEERS::response res;
	
	epee::json_rpc::error error_resp;

	req.out_peers = limit;
	
	std::string fail_message = "Unsuccessful";

	if (!m_rpc_server->on_out_peers(req, res))
	{
		tools::fail_msg_writer() << fail_message.c_str();
		return true;
	}
#endif

	return true;
}

bool t_rpc_command_executor::start_save_graph()
{
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  if (wap_client_start_save_graph(ipc_client) < 0) {
    tools::fail_msg_writer() << "Failed to start saving graph";
    return true;
  }
  tools::success_msg_writer() << "Started saving graph";
  return true;
}

bool t_rpc_command_executor::stop_save_graph()
{
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  if (wap_client_stop_save_graph(ipc_client) < 0) {
    tools::fail_msg_writer() << "Failed to stop saving graph";
    return true;
  }
  tools::success_msg_writer() << "Stopped saving graph";
  return true;
}

bool t_rpc_command_executor::hard_fork_info(uint8_t version)
{
  if (!connect_to_daemon()) {
    tools::fail_msg_writer() << "Failed to connect to daemon";
    return true;
  }
  int status = wap_client_get_hard_fork_info(ipc_client, version);
  if (status) {
    tools::fail_msg_writer() << "Error: " << IPC::get_status_string(status);
    return true;
  }
  uint8_t hfversion = wap_client_hfversion(ipc_client);
  cryptonote::HardFork::State state = (cryptonote::HardFork::State)wap_client_hfstate(ipc_client);
  version = version > 0 ? version : hfversion;
  bool enabled = wap_client_enabled(ipc_client);
  uint32_t votes = wap_client_votes(ipc_client);
  uint32_t window = wap_client_window(ipc_client);
  uint32_t threshold = wap_client_threshold(ipc_client);
  uint8_t voting = wap_client_voting(ipc_client);
  tools::msg_writer() << "version " << (uint32_t)version << " " << (enabled ? "enabled" : "not enabled") <<
      ", " << votes << "/" << window << " votes, threshold " << threshold;
  tools::msg_writer() << "current version " << (uint32_t)hfversion << ", voting for version " << (uint32_t)voting;
  return true;
}

}// namespace daemonize
