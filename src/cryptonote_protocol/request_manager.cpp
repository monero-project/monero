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

request_manager::request_manager() : m_requested_txs(), m_mutex() {}

bool request_manager::remove_announcement(const crypto::hash &tx_hash) {
  MINFO("Removing transaction: " << epee::string_tools::pod_to_hex(tx_hash));
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& requests_by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  auto range = requests_by_tx_hash.equal_range(tx_hash);
  if (range.first != range.second) {
    size_t removed = std::distance(range.first, range.second);
    requests_by_tx_hash.erase(range.first, range.second);
    MINFO("Removed " << removed << " requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
    return true;
  }
  return false;
}

bool request_manager::received_requested(const boost::uuids::uuid &peer_id, const crypto::hash &tx_hash) {
  MINFO("Removing transaction: " << epee::string_tools::pod_to_hex(tx_hash)
        << " for peer: " << epee::string_tools::pod_to_hex(peer_id));
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& requests_by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
  auto it = requests_by_peer_and_tx.find(boost::make_tuple(peer_id, tx_hash));
  if (it != requests_by_peer_and_tx.end()) {
    requests_by_peer_and_tx.erase(it);
    MINFO("Removed request for tx " << epee::string_tools::pod_to_hex(tx_hash)
          << " from peer " << epee::string_tools::pod_to_hex(peer_id));
    return true;
  }
  return false;
}

void request_manager::remove_peer(const boost::uuids::uuid &peer_id) {
  MINFO("Removing all requests for disconnected peer: " << epee::string_tools::pod_to_hex(peer_id));
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_peer = get_requests_by_peer_id(m_requested_txs);
  auto peer_range = by_peer.equal_range(peer_id);
  if (peer_range.first != peer_range.second) {
    size_t removed = std::distance(peer_range.first, peer_range.second);
    by_peer.erase(peer_range.first, peer_range.second);
    MINFO("Removed " << removed << " requests for peer " << epee::string_tools::pod_to_hex(peer_id));
  }
}

void request_manager::cleanup_stale_requests(uint32_t max_age_seconds) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  size_t removed = 0;

  auto now = std::chrono::steady_clock::now();
  auto it = m_requested_txs.begin();
  while (it != m_requested_txs.end()) {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->firstseen_timestamp);
    if (elapsed.count() > max_age_seconds) {
      MINFO("Removing stale request for tx " << epee::string_tools::pod_to_hex(it->tx_hash) 
            << " from peer " << epee::string_tools::pod_to_hex(it->peer_id)
            << ", age: " << elapsed.count() << " seconds");
      it = m_requested_txs.erase(it);
      ++removed;
    } else {
      ++it;
    }
  }

  MINFO("Cleaned up " << removed << " stale requests");
  return;
}

bool request_manager::already_requested_tx(const crypto::hash &tx_hash) const {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  for (const auto& req : boost::make_iterator_range(by_tx_hash.equal_range(tx_hash))) {
    MINFO("Found request for tx " << epee::string_tools::pod_to_hex(req.tx_hash) << " from peer " << epee::string_tools::pod_to_hex(req.peer_id) << " request is" << (req.is_in_flight() ? " " : " not ")  << "in flight.");
    if (req.is_in_flight()) {
      return true;
    }
  }
  MINFO("No in-flight request found for tx " << epee::string_tools::pod_to_hex(tx_hash));
  return false;
}

bool request_manager::already_requested_tx(const crypto::hash &tx_hash, const boost::uuids::uuid &id) const {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
  for (const auto& req : boost::make_iterator_range(by_peer_and_tx.equal_range(boost::make_tuple(id, tx_hash)))) {
    MINFO("Found request for tx " << epee::string_tools::pod_to_hex(req.tx_hash) << " from peer " << epee::string_tools::pod_to_hex(req.peer_id) << " request is" << (req.is_in_flight() ? " " : " not ")  << "in flight.");
    if (req.is_in_flight()) {
      return true;
    }
  }
  MINFO("No in-flight request found for tx " << epee::string_tools::pod_to_hex(tx_hash));
  return false;
}

bool request_manager::add_announcement(const crypto::hash &tx_hash,
                                      const boost::uuids::uuid &id) {
  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  MINFO("Adding peer: " << epee::string_tools::pod_to_hex(id)
        << " to transaction: "
        << epee::string_tools::pod_to_hex(tx_hash));
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
  auto it = by_peer_and_tx.find(boost::make_tuple(id, tx_hash));
  if (it != by_peer_and_tx.end()) {
    // already have this peer for this tx
    MDEBUG("Peer " << epee::string_tools::pod_to_hex(id)
           << " already in request queue for tx "
           << epee::string_tools::pod_to_hex(tx_hash));
    return false;

  }
  m_requested_txs.insert(tx_request(id, tx_hash, now));
  return true;
}

void request_manager::add_request(const crypto::hash &tx_hash, const boost::uuids::uuid &peer_id) {
  MINFO("Requesting from peer: " << epee::string_tools::pod_to_hex(peer_id)
        << " the transaction: "
        << epee::string_tools::pod_to_hex(tx_hash));
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
  auto it = by_peer_and_tx.find(boost::make_tuple(peer_id, tx_hash));
  if (it != by_peer_and_tx.end()) {
    it->fly();
    return;
  }
  MINFO("Requested tx " << tx_hash << " that does not exist in request_manager queue.");
}

bool request_manager::get_current_request_peer_id(const crypto::hash &tx_hash, boost::uuids::uuid &out) const {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  auto it = by_tx_hash.find(tx_hash);
  if (it == by_tx_hash.end()) {
    MDEBUG("No requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
    return false;
  }
  for (const auto& req : boost::make_iterator_range(by_tx_hash.equal_range(tx_hash))) {
    if (req.is_in_flight()) {
      out = req.peer_id;
      return true;
    }
  }
  return false;
}

boost::uuids::uuid request_manager::request_from_next_peer(const crypto::hash &tx_hash, std::chrono::steady_clock::time_point now) {
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
  auto it = by_tx_hash.find(tx_hash);
  if (it == by_tx_hash.end()) {
    MDEBUG("No requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
    return boost::uuids::nil_uuid();
  }

  // remove in flight request that have timed out
  auto range = by_tx_hash.equal_range(tx_hash);
  it = range.first;
  while (it != range.second) {
    if (it->is_in_flight()) {
      MINFO("Erasing " << epee::string_tools::pod_to_hex(tx_hash)
            << " from peer "
            << epee::string_tools::pod_to_hex(it->peer_id));
      it->mark_for_removal();
    } else {
      ++it;
    }
  }

  // find the earliest non in-flight request
  auto earliest_request = by_tx_hash.end();
  it = by_tx_hash.find(tx_hash);
  while (it != by_tx_hash.end() && it->tx_hash == tx_hash) {
    if (!it->is_in_flight() && !it->is_marked_for_removal()) {
      if (earliest_request == by_tx_hash.end() ||
          it->firstseen_timestamp < earliest_request->firstseen_timestamp) {
        earliest_request = it;
      }
    }
    ++it;
  }

  if (earliest_request != by_tx_hash.end()) {
    earliest_request->fly();
    MINFO("Requesting tx " << epee::string_tools::pod_to_hex(tx_hash)
          << " from peer "
          << epee::string_tools::pod_to_hex(earliest_request->peer_id)
          << ", first seen " << earliest_request->firstseen_timestamp.time_since_epoch().count()
          << ", now " << now.time_since_epoch().count());
    return earliest_request->peer_id;
  }

  MINFO("No available requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
  return boost::uuids::nil_uuid();
}

void request_manager::for_each_request(std::function<void(const tx_request &, const uint32_t)> &f, const uint32_t request_deadline) {
  MINFO("Iterating over " << m_requested_txs.size() << " requests.");
  std::unique_lock<std::recursive_mutex> lock(m_mutex);
  std::vector<tx_request> requests_copy;
  requests_copy.reserve(m_requested_txs.size());
  // First copy all requests
  for (const auto& req : m_requested_txs) {
      requests_copy.push_back(req);
  }
  // Process requests on the copy
  for (const auto& request : requests_copy) {
      f(request, request_deadline);
  }
  // Remove marked items directly from main container
  size_t removed_count = 0;
  auto it = m_requested_txs.begin();
  while (it != m_requested_txs.end()) {
      if (it->is_marked_for_removal()) {
          it = m_requested_txs.erase(it);
          ++removed_count;
      } else {
          ++it;
      }
  }
  MINFO("Removed " << removed_count << " requests.");
}