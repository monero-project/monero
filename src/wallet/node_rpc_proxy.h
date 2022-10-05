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

#pragma once

#include <string>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "include_base_utils.h"
#include "net/abstract_http_client.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet_rpc_helpers.h"

namespace tools
{

class NodeRPCProxy
{
public:
  NodeRPCProxy(epee::net_utils::http::abstract_http_client &http_client, rpc_payment_state_t &rpc_payment_state, boost::recursive_mutex &mutex);

  void set_client_secret_key(const crypto::secret_key &skey) { m_client_id_secret_key = skey; }
  void invalidate();
  void set_offline(bool offline) { m_offline = offline; }

  boost::optional<std::string> get_rpc_version(uint32_t &rpc_version, std::vector<std::pair<uint8_t, uint64_t>> &daemon_hard_forks, uint64_t &height, uint64_t &target_height);
  boost::optional<std::string> get_height(uint64_t &height);
  void set_height(uint64_t h);
  boost::optional<std::string> get_target_height(uint64_t &height);
  boost::optional<std::string> get_block_weight_limit(uint64_t &block_weight_limit);
  boost::optional<std::string> get_adjusted_time(uint64_t &adjusted_time);
  boost::optional<std::string> get_earliest_height(uint8_t version, uint64_t &earliest_height);
  boost::optional<std::string> get_dynamic_base_fee_estimate(uint64_t grace_blocks, uint64_t &fee);
  boost::optional<std::string> get_dynamic_base_fee_estimate_2021_scaling(uint64_t grace_blocks, std::vector<uint64_t> &fees);
  boost::optional<std::string> get_fee_quantization_mask(uint64_t &fee_quantization_mask);
  boost::optional<std::string> get_rpc_payment_info(bool mining, bool &payment_required, uint64_t &credits, uint64_t &diff, uint64_t &credits_per_hash_found, cryptonote::blobdata &blob, uint64_t &height, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, uint32_t &cookie);

  // Type aliases used for get_tranaction[s]() methods
  template <typename T>
  using tx_cont_t = std::vector<T>;
  using tx_t = cryptonote::transaction;
  using tx_entry_t = cryptonote::COMMAND_RPC_GET_TRANSACTIONS::entry;
  using tx_hash_t = crypto::hash;
  using tx_handler_t = std::function<bool(tx_t&&, tx_entry_t&&, const tx_hash_t&)>;

  /**
   * @brief Invoke /get_transactions with list of hashes, and call handler for each entry.
   *
   * We make a /get_transactions request with the given txids encoded as hex strings,
   * decode_as_json=false, pruned=true, and split=false. If there are too many txids for one RPC
   * request, we chunk them up into multiple requests. The tx_handler_t callback takes three
   * arguments: a tx_t&&, a tx_entry_t&&m and a const tx_hash&. RPC errors, transaction parsing
   * errors, missing txids, and mismatched txids are all checked before invoking the callback, so
   * the caller can assume base transaction validity. This method is thread-safe.
   *
   * This method is slower than the other get_transactions endpoint since the crypto::hash values
   * have to be translated into std::strings before being sent over the wire.
   *
   * @param txids contains request txids
   * @param cb tx_handler_t which processes received transactions
   *
   * @return boost::none on success, otherwise returns error message
   */
  boost::optional<std::string> get_transactions(const tx_cont_t<tx_hash_t>& txids, tx_handler_t& cb);

  /**
   * @brief Invoke /get_transactions with list of hashes, and call handler for each entry.
   *
   * Same as previous get_transactions() method, but txids container is moved instead of copied.
   *
   * @param txids contains request txids
   * @param cb tx_handler_t which processes received transactions
   *
   * @return boost::none on success, otherwise returns error message
   */
  boost::optional<std::string> get_transactions(tx_cont_t<std::string>&& txids, tx_handler_t& cb);

  /**
   * @brief Invoke /get_transactions with one hash, and return parsed transaction.
   *
   * Same as previous get_transactions() method, but we modify the transaction by reference instead
   * of using a callback.
   *
   * @param txids contains request txids
   * @param[out] tx_res parsed transaction returned by /get_transactions
   *
   * @return boost::none on success, otherwise returns error message
   */
  boost::optional<std::string> get_transaction(const tx_hash_t& txid, tx_t& tx_res);

  /**
   * @brief Invoke /get_transactions  with one hash, and return parsed transaction.
   *
   * Same as previous get_transaction() method, but we also get a tx entry output paramter.
   *
   * @param txids contains request txids
   * @param[out] tx_res parsed transaction returned by /get_transactions
   * @param[out] tx_entry_res raw transaction entry received from response
   *
   * @return boost::none on success, otherwise returns error message
   */
  boost::optional<std::string> get_transaction(const tx_hash_t& txid, tx_t& tx_res, tx_entry_t& tx_entry_res);

private:
  template<typename T> void handle_payment_changes(const T &res, std::true_type) {
    if (res.status == CORE_RPC_STATUS_OK || res.status == CORE_RPC_STATUS_PAYMENT_REQUIRED)
      m_rpc_payment_state.credits = res.credits;
    if (res.top_hash != m_rpc_payment_state.top_hash)
    {
      m_rpc_payment_state.top_hash = res.top_hash;
      m_rpc_payment_state.stale = true;
    }
  }
  template<typename T> void handle_payment_changes(const T &res, std::false_type) {}

private:
  boost::optional<std::string> get_info();

  /**
   * @brief Invoke /get_transactions _once_ for one RPC chunk, chunk size is implementation detail
   *
   * Used internally by get_transaction[s]()
   *
   * @param txids contains request txids
   * @param cb tx_handler_t which processes received transactions
   *
   * @return boost::none on success, otherwise returns error message
   */
  boost::optional<std::string> get_transactions_one_chunk(tx_cont_t<std::string>&& txids, tx_handler_t& cb);

  epee::net_utils::http::abstract_http_client &m_http_client;
  rpc_payment_state_t &m_rpc_payment_state;
  boost::recursive_mutex &m_daemon_rpc_mutex;
  crypto::secret_key m_client_id_secret_key;
  bool m_offline;

  uint64_t m_height;
  uint64_t m_earliest_height[256];
  uint64_t m_dynamic_base_fee_estimate;
  uint64_t m_dynamic_base_fee_estimate_cached_height;
  uint64_t m_dynamic_base_fee_estimate_grace_blocks;
  std::vector<uint64_t> m_dynamic_base_fee_estimate_vector;
  uint64_t m_fee_quantization_mask;
  uint64_t m_adjusted_time;
  uint32_t m_rpc_version;
  uint64_t m_target_height;
  uint64_t m_block_weight_limit;
  time_t m_get_info_time;
  time_t m_rpc_payment_info_time;
  uint64_t m_rpc_payment_diff;
  uint64_t m_rpc_payment_credits_per_hash_found;
  cryptonote::blobdata m_rpc_payment_blob;
  uint64_t m_rpc_payment_height;
  uint64_t m_rpc_payment_seed_height;
  crypto::hash m_rpc_payment_seed_hash;
  crypto::hash m_rpc_payment_next_seed_hash;
  uint32_t m_rpc_payment_cookie;
  time_t m_height_time;
  time_t m_target_height_time;
  std::vector<std::pair<uint8_t, uint64_t>> m_daemon_hard_forks;
};

}
