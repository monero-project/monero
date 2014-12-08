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

#include "json_rpc_http_server.h"

#include <iostream>

namespace IPC
{
  Daemon_ipc_server::Daemon_ipc_server(const std::string &ip, const std::string &port,
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
    cryptonote::core *p_core)
  {
    if (m_is_running)
    {
      return false;
    }
    p2p = p_p2p;
    core = p_core;
    m_ip = ip;
    m_port = port;
    zmq::context_t context(1);
    socket = new zmq::socket_t(context, ZMQ_REQ);
    return true;
  }

  void Daemon_ipc_server::start()
  {
    socket->bind(std::string("tcp://" + m_ip + ":" + m_port);
    m_is_running = true;
    // Start a new thread so it doesn't block.
    server_thread = new boost::thread(&Daemon_ipc_server::poll, this);
  }

  Daemon_ipc_server::~Daemon_ipc_server()
  {
    stop();
  }

  bool Daemon_ipc_server::check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return false;
    }
    return true;
  }

  /*!
   * \brief Stops the server
   */
  void Daemon_ipc_server::stop()
  {
    m_is_running = false;
    server_thread->join();
    delete server_thread;
  }

  void Daemon_ipc_server::getblocks(std::string &response)
  {
    
  }
  void Daemon_ipc_server::sendtransactions(std::string &response);
  void Daemon_ipc_server::get_o_indexes(std::string &response);

  void handle_request(const std::string &request_string, std::string &response)
  {
    if (check_core_busy())
    {
      response = "";
    }
    if (request_string == "getblocks")
    {
      getblocks(response);
    }
    else if (request_string == "sendrawtransaction")
    {
      sendrawtransaction(response);
    }
    else if (request_string == "get_o_indexes")
    {
      get_o_indexes(response);
    }
    else
    {
      response = "";
    }
  }

  /*!
   * \brief Repeatedly loops processing requests if any.
   */
  void Daemon_ipc_server::poll()
  {
    // Loop until the server is running and poll.
    while (m_is_running) {
      zmq::message_t request;
      // Wait for next request from client
      socket->recv(&request);
      std::string request_string((char*)request.data(), request.size());
      std::string response;
      handle_request(request_string, response);
      zmq::message_t reply(response.length());
      memcpy((void*)reply.data(), response.c_str(), response.length());
      socket->send(reply);
    }
  }
}
