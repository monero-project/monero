// Copyright (c) 2014-2024, The Monero Project
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

#include "daemon.h"
#include <common/command_line.h>

using namespace std;
using namespace daemonize;
namespace po = boost::program_options;

bool mock_rpc_daemon::on_send_raw_tx_2(const cryptonote::COMMAND_RPC_SEND_RAW_TX::request& req, cryptonote::COMMAND_RPC_SEND_RAW_TX::response& res, const cryptonote::core_rpc_server::connection_context *ctx)
{
  cryptonote::COMMAND_RPC_SEND_RAW_TX::request req2(req);
  req2.do_not_relay = true;  // Do not relay in test setup, only one daemon running.
  return cryptonote::core_rpc_server::on_send_raw_tx(req2, res, ctx);
}

void mock_daemon::init_options(boost::program_options::options_description & option_spec)
{
  cryptonote::core::init_options(option_spec);
  t_node_server::init_options(option_spec);
  cryptonote::core_rpc_server::init_options(option_spec);

  command_line::add_arg(option_spec, daemon_args::arg_zmq_rpc_bind_ip);
  command_line::add_arg(option_spec, daemon_args::arg_zmq_rpc_bind_port);
}

void mock_daemon::default_options(boost::program_options::variables_map & vm)
{
  std::vector<std::string> exclusive_nodes{"127.0.0.1:65525"};
  tools::options::set_option(vm, nodetool::arg_p2p_add_exclusive_node, po::variable_value(exclusive_nodes, false));

  tools::options::set_option(vm, nodetool::arg_p2p_bind_ip, po::variable_value(std::string("127.0.0.1"), false));
  tools::options::set_option(vm, nodetool::arg_no_igd, po::variable_value(true, false));
  tools::options::set_option(vm, cryptonote::arg_offline, po::variable_value(true, false));
  tools::options::set_option(vm, "disable-dns-checkpoints", po::variable_value(true, false));

  const char *test_mainnet = getenv("TEST_MAINNET");
  if (!test_mainnet || atoi(test_mainnet) == 0)
  {
    tools::options::set_option(vm, cryptonote::arg_testnet_on, po::variable_value(true, false));
  }

  // By default pick non-standard ports to avoid confusion with possibly locally running daemons (mainnet/testnet)
  const char *test_p2p_port = getenv("TEST_P2P_PORT");
  auto p2p_port = std::string(test_p2p_port && strlen(test_p2p_port) > 0 ? test_p2p_port : "61340");
  tools::options::set_option(vm, nodetool::arg_p2p_bind_port, po::variable_value(p2p_port, false));

  const char *test_rpc_port = getenv("TEST_RPC_PORT");
  auto rpc_port = std::string(test_rpc_port && strlen(test_rpc_port) > 0 ? test_rpc_port : "61341");
  tools::options::set_option(vm, cryptonote::core_rpc_server::arg_rpc_bind_port, po::variable_value(rpc_port, false));

  const char *test_zmq_port = getenv("TEST_ZMQ_PORT");
  auto zmq_port = std::string(test_zmq_port && strlen(test_zmq_port) > 0 ? test_zmq_port : "61342");
  tools::options::set_option(vm, daemon_args::arg_zmq_rpc_bind_port, po::variable_value(zmq_port, false));

  po::notify(vm);
}

void mock_daemon::set_ports(boost::program_options::variables_map & vm, unsigned initial_port)
{
  CHECK_AND_ASSERT_THROW_MES(initial_port < 65535-2, "Invalid port number");
  tools::options::set_option(vm, nodetool::arg_p2p_bind_port, po::variable_value(std::to_string(initial_port), false));
  tools::options::set_option(vm, cryptonote::core_rpc_server::arg_rpc_bind_port, po::variable_value(std::to_string(initial_port + 1), false));
  tools::options::set_option(vm, daemon_args::arg_zmq_rpc_bind_port, po::variable_value(std::to_string(initial_port + 2), false));
  po::notify(vm);
}

void mock_daemon::load_params(boost::program_options::variables_map const & vm)
{
  m_p2p_bind_port = command_line::get_arg(vm, nodetool::arg_p2p_bind_port);
  m_rpc_bind_port = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_port);
  m_zmq_bind_port = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_bind_port);
  m_network_type = command_line::get_arg(vm, cryptonote::arg_testnet_on) ? cryptonote::TESTNET : cryptonote::MAINNET;
}

mock_daemon::~mock_daemon()
{
  if(m_http_client.is_connected())
    m_http_client.disconnect();

  if (!m_terminated)
  {
    try
    {
      stop();
    }
    catch (...)
    {
      MERROR("Failed to stop");
    }
  }

  if (!m_deinitalized)
  {
    deinit();
  }
}

void mock_daemon::init()
{
  m_deinitalized = false;
  const auto main_rpc_port = command_line::get_arg(m_vm, cryptonote::core_rpc_server::arg_rpc_bind_port);
  m_rpc_server.nettype(m_network_type);

  CHECK_AND_ASSERT_THROW_MES(m_protocol.init(m_vm), "Failed to initialize cryptonote protocol.");
  CHECK_AND_ASSERT_THROW_MES(m_rpc_server.init(m_vm, false, main_rpc_port, false), "Failed to initialize RPC server.");

  if (m_start_p2p)
    CHECK_AND_ASSERT_THROW_MES(m_server.init(m_vm), "Failed to initialize p2p server.");

  if(m_http_client.is_connected())
    m_http_client.disconnect();

  CHECK_AND_ASSERT_THROW_MES(m_http_client.set_server(rpc_addr(), boost::none, epee::net_utils::ssl_support_t::e_ssl_support_disabled), "RPC client init fail");
}

void mock_daemon::deinit()
{
  if(m_http_client.is_connected())
    m_http_client.disconnect();

  try
  {
    m_rpc_server.deinit();
  }
  catch (...)
  {
    MERROR("Failed to deinitialize RPC server...");
  }

  if (m_start_p2p)
  {
    try
    {
      m_server.deinit();
    }
    catch (...)
    {
      MERROR("Failed to deinitialize p2p...");
    }
  }

  try
  {
    m_protocol.deinit();
    m_protocol.set_p2p_endpoint(nullptr);
  }
  catch (...)
  {
    MERROR("Failed to stop cryptonote protocol!");
  }

  m_deinitalized = true;
}

void mock_daemon::init_and_run()
{
  init();
  run();
}

void mock_daemon::stop_and_deinit()
{
  stop();
  deinit();
}

void mock_daemon::try_init_and_run(boost::optional<unsigned> initial_port)
{
  const unsigned max_attempts = 3;
  for(unsigned attempts=0; attempts < max_attempts; ++attempts)
  {
    if (initial_port)
    {
      set_ports(m_vm, initial_port.get());
      load_params(m_vm);
      MDEBUG("Ports changed, RPC: " << rpc_addr());
    }

    try
    {
      init_and_run();
      return;
    }
    catch(const std::exception &e)
    {
      MWARNING("Could not init and start, attempt: " << attempts << ", reason: " << e.what());
      if (attempts + 1 >= max_attempts)
      {
        throw;
      }
    }
  }
}

void mock_daemon::run()
{
  m_run_thread = boost::thread(boost::bind(&mock_daemon::run_main, this));
}

bool mock_daemon::run_main()
{
  CHECK_AND_ASSERT_THROW_MES(!m_terminated, "Can't run stopped daemon");
  CHECK_AND_ASSERT_THROW_MES(!m_start_zmq || m_start_p2p, "ZMQ requires P2P");
  boost::thread stop_thread = boost::thread([this] {
    while (!this->m_stopped)
      epee::misc_utils::sleep_no_w(100);
    this->stop_p2p();
  });

  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    m_stopped = true;
    stop_thread.join();
  });

  try
  {
    CHECK_AND_ASSERT_THROW_MES(m_rpc_server.run(2, false), "Failed to start RPC");
    cryptonote::rpc::DaemonHandler rpc_daemon_handler(*m_core, m_server);
    cryptonote::rpc::ZmqServer zmq_server(rpc_daemon_handler);

    if (m_start_zmq)
    {
      if (!zmq_server.init_rpc("127.0.0.1", m_zmq_bind_port))
      {
        MERROR("Failed to add TCP Socket (127.0.0.1:" << m_zmq_bind_port << ") to ZMQ RPC Server");

        stop_rpc();
        return false;
      }

      MINFO("Starting ZMQ server...");
      zmq_server.run();

      MINFO("ZMQ server started at 127.0.0.1: " << m_zmq_bind_port);
    }

    if (m_start_p2p)
    {
      m_server.run();  // blocks until p2p goes down
    }
    else
    {
      while (!this->m_stopped)
        epee::misc_utils::sleep_no_w(100);
    }

    if (m_start_zmq)
      zmq_server.stop();

    stop_rpc();
    return true;
  }
  catch (std::exception const & ex)
  {
    MFATAL("Uncaught exception! " << ex.what());
    return false;
  }
  catch (...)
  {
    MFATAL("Uncaught exception!");
    return false;
  }
}

void mock_daemon::stop()
{
  CHECK_AND_ASSERT_THROW_MES(!m_terminated, "Can't stop stopped daemon");
  m_stopped = true;
  m_terminated = true;
  m_run_thread.join();
}

void mock_daemon::stop_rpc()
{
  m_rpc_server.send_stop_signal();
  m_rpc_server.timed_wait_server_stop(5000);
}

void mock_daemon::stop_p2p()
{
  if (m_start_p2p)
    m_server.send_stop_signal();
}

void mock_daemon::mine_blocks(size_t num_blocks, const std::string &miner_address, std::chrono::seconds timeout)
{
  bool blocks_mined = false;
  const uint64_t start_height = get_height();
  MDEBUG("Current height before mining: " << start_height);

  start_mining(miner_address);
  auto mining_started = std::chrono::system_clock::now();

  while(true) {
    epee::misc_utils::sleep_no_w(100);
    const uint64_t cur_height = get_height();

    if (cur_height - start_height >= num_blocks)
    {
      MDEBUG("Cur blocks: " << cur_height << " start: " << start_height);
      blocks_mined = true;
      break;
    }

    auto current_time = std::chrono::system_clock::now();
    if (timeout < current_time - mining_started)
    {
      break;
    }
  }

  stop_mining();
  CHECK_AND_ASSERT_THROW_MES(blocks_mined, "Mining failed in the time limit: " << timeout.count());
}

constexpr const std::chrono::seconds mock_daemon::rpc_timeout;

void mock_daemon::start_mining(const std::string &miner_address, uint64_t threads_count, bool do_background_mining, bool ignore_battery)
{
  cryptonote::COMMAND_RPC_START_MINING::request req;
  req.miner_address = miner_address;
  req.threads_count = threads_count;
  req.do_background_mining = do_background_mining;
  req.ignore_battery = ignore_battery;

  cryptonote::COMMAND_RPC_START_MINING::response resp;
  bool r = epee::net_utils::invoke_http_json("/start_mining", req, resp, m_http_client, rpc_timeout);
  CHECK_AND_ASSERT_THROW_MES(r, "RPC error - start mining");
  CHECK_AND_ASSERT_THROW_MES(resp.status != CORE_RPC_STATUS_BUSY, "Daemon busy");
  CHECK_AND_ASSERT_THROW_MES(resp.status == CORE_RPC_STATUS_OK, "Daemon response invalid: " << resp.status);
}

void mock_daemon::stop_mining()
{
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response resp;
  bool r = epee::net_utils::invoke_http_json("/stop_mining", req, resp, m_http_client, rpc_timeout);
  CHECK_AND_ASSERT_THROW_MES(r, "RPC error - stop mining");
  CHECK_AND_ASSERT_THROW_MES(resp.status != CORE_RPC_STATUS_BUSY, "Daemon busy");
  CHECK_AND_ASSERT_THROW_MES(resp.status == CORE_RPC_STATUS_OK, "Daemon response invalid: " << resp.status);
}

uint64_t mock_daemon::get_height()
{
  return m_core->get_blockchain_storage().get_current_blockchain_height();
}
