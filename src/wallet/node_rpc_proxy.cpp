// Copyright (c) 2017-2024, The Monero Project
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

#include "node_rpc_proxy.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/http_abstract_invoke.h"

#include <boost/thread.hpp>

#define RETURN_ON_RPC_RESPONSE_ERROR(r, error, res, method) \
  do { \
    CHECK_AND_ASSERT_MES(error.code == 0, error.message, error.message); \
    CHECK_AND_ASSERT_MES(r, std::string("Failed to connect to daemon"), "Failed to connect to daemon"); \
    /* empty string -> not connection */ \
    CHECK_AND_ASSERT_MES(!res.status.empty(), res.status, "No connection to daemon"); \
    CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_BUSY, res.status, "Daemon busy"); \
    CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_PAYMENT_REQUIRED, res.status, "Payment required"); \
    CHECK_AND_ASSERT_MES(res.status == CORE_RPC_STATUS_OK, res.status, "Error calling " + std::string(method) + " daemon RPC"); \
  } while(0)

using namespace epee;

namespace tools
{

static const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

NodeRPCProxy::NodeRPCProxy(epee::net_utils::http::abstract_http_client &http_client, boost::recursive_mutex &mutex)
  : m_http_client(http_client)
  , m_daemon_rpc_mutex(mutex)
  , m_offline(false)
{
  invalidate();
}

void NodeRPCProxy::invalidate()
{
  m_height = 0;
  for (size_t n = 0; n < 256; ++n)
    m_earliest_height[n] = 0;
  m_dynamic_base_fee_estimate = 0;
  m_dynamic_base_fee_estimate_cached_height = 0;
  m_dynamic_base_fee_estimate_grace_blocks = 0;
  m_dynamic_base_fee_estimate_vector.clear();
  m_fee_quantization_mask = 1;
  m_rpc_version = 0;
  m_target_height = 0;
  m_block_weight_limit = 0;
  m_adjusted_time = 0;
  m_get_info_time = 0;
  m_height_time = 0;
  m_target_height_time = 0;
  m_daemon_hard_forks.clear();
}

boost::optional<std::string> NodeRPCProxy::get_rpc_version(uint32_t &rpc_version, std::vector<std::pair<uint8_t, uint64_t>> &daemon_hard_forks, uint64_t &height, uint64_t &target_height)
{
  if (m_offline)
    return boost::optional<std::string>("offline");
  if (m_rpc_version == 0)
  {
    const time_t now = time(NULL);
    cryptonote::COMMAND_RPC_GET_VERSION::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_VERSION::response resp_t = AUTO_VAL_INIT(resp_t);
    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_version", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "get_version");
    }

    m_rpc_version = resp_t.version;
    m_daemon_hard_forks.clear();
    for (const auto &hf : resp_t.hard_forks)
      m_daemon_hard_forks.push_back(std::make_pair(hf.hf_version, hf.height));
    if (resp_t.current_height > 0 || resp_t.target_height > 0)
    {
      m_height = resp_t.current_height;
      m_target_height = resp_t.target_height;
      m_height_time = now;
      m_target_height_time = now;
    }
  }

  rpc_version = m_rpc_version;
  daemon_hard_forks = m_daemon_hard_forks;
  boost::optional<std::string> result = get_height(height);
  if (result)
    return result;
  result = get_target_height(target_height);
  if (result)
    return result;
  return boost::optional<std::string>();
}

void NodeRPCProxy::set_height(uint64_t h)
{
  m_height = h;
  m_height_time = time(NULL);
}

boost::optional<std::string> NodeRPCProxy::get_info()
{
  if (m_offline)
    return boost::optional<std::string>("offline");
  const time_t now = time(NULL);
  if (now >= m_get_info_time + 30) // re-cache every 30 seconds
  {
    cryptonote::COMMAND_RPC_GET_INFO::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_info", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "get_info");
    }

    m_height = resp_t.height;
    m_target_height = resp_t.target_height;
    m_block_weight_limit = resp_t.block_weight_limit ? resp_t.block_weight_limit : resp_t.block_size_limit;
    m_adjusted_time = resp_t.adjusted_time;
    m_get_info_time = now;
    m_height_time = now;
    m_target_height_time = now;
  }
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_height(uint64_t &height)
{
  const time_t now = time(NULL);
  if (now < m_height_time + 30) // re-cache every 30 seconds
  {
    height = m_height;
    return boost::optional<std::string>();
  }

  auto res = get_info();
  if (res)
    return res;
  height = m_height;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_target_height(uint64_t &height)
{
  const time_t now = time(NULL);
  if (now < m_target_height_time + 30) // re-cache every 30 seconds
  {
    height = m_target_height;
    return boost::optional<std::string>();
  }

  auto res = get_info();
  if (res)
    return res;
  height = m_target_height;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_block_weight_limit(uint64_t &block_weight_limit)
{
  auto res = get_info();
  if (res)
    return res;
  block_weight_limit = m_block_weight_limit;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_adjusted_time(uint64_t &adjusted_time)
{
    auto res = get_info();
    if (res)
        return res;
    adjusted_time = m_adjusted_time;
    return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_earliest_height(uint8_t version, uint64_t &earliest_height)
{
  if (m_offline)
    return boost::optional<std::string>("offline");
  if (m_earliest_height[version] == 0)
  {
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::response resp_t = AUTO_VAL_INIT(resp_t);
    req_t.version = version;

    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      bool r = net_utils::invoke_http_json_rpc("/json_rpc", "hard_fork_info", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "hard_fork_info");
    }

    m_earliest_height[version] = resp_t.earliest_height;
  }

  earliest_height = m_earliest_height[version];
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_dynamic_base_fee_estimate_2021_scaling(uint64_t grace_blocks, std::vector<uint64_t> &fees)
{
  uint64_t height;

  boost::optional<std::string> result = get_height(height);
  if (result)
    return result;

  if (m_offline)
    return boost::optional<std::string>("offline");
  if (m_dynamic_base_fee_estimate_cached_height != height || m_dynamic_base_fee_estimate_grace_blocks != grace_blocks)
  {
    cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response resp_t = AUTO_VAL_INIT(resp_t);
    req_t.grace_blocks = grace_blocks;

    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_fee_estimate", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "get_fee_estimate");
    }

    m_dynamic_base_fee_estimate = resp_t.fee;
    m_dynamic_base_fee_estimate_cached_height = height;
    m_dynamic_base_fee_estimate_grace_blocks = grace_blocks;
    m_dynamic_base_fee_estimate_vector = !resp_t.fees.empty() ? std::move(resp_t.fees) : std::vector<uint64_t>{m_dynamic_base_fee_estimate};
    m_fee_quantization_mask = resp_t.quantization_mask;
  }

  fees = m_dynamic_base_fee_estimate_vector;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_dynamic_base_fee_estimate(uint64_t grace_blocks, uint64_t &fee)
{
  std::vector<uint64_t> fees;
  auto res = get_dynamic_base_fee_estimate_2021_scaling(grace_blocks, fees);
  if (res)
    return res;
  fee = fees[0];
  return boost::none;
}

boost::optional<std::string> NodeRPCProxy::get_fee_quantization_mask(uint64_t &fee_quantization_mask)
{
  uint64_t height;

  boost::optional<std::string> result = get_height(height);
  if (result)
    return result;

  if (m_offline)
    return boost::optional<std::string>("offline");
  if (m_dynamic_base_fee_estimate_cached_height != height)
  {
    cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_BASE_FEE_ESTIMATE::response resp_t = AUTO_VAL_INIT(resp_t);
    req_t.grace_blocks = m_dynamic_base_fee_estimate_grace_blocks;

    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_fee_estimate", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "get_fee_estimate");
    }

    m_dynamic_base_fee_estimate = resp_t.fee;
    m_dynamic_base_fee_estimate_cached_height = height;
    m_fee_quantization_mask = resp_t.quantization_mask;
  }

  fee_quantization_mask = m_fee_quantization_mask;
  if (fee_quantization_mask == 0)
  {
    MERROR("Fee quantization mask is 0, forcing to 1");
    fee_quantization_mask = 1;
  }
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_transactions(const std::vector<crypto::hash> &txids, const std::function<void(const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request&, const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response&, bool)> &f)
{
  const size_t SLICE_SIZE = 100; // RESTRICTED_TRANSACTIONS_COUNT as defined in rpc/core_rpc_server.cpp
  for (size_t offset = 0; offset < txids.size(); offset += SLICE_SIZE)
  {
    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response resp_t = AUTO_VAL_INIT(resp_t);

    const size_t n_txids = std::min<size_t>(SLICE_SIZE, txids.size() - offset);
    for (size_t n = offset; n < (offset + n_txids); ++n)
      req_t.txs_hashes.push_back(epee::string_tools::pod_to_hex(txids[n]));
    MDEBUG("asking for " << req_t.txs_hashes.size() << " transactions");
    req_t.decode_as_json = false;
    req_t.prune = true;

    bool r = false;
    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      r = net_utils::invoke_http_json("/gettransactions", req_t, resp_t, m_http_client, rpc_timeout);
    }

    f(req_t, resp_t, r);
  }
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_block_header_by_height(uint64_t height, cryptonote::block_header_response &block_header)
{
  if (m_offline)
    return boost::optional<std::string>("offline");

  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::request req_t = AUTO_VAL_INIT(req_t);
  cryptonote::COMMAND_RPC_GET_BLOCK_HEADER_BY_HEIGHT::response resp_t = AUTO_VAL_INIT(resp_t);
  req_t.height = height;

  {
    const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
    bool r = net_utils::invoke_http_json_rpc("/json_rpc", "getblockheaderbyheight", req_t, resp_t, m_http_client, rpc_timeout);
    RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "getblockheaderbyheight");
  }

  block_header = std::move(resp_t.block_header);
  return boost::optional<std::string>();
}

}
