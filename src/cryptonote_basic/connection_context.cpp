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

#include "connection_context.h"

#include <boost/optional/optional.hpp>
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "p2p/p2p_protocol_defs.h"
#include "cryptonote_config.h"          // for P2P_REQUEST_FAILURE_THRESHOLD_PERCENTAGE
#include "string_tools.h"               // for pod_to_hex
#include <sstream>

namespace cryptonote
{
  std::size_t cryptonote_connection_context::get_max_bytes(const int command) noexcept
  {
    switch (command)
    {
    case nodetool::COMMAND_HANDSHAKE_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_TIMED_SYNC_T<cryptonote::CORE_SYNC_DATA>::ID:
      return 65536;
    case nodetool::COMMAND_PING::ID:
      return 4096;
    case nodetool::COMMAND_REQUEST_SUPPORT_FLAGS::ID:
      return 4096;
    case cryptonote::NOTIFY_NEW_BLOCK::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_NEW_TRANSACTIONS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_GET_OBJECTS::ID:
      return 1024 * 1024 * 2; // 2 MB
    case cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::ID:
      return 1024 * 1024 * 128; // 128 MB (max packet is a bit less than 100 MB though)
    case cryptonote::NOTIFY_REQUEST_CHAIN::ID:
      return 512 * 1024; // 512 kB
    case cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::ID:
      return 1024 * 1024 * 4; // 4 MB
    case cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::ID:
      return 1024 * 1024 * 4; // 4 MB, but it does not includes transaction data
    case cryptonote::NOTIFY_REQUEST_FLUFFY_MISSING_TX::ID:
      return 1024 * 1024; // 1 MB
    case cryptonote::NOTIFY_GET_TXPOOL_COMPLEMENT::ID:
      return 1024 * 1024 * 4; // 4 MB
    case cryptonote::NOTIFY_TX_POOL_INV::ID:
      return 1024 * 1024 * 2; // 2 MB
    case cryptonote::NOTIFY_REQUEST_TX_POOL_TXS::ID:
      return 1024 * 1024 * 2; // 2 MB
    default:
      break;
    };
    return std::numeric_limits<size_t>::max();
  }

  void cryptonote_connection_context::set_state_normal()
  {
    m_state = state_normal;
    m_expected_heights_start = 0;
    m_needed_objects.clear();
    m_needed_objects.shrink_to_fit();
    m_expected_heights.clear();
    m_expected_heights.shrink_to_fit();
    m_requested_objects.clear();
  }

  boost::optional<crypto::hash> cryptonote_connection_context::get_expected_hash(const uint64_t height) const
  {
    const auto difference = height - m_expected_heights_start;
    if (height < m_expected_heights_start || m_expected_heights.size() <= difference)
      return boost::none;
    return m_expected_heights[difference];
  }

  void cryptonote_connection_context::reset()
  {
    MINFO("Resetting connection statistics for peer: " << epee::string_tools::pod_to_hex(m_connection_id));
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    m_connection_stats->tx_announcements.clear();
    m_connection_stats->received = 0;
    m_connection_stats->requested_from_me = 0;
    m_connection_stats->requested_from_peer = 0;
    m_connection_stats->sent = 0;
    m_connection_stats->missed = 0;
  }

  void cryptonote_connection_context::add_announcement(const crypto::hash &tx_hash)
  {
    MINFO("Adding announcement for tx " << epee::string_tools::pod_to_hex(tx_hash)
          << " to peer: " << epee::string_tools::pod_to_hex(m_connection_id));
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    m_connection_stats->tx_announcements.insert(tx_hash);
  }

  void cryptonote_connection_context::add_received()
  {
    MINFO("Incrementing received count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current received: " << m_connection_stats->received);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    ++m_connection_stats->received;
  }

  void cryptonote_connection_context::add_requested_from_me()
  {
    MINFO("Incrementing requested_from_me count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current in_flight_requests: " << m_connection_stats-> in_flight_requests << ", current requested_from_me: " << m_connection_stats->requested_from_me);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    ++m_connection_stats->requested_from_me;
  }

  void cryptonote_connection_context::add_requested_from_peer()
  {
    MINFO("Incrementing requested_from_peer count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current in_flight_requests: " << m_connection_stats->in_flight_requests << ", current requested_from_peer: " << m_connection_stats->requested_from_peer);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    // increment in_flight_requests directly to avoid recursive locking deadlock
    ++m_connection_stats->in_flight_requests;
    ++m_connection_stats->requested_from_peer;
  }

  void cryptonote_connection_context::add_sent()
  {
    MINFO("Incrementing sent count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current sent: " << m_connection_stats->sent);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    ++m_connection_stats->sent;
  }

  void cryptonote_connection_context::add_in_flight_requests()
  {
    MINFO("Incrementing in_flight_requests count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current in_flight_requests: " << m_connection_stats->in_flight_requests);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    ++m_connection_stats->in_flight_requests;
  }

  void cryptonote_connection_context::remove_in_flight_request()
  {
    MINFO("Decrementing in_flight_requests count for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current in_flight_requests: " << m_connection_stats->in_flight_requests);
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    if (m_connection_stats->in_flight_requests > 0)
      --m_connection_stats->in_flight_requests;
  }

  bool cryptonote_connection_context::can_process_additional_request(size_t requests)
  {
    MINFO("Checking if can process additional requests for peer: " << epee::string_tools::pod_to_hex(m_connection_id) << ", current in_flight_requests: " << m_connection_stats->in_flight_requests << ", additional requests: " << requests << " we can" << ((m_connection_stats->in_flight_requests + requests) < P2P_MAX_IN_FLIGHT_REQUESTS ? " " : " not ") << "process.");
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return (m_connection_stats->in_flight_requests + requests) < P2P_MAX_IN_FLIGHT_REQUESTS;
  }

  size_t cryptonote_connection_context::get_announcement_size() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->tx_announcements.size();
  }

  size_t cryptonote_connection_context::get_received() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->received;
  }

  size_t cryptonote_connection_context::get_requested_from_me() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->requested_from_me;
  }

  size_t cryptonote_connection_context::get_requested_from_peer() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->requested_from_peer;
  }

  size_t cryptonote_connection_context::get_sent() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->sent;
  }

  size_t cryptonote_connection_context::get_total() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->tx_announcements.size()
        + m_connection_stats->received
        + m_connection_stats->requested_from_me
        + m_connection_stats->requested_from_peer
        + m_connection_stats->sent;
  }

  size_t cryptonote_connection_context::get_missed() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->missed;
  }

  size_t cryptonote_connection_context::get_in_flight_requests() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    return m_connection_stats->in_flight_requests;
  }

  std::string cryptonote_connection_context::get_info() const
  {
    std::shared_lock<std::shared_timed_mutex> r_lock(m_connection_stats->mutex);
    std::ostringstream oss;
    oss << "Peer ID: " << epee::string_tools::pod_to_hex(m_connection_id)
        << ", Announcements size: " << m_connection_stats->tx_announcements.size()
        << ", Received: " << m_connection_stats->received
        << ", Requested from me : " << m_connection_stats->requested_from_me
        << ", Requested from peer: " << m_connection_stats->requested_from_peer
        << ", Sent: " << m_connection_stats->sent
        << ", Missed: " << m_connection_stats->missed;
    return oss.str();
  }

  bool cryptonote_connection_context::missed_announced_tx()
  {
    std::unique_lock<std::shared_timed_mutex> w_lock(m_connection_stats->mutex);
    ++m_connection_stats->missed;
    const size_t announced = m_connection_stats->tx_announcements.size();
    if (announced == 0) return false;
    const size_t percent = (m_connection_stats->missed * 100) / announced;
    MINFO("Peer " << epee::string_tools::pod_to_hex(m_connection_id)
          << " has missed " << m_connection_stats->missed << " out of "
          << announced << " announced txs (" << percent << "%)");
    return percent > P2P_REQUEST_FAILURE_THRESHOLD_PERCENTAGE;
  }

} // cryptonote
