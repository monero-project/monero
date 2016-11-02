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

namespace cryptonote
{

namespace rpc
{

ZmqClient::ZmqClient() : context(1 /* one zmq thread */),
              req_socket(NULL)
{
}

ZmqClient::~ZmqClient()
{
  if (req_socket)
  {
    delete req_socket;  // dtor handles cleanup
  }
}

void ZmqClient::connect(const std::string& address, const std::string& port)
{
  try
  {
    std::string addr_prefix("tcp://");

    zmq::socket_t *socket = new zmq::socket_t(context, ZMQ_REQ);

    std::string connect_address = addr_prefix + address + std::string(":") + port;
    socket->connect(connect_address.c_str());
    req_socket = socket;
  }
  catch (...)
  {
  }
}

std::string ZmqClient::doRequest(const std::string& request, uint64_t timeout_ms)
{
  // TODO: error handling
  if (req_socket == NULL)
  {
    return std::string();
  }

  zmq::message_t request_message(request.size());
  memcpy((void *) request_message.data(), request.c_str(), request.size());

  req_socket->send(request_message);

  std::string response;

  // ignore timeout for now
  // TODO: implement timeout feature
  if (timeout_ms == 0)
  {
    zmq::message_t response_message;

    req_socket->recv(&response_message);

    response = std::string(reinterpret_cast<const char *>(response_message.data()), response_message.size());
  }

  return response;
}


}  // namespace rpc

}  // namespace cryptonote
