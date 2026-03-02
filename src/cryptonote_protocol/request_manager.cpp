// Copyright (c) 2014-2025, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list
//    of conditions and the following disclaimer in the documentation and/or
//    other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
// may be
//    used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.


#include "cryptonote_protocol/request_manager.h"
#include "cryptonote_protocol/txrequestqueue.h"
#include "misc_log_ex.h"
#include <cstddef>
#include <cstdint>

void request_manager::remove_peer(const boost::uuids::uuid &peer_id) {
  MINFO("Removing all requests for disconnected peer: " << epee::string_tools::pod_to_hex(peer_id));
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto& by_peer = get_requests_by_peer_id(m_requested_txs);
  auto peer_range = by_peer.equal_range(peer_id);
  if (peer_range.first != peer_range.second) {
    size_t removed = std::distance(peer_range.first, peer_range.second);
    by_peer.erase(peer_range.first, peer_range.second);
    MINFO("Removed " << removed << " requests for peer " << epee::string_tools::pod_to_hex(peer_id));
  }
  m_connection_stats.erase(peer_id);
}

std::unordered_set<boost::uuids::uuid> request_manager::remove_stale_requests() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::unordered_set<boost::uuids::uuid> drop_peers_out;
  size_t removed = 0;
  auto now = std::chrono::steady_clock::now();
  for (auto it = m_requested_txs.begin(); it != m_requested_txs.end();) {
    // Stale means it's been in flight for longer than the allowed timeout
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->last_action_timestamp);
    if (!it->in_flight || elapsed.count() < m_request_timeout) {
      ++it;
      continue;
    }

    MINFO("Removing stale request for tx " << it->tx_hash 
          << " from peer " << epee::string_tools::pod_to_hex(it->peer_id)
          << ", age: " << elapsed.count() << "ms");

    // If this peer has missed too many requests, we want to drop it
    if (this->missed_request(it->peer_id, 1)) {
      drop_peers_out.insert(it->peer_id);
    }

    it = m_requested_txs.erase(it);
    ++removed;
  }

  MINFO("Cleaned up " << removed << " stale requests");
  return drop_peers_out;
}

bool request_manager::add_request(const crypto::hash &tx_hash, const boost::uuids::uuid &peer_id) {
  MINFO("Requesting from peer: " << epee::string_tools::pod_to_hex(peer_id) << " the transaction: " << tx_hash);
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
  auto it = by_peer_and_tx.find(boost::make_tuple(peer_id, tx_hash));

  const bool in_queue = it != by_peer_and_tx.end();
  const bool let_it_fly = m_connection_stats[peer_id].in_flight_requests < m_max_in_flight
      && !this->request_is_in_flight(tx_hash);

  if (in_queue && !let_it_fly) {
    // already have this peer for this tx, we can't process additional reqs at this time
    MDEBUG("Peer " << epee::string_tools::pod_to_hex(peer_id)
           << " already in request queue for tx " << tx_hash);
    return false;
  }

  if (!in_queue) {
    // Add the new request!
    m_requested_txs.insert(tx_request(peer_id, tx_hash, let_it_fly));
  } else if (let_it_fly) {
    // let_it_fly should always be true here
    it->fly();
  }

  if (let_it_fly) {
    ++m_connection_stats[peer_id].n_total_requests;
    ++m_connection_stats[peer_id].in_flight_requests;
  }

  return let_it_fly;
}

std::vector<crypto::hash> request_manager::fly_available_requests(const boost::uuids::uuid &peer_id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  std::vector<crypto::hash> tx_hashes;
  auto& by_peer = get_requests_by_peer_id(m_requested_txs);
  auto range = by_peer.equal_range(peer_id);
  for (auto it = range.first; it != range.second; ++it)
  {
    if (m_connection_stats[peer_id].in_flight_requests >= m_max_in_flight)
      break;
    if (it->in_flight || this->request_is_in_flight(it->tx_hash))
      continue;

    it->fly();

    ++m_connection_stats[peer_id].n_total_requests;
    ++m_connection_stats[peer_id].in_flight_requests;

    tx_hashes.push_back(it->tx_hash);
  }
  return tx_hashes;
}

bool request_manager::remove_request(const crypto::hash &tx_hash) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  auto range = by_tx_hash.equal_range(tx_hash);
  if (range.first == range.second) {
    MDEBUG("No requests for tx " << tx_hash);
    return false;
  }
  for (auto it = range.first; it != range.second;) {
    const boost::uuids::uuid &peer_id = it->peer_id;
    MDEBUG("Removing tx request " << it->tx_hash << " for peer " << epee::string_tools::pod_to_hex(peer_id));
    if (it->in_flight && m_connection_stats[peer_id].in_flight_requests > 0)
    {
      --m_connection_stats[peer_id].in_flight_requests;
      MINFO("Decremented in_flight_requests count for peer: " << epee::string_tools::pod_to_hex(peer_id) << ", current in_flight_requests: " << m_connection_stats[peer_id].in_flight_requests);
    }
    it = by_tx_hash.erase(it);
  }
  return true;
}

bool request_manager::missed_request(const boost::uuids::uuid &peer_id, const std::size_t n_missed_reqs) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if ((m_connection_stats[peer_id].missed + n_missed_reqs) > m_connection_stats[peer_id].missed)
    m_connection_stats[peer_id].missed += n_missed_reqs;
  const size_t n_total_reqs = m_connection_stats[peer_id].n_total_requests;

  // Return false if we don't have enough samples
  if (n_total_reqs < P2P_MIN_SAMPLE_SIZE_FOR_DROPPING) return false;

  const size_t percent = (m_connection_stats[peer_id].missed * 100) / n_total_reqs;
  MINFO("Peer " << epee::string_tools::pod_to_hex(peer_id)
        << " has missed " << m_connection_stats[peer_id].missed << " out of "
        << n_total_reqs << " total requests (" << percent << "%)");
  return percent > P2P_REQUEST_FAILURE_THRESHOLD_PERCENTAGE;
}

bool request_manager::request_is_in_flight(const crypto::hash &tx_hash) const {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  auto range = by_tx_hash.equal_range(tx_hash);
  for (auto it = range.first; it != range.second; ++it)
    if (it->in_flight)
      return true;
  return false;
}
