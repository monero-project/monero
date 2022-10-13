// Copyright (c) 2017-2022, The Monero Project
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

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "node_rpc_proxy.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "rpc/rpc_payment_signature.h"
#include "rpc/rpc_payment_costs.h"
#include "storages/http_abstract_invoke.h"

#include <boost/thread.hpp>

#define RETURN_ON_RPC_RESPONSE_ERROR(r, error, res, method) \
  do { \
    CHECK_AND_ASSERT_MES(error.code == 0, error.message, error.message); \
    handle_payment_changes(res, std::integral_constant<bool, HasCredits<decltype(res)>::Has>()); \
    CHECK_AND_ASSERT_MES(r, std::string("Failed to connect to daemon"), "Failed to connect to daemon"); \
    /* empty string -> not connection */ \
    CHECK_AND_ASSERT_MES(!res.status.empty(), res.status, "No connection to daemon"); \
    CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_BUSY, res.status, "Daemon busy"); \
    CHECK_AND_ASSERT_MES(res.status != CORE_RPC_STATUS_PAYMENT_REQUIRED, res.status, "Payment required"); \
    CHECK_AND_ASSERT_MES(res.status == CORE_RPC_STATUS_OK, res.status, "Error calling " + std::string(method) + " daemon RPC"); \
  } while(0)

/**
 * @brief Used to assert conditions and exit early. Logs an error message and returns the same string.
 *
 * @param cond expression which is expected to be truthy
 * @param msg a "stream-like" expression which will feed into a std::stringstream.
 *
 * @return std::string when cond is false, otherwise does not return
 */
#define RETURN_ERROR_IF_FALSE(cond, msg)    \
  do {                                      \
    if (!(cond)) {                          \
      std::stringstream ss;                 \
      ss << msg;                            \
      const std::string err_msg = ss.str(); \
      MERROR(err_msg);                      \
      return err_msg;                       \
    }                                       \
  } while (0);                              \

/**
 * @brief Acquire RPC mutex and invoke /json_rpc endpoint
 *
 * Only really useful within NodeRPCProxy because it assumes the existence of variables m_daemon_rpc_mutex,
 * m_http_client, and rpc_timeout. The lock_guard is scoped so that it lives as short as possible.
 *
 * @param method JSON RPC method name as a string
 * @param req cryptonote::COMMAND_RPC_x::request form which will be used with invoke_http_json_rpc
 * @param res cryptonote::COMMAND_RPC_x::response form which will be used with invoke_http_json_rpc
 *
 * @return std::string on RPC error, otherwise does not return
 */
#define NRP_INVOKE_JSON_RPC(method, req, res)                                   \
  do {                                                                          \
    bool r = false;                                                             \
    {                                                                           \
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex}; \
      r = epee::net_utils::invoke_http_json_rpc("/json_rpc", method, req, res,  \
        m_http_client, rpc_timeout);                                            \
    }                                                                           \
    RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, method);   \
  } while (0);                                                                  \

/**
 * @brief Set client signature, acquire RPC mutex, invoke /json_rpc endpoint, and check RPC cost
 *
 * Only really useful within NodeRPCProxy because it assumes the existence of variables
 * m_daemon_rpc_mutex, m_http_client, m_client_id_secret_key, and rpc_timeout. The lock_guard is
 * scoped so that it lives as short as possible.
 *
 * @param method JSON RPC method name as a string
 * @param req cryptonote::COMMAND_RPC_x::request form which will be used with invoke_http_json_rpc
 * @param res cryptonote::COMMAND_RPC_x::response form which will be used with invoke_http_json_rpc
 * @param rpc_price RPC credit cost for given method, usually specified in macros named COST_PER_x
 *
 * @return std::string on RPC error, otherwise does not return
 */

#define NRP_INVOKE_JSON_RPC_WITH_PRICE(method, req, res, rpc_price)                          \
  do {                                                                                       \
    req.client = cryptonote::make_rpc_payment_signature(m_client_id_secret_key);             \
    {                                                                                        \
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};              \
      const uint64_t pre_call_credits = m_rpc_payment_state.credits;                         \
      const bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", method, req, res,    \
        m_http_client, rpc_timeout);                                                         \
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, method);              \
      check_rpc_cost(m_rpc_payment_state, method, res.credits, pre_call_credits, rpc_price); \
    }                                                                                        \
  } while (0);                                                                               \

/**
 * @brief Set client signature, acquire RPC mutex, invoke json endpoint, and check RPC cost
 *
 * Only really useful within NodeRPCProxy because it assumes the existence of variables
 * m_daemon_rpc_mutex, m_http_client, m_client_id_secret_key, and rpc_timeout. The lock_guard is
 * scoped so that it lives as short as possible.
 *
 * @param uri URI of json endpoint
 * @param req cryptonote::COMMAND_RPC_x::request form which will be used with invoke_http_json_rpc
 * @param res cryptonote::COMMAND_RPC_x::response form which will be used with invoke_http_json_rpc
 * @param rpc_price RPC credit cost for given method, usually specified in macros named COST_PER_x
 *
 * @return std::string on RPC error, otherwise does not return
 */

#define NRP_INVOKE_JSON_WITH_PRICE(uri, req, res, rpc_price)                                 \
  do {                                                                                       \
    req.client = cryptonote::make_rpc_payment_signature(m_client_id_secret_key);             \
    {                                                                                        \
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};              \
      const uint64_t pre_call_credits = m_rpc_payment_state.credits;                         \
      bool r = epee::net_utils::invoke_http_json(uri, req, res, m_http_client, rpc_timeout); \
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, res, uri);                    \
      check_rpc_cost(m_rpc_payment_state, uri, res.credits, pre_call_credits, rpc_price);    \
    }                                                                                        \
  } while (0);                                                                               \

namespace
{
static constexpr const size_t GET_TX_CHUNK_SIZE = 100; // Must be same as RESTRICTED_TRANSACTIONS_COUNT in rpc/core_rpc_server.cpp

/**
 * @brief Parse and validate pruned transaction entry into cryptonote::transaction and its hash.
 *
 * This function can be used on full AND pruned transaction entries, but is mostly used for pruned transaction entries.
 * Takes tx version into account when parsing and validating. Also checks that actual hash values match with expected hash values.
 * However, with v1 transactions, the hash check is skipped for pruned transactions since there is not a way to validate it statically.
 *
 * @param[in] entry Transaction entry response from RPC server with pruned=true, decode_as_json=false
 * @param[out] tx the result of parsing the transaction entry
 * @param[out] tx_hash the result of hashing the pruned transaction
 * @return true on success, false otherwise
 */
bool get_pruned_tx(const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::entry &entry, cryptonote::transaction &tx, crypto::hash &tx_hash)
{
  cryptonote::blobdata bd;

  // easy case if we have the whole tx
  if (!entry.as_hex.empty() || (!entry.prunable_as_hex.empty() && !entry.pruned_as_hex.empty()))
  {
    CHECK_AND_ASSERT_MES(epee::string_tools::parse_hexstr_to_binbuff(entry.as_hex.empty() ? entry.pruned_as_hex + entry.prunable_as_hex : entry.as_hex, bd), false, "Failed to parse tx data");
    CHECK_AND_ASSERT_MES(cryptonote::parse_and_validate_tx_from_blob(bd, tx), false, "Invalid tx data");
    tx_hash = cryptonote::get_transaction_hash(tx);
    // if the hash was given, check it matches
    CHECK_AND_ASSERT_MES(entry.tx_hash.empty() || epee::string_tools::pod_to_hex(tx_hash) == entry.tx_hash, false,
        "Response claims a different hash than the data yields");
    return true;
  }
  // case of a pruned tx with its prunable data hash
  if (!entry.pruned_as_hex.empty() && !entry.prunable_hash.empty())
  {
    crypto::hash ph;
    CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(entry.prunable_hash, ph), false, "Failed to parse prunable hash");
    CHECK_AND_ASSERT_MES(epee::string_tools::parse_hexstr_to_binbuff(entry.pruned_as_hex, bd), false, "Failed to parse pruned data");
    CHECK_AND_ASSERT_MES(parse_and_validate_tx_base_from_blob(bd, tx), false, "Invalid base tx data");
    // only v2 txes can calculate their txid after pruned
    if (bd[0] > 1)
    {
      tx_hash = cryptonote::get_pruned_transaction_hash(tx, ph);
    }
    else
    {
      // for v1, we trust the dameon
      CHECK_AND_ASSERT_MES(epee::string_tools::hex_to_pod(entry.tx_hash, tx_hash), false, "Failed to parse tx hash");
    }
    return true;
  }
  return false;
}
} // anonymous namespace

namespace tools
{

static const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

NodeRPCProxy::NodeRPCProxy(epee::net_utils::http::abstract_http_client &http_client, rpc_payment_state_t &rpc_payment_state, boost::recursive_mutex &mutex)
  : m_http_client(http_client)
  , m_rpc_payment_state(rpc_payment_state)
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
  m_rpc_payment_info_time = 0;
  m_rpc_payment_seed_height = 0;
  m_rpc_payment_seed_hash = crypto::null_hash;
  m_rpc_payment_next_seed_hash = crypto::null_hash;
  m_height_time = 0;
  m_target_height_time = 0;
  m_rpc_payment_diff = 0;
  m_rpc_payment_credits_per_hash_found = 0;
  m_rpc_payment_height = 0;
  m_rpc_payment_cookie = 0;
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
    NRP_INVOKE_JSON_RPC("get_version", req_t, resp_t);

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
    NRP_INVOKE_JSON_RPC_WITH_PRICE("get_info", req_t, resp_t, COST_PER_GET_INFO);

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
    NRP_INVOKE_JSON_RPC_WITH_PRICE("hard_fork_info", req_t, resp_t, COST_PER_HARD_FORK_INFO);

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
    NRP_INVOKE_JSON_RPC_WITH_PRICE("get_fee_estimate", req_t, resp_t, COST_PER_FEE_ESTIMATE);

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
    NRP_INVOKE_JSON_RPC_WITH_PRICE("get_fee_estimate", req_t, resp_t, COST_PER_FEE_ESTIMATE);

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

boost::optional<std::string> NodeRPCProxy::get_rpc_payment_info(bool mining, bool &payment_required, uint64_t &credits, uint64_t &diff, uint64_t &credits_per_hash_found, cryptonote::blobdata &blob, uint64_t &height, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, uint32_t &cookie)
{
  const time_t now = time(NULL);
  if (m_rpc_payment_state.stale || now >= m_rpc_payment_info_time + 5*60 || (mining && now >= m_rpc_payment_info_time + 10)) // re-cache every 10 seconds if mining, 5 minutes otherwise
  {
    cryptonote::COMMAND_RPC_ACCESS_INFO::request req_t = AUTO_VAL_INIT(req_t);
    cryptonote::COMMAND_RPC_ACCESS_INFO::response resp_t = AUTO_VAL_INIT(resp_t);

    {
      const boost::lock_guard<boost::recursive_mutex> lock{m_daemon_rpc_mutex};
      req_t.client = cryptonote::make_rpc_payment_signature(m_client_id_secret_key);
      bool r = epee::net_utils::invoke_http_json_rpc("/json_rpc", "rpc_access_info", req_t, resp_t, m_http_client, rpc_timeout);
      RETURN_ON_RPC_RESPONSE_ERROR(r, epee::json_rpc::error{}, resp_t, "rpc_access_info");
      m_rpc_payment_state.stale = false;
    }

    m_rpc_payment_diff = resp_t.diff;
    m_rpc_payment_credits_per_hash_found = resp_t.credits_per_hash_found;
    m_rpc_payment_height = resp_t.height;
    m_rpc_payment_seed_height = resp_t.seed_height;
    m_rpc_payment_cookie = resp_t.cookie;

    if (m_rpc_payment_diff == 0)
    {
      // If no payment required daemon doesn't give us back a hashing blob
      m_rpc_payment_blob.clear();
    }
    else if (!epee::string_tools::parse_hexstr_to_binbuff(resp_t.hashing_blob, m_rpc_payment_blob) || m_rpc_payment_blob.size() < 43)
    {
      MERROR("Invalid hashing blob: " << resp_t.hashing_blob);
      return std::string("Invalid hashing blob");
    }
    if (resp_t.seed_hash.empty())
    {
      m_rpc_payment_seed_hash = crypto::null_hash;
    }
    else if (!epee::string_tools::hex_to_pod(resp_t.seed_hash, m_rpc_payment_seed_hash))
    {
      MERROR("Invalid seed_hash: " << resp_t.seed_hash);
      return std::string("Invalid seed hash");
    }
    if (resp_t.next_seed_hash.empty())
    {
      m_rpc_payment_next_seed_hash = crypto::null_hash;
    }
    else if (!epee::string_tools::hex_to_pod(resp_t.next_seed_hash, m_rpc_payment_next_seed_hash))
    {
      MERROR("Invalid next_seed_hash: " << resp_t.next_seed_hash);
      return std::string("Invalid next seed hash");
    }
    m_rpc_payment_info_time = now;
  }

  payment_required = m_rpc_payment_diff > 0;
  credits = m_rpc_payment_state.credits;
  diff = m_rpc_payment_diff;
  credits_per_hash_found = m_rpc_payment_credits_per_hash_found;
  blob = m_rpc_payment_blob;
  height = m_rpc_payment_height;
  seed_height = m_rpc_payment_seed_height;
  seed_hash = m_rpc_payment_seed_hash;
  next_seed_hash = m_rpc_payment_next_seed_hash;
  cookie = m_rpc_payment_cookie;
  return boost::none;
} // get_rpc_payment_info()

boost::optional<std::string> NodeRPCProxy::get_transactions(const tx_cont_t<tx_hash_t>& txids, tx_handler_t& cb)
{
  // Start chunking up and translating txids into chunk_txid_strs and call get_transactions_one_chunk for each chunk
  const size_t num_txs = txids.size();
  for (size_t num_txs_done = 0; num_txs_done < num_txs; num_txs_done += GET_TX_CHUNK_SIZE)
  {
    const size_t txs_remaining = num_txs - num_txs_done;
    const size_t this_chunk_size = std::min(txs_remaining, GET_TX_CHUNK_SIZE);

    // Translate this chunk of crypto::hash txids into hex-encoded strings 
    std::vector<std::string> chunk_txid_strs;
    chunk_txid_strs.reserve(this_chunk_size);
    for (size_t chunk_index = 0; chunk_index < this_chunk_size; chunk_index++)
    {
      const crypto::hash& txid = txids[num_txs_done + chunk_index];
      chunk_txid_strs.push_back(epee::string_tools::pod_to_hex(txid));
    }

    const auto result = get_transactions_one_chunk(std::move(chunk_txid_strs), &txids[num_txs_done], cb);
    if (result)
    {
      return result;
    }
  }

  return boost::none;
}

boost::optional<std::string> NodeRPCProxy::get_transaction(const crypto::hash& tx_hash, tx_t& tx_out, tx_entry_t& entry_out)
{
  tx_handler_t tx_passthru_assignment = [&tx_out, &entry_out]
    (tx_t&& tx_m, tx_entry_t&& tx_entry_m, const tx_hash_t& ignored) -> bool
  {
    tx_out = tx_m;
    entry_out = tx_entry_m;
    return true;
  };

  return get_transactions_one_chunk({epee::string_tools::pod_to_hex(tx_hash)}, &tx_hash, tx_passthru_assignment);
}

boost::optional<std::string> NodeRPCProxy::get_transaction(const crypto::hash& tx_hash, tx_t& tx_out)
{
  tx_handler_t tx_passthru_assignment = [&tx_out]
    (tx_t&& tx_m, tx_entry_t&& ignored_1, const tx_hash_t& ignored_2) -> bool
  {
    tx_out = tx_m;
    return true;
  };

  return get_transactions_one_chunk({epee::string_tools::pod_to_hex(tx_hash)}, &tx_hash, tx_passthru_assignment);
}

boost::optional<std::string> NodeRPCProxy::get_transactions_one_chunk(tx_cont_t<std::string>&& tx_hashes_ref, const tx_hash_t* txid_check, tx_handler_t& cb)
{
  // Check if offline
  if (m_offline)
  {
    return std::string("NodeRPCProxy offline");
  }

  // Setup request/response forms
  const size_t num_txs = tx_hashes_ref.size();
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req_t = AUTO_VAL_INIT(req_t);
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response resp_t = AUTO_VAL_INIT(resp_t);
  req_t.txs_hashes = tx_hashes_ref;
  req_t.decode_as_json = false;
  req_t.prune = true;
  req_t.split = false;
  resp_t.txs.reserve(num_txs);

  // Do the request!
  NRP_INVOKE_JSON_WITH_PRICE("/gettransactions", req_t, resp_t, req_t.txs_hashes.size() * COST_PER_TX);

  // Check the number of transaction entries in reponse matches number of requested transactions
  const size_t num_txs_received = resp_t.txs.size();
  RETURN_ERROR_IF_FALSE(num_txs_received == num_txs, "Requested " << num_txs << " txs but got " << num_txs_received);

  // For each tx entry in response...
  for (size_t chunk_index = 0; chunk_index < num_txs; chunk_index++)
  {
    const auto& tx_entry = resp_t.txs[chunk_index];

    // Parse transaction entry to cryptnote::transaction and calculate txid
    cryptonote::transaction tx;
    crypto::hash calculated_tx_hash;
    RETURN_ERROR_IF_FALSE
    (
      get_pruned_tx(tx_entry, tx, calculated_tx_hash),
      "get_pruned_tx() failed on a transaction entry, exiting get_transactions()"
    );

    // Check that the tx hash calculated by get_pruned_tx matches the requested hash
    const crypto::hash& requested_hash = txid_check[chunk_index];
    RETURN_ERROR_IF_FALSE
    (
      calculated_tx_hash == requested_hash,
      "get_transactions() hash mismatch: " << epee::string_tools::pod_to_hex(requested_hash) << " vs "
      << epee::string_tools::pod_to_hex(calculated_tx_hash)
    );

    // This transaction is good. Try calling cb with transaction and its entry and check for cb success
    try
    {
      RETURN_ERROR_IF_FALSE
      (
        cb(std::move(tx), std::move(resp_t.txs[chunk_index]), calculated_tx_hash),
        "get_transactions() callback returned fail code on tx with id " << epee::string_tools::pod_to_hex(requested_hash)
      );
    }
    catch (const std::exception& e)
    {
      // If we catch a callback error, return error status string
      RETURN_ERROR_IF_FALSE(false, "get_transactions_one_chunk() callback exception: " << e.what());
    }
  } // for chunk_index ...

  return boost::none;
}

}
