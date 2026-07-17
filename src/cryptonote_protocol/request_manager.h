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
#include "txrequestqueue.h"

#include <atomic>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

class request_manager {

private:
  request_container m_requested_txs;
  mutable std::recursive_mutex m_mutex;

  const std::size_t m_max_in_flight;
  const int64_t m_request_timeout;

  request_manager(const request_manager &) = delete;
  request_manager &operator=(const request_manager &) = delete;
  request_manager(request_manager &&) noexcept = delete;
  request_manager &operator=(request_manager &&) = delete;

private:
  struct connection_statistics
  {
    std::atomic<uint64_t> cur_nonce = 0;
    std::atomic<size_t> n_total_requests = 0;
    std::atomic<size_t> missed = 0;
    std::atomic<size_t> in_flight_requests = 0;
  };
  std::unordered_map<boost::uuids::uuid, connection_statistics> m_connection_stats;

public:
  struct tx_request_t
  {
    const uint64_t nonce;
    std::vector<crypto::hash> tx_hashes;

    tx_request_t(request_manager& rm, const boost::uuids::uuid &peer_id)
      : nonce(rm.bump_nonce(peer_id))
      {};
  };

  request_manager(const std::size_t max_in_flight,
      const int64_t request_timeout = P2P_DEFAULT_REQUEST_TIMEOUT)
    : m_requested_txs(),
      m_mutex(),
      m_connection_stats(),
      m_max_in_flight(max_in_flight),
      m_request_timeout(request_timeout)
    {};

  void remove_peer(const boost::uuids::uuid &peer_id);

  // Returns the set of peers to drop because they've missed too many requests
  std::unordered_set<boost::uuids::uuid> remove_stale_requests();

  // Return the tx_request_t object that should be immediately sent over the wire
  // The function internally adds all passed tx_hashes to the request manager queue for the
  // given peer, but may only need to request some of them.
  tx_request_t enqueue_requests(const std::vector<crypto::hash> &tx_hashes, const boost::uuids::uuid &id);

  // Remove current in-flight request for a transaction, if present
  // true: found, false: not found or none in-flight
  bool remove_request(const crypto::hash &tx_hash);

  // Processing received txs from peer. This function does 2 things:
  // 1) Marks requested txs as processing.
  // 2) If some tx hashes are missing that were requested at the given peer and nonce, removes those reqs from the queue.
  // true: if any missing txs were removed from the queue, false: no missing txs removed from the queue
  bool processing_txs(const boost::uuids::uuid &peer_id, const uint64_t nonce, const std::vector<crypto::hash> &tx_hashes);

  // Return the tx_request_t object that should be immediately sent over the wire
  tx_request_t fly_available_requests(const boost::uuids::uuid &peer_id);

private:
  // Bump the tx request nonce for that peer
  uint64_t bump_nonce(const boost::uuids::uuid &peer_id);

  // Removes a tx request that has already been found in the container
  // Returns the iterator pointing to the next elem in the container
  template<typename T, typename U>
  T erase_tx_request(T &it, U &container);

  // Return true if we should drop the peer because it exceeded threshold for allowed missed reqs
  bool missed_request(const boost::uuids::uuid &peer_id, const std::size_t n_missed_reqs = 1);

  // Return true if *any* peer has the provided tx hash request in flight
  bool request_is_in_flight(const crypto::hash &tx_hash) const;
};

#endif // CRYPTONOTE_PROTOCOL_REQUEST_MANAGER_H
