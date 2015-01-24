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

#include "daemon_ipc_handlers.h"

#include <iostream>

namespace
{
  cryptonote::core *core; /*!< Pointer to the core */
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
    /*!< Pointer to p2p node server */
  zactor_t *server;
  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return false;
    }
    return true;
  }
}

namespace IPC
{
  namespace Daemon
  {
    void init(cryptonote::core *p_core,
      nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p)
    {
      p2p = p_p2p;
      core = p_core;
      server = zactor_new (wap_server, NULL);
      zsock_send (server, "ss", "BIND", "ipc://@/monero");
      zsock_send (server, "sss", "SET", "server/timeout", "5000");
    }

    void start_mining(wap_proto_t *message)
    {
      uint64_t start_height = wap_proto_start_height(message);
      wap_proto_set_curr_height(message, 2 * start_height);
    }

    void getblocks(wap_proto_t *message)
    {
      
    }

    void sendtransactions(wap_proto_t *message)
    {

    }

    void get_o_indexes(wap_proto_t *message)
    {

    }
  }
}
