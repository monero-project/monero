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

bool DaemonRPCClient::getHeight(uint64_t& height)
{
  try
  {
    GetHeight::Request request;

    GetHeight::Response response = doRequest<GetHeight>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    height = response.height;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getBlocksFast(
    const std::list<crypto::hash>& block_ids,
    const uint64_t start_height_in,
    std::vector<cryptonote::rpc::block_with_transactions>& blocks,
    uint64_t& start_height_out,
    uint64_t& current_height)
{
  try
  {
    GetBlocksFast::Request request;

    request.block_ids = block_ids;
    request.start_height = start_height_in;

    GetBlocksFast::Response response = doRequest<GetBlocksFast>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    blocks = response.blocks;
    start_height_out = response.start_height;
    current_height = response.current_height;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getHashesFast(
    const std::list<crypto::hash>& known_hashes,
    const uint64_t start_height_in,
    std::list<crypto::hash>& hashes,
    uint64_t& start_height_out,
    uint64_t& current_height)
{
  try
  {
    GetHashesFast::Request request;

    request.known_hashes = known_hashes;
    request.start_height = start_height_in;

    GetHashesFast::Response response = doRequest<GetHashesFast>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    hashes = response.hashes;
    start_height_out = response.start_height;
    current_height = response.current_height;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getTransactionPool(
    std::unordered_map<crypto::hash, cryptonote::rpc::tx_in_pool>& transactions,
    std::unordered_map<crypto::key_image, std::vector<crypto::hash> >& key_images)
{
  try
  {
    GetTransactionPool::Request request;

    GetTransactionPool::Response response = doRequest<GetTransactionPool>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    transactions = response.transactions;
    key_images = response.key_images;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getTxGlobalOutputIndices(const crypto::hash& tx_hash, std::vector<uint64_t>& output_indices)
{
  try
  {
    GetTxGlobalOutputIndices::Request request;

    request.tx_hash = tx_hash;

    GetTxGlobalOutputIndices::Response response = doRequest<GetTxGlobalOutputIndices>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    output_indices = response.output_indices;

    return true;
  }
  catch (...)
  {
  }

  return false;
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
