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

#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/optional/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>


#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "net/enums.h"
#include "p2p_protocol_defs.h"
#include "syncobj.h"

namespace nodetool
{
  using netgroup = std::array<uint8_t, 5>;

  inline boost::optional<netgroup> get_netgroup(const epee::net_utils::network_address& address)
  {
    boost::optional<epee::net_utils::ipv4_network_address> ipv4 = epee::net_utils::get_ipv4_mapped_address(address);
    if (address.get_type_id() == epee::net_utils::ipv4_network_address::get_type_id())
      ipv4 = address.as<const epee::net_utils::ipv4_network_address>();
    if (ipv4)
    {
      const uint32_t ip = ipv4->ip();
      return netgroup{{4, uint8_t(ip), uint8_t(ip >> 8), 0, 0}};
    }

    if (address.get_type_id() != epee::net_utils::ipv6_network_address::get_type_id())
      return boost::none;

    const boost::asio::ip::address_v6::bytes_type bytes = address.as<const epee::net_utils::ipv6_network_address>().ip().to_bytes();
    return netgroup{{6, bytes[0], bytes[1], bytes[2], bytes[3]}};
  }

  inline std::string get_bucket_group(const epee::net_utils::network_address& address)
  {
    const boost::optional<netgroup> group = get_netgroup(address);
    return group ? std::string(group->begin(), group->end()) : address.host_str();
  }

  struct peerlist_types
  {
    std::vector<peerlist_entry> white;
    std::vector<peerlist_entry> gray;
    std::vector<anchor_peerlist_entry> anchor;
  };

  class peerlist_storage
  {
  public:
    peerlist_storage()
      : m_types{}
    {}

    //! \return Peers stored in stream `src` in `new_format` (portable archive or older non-portable).
    static boost::optional<peerlist_storage> open(std::istream& src, const bool new_format);

    //! \return Peers stored in file at `path`
    static boost::optional<peerlist_storage> open(const std::string& path);

    peerlist_storage(peerlist_storage&&) = default;
    peerlist_storage(const peerlist_storage&) = delete;

    ~peerlist_storage() noexcept;

    peerlist_storage& operator=(peerlist_storage&&) = default;
    peerlist_storage& operator=(const peerlist_storage&) = delete;

    //! Save peers from `this` and `other` in stream `dest`.
    bool store(std::ostream& dest, const peerlist_types& other) const;

    //! Save peers from `this` and `other` in one file at `path`.
    bool store(const std::string& path, const peerlist_types& other) const;

    //! \return Peers in `zone` and from remove from `this`.
    peerlist_types take_zone(epee::net_utils::zone zone);

  private:
    peerlist_types m_types;
  };

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class peerlist_manager
  {
  public: 
    bool init(peerlist_types&& peers, bool allow_local_ip);
    size_t get_white_peers_count(){CRITICAL_REGION_LOCAL(m_peerlist_lock); return m_peers_white.size();}
    size_t get_gray_peers_count(){CRITICAL_REGION_LOCAL(m_peerlist_lock); return m_peers_gray.size();}
    bool merge_peerlist(const std::vector<peerlist_entry>& outer_bs, const epee::net_utils::network_address& source, const std::function<bool(const peerlist_entry&)> &f = NULL);
    bool get_peerlist_head(std::vector<peerlist_entry>& bs_head, bool anonymize, uint32_t depth = P2P_DEFAULT_PEERS_IN_HANDSHAKE);
    void get_peerlist(std::vector<peerlist_entry>& pl_gray, std::vector<peerlist_entry>& pl_white);
    void get_peerlist(peerlist_types& peers);
    template<typename F> bool foreach(bool white, const F &f);
    void evict_host_from_peerlist(bool white, const peerlist_entry& pr);
    bool append_with_peer_white(const peerlist_entry& pr, bool trust_last_seen = false);
    bool append_with_peer_gray(const peerlist_entry& pr, const epee::net_utils::network_address& source);
    bool append_with_peer_anchor(const anchor_peerlist_entry& ple);
    bool set_peer_just_seen(peerid_type peer, const epee::net_utils::network_address& addr, uint32_t pruning_seed, uint16_t rpc_port, uint32_t rpc_credits_per_hash);
    bool is_host_allowed(const epee::net_utils::network_address &address);
    bool get_random_gray_peer(peerlist_entry& pe);
    bool remove_from_peer_gray(const peerlist_entry& pe);
    bool get_and_empty_anchor_peerlist(std::vector<anchor_peerlist_entry>& apl);
    bool remove_from_peer_anchor(const epee::net_utils::network_address& addr);
    bool remove_from_peer_white(const peerlist_entry& pe);
    bool get_white_collision(peerlist_entry& oldest, peerlist_entry& newcomer);
    template<typename F> size_t filter(bool white, const F &f); // f returns true: drop, false: keep
    
  private:
    struct by_time{};
    struct by_addr{};

    typedef boost::multi_index_container<
      peerlist_entry,
      boost::multi_index::indexed_by<
      // access by peerlist_entry::net_address
      boost::multi_index::ordered_unique<boost::multi_index::tag<by_addr>, boost::multi_index::member<peerlist_entry,epee::net_utils::network_address,&peerlist_entry::adr> >,
      // sort by peerlist_entry::last_seen<
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_time>, boost::multi_index::member<peerlist_entry,int64_t,&peerlist_entry::last_seen> >
      > 
    > peers_indexed;

    typedef boost::multi_index_container<
      anchor_peerlist_entry,
      boost::multi_index::indexed_by<
      // access by anchor_peerlist_entry::net_address
      boost::multi_index::ordered_unique<boost::multi_index::tag<by_addr>, boost::multi_index::member<anchor_peerlist_entry,epee::net_utils::network_address,&anchor_peerlist_entry::adr> >,
      // sort by anchor_peerlist_entry::first_seen
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_time>, boost::multi_index::member<anchor_peerlist_entry,int64_t,&anchor_peerlist_entry::first_seen> >
      >
    > anchor_peers_indexed;

  private: 
    void trim_white_peerlist();
    bool white_has_room();
    uint32_t get_gray_bucket(const epee::net_utils::network_address& adr, const epee::net_utils::network_address& source) const;
    uint64_t keyed_hash(std::initializer_list<std::string> parts) const;
    peers_indexed::index<by_addr>::type::iterator erase_gray(peers_indexed::index<by_addr>::type::iterator it);
    static peerlist_entry get_nth_latest_peer(peers_indexed& peerlist, size_t n);

    friend class boost::serialization::access;
    epee::critical_section m_peerlist_lock;
    std::string m_config_folder;
    bool m_allow_local_ip;


    peers_indexed m_peers_gray;
    peers_indexed m_peers_white;
    anchor_peers_indexed m_peers_anchor;
    std::deque<peerlist_entry> m_white_collisions;
    crypto::hash m_bucket_key = crypto::rand<crypto::hash>();
    std::map<epee::net_utils::network_address, uint32_t> m_gray_buckets;
    std::vector<uint32_t> m_gray_bucket_sizes = std::vector<uint32_t>(P2P_LOCAL_GRAY_BUCKET_COUNT);
  };
  //--------------------------------------------------------------------------------------------------
  inline uint64_t peerlist_manager::keyed_hash(std::initializer_list<std::string> parts) const
  {
    std::string data(std::begin(m_bucket_key.data), std::end(m_bucket_key.data));
    for (const std::string& part : parts)
      data += std::to_string(part.size()) + ':' + part;
    const crypto::hash hash = crypto::cn_fast_hash(data.data(), data.size());
    uint64_t result;
    memcpy(&result, hash.data, sizeof(result));
    return result;
  }
  //--------------------------------------------------------------------------------------------------
  inline uint32_t peerlist_manager::get_gray_bucket(const epee::net_utils::network_address& adr, const epee::net_utils::network_address& source) const
  {
    const std::string adr_group = get_bucket_group(adr);
    const std::string source_group = get_bucket_group(source);
    const uint64_t source_bucket = keyed_hash({adr_group, source_group}) % P2P_LOCAL_GRAY_BUCKETS_PER_SOURCE_GROUP;
    return keyed_hash({source_group, std::to_string(source_bucket)}) % P2P_LOCAL_GRAY_BUCKET_COUNT;
  }
  //--------------------------------------------------------------------------------------------------
  inline peerlist_manager::peers_indexed::index<peerlist_manager::by_addr>::type::iterator peerlist_manager::erase_gray(peers_indexed::index<by_addr>::type::iterator it)
  {
    const auto bucket = m_gray_buckets.find(it->adr);
    --m_gray_bucket_sizes[bucket->second];
    m_gray_buckets.erase(bucket);
    return m_peers_gray.get<by_addr>().erase(it);
  }
  //--------------------------------------------------------------------------------------------------
  inline void peerlist_manager::trim_white_peerlist()
  {
    while(m_peers_white.size() > P2P_LOCAL_WHITE_PEERLIST_LIMIT)
    {
      peers_indexed::index<by_time>::type& sorted_index=m_peers_white.get<by_time>();
      sorted_index.erase(sorted_index.begin());
    }
  }
  //--------------------------------------------------------------------------------------------------
  inline bool peerlist_manager::white_has_room()
  {
    if (m_peers_white.size() < P2P_LOCAL_WHITE_PEERLIST_LIMIT)
      return true;
    const int64_t oldest_last_seen = m_peers_white.get<by_time>().begin()->last_seen;
    return time(nullptr) - oldest_last_seen > P2P_WHITE_PEER_STALE_SECONDS;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  peerlist_entry peerlist_manager::get_nth_latest_peer(peers_indexed& peerlist, const size_t n)
  {
    // Is not thread-safe nor does it check bounds. Do this before calling. Indexing starts at 0.
    peers_indexed::index<by_time>::type& by_time_index = peerlist.get<by_time>();
    auto by_time_it = --by_time_index.end();
    std::advance(by_time_it, -static_cast<long long>(n));
    return *by_time_it;
  }
  //--------------------------------------------------------------------------------------------------
  inline 
  bool peerlist_manager::merge_peerlist(const std::vector<peerlist_entry>& outer_bs, const epee::net_utils::network_address& source, const std::function<bool(const peerlist_entry&)> &f)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    for(const peerlist_entry& be:  outer_bs)
    {
      if (!f || f(be))
        append_with_peer_gray(be, source);
    }
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::is_host_allowed(const epee::net_utils::network_address &address)
  {
    //never allow loopback ip
    if(address.is_loopback())
      return false;

    if(!m_allow_local_ip && address.is_local())
      return false;

    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline 
  bool peerlist_manager::get_peerlist_head(std::vector<peerlist_entry>& bs_head, bool anonymize, uint32_t depth)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers_indexed::index<by_time>::type& by_time_index=m_peers_white.get<by_time>();
    uint32_t cnt = 0;

    // picks a random set of peers within the whole set, rather pick the first depth elements.
    // The intent is that if someone asks twice, they can't easily tell:
    // - this address was not in the first list, but is in the second, so the only way this can be
    // is if its last_seen was recently reset, so this means the target node recently had a new
    // connection to that address
    // - this address was in the first list, and not in the second, which means either the address
    // was moved to the gray list (if it's not accessible, which the attacker can check if
    // the address accepts incoming connections) or it was the oldest to still fit in the 250 items,
    // so its last_seen is old.
    //
    // See Cao, Tong et al. "Exploring the Monero Peer-to-Peer Network". https://eprint.iacr.org/2019/411
    //
    const uint32_t pick_depth = anonymize ? m_peers_white.size() : depth;
    const int64_t stale_before = time(nullptr) - P2P_WHITE_PEER_STALE_SECONDS;
    bs_head.reserve(pick_depth);
    for(const peers_indexed::value_type& vl: boost::adaptors::reverse(by_time_index))
    {
      if(cnt++ >= pick_depth || vl.last_seen < stale_before)
        break;

      bs_head.push_back(vl);
    }

    if (anonymize)
    {
      std::shuffle(bs_head.begin(), bs_head.end(), crypto::random_device{});
      if (bs_head.size() > depth)
        bs_head.resize(depth);
      for (auto &e: bs_head)
        e.last_seen = 0;
    }

    return true;
  }
  //--------------------------------------------------------------------------------------------------
  template<typename F> inline
  bool peerlist_manager::foreach(bool white, const F &f)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers_indexed::index<by_time>::type& by_time_index = white ? m_peers_white.get<by_time>() : m_peers_gray.get<by_time>();
    for(const peers_indexed::value_type& vl: boost::adaptors::reverse(by_time_index))
      if (!f(vl))
        return false;
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::set_peer_just_seen(peerid_type peer, const epee::net_utils::network_address& addr, uint32_t pruning_seed, uint16_t rpc_port, uint32_t rpc_credits_per_hash)
  {
    TRY_ENTRY();
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    //find in white list
    peerlist_entry ple;
    ple.adr = addr;
    ple.id = peer;
    ple.last_seen = time(NULL);
    ple.pruning_seed = pruning_seed;
    ple.rpc_port = rpc_port;
    ple.rpc_credits_per_hash = rpc_credits_per_hash;
    return append_with_peer_white(ple, true);
    CATCH_ENTRY_L0("peerlist_manager::set_peer_just_seen()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::append_with_peer_white(const peerlist_entry& ple, bool trust_last_seen)
  {
    TRY_ENTRY();
    if(!is_host_allowed(ple.adr))
      return true;

     CRITICAL_REGION_LOCAL(m_peerlist_lock);
    //find in white list
    auto by_addr_it_wt = m_peers_white.get<by_addr>().find(ple.adr);
    if(by_addr_it_wt == m_peers_white.get<by_addr>().end())
    {
      //put new record into white list
      evict_host_from_peerlist(true, ple);
      if (!white_has_room())
      {
        const bool queued = std::any_of(m_white_collisions.begin(), m_white_collisions.end(),
          [&ple](const peerlist_entry& pe) { return pe.adr == ple.adr; });
        if (!queued && m_white_collisions.size() < P2P_LOCAL_WHITE_PEERLIST_COLLISIONS_MAX)
          m_white_collisions.push_back(ple);
        return true;
      }
      m_peers_white.insert(ple);
      trim_white_peerlist();
    }else
    {
      //update record in white list
      peerlist_entry new_ple = ple;
      if (by_addr_it_wt->pruning_seed && ple.pruning_seed == 0) // guard against older nodes not passing pruning info around
        new_ple.pruning_seed = by_addr_it_wt->pruning_seed;
      if (by_addr_it_wt->rpc_port && ple.rpc_port == 0) // guard against older nodes not passing RPC port around
        new_ple.rpc_port = by_addr_it_wt->rpc_port;
      if (!trust_last_seen)
        new_ple.last_seen = by_addr_it_wt->last_seen; // do not overwrite the last seen timestamp, incoming peer lists are untrusted
      m_peers_white.replace(by_addr_it_wt, new_ple);
    }
    //remove from gray list, if need
    auto by_addr_it_gr = m_peers_gray.get<by_addr>().find(ple.adr);
    if(by_addr_it_gr != m_peers_gray.get<by_addr>().end())
    {
      erase_gray(by_addr_it_gr);
    }
    return true;
    CATCH_ENTRY_L0("peerlist_manager::append_with_peer_white()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::append_with_peer_gray(const peerlist_entry& ple, const epee::net_utils::network_address& source)
  {
    TRY_ENTRY();
    if(!is_host_allowed(ple.adr))
      return true;

    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    //find in white list
    auto by_addr_it_wt = m_peers_white.get<by_addr>().find(ple.adr);
    if(by_addr_it_wt != m_peers_white.get<by_addr>().end())
      return true;

    //update gray list
    auto by_addr_it_gr = m_peers_gray.get<by_addr>().find(ple.adr);
    if(by_addr_it_gr == m_peers_gray.get<by_addr>().end())
    {
      //put new record into gray list, if its bucket has room
      const uint32_t bucket = get_gray_bucket(ple.adr, source);
      if (m_gray_bucket_sizes[bucket] >= P2P_LOCAL_GRAY_BUCKET_SIZE)
        return true;
      ++m_gray_bucket_sizes[bucket];
      m_gray_buckets.emplace(ple.adr, bucket);
      m_peers_gray.insert(ple);
    }else
    {
      //update record in gray list
      peerlist_entry new_ple = ple;
      if (by_addr_it_gr->pruning_seed && ple.pruning_seed == 0) // guard against older nodes not passing pruning info around
        new_ple.pruning_seed = by_addr_it_gr->pruning_seed;
      if (by_addr_it_gr->rpc_port && ple.rpc_port == 0) // guard against older nodes not passing RPC port around
        new_ple.rpc_port = by_addr_it_gr->rpc_port;
      new_ple.last_seen = by_addr_it_gr->last_seen; // do not overwrite the last seen timestamp, incoming peer list are untrusted
      m_peers_gray.replace(by_addr_it_gr, new_ple);
    }
    return true;
    CATCH_ENTRY_L0("peerlist_manager::append_with_peer_gray()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::append_with_peer_anchor(const anchor_peerlist_entry& ple)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    auto by_addr_it_anchor = m_peers_anchor.get<by_addr>().find(ple.adr);

    if(by_addr_it_anchor == m_peers_anchor.get<by_addr>().end()) {
      m_peers_anchor.insert(ple);
    }

    return true;

    CATCH_ENTRY_L0("peerlist_manager::append_with_peer_anchor()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::get_random_gray_peer(peerlist_entry& pe)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    if (m_peers_gray.empty()) {
      return false;
    }

    size_t random_index = crypto::rand_idx(m_peers_gray.size());
    pe = peerlist_manager::get_nth_latest_peer(m_peers_gray, random_index);

    return true;

    CATCH_ENTRY_L0("peerlist_manager::get_random_gray_peer()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::remove_from_peer_white(const peerlist_entry& pe)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    peers_indexed::index_iterator<by_addr>::type iterator = m_peers_white.get<by_addr>().find(pe.adr);

    if (iterator != m_peers_white.get<by_addr>().end()) {
      m_peers_white.erase(iterator);
    }

    m_white_collisions.erase(std::remove_if(m_white_collisions.begin(), m_white_collisions.end(),
      [&pe](const peerlist_entry& queued) { return queued.adr == pe.adr; }), m_white_collisions.end());

    return true;

    CATCH_ENTRY_L0("peerlist_manager::remove_from_peer_white()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::get_white_collision(peerlist_entry& oldest, peerlist_entry& newcomer)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    while (!m_white_collisions.empty())
    {
      newcomer = m_white_collisions.front();
      m_white_collisions.pop_front();
      if (m_peers_white.get<by_addr>().count(newcomer.adr))
        continue;
      if (white_has_room())
      {
        append_with_peer_white(newcomer);
        continue;
      }
      oldest = *m_peers_white.get<by_time>().begin();
      return true;
    }

    return false;

    CATCH_ENTRY_L0("peerlist_manager::get_white_collision()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::remove_from_peer_gray(const peerlist_entry& pe)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    peers_indexed::index_iterator<by_addr>::type iterator = m_peers_gray.get<by_addr>().find(pe.adr);

    if (iterator != m_peers_gray.get<by_addr>().end()) {
      erase_gray(iterator);
    }

    return true;

    CATCH_ENTRY_L0("peerlist_manager::remove_from_peer_gray()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::get_and_empty_anchor_peerlist(std::vector<anchor_peerlist_entry>& apl)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    auto begin = m_peers_anchor.get<by_time>().begin();
    auto end = m_peers_anchor.get<by_time>().end();

    std::for_each(begin, end, [&apl](const anchor_peerlist_entry &a) {
      apl.push_back(a);
    });

    m_peers_anchor.get<by_time>().clear();

    return true;

    CATCH_ENTRY_L0("peerlist_manager::get_and_empty_anchor_peerlist()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::remove_from_peer_anchor(const epee::net_utils::network_address& addr)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    anchor_peers_indexed::index_iterator<by_addr>::type iterator = m_peers_anchor.get<by_addr>().find(addr);

    if (iterator != m_peers_anchor.get<by_addr>().end()) {
      m_peers_anchor.erase(iterator);
    }

    return true;

    CATCH_ENTRY_L0("peerlist_manager::remove_from_peer_anchor()", false);
  }
  //--------------------------------------------------------------------------------------------------
  template<typename F> size_t peerlist_manager::filter(bool white, const F &f)
  {
    size_t filtered = 0;
    TRY_ENTRY();
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers_indexed::index<by_addr>::type& sorted_index = white ? m_peers_white.get<by_addr>() : m_peers_gray.get<by_addr>();
    auto i = sorted_index.begin();
    while (i != sorted_index.end())
    {
      if (f(*i))
      {
        i = white ? sorted_index.erase(i) : erase_gray(i);
        ++filtered;
      }
      else
        ++i;
    }
    if (white)
      m_white_collisions.erase(std::remove_if(m_white_collisions.begin(), m_white_collisions.end(), f), m_white_collisions.end());
    CATCH_ENTRY_L0("peerlist_manager::filter()", filtered);
    return filtered;
  }
  //--------------------------------------------------------------------------------------------------
}

