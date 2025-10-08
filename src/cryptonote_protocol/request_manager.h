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
#include <shared_mutex>

class request_manager {

private:
  request_container m_requested_txs;
  mutable std::shared_timed_mutex m_mutex;

  request_manager(const request_manager &) = delete;
  request_manager &operator=(const request_manager &) = delete;
  request_manager(request_manager &&) noexcept = delete;
  request_manager &operator=(request_manager &&) = delete;

public:
  request_manager();

  bool remove_transaction(const crypto::hash &tx_hash);

  bool remove_requested_transaction(const boost::uuids::uuid &peer_id, const crypto::hash &tx_hash);

  void remove_peer_requests(const boost::uuids::uuid &peer_id);

  void cleanup_stale_requests(std::time_t max_age_seconds);

  bool already_requested_tx(const crypto::hash &tx_hash) const;

  bool add_transaction(const crypto::hash &tx_hash,
                       const boost::uuids::uuid &id, std::time_t first_seen);

  // Get the peer ID of the current in-flight request for a transaction, if any
  // true: found, false: not found or none in-flight
  bool get_current_request_peer_id(const crypto::hash &tx_hash, boost::uuids::uuid &out) const;

  // Get next peer ID to request a transaction from, and mark it in-flight,
  // Remove the the current in-flight requests
  boost::uuids::uuid request_from_next_peer(const crypto::hash &tx_hash, std::time_t now);

  void for_each_request(std::function<void(request_manager&, const request &, const std::time_t)> &f,
                        const std::time_t request_deadline);
};

#endif // CRYPTONOTE_PROTOCOL_REQUEST_MANAGER_H
