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

#include "rpc/daemon_rpc_client.h"

#include "rpc/daemon_messages.h"

namespace cryptonote
{

namespace rpc
{

DaemonRPCClient::DaemonRPCClient()
{
}
DaemonRPCClient::~DaemonRPCClient()
{
}

void DaemonRPCClient::connect(const std::string& addr, const std::string& port)
{
  zmq_client.connect(addr, port);
}

uint64_t DaemonRPCClient::getHeight()
{
  GetHeight::Request request;

  GetHeight::Response response = doRequest<GetHeight>(request);

  return response.height;
}


template <typename ReqType>
typename ReqType::Response DaemonRPCClient::doRequest(typename ReqType::Request& request)
{
  FullMessage full_request(1, ReqType::name, &request);

  std::string resp = zmq_client.doRequest(full_request.getJson());

  FullMessage full_response(resp);

  typename ReqType::Response response;

  response.fromJson(full_response.getMessage());

  return response;
}

}  // namespace rpc

}  // namespace cryptonote
