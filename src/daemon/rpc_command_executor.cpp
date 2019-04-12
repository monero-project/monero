// Copyright (c) 2014-2019, The Monero Project
// Copyright (c)      2018, The Loki Project
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
#include "common/pruning.h"
#include "daemon/rpc_command_executor.h"
#include "int-util.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/service_node_rules.h"
#include "cryptonote_basic/hardfork.h"
#include <boost/format.hpp>

#include "common/loki_integration_test_hooks.h"

#include <fstream>
#include <ctime>
#include <string>
#include <numeric>

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize {

namespace {

  enum class input_line_result { yes, no, cancel, back, };

  std::string input_line(std::string const &prompt)
  {
    std::cout << prompt << std::flush;
    std::string result;
#if defined (LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
    loki::write_redirected_stdout_to_shared_mem();
    loki::fixed_buffer buffer = loki::read_from_stdin_shared_mem();
    result.reserve(buffer.len);
    result = buffer.data;
#else
  #ifdef HAVE_READLINE
    rdln::suspend_readline pause_readline;
  #endif
    std::cin >> result;
#endif

    return result;
  }

  input_line_result input_line_yes_no_back_cancel(char const *msg)
  {
    std::string prompt = std::string(msg);
    prompt += " (Y/Yes/N/No/B/Back/C/Cancel): ";
    std::string input = input_line(prompt);

    if (command_line::is_yes(input))  return input_line_result::yes;
    if (command_line::is_no(input))   return input_line_result::no;
    if (command_line::is_back(input)) return input_line_result::back;
    return input_line_result::cancel;
  }

  input_line_result input_line_yes_no_cancel(char const *msg)
  {
    std::string prompt = msg;
    prompt += " (Y/Yes/N/No/C/Cancel): ";
    std::string input = input_line(prompt);

    if (command_line::is_yes(input)) return input_line_result::yes;
    if (command_line::is_no(input))  return input_line_result::no;
    return input_line_result::cancel;
  }


  input_line_result input_line_back_cancel_get_input(char const *msg, std::string &input)
  {
    std::string prompt = msg;
    prompt += " (B/Back/C/Cancel): ";
    input   = input_line(prompt);

    if (command_line::is_back(input))   return input_line_result::back;
    if (command_line::is_cancel(input)) return input_line_result::cancel;
    return input_line_result::yes;
  }

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
    std::string rpc_port = peer.rpc_port ? std::to_string(peer.rpc_port) : "-";
    std::string pruning_seed = epee::string_tools::to_string_hex(peer.pruning_seed);
    tools::msg_writer() << boost::format("%-10s %-25s %-25s %-5s %-4s %s") % prefix % id_str % addr_str % rpc_port % pruning_seed % elapsed;
  }

  void print_block_header(cryptonote::block_header_response const & header)
  {
    tools::success_msg_writer()
      << "timestamp: " << boost::lexical_cast<std::string>(header.timestamp) << " (" << tools::get_human_readable_timestamp(header.timestamp) << ")" << std::endl
      << "previous hash: " << header.prev_hash << std::endl
      << "nonce: " << boost::lexical_cast<std::string>(header.nonce) << std::endl
      << "is orphan: " << header.orphan_status << std::endl
      << "height: " << boost::lexical_cast<std::string>(header.height) << std::endl
      << "depth: " << boost::lexical_cast<std::string>(header.depth) << std::endl
      << "hash: " << header.hash << std::endl
      << "difficulty: " << boost::lexical_cast<std::string>(header.difficulty) << std::endl
      << "POW hash: " << header.pow_hash << std::endl
      << "block size: " << header.block_size << std::endl
      << "block weight: " << header.block_weight << std::endl
      << "long term weight: " << header.long_term_weight << std::endl
      << "num txes: " << header.num_txes << std::endl
      << "reward: " << cryptonote::print_money(header.reward);
  }

  std::string get_human_time_ago(time_t t, time_t now)
  {
    if (t == now)
      return "now";
    time_t dt = t > now ? t - now : now - t;
    std::string s;
    if (dt < 90)
      s = boost::lexical_cast<std::string>(dt) + (dt == 1 ? " second" : " seconds");
    else if (dt < 90 * 60)
      s = (boost::format("%.1f minutes") % ((float)dt/60)).str();
    else if (dt < 36 * 3600)
      s = (boost::format("%.1f hours") % ((float)dt/3600)).str();
    else
      s = (boost::format("%.1f days") % ((float)dt/(3600*24))).str();
    return s + " " + (t > now ? "in the future" : "ago");
  }

  char const *get_date_time(time_t t)
  {
    static char buf[128];
    buf[0] = 0;

    struct tm tm;
    epee::misc_utils::get_gmt_time(t, tm);
    strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S %p", &tm);
    return buf;
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

bool t_rpc_command_executor::print_peer_list(bool white, bool gray, size_t limit) {
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

  if (white)
  {
    auto peer = res.white_list.cbegin();
    const auto end = limit ? peer + std::min(limit, res.white_list.size()) : res.white_list.cend();
    for (; peer != end; ++peer)
    {
      print_peer("white", *peer);
    }
  }

  if (gray)
  {
    auto peer = res.gray_list.cbegin();
    const auto end = limit ? peer + std::min(limit, res.gray_list.size()) : res.gray_list.cend();
    for (; peer != end; ++peer)
    {
      print_peer("gray", *peer);
    }
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

static float get_sync_percentage(uint64_t height, uint64_t target_height)
{
  target_height = target_height ? target_height < height ? height : target_height : height;
  float pc = 100.0f * height / target_height;
  if (height < target_height && pc > 99.9f)
    return 99.9f; // to avoid 100% when not fully synced
  return pc;
}
static float get_sync_percentage(const cryptonote::COMMAND_RPC_GET_INFO::response &ires)
{
  return get_sync_percentage(ires.height, ires.target_height);
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
  std::string bootstrap_msg;
  if (ires.was_bootstrap_ever_used)
  {
    bootstrap_msg = ", bootstrapping from " + ires.bootstrap_daemon_address;
    if (ires.untrusted)
    {
      bootstrap_msg += (boost::format(", local height: %llu (%.1f%%)") % ires.height_without_bootstrap % get_sync_percentage(ires.height_without_bootstrap, net_height)).str();
    }
    else
    {
      bootstrap_msg += " was used before";
    }
  }

  std::stringstream str;
  str << boost::format("Height: %llu/%llu (%.1f%%) on %s%s, %s, net hash %s, v%u%s, %s, %u(out)+%u(in) connections")
    % (unsigned long long)ires.height
    % (unsigned long long)net_height
    % get_sync_percentage(ires)
    % (ires.testnet ? "testnet" : ires.stagenet ? "stagenet" : "mainnet")
    % bootstrap_msg
    % (!has_mining_info ? "mining info unavailable" : mining_busy ? "syncing" : mres.active ? ( ( mres.is_background_mining_enabled ? "smart " : "" ) + std::string("mining at ") + get_mining_speed(mres.speed)) : "not mining")
    % get_mining_speed(ires.difficulty / ires.target)
    % (unsigned)hfres.version
    % get_fork_extra_info(hfres.earliest_height, net_height, ires.target)
    % (hfres.state == cryptonote::HardFork::Ready ? "up to date" : hfres.state == cryptonote::HardFork::UpdateNeeded ? "update needed" : "out of date, likely forked")
    % (unsigned)ires.outgoing_connections_count
    % (unsigned)ires.incoming_connections_count
  ;

  // restricted RPC does not disclose start time
  if (ires.start_time)
  {
    str << boost::format(", uptime %ud %uh %um %us")
      % (unsigned int)floor(uptime / 60.0 / 60.0 / 24.0)
      % (unsigned int)floor(fmod((uptime / 60.0 / 60.0), 24.0))
      % (unsigned int)floor(fmod((uptime / 60.0), 60.0))
      % (unsigned int)fmod(uptime, 60.0)
    ;
  }

  tools::success_msg_writer() << str.str();

  return true;
}

bool t_rpc_command_executor::mining_status() {
  cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
  cryptonote::COMMAND_RPC_MINING_STATUS::response mres;
  epee::json_rpc::error error_resp;
  bool has_mining_info = true;

  std::string fail_message = "Problem fetching info";

  bool mining_busy = false;
  if (m_is_rpc)
  {
    // mining info is only available non unrestricted RPC mode
    has_mining_info = m_rpc_client->rpc_request(mreq, mres, "/mining_status", fail_message.c_str());
  }
  else
  {
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

  if (!has_mining_info)
  {
    tools::fail_msg_writer() << "Mining info unavailable";
    return true;
  }

  if (mining_busy || !mres.active)
  {
    tools::msg_writer() << "Not currently mining";
  }
  else
  {
    tools::msg_writer() << "Mining at " << get_mining_speed(mres.speed) << " with " << mres.threads_count << " threads";
  }

  if (mres.active || mres.is_background_mining_enabled)
  {
    tools::msg_writer() << "PoW algorithm: " << mres.pow_algorithm;
    tools::msg_writer() << "Mining address: " << mres.address;
  }

  if (mres.is_background_mining_enabled)
  {
    tools::msg_writer() << "Smart mining enabled:";
    tools::msg_writer() << "  Target: " << (unsigned)mres.bg_target << "% CPU";
    tools::msg_writer() << "  Idle threshold: " << (unsigned)mres.bg_idle_threshold << "% CPU";
    tools::msg_writer() << "  Min idle time: " << (unsigned)mres.bg_min_idle_seconds << " seconds";
    tools::msg_writer() << "  Ignore battery: " << (mres.bg_ignore_battery ? "yes" : "no");
  }

  if (!mining_busy && mres.active && mres.speed > 0 && mres.block_target > 0 && mres.difficulty > 0)
  {
    double ratio = mres.speed * mres.block_target / mres.difficulty;
    uint64_t daily = 86400ull / mres.block_target * mres.block_reward * ratio;
    uint64_t monthly = 86400ull / mres.block_target * 30.5 * mres.block_reward * ratio;
    uint64_t yearly = 86400ull / mres.block_target * 356 * mres.block_reward * ratio;
    tools::msg_writer() << "Expected: " << cryptonote::print_money(daily) << " monero daily, "
        << cryptonote::print_money(monthly) << " monero monthly, " << cryptonote::print_money(yearly) << " yearly";
  }

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
      << std::setw(6) << "SSL"
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
     << std::setw(6) << (info.ssl ? "yes" : "no")
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

bool t_rpc_command_executor::print_net_stats()
{
  cryptonote::COMMAND_RPC_GET_NET_STATS::request net_stats_req;
  cryptonote::COMMAND_RPC_GET_NET_STATS::response net_stats_res;
  cryptonote::COMMAND_RPC_GET_LIMIT::request limit_req;
  cryptonote::COMMAND_RPC_GET_LIMIT::response limit_res;

  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(net_stats_req, net_stats_res, "get_net_stats", fail_message.c_str()))
    {
      return true;
    }
    if (!m_rpc_client->json_rpc_request(limit_req, limit_res, "get_limit", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_net_stats(net_stats_req, net_stats_res) || net_stats_res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, net_stats_res.status);
      return true;
    }
    if (!m_rpc_server->on_get_limit(limit_req, limit_res) || limit_res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, limit_res.status);
      return true;
    }
  }

  uint64_t seconds = (uint64_t)time(NULL) - net_stats_res.start_time;
  uint64_t average = seconds > 0 ? net_stats_res.total_bytes_in / seconds : 0;
  uint64_t limit = limit_res.limit_down * 1024;   // convert to bytes, as limits are always kB/s
  double percent = (double)average / (double)limit * 100.0;
  tools::success_msg_writer() << boost::format("Received %u bytes (%s) in %u packets, average %s/s = %.2f%% of the limit of %s/s")
    % net_stats_res.total_bytes_in
    % tools::get_human_readable_bytes(net_stats_res.total_bytes_in)
    % net_stats_res.total_packets_in
    % tools::get_human_readable_bytes(average)
    % percent
    % tools::get_human_readable_bytes(limit);

  average = seconds > 0 ? net_stats_res.total_bytes_out / seconds : 0;
  limit = limit_res.limit_up * 1024;
  percent = (double)average / (double)limit * 100.0;
  tools::success_msg_writer() << boost::format("Sent %u bytes (%s) in %u packets, average %s/s = %.2f%% of the limit of %s/s")
    % net_stats_res.total_bytes_out
    % tools::get_human_readable_bytes(net_stats_res.total_bytes_out)
    % net_stats_res.total_packets_out
    % tools::get_human_readable_bytes(average)
    % percent
    % tools::get_human_readable_bytes(limit);

  return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index) {
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;
  epee::json_rpc::error error_resp;

  req.start_height = start_block_index;
  req.end_height = end_block_index;
  req.fill_pow_hash = false;

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
      tools::msg_writer() << "" << std::endl;
    tools::msg_writer()
      << "height: " << header.height << ", timestamp: " << header.timestamp << " (" << tools::get_human_readable_timestamp(header.timestamp) << ")"
      << ", size: " << header.block_size << ", weight: " << header.block_weight << ", transactions: " << header.num_txes << std::endl
      << "major version: " << (unsigned)header.major_version << ", minor version: " << (unsigned)header.minor_version << std::endl
      << "block id: " << header.hash << ", previous block id: " << header.prev_hash << std::endl
      << "difficulty: " << header.difficulty << ", nonce " << header.nonce << ", reward " << cryptonote::print_money(header.reward) << std::endl;
    first = false;
  }

  return true;
}

bool t_rpc_command_executor::print_quorum_state(uint64_t height)
{
  cryptonote::COMMAND_RPC_GET_QUORUM_STATE::request req;
  cryptonote::COMMAND_RPC_GET_QUORUM_STATE::response res;
  epee::json_rpc::error error_resp;

  req.height = height;
  std::string fail_message = "Unsuccessful";

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "get_quorum_state", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_quorum_state(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "Quorum Service Nodes [" << res.quorum_nodes.size() << "]";
  for (size_t i = 0; i < res.quorum_nodes.size(); i++)
  {
    const std::string &entry = res.quorum_nodes[i];
    tools::msg_writer() << "[" << i << "] " << entry;
  }

  tools::msg_writer() << "Service Nodes To Test [" << res.nodes_to_test.size() << "]";
  for (size_t i = 0; i < res.nodes_to_test.size(); i++)
  {
    const std::string &entry = res.nodes_to_test[i];
    tools::msg_writer() << "[" << i << "] " << entry;
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

  tools::success_msg_writer() << "Log categories are now " << res.categories;

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

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash, bool include_hex) {
  cryptonote::COMMAND_RPC_GET_BLOCK::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK::response res;
  epee::json_rpc::error error_resp;

  req.hash = epee::string_tools::pod_to_hex(block_hash);
  req.fill_pow_hash = true;

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

  if (include_hex)
    tools::success_msg_writer() << res.blob << std::endl;
  print_block_header(res.block_header);
  tools::success_msg_writer() << res.json << ENDL;

  return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height, bool include_hex) {
  cryptonote::COMMAND_RPC_GET_BLOCK::request req;
  cryptonote::COMMAND_RPC_GET_BLOCK::response res;
  epee::json_rpc::error error_resp;

  req.height = height;
  req.fill_pow_hash = true;

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

  if (include_hex)
    tools::success_msg_writer() << res.blob << std::endl;
  print_block_header(res.block_header);
  tools::success_msg_writer() << res.json << ENDL;

  return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash,
  bool include_hex,
  bool include_json) {
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  std::string fail_message = "Problem fetching transaction";

  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(transaction_hash));
  req.decode_as_json = false;
  req.split = true;
  req.prune = false;
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
        tools::success_msg_writer() << "Found in blockchain at height " << res.txs.front().block_height << (res.txs.front().prunable_as_hex.empty() ? " (pruned)" : "");
    }

    const std::string &as_hex = (1 == res.txs.size()) ? res.txs.front().as_hex : res.txs_as_hex.front();
    const std::string &pruned_as_hex = (1 == res.txs.size()) ? res.txs.front().pruned_as_hex : "";
    const std::string &prunable_as_hex = (1 == res.txs.size()) ? res.txs.front().prunable_as_hex : "";
    // Print raw hex if requested
    if (include_hex)
    {
      if (!as_hex.empty())
      {
        tools::success_msg_writer() << as_hex << std::endl;
      }
      else
      {
        std::string output = pruned_as_hex + prunable_as_hex;
        tools::success_msg_writer() << output << std::endl;
      }
    }

    // Print json if requested
    if (include_json)
    {
      crypto::hash tx_hash, tx_prefix_hash;
      cryptonote::transaction tx;
      cryptonote::blobdata blob;
      std::string source = as_hex.empty() ? pruned_as_hex + prunable_as_hex : as_hex;
      bool pruned = !pruned_as_hex.empty() && prunable_as_hex.empty();
      if (!string_tools::parse_hexstr_to_binbuff(source, blob))
      {
        tools::fail_msg_writer() << "Failed to parse tx to get json format";
      }
      else
      {
        bool ret;
        if (pruned)
          ret = cryptonote::parse_and_validate_tx_base_from_blob(blob, tx);
        else
          ret = cryptonote::parse_and_validate_tx_from_blob(blob, tx);
        if (!ret)
        {
          tools::fail_msg_writer() << "Failed to parse tx blob to get json format";
        }
        else
        {
          tools::success_msg_writer() << cryptonote::obj_to_json_str(tx) << std::endl;
        }
      }
    }
  }
  else
  {
    tools::fail_msg_writer() << "Transaction wasn't found: " << transaction_hash << std::endl;
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
    tools::success_msg_writer() << ki << ": " << (res.spent_status.front() ? "spent" : "unspent") << (res.spent_status.front() == cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_POOL ? " (in pool)" : "");
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
                          << "weight: " << tx_info.weight << std::endl
                          << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                          << "fee/byte: " << cryptonote::print_money(tx_info.fee / (double)tx_info.weight) << std::endl
                          << "receive_time: " << tx_info.receive_time << " (" << get_human_time_ago(tx_info.receive_time, now) << ")" << std::endl
                          << "relayed: " << [&](const cryptonote::tx_info &tx_info)->std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; } (tx_info) << std::endl
                          << "do_not_relay: " << (tx_info.do_not_relay ? 'T' : 'F')  << std::endl
                          << "kept_by_block: " << (tx_info.kept_by_block ? 'T' : 'F') << std::endl
                          << "double_spend_seen: " << (tx_info.double_spend_seen ? 'T' : 'F')  << std::endl
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
                          << "weight: " << tx_info.weight << std::endl
                          << "fee: " << cryptonote::print_money(tx_info.fee) << std::endl
                          << "fee/byte: " << cryptonote::print_money(tx_info.fee / (double)tx_info.weight) << std::endl
                          << "receive_time: " << tx_info.receive_time << " (" << get_human_time_ago(tx_info.receive_time, now) << ")" << std::endl
                          << "relayed: " << [&](const cryptonote::tx_info &tx_info)->std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; } (tx_info) << std::endl
                          << "do_not_relay: " << (tx_info.do_not_relay ? 'T' : 'F')  << std::endl
                          << "kept_by_block: " << (tx_info.kept_by_block ? 'T' : 'F') << std::endl
                          << "double_spend_seen: " << (tx_info.double_spend_seen ? 'T' : 'F') << std::endl
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
    res.pool_stats = {};
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
  const uint64_t full_reward_zone = ires.block_weight_limit / 2;
  if (res.pool_stats.bytes_total <= full_reward_zone)
  {
    backlog_message = "no backlog";
  }
  else
  {
    uint64_t backlog = (res.pool_stats.bytes_total + full_reward_zone - 1) / full_reward_zone;
    backlog_message = (boost::format("estimated %u block (%u minutes) backlog") % backlog % (backlog * DIFFICULTY_TARGET_V2 / 60)).str();
  }

  tools::msg_writer() << n_transactions << " tx(es), " << res.pool_stats.bytes_total << " bytes total (min " << res.pool_stats.bytes_min << ", max " << res.pool_stats.bytes_max << ", avg " << avg_bytes << ", median " << res.pool_stats.bytes_med << ")" << std::endl
      << "fees " << cryptonote::print_money(res.pool_stats.fee_total) << " (avg " << cryptonote::print_money(n_transactions ? res.pool_stats.fee_total / n_transactions : 0) << " per tx" << ", " << cryptonote::print_money(res.pool_stats.bytes_total ? res.pool_stats.fee_total / res.pool_stats.bytes_total : 0) << " per byte)" << std::endl
      << res.pool_stats.num_double_spends << " double spends, " << res.pool_stats.num_not_relayed << " not relayed, " << res.pool_stats.num_failing << " failing, " << res.pool_stats.num_10m << " older than 10 minutes (oldest " << (res.pool_stats.oldest == 0 ? "-" : get_human_time_ago(res.pool_stats.oldest, now)) << "), " << backlog_message;

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
      tools::msg_writer() << get_time_hms(times[i]) << std::setw(8) << res.pool_stats.histo[i].txs << std::setw(12) << res.pool_stats.histo[i].bytes;
    }
  }
  tools::msg_writer();

  return true;
}

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads, cryptonote::network_type nettype, bool do_background_mining, bool ignore_battery) {
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;
  req.miner_address = cryptonote::get_account_address_as_str(nettype, false, address);
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
//    bool ok = windows::stop_service("Loki Daemon");
//    ok = windows::uninstall_service("Loki Daemon");
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
    tools::success_msg_writer() << "lokid is running";
  }
  else {
    tools::fail_msg_writer() << "lokid is NOT running";
  }

  return true;
}

bool t_rpc_command_executor::get_limit()
{
  cryptonote::COMMAND_RPC_GET_LIMIT::request req;
  cryptonote::COMMAND_RPC_GET_LIMIT::response res;

  std::string failure_message = "Couldn't get limit";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(failure_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "limit-down is " << res.limit_down << " kB/s";
  tools::msg_writer() << "limit-up is " << res.limit_up << " kB/s";
  return true;
}

bool t_rpc_command_executor::set_limit(int64_t limit_down, int64_t limit_up)
{
  cryptonote::COMMAND_RPC_SET_LIMIT::request req;
  cryptonote::COMMAND_RPC_SET_LIMIT::response res;

  req.limit_down = limit_down;
  req.limit_up = limit_up;

  std::string failure_message = "Couldn't set limit";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/set_limit", failure_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_set_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(failure_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "Set limit-down to " << res.limit_down << " kB/s";
  tools::msg_writer() << "Set limit-up to " << res.limit_up << " kB/s";
  return true;
}

bool t_rpc_command_executor::get_limit_up()
{
  cryptonote::COMMAND_RPC_GET_LIMIT::request req;
  cryptonote::COMMAND_RPC_GET_LIMIT::response res;

  std::string failure_message = "Couldn't get limit";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(failure_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "limit-up is " << res.limit_up << " kB/s";
  return true;
}

bool t_rpc_command_executor::get_limit_down()
{
  cryptonote::COMMAND_RPC_GET_LIMIT::request req;
  cryptonote::COMMAND_RPC_GET_LIMIT::response res;

  std::string failure_message = "Couldn't get limit";

  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(failure_message, res.status);
      return true;
    }
  }

  tools::msg_writer() << "limit-down is " << res.limit_down << " kB/s";
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
		if (!m_rpc_client->rpc_request(req, res, "/out_peers", fail_message.c_str()))
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

	tools::msg_writer() << "Max number of out peers set to " << limit << std::endl;

	return true;
}

bool t_rpc_command_executor::in_peers(uint64_t limit)
{
	cryptonote::COMMAND_RPC_IN_PEERS::request req;
	cryptonote::COMMAND_RPC_IN_PEERS::response res;

	epee::json_rpc::error error_resp;

	req.in_peers = limit;

	std::string fail_message = "Unsuccessful";

	if (m_is_rpc)
	{
		if (!m_rpc_client->rpc_request(req, res, "/in_peers", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if (!m_rpc_server->on_in_peers(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			tools::fail_msg_writer() << make_error(fail_message, res.status);
			return true;
		}
	}

	tools::msg_writer() << "Max number of in peers set to " << limit << std::endl;

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

	tools::success_msg_writer() << "Saving graph is now on";
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
	tools::success_msg_writer() << "Saving graph is now off";
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

    tools::success_msg_writer() << "Pool successfully flushed";
    return true;
}

bool t_rpc_command_executor::output_histogram(const std::vector<uint64_t> &amounts, uint64_t min_count, uint64_t max_count)
{
    cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
    cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.amounts = amounts;
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

bool t_rpc_command_executor::alt_chain_info(const std::string &tip)
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

  if (tip.empty())
  {
    tools::msg_writer() << boost::lexical_cast<std::string>(res.chains.size()) << " alternate chains found:";
    for (const auto &chain: res.chains)
    {
      uint64_t start_height = (chain.height - chain.length + 1);
      tools::msg_writer() << chain.length << " blocks long, from height " << start_height << " (" << (ires.height - start_height - 1)
          << " deep), diff " << chain.difficulty << ": " << chain.block_hash;
    }
  }
  else
  {
    const auto i = std::find_if(res.chains.begin(), res.chains.end(), [&tip](cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::chain_info &info){ return info.block_hash == tip; });
    if (i != res.chains.end())
    {
      const auto &chain = *i;
      tools::success_msg_writer() << "Found alternate chain with tip " << tip;
      uint64_t start_height = (chain.height - chain.length + 1);
      tools::msg_writer() << chain.length << " blocks long, from height " << start_height << " (" << (ires.height - start_height - 1)
          << " deep), diff " << chain.difficulty << ":";
      for (const std::string &block_id: chain.block_hashes)
        tools::msg_writer() << "  " << block_id;
      tools::msg_writer() << "Chain parent on main chain: " << chain.main_chain_parent_block;
    }
    else
      tools::fail_msg_writer() << "Block hash " << tip << " is not the tip of any known alternate chain";
  }
  return true;
}

bool t_rpc_command_executor::print_blockchain_dynamic_stats(uint64_t nblocks)
{
  cryptonote::COMMAND_RPC_GET_INFO::request ireq;
  cryptonote::COMMAND_RPC_GET_INFO::response ires;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request bhreq;
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response bhres;
  cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request fereq;
  cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response feres;
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::request hfreq;
  cryptonote::COMMAND_RPC_HARD_FORK_INFO::response hfres;
  epee::json_rpc::error error_resp;

  std::string fail_message = "Problem fetching info";

  fereq.grace_blocks = 0;
  hfreq.version = HF_VERSION_PER_BYTE_FEE;
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
    if (!m_rpc_client->json_rpc_request(hfreq, hfres, "hard_fork_info", fail_message.c_str()))
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
    if (!m_rpc_server->on_get_base_fee_estimate(fereq, feres, error_resp) || feres.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, feres.status);
      return true;
    }
    if (!m_rpc_server->on_hard_fork_info(hfreq, hfres, error_resp) || hfres.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, hfres.status);
      return true;
    }
  }

  tools::msg_writer() << "Height: " << ires.height << ", diff " << ires.difficulty << ", cum. diff " << ires.cumulative_difficulty
      << ", target " << ires.target << " sec" << ", dyn fee " << cryptonote::print_money(feres.fee) << "/" << (hfres.enabled ? "byte" : "kB");

  if (nblocks > 0)
  {
    if (nblocks > ires.height)
      nblocks = ires.height;

    bhreq.start_height = ires.height - nblocks;
    bhreq.end_height = ires.height - 1;
    bhreq.fill_pow_hash = false;
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
    std::vector<uint64_t> weights;
    weights.reserve(nblocks);
    uint64_t earliest = std::numeric_limits<uint64_t>::max(), latest = 0;
    std::vector<unsigned> major_versions(256, 0), minor_versions(256, 0);
    for (const auto &bhr: bhres.headers)
    {
      avgdiff += bhr.difficulty;
      avgnumtxes += bhr.num_txes;
      avgreward += bhr.reward;
      weights.push_back(bhr.block_weight);
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
    uint64_t median_block_weight = epee::misc_utils::median(weights);
    tools::msg_writer() << "Last " << nblocks << ": avg. diff " << (uint64_t)avgdiff << ", " << (latest - earliest) / nblocks << " avg sec/block, avg num txes " << avgnumtxes
        << ", avg. reward " << cryptonote::print_money(avgreward) << ", median block weight " << median_block_weight;

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

    tools::success_msg_writer() << "Transaction successfully relayed";
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
    if (res.next_needed_pruning_seed)
      tools::success_msg_writer() << "Next needed pruning seed: " << res.next_needed_pruning_seed;

    tools::success_msg_writer() << std::to_string(res.peers.size()) << " peers";
    for (const auto &p: res.peers)
    {
      std::string address = epee::string_tools::pad_string(p.info.address, 24);
      uint64_t nblocks = 0, size = 0;
      for (const auto &s: res.spans)
        if (s.connection_id == p.info.connection_id)
          nblocks += s.nblocks, size += s.size;
      tools::success_msg_writer() << address << "  " << epee::string_tools::pad_string(p.info.peer_id, 16, '0', true) << "  " <<
          epee::string_tools::pad_string(p.info.state, 16) << "  " <<
          epee::string_tools::pad_string(epee::string_tools::to_string_hex(p.info.pruning_seed), 8) << "  " << p.info.height << "  "  <<
          p.info.current_download << " kB/s, " << nblocks << " blocks / " << size/1e6 << " MB queued";
    }

    uint64_t total_size = 0;
    for (const auto &s: res.spans)
      total_size += s.size;
    tools::success_msg_writer() << std::to_string(res.spans.size()) << " spans, " << total_size/1e6 << " MB";
    tools::success_msg_writer() << res.overview;
    for (const auto &s: res.spans)
    {
      std::string address = epee::string_tools::pad_string(s.remote_address, 24);
      std::string pruning_seed = epee::string_tools::to_string_hex(tools::get_pruning_seed(s.start_block_height, std::numeric_limits<uint64_t>::max(), CRYPTONOTE_PRUNING_LOG_STRIPES));
      if (s.size == 0)
      {
        tools::success_msg_writer() << address << "  " << s.nblocks << "/" << pruning_seed << " (" << s.start_block_height << " - " << (s.start_block_height + s.nblocks - 1) << ")  -";
      }
      else
      {
        tools::success_msg_writer() << address << "  " << s.nblocks << "/" << pruning_seed << " (" << s.start_block_height << " - " << (s.start_block_height + s.nblocks - 1) << ", " << (uint64_t)(s.size/1e3) << " kB)  " << (unsigned)(s.rate/1e3) << " kB/s (" << s.speed/100.0f << ")";
      }
    }

    return true;
}

static void append_printable_service_node_list_entry(cryptonote::network_type nettype, int hard_fork_version, uint64_t curr_height, uint64_t entry_index, cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry const &entry, std::string &buffer)
{
  const char indent1[] = "    ";
  const char indent2[] = "        ";
  const char indent3[] = "            ";
  bool is_registered = entry.total_contributed >= entry.staking_requirement;

  // Print Funding Status
  {
    buffer.append(indent1);
    buffer.append("[");
    buffer.append(std::to_string(entry_index));
    buffer.append("] Service Node: ");
    buffer.append(entry.service_node_pubkey);
    buffer.append("\n");

    buffer.append(indent2);
    buffer.append("Total Contributed/Staking Requirement: ");
    buffer.append(cryptonote::print_money(entry.total_contributed));
    buffer.append("/");
    buffer.append(cryptonote::print_money(entry.staking_requirement));
    buffer.append("\n");

    buffer.append(indent2);
    buffer.append("Total Reserved: ");
    buffer.append(cryptonote::print_money(entry.total_reserved));
    buffer.append("\n");
  }

  // Print expiry information
  {
    uint64_t const now = time(nullptr);
    uint64_t expiry_height = 0;
    if (hard_fork_version >= cryptonote::network_version_11_infinite_staking)
    {
      expiry_height = entry.requested_unlock_height;
    }
    else if (hard_fork_version >= cryptonote::network_version_10_bulletproofs)
    {
        expiry_height = entry.registration_height + service_nodes::staking_num_lock_blocks(nettype);
        expiry_height += STAKING_REQUIREMENT_LOCK_BLOCKS_EXCESS;
    }
    else
    {
        expiry_height = entry.registration_height + service_nodes::staking_num_lock_blocks(nettype);
    }

    buffer.append(indent2);
    buffer.append("Register/Expiry Height: ");
    if (expiry_height == service_nodes::KEY_IMAGE_AWAITING_UNLOCK_HEIGHT)
    {
        buffer.append("Staking Infinitely (stake unlock not requested yet)\n");
    }
    else
    {
      uint64_t delta_height      = expiry_height - curr_height;
      uint64_t expiry_epoch_time = now + (delta_height * DIFFICULTY_TARGET_V2);

      buffer.append(std::to_string(entry.registration_height));
      buffer.append("/");
      buffer.append(std::to_string(expiry_height));
      buffer.append(" (in ");
      buffer.append(std::to_string(delta_height));
      buffer.append(") blocks\n");

      buffer.append(indent2);
      buffer.append("Expiry Date (Est. UTC): ");
      buffer.append(get_date_time(expiry_epoch_time));
      buffer.append(" (");
      buffer.append(get_human_time_ago(expiry_epoch_time, now));
      buffer.append(")\n");
    }
  }

  if (is_registered) // Print reward status
  {
    buffer.append(indent2);
    buffer.append("Last Reward At (Height/TX Index): ");
    buffer.append(std::to_string(entry.last_reward_block_height));
    buffer.append("/");
    buffer.append(std::to_string(entry.last_reward_transaction_index));
    buffer.append("\n");
  }

  // Print operator information
  {
    buffer.append(indent2);
    buffer.append("Operator Cut (\% Of Reward): ");
    buffer.append(std::to_string((entry.portions_for_operator / (double)STAKING_PORTIONS) * 100.0));
    buffer.append("%\n");

    buffer.append(indent2);
    buffer.append("Operator Address: ");
    buffer.append(entry.operator_address);
    buffer.append("\n");
  }

  if (is_registered) // Print service node tests
  {
    epee::console_colors uptime_proof_color = (entry.last_uptime_proof == 0) ? epee::console_color_red : epee::console_color_green;

    buffer.append(indent2);
    if (entry.last_uptime_proof == 0)
    {
      buffer.append("Last Uptime Proof Received: Not Received Yet");
    }
    else
    {
      buffer.append("Last Uptime Proof Received: ");
      buffer.append(get_human_time_ago(entry.last_uptime_proof, time(nullptr)));
    }
    buffer.append("\n");
  }

  // Print contributors
  {
    buffer.append("\n");
    for (size_t j = 0; j < entry.contributors.size(); ++j)
    {
      const cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::contributor &contributor = entry.contributors[j];

      buffer.append(indent2);
      buffer.append("[");
      buffer.append(std::to_string(j));
      buffer.append("] Contributor: ");
      buffer.append(contributor.address);
      buffer.append("\n");

      buffer.append(indent3);
      buffer.append("Amount / Reserved: ");
      buffer.append(cryptonote::print_money(contributor.amount));
      buffer.append("/");
      buffer.append(cryptonote::print_money(contributor.reserved));
      buffer.append("\n");
    }
  }
}

bool t_rpc_command_executor::print_sn(const std::vector<std::string> &args)
{
    cryptonote::COMMAND_RPC_GET_SERVICE_NODES::request req = {};
    cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response res = {};
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;
    
    for (unsigned int i=0; i<args.size(); ++i) 
    {
      if (!(args[i] == "+json")) 
      {
        req.service_node_pubkeys.push_back(args[i]);
      }
      else {
        req.include_json = true;
      }
    }

    cryptonote::COMMAND_RPC_GET_INFO::request get_info_req;
    cryptonote::COMMAND_RPC_GET_INFO::response get_info_res;

    cryptonote::network_type nettype = cryptonote::UNDEFINED;
    uint64_t curr_height  = 0;
    if (m_is_rpc)
    {
      if (!m_rpc_client->rpc_request(get_info_req, get_info_res, "/getinfo", fail_message.c_str()))
      {
        tools::fail_msg_writer() << make_error(fail_message, get_info_res.status);
        return true;
      }

      if (!m_rpc_client->json_rpc_request(req, res, "get_service_nodes", fail_message.c_str()))
      {
        tools::fail_msg_writer() << make_error(fail_message, res.status);
        return true;
      }

      if (get_info_res.mainnet) nettype       = cryptonote::MAINNET;
      else if (get_info_res.stagenet) nettype = cryptonote::STAGENET;
      else if (get_info_res.testnet) nettype  = cryptonote::TESTNET;
      curr_height = get_info_res.height;
    }
    else
    {
      if (m_rpc_server->on_get_info(get_info_req, get_info_res) || get_info_res.status == CORE_RPC_STATUS_OK)
        curr_height = get_info_res.height;

      if (!m_rpc_server->on_get_service_nodes(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
      {
          tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
          return true;
      }
      nettype = m_rpc_server->nettype();
    }

    std::vector<cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *> unregistered;
    std::vector<cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *> registered;
    registered.reserve  (res.service_node_states.size());
    unregistered.reserve(res.service_node_states.size() * 0.5f);

    for (auto &entry : res.service_node_states)
    {
      if (entry.total_contributed == entry.staking_requirement)
      {
        registered.push_back(&entry);
      }
      else
      {
        unregistered.push_back(&entry);
      }
    }

    std::sort(unregistered.begin(), unregistered.end(),
        [](const cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *a, const cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *b) {
        uint64_t a_remaining = a->staking_requirement - a->total_reserved;
        uint64_t b_remaining = b->staking_requirement - b->total_reserved;

        if (b_remaining == a_remaining)
          return b->portions_for_operator < a->portions_for_operator;

        return b_remaining < a_remaining;
    });

    std::stable_sort(registered.begin(), registered.end(),
        [](const cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *a, const cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry *b) {
        if (a->last_reward_block_height == b->last_reward_block_height)
          return a->last_reward_transaction_index < b->last_reward_transaction_index;

        return a->last_reward_block_height < b->last_reward_block_height;
    });

    if (req.include_json)
    {
      std::cout << res.as_json << std::endl;
      return true;
    }

    if (unregistered.size() == 0 && registered.size() == 0)
    {
      if (req.service_node_pubkeys.size() > 0)
      {
        int str_size = 0;
        for (const std::string &arg : args) str_size += (arg.size() + 2);

        std::string buffer;
        buffer.reserve(str_size);
        for (size_t i = 0; i < args.size(); ++i)
        {
          buffer.append(args[i]);
          if (i < args[i].size() - 1) buffer.append(", ");
        }

        tools::msg_writer() << "No service node is currently known on the network: " << buffer;
      }
      else
      {
        tools::msg_writer() << "No service node is currently known on the network";
      }

      return true;
    }

    using hard_fork_height = uint64_t;
    std::array<hard_fork_height, cryptonote::network_version_count> hf_heights = {};
    for (size_t version = cryptonote::network_version_9_service_nodes; version < hf_heights.size(); ++version)
    {
      cryptonote::COMMAND_RPC_HARD_FORK_INFO::request  request  = {};
      cryptonote::COMMAND_RPC_HARD_FORK_INFO::response response = {};
      request.version                                           = version;

      if (m_is_rpc)
      {
        if (!m_rpc_client->json_rpc_request(request, response, "hard_fork_info", fail_message.c_str()))
        {
          tools::fail_msg_writer() << make_error(fail_message, response.status);
          return true;
        }
      }
      else
      {
        if (!m_rpc_server->on_hard_fork_info(request, response, error_resp) || response.status != CORE_RPC_STATUS_OK)
        {
          tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
          return true;
        }
      }

      hf_heights[request.version] = response.earliest_height;
    }

    auto const find_closest_hardfork = [](std::array<hard_fork_height, cryptonote::network_version_count> const &hf_heights, uint64_t registration_height) -> size_t {
      uint64_t result = cryptonote::network_version_count - 1;
      for (int version = cryptonote::network_version_7; version < cryptonote::network_version_count; version++)
      {
        uint64_t hf_height = hf_heights[version];
        if (registration_height < hf_height) break;
        result = version;
      }
      return result;
    };

    std::string unregistered_print_data;
    std::string registered_print_data;
    for (size_t i = 0; i < unregistered.size(); i++)
    {
      cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry const &entry = (*unregistered[i]);
      size_t hf_version = find_closest_hardfork(hf_heights, entry.registration_height);
      append_printable_service_node_list_entry(nettype, hf_version, curr_height, i, entry, unregistered_print_data);
      if (i < unregistered.size())
        unregistered_print_data.append("\n");
    }

    for (size_t i = 0; i < registered.size(); i++)
    {
      cryptonote::COMMAND_RPC_GET_SERVICE_NODES::response::entry const &entry = (*registered[i]);
      size_t hf_version = find_closest_hardfork(hf_heights, entry.registration_height);
      append_printable_service_node_list_entry(nettype, hf_version, curr_height, i, entry, registered_print_data);
      if (i < registered.size())
        registered_print_data.append("\n");
    }

    if (unregistered.size() > 0)
      tools::msg_writer() << "Service Node Unregistered State [" << unregistered.size() << "]\n" << unregistered_print_data;

    if (registered.size() > 0)
      tools::msg_writer() << "Service Node Registration State [" << registered.size() << "]\n"   << registered_print_data;

    return true;
}

bool t_rpc_command_executor::print_sn_status(const std::vector<std::string>& args)
{
  cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::response res = {};
  {
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::request req = {};
    std::string fail_message = "Unsuccessful";

    if (m_is_rpc)
    {
      if (!m_rpc_client->json_rpc_request(req, res, "get_service_node_key", fail_message.c_str()))
      {
        tools::fail_msg_writer() << make_error(fail_message, res.status);
        return true;
      }
    }
    else
    {
      epee::json_rpc::error error_resp;
      if (!m_rpc_server->on_get_service_node_key(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
        return true;
      }
    }
  }

  if (args.size() > 1)
  {
    tools::fail_msg_writer() << "Unexpected arguments";
    return false;
  }

  bool result = false;
  if (args.size() == 1) 
  {
    result = print_sn({res.service_node_pubkey, args[0]});
  }
  else
  {
    result = print_sn({res.service_node_pubkey});
  }

  return result;
}

bool t_rpc_command_executor::print_sr(uint64_t height)
{
  cryptonote::COMMAND_RPC_GET_STAKING_REQUIREMENT::request req = {};
  cryptonote::COMMAND_RPC_GET_STAKING_REQUIREMENT::response res = {};
  std::string fail_message = "Unsuccessful";
  epee::json_rpc::error error_resp;
  req.height = height;

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "get_staking_requirement", fail_message.c_str()))
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }
  else
  {
    epee::json_rpc::error error_resp;
    if (!m_rpc_server->on_get_staking_requirement(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
      return true;
    }
  }

  tools::success_msg_writer() << "Staking Requirement: " << cryptonote::print_money(res.staking_requirement);
  return true;
}

bool t_rpc_command_executor::pop_blocks(uint64_t num_blocks)
{
  cryptonote::COMMAND_RPC_POP_BLOCKS::request req;
  cryptonote::COMMAND_RPC_POP_BLOCKS::response res;
  std::string fail_message = "pop_blocks failed";

  req.nblocks = num_blocks;
  if (m_is_rpc)
  {
    if (!m_rpc_client->rpc_request(req, res, "/pop_blocks", fail_message.c_str()))
    {
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_pop_blocks(req, res) || res.status != CORE_RPC_STATUS_OK)
    {
       tools::fail_msg_writer() << make_error(fail_message, res.status);
       return true;
    }
  }

  tools::success_msg_writer() << "new height: " << res.height;
  return true;
}

bool t_rpc_command_executor::print_sn_key()
{
  cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::request req = {};
  cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::response res = {};
  std::string fail_message = "Unsuccessful";
  epee::json_rpc::error error_resp;

  if (m_is_rpc)
  {
    if (!m_rpc_client->json_rpc_request(req, res, "get_service_node_key", fail_message.c_str()))
    {
      tools::fail_msg_writer() << make_error(fail_message, res.status);
      return true;
    }
  }
  else
  {
    if (!m_rpc_server->on_get_service_node_key(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
    {
      tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
      return true;
    }
  }

  std::string const msg_buf = "Service Node Public Key: " + res.service_node_pubkey;
  tools::success_msg_writer() << msg_buf;
  return true;
}

// Returns lowest x such that (STAKING_PORTIONS * x/amount) >= portions
static uint64_t get_amount_to_make_portions(uint64_t amount, uint64_t portions)
{
  uint64_t lo, hi, resulthi, resultlo;
  lo = mul128(amount, portions, &hi);
  if (lo > UINT64_MAX - (STAKING_PORTIONS - 1))
    hi++;
  lo += STAKING_PORTIONS-1;
  div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
  return resultlo;
}

static uint64_t get_actual_amount(uint64_t amount, uint64_t portions)
{
  uint64_t lo, hi, resulthi, resultlo;
  lo = mul128(amount, portions, &hi);
  div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
  return resultlo;
}

bool t_rpc_command_executor::prepare_registration()
{
  // RAII-style class to temporarily clear categories and restore upon destruction (i.e. upon returning).
  struct clear_log_categories {
    std::string categories;
    clear_log_categories() { categories = mlog_get_categories(); mlog_set_categories(""); }
    ~clear_log_categories() { mlog_set_categories(categories.c_str()); }
  };
  auto scoped_log_cats = std::unique_ptr<clear_log_categories>(new clear_log_categories());

  // Check if the daemon was started in Service Node or not
  {
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::request keyreq = {};
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_KEY::response keyres = {};
    std::string const fail_msg = "Cannot get service node key. Make sure you are running daemon with --service-node flag";

    if (m_is_rpc)
    {
      if (!m_rpc_client->json_rpc_request(keyreq, keyres, "get_service_node_key", fail_msg))
        return true;
    }
    else
    {
      epee::json_rpc::error error_resp;
      if (!m_rpc_server->on_get_service_node_key(keyreq, keyres, error_resp) || keyres.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(fail_msg, error_resp.message);
        return true;
      }
    }
  }

  // Query the latest known block height and nettype
  uint64_t block_height            = 0;
  int hf_version                   = cryptonote::network_version_9_service_nodes;
  cryptonote::network_type nettype = cryptonote::UNDEFINED;
  {
    cryptonote::COMMAND_RPC_GET_INFO::request req;
    cryptonote::COMMAND_RPC_GET_INFO::response res;
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::request hf_req;
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::response hf_res;
    std::string const info_fail_message = "Could not get current blockchain info";

    if (m_is_rpc)
    {
      if (!m_rpc_client->rpc_request(req, res, "/getinfo", info_fail_message.c_str()))
        return true;

      if (!m_rpc_client->json_rpc_request(hf_req, hf_res, "hard_fork_info", info_fail_message.c_str()))
        return true;

      if      (res.mainnet) nettype  = cryptonote::MAINNET;
      else if (res.stagenet) nettype = cryptonote::STAGENET;
      else if (res.testnet) nettype  = cryptonote::TESTNET;
    }
    else
    {
      if (!m_rpc_server->on_get_info(req, res) || res.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(info_fail_message, res.status);
        return true;
      }

      epee::json_rpc::error error_resp;
      if (!m_rpc_server->on_hard_fork_info(hf_req, hf_res, error_resp) || hf_res.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(info_fail_message, hf_res.status);
        return true;
      }

      nettype = m_rpc_server->nettype();
    }

    hf_version   = hf_res.version;
    block_height = std::max(res.height, res.target_height);
  }

  // Query the latest block we've synced and check that the timestamp is sensible, issue a warning if not
  {
    cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::request req  = {};
    cryptonote::COMMAND_RPC_GET_LAST_BLOCK_HEADER::response res = {};
    std::string const fail_msg = "Get latest block failed, unable to check sync status";

    if (m_is_rpc)
    {
      if (!m_rpc_client->json_rpc_request(req, res, "get_last_block_header", fail_msg))
        return true;
    }
    else
    {
      epee::json_rpc::error error_resp;
      if (!m_rpc_server->on_get_last_block_header(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(fail_msg, res.status);
        return true;
      }
    }

    cryptonote::block_header_response const &header = res.block_header;
    uint64_t const now                              = time(nullptr);

    if (now >= header.timestamp)
    {
      uint64_t delta = now - header.timestamp;
      if (delta > (60 * 60))
      {
        tools::fail_msg_writer() << "The last block this Service Node knows about was at least " << get_human_time_ago(header.timestamp, now)
                                 << "\nYour node is possibly desynced from the network or still syncing to the network."
                                 << "\n\nRegistering this node may result in a deregistration due to being out of date with the network\n";
      }
    }

    if (block_height >= header.height)
    {
      uint64_t delta = block_height - header.height;
      if (delta > 15)
      {
        tools::fail_msg_writer() << "The last block this Service Node synced is " << delta << " blocks away from the longest chain we know about."
                                 << "\n\nRegistering this node may result in a deregistration due to being out of date with the network\n";
      }
    }
  }

  const uint64_t staking_requirement =
    std::max(service_nodes::get_staking_requirement(nettype, block_height, hf_version),
             service_nodes::get_staking_requirement(nettype, block_height + 30 * 24, hf_version)); // allow 1 day

  // anything less than DUST will be added to operator stake
  const uint64_t DUST = MAX_NUMBER_OF_CONTRIBUTORS;
  std::cout << "Current staking requirement: " << cryptonote::print_money(staking_requirement) << " " << cryptonote::get_unit() << std::endl;

  enum struct register_step
  {
    ask_is_solo_stake = 0,
    is_solo_stake__operator_address_to_reserve,

    is_open_stake__get_operator_fee,
    is_open_stake__do_you_want_to_reserve_other_contributors,
    is_open_stake__how_many_more_contributors,
    is_open_stake__operator_amount_to_reserve,
    is_open_stake__operator_address_to_reserve,
    is_open_stake__contributor_address_to_reserve,
    is_open_stake__contributor_amount_to_reserve,
    is_open_stake__summary_info,
    final_summary,
    cancelled_by_user,
  };

  struct prepare_registration_state
  {
    register_step            prev_step                    = register_step::ask_is_solo_stake;
    bool                     is_solo_stake;
    size_t                   num_participants             = 1;
    uint64_t                 operator_fee_portions        = STAKING_PORTIONS;
    uint64_t                 portions_remaining           = STAKING_PORTIONS;
    uint64_t                 total_reserved_contributions = 0;
    std::vector<std::string> addresses;
    std::vector<uint64_t>    contributions;
  };

  prepare_registration_state state = {};
  std::stack<prepare_registration_state> state_stack;
  state_stack.push(state);

  bool finished = false;
  register_step step = register_step::ask_is_solo_stake;
  for (input_line_result last_input_result = input_line_result::yes; !finished;)
  {
    if (last_input_result == input_line_result::back)
    {
      step = state.prev_step;
      state_stack.pop();
      state = state_stack.top();
      std::cout << std::endl;
    }

    switch(step)
    {
      case register_step::ask_is_solo_stake:
      {
        last_input_result = input_line_yes_no_cancel("Will the operator contribute the entire stake?");
        if(last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        state.is_solo_stake = (last_input_result == input_line_result::yes);
        if (state.is_solo_stake)
        {
          std::cout << std::endl;
          step = register_step::is_solo_stake__operator_address_to_reserve;
        }
        else
        {
          step = register_step::is_open_stake__get_operator_fee;
        }

        state_stack.push(state);
        continue;
      }

      case register_step::is_solo_stake__operator_address_to_reserve:
      {
        std::string address_str;
        last_input_result = input_line_back_cancel_get_input("Enter the loki address for the solo staker", address_str);
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        state.addresses.push_back(address_str); // the addresses will be validated later down the line
        state.contributions.push_back(STAKING_PORTIONS);
        state.portions_remaining = 0;
        state.total_reserved_contributions += staking_requirement;
        state.prev_step = step;
        step            = register_step::final_summary;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__get_operator_fee:
      {
        std::string operator_fee_str;
        last_input_result = input_line_back_cancel_get_input("What percentage of the total staking reward would the operator like to reserve as an operator fee [0-100]%", operator_fee_str);

        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        if (!service_nodes::get_portions_from_percent_str(operator_fee_str, state.operator_fee_portions))
        {
          std::cout << "Invalid value: " << operator_fee_str << ". Should be between [0-100]" << std::endl;
          continue;
        }

        step = register_step::is_open_stake__do_you_want_to_reserve_other_contributors;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__do_you_want_to_reserve_other_contributors:
      {
        last_input_result = input_line_yes_no_back_cancel("Do you want to reserve portions of the stake for other specific contributors?");
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        state.prev_step = step;
        if(last_input_result == input_line_result::yes)
        {
          step = register_step::is_open_stake__how_many_more_contributors;
        }
        else
        {
          std::cout << std::endl;
          step = register_step::is_open_stake__operator_address_to_reserve;
        }

        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__how_many_more_contributors:
      {
        std::string prompt = "Number of additional contributors [1-" + std::to_string(MAX_NUMBER_OF_CONTRIBUTORS - 1) + "]";
        std::string input;
        last_input_result = input_line_back_cancel_get_input(prompt.c_str(), input);

        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        long additional_contributors = strtol(input.c_str(), NULL, 10 /*base 10*/);
        if(additional_contributors < 1 || additional_contributors > (MAX_NUMBER_OF_CONTRIBUTORS - 1))
        {
          std::cout << "Invalid value. Should be between [1-" << (MAX_NUMBER_OF_CONTRIBUTORS - 1) << "]" << std::endl;
          continue;
        }

        std::cout << std::endl;
        state.num_participants += static_cast<size_t>(additional_contributors);
        state.prev_step = step;
        step            = register_step::is_open_stake__operator_address_to_reserve;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__operator_address_to_reserve:
      {
        std::string address_str;
        last_input_result = input_line_back_cancel_get_input("Enter the loki address for the operator", address_str);
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        state.addresses.push_back(address_str); // the addresses will be validated later down the line
        state.prev_step = step;
        step            = register_step::is_open_stake__operator_amount_to_reserve;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__operator_amount_to_reserve:
      {
        uint64_t min_contribution_portions = service_nodes::get_min_node_contribution_in_portions(hf_version, staking_requirement, 0, 0);
        const uint64_t min_contribution    = get_amount_to_make_portions(staking_requirement, min_contribution_portions);
        std::cout << "Minimum amount that can be reserved: " << cryptonote::print_money(min_contribution) << " " << cryptonote::get_unit() << std::endl;

        std::string contribution_str;
        last_input_result = input_line_back_cancel_get_input("How much loki does the operator want to reserve in the stake?", contribution_str);
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        uint64_t contribution;
        if(!cryptonote::parse_amount(contribution, contribution_str))
        {
          std::cout << "Invalid amount." << std::endl;
          continue;
        }

        uint64_t portions = service_nodes::get_portions_to_make_amount(staking_requirement, contribution);
        if(portions < min_contribution_portions)
        {
          std::cout << "The operator needs to contribute at least 25% of the stake requirement (" << cryptonote::print_money(min_contribution) << " " << cryptonote::get_unit() << "). Aborted." << std::endl;
          continue;
        }

        if(portions > state.portions_remaining)
        {
          std::cout << "The operator contribution is higher than the staking requirement. Any excess contribution will be locked for the staking duration, but won't yield any additional reward." << std::endl;
          portions = state.portions_remaining;
        }

        state.contributions.push_back(portions);
        state.portions_remaining -= portions;
        state.total_reserved_contributions += get_actual_amount(staking_requirement, portions);
        state.prev_step = step;

        if (state.num_participants > 1)
        {
          step = register_step::is_open_stake__contributor_address_to_reserve;
        }
        else
        {
          step = register_step::is_open_stake__summary_info;
        }

        std::cout << std::endl;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__contributor_address_to_reserve:
      {
        std::string const prompt = "Enter the loki address for contributor " + std::to_string(state.contributions.size() + 1);
        std::string address_str;
        last_input_result = input_line_back_cancel_get_input(prompt.c_str(), address_str);
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        // the addresses will be validated later down the line
        state.addresses.push_back(address_str);
        state.prev_step = step;
        step            = register_step::is_open_stake__contributor_amount_to_reserve;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__contributor_amount_to_reserve:
      {
        const uint64_t amount_left         = staking_requirement - state.total_reserved_contributions;
        uint64_t min_contribution_portions = service_nodes::get_min_node_contribution_in_portions(hf_version, staking_requirement, state.total_reserved_contributions, state.contributions.size());
        const uint64_t min_contribution    = service_nodes::portions_to_amount(staking_requirement, min_contribution_portions);

        std::cout << "The minimum amount possible to contribute is " << cryptonote::print_money(min_contribution) << " " << cryptonote::get_unit() << std::endl;
        std::cout << "There is " << cryptonote::print_money(amount_left) << " " << cryptonote::get_unit() << " left to meet the staking requirement." << std::endl;

        std::string contribution_str;
        std::string const prompt = "How much loki does contributor " + std::to_string(state.contributions.size() + 1) + " want to reserve in the stake?";
        last_input_result        = input_line_back_cancel_get_input(prompt.c_str(), contribution_str);
        if (last_input_result == input_line_result::back)
          continue;

        if (last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        uint64_t contribution;
        if (!cryptonote::parse_amount(contribution, contribution_str))
        {
          std::cout << "Invalid amount." << std::endl;
          continue;
        }

        uint64_t portions = service_nodes::get_portions_to_make_amount(staking_requirement, contribution);
        if (portions < min_contribution_portions)
        {
          std::cout << "The amount is too small." << std::endl;
          continue;
        }

        if (portions > state.portions_remaining)
          portions = state.portions_remaining;

        state.contributions.push_back(portions);
        state.portions_remaining -= portions;
        state.total_reserved_contributions += get_actual_amount(staking_requirement, portions);
        state.prev_step = step;

        if (state.contributions.size() == state.num_participants)
          step = register_step::is_open_stake__summary_info;
        else
          step = register_step::is_open_stake__contributor_address_to_reserve;

        std::cout << std::endl;
        state_stack.push(state);
        continue;
      }

      case register_step::is_open_stake__summary_info:
      {
        const uint64_t amount_left = staking_requirement - state.total_reserved_contributions;
        std::cout << "Total staking contributions reserved: " << cryptonote::print_money(state.total_reserved_contributions) << " " << cryptonote::get_unit() << std::endl;
        if (amount_left > DUST)
        {
          std::cout << "Your total reservations do not equal the staking requirement." << std::endl;
          std::cout << "You will leave the remaining portion of " << cryptonote::print_money(amount_left) << " " << cryptonote::get_unit() << " open to contributions from anyone, and the Service Node will not activate until the full staking requirement is filled." << std::endl;

          last_input_result = input_line_yes_no_back_cancel("Is this ok?\n");
          if(last_input_result == input_line_result::no || last_input_result == input_line_result::cancel)
          {
            step = register_step::cancelled_by_user;
            continue;
          }

          if(last_input_result == input_line_result::back)
            continue;

          state_stack.push(state);
          state.prev_step = step;
        }

        step = register_step::final_summary;
        continue;
      }

      case register_step::final_summary:
      {
        assert(state.addresses.size() == state.contributions.size());
        const uint64_t amount_left = staking_requirement - state.total_reserved_contributions;

        std::cout << "Summary:" << std::endl;
        std::cout << "Operating costs as % of reward: " << (state.operator_fee_portions * 100.0 / STAKING_PORTIONS) << "%" << std::endl;
        printf("%-16s%-9s%-19s%-s\n", "Contributor", "Address", "Contribution", "Contribution(%)");
        printf("%-16s%-9s%-19s%-s\n", "___________", "_______", "____________", "_______________");

        for (size_t i = 0; i < state.num_participants; ++i)
        {
          const std::string participant_name = (i==0) ? "Operator" : "Contributor " + std::to_string(i);
          uint64_t amount = get_actual_amount(staking_requirement, state.contributions[i]);
          if (amount_left <= DUST && i == 0)
            amount += amount_left; // add dust to the operator.
          printf("%-16s%-9s%-19s%-.9f\n", participant_name.c_str(), state.addresses[i].substr(0,6).c_str(), cryptonote::print_money(amount).c_str(), (double)state.contributions[i] * 100 / STAKING_PORTIONS);
        }

        if (amount_left > DUST)
        {
          printf("%-16s%-9s%-19s%-.2f\n", "(open)", "", cryptonote::print_money(amount_left).c_str(), amount_left * 100.0 / staking_requirement);
        }
        else if (amount_left > 0)
        {
          std::cout << "\nActual amounts may differ slightly from specification. This is due to\n" << std::endl;
          std::cout << "limitations on the way fractions are represented internally.\n" << std::endl;
        }

        std::cout << "\nBecause the actual requirement will depend on the time that you register, the\n";
        std::cout << "amounts shown here are used as a guide only, and the percentages will remain\n";
        std::cout << "the same." << std::endl << std::endl;

        last_input_result = input_line_yes_no_back_cancel("Do you confirm the information above is correct?");
        if(last_input_result == input_line_result::no || last_input_result == input_line_result::cancel)
        {
          step = register_step::cancelled_by_user;
          continue;
        }

        if(last_input_result == input_line_result::back)
          continue;

        finished = true;
        continue;
      }

      case register_step::cancelled_by_user:
      {
        std::cout << "Cancel requested in prepare registration. Aborting." << std::endl;
        return true;
      }
    }
  }

  // <operator cut> <address> <fraction> [<address> <fraction> [...]]]
  std::vector<std::string> args;
  args.push_back(std::to_string(state.operator_fee_portions));
  for (size_t i = 0; i < state.num_participants; ++i)
  {
    args.push_back(state.addresses[i]);
    args.push_back(std::to_string(state.contributions[i]));
  }

  for (size_t i = 0; i < state.addresses.size(); i++)
  {
    for (size_t j = 0; j < i; j++)
    {
      if (state.addresses[i] == state.addresses[j])
      {
        std::cout << "Must not provide the same address twice" << std::endl;
        return true;
      }
    }
  }

  scoped_log_cats.reset();

  {
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_CMD_RAW::request req;
    cryptonote::COMMAND_RPC_GET_SERVICE_NODE_REGISTRATION_CMD_RAW::response res;
    std::string fail_message = "Unsuccessful";

    req.args = args;
    req.make_friendly = true;
    req.staking_requirement = staking_requirement;
    if (m_is_rpc)
    {
      if (!m_rpc_client->json_rpc_request(req, res, "get_service_node_registration_cmd_raw", fail_message))
      {
        tools::fail_msg_writer() << "Failed to validate registration arguments; check the addresses and registration parameters and that the Daemon is running with the '--service-node' flag";
        return true;
      }
    }
    else
    {
      epee::json_rpc::error error_resp;
      if (!m_rpc_server->on_get_service_node_registration_cmd_raw(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
      {
        tools::fail_msg_writer() << make_error(fail_message, error_resp.message);
        return true;
      }
    }

    tools::success_msg_writer() << res.registration_cmd;
  }

  return true;
}

bool t_rpc_command_executor::prune_blockchain()
{
    cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::request req;
    cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.check = false;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "prune_blockchain", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_prune_blockchain(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    tools::success_msg_writer() << "Blockchain pruned: seed " << epee::string_tools::to_string_hex(res.pruning_seed);
    return true;
}

bool t_rpc_command_executor::check_blockchain_pruning()
{
    cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::request req;
    cryptonote::COMMAND_RPC_PRUNE_BLOCKCHAIN::response res;
    std::string fail_message = "Unsuccessful";
    epee::json_rpc::error error_resp;

    req.check = true;

    if (m_is_rpc)
    {
        if (!m_rpc_client->json_rpc_request(req, res, "prune_blockchain", fail_message.c_str()))
        {
            return true;
        }
    }
    else
    {
        if (!m_rpc_server->on_prune_blockchain(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
        {
            tools::fail_msg_writer() << make_error(fail_message, res.status);
            return true;
        }
    }

    if (res.pruning_seed)
    {
      tools::success_msg_writer() << "Blockchain is pruned";
    }
    else
    {
      tools::success_msg_writer() << "Blockchain is not pruned";
    }
    return true;
}

}// namespace daemonize
