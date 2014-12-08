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

#ifndef JSON_RPC_HTTP_SERVER_H
#define JSON_RPC_HTTP_SERVER_H

#include "include_base_utils.h"
using namespace epee;

#include "wallet_rpc_server.h"
#include "common/command_line.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/account.h"
#include "wallet_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "string_tools.h"
#include "crypto/hash.h"
#include "zmq.hpp"

namespace IPC
{
  class Daemon_ipc_server
  {
    boost::thread *server_thread; /*!< Server runs on this thread */
    /*!
     * \brief Repeatedly loops processing requests if any.
     */
    void poll();
    void handle_request(const std::string &request_string, std::string &response);
    bool check_core_busy();

    void getblocks(std::string &response);
    void sendtransactions(std::string &response);
    void get_o_indexes(std::string &response);
    std::string m_ip; /*!< IP address where its listening */
    std::string m_port; /*!< Port where its listening */
    bool m_is_running; /*!< Whether the server is currently running */
    zmq::socket_t *socket; /*!< 0MQ Socket pointer */
    cryptonote::core *core; /*!< Pointer to the core */
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
      /*!< Pointer to p2p node server */

  public:
    /**
     * \brief Constructor
     * \param ip IP address to bind
     * \param port Port number to bind
     * \param ev_handler Event handler function pointer
     */
    Daemon_ipc_server(const std::string &ip, const std::string &port);

    /**
     * \brief Destructor
     */
    ~Daemon_ipc_server();

    /*!
     * \brief Starts the server
     * \return True if start was successful
     */
    bool start();

    /*!
     * \brief Stops the server
     */
    void stop();
  };
}

#endif
