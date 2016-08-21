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

bool DaemonRPCClient::keyImagesSpent(
    const std::vector<crypto::key_image>& images,
    std::vector<bool>& spent,
    std::vector<bool>& spent_in_chain,
    std::vector<bool>& spent_in_pool,
    bool where)
{
  try
  {
    KeyImagesSpent::Request request;

    request.key_images = images;

    KeyImagesSpent::Response response = doRequest<KeyImagesSpent>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    spent.resize(images.size());
    if (where)
    {
      spent_in_chain.resize(images.size());
      spent_in_pool.resize(images.size());
    }

    for (size_t i=0; i < response.spent_status.size(); i++)
    {
      spent[i] = (response.spent_status[i] != KeyImagesSpent::UNSPENT);
      if (where)
      {
        spent_in_chain[i] = (response.spent_status[i] == KeyImagesSpent::SPENT_IN_BLOCKCHAIN);
        spent_in_pool[i] = (response.spent_status[i] == KeyImagesSpent::SPENT_IN_POOL);
      }
    }

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

bool DaemonRPCClient::getRandomOutputsForAmounts(
    const std::vector<uint64_t>& amounts,
    const uint64_t count,
    std::vector<amount_with_random_outputs>& amounts_with_outputs)
{
  try
  {
    GetRandomOutputsForAmounts::Request request;

    request.amounts = amounts;
    request.count = count;

    GetRandomOutputsForAmounts::Response response = doRequest<GetRandomOutputsForAmounts>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    amounts_with_outputs = response.amounts_with_outputs;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::sendRawTx(
    const cryptonote::transaction& tx,
    bool& relayed,
    std::string& error_details,
    bool relay)
{
  try
  {
    SendRawTx::Request request;

    request.tx = tx;
    request.relay = relay;

    SendRawTx::Response response = doRequest<SendRawTx>(request);

    error_details = response.error_details;

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    relayed = response.relayed;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::hardForkInfo(
    const uint8_t version,
    hard_fork_info& info)
{
  try
  {
    HardForkInfo::Request request;

    request.version = version;

    HardForkInfo::Response response = doRequest<HardForkInfo>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    info = response.info;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getOutputHistogram(
    const std::vector<uint64_t>& amounts,
    uint64_t min_count,
    uint64_t max_count,
    std::vector<output_amount_count>& histogram)
{
  try
  {
    GetOutputHistogram::Request request;

    request.amounts = amounts;
    request.min_count = min_count;
    request.max_count = max_count;

    GetOutputHistogram::Response response = doRequest<GetOutputHistogram>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    histogram = response.histogram;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getRPCVersion(uint32_t& version)
{
  try
  {
    GetRPCVersion::Request request;

    GetRPCVersion::Response response = doRequest<GetRPCVersion>(request);

    if (response.status != Message::STATUS_OK)
    {
      return false;
    }

    version = response.version;

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
