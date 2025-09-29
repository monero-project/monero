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

#ifndef CRYPTONOTE_PROTOCOL_REQUEST_MANAGER_H
#define CRYPTONOTE_PROTOCOL_REQUEST_MANAGER_H

#include "crypto/hash.h"
#include "string_tools.h"
#include "syncobj.h"
#include "txrequestqueue.h"

#include <boost/functional/hash.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <unordered_map>

class request_manager {

private:
  request_container m_requested_txs;
  mutable epee::rw_mutex m_mutex;

  request_manager(const request_manager &) = delete;
  request_manager &operator=(const request_manager &) = delete;
  request_manager(request_manager &&) noexcept = delete;
  request_manager &operator=(request_manager &&) = delete;

public:
  request_manager() : m_requested_txs(), m_mutex() {}

  bool remove_transaction(const crypto::hash &tx_hash) {
    MINFO("Removing transaction: " << epee::string_tools::pod_to_hex(tx_hash));
    epee::write_lock w_lock(m_mutex);
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

  bool remove_requested_transaction(const boost::uuids::uuid &peer_id, const crypto::hash &tx_hash) {
    MINFO("Removing transaction: " << epee::string_tools::pod_to_hex(tx_hash)
                                    << " for peer: " << epee::string_tools::pod_to_hex(peer_id));
    epee::write_lock w_lock(m_mutex);
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

  void remove_peer_requests(const boost::uuids::uuid &peer_id) {
    MINFO("Removing all requests for disconnected peer: " << epee::string_tools::pod_to_hex(peer_id));
    epee::write_lock w_lock(m_mutex);
    auto& by_peer = get_requests_by_peer_id(m_requested_txs);
    auto peer_range = by_peer.equal_range(peer_id);
    if (peer_range.first != peer_range.second) {
      size_t removed = std::distance(peer_range.first, peer_range.second);
      by_peer.erase(peer_range.first, peer_range.second);
      MINFO("Removed " << removed << " requests for peer " << epee::string_tools::pod_to_hex(peer_id));
    }
  }

  void cleanup_stale_requests(std::time_t max_age_seconds) {
    std::time_t now = std::time(nullptr);
    std::time_t cutoff_time = now - max_age_seconds;

    epee::write_lock w_lock(m_mutex);
    size_t removed = 0;

    auto it = m_requested_txs.begin();
    while (it != m_requested_txs.end()) {
      if (it->firstseen_timestamp < cutoff_time) {
        MINFO("Removing stale request for tx " << epee::string_tools::pod_to_hex(it->tx_hash) 
              << " from peer " << epee::string_tools::pod_to_hex(it->peer_id)
              << ", age: " << (now - it->firstseen_timestamp) << " seconds");
        it = m_requested_txs.erase(it);
        ++removed;
      } else {
        ++it;
      }
    }

    if (removed > 0) {
      MINFO("Cleaned up " << removed << " stale requests");
    }
    return;
  }

  bool already_requested_tx(const crypto::hash &tx_hash) const {
    epee::read_lock r_lock(m_mutex);
    auto& by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
    for (const auto& req : boost::make_iterator_range(by_tx_hash.equal_range(tx_hash))) {
      if (req.is_in_flight()) {
        return true;
      }
    }
    return false;
  }

  bool add_transaction(const crypto::hash &tx_hash,
                       const boost::uuids::uuid &id, std::time_t first_seen) {
    MINFO("Adding peer: " << epee::string_tools::pod_to_hex(id)
                          << " to transaction: "
                          << epee::string_tools::pod_to_hex(tx_hash)
                          << ", first seen: " << first_seen);
    epee::write_lock w_lock(m_mutex);
    auto& by_peer_and_tx = get_requests_by_peer_and_tx(m_requested_txs);
    auto it = by_peer_and_tx.find(boost::make_tuple(id, tx_hash));
    if (it != by_peer_and_tx.end()) {
      // already have this peer for this tx
      MDEBUG("Peer " << epee::string_tools::pod_to_hex(id)
                     << " already in request queue for tx "
                     << epee::string_tools::pod_to_hex(tx_hash));
      return false;

    }
    m_requested_txs.insert(request(id, tx_hash, first_seen));
    return true;
  }

  // Get the peer ID of the current in-flight request for a transaction, if any
  // true: found, false: not found or none in-flight
  bool get_current_request_peer_id(const crypto::hash &tx_hash, boost::uuids::uuid &out) const {
    epee::read_lock r_lock(m_mutex);
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

  // Get next peer ID to request a transaction from, and mark it in-flight,
  // Remove the the current in-flight requests
  boost::uuids::uuid request_from_next_peer(const crypto::hash &tx_hash, std::time_t now) {
    epee::write_lock w_lock(m_mutex);
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
        it = by_tx_hash.erase(it);
      } else {
        ++it;
      }
    }

    // Recompute range after potential erasures.
    by_tx_hash = get_requests_by_tx_hash(m_requested_txs);
    if (by_tx_hash.empty()) {
      MINFO("No available requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
      return boost::uuids::nil_uuid();
    }

    // find the earliest non in-flight request
    auto earliest_request = by_tx_hash.end();
    it = by_tx_hash.find(tx_hash);
    while (it != by_tx_hash.end() && it->tx_hash == tx_hash) {
      if (!it->is_in_flight()) {
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
                             << ", first seen " << earliest_request->firstseen_timestamp
                             << ", now " << now);
      return earliest_request->peer_id;
    }

    MINFO("No available requests for tx " << epee::string_tools::pod_to_hex(tx_hash));
    return boost::uuids::nil_uuid();
  }

  void for_each_request(std::function<void(request_manager&, const request &, const std::time_t)> &f,
                                            const std::time_t request_deadline) {
    MINFO("Iterating over requested transactions");
    for (const auto &request : m_requested_txs) {
      f(*this, request, request_deadline);
    }
  }
};

#endif // CRYPTONOTE_PROTOCOL_REQUEST_MANAGER_H