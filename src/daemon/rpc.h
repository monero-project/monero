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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "rpc/core_rpc_server.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{

class t_rpc final
{
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    cryptonote::core_rpc_server::init_options(option_spec);
  }
private:
  cryptonote::core_rpc_server m_server;
  const std::string m_description;
public:
  t_rpc(
      boost::program_options::variables_map const & vm
    , t_core & core
    , t_p2p & p2p
    , const bool restricted
    , const std::string & port
    , const std::string & description
    , bool allow_rpc_payment
    )
    : m_server{core.get(), p2p.get()}, m_description{description}
  {
    MGINFO("Initializing " << m_description << " RPC server...");

    if (!m_server.init(vm, restricted, port, allow_rpc_payment, command_line::get_arg(vm, daemon_args::arg_proxy)))
    {
      throw std::runtime_error("Failed to initialize " + m_description + " RPC server.");
    }
    MGINFO(m_description << " RPC server initialized OK on port: " << m_server.get_binded_port());
  }

  void run()
  {
    MGINFO("Starting " << m_description << " RPC server...");
    if (!m_server.run(2, false))
    {
      throw std::runtime_error("Failed to start " + m_description + " RPC server.");
    }
    MGINFO(m_description << " RPC server started ok");
  }

  void stop()
  {
    MGINFO("Stopping " << m_description << " RPC server...");
    m_server.send_stop_signal();
    m_server.timed_wait_server_stop(5000);
  }

  cryptonote::core_rpc_server* get_server()
  {
    return &m_server;
  }

  ~t_rpc()
  {
    MGINFO("Deinitializing " << m_description << " RPC server...");
    try {
      m_server.deinit();
    } catch (...) {
      MERROR("Failed to deinitialize " << m_description << " RPC server...");
    }
  }
};

}
