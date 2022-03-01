// Copyright (c) 2014-2022, The Monero Project
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

#pragma once

#include "misc_log_ex.h"
#include "daemon/daemon.h"
#include "rpc/daemon_handler.h"
#include "rpc/zmq_server.h"
#include "common/password.h"
#include "common/util.h"
#include "daemon/core.h"
#include "daemon/p2p.h"
#include "daemon/protocol.h"
#include "daemon/rpc.h"
#include "daemon/command_server.h"
#include "daemon/command_server.h"
#include "daemon/command_line_args.h"
#include "version.h"
#include "tools.h"


class mock_rpc_daemon : public cryptonote::core_rpc_server {
public:
  mock_rpc_daemon(
    cryptonote::core& cr
  , nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> >& p2p
  ): cryptonote::core_rpc_server(cr, p2p) {}

  static void init_options(boost::program_options::options_description& desc){ cryptonote::core_rpc_server::init_options(desc); }
  cryptonote::network_type nettype() const { return m_network_type; }
  void nettype(cryptonote::network_type nettype) { m_network_type = nettype; }

  CHAIN_HTTP_TO_MAP2(cryptonote::core_rpc_server::connection_context); //forward http requests to uri map
  BEGIN_URI_MAP2()
    MAP_URI_AUTO_JON2("/send_raw_transaction", on_send_raw_tx_2, cryptonote::COMMAND_RPC_SEND_RAW_TX)
    MAP_URI_AUTO_JON2("/sendrawtransaction", on_send_raw_tx_2, cryptonote::COMMAND_RPC_SEND_RAW_TX)
    else {  // Default to parent for non-overriden callbacks
      return cryptonote::core_rpc_server::handle_http_request_map(query_info, response_info, m_conn_context);
    }
  END_URI_MAP2()

  bool on_send_raw_tx_2(const cryptonote::COMMAND_RPC_SEND_RAW_TX::request& req, cryptonote::COMMAND_RPC_SEND_RAW_TX::response& res, const cryptonote::core_rpc_server::connection_context *ctx);

protected:
  cryptonote::network_type m_network_type;
};

class mock_daemon {
public:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  typedef nodetool::node_server<t_protocol_raw> t_node_server;

  static constexpr const std::chrono::seconds rpc_timeout = std::chrono::seconds(120);

  cryptonote::core * m_core;
  t_protocol_raw m_protocol;
  mock_rpc_daemon m_rpc_server;
  t_node_server m_server;
  cryptonote::network_type m_network_type;
  epee::net_utils::http::http_simple_client m_http_client;

  bool m_start_p2p;
  bool m_start_zmq;
  boost::program_options::variables_map m_vm;

  std::string m_p2p_bind_port;
  std::string m_rpc_bind_port;
  std::string m_zmq_bind_port;

  std::atomic<bool> m_stopped;
  std::atomic<bool> m_terminated;
  std::atomic<bool> m_deinitalized;
  boost::thread m_run_thread;

  mock_daemon(
      cryptonote::core * core,
      boost::program_options::variables_map const & vm
  )
      : m_core(core)
      , m_vm(vm)
      , m_start_p2p(false)
      , m_start_zmq(false)
      , m_terminated(false)
      , m_deinitalized(false)
      , m_stopped(false)
      , m_protocol{*core, nullptr, command_line::get_arg(vm, cryptonote::arg_offline)}
      , m_server{m_protocol}
      , m_rpc_server{*core, m_server}
  {
    // Handle circular dependencies
    m_protocol.set_p2p_endpoint(&m_server);
    m_core->set_cryptonote_protocol(&m_protocol);
    load_params(vm);
  }

  virtual ~mock_daemon();

  static void init_options(boost::program_options::options_description & option_spec);
  static void default_options(boost::program_options::variables_map & vm);
  static void set_ports(boost::program_options::variables_map & vm, unsigned initial_port);

  mock_daemon * set_start_p2p(bool fl) { m_start_p2p = fl; return this; }
  mock_daemon * set_start_zmq(bool fl) { m_start_zmq = fl; return this; }

  void init();
  void deinit();
  void run();
  bool run_main();
  void stop();
  void stop_p2p();
  void stop_rpc();
  void init_and_run();
  void stop_and_deinit();
  void try_init_and_run(boost::optional<unsigned> initial_port=boost::none);

  void mine_blocks(size_t num_blocks, const std::string &miner_address);
  void start_mining(const std::string &miner_address, uint64_t threads_count=1, bool do_background_mining=false, bool ignore_battery=true);
  void stop_mining();
  uint64_t get_height();

  void load_params(boost::program_options::variables_map const & vm);

  std::string zmq_addr() const { return std::string("127.0.0.1:") + m_zmq_bind_port; }
  std::string rpc_addr() const { return std::string("127.0.0.1:") + m_rpc_bind_port; }
  std::string p2p_addr() const { return std::string("127.0.0.1:") + m_p2p_bind_port; }
  cryptonote::network_type nettype() const { return m_network_type; }
  cryptonote::core * core() const { return m_core; }
};
