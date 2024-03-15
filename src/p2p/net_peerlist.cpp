// Copyright (c) 2018-2024, The Monero Project

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

#include "net_peerlist.h"

#include <algorithm>
#include <functional>
#include <fstream>
#include <iterator>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/portable_binary_oarchive.hpp>
#include <boost/archive/portable_binary_iarchive.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/join.hpp>
#include <boost/serialization/version.hpp>

#include "net_peerlist_boost_serialization.h"


namespace nodetool
{
  namespace
  {
    constexpr unsigned CURRENT_PEERLIST_STORAGE_ARCHIVE_VER = 6;
 
    struct by_zone
    {
      using zone = epee::net_utils::zone;

      template<typename T>
      bool operator()(const T& left, const zone right) const
      {
        return left.adr.get_zone() < right;
      }

      template<typename T>
      bool operator()(const zone left, const T& right) const
      {
        return left < right.adr.get_zone();
      }

      template<typename T, typename U>
      bool operator()(const T& left, const U& right) const
      {
        return left.adr.get_zone() < right.adr.get_zone();
      }
    };

    template<typename Elem, typename Archive>
    std::vector<Elem> load_peers(Archive& a, unsigned ver)
    {
      // at v6, we drop existing peerlists, because annoying change
      if (ver < 6)
        return {};

      uint64_t size = 0;
      a & size;
      
      Elem ple{};

      std::vector<Elem> elems{};
      elems.reserve(size);
      while (size--)
      {
        a & ple;
        elems.push_back(std::move(ple));
      }

      return elems;
    }

    template<typename Archive, typename Range>
    void save_peers(Archive& a, const Range& elems)
    {
      const uint64_t size = elems.size();
      a & size;
      for (const auto& elem : elems)
        a & elem;
    }
 
    template<typename T>
    std::vector<T> do_take_zone(std::vector<T>& src, epee::net_utils::zone zone)
    {
      const auto start = std::lower_bound(src.begin(), src.end(), zone, by_zone{});
      const auto end = std::upper_bound(start, src.end(), zone, by_zone{});

      std::vector<T> out{};
      out.assign(std::make_move_iterator(start), std::make_move_iterator(end));
      src.erase(start, end);
      return out;
    }

    template<typename Container, typename T>
    void add_peers(Container& dest, std::vector<T>&& src)
    {
      dest.insert(std::make_move_iterator(src.begin()), std::make_move_iterator(src.end()));
    }

    template<typename Container, typename Range>
    void copy_peers(Container& dest, const Range& src)
    {
      std::copy(src.begin(), src.end(), std::back_inserter(dest));
    }
  } // anonymous

  struct peerlist_join
  {
    const peerlist_types& ours;
    const peerlist_types& other;
  };

  template<typename Archive>
  void serialize(Archive& a, peerlist_types& elem, unsigned ver)
  {
    elem.white = load_peers<peerlist_entry>(a, ver);
    elem.gray = load_peers<peerlist_entry>(a, ver);
    elem.anchor = load_peers<anchor_peerlist_entry>(a, ver);

    if (ver == 0)
    {
      // from v1, we do not store the peer id anymore
      peerid_type peer_id{};
      a & peer_id;
    }
  }
 
  template<typename Archive>
  void serialize(Archive& a, peerlist_join elem, unsigned ver)
  {
    save_peers(a, boost::range::join(elem.ours.white, elem.other.white));
    save_peers(a, boost::range::join(elem.ours.gray, elem.other.gray));
    save_peers(a, boost::range::join(elem.ours.anchor, elem.other.anchor));
  }

  boost::optional<peerlist_storage> peerlist_storage::open(std::istream& src, const bool new_format)
  {
    try
    {
      peerlist_storage out{};
      if (new_format)
      {
        boost::archive::portable_binary_iarchive a{src};
        a >> out.m_types;
      }
      else
      {
        boost::archive::binary_iarchive a{src};
        a >> out.m_types;
      }

      if (src.good())
      {
        std::sort(out.m_types.white.begin(), out.m_types.white.end(), by_zone{});
        std::sort(out.m_types.gray.begin(), out.m_types.gray.end(), by_zone{});
        std::sort(out.m_types.anchor.begin(), out.m_types.anchor.end(), by_zone{});
        return {std::move(out)};
      }
    }
    catch (const std::exception& e)
    {}

    return boost::none;
  }

  boost::optional<peerlist_storage> peerlist_storage::open(const std::string& path)
  {
    std::ifstream src_file{};
    src_file.open( path , std::ios_base::binary | std::ios_base::in);
    if(src_file.fail())
      return boost::none;

    boost::optional<peerlist_storage> out = open(src_file, true);
    if (!out)
    {
      // if failed, try reading in unportable mode
      boost::filesystem::copy_file(path, path + ".unportable", boost::filesystem::copy_option::overwrite_if_exists);
      src_file.close();
      src_file.open( path , std::ios_base::binary | std::ios_base::in);
      if(src_file.fail())
        return boost::none;

      out = open(src_file, false);
      if (!out)
      {
        // This is different from the `return boost::none` cases above. Those
        // cases could fail due to bad file permissions, so a shutdown is
        // likely more appropriate.
        MWARNING("Failed to load p2p config file, falling back to default config");
        out.emplace();
      }
    }

    return out;
  }

  peerlist_storage::~peerlist_storage() noexcept
  {}

  bool peerlist_storage::store(std::ostream& dest, const peerlist_types& other) const
  {
    try
    {
      boost::archive::portable_binary_oarchive a{dest};
      const peerlist_join pj{std::cref(m_types), std::cref(other)};
      a << pj;
      return dest.good();
    }
    catch (const boost::archive::archive_exception& e)
    {}

    return false;
  }

  bool peerlist_storage::store(const std::string& path, const peerlist_types& other) const
  {
    std::ofstream dest_file{};
    dest_file.open( path , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if(dest_file.fail())
      return false;

    return store(dest_file, other);
  }

  peerlist_types peerlist_storage::take_zone(epee::net_utils::zone zone)
  {
    peerlist_types out{};
    out.white = do_take_zone(m_types.white, zone);
    out.gray = do_take_zone(m_types.gray, zone);
    out.anchor = do_take_zone(m_types.anchor, zone);
    return out;
  }

  bool peerlist_manager::init(peerlist_types&& peers, bool allow_local_ip)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);

    if (!m_peers_white.empty() || !m_peers_gray.empty() || !m_peers_anchor.empty())
      return false;

    add_peers(m_peers_white.get<by_addr>(), std::move(peers.white));
    add_peers(m_peers_gray.get<by_addr>(), std::move(peers.gray));
    add_peers(m_peers_anchor.get<by_addr>(), std::move(peers.anchor));
    m_allow_local_ip = allow_local_ip;
    return true;
  }

  void peerlist_manager::get_peerlist(std::vector<peerlist_entry>& pl_gray, std::vector<peerlist_entry>& pl_white)
  {
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    copy_peers(pl_gray, m_peers_gray.get<by_addr>());
    copy_peers(pl_white, m_peers_white.get<by_addr>());
  }

  void peerlist_manager::get_peerlist(peerlist_types& peers)
  { 
    CRITICAL_REGION_LOCAL(m_peerlist_lock);
    peers.white.reserve(peers.white.size() + m_peers_white.size());
    peers.gray.reserve(peers.gray.size() + m_peers_gray.size());
    peers.anchor.reserve(peers.anchor.size() + m_peers_anchor.size());

    copy_peers(peers.white, m_peers_white.get<by_addr>());
    copy_peers(peers.gray, m_peers_gray.get<by_addr>());
    copy_peers(peers.anchor, m_peers_anchor.get<by_addr>());
  }

  void peerlist_manager::evict_host_from_peerlist(bool use_white, const peerlist_entry& pr)
  {
    filter(use_white, [&pr](const peerlist_entry& pe){ return pe.adr.is_same_host(pr.adr); });
  }
}

BOOST_CLASS_VERSION(nodetool::peerlist_types, nodetool::CURRENT_PEERLIST_STORAGE_ARCHIVE_VER);
BOOST_CLASS_VERSION(nodetool::peerlist_join, nodetool::CURRENT_PEERLIST_STORAGE_ARCHIVE_VER);

