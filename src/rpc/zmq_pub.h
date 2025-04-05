// Copyright (c) 2020-2024, The Monero Project

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

#include <array>
#include <boost/thread/mutex.hpp>
#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

#include "crypto/crypto.h"
#include "cryptonote_basic/fwd.h"
#include "net/zmq.h"
#include "span.h"
#include "cryptonote_basic/difficulty.h"

namespace cryptonote { namespace listener
{
/*! \brief Sends ZMQ PUB messages on cryptonote events

    Clients must ensure that all transaction(s) are notified before any blocks
    they are contained in, and must ensure that each block is notified in chain
    order. An external lock **must** be held by clients during the entire
    txpool check and notification sequence and (a possibly second) lock is held
    during the entire block check and notification sequence. Otherwise, events
    could be sent in a different order than processed. */
class zmq_pub
{
  /* Each socket has its own internal queue. So we can only use one socket, else
     the messages being published are not guaranteed to be in the same order
     pushed. */

    net::zmq::socket relay_;
    std::deque<std::vector<txpool_event>> txes_;
    std::array<std::size_t, 2> chain_subs_;
    std::array<std::size_t, 1> miner_subs_;
    std::array<std::size_t, 2> txpool_subs_;
    boost::mutex sync_; //!< Synchronizes counts in `*_subs_` arrays.

  public:
    //! \return Name of ZMQ_PAIR endpoint for pub notifications
    static constexpr const char* relay_endpoint() noexcept { return "inproc://pub_relay"; }

    explicit zmq_pub(void* context);

    zmq_pub(const zmq_pub&) = delete;
    zmq_pub(zmq_pub&&) = delete;

    ~zmq_pub();

    zmq_pub& operator=(const zmq_pub&) = delete;
    zmq_pub& operator=(zmq_pub&&) = delete;

    //! Process a client subscription request (from XPUB sockets). Thread-safe.
    bool sub_request(const boost::string_ref message);

    /*! Forward ZMQ messages sent to `relay` via `send_chain_main` or
      `send_txpool_add` to `pub`. Used by `ZmqServer`. */
    bool relay_to_pub(void* relay, void* pub);

    /*! Send a `ZMQ_PUB` notification for a change to the main chain.
        Thread-safe.
        \return Number of ZMQ messages sent to relay. */
    std::size_t send_chain_main(std::uint64_t height, epee::span<const cryptonote::block> blocks);

    /*! Send a `ZMQ_PUB` notification for a new miner data.
        Thread-safe.
        \return Number of ZMQ messages sent to relay. */
    std::size_t send_miner_data(uint8_t major_version, uint64_t height, const crypto::hash& prev_id, const crypto::ec_point& fcmp_pp_tree_root, const crypto::hash& seed_hash, difficulty_type diff, uint64_t median_weight, uint64_t already_generated_coins, const std::vector<tx_block_template_backlog_entry>& tx_backlog);

    /*! Send a `ZMQ_PUB` notification for new tx(es) being added to the local
        pool. Thread-safe.
        \return Number of ZMQ messages sent to relay. */
    std::size_t send_txpool_add(std::vector<cryptonote::txpool_event> txes);

    //! Callable for `send_chain_main` with weak ownership to `zmq_pub` object.
    struct chain_main
    {
      std::weak_ptr<zmq_pub> self_;
      void operator()(std::uint64_t height, epee::span<const cryptonote::block> blocks) const;
    };

    //! Callable for `send_miner_data` with weak ownership to `zmq_pub` object.
    struct miner_data
    {
      std::weak_ptr<zmq_pub> self_;
      void operator()(uint8_t major_version, uint64_t height, const crypto::hash& prev_id, const crypto::ec_point& fcmp_pp_tree_root, const crypto::hash& seed_hash, difficulty_type diff, uint64_t median_weight, uint64_t already_generated_coins, const std::vector<tx_block_template_backlog_entry>& tx_backlog) const;
    };

    //! Callable for `send_txpool_add` with weak ownership to `zmq_pub` object.
    struct txpool_add
    {
      std::weak_ptr<zmq_pub> self_;
      void operator()(std::vector<cryptonote::txpool_event> txes) const;
    };
  };
}}
