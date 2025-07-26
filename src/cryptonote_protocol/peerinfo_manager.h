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

#ifndef CRYPTONOTE_PROTOCOL_PEERINFO_MANAGER_H
#define CRYPTONOTE_PROTOCOL_PEERINFO_MANAGER_H

#include <boost/functional/hash.hpp>
#include <boost/utility/value_init.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <sstream>
#include <string>
#include <unordered_set>

#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "misc_log_ex.h"
#include "string_tools.h"
#include "syncobj.h"

class peer_info_manager {

public:
  class peer_info {

  private:
    struct peer_info_state {
      boost::uuids::uuid connection_id;
      std::unordered_set<crypto::hash> announcements;
      size_t received = 0;
      size_t requested_from_me = 0;
      size_t requested_from_peer = 0;
      size_t sent = 0;
      size_t missed = 0;
      mutable epee::rw_mutex m_mutex;

      // delete moving, copying, and assignment
      peer_info_state() = default;
      peer_info_state(const peer_info_state &) = delete;
      peer_info_state(peer_info_state &&) = delete;
      peer_info_state &operator=(const peer_info_state &) = delete;
      peer_info_state &operator=(peer_info_state &&) = delete;
      ~peer_info_state() = default;
      peer_info_state(const boost::uuids::uuid &id)
          : connection_id(id), received(0), requested_from_me(0),
            requested_from_peer(0), sent(0), missed(0) {}
    };

    std::shared_ptr<peer_info_state> m_data;

  public:
    peer_info(const boost::uuids::uuid &id)
        : m_data(std::make_shared<peer_info_state>(id)) {}

    peer_info(peer_info &&other) noexcept = default;
    peer_info &operator=(peer_info &&other) noexcept = default;

    struct hash {
      std::size_t operator()(const peer_info &p) const {
        // We don't care about the bool value, so we only hash the UUID
        return boost::hash<boost::uuids::uuid>()(p.get_connection_id());
      }
    };

    peer_info &operator=(const peer_info &other) = delete;
    peer_info(const peer_info &other) = delete;
    bool operator>(const peer_info &other) const = delete;
    bool operator<(const peer_info &other) const = delete;

    bool operator==(const peer_info &other) const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->connection_id == other.get_connection_id();
    }

    bool operator!=(const peer_info &other) const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->connection_id != other.get_connection_id();
    }

    void reset() {
      epee::write_lock w_lock(m_data->m_mutex);
      m_data->announcements.clear();
      m_data->received = 0;
      m_data->requested_from_me = 0;
      m_data->sent = 0;
    }

    void add_announcement(const crypto::hash &tx_hash) {
      epee::write_lock w_lock(m_data->m_mutex);
      m_data->announcements.insert(tx_hash);
    }

    void add_received() {
      epee::write_lock w_lock(m_data->m_mutex);
      ++m_data->received;
    }

    void add_requested_from_me() {
      epee::write_lock w_lock(m_data->m_mutex);
      ++m_data->requested_from_me;
    }

    void add_requested_from_peer() {
      epee::write_lock w_lock(m_data->m_mutex);
      ++m_data->requested_from_peer;
    }

    void add_sent() {
      epee::write_lock w_lock(m_data->m_mutex);
      ++m_data->sent;
    }

    void add_missed() {
      epee::write_lock w_lock(m_data->m_mutex);
      ++m_data->missed;
    }

    size_t get_announcement_size() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->announcements.size();
    }

    size_t get_received() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->received;
    }

    size_t get_requested_from_me() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->requested_from_me;
    }

    size_t get_requested_from_peer() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->requested_from_peer;
    }

    size_t get_sent() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->sent;
    }

    const boost::uuids::uuid &get_connection_id() const {
      return m_data->connection_id;
    }

    size_t get_total() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->announcements.size() + m_data->received + m_data->requested_from_me +
             m_data->requested_from_peer + m_data->sent;
    }

    size_t get_missed() const {
      epee::read_lock r_lock(m_data->m_mutex);
      return m_data->missed;
    }

    std::string get_info() const {
      epee::read_lock r_lock(m_data->m_mutex);

      std::ostringstream oss;
      oss << "Peer ID: " << epee::string_tools::pod_to_hex(m_data->connection_id)
          << ", Announcements size: " << m_data->announcements.size()
          << ", Received: " << m_data->received
          << ", Requested from me : " << m_data->requested_from_me
          << ", Requested from peer: " << m_data->requested_from_peer
          << ", Sent: " << m_data->sent << ", Missed: " << m_data->missed;
      return oss.str();
    }
  };

private:
  std::unordered_set<peer_info, peer_info::hash> m_peer_info;

public:
  peer_info_manager() = default;

  void add_peer(const boost::uuids::uuid &id) { m_peer_info.emplace(id); }

  void remove_peer(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      m_peer_info.erase(it);
    }
  }

  std::string get_peer_info(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_info();
    }
    return std::string();
  }

  std::string get_all_peers_info() const {
    std::ostringstream oss;
    oss << "Total Peers: " << m_peer_info.size() << std::endl;
    oss << "Peer Information:" << std::endl;
    for (const auto &peer : m_peer_info) {
      oss << peer.get_info() << std::endl;
    }
    return oss.str();
  }

  void reset_peer_info(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      // We need to cast away constness, since we don't change hash of the
      // object
      const_cast<peer_info &>(*it).reset();
    }
  }

  void add_announcement(const boost::uuids::uuid &id,
                        const crypto::hash &tx_hash) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_announcement(tx_hash);
    } else {
      peer_info new_peer(id);
      new_peer.add_announcement(tx_hash);
      m_peer_info.emplace(std::move(new_peer));
    }
  }

  void add_received(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_received();
    } else {
      peer_info new_peer(id);
      new_peer.add_received();
      m_peer_info.emplace(std::move(new_peer));
    }
  }

  void add_requested_from_me(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_requested_from_me();
    } else {
      peer_info new_peer(id);
      new_peer.add_requested_from_me();
      m_peer_info.emplace(std::move(new_peer));
    }
  }

  void add_requested_from_peer(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_requested_from_peer();
    } else {
      peer_info new_peer(id);
      new_peer.add_requested_from_peer();
      m_peer_info.emplace(std::move(new_peer));
    }
  }

  void add_sent(const boost::uuids::uuid &id) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_sent();
    } else {
      peer_info new_peer(id);
      new_peer.add_sent();
      m_peer_info.emplace(std::move(new_peer));
    }
  }

  size_t get_announcement(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_announcement_size();
    }
    return 0;
  }

  size_t get_received(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_received();
    }
    return 0;
  }

  size_t get_requested_from_me(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_requested_from_me();
    }
    return 0;
  }

  size_t get_requested_from_peer(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_requested_from_peer();
    }
    return 0;
  }

  size_t get_sent(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_sent();
    }
    return 0;
  }

  size_t get_total(const boost::uuids::uuid &id) const {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      return it->get_total();
    }
    return 0;
  }

  bool missed_announced_tx(const boost::uuids::uuid &id,
                           const crypto::hash &tx_hash) {
    auto it = std::find_if(m_peer_info.begin(), m_peer_info.end(),
                           [&id](const peer_info &peer) {
                             return peer.get_connection_id() == id;
                           });
    if (it != m_peer_info.end()) {
      const_cast<peer_info &>(*it).add_missed();
      // P2P_REQUEST_FAILURE_THRESHOLD_PERCENTAGE% of the announcements are
      // missed
      return (it->get_missed() * 100 / it->get_announcement_size()) >
             P2P_REQUEST_FAILURE_THRESHOLD_PERCENTAGE;
    } else {
      MWARNING("Peer not found: " << epee::string_tools::pod_to_hex(id)
                                  << ", tx: "
                                  << epee::string_tools::pod_to_hex(tx_hash)
                                  << ", cannot check missed announcements");
    }
    return false;
  }
};

#endif // CRYPTONOTE_PROTOCOL_PEERINFO_MANAGER_H