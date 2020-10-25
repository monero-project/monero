// Copyright (c) 2017-2019, AEON, The Monero Project
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
#include "common/json_util.h"
#include "storages/http_abstract_invoke.h"

using namespace epee;

namespace tools
{

static const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

NodeRPCProxy::NodeRPCProxy(epee::net_utils::http::http_simple_client &http_client, boost::mutex &mutex)
  : m_http_client(http_client)
  , m_daemon_rpc_mutex(mutex)
{
  invalidate();
}

void NodeRPCProxy::invalidate()
{
  m_height = 0;
  for (size_t n = 0; n < 256; ++n)
    m_earliest_height[n] = 0;
  m_rpc_version = 0;
  m_target_height = 0;
  m_block_weight_limit = 0;
  m_adjusted_time = 0;
  m_get_info_time = 0;
  m_height_time = 0;
}

boost::optional<std::string> NodeRPCProxy::get_rpc_version(uint32_t &rpc_version) const
{
  if (m_rpc_version == 0)
  {
    cryptonote::COMMAND_RPC_GET_VERSION::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_VERSION::response resp_t = AUTO_VAL_INIT(resp_t);
    m_daemon_rpc_mutex.lock();
    bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_version", req_t, resp_t, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get daemon RPC version");
    m_rpc_version = resp_t.version;
  }
  rpc_version = m_rpc_version;
  return boost::optional<std::string>();
}

void NodeRPCProxy::set_height(uint64_t h)
{
  m_height = h;
  m_height_time = time(NULL);
}

boost::optional<std::string> NodeRPCProxy::get_info() const
{
  const time_t now = time(NULL);
  if (now >= m_get_info_time + 30) // re-cache every 30 seconds
  {
    cryptonote::COMMAND_RPC_GET_INFO::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_GET_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

    m_daemon_rpc_mutex.lock();
    bool r = net_utils::invoke_http_json_rpc("/json_rpc", "get_info", req_t, resp_t, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();

    CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get target blockchain height");
    m_height = resp_t.height;
    m_target_height = resp_t.target_height;
    m_block_weight_limit = resp_t.block_weight_limit ? resp_t.block_weight_limit : resp_t.block_size_limit;
    m_adjusted_time = resp_t.adjusted_time;
    m_get_info_time = now;
    m_height_time = now;
  }
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_height(uint64_t &height) const
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

boost::optional<std::string> NodeRPCProxy::get_target_height(uint64_t &height) const
{
  auto res = get_info();
  if (res)
    return res;
  height = m_target_height;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_block_weight_limit(uint64_t &block_weight_limit) const
{
  auto res = get_info();
  if (res)
    return res;
  block_weight_limit = m_block_weight_limit;
  return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_adjusted_time(uint64_t &adjusted_time) const
{
    auto res = get_info();
    if (res)
        return res;
    adjusted_time = m_adjusted_time;
    return boost::optional<std::string>();
}

boost::optional<std::string> NodeRPCProxy::get_earliest_height(uint8_t version, uint64_t &earliest_height) const
{
  if (m_earliest_height[version] == 0)
  {
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_HARD_FORK_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

    m_daemon_rpc_mutex.lock();
    req_t.version = version;
    bool r = net_utils::invoke_http_json_rpc("/json_rpc", "hard_fork_info", req_t, resp_t, m_http_client, rpc_timeout);
    m_daemon_rpc_mutex.unlock();
    CHECK_AND_ASSERT_MES(r, std::string(), "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status != CORE_RPC_STATUS_BUSY, resp_t.status, "Failed to connect to daemon");
    CHECK_AND_ASSERT_MES(resp_t.status == CORE_RPC_STATUS_OK, resp_t.status, "Failed to get hard fork status");
    m_earliest_height[version] = resp_t.earliest_height;
  }

  earliest_height = m_earliest_height[version];
  return boost::optional<std::string>();
}

}
