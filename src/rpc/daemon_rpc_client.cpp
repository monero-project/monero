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
#include "serialization/json_object.h"

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
  rapidjson::Value response_json;

  try
  {
    GetHeight::Request request;

    response_json = doRequest<GetHeight>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetHeight::Response response = parseResponse<GetHeight>(response_json);

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
    uint64_t& current_height,
    std::vector<cryptonote::rpc::block_output_indices>& output_indices,
    std::string& error_details)
{
  rapidjson::Value response_json;

  try
  {
    GetBlocksFast::Request request;

    request.block_ids = block_ids;
    request.start_height = start_height_in;

    response_json = doRequest<GetBlocksFast>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetBlocksFast::Response response = parseResponse<GetBlocksFast>(response_json);

    blocks = response.blocks;
    start_height_out = response.start_height;
    current_height = response.current_height;
    output_indices = response.output_indices;

    return true;
  }
  catch (...)
  {
    try
    {
      cryptonote::rpc::error err = parseError(response_json);

      error_details = err.error_str;
    }
    catch (...)
    {
      error_details = "Daemon returned improper JSON-RPC response.";
    }
  }

  return false;
}

bool DaemonRPCClient::getHashesFast(
    const std::list<crypto::hash>& known_hashes,
    const uint64_t start_height_in,
    std::list<crypto::hash>& hashes,
    uint64_t& start_height_out,
    uint64_t& current_height,
    std::string& error_details)
{
  rapidjson::Value response_json;

  try
  {
    GetHashesFast::Request request;

    request.known_hashes = known_hashes;
    request.start_height = start_height_in;

    response_json = doRequest<GetHashesFast>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetHashesFast::Response response = parseResponse<GetHashesFast>(response_json);

    hashes = response.hashes;
    start_height_out = response.start_height;
    current_height = response.current_height;

    return true;
  }
  catch (...)
  {
    try
    {
      cryptonote::rpc::error err = parseError(response_json);

      error_details = err.error_str;
    }
    catch (...)
    {
      error_details = "Daemon returned improper JSON-RPC response.";
    }
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
  rapidjson::Value response_json;

  try
  {
    KeyImagesSpent::Request request;

    request.key_images = images;

    response_json = doRequest<KeyImagesSpent>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    KeyImagesSpent::Response response = parseResponse<KeyImagesSpent>(response_json);

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
  rapidjson::Value response_json;

  try
  {
    GetTransactionPool::Request request;

    response_json = doRequest<GetTransactionPool>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetTransactionPool::Response response = parseResponse<GetTransactionPool>(response_json);

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
  rapidjson::Value response_json;

  try
  {
    GetTxGlobalOutputIndices::Request request;

    request.tx_hash = tx_hash;

    response_json = doRequest<GetTxGlobalOutputIndices>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetTxGlobalOutputIndices::Response response = parseResponse<GetTxGlobalOutputIndices>(response_json);

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
  rapidjson::Value response_json;

  try
  {
    GetRandomOutputsForAmounts::Request request;

    request.amounts = amounts;
    request.count = count;

    response_json = doRequest<GetRandomOutputsForAmounts>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetRandomOutputsForAmounts::Response response = parseResponse<GetRandomOutputsForAmounts>(response_json);

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
  rapidjson::Value response_json;

  try
  {
    SendRawTx::Request request;

    request.tx = tx;
    request.relay = relay;

    response_json = doRequest<SendRawTx>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    SendRawTx::Response response = parseResponse<SendRawTx>(response_json);

    relayed = response.relayed;

    return true;
  }
  catch (...)
  {
    try
    {
      cryptonote::rpc::error err = parseError(response_json);

      error_details = err.error_str;
    }
    catch (...)
    {
      error_details = "Daemon returned improper JSON-RPC response.";
    }
  }

  return false;
}

bool DaemonRPCClient::hardForkInfo(
    const uint8_t version,
    hard_fork_info& info)
{
  rapidjson::Value response_json;

  try
  {
    HardForkInfo::Request request;

    request.version = version;

    response_json = doRequest<HardForkInfo>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    HardForkInfo::Response response = parseResponse<HardForkInfo>(response_json);

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
    bool unlocked,
    std::vector<output_amount_count>& histogram)
{
  rapidjson::Value response_json;

  try
  {
    GetOutputHistogram::Request request;

    request.amounts = amounts;
    request.min_count = min_count;
    request.max_count = max_count;
    request.unlocked = unlocked;

    response_json = doRequest<GetOutputHistogram>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetOutputHistogram::Response response = parseResponse<GetOutputHistogram>(response_json);

    histogram = response.histogram;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getOutputKeys(
    std::vector<output_amount_and_index>& outputs,
    std::vector<output_key_mask_unlocked>& keys)
{
  rapidjson::Value response_json;

  try
  {
    GetOutputKeys::Request request;

    request.outputs = outputs;

    response_json = doRequest<GetOutputKeys>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetOutputKeys::Response response = parseResponse<GetOutputKeys>(response_json);

    keys = response.keys;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

bool DaemonRPCClient::getRPCVersion(uint32_t& version)
{
  rapidjson::Value response_json;

  try
  {
    GetRPCVersion::Request request;

    response_json = doRequest<GetRPCVersion>(request);
  }
  catch (...)
  {
    return false;
  }

  try
  {
    GetRPCVersion::Response response = parseResponse<GetRPCVersion>(response_json);

    version = response.version;

    return true;
  }
  catch (...)
  {
  }

  return false;
}

template <typename ReqType>
rapidjson::Value DaemonRPCClient::doRequest(typename ReqType::Request& request)
{
  auto full_request = FullMessage::requestMessage(DAEMON_RPC_VERSION, ReqType::name, &request);

  std::string resp = zmq_client.doRequest(full_request.getJson());

  FullMessage full_response(resp);

  return full_response.getMessageCopy();
}

template <typename ReqType>
typename ReqType::Response DaemonRPCClient::parseResponse(rapidjson::Value& resp)
{
  typename ReqType::Response response;

  response.fromJson(resp);

  return response;
}

cryptonote::rpc::error DaemonRPCClient::parseError(rapidjson::Value& err)
{
  return cryptonote::json::fromJsonValue<cryptonote::rpc::error>(err);
}

}  // namespace rpc

}  // namespace cryptonote
