// Copyright (c) 2014-2016, The Monero Project
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

#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "p2p/net_node.h"
#include "daemon/protocol.h"

namespace daemonize
{

class t_p2p final
{
private:
  typedef cryptonote::t_cryptonote_protocol_handler<cryptonote::core> t_protocol_raw;
  typedef nodetool::node_server<t_protocol_raw> t_node_server;
public:
  static void init_options(boost::program_options::options_description & option_spec)
  {
    t_node_server::init_options(option_spec);
  }
private:
  t_node_server m_server;
public:
  t_p2p(
      boost::program_options::variables_map const & vm
    , t_protocol & protocol
    )
    : m_server{protocol.get()}
  {
    //initialize objects
    LOG_PRINT_L0("Initializing p2p server...");
    if (!m_server.init(vm))
    {
      throw std::runtime_error("Failed to initialize p2p server.");
    }
    LOG_PRINT_L0("P2p server initialized OK");
  }

  t_node_server & get()
  {
    return m_server;
  }

  void run()
  {
    LOG_PRINT_L0("Starting p2p net loop...");
    m_server.run();
    LOG_PRINT_L0("p2p net loop stopped");
  }

  void stop()
  {
    m_server.send_stop_signal();
  }

  ~t_p2p()
  {
    LOG_PRINT_L0("Deinitializing p2p...");
    try {
      m_server.deinit();
    } catch (...) {
      LOG_PRINT_L0("Failed to deinitialize p2p...");
    }
  }
};

}
