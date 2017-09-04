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

#include "string_tools.h"
#include "common/password.h"
#include "common/scoped_message_writer.h"
#include "daemon/rpc_command_executor.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_basic/hardfork.h"
#include <boost/format.hpp>
#include <ctime>
#include <string>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize {

namespace {
  void print_peer(std::string const & prefix, cryptonote::peer const & peer)
  {
    time_t now;
    time(&now);
    time_t last_seen = static_cast<time_t>(peer.last_seen);

    std::string id_str;
    std::string port_str;
    std::string elapsed = epee::misc_utils::get_time_interval_string(now - last_seen);
    std::string ip_str = epee::string_tools::get_ip_string_from_int32(peer.ip);
    std::stringstream peer_id_str;
    peer_id_str << std::hex << std::setw(16) << peer.id;
    peer_id_str >> id_str;
    epee::string_tools::xtype_to_string(peer.port, port_str);
    std::string addr_str = ip_str + ":" + port_str;
    tools::msg_writer() << boost::format("%-10s %-25s %-25s %s") % prefix % id_str % addr_str % elapsed;
  }

  void print_block_header(cryptonote::block_header_response const & header)
  {
    tools::success_msg_writer()
      << "timestamp: " << boost::lexical_cast<std::string>(header.timestamp) << std::endl
      << "previous hash: " << header.prev_hash << std::endl
      << "nonce: " << boost::lexical_cast<std::string>(header.nonce) << std::endl
      << "is orphan: " << header.orphan_status << std::endl
      << "height: " << boost::lexical_cast<std::string>(header.height) << std::endl
      << "depth: " << boost::lexical_cast<std::string>(header.depth) << std::endl
      << "hash: " << header.hash << std::endl
      << "difficulty: " << boost::lexical_cast<std::string>(header.difficulty) << std::endl
      << "reward: " << boost::lexical_cast<std::string>(header.reward);
  }

  std::string get_human_time_ago(time_t t, time_t now)
  {
    if (t == now)
      return "now";
    time_t dt = t > now ? t - now : now - t;
    std::string s;
    if (dt < 90)
      s = boost::lexical_cast<std::string>(dt) + " seconds";
    else if (dt < 90 * 60)
      s = boost::lexical_cast<std::string>(dt/60) + " minutes";
    else if (dt < 36 * 3600)
      s = boost::lexical_cast<std::string>(dt/3600) + " hours";
    else
      s = boost::lexical_cast<std::string>(dt/(3600*24)) + " days";
    return s + " " + (t > now ? "in the future" : "ago");
  }

  std::string get_time_hms(time_t t)
  {
    unsigned int hours, minutes, seconds;
    char buffer[24];
    hours = t / 3600;
    t %= 3600;
    minutes = t / 60;
    t %= 60;
    seconds = t;
    snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
    return std::string(buffer);
  }

  std::string make_error(const std::string &base, const std::string &status)
  {
    if (status == CORE_RPC_STATUS_OK)
      return base;
    return base + " -- " + status;
  }
}

t_rpc_command_executor::t_rpc_command_executor(
    uint32_t ip
  , uint16_t port
  , const boost::optional<tools::login>& login
  , bool is_rpc
  , cryptonote::core_rpc_server* rpc_server
  )
  : m_rpc_client(NULL), m_rpc_server(rpc_server)
{
  if (is_rpc)
  {
    boost::optional<epee::net_utils::http::login> http_login{};
    if (login)
      http_login.emplace(login->username, login->password.password());
    m_rpc_client = new tools::t_rpc_client(ip, port, std::move(http_login));
  }
  else
  {
    if (rpc_server == NULL)
    {
      throw std::runtime_error("If not calling commands via RPC, rpc_server pointer must be non-null");
    }
  }

  m_is_rpc = is_rpc;
}

t_rpc_command_executor::~t_rpc_command_executor()
{
  if (m_rpc_client != NULL)
  {
    delete m_rpc_client;
  }
}

bool t_rpc_command_executor::print_peer_list() {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  std::string failure_message = "Couldn't retrieve peer list";
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_peer_list", failure_message.c_str()))
    {
      return false;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_peer_list(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << failure_message;
      return false;
    }
  }

  for (auto & peer : res.white_list)
  {
    print_peer("white", peer);
  }

  for (auto & peer : res.gray_list)
  {
    print_peer("gray", peer);
  }

  return true;
}

bool t_rpc_command_executor::print_peer_list_stats() {
  cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
  cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

  std::string failure_message = "Couldn't retrieve peer list";
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_peer_list", failure_message.c_str()))
    {
      return false;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_peer_list(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << failure_message;
      return false;
    }
  }

  tools::msg_writer()
    << "White list size: " << res.white_list.size() << "/" << P2P_LOCAL_WHITE_PEERLIST_LIMIT << " (" << res.white_list.size() *  100.0 / P2P_LOCAL_WHITE_PEERLIST_LIMIT << "%)" << std::endl
    << "Gray list size: " << res.gray_list.size() << "/" << P2P_LOCAL_GRAY_PEERLIST_LIMIT << " (" << res.gray_list.size() *  100.0 / P2P_LOCAL_GRAY_PEERLIST_LIMIT << "%)";

  return true;
}

bool t_rpc_command_executor::save_blockchain() {
  cryptonote::COMMAND_RPC_SAVE_BC::request req;
  cryptonote::COMMAND_RPC_SAVE_BC::response res;

  std::string fail_message = "Couldn't save blockchain";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/save_bc", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_save_bc(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Blockchain saved";

  return true;
}

bool t_rpc_command_executor::show_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = true;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_hash_rate(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
    }
  }

  tools::success_msg_writer() << "Hash rate logging is on";

  return true;
}

bool t_rpc_command_executor::hide_hash_rate() {
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
  cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
  req.visible = false;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_hash_rate(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Hash rate logging is off";

  return true;
}

bool t_rpc_command_executor::show_difficulty() {
  cryptonote::COMMAND_RPC_GET_INFO::request req;
  cryptonote::COMMAND_RPC_GET_INFO::response res;

  std::string fail_message = "Problem fetching info";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_info(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message.c_str(), res.status);
      return true;
    }
  }

  tools::success_msg_writer() <<   "BH: " << res.height
                              << ", TH: " << res.top_block_hash
                              << ", DIFF: " << res.difficulty
                              << ", HR: " << res.difficulty / res.target << " H/s";

  return true;
}

static std::string get_mining_speed(uint64_t hr)
{
  if (hr>1e9) return (boost::format("%.2f GH/s") % (hr/1e9)).str();
  if (hr>1e6) return (boost::format("%.2f MH/s") % (hr/1e6)).str();
  if (hr>1e3) return (boost::format("%.2f kH/s") % (hr/1e3)).str();
  return (boost::format("%.0f H/s") % hr).str();
}

static std::string get_fork_extra_info(uint64_t t, uint64_t now, uint64_t block_time)
{
  uint64_t blocks_per_day = 86400 / block_time;

  if (t == now)
    return " (forking now)";

  if (t > now)
  {
    uint64_t dblocks = t - now;
    if (dblocks <= 30)
      return (boost::format(" (next fork in %u blocks)") % (unsigned)dblocks).str();
    if (dblocks <= blocks_per_day / 2)
      return (boost::format(" (next fork in %.1f hours)") % (dblocks / (float)(blocks_per_day / 24))).str();
    if (dblocks <= blocks_per_day * 30)
      return (boost::format(" (next fork in %.1f days)") % (dblocks / (float)blocks_per_day)).str();
    return "";
  }
  return "";
}

static float get_sync_percentage(const cryptonote::COMMAND_RPC_GET_INFO::response &ires)
{
  uint64_t height = ires.height;
  uint64_t target_height = ires.target_height ? ires.target_height < ires.height ? ires.height : ires.target_height : ires.height;
  float pc = 100.0f * height / target_height;
  if (height < target_height && pc > 99.9f)
    return 99.9f; // to avoid 100% when not fully synced
  return pc;
}

bool t_rpc_command_executor::show_status() {
  cryptonote::COMMAND_RPC_GET_INFO::request ireq;
  cryptonote::COMMAND_RPC_GET_INFO::response ires;
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::request hfreq;
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::response hfres;
  cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
  cryptonote::COMMAND_RPC_MINING_STATUS::response mres;
  epee::json_rpc::error error_resp;
  bool has_mining_info = true;

  std::string fail_message = "Problem fetching info";

  hfreq.version = 0;
  bool mining_busy = false;
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
    if (!m_rpc_client->json_rpc_request(hfreq, hfres, "hard_fork_info", fail_message.c_str()))
    {
      return true;
    }
    // mining info is only available non unrestricted RPC mode
    has_mining_info = m_rpc_client->rpc_request(mreq, mres, "/mining_status", fail_message.c_str());
  }
  else
  {
    if (!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, ires.status);
      return true;
    }
    if (!m_rpc_server->on_hard_fork_info(hfreq, hfres, error_resp) || hfres.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, hfres.status);
      return true;
    }
    if (!m_rpc_server->on_mining_status(mreq, mres))
    {
      tools::fail_msg_writer() << fail_message.c_str();
      return true;
    }

    if (mres.status == CORE_RPC_STATUS_BUSY)
    {
      mining_busy = true;
    }
    else if (mres.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, mres.status);
      return true;
    }
  }

  std::time_t uptime = std::time(nullptr) - ires.start_time;
  uint64_t net_height = ires.target_height > ires.height ? ires.target_height : ires.height;

  tools::success_msg_writer() << boost::format("Height: %llu/%llu (%.1f%%) on %s, %s, net hash %s, v%u%s, %s, %u(out)+%u(in) connections, uptime %ud %uh %um %us")
    % (unsigned long long)ires.height
    % (unsigned long long)net_height
    % get_sync_percentage(ires)
    % (ires.testnet ? "testnet" : "mainnet")
    % (!has_mining_info ? "mining info unavailable" : mining_busy ? "syncing" : mres.active ? ( ( mres.is_background_mining_enabled ? "smart " : "" ) + std::string("mining at ") + get_mining_speed(mres.speed) ) : "not mining")
    % get_mining_speed(ires.difficulty / ires.target)
    % (unsigned)hfres.version
    % get_fork_extra_info(hfres.earliest_height, net_height, ires.target)
    % (hfres.state == cryptonote::HardFork::Ready ? "up to date" : hfres.state == cryptonote::HardFork::UpdateNeeded ? "update needed" : "out of date, likely forked")
    % (unsigned)ires.outgoing_connections_count
    % (unsigned)ires.incoming_connections_count
    % (unsigned int)floor(uptime / 60.0 / 60.0 / 24.0)
    % (unsigned int)floor(fmod((uptime / 60.0 / 60.0), 24.0))
    % (unsigned int)floor(fmod((uptime / 60.0), 60.0))
    % (unsigned int)fmod(uptime, 60.0)
  ;

  return true;
}

bool t_rpc_command_executor::print_connections() {
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::request req;
  cryptonote::COMMAND_RPC_GET_CONNECTIONS::response res;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "get_connections", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_connections(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << std::setw(30) << std::left << "Remote Host"
      << std::setw(20) << "Peer id"
      << std::setw(20) << "Support Flags"      
      << std::setw(30) << "Recv/Sent (inactive,sec)"
      << std::setw(25) << "State"
      << std::setw(20) << "Livetime(sec)"
      << std::setw(12) << "Down (kB/s)"
      << std::setw(14) << "Down(now)"
      << std::setw(10) << "Up (kB/s)" 
      << std::setw(13) << "Up(now)"
      << std::endl;

  for (auto & info : res.connections)
  {
    std::string address = info.incoming ? "INC " : "OUT ";
    address += info.ip + ":" + info.port;
    //std::string in_out = info.incoming ? "INC " : "OUT ";
    tools::msg_writer() 
     //<< std::setw(30) << std::left << in_out
     << std::setw(30) << std::left << address
     << std::setw(20) << epee::string_tools::pad_string(info.peer_id, 16, '0', true)
     << std::setw(20) << info.support_flags
     << std::setw(30) << std::to_string(info.recv_count) + "("  + std::to_string(info.recv_idle_time) + ")/" + std::to_string(info.send_count) + "(" + std::to_string(info.send_idle_time) + ")"
     << std::setw(25) << info.state
     << std::setw(20) << info.live_time
     << std::setw(12) << info.avg_download
     << std::setw(14) << info.current_download
     << std::setw(10) << info.avg_upload
     << std::setw(13) << info.current_upload
     
     << std::left << (info.localhost ? "[LOCALHOST]" : "")
     << std::left << (info.local_ip ? "[LAN]" : "");
    //tools::msg_writer() << boost::format("%-25s peer_id: %-25s %s") % address % info.peer_id % in_out;
    
  }

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
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
    if (!m_rpc_server->on_get_block_headers_range(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  bool first = true;
  for (auto & header : res.headers)
  {
    if (!first)
      std::cout << std::endl;
    std::cout
      << "height: " << header.height << ", timestamp: " << header.timestamp << ", difficulty: " << header.difficulty
      << ", size: " << header.block_size << std::endl
      << "major version: " << (unsigned)header.major_version << ", minor version: " << (unsigned)header.minor_version << std::endl
      << "block id: " << header.hash << ", previous block id: " << header.prev_hash << std::endl
      << "difficulty: " << header.difficulty << ", nonce " << header.nonce << ", reward " << cryptonote::print_money(header.reward) << std::endl;
    first = false;
  }

  return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level) {
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
  cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;
  req.level = level;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_level", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_level(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Log level is now " << std::to_string(level);

  return true;
}

bool t_rpc_command_executor::set_log_categories(const std::string &categories) {
  cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::request req;
  cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::response res;
  req.categories = categories;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_log_categories", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_log_categories(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Log categories are now " << categories;

  return true;
}

bool t_rpc_command_executor::print_height() {
  cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
  cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/getheight", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_height(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << boost::lexical_cast<std::string>(res.height);

  return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash) {
  cryptonote::COMMAND_RPC_GET_BLOCK::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK::response res;
  epee::json_rpc::error error_resp;

  req.hash = epee::string_tools::pod_to_hex(block_hash);

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblock", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_block(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  print_block_header(res.block_header);
  tools::success_msg_writer() << res.json << ENDL;

  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height) {
  cryptonote::COMMAND_RPC_GET_BLOCK::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK::response res;
  epee::json_rpc::error error_resp;

  req.height = height;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "getblock", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_block(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  print_block_header(res.block_header);
  tools::success_msg_writer() << res.json << ENDL;

  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  std::string fail_message = "Problem fetching transaction";

  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(transaction_hash));
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/gettransactions", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transactions(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  if (1 == res.txs.size() || 1 == res.txs_as_hex.size())
  {
    if (1 == res.txs.size())
    {
      // only available for new style answers
      if (res.txs.front().in_pool)
        tools::success_msg_writer() << "Found in pool";
      else
        tools::success_msg_writer() << "Found in blockchain at height " << res.txs.front().block_height;
    }

    // first as hex
    const std::string &as_hex = (1 == res.txs.size()) ? res.txs.front().as_hex : res.txs_as_hex.front();
    tools::success_msg_writer() << as_hex;

    // then as json
    crypto::hash tx_hash, tx_prefix_hash;
    cryptonote::transaction tx;
    cryptonote::blobdata blob;
    if (!string_tools::parse_hexstr_to_binbuff(as_hex, blob))
    {
      tools::fail_msg_writer() << "Failed to parse tx";
    }
    else if (!cryptonote::parse_and_validate_tx_from_blob(blob, tx, tx_hash, tx_prefix_hash))
    {
      tools::fail_msg_writer() << "Failed to parse tx blob";
    }
    else
    {
      tools::success_msg_writer() << cryptonote::obj_to_json_str(tx) << std::endl;
    }
  }
  else
  {
    tools::fail_msg_writer() << "transaction wasn't found: " << transaction_hash << std::endl;
  }

  return true;
}

bool t_rpc_command_executor::is_key_image_spent(const crypto::key_image &ki) {
  cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req;
  cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::response res;

  std::string fail_message = "Problem checking key image";

  req.key_images.push_back(epee::string_tools::pod_to_hex(ki));
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/is_key_image_spent", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_is_key_image_spent(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  if (1 == res.spent_status.size())
  {
    // first as hex
    tools::success_msg_writer() << ki << ": " << (res.spent_status.front() ? "spent" : "unspent");
  }
  else
  {
    tools::fail_msg_writer() << "key image status could not be determined" << std::endl;
  }

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_long() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transaction_pool(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  if (res.transactions.empty() && res.spent_key_images.empty())
  {
    tools::msg_writer() << "Pool is empty" << std::endl;
  }
  if (! res.transactions.empty())
  {
    const time_t now = time(NULL);
    tools::msg_writer() << "Transactions: ";
    for (auto & tx_info : res.transactions)
    {
      tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                          << tx_info.tx_json << std::endl
                          << "blob_size: " << tx_info.blob_size << std::endl
                          << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                          << "fee/byte: " << cryptonote::print_money(tx_info.fee / (double)tx_info.blob_size) << std::endl
                          << "receive_time: " << tx_info.receive_time << " (" << get_human_time_ago(tx_info.receive_time, now) << ")" << std::endl
                          << "relayed: " << [&](const cryptonote::tx_info &tx_info)->std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; } (tx_info) << std::endl
                          << "do_not_relay: " << (tx_info.do_not_relay ? 'T' : 'F')  << std::endl
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

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_short() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

  std::string fail_message = "Problem fetching transaction pool";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_transaction_pool(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  if (res.transactions.empty())
  {
    tools::msg_writer() << "Pool is empty" << std::endl;
  }
  else
  {
    const time_t now = time(NULL);
    for (auto & tx_info : res.transactions)
    {
      tools::msg_writer() << "id: " << tx_info.id_hash << std::endl
                          << "blob_size: " << tx_info.blob_size << std::endl
                          << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                          << "fee/byte: " << cryptonote::print_money(tx_info.fee / (double)tx_info.blob_size) << std::endl
                          << "receive_time: " << tx_info.receive_time << " (" << get_human_time_ago(tx_info.receive_time, now) << ")" << std::endl
                          << "relayed: " << [&](const cryptonote::tx_info &tx_info)->std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; } (tx_info) << std::endl
                          << "do_not_relay: " << (tx_info.do_not_relay ? 'T' : 'F')  << std::endl
                          << "kept_by_block: " << (tx_info.kept_by_block ? 'T' : 'F') << std::endl
                          << "max_used_block_height: " << tx_info.max_used_block_height << std::endl
                          << "max_used_block_id: " << tx_info.max_used_block_id_hash << std::endl
                          << "last_failed_height: " << tx_info.last_failed_height << std::endl
                          << "last_failed_id: " << tx_info.last_failed_id_hash << std::endl;
    }
  }

  return true;
}

bool t_rpc_command_executor::print_transaction_pool_stats() {
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response res;
  cryptonote::COMMAND_RPC_GET_INFO::request ireq;
  cryptonote::COMMAND_RPC_GET_INFO::response ires;

  std::string fail_message = "Problem fetching transaction pool stats";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_transaction_pool_stats", fail_message.c_str()))
    {
      return true;
    }
    if (!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    memset(&res.pool_stats, 0, sizeof(res.pool_stats));
    if (!m_rpc_server->on_get_transaction_pool_stats(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
    if (!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, ires.status);
      return true;
    }
  }

  size_t n_transactions = res.pool_stats.txs_total;
  const uint64_t now = time(NULL);
  size_t avg_bytes = n_transactions ? res.pool_stats.bytes_total / n_transactions : 0;

  std::string backlog_message;
  const uint64_t full_reward_zone = ires.block_size_limit / 2;
  if (res.pool_stats.bytes_total <= full_reward_zone)
  {
    backlog_message = "no backlog";
  }
  else
  {
    uint64_t backlog = (res.pool_stats.bytes_total + full_reward_zone - 1) / full_reward_zone;
    backlog_message = (boost::format("estimated %u block (%u minutes) backlog") % backlog % (backlog * DIFFICULTY_TARGET_V2 / 60)).str();
  }

  tools::msg_writer() << n_transactions << " tx(es), " << res.pool_stats.bytes_total << " bytes total (min " << res.pool_stats.bytes_min << ", max " << res.pool_stats.bytes_max << ", avg " << avg_bytes << ")" << std::endl
      << "fees " << cryptonote::print_money(res.pool_stats.fee_total) << " (avg " << cryptonote::print_money(n_transactions ? res.pool_stats.fee_total / n_transactions : 0) << " per tx" << ", " << cryptonote::print_money(res.pool_stats.bytes_total ? res.pool_stats.fee_total / res.pool_stats.bytes_total : 0) << " per byte)" << std::endl
      << res.pool_stats.num_not_relayed << " not relayed, " << res.pool_stats.num_failing << " failing, " << res.pool_stats.num_10m << " older than 10 minutes (oldest " << (res.pool_stats.oldest == 0 ? "-" : get_human_time_ago(res.pool_stats.oldest, now)) << "), " << backlog_message;

  if (n_transactions > 1 && res.pool_stats.histo.size())
  {
    std::vector<uint64_t> times;
    uint64_t numer;
    size_t i, n = res.pool_stats.histo.size(), denom;
    times.resize(n);
    if (res.pool_stats.histo_98pc)
    {
      numer = res.pool_stats.histo_98pc;
      denom = n-1;
      for (i=0; i<denom; i++)
        times[i] = i * numer / denom;
      times[i] = now - res.pool_stats.oldest;
    } else
    {
      numer = now - res.pool_stats.oldest;
      denom = n;
      for (i=0; i<denom; i++)
        times[i] = i * numer / denom;
    }
    tools::msg_writer() << "   Age      Txes       Bytes";
    for (i=0; i<n; i++)
    {
      tools::msg_writer() << get_time_hms(times[i]) << setw(8) << res.pool_stats.histo[i].txs << setw(12) << res.pool_stats.histo[i].bytes;
    }
  }
  tools::msg_writer();

  return true;
}

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads, bool testnet, bool do_background_mining, bool ignore_battery) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(testnet, address);
  req.threads_count = num_threads;
  req.do_background_mining = do_background_mining;
  req.ignore_battery = ignore_battery;
  
  std::string fail_message = "Mining did not start";

  if (m_is_rpc)
  {
    if (m_rpc_client->rpc_request(req, res, "/start_mining", fail_message.c_str()))
    {
      tools::success_msg_writer() << "Mining started";
    }
  }
  else
  {
    if (!m_rpc_server->on_start_mining(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  return true;
}

bool t_rpc_command_executor::stop_mining() {
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  std::string fail_message = "Mining did not stop";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/stop_mining", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_stop_mining(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Mining stopped";
  return true;
}

bool t_rpc_command_executor::stop_daemon()
{
  cryptonote::COMMAND_RPC_STOP_DAEMON::request req;
  cryptonote::COMMAND_RPC_STOP_DAEMON::response res;

//# ifdef WIN32
//    // Stop via service API
//    // TODO - this is only temporary!  Get rid of hard-coded constants!
//    bool ok = windows::stop_service("BitMonero Daemon");
//    ok = windows::uninstall_service("BitMonero Daemon");
//    //bool ok = windows::stop_service(SERVICE_NAME);
//    //ok = windows::uninstall_service(SERVICE_NAME);
//    if (ok)
//    {
//      return true;
//    }
//# endif

  // Stop via RPC
  std::string fail_message = "Daemon did not stop";

  if (m_is_rpc)
  {
    if(!m_rpc_client->rpc_request(req, res, "/stop_daemon", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_stop_daemon(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::success_msg_writer() << "Stop signal sent";

  return true;
}

bool t_rpc_command_executor::print_status()
{
  if (!m_is_rpc)
  {
    tools::success_msg_writer() << "print_status makes no sense in interactive mode";
    return true;
  }

  bool daemon_is_alive = m_rpc_client->check_connection();

  if(daemon_is_alive) {
    tools::success_msg_writer() << "monerod is running";
  }
  else {
    tools::fail_msg_writer() << "monerod is NOT running";
  }

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

bool t_rpc_command_executor::out_peers(uint64_t limit)
{
	cryptonote::COMMAND_RPC_OUT_PEERS::request req;
	cryptonote::COMMAND_RPC_OUT_PEERS::response res;
	
	epee::json_rpc::error error_resp;

	req.out_peers = limit;
	
	std::string fail_message = "Unsuccessful";

	if (m_is_rpc)
	{
		if (!m_rpc_client->json_rpc_request(req, res, "out_peers", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if (!m_rpc_server->on_out_peers(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			tools::fail_msg_writer() << make_error(fail_message, res.status);
			return true;
		}
	}

	std::cout << "Max number of out peers set to " << limit << std::endl;

	return true;
}

bool t_rpc_command_executor::start_save_graph()
{
	cryptonote::COMMAND_RPC_START_SAVE_GRAPH::request req;
	cryptonote::COMMAND_RPC_START_SAVE_GRAPH::response res;
	std::string fail_message = "Unsuccessful";
	
	if (m_is_rpc)
	{
		if (!m_rpc_client->rpc_request(req, res, "/start_save_graph", fail_message.c_str()))
		{
			return true;
		}
	}
	
	else
    {
		if (!m_rpc_server->on_start_save_graph(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			tools::fail_msg_writer() << make_error(fail_message, res.status);
			return true;
		}
	}
	
	return true;
}

bool t_rpc_command_executor::stop_save_graph()
{
	cryptonote::COMMAND_RPC_STOP_SAVE_GRAPH::request req;
	cryptonote::COMMAND_RPC_STOP_SAVE_GRAPH::response res;
	std::string fail_message = "Unsuccessful";
	
	if (m_is_rpc)
	{
		if (!m_rpc_client->rpc_request(req, res, "/stop_save_graph", fail_message.c_str()))
		{
			return true;
		}
	}
	
	else
    {
		if (!m_rpc_server->on_stop_save_graph(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			tools::fail_msg_writer() << make_error(fail_message, res.status);
			return true;
		}
	}
	return true;
}

bool t_rpc_command_executor::hard_fork_info(uint8_t version)
{
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req;
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.version = version;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "hard_fork_info", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_hard_fork_info(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    version = version > 0 ? version : res.voting;
    tools::msg_writer() << "version " << (uint32_t)version << " " << (res.enabled ? "enabled" : "not enabled") <<
        ", " << res.votes << "/" << res.window << " votes, threshold " << res.threshold;
    tools::msg_writer() << "current version " << (uint32_t)res.version << ", voting for version " << (uint32_t)res.voting;

    return true;
}

bool t_rpc_command_executor::print_bans()
{
    cryptonote::COMMAND_RPC_GETBANS::request req;
    cryptonote::COMMAND_RPC_GETBANS::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "get_bans", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_get_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    for (auto i = res.bans.begin(); i != res.bans.end(); ++i)
    {
        tools::msg_writer() << epee::string_tools::get_ip_string_from_int32(i->ip) << " banned for " << i->seconds << " seconds";
    }

    return true;
}


bool t_rpc_command_executor::ban(const std::string &ip, time_t seconds)
{
    cryptonote::COMMAND_RPC_SETBANS::request req;
    cryptonote::COMMAND_RPC_SETBANS::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    cryptonote::COMMAND_RPC_SETBANS::ban ban;
    if (!epee::string_tools::get_ip_int32_from_string(ban.ip, ip))
    {
        tools::fail_msg_writer() << "Invalid IP";
        return true;
    }
    ban.ban = true;
    ban.seconds = seconds;
    req.bans.push_back(ban);

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "set_bans", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_set_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    return true;
}

bool t_rpc_command_executor::unban(const std::string &ip)
{
    cryptonote::COMMAND_RPC_SETBANS::request req;
    cryptonote::COMMAND_RPC_SETBANS::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    cryptonote::COMMAND_RPC_SETBANS::ban ban;
    if (!epee::string_tools::get_ip_int32_from_string(ban.ip, ip))
    {
        tools::fail_msg_writer() << "Invalid IP";
        return true;
    }
    ban.ban = false;
    ban.seconds = 0;
    req.bans.push_back(ban);

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "set_bans", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_set_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    return true;
}

bool t_rpc_command_executor::flush_txpool(const std::string &txid)
{
    cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::request req;
    cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    if (!txid.empty())
      req.txids.push_back(txid);

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "flush_txpool", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_flush_txpool(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    return true;
}

bool t_rpc_command_executor::output_histogram(uint64_t min_count, uint64_t max_count)
{
    cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
    cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.min_count = min_count;
    req.max_count = max_count;
    req.unlocked = false;
    req.recent_cutoff = 0;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "get_output_histogram", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_get_output_histogram(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    std::sort(res.histogram.begin(), res.histogram.end(),
        [](const cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry &e1, const cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry &e2)->bool { return e1.total_instances < e2.total_instances; });
    for (const auto &e: res.histogram)
    {
        tools::msg_writer() << e.total_instances << "  " << cryptonote::print_money(e.amount);
    }

    return true;
}

bool t_rpc_command_executor::print_coinbase_tx_sum(uint64_t height, uint64_t count)
{
  cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::request req;
  cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::response res;
  epee::json_rpc::error error_resp;

  req.height = height;
  req.count = count;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "get_coinbase_tx_sum", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_coinbase_tx_sum(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "Sum of coinbase transactions between block heights ["
    << height << ", " << (height + count) << ") is "
    << cryptonote::print_money(res.emission_amount + res.fee_amount) << " "
    << "consisting of " << cryptonote::print_money(res.emission_amount) 
    << " in emissions, and " << cryptonote::print_money(res.fee_amount) << " in fees";
  return true;
}

bool t_rpc_command_executor::alt_chain_info()
{
  cryptonote::COMMAND_RPC_GET_INFO::request ireq;
  cryptonote::COMMAND_RPC_GET_INFO::response ires;
  cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::request req;
  cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::response res;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
    if (!m_rpc_client->json_rpc_request(req, res, "get_alternate_chains", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, ires.status);
      return true;
    }
    if (!m_rpc_server->on_get_alternate_chains(req, res, error_resp))
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << boost::lexical_cast<std::string>(res.chains.size()) << " alternate chains found:";
  for (const auto chain: res.chains)
  {
    uint64_t start_height = (chain.height - chain.length + 1);
    tools::msg_writer() << chain.length << " blocks long, from height " << start_height << " (" << (ires.height - start_height - 1)
        << " deep), diff " << chain.difficulty << ": " << chain.block_hash;
  }
  return true;
}

bool t_rpc_command_executor::print_blockchain_dynamic_stats(uint64_t nblocks)
{
  cryptonote::COMMAND_RPC_GET_INFO::request ireq;
  cryptonote::COMMAND_RPC_GET_INFO::response ires;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request bhreq;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response bhres;
  cryptonote::COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::request fereq;
  cryptonote::COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::response feres;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Problem fetching info";

  fereq.grace_blocks = 0;
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
    {
      return true;
    }
    if (!m_rpc_client->json_rpc_request(fereq, feres, "get_fee_estimate", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, ires.status);
      return true;
    }
    if (!m_rpc_server->on_get_per_kb_fee_estimate(fereq, feres, error_resp) || feres.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, feres.status);
      return true;
    }
  }

  tools::msg_writer() << "Height: " << ires.height << ", diff " << ires.difficulty << ", cum. diff " << ires.cumulative_difficulty
      << ", target " << ires.target << " sec" << ", dyn fee " << cryptonote::print_money(feres.fee) << "/kB";

  if (nblocks > 0)
  {
    if (nblocks > ires.height)
      nblocks = ires.height;

    bhreq.start_height = ires.height - nblocks;
    bhreq.end_height = ires.height - 1;
    if (m_is_rpc)
    {
      if (!m_rpc_client->json_rpc_request(bhreq, bhres, "getblockheadersrange", fail_message.c_str()))
      {
        return true;
      }
    }
    else
    {
      if (!m_rpc_server->on_get_block_headers_range(bhreq, bhres, error_resp) || bhres.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(fail_message, bhres.status);
        return true;
      }
    }

    double avgdiff = 0;
    double avgnumtxes = 0;
    double avgreward = 0;
    std::vector<uint64_t> sizes;
    sizes.reserve(nblocks);
    uint64_t earliest = std::numeric_limits<uint64_t>::max(), latest = 0;
    std::vector<unsigned> major_versions(256, 0), minor_versions(256, 0);
    for (const auto &bhr: bhres.headers)
    {
      avgdiff += bhr.difficulty;
      avgnumtxes += bhr.num_txes;
      avgreward += bhr.reward;
      sizes.push_back(bhr.block_size);
      static_assert(sizeof(bhr.major_version) == 1, "major_version expected to be uint8_t");
      static_assert(sizeof(bhr.minor_version) == 1, "major_version expected to be uint8_t");
      major_versions[(unsigned)bhr.major_version]++;
      minor_versions[(unsigned)bhr.minor_version]++;
      earliest = std::min(earliest, bhr.timestamp);
      latest = std::max(latest, bhr.timestamp);
    }
    avgdiff /= nblocks;
    avgnumtxes /= nblocks;
    avgreward /= nblocks;
    uint64_t median_block_size = epee::misc_utils::median(sizes);
    tools::msg_writer() << "Last " << nblocks << ": avg. diff " << (uint64_t)avgdiff << ", " << (latest - earliest) / nblocks << " avg sec/block, avg num txes " << avgnumtxes
        << ", avg. reward " << cryptonote::print_money(avgreward) << ", median block size " << median_block_size;

    unsigned int max_major = 256, max_minor = 256;
    while (max_major > 0 && !major_versions[--max_major]);
    while (max_minor > 0 && !minor_versions[--max_minor]);
    std::string s = "";
    for (unsigned n = 0; n <= max_major; ++n)
      if (major_versions[n])
        s += (s.empty() ? "" : ", ") + boost::lexical_cast<std::string>(major_versions[n]) + std::string(" v") + boost::lexical_cast<std::string>(n);
    tools::msg_writer() << "Block versions: " << s;
    s = "";
    for (unsigned n = 0; n <= max_minor; ++n)
      if (minor_versions[n])
        s += (s.empty() ? "" : ", ") + boost::lexical_cast<std::string>(minor_versions[n]) + std::string(" v") + boost::lexical_cast<std::string>(n);
    tools::msg_writer() << "Voting for: " << s;
  }
  return true;
}

bool t_rpc_command_executor::update(const std::string &command)
{
  cryptonote::COMMAND_RPC_UPDATE::request req;
  cryptonote::COMMAND_RPC_UPDATE::response res;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Problem fetching info";

  req.command = command;
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/update", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_update(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  if (!res.update)
  {
    tools::msg_writer() << "No update available";
    return true;
  }

  tools::msg_writer() << "Update available: v" << res.version << ": " << res.user_uri << ", hash " << res.hash;
  if (command == "check")
    return true;

  if (!res.path.empty())
    tools::msg_writer() << "Update downloaded to: " << res.path;
  else
    tools::msg_writer() << "Update download failed: " << res.status;
  if (command == "download")
    return true;

  tools::msg_writer() << "'update' not implemented yet";

  return true;
}

bool t_rpc_command_executor::relay_tx(const std::string &txid)
{
    cryptonote::COMMAND_RPC_RELAY_TX::request req;
    cryptonote::COMMAND_RPC_RELAY_TX::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.txids.push_back(txid);

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "relay_tx", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_relay_tx(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    return true;
}

bool t_rpc_command_executor::sync_info()
{
    cryptonote::COMMAND_RPC_SYNC_INFO::request req;
    cryptonote::COMMAND_RPC_SYNC_INFO::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "sync_info", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_sync_info(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    uint64_t target = res.target_height < res.height ? res.height : res.target_height;
    tools::success_msg_writer() << "Height: " << res.height << ", target: " << target << " (" << (100.0 * res.height / target) << "%)";
    uint64_t current_download = 0;
    for (const auto &p: res.peers)
      current_download += p.info.current_download;
    tools::success_msg_writer() << "Downloading at " << current_download << " kB/s";

    tools::success_msg_writer() << std::to_string(res.peers.size()) << " peers";
    for (const auto &p: res.peers)
    {
      std::string address = epee::string_tools::pad_string(p.info.address, 24);
      uint64_t nblocks = 0, size = 0;
      for (const auto &s: res.spans)
        if (s.rate > 0.0f && s.connection_id == p.info.connection_id)
          nblocks += s.nblocks, size += s.size;
      tools::success_msg_writer() << address << "  " << epee::string_tools::pad_string(p.info.peer_id, 16, '0', true) << "  " << p.info.height << "  "  << p.info.current_download << " kB/s, " << nblocks << " blocks / " << size/1e6 << " MB queued";
    }

    uint64_t total_size = 0;
    for (const auto &s: res.spans)
      total_size += s.size;
    tools::success_msg_writer() << std::to_string(res.spans.size()) << " spans, " << total_size/1e6 << " MB";
    for (const auto &s: res.spans)
    {
      std::string address = epee::string_tools::pad_string(s.remote_address, 24);
      if (s.size == 0)
      {
        tools::success_msg_writer() << address << "  " << s.nblocks << " (" << s.start_block_height << " - " << (s.start_block_height + s.nblocks - 1) << ")  -";
      }
      else
      {
        tools::success_msg_writer() << address << "  " << s.nblocks << " (" << s.start_block_height << " - " << (s.start_block_height + s.nblocks - 1) << ", " << (uint64_t)(s.size/1e3) << " kB)  " << (unsigned)(s.rate/1e3) << " kB/s (" << s.speed/100.0f << ")";
      }
    }

    return true;
}

}// namespace daemonize
