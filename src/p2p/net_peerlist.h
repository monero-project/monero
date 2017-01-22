// Copyright (c) 2014-2016, The Monero Project
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

#include <list>
#include <set>
#include <map>
#include <boost/foreach.hpp>
//#include <boost/bimap.hpp>
//#include <boost/bimap/multiset_of.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/serialization/version.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>


#include "syncobj.h"
#include "net/local_ip.h"
#include "p2p_protocol_defs.h"
#include "cryptonote_config.h"
#include "net_peerlist_boost_serialization.h"


#define CURRENT_PEERLIST_STORAGE_ARCHIVE_VER    4

namespace nodetool
{


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class peerlist_manager
  {
  public: 
    bool init(bool allow_local_ip);
    bool deinit();
    size_t get_white_peers_count(){CRITICAL_REGION_LOCAL(m_peerlist_lock); return m_peers_white.size();}
    size_t get_gray_peers_count(){CRITICAL_REGION_LOCAL(m_peerlist_lock); return m_peers_gray.size();}
    bool merge_peerlist(const std::list<peerlist_entry>& outer_bs);
    bool get_peerlist_head(std::list<peerlist_entry>& bs_head, uint32_t depth = P2P_DEFAULT_PEERS_IN_HANDSHAKE);
    bool get_peerlist_full(std::list<peerlist_entry>& pl_gray, std::list<peerlist_entry>& pl_white);
    bool get_white_peer_by_index(peerlist_entry& p, size_t i);
    bool get_gray_peer_by_index(peerlist_entry& p, size_t i);
    bool append_with_peer_white(const peerlist_entry& pr);
    bool append_with_peer_gray(const peerlist_entry& pr);
    bool set_peer_just_seen(peerid_type peer, uint32_t ip, uint32_t port);
    bool set_peer_just_seen(peerid_type peer, const net_address& addr);
    bool set_peer_unreachable(const peerlist_entry& pr);
    bool is_ip_allowed(uint32_t ip);
    bool get_random_gray_peer(peerlist_entry& pe);
    bool remove_from_peer_gray(const peerlist_entry& pe);

    
  private:
    struct by_time{};
    struct by_id{};
    struct by_addr{};

    struct modify_all_but_id
    {
      modify_all_but_id(const peerlist_entry& ple):m_ple(ple){}
      void operator()(peerlist_entry& e)
      {
        e.id = m_ple.id;
      }
    private:
      const peerlist_entry& m_ple;
    };

    struct modify_all
    {
      modify_all(const peerlist_entry& ple):m_ple(ple){}
      void operator()(peerlist_entry& e)
      {
        e = m_ple;
      }
    private:
      const peerlist_entry& m_ple;
    };

    struct modify_last_seen
    {
      modify_last_seen(time_t last_seen):m_last_seen(last_seen){}
      void operator()(peerlist_entry& e)
      {
        e.last_seen = m_last_seen;
      }
    private:
      time_t m_last_seen;
    };


    typedef boost::multi_index_container<
      peerlist_entry,
      boost::multi_index::indexed_by<
      // access by peerlist_entry::net_adress
      boost::multi_index::ordered_unique<boost::multi_index::tag<by_addr>, boost::multi_index::member<peerlist_entry,net_address,&peerlist_entry::adr> >,
      // sort by peerlist_entry::last_seen<
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_time>, boost::multi_index::member<peerlist_entry,int64_t,&peerlist_entry::last_seen> >
      > 
    > peers_indexed;

    typedef boost::multi_index_container<
      peerlist_entry,
      boost::multi_index::indexed_by<
      // access by peerlist_entry::id<
      boost::multi_index::ordered_unique<boost::multi_index::tag<by_id>, boost::multi_index::member<peerlist_entry,uint64_t,&peerlist_entry::id> >,
      // access by peerlist_entry::net_adress
      boost::multi_index::ordered_unique<boost::multi_index::tag<by_addr>, boost::multi_index::member<peerlist_entry,net_address,&peerlist_entry::adr> >,
      // sort by peerlist_entry::last_seen<
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_time>, boost::multi_index::member<peerlist_entry,int64_t,&peerlist_entry::last_seen> >
      > 
    > peers_indexed_old;
  public:    
    
    template <class Archive, class t_version_type>
    void serialize(Archive &a,  const t_version_type ver)
    {
      if(ver < 3)
        return;
      CRITICAL_REGION_LOCAL(m_peerlist_lock);
      if(ver < 4)
      {
        //loading data from old storage
        peers_indexed_old pio; 
        a & pio;
        peers_indexed_from_old(pio, m_peers_white);
        return;
      }
      a & m_peers_white;
      a & m_peers_gray;
    }

  private: 
    bool peers_indexed_from_old(const peers_indexed_old& pio, peers_indexed& pi);
    void trim_white_peerlist();
    void trim_gray_peerlist();

    friend class boost::serialization::access;
    epee::critical_section m_peerlist_lock;
    std::string m_config_folder;
    bool m_allow_local_ip;


    peers_indexed m_peers_gray;
    peers_indexed m_peers_white;
  };
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::init(bool allow_local_ip)
  {
    m_allow_local_ip = allow_local_ip;
    return true;
  } 
  //--------------------------------------------------------------------------------------------------
  inline
    bool peerlist_manager::deinit()
  {
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline 
  bool peerlist_manager::peers_indexed_from_old(const peers_indexed_old& pio, peers_indexed& pi)
  {
    for(auto x: pio)
    {
      auto by_addr_it = pi.get<by_addr>().find(x.adr);
      if(by_addr_it == pi.get<by_addr>().end())
      {
        pi.insert(x);
      }
    }

    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline void peerlist_manager::trim_gray_peerlist()
  {
    while(m_peers_gray.size() > P2P_LOCAL_GRAY_PEERLIST_LIMIT)
    {
      peers_indexed::index<by_time>::type& sorted_index=m_peers_gray.get<by_time>();
      sorted_index.erase(sorted_index.begin());
    }
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
  inline 
  bool peerlist_manager::merge_peerlist(const std::list<peerlist_entry>& outer_bs)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    BOOST_FOREACH(const peerlist_entry& be,  outer_bs)
    {
      append_with_peer_gray(be);
    }
    // delete extra elements
    trim_gray_peerlist();    
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::get_white_peer_by_index(peerlist_entry& p, size_t i)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    if(i >= m_peers_white.size())
      return false;

    peers_indexed::index<by_time>::type& by_time_index = m_peers_white.get<by_time>();
    p = *epee::misc_utils::move_it_backward(--by_time_index.end(), i);    
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
    bool peerlist_manager::get_gray_peer_by_index(peerlist_entry& p, size_t i)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    if(i >= m_peers_gray.size())
      return false;

    peers_indexed::index<by_time>::type& by_time_index = m_peers_gray.get<by_time>();
    p = *epee::misc_utils::move_it_backward(--by_time_index.end(), i);    
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline 
  bool peerlist_manager::is_ip_allowed(uint32_t ip)
  {
    //never allow loopback ip
    if(epee::net_utils::is_ip_loopback(ip))
      return false;

    if(!m_allow_local_ip && epee::net_utils::is_ip_local(ip))
      return false;

    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline 
  bool peerlist_manager::get_peerlist_head(std::list<peerlist_entry>& bs_head, uint32_t depth)
  {
    
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers_indexed::index<by_time>::type& by_time_index=m_peers_white.get<by_time>();
    uint32_t cnt = 0;
    BOOST_REVERSE_FOREACH(const peers_indexed::value_type& vl, by_time_index)
    {
      if(!vl.last_seen)
        continue;

      if(cnt++ >= depth)
        break;

      bs_head.push_back(vl);
    }
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::get_peerlist_full(std::list<peerlist_entry>& pl_gray, std::list<peerlist_entry>& pl_white)
  {    
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers_indexed::index<by_time>::type& by_time_index_gr=m_peers_gray.get<by_time>();
    BOOST_REVERSE_FOREACH(const peers_indexed::value_type& vl, by_time_index_gr)
    {
      pl_gray.push_back(vl);      
    }

    peers_indexed::index<by_time>::type& by_time_index_wt=m_peers_white.get<by_time>();
    BOOST_REVERSE_FOREACH(const peers_indexed::value_type& vl, by_time_index_wt)
    {
      pl_white.push_back(vl);      
    }

    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::set_peer_just_seen(peerid_type peer, uint32_t ip, uint32_t port)
  {
    net_address addr;
    addr.ip = ip;
    addr.port = port;
    return set_peer_just_seen(peer, addr);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::set_peer_just_seen(peerid_type peer, const net_address& addr)
  {
    TRY_ENTRY();
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    //find in white list
    peerlist_entry ple;
    ple.adr = addr;
    ple.id = peer;
    ple.last_seen = time(NULL);
    return append_with_peer_white(ple);
    CATCH_ENTRY_L0("peerlist_manager::set_peer_just_seen()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::append_with_peer_white(const peerlist_entry& ple)
  {
    TRY_ENTRY();
    if(!is_ip_allowed(ple.adr.ip))
      return true;

     CRITICAL_REGION_LOCAL(m_peerlist_lock);
    //find in white list
    auto by_addr_it_wt = m_peers_white.get<by_addr>().find(ple.adr);
    if(by_addr_it_wt == m_peers_white.get<by_addr>().end())
    {
      //put new record into white list
      m_peers_white.insert(ple);
      trim_white_peerlist();
    }else
    {
      //update record in white list 
      m_peers_white.replace(by_addr_it_wt, ple);      
    }
    //remove from gray list, if need
    auto by_addr_it_gr = m_peers_gray.get<by_addr>().find(ple.adr);
    if(by_addr_it_gr != m_peers_gray.get<by_addr>().end())
    {
      m_peers_gray.erase(by_addr_it_gr);
    }
    return true;
    CATCH_ENTRY_L0("peerlist_manager::append_with_peer_white()", false);
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::append_with_peer_gray(const peerlist_entry& ple)
  {
    TRY_ENTRY();
    if(!is_ip_allowed(ple.adr.ip))
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
      //put new record into white list
      m_peers_gray.insert(ple);
      trim_gray_peerlist();    
    }else
    {
      //update record in white list 
      m_peers_gray.replace(by_addr_it_gr, ple);      
    }
    return true;
    CATCH_ENTRY_L0("peerlist_manager::append_with_peer_gray()", false);
    return true;
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

    size_t x = crypto::rand<size_t>() % (m_peers_gray.size() + 1);
    size_t res = (x * x * x) / (m_peers_gray.size() * m_peers_gray.size()); //parabola \/

    LOG_PRINT_L3("Random gray peer index=" << res << "(x="<< x << ", max_index=" << m_peers_gray.size() << ")");

    peers_indexed::index<by_time>::type& by_time_index = m_peers_gray.get<by_time>();
    pe = *epee::misc_utils::move_it_backward(--by_time_index.end(), res);

    return true;

    CATCH_ENTRY_L0("peerlist_manager::get_random_gray_peer()", false);
    return true;
  }
  //--------------------------------------------------------------------------------------------------
  inline
  bool peerlist_manager::remove_from_peer_gray(const peerlist_entry& pe)
  {
    TRY_ENTRY();

    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    peers_indexed::index_iterator<by_addr>::type iterator = m_peers_gray.get<by_addr>().find(pe.adr);

    if (iterator != m_peers_gray.get<by_addr>().end()) {
      m_peers_gray.erase(iterator);
    }

    return true;

    CATCH_ENTRY_L0("peerlist_manager::remove_from_peer_gray()", false);
    return true;
  }
    //--------------------------------------------------------------------------------------------------
}

BOOST_CLASS_VERSION(nodetool::peerlist_manager, CURRENT_PEERLIST_STORAGE_ARCHIVE_VER)
