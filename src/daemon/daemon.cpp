// Copyright (c) 2014-2019, The Monero Project
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

#include <memory>
#include <stdexcept>
#include <boost/algorithm/string/split.hpp>
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
#include "net/net_ssl.h"
#include "version.h"

using namespace epee;

#include <functional>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize {

struct t_internals {
private:
  t_protocol protocol;
public:
  t_core core;
  t_p2p p2p;
  std::vector<std::unique_ptr<t_rpc>> rpcs;

  t_internals(
      boost::program_options::variables_map const & vm
    )
    : core{vm}
    , protocol{vm, core, command_line::get_arg(vm, cryptonote::arg_offline)}
    , p2p{vm, protocol}
  {
    // Handle circular dependencies
    protocol.set_p2p_endpoint(p2p.get());
    core.set_protocol(protocol.get());

    const auto restricted = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_restricted_rpc);
    const auto main_rpc_port = command_line::get_arg(vm, cryptonote::core_rpc_server::arg_rpc_bind_port);
    const auto restricted_rpc_port_arg = cryptonote::core_rpc_server::arg_rpc_restricted_bind_port;
    const bool has_restricted_rpc_port_arg = !command_line::is_arg_defaulted(vm, restricted_rpc_port_arg);
    rpcs.emplace_back(new t_rpc{vm, core, p2p, restricted, main_rpc_port, "core", !has_restricted_rpc_port_arg});

    if(has_restricted_rpc_port_arg)
    {
      auto restricted_rpc_port = command_line::get_arg(vm, restricted_rpc_port_arg);
      rpcs.emplace_back(new t_rpc{vm, core, p2p, true, restricted_rpc_port, "restricted", true});
    }
  }
};

void t_daemon::init_options(boost::program_options::options_description & option_spec)
{
  t_core::init_options(option_spec);
  t_p2p::init_options(option_spec);
  t_rpc::init_options(option_spec);
}

t_daemon::t_daemon(
    boost::program_options::variables_map const & vm,
    uint16_t public_rpc_port
  )
  : mp_internals{new t_internals{vm}},
  public_rpc_port(public_rpc_port)
{
  zmq_rpc_bind_port = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_bind_port);
  zmq_rpc_bind_address = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_bind_ip);
  zmq_rpc_disabled = command_line::get_arg(vm, daemon_args::arg_zmq_rpc_disabled);
}

t_daemon::~t_daemon() = default;

// MSVC is brain-dead and can't default this...
t_daemon::t_daemon(t_daemon && other)
{
  if (this != &other)
  {
    mp_internals = std::move(other.mp_internals);
    other.mp_internals.reset(nullptr);
    public_rpc_port = other.public_rpc_port;
  }
}

// or this
t_daemon & t_daemon::operator=(t_daemon && other)
{
  if (this != &other)
  {
    mp_internals = std::move(other.mp_internals);
    other.mp_internals.reset(nullptr);
    public_rpc_port = other.public_rpc_port;
  }
  return *this;
}

bool t_daemon::run(bool interactive)
{
  if (nullptr == mp_internals)
  {
    throw std::runtime_error{"Can't run stopped daemon"};
  }

  std::atomic<bool> stop(false), shutdown(false);
  boost::thread stop_thread = boost::thread([&stop, &shutdown, this] {
    while (!stop)
      epee::misc_utils::sleep_no_w(100);
    if (shutdown)
      this->stop_p2p();
  });
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    stop = true;
    stop_thread.join();
  });
  tools::signal_handler::install([&stop, &shutdown](int){ stop = shutdown = true; });

  try
  {
    if (!mp_internals->core.run())
      return false;

    for(auto& rpc: mp_internals->rpcs)
      rpc->run();

    std::unique_ptr<daemonize::t_command_server> rpc_commands;
    if (interactive && mp_internals->rpcs.size())
    {
      // The first three variables are not used when the fourth is false
      rpc_commands.reset(new daemonize::t_command_server(0, 0, boost::none, epee::net_utils::ssl_support_t::e_ssl_support_disabled, false, mp_internals->rpcs.front()->get_server()));
      rpc_commands->start_handling(std::bind(&daemonize::t_daemon::stop_p2p, this));
    }

    cryptonote::rpc::DaemonHandler rpc_daemon_handler(mp_internals->core.get(), mp_internals->p2p.get());
    cryptonote::rpc::ZmqServer zmq_server(rpc_daemon_handler);

    if (!zmq_rpc_disabled)
    {
      if (!zmq_server.addTCPSocket(zmq_rpc_bind_address, zmq_rpc_bind_port))
      {
        LOG_ERROR(std::string("Failed to add TCP Socket (") + zmq_rpc_bind_address
            + ":" + zmq_rpc_bind_port + ") to ZMQ RPC Server");

        if (rpc_commands)
          rpc_commands->stop_handling();

        for(auto& rpc : mp_internals->rpcs)
          rpc->stop();

        return false;
      }

      MINFO("Starting ZMQ server...");
      zmq_server.run();

      MINFO(std::string("ZMQ server started at ") + zmq_rpc_bind_address
            + ":" + zmq_rpc_bind_port + ".");
    }
    else
      MINFO("ZMQ server disabled");

    if (public_rpc_port > 0)
    {
      MGINFO("Public RPC port " << public_rpc_port << " will be advertised to other peers over P2P");
      mp_internals->p2p.get().set_rpc_port(public_rpc_port);
    }
    
    mp_internals->p2p.run(); // blocks until p2p goes down

    if (rpc_commands)
      rpc_commands->stop_handling();

    if (!zmq_rpc_disabled)
      zmq_server.stop();

    for(auto& rpc : mp_internals->rpcs)
      rpc->stop();
    MGINFO("Node stopped.");
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

void t_daemon::stop()
{
  if (nullptr == mp_internals)
  {
    throw std::runtime_error{"Can't stop stopped daemon"};
  }
  mp_internals->p2p.stop();
  for(auto& rpc : mp_internals->rpcs)
    rpc->stop();

  mp_internals.reset(nullptr); // Ensure resources are cleaned up before we return
}

void t_daemon::stop_p2p()
{
  if (nullptr == mp_internals)
  {
    throw std::runtime_error{"Can't send stop signal to a stopped daemon"};
  }
  mp_internals->p2p.get().send_stop_signal();
}

} // namespace daemonize
