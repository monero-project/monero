// Copyright (c) 2016-2018, The Monero Project
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

#include "zmq_server.h"
#include <boost/chrono/chrono.hpp>

namespace cryptonote
{

namespace rpc
{

ZmqServer::ZmqServer(RpcHandler& h) :
    handler(h),
    stop_signal(false),
    running(false),
    context(DEFAULT_NUM_ZMQ_THREADS) // TODO: make this configurable
{
}

ZmqServer::~ZmqServer()
{
}

void ZmqServer::serve()
{

  while (1)
  {
    try
    {
      zmq::message_t message;

      if (!rep_socket)
      {
        throw std::runtime_error("ZMQ RPC server reply socket is null");
      }
      while (rep_socket->recv(&message))
      {
        std::string message_string(reinterpret_cast<const char *>(message.data()), message.size());

        MDEBUG(std::string("Received RPC request: \"") + message_string + "\"");

        std::string response = handler.handle(message_string);

        zmq::message_t reply(response.size());
        memcpy((void *) reply.data(), response.c_str(), response.size());

        rep_socket->send(reply);
        MDEBUG(std::string("Sent RPC reply: \"") + response + "\"");

      }
    }
    catch (const boost::thread_interrupted& e)
    {
      MDEBUG("ZMQ Server thread interrupted.");
    }
    catch (const zmq::error_t& e)
    {
      MERROR(std::string("ZMQ error: ") + e.what());
    }
    boost::this_thread::interruption_point();
  }
}

bool ZmqServer::addIPCSocket(std::string address, std::string port)
{
  MERROR("ZmqServer::addIPCSocket not yet implemented!");
  return false;
}

bool ZmqServer::addTCPSocket(std::string address, std::string port)
{
  try
  {
    std::string addr_prefix("tcp://");

    rep_socket.reset(new zmq::socket_t(context, ZMQ_REP));

    rep_socket->setsockopt(ZMQ_RCVTIMEO, &DEFAULT_RPC_RECV_TIMEOUT_MS, sizeof(DEFAULT_RPC_RECV_TIMEOUT_MS));

    std::string bind_address = addr_prefix + address + std::string(":") + port;
    rep_socket->bind(bind_address.c_str());
  }
  catch (const std::exception& e)
  {
    MERROR(std::string("Error creating ZMQ Socket: ") + e.what());
    return false;
  }
  return true;
}

void ZmqServer::run()
{
  running = true;
  run_thread = boost::thread(boost::bind(&ZmqServer::serve, this));
}

void ZmqServer::stop()
{
  if (!running) return;

  stop_signal = true;

  run_thread.interrupt();
  run_thread.join();

  running = false;

  return;
}


}  // namespace cryptonote

}  // namespace rpc
