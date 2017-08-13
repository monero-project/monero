// Copyright (c) 2017, The Monero Project
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

#include <chrono>

#include "rpc/daemon_rpc_client_old.h"

#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

namespace {

  static const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

  //TODO: threaded?
  bool parseBlocksWithTransactions(const std::list<cryptonote::block_complete_entry>& blocks_in, std::vector<cryptonote::rpc::block_with_transactions>& blocks_out)
  {
    blocks_out.clear();
    blocks_out.resize(blocks_in.size());

    size_t i = 0;
    for (const cryptonote::block_complete_entry& entry : blocks_in)
    {
      if (!parse_and_validate_block_from_blob(entry.block, blocks_out[i].block))
      {
        return false;
      }

      for (const cryptonote::blobdata& tx_blob : entry.txs)
      {
        cryptonote::transaction tx;
        crypto::hash tx_hash;
        crypto::hash tx_prefix_hash;

        if (!cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx, tx_hash, tx_prefix_hash))
        {
          return false;
        }
        blocks_out[i].transactions[tx_hash] = tx;
      }
      i++;
    }

    return true;
  }

}  // anonymous namespace

namespace cryptonote
{

namespace rpc
{

DaemonRPCClientOld::DaemonRPCClientOld(const std::string& address, boost::optional<epee::net_utils::http::login> loginInfo) :
    daemonAddress(address),
    loginInfo(loginInfo)
{
  resetCachedValues();
  httpClient.set_server(address, loginInfo);
  connect();
}

boost::optional<std::string> DaemonRPCClientOld::checkConnection(
    uint32_t timeout,
    uint32_t& version)
{
  boost::lock_guard<boost::mutex> lock(inUseMutex);

  if (!connect(timeout))
  {
    return boost::optional<std::string>("Failed to connect to daemon");
  }

  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
  req_t.jsonrpc = "2.0";
  req_t.id = epee::serialization::storage_entry(0);
  req_t.method = "get_version";

  bool r = epee::net_utils::invoke_http_json("/json_rpc", req_t, resp_t, httpClient);
  if(!r) {
    version = 0;
    return boost::optional<std::string>("No response from daemon");
  }

  if (resp_t.result.status != CORE_RPC_STATUS_OK)
    version = 0;
  else
    version = resp_t.result.version;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getHeight(
    uint64_t& height)
{
  const time_t now = time(NULL);

  if (cachedHeight == 0 || now >= lastHeightCheckTime + 30) // re-cache every 30 seconds
  {

    cryptonote::COMMAND_RPC_GET_HEIGHT::request req = AUTO_VAL_INIT(req);
    cryptonote::COMMAND_RPC_GET_HEIGHT::response res = AUTO_VAL_INIT(res);

    bool r;
    {
      boost::lock_guard<boost::mutex> lock(inUseMutex);
      r = epee::net_utils::invoke_http_json("/getheight", req, res, httpClient, rpc_timeout);
    }

    CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_BUSY, res.status, "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(res.status == CORE_RPC_STATUS_OK, res.status, "Failed to get current blockchain height");

    cachedHeight = res.height;
    lastHeightCheckTime = now;
  }

  height = cachedHeight;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getTargetHeight(
    uint64_t& target_height)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_INFO::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_INFO::response, std::string> res = AUTO_VAL_INIT(res);

  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "get_info";

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.result.status != CORE_RPC_STATUS_OK)
  {
    return res.result.status;
  }

  target_height = res.result.target_height;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getNetworkDifficulty(
    uint64_t& difficulty)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_INFO::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_INFO::response, std::string> res = AUTO_VAL_INIT(res);

  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "get_info";

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.result.status != CORE_RPC_STATUS_OK)
  {
    return res.result.status;
  }

  difficulty = res.result.difficulty;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getDifficultyTarget(
    uint64_t& target)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_INFO::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_INFO::response, std::string> res = AUTO_VAL_INIT(res);

  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "get_info";

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.result.status != CORE_RPC_STATUS_OK)
  {
    return res.result.status;
  }

  target = res.result.target;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getBlocksFast(
    const std::list<crypto::hash>& short_chain_history,
    const uint64_t start_height_in,
    const bool prune,
    std::vector<cryptonote::rpc::block_with_transactions>& blocks,
    uint64_t& start_height_out,
    uint64_t& current_height,
    std::vector<cryptonote::rpc::block_output_indices>& output_indices)
{
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response res = AUTO_VAL_INIT(res);

  uint32_t rpc_version;
  boost::optional<std::string> result = getRPCVersion(rpc_version);
  if (result)
  {
    return result;
  }

  if (rpc_version >= MAKE_CORE_RPC_VERSION(1, 7))
  {
    MDEBUG("Daemon is recent enough, asking for pruned blocks");
    req.prune = true;
  }
  else
  {
    MDEBUG("Daemon is too old, not asking for pruned blocks");
    req.prune = false;
  }

  req.block_ids = short_chain_history;
  req.start_height = start_height_in;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/getblocks.bin", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  CHECK_AND_ASSERT_MES(res.blocks.size() == res.output_indices.size(),
      std::string("RPC response did not match request"),
      std::string("mismatched blocks (") +
          boost::lexical_cast<std::string>(res.blocks.size()) +
          ") and output_indices (" +
          boost::lexical_cast<std::string>(res.output_indices.size()) +
          ") sizes from daemon");

  start_height_out = res.start_height;
  current_height = res.current_height;

  if (!parseBlocksWithTransactions(res.blocks, blocks))
  {
    return std::string("Parsing of blocks or transactions failed");
  }

  output_indices.resize(res.output_indices.size());
  for (size_t i = 0; i < res.output_indices.size(); i++)
  {
    output_indices[i].resize(res.output_indices[i].indices.size());
    for (size_t j = 0; j < res.output_indices[i].indices.size(); j++)
    {
      output_indices[i][j] = std::move(res.output_indices[i].indices[j].indices);
    }
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getHashesFast(
    const std::list<crypto::hash>& short_chain_history,
    const uint64_t start_height_in,
    std::list<crypto::hash>& hashes,
    uint64_t& start_height_out,
    uint64_t& current_height)
{
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::request req = AUTO_VAL_INIT(req);
  cryptonote::COMMAND_RPC_GET_HASHES_FAST::response res = AUTO_VAL_INIT(res);

  req.block_ids = short_chain_history;
  req.start_height = start_height_in;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/gethashes.bin", req, res, httpClient, rpc_timeout);
  }

  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  start_height_out = res.start_height;
  hashes = res.m_block_ids;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getTransactions(
    const std::vector<crypto::hash>& tx_hashes,
    std::unordered_map<crypto::hash, cryptonote::rpc::transaction_info> txs,
    std::vector<crypto::hash> missed_hashes)
{
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

  for (const auto& hash : tx_hashes)
  {
    req.txs_hashes.push_back(epee::string_tools::pod_to_hex(hash));
  }

  req.decode_as_json = false;
  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/gettransactions", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  for (const auto& tx_data : res.txs)
  {
    cryptonote::transaction tx;
    cryptonote::blobdata bd;
    crypto::hash tx_hash, tx_prefix_hash;
    if (!epee::string_tools::parse_hexstr_to_binbuff(tx_data.as_hex, bd))
    {
      return std::string("Failed to parse tx from RPC response");
    }
    if (!cryptonote::parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash))
    {
      return std::string("Failed to parse tx from RPC response");
    }

    cryptonote::rpc::transaction_info tx_info;

    tx_info.transaction = tx;
    tx_info.in_pool = tx_data.in_pool;
    tx_info.height = tx_data.block_height;
    txs[tx_hash] = tx_info;
  }

  for (const std::string& tx_hash : res.missed_tx)
  {
    crypto::hash h;
    if (!epee::string_tools::hex_to_pod(tx_hash, h))
    {
      return std::string("Failed to parse tx hash from RPC response");
    }
    missed_hashes.push_back(h);
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getBlockHeadersByHeight(
    const std::vector<uint64_t>& heights,
    std::vector<cryptonote::rpc::BlockHeaderResponse> headers)
{

  COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::request req;
  COMMAND_RPC_GET_BLOCKS_BY_HEIGHT::response res;

  req.heights = heights;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/getblocks_by_height.bin", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }
  if (res.blocks.size() != heights.size())
  {
    return std::string("Mismatched RPC response size");
  }

  headers.clear();
  headers.resize(heights.size());

  for (size_t i=0; i < heights.size(); i++)
  {
    cryptonote::block bl;
    if (!parse_and_validate_block_from_blob(res.blocks[i].block, bl))
    {
      return std::string("Failed to parse response from RPC");
    }

    //TODO: add other header members if needed
    headers[i].timestamp = bl.timestamp;
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::keyImagesSpent(
    const std::vector<crypto::key_image>& images,
    std::vector<bool>& spent,
    std::vector<bool>& spent_in_chain,
    std::vector<bool>& spent_in_pool)
{

  COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_IS_KEY_IMAGE_SPENT::response res = AUTO_VAL_INIT(res);

  for (const auto& key_image : images)
  {
    req.key_images.push_back(epee::string_tools::pod_to_hex(key_image));
  }

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/is_key_image_spent", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }
  if (res.spent_status.size() != images.size())
  {
    return std::string("Mismatched RPC response size");
  }

  spent.clear();
  spent_in_chain.clear();
  spent_in_pool.clear();

  spent.resize(images.size());
  spent_in_chain.resize(images.size());
  spent_in_pool.resize(images.size());

  for (size_t i = 0; i < images.size(); i++)
  {
    spent[i] = (res.spent_status[i] != COMMAND_RPC_IS_KEY_IMAGE_SPENT::UNSPENT);
    spent_in_chain[i] = (res.spent_status[i] == COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_BLOCKCHAIN);
    spent_in_pool[i] = (res.spent_status[i] == COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_POOL);
  }

  return boost::none;
}

// TODO: if key_images is needed by anything using this hopefully
// soon to be deprecated port of the old RPC, implement it.
boost::optional<std::string> DaemonRPCClientOld::getTransactionPool(
    std::unordered_map<crypto::hash, cryptonote::rpc::tx_in_pool>& transactions,
    std::unordered_map<crypto::key_image, std::vector<crypto::hash> >& key_images)
{

  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::request hashes_req;
  cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_HASHES::response hashes_res;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/get_transaction_pool_hashes.bin", hashes_req, hashes_res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (hashes_res.status != CORE_RPC_STATUS_OK)
  {
    return hashes_res.status;
  }

  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request txs_req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response txs_res;

  for (const auto &txid : hashes_res.tx_hashes)
  {
    txs_req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  }

  txs_req.decode_as_json = false;

  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/gettransactions", txs_req, txs_res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (txs_res.status != CORE_RPC_STATUS_OK)
  {
    return txs_res.status;
  }

  for (const auto& tx_data : txs_res.txs)
  {
    if (tx_data.in_pool)
    {
      cryptonote::transaction tx;
      cryptonote::blobdata bd;
      crypto::hash tx_hash, tx_prefix_hash;
      if (!epee::string_tools::parse_hexstr_to_binbuff(tx_data.as_hex, bd))
      {
        return std::string("Failed to parse tx from RPC response");
      }
      if (!cryptonote::parse_and_validate_tx_from_blob(bd, tx, tx_hash, tx_prefix_hash))
      {
        return std::string("Failed to parse tx from RPC response");
      }

      cryptonote::rpc::tx_in_pool in_pool;

      in_pool.tx = tx;
      in_pool.tx_hash = tx_hash;
      transactions[tx_hash] = in_pool;
    }
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getRandomOutputsForAmounts(
    const std::vector<uint64_t>& amounts,
    const uint64_t count,
    std::vector<amount_with_random_outputs>& amounts_with_outputs)
{

  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response res = AUTO_VAL_INIT(res);

  req.amounts = amounts;
  req.outs_count = count;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/getrandom_outs.bin", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }
  if (res.outs.size() != amounts.size())
  {
    return std::string("Mismatched RPC response size");
  }

  amounts_with_outputs.clear();
  amounts_with_outputs.resize(amounts.size());

  for (size_t i=0; i < amounts.size(); i++)
  {
    amounts_with_outputs[i].amount = res.outs[i].amount;
    amounts_with_outputs[i].outputs.resize(res.outs[i].outs.size());
    size_t j = 0;
    for (const auto& out : res.outs[i].outs)
    {
      amounts_with_outputs[i].outputs[j].amount_index = out.global_amount_index;
      amounts_with_outputs[i].outputs[j].key = out.out_key;
      j++;
    }
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::sendRawTx(
    const cryptonote::transaction& tx,
    bool& relayed,
    bool relay)
{

  crypto::hash txid;

  COMMAND_RPC_SEND_RAW_TX::request req;
  COMMAND_RPC_SEND_RAW_TX::response res;
  req.tx_as_hex = epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(tx));
  req.do_not_relay = false;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/sendrawtransaction", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (!res.reason.empty())
  {
    return std::string("Daemon rejected tx, reason: ") + res.reason;
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  relayed = !res.not_relayed;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::hardForkInfo(
    const uint8_t version,
    hard_fork_info& info)
{
  boost::lock_guard<boost::mutex> lock(inUseMutex);

  epee::json_rpc::request<cryptonote::COMMAND_RPC_HARD_FORK_INFO::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_HARD_FORK_INFO::response, std::string> res = AUTO_VAL_INIT(res);

  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "hard_fork_info";
  req.params.version = version;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.result.status != CORE_RPC_STATUS_OK)
  {
    return res.result.status;
  }

  info.version = res.result.version;
  info.enabled = res.result.enabled;
  info.window = res.result.window;
  info.votes = res.result.votes;
  info.threshold = res.result.threshold;
  info.voting = res.result.voting;
  info.state = res.result.state;
  info.earliest_height = res.result.earliest_height;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getHardForkEarliestHeight(
    const uint8_t version,
    uint64_t& earliest_height)
{
  if (cachedHardForkEarliestHeights[version] == 0)
  {
    hard_fork_info info;

    boost::optional<std::string> r = hardForkInfo(version, info);

    if (r) return r;

    cachedHardForkEarliestHeights[version] = info.enabled ? info.earliest_height : std::numeric_limits<uint64_t>::max();
  }

  earliest_height = cachedHardForkEarliestHeights[version];

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getOutputHistogram(
    const std::vector<uint64_t>& amounts,
    uint64_t min_count,
    uint64_t max_count,
    bool unlocked,
    uint64_t recent_cutoff,
    std::vector<output_amount_count>& histogram)
{
  epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request> req = AUTO_VAL_INIT(req);
  epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response, std::string> res = AUTO_VAL_INIT(res);

  req.jsonrpc = "2.0";
  req.id = epee::serialization::storage_entry(0);
  req.method = "get_output_histogram";

  req.params.amounts = amounts;

  req.params.unlocked = unlocked;
  req.params.min_count = min_count;
  req.params.max_count = max_count;
  req.params.recent_cutoff = recent_cutoff;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.result.status != CORE_RPC_STATUS_OK)
  {
    return res.result.status;
  }

  histogram.clear();
  histogram.resize(res.result.histogram.size());

  for (size_t i=0; i < histogram.size(); i++)
  {
    histogram[i].amount = res.result.histogram[i].amount;
    histogram[i].total_count = res.result.histogram[i].total_instances;
    histogram[i].unlocked_count = res.result.histogram[i].unlocked_instances;
    histogram[i].recent_count = res.result.histogram[i].recent_instances;
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getOutputKeys(
    const std::vector<output_amount_and_index>& outputs,
    std::vector<output_key_mask_unlocked>& keys)
{

  COMMAND_RPC_GET_OUTPUTS_BIN::request req = AUTO_VAL_INIT(req);
  COMMAND_RPC_GET_OUTPUTS_BIN::response res = AUTO_VAL_INIT(res);

  req.outputs.resize(outputs.size());
  for (size_t i=0; i < outputs.size(); i++)
  {
    req.outputs[i].amount = outputs[i].amount;
    req.outputs[i].index = outputs[i].index;
  }

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_bin("/get_outs.bin", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }
  if (res.outs.size() != outputs.size())
  {
    return std::string("Mismatched RPC response size");
  }

  keys.clear();
  keys.resize(outputs.size());

  for (size_t i=0; i < keys.size(); i++)
  {
    keys[i].key = res.outs[i].key;
    keys[i].mask = res.outs[i].mask;
    keys[i].unlocked = res.outs[i].unlocked;
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getRPCVersion(
    uint32_t& version)
{

  if (cachedRPCVersion == 0)
  {
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req = AUTO_VAL_INIT(req);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> res = AUTO_VAL_INIT(res);
    req.jsonrpc = "2.0";
    req.id = epee::serialization::storage_entry(0);
    req.method = "get_version";

    bool r;
    {
      boost::lock_guard<boost::mutex> lock(inUseMutex);
      r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient, rpc_timeout);
    }
    if (!r)
    {
      return std::string("Failed to connect to daemon");
    }
    if (res.result.status != CORE_RPC_STATUS_OK)
    {
      return res.result.status;
    }

    cachedRPCVersion = res.result.version;
  }

  version = cachedRPCVersion;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getPerKBFeeEstimate(
    const uint64_t num_grace_blocks,
    uint64_t& estimated_per_kb_fee)
{

  uint64_t height;

  boost::optional<std::string> result = getHeight(height);
  if (result)
    return result;

  if (cachedPerKBFeeEstimateHeight != height || cachedPerKBFeeEstimateGraceBlocks != num_grace_blocks)
  {
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::request> req = AUTO_VAL_INIT(req);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_PER_KB_FEE_ESTIMATE::response, std::string> res = AUTO_VAL_INIT(res);

    req.jsonrpc = "2.0";
    req.id = epee::serialization::storage_entry(0);
    req.method = "get_fee_estimate";
    req.params.grace_blocks = num_grace_blocks;

    bool r;
    {
      boost::lock_guard<boost::mutex> lock(inUseMutex);
      r = epee::net_utils::invoke_http_json("/json_rpc", req, res, httpClient, rpc_timeout);
    }
    if (!r)
    {
      return std::string("Failed to connect to daemon");
    }
    if (res.result.status != CORE_RPC_STATUS_OK)
    {
      return res.result.status;
    }

    cachedPerKBFeeEstimate = res.result.fee;
    cachedPerKBFeeEstimateHeight = height;
    cachedPerKBFeeEstimateGraceBlocks = num_grace_blocks;
  }

  estimated_per_kb_fee = cachedPerKBFeeEstimate;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::getMiningHashRate(
    uint64_t& speed)
{
  cryptonote::COMMAND_RPC_MINING_STATUS::request req;
  cryptonote::COMMAND_RPC_MINING_STATUS::response res;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/mining_status", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  speed = res.active ? res.speed : 0;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::isMining(
    bool& status)
{
  cryptonote::COMMAND_RPC_MINING_STATUS::request req;
  cryptonote::COMMAND_RPC_MINING_STATUS::response res;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/mining_status", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  status = res.active;

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::startMining(
    const std::string& miner_address,
    const uint64_t threads_count,
    const bool do_background_mining,
    const bool ignore_battery)
{
  cryptonote::COMMAND_RPC_START_MINING::request req;
  cryptonote::COMMAND_RPC_START_MINING::response res;

  req.miner_address = miner_address;
  req.threads_count = threads_count;
  req.do_background_mining = do_background_mining;
  req.ignore_battery = ignore_battery;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/start_mining", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  return boost::none;
}

boost::optional<std::string> DaemonRPCClientOld::stopMining()
{
  cryptonote::COMMAND_RPC_STOP_MINING::request req;
  cryptonote::COMMAND_RPC_STOP_MINING::response res;

  bool r;
  {
    boost::lock_guard<boost::mutex> lock(inUseMutex);
    r = epee::net_utils::invoke_http_json("/stop_mining", req, res, httpClient, rpc_timeout);
  }
  if (!r)
  {
    return std::string("Failed to connect to daemon");
  }
  if (res.status != CORE_RPC_STATUS_OK)
  {
    return res.status;
  }

  return boost::none;
}

uint32_t DaemonRPCClientOld::getOurRPCVersion()
{
  return CORE_RPC_VERSION;
}

bool DaemonRPCClientOld::connect(uint32_t timeout)
{
  if(!httpClient.is_connected())
  {
    resetCachedValues();
    if (!httpClient.connect(std::chrono::milliseconds(timeout)))
    {
      return false;
    }
  }
  return true;
}

void DaemonRPCClientOld::resetCachedValues()
{
  cachedHeight = 0;
  cachedRPCVersion = 0;
  lastHeightCheckTime = 0;

  for (size_t n = 0; n < 256; ++n)
    cachedHardForkEarliestHeights[n] = 0;
}


} // namespace rpc
} // namespace cryptonote
