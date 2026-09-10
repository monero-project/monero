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
#include <boost/uuid/uuid_io.hpp>
#include <cstddef>
#include <cstdint>

/*
  Using boost multi-index's modify() to update elems ensures all indexes
  are updated in addition to the underlying element, without doing any copies.
  modify() fails when there is a collision; there should be no collisions in
  this file's code given how we expect to update elems.

  Warning: if this calls fails, the passed-in iterator can be rendered invalid
  (and the elem erased from the container).

  Source: https://www.boost.org/latest/libs/multi_index/doc/tutorial/basics.html#ord_updating
*/
template<typename T, typename U>
static bool fly_tx_req(T &it, U &container, const uint64_t nonce) noexcept {
  return container.modify(it, [nonce](tx_request &tx_req) {
    tx_req.fly(nonce);
  });
}

template<typename T, typename U>
static bool start_processing_tx_req(T &it, U &container) noexcept {
  return container.modify(it, [](tx_request &tx_req) {
    tx_req.start_processing();
  });
}

uint64_t request_manager::bump_nonce(const boost::uuids::uuid &peer_id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if (++m_connection_stats[peer_id].cur_nonce == 0)
    ++m_connection_stats[peer_id].cur_nonce;
  return m_connection_stats[peer_id].cur_nonce;
}

void request_manager::remove_peer(const boost::uuids::uuid &peer_id) {
  MINFO("Removing all requests for disconnected peer: " << peer_id);
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto& by_peer = get_requests_by_peer_id(m_requested_txs);
  auto peer_range = by_peer.equal_range(peer_id);
  if (peer_range.first != peer_range.second) {
    size_t removed = std::distance(peer_range.first, peer_range.second);
    by_peer.erase(peer_range.first, peer_range.second);
    MINFO("Removed " << removed << " requests for peer " << peer_id);
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
    if (it->processing || !it->in_flight || elapsed.count() < m_request_timeout) {
      ++it;
      continue;
    }

    MINFO("Removing stale request for tx " << it->tx_hash 
          << " from peer " << it->peer_id
          << ", age: " << elapsed.count() << "ms");

    const auto &peer_id = it->peer_id;

    if (m_connection_stats[peer_id].in_flight_requests > 0)
      --m_connection_stats[peer_id].in_flight_requests;

    // If this peer has missed too many requests, we want to drop it
    if (this->missed_request(peer_id, 1)) {
      drop_peers_out.insert(peer_id);
    }

    it = m_requested_txs.erase(it);
    ++removed;
  }

  MINFO("Cleaned up " << removed << " stale requests");
  return drop_peers_out;
}

request_manager::tx_request_t request_manager::enqueue_requests(const std::vector<crypto::hash> &tx_hashes, const boost::uuids::uuid &peer_id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  tx_request_t tx_req(*this, peer_id);
  for (const auto &tx_hash : tx_hashes)
  {
    MINFO("Requesting from peer: " << peer_id << " the transaction: " << tx_hash);

    auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
    auto it = by_peer_and_tx.find(boost::make_tuple(peer_id, tx_hash));

    const bool in_queue = it != by_peer_and_tx.end();
    const bool let_it_fly = m_connection_stats[peer_id].in_flight_requests < m_max_in_flight
        && !this->request_is_in_flight(tx_hash);

    if (!let_it_fly) {
      if (!in_queue) {
        // Add the new request! Nonce doesn't get set until it's in flight.
        m_requested_txs.insert(tx_request(peer_id, tx_hash, 0/*nonce*/, let_it_fly));
      } else {
        // already have this peer for this tx, but we can't process additional reqs at this time
        MDEBUG("Peer " << peer_id
              << " already in request queue for tx " << tx_hash);
      }
      continue;
    }

    // let_it_fly is true
    assert(let_it_fly);
    if (!in_queue) {
      m_requested_txs.insert(tx_request(peer_id, tx_hash, tx_req.nonce, let_it_fly));
    } else {
      const bool r = fly_tx_req(it, by_peer_and_tx, tx_req.nonce);
      CHECK_AND_ASSERT_MES(r, tx_req, "Failed to fly tx request for tx " << tx_hash);
    }

    ++m_connection_stats[peer_id].n_total_requests;
    ++m_connection_stats[peer_id].in_flight_requests;

    tx_req.tx_hashes.push_back(tx_hash);
  }

  return tx_req;
}

request_manager::tx_request_t request_manager::fly_available_requests(const boost::uuids::uuid &peer_id) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);

  tx_request_t tx_req(*this, peer_id);
  auto& by_peer = get_requests_by_peer_id(m_requested_txs);
  auto range = by_peer.equal_range(peer_id);
  for (auto it = range.first; it != range.second; ++it)
  {
    if (m_connection_stats[peer_id].in_flight_requests >= m_max_in_flight)
      break;
    if (it->in_flight || this->request_is_in_flight(it->tx_hash))
      continue;

    const bool r = fly_tx_req(it, by_peer, tx_req.nonce);
    CHECK_AND_ASSERT_MES(r, tx_req, "Failed to fly available tx request");

    ++m_connection_stats[peer_id].n_total_requests;
    ++m_connection_stats[peer_id].in_flight_requests;

    tx_req.tx_hashes.push_back(it->tx_hash);
  }
  return tx_req;
}

template<typename T, typename U>
T request_manager::erase_tx_request(T &it, U &container) {
  CHECK_AND_ASSERT_MES(it != container.end(), container.end(), "Expected found tx request");
  const boost::uuids::uuid &peer_id = it->peer_id;
  if (it->in_flight && m_connection_stats[peer_id].in_flight_requests > 0)
  {
    --m_connection_stats[peer_id].in_flight_requests;
    MINFO("Decremented in_flight_requests count for peer: " << peer_id << ", current in_flight_requests: " << m_connection_stats[peer_id].in_flight_requests);
  }
  return container.erase(it);
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
    MDEBUG("Removing tx request " << it->tx_hash << " for peer " << it->peer_id);
    it = erase_tx_request(it, by_tx_hash);
  }
  return true;
}

bool request_manager::processing_txs(const boost::uuids::uuid &peer_id, const uint64_t nonce, const std::vector<crypto::hash> &tx_hashes) {
  // Any txs we requested are expected to have nonce >0
  if (nonce == 0)
    return false;
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  // If no reqs found for that peer id and nonce, then we didn't request these hashes
  auto& by_peer_and_nonce = get_requests_by_peer_and_nonce(m_requested_txs);
  auto range = by_peer_and_nonce.equal_range(boost::make_tuple(peer_id, nonce));
  if (range.first == range.second)
    return false;

  // Peer shouldn't have included *more* txs in the response than can be requested at once.
  CHECK_AND_ASSERT_MES(tx_hashes.size() <= m_max_in_flight, false, "Too many tx_hashes in response");
  const std::unordered_set<crypto::hash> peer_hashes(tx_hashes.begin(), tx_hashes.end());

  MINFO("Processing " << tx_hashes.size() << " txs received from peer " << peer_id
      << " (nonce=" << nonce << ")");

  bool req_removed = false;
  for (auto it = range.first; it != range.second;) {
    const auto peer_hash_it = peer_hashes.find(it->tx_hash);
    if (peer_hash_it == peer_hashes.end()) {
      // If we requested a tx at this nonce, but it wasn't included in the peer's resp, then the peer is saying they
      // didn't have the tx in their pool. We can remove it from our request queue for that specific peer.
      MDEBUG("Removing tx request " << it->tx_hash << " only for peer " << it->peer_id);
      it = erase_tx_request(it, by_peer_and_nonce);
      req_removed = true;
      continue;
    }

    // We requested the tx from the peer, the peer responded with it, and now we're going to process it.
    const bool r = start_processing_tx_req(it, by_peer_and_nonce);
    CHECK_AND_ASSERT_MES(r, req_removed, "Failed to mark tx " << *peer_hash_it << " as processing");
    ++it;
  }

  return req_removed;
}

bool request_manager::missed_request(const boost::uuids::uuid &peer_id, const std::size_t n_missed_reqs) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  if ((m_connection_stats[peer_id].missed + n_missed_reqs) > m_connection_stats[peer_id].missed)
    m_connection_stats[peer_id].missed += n_missed_reqs;
  const size_t n_total_reqs = m_connection_stats[peer_id].n_total_requests;

  // Return false if we don't have enough samples
  if (n_total_reqs < P2P_MIN_SAMPLE_SIZE_FOR_DROPPING) return false;

  const size_t percent = (m_connection_stats[peer_id].missed * 100) / n_total_reqs;
  MINFO("Peer " << peer_id
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
