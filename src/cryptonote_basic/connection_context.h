// Copyright (c) 2014-2024, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers
// Parts of this file are originally copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net

#pragma once
#include <unordered_set>
#include <atomic>
#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional/optional_fwd.hpp>
#include "net/net_utils_base.h"
#include "crypto/hash.h"

namespace cryptonote
{
  struct cryptonote_connection_context: public epee::net_utils::connection_context_base
  {
    cryptonote_connection_context(): m_state(state_before_handshake), m_remote_blockchain_height(0), m_expected_heights_start(0),
        m_last_response_height(0), m_last_known_hash(crypto::null_hash), m_expect_response(0),
        m_last_request_time(boost::date_time::not_a_date_time), m_expect_height(0), m_num_requested(0), m_callback_request_count(0),
        m_pruning_seed(0), m_rpc_port(0), m_rpc_credits_per_hash(0), m_anchor(false), m_score(0) {}

    /**
     * Per-connection protocol state. Order is important here; as the code
     * compares states with `<` and `>=` operators, do not re-order them.
     */
    enum state
    {
      /**
       * Default state; a peer sending `NOTIFY_REQUEST_CHAIN`,
       * `NOTIFY_REQUEST_GET_OBJECTS`, or `NOTIFY_REQUEST_FLUFFY_MISSING_TX`
       * during this state will be dropped.
       */
      state_before_handshake = 0,

      /**
       * State for synchronizing the chain with a peer. Timed-sync data from the
       * peer is ignored while in this state.
       */
      state_synchronizing,

      /**
       * Sync with the peer is paused in this state, due to the block queue
       * being full, the peer having the wrong stripe, or because a different
       * peer holds the next span.
       */
      state_standby,

      /**
       * State where we are in sync with this peer. This is the only
       * state that accepts `NOTIFY_NEW_FLUFFY_BLOCK`, `NOTIFY_NEW_TRANSACTIONS`,
       * and `NOTIFY_GET_TXPOOL_COMPLEMENT` messages.
       */
      state_normal
    };

    /*
      This class was originally from the EPEE module. It is identical in function to std::atomic<uint32_t> except
      that it has copy-construction and copy-assignment defined, which means that earlier devs didn't have to write
      custom copy-constructors and copy-assignment operators for the outer class, cryptonote_connection_context.
      cryptonote_connection_context should probably be refactored because it is both trying to be POD-like while
      also (very loosely) controlling access to its atomic members.
    */
    class copyable_atomic: public std::atomic<uint32_t>
    {
    public:
      copyable_atomic()
      {};
      copyable_atomic(uint32_t value)
      { store(value); }
      copyable_atomic(const copyable_atomic& a):std::atomic<uint32_t>(a.load())
      {}
      copyable_atomic& operator= (const copyable_atomic& a)
      {
        store(a.load());
        return *this;
      }
      uint32_t operator++()
      {
        return std::atomic<uint32_t>::operator++();
      }
      uint32_t operator++(int fake)
      {
        return std::atomic<uint32_t>::operator++(fake);
      }
    };

    static constexpr int handshake_command() noexcept { return 1001; }
    bool handshake_complete() const noexcept { return m_state != state_before_handshake; }

    //! \return Maximum number of bytes permissible for `command`.
    static size_t get_max_bytes(int command) noexcept;

    //! Use this instead of `m_state = state_normal`.
    void set_state_normal();

    boost::optional<crypto::hash> get_expected_hash(uint64_t height) const;

    //! Protocol state of this connection; see `state`.
    state m_state;

    // Peer's chain, as far as we know it

    /**
     * The sync height claimed by the remote peer, updated from sync data, chain
     * entries, and object responses. If it ever decreases, the peer is hit with
     * `hit_score(1)`.
     */
    uint64_t m_remote_blockchain_height;

    /**
     * The hashes from the peer's last chain entry, indexed by height, starting
     * at `m_expected_heights_start`. Every block received in
     * `NOTIFY_RESPONSE_GET_OBJECTS` is checked against these through
     * `get_expected_hash()`, with a mismatch meaning the peer sent an invalid
     * chain, and resulting in a drop.
     */
    std::vector<crypto::hash> m_expected_heights;

    //! Height of `m_expected_heights[0]`.
    uint64_t m_expected_heights_start;

    /**
     * Hash and weight pairs from a chain entry which we don't have and haven't
     * yet queued. `skip_unneeded_hashes` trims them from the front.
     *
     * As this is always the tail ending at `m_last_response_height`, the first
     * needed height = `m_last_response_height` - `m_needed_objects.size()` + 1.
     */
    std::vector<std::pair<crypto::hash, uint64_t>> m_needed_objects;

    /**
     * Height of the last hash in the peer's last chain entry, minus any hashes
     * that were rejected in prevalidation.
     */
    uint64_t m_last_response_height;

    /**
     * The last block hash received from this peer, or the last needed
     * hash that was skipped. The next `NOTIFY_REQUEST_CHAIN` will continue from
     * this, and it's reset to null on a refresh.
     */
    crypto::hash m_last_known_hash;

    // Request in flight

    /**
     * ID of the reply being waited for, or 0 for none. A reply that doesn't fit
     * results in the peer being dropped.
     */
    int m_expect_response;

    /**
     * The timestamp of when this last request was sent. `not_a_date_time` means
     * nothing is in flight, and `on_callback` will send a new
     * `NOTIFY_REQUEST_CHAIN`. The idle kicker reads this as well.
     */
    boost::posix_time::ptime m_last_request_time;

    /**
     * Requested block hashes in the current `NOTIFY_REQUEST_GET_OBJECTS`
     * request. Each hash is removed right after it arrives; any still left
     * when the response is processed get the peer dropped.
     */
    std::unordered_set<crypto::hash> m_requested_objects;

    /**
     * Height used to check the reply against the request in flight, with the
     * meaning depending on the request.
     *
     * For `NOTIFY_REQUEST_CHAIN`, this is our chain height when the request was
     * sent. The reply must start below it, at a block we already have. It's set
     * to the maximum value to disable this check when continuing from
     * `m_last_known_hash`.
     *
     * For `NOTIFY_REQUEST_GET_OBJECTS`, this is the start height of the
     * requested span, and the first returned block must not be above it.
     */
    uint64_t m_expect_height;

    /**
     * Number of blocks requested from this peer since its last chain entry.
     * It's reset to 0 when sync starts and on each new chain entry. If there's
     * nothing left to request, this is 0, and the peer is still ahead of us,
     * that's treated as an error.
     */
    size_t m_num_requested;

    // Callback flags

    /**
     * How many of `request_callback` are currently pending. `on_callback`
     * asserts this is above 0 and decrements it.
     */
    copyable_atomic m_callback_request_count;

    /**
     * Flag set by `kick_idle_peers`. It tells `on_callback` to check whether
     * this peer should be kicked.
     */
    copyable_atomic m_idle_peer_notification{0};

    /**
     * Flag set by `notify_new_stripe`. It moves a `normal` peer back to
     * `synchronizing`.
     */
    copyable_atomic m_new_stripe_notification{0};

    /**
     * Whether the `NOTIFY_REQUEST_CHAIN` in flight is a refresh, meant to let
     * this peer take over the next span, which another peer is stuck on. When
     * the reply arrives, it's checked to decide whether to force that span.
     */
    bool m_refresh_for_next_span = false;

    // Peer info

    uint32_t m_pruning_seed;
    uint16_t m_rpc_port;
    uint32_t m_rpc_credits_per_hash;
    bool m_anchor;

    //! Protocol-layer connection score; starts at 0.
    int32_t m_score;
  };

  inline std::string get_protocol_state_string(cryptonote_connection_context::state s)
  {
    switch (s)
    {
    case cryptonote_connection_context::state_before_handshake:
      return "before_handshake";
    case cryptonote_connection_context::state_synchronizing:
      return "synchronizing";
    case cryptonote_connection_context::state_standby:
      return "standby";
    case cryptonote_connection_context::state_normal:
      return "normal";
    default:
      return "unknown";
    }    
  }

  inline char get_protocol_state_char(cryptonote_connection_context::state s)
  {
    switch (s)
    {
    case cryptonote_connection_context::state_before_handshake:
      return 'h';
    case cryptonote_connection_context::state_synchronizing:
      return 's';
    case cryptonote_connection_context::state_standby:
      return 'w';
    case cryptonote_connection_context::state_normal:
      return 'n';
    default:
      return 'u';
    }
  }

}
