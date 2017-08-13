// Copyright (c) 2016, The Monero Project
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

#include "zmq_client.h"
#include "../../contrib/epee/include/include_base_utils.h"
#include <iostream>

namespace cryptonote
{

namespace rpc
{

ZmqClient::ZmqClient() : context(1 /* one zmq thread */),
              req_socket(nullptr),
              connect_address(""),
              timeout(DEFAULT_RPC_RECEIVE_TIMEOUT_MS)
{
}

ZmqClient::~ZmqClient()
{
}

bool ZmqClient::connect(const std::string& address_with_port, int timeout_ms)
{
  std::string addr_prefix("tcp://");
  connect_address = addr_prefix + address_with_port;
  timeout = timeout_ms;

  return createSocket();
}

bool ZmqClient::connect(const std::string& address, const std::string& port, int timeout_ms)
{
  std::string address_with_port = address + std::string(":") + port;
  return connect(address_with_port, timeout_ms);
}

boost::optional<std::string> ZmqClient::doRequest(const std::string& request, std::string& response)
{
  if (!req_socket)
  {
    resetSocket();
    if (!req_socket)
    {
      LOG_PRINT_L0("RPC request received, but unable to connect to daemon");
      return std::string("No connection to daemon");
    }
  }

  zmq::message_t request_message(request.size());
  memcpy((void *) request_message.data(), request.c_str(), request.size());

  LOG_PRINT_L2(std::string("Sending ZMQ RPC request: \"") + request + "\"");
  req_socket->send(request_message);

  zmq::message_t response_message;

  if (req_socket->recv(&response_message) > 0)
  {
    response = std::string(reinterpret_cast<const char *>(response_message.data()), response_message.size());

    LOG_PRINT_L2(std::string("Recieved ZMQ RPC response: \"") + response + "\"");
  }
  else
  {
    // in case the fault for the failed receive was on our end,
    // reset/recreate the socket.
    resetSocket();
    return std::string("No response from daemon, retry later.");
  }

  return boost::none;
}

bool ZmqClient::createSocket()
{
  static int linger_time = 0;
  try
  {
    LOG_PRINT_L2("Creating ZMQ Socket at: " << connect_address);
    req_socket.reset(new zmq::socket_t(context, ZMQ_REQ));
    req_socket->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    req_socket->setsockopt(ZMQ_LINGER, &linger_time, sizeof(linger_time));
    req_socket->connect(connect_address.c_str());

    LOG_PRINT_L0(std::string("Created ZMQ socket at: ") + connect_address);
  }
  catch (std::exception& e)
  {
    LOG_PRINT_L2("Exception thrown while creating ZMQ Socket at: " << connect_address + ", e.what(): " << e.what());
    req_socket.reset();
    LOG_ERROR(std::string("Failed to connect to ZMQ RPC endpoint at ") + connect_address);
    return false;
  }
  return true;
}

bool ZmqClient::resetSocket()
{
  return createSocket();
}

}  // namespace rpc

}  // namespace cryptonote
