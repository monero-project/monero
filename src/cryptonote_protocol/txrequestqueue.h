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

#ifndef CRYPTONOTE_PROTOCOL_TXREQUESTQUEUE_H
#define CRYPTONOTE_PROTOCOL_TXREQUESTQUEUE_H

#include <boost/functional/hash.hpp>
#include <boost/multi_index_container_fwd.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

#include "misc_log_ex.h"
#include "string_tools.h"
#include "syncobj.h"
#include "common/util.h"


struct tx_request {
  boost::uuids::uuid peer_id;
  crypto::hash tx_hash;
  std::chrono::steady_clock::time_point firstseen_timestamp;
  mutable bool in_flight = false;
  mutable bool marked_for_removal = false;

  tx_request(const boost::uuids::uuid& peer_id, const crypto::hash& tx_hash, std::chrono::steady_clock::time_point firstseen_timestamp)
    : peer_id(peer_id), tx_hash(tx_hash), firstseen_timestamp(firstseen_timestamp) {}

  void fly() const { in_flight = true; }
  bool is_in_flight() const { return in_flight; }
  void mark_for_removal() const { in_flight = false; marked_for_removal = true; }
  bool is_marked_for_removal() const { return marked_for_removal; }
};

using boost::multi_index::hashed_non_unique;
using boost::multi_index::hashed_unique;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::multi_index_container;
using boost::multi_index::composite_key;

typedef multi_index_container<
    tx_request,
    indexed_by<
        // Index 0: by peer_id - all requests for a peer
        hashed_non_unique<member<tx_request, boost::uuids::uuid, &tx_request::peer_id>>,
        // Index 1: by tx_hash - all requests for a tx
        hashed_non_unique<member<tx_request, crypto::hash, &tx_request::tx_hash>>,
        // Index 2: by (peer_id, tx_hash) - unique requests
        hashed_unique<composite_key<tx_request,
            member<tx_request, boost::uuids::uuid, &tx_request::peer_id>,
            member<tx_request, crypto::hash, &tx_request::tx_hash>
        >>
    >
> request_container;

template<typename container_t>
decltype(auto) get_requests_by_peer_id(container_t&& container) {
    return std::forward<container_t>(container).template get<0>();
}

template<typename container_t>
decltype(auto) get_requests_by_tx_hash(container_t&& container) {
    return std::forward<container_t>(container).template get<1>();
}

template<typename container_t>
decltype(auto) get_requests_by_peer_and_tx(container_t&& container) {
    return std::forward<container_t>(container).template get<2>();
}

#endif // CRYPTONOTE_PROTOCOL_TXREQUESTQUEUE_H
