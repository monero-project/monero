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

#include <cstring>

#include "common/expect.h"
#include "net/net_utils_base.h"
#include "net/tor_address.h"
#include "net/i2p_address.h"
#include "p2p/p2p_protocol_defs.h"

BOOST_CLASS_VERSION(nodetool::peerlist_entry, 3)

namespace boost
{
  namespace serialization
  {
    template <class T, class Archive>
    inline void do_serialize(boost::mpl::false_, Archive &a, epee::net_utils::network_address& na)
    {
      T addr{};
      a & addr;
      na = std::move(addr);
    }

    template <class T, class Archive>
    inline void do_serialize(boost::mpl::true_, Archive &a, const epee::net_utils::network_address& na)
    {
      a & na.as<T>();
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, epee::net_utils::network_address& na, const ver_type ver)
    {
      static constexpr const typename Archive::is_saving is_saving{};

      uint8_t type;
      if (is_saving)
        type = uint8_t(na.get_type_id());
      a & type;
      switch (epee::net_utils::address_type(type))
      {
        case epee::net_utils::ipv4_network_address::get_type_id():
          do_serialize<epee::net_utils::ipv4_network_address>(is_saving, a, na);
          break;
        case epee::net_utils::ipv6_network_address::get_type_id():
          do_serialize<epee::net_utils::ipv6_network_address>(is_saving, a, na);
          break;
        case net::tor_address::get_type_id():
          do_serialize<net::tor_address>(is_saving, a, na);
          break;
        case net::i2p_address::get_type_id():
          do_serialize<net::i2p_address>(is_saving, a, na);
          break;
        case epee::net_utils::address_type::invalid:
        default:
          throw std::runtime_error("Unsupported network address type");
      }
    }
    template <class Archive, class ver_type>
    inline void serialize(Archive &a, epee::net_utils::ipv4_network_address& na, const ver_type ver)
    {
      uint32_t ip{na.ip()};
      uint16_t port{na.port()};
      ip = SWAP32LE(ip);
      a & ip;
      ip = SWAP32LE(ip);
      a & port;
      if (!typename Archive::is_saving())
        na = epee::net_utils::ipv4_network_address{ip, port};
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, boost::asio::ip::address_v6& v6, const ver_type ver)
    {
      if (typename Archive::is_saving())
      {
        auto bytes = v6.to_bytes();
        for (auto &e: bytes) a & e;
      }
      else
      {
        boost::asio::ip::address_v6::bytes_type bytes;
        for (auto &e: bytes) a & e;
        v6 = boost::asio::ip::address_v6(bytes);
      }
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, epee::net_utils::ipv6_network_address& na, const ver_type ver)
    {
      boost::asio::ip::address_v6 ip{na.ip()};
      uint16_t port{na.port()};
      a & ip;
      a & port;
      if (!typename Archive::is_saving())
        na = epee::net_utils::ipv6_network_address{ip, port};
    }


    template <class Archive, class ver_type>
    inline void save(Archive& a, const net::tor_address& na, const ver_type)
    {
      const size_t length = std::strlen(na.host_str());
      if (length > 255)
        MONERO_THROW(net::error::invalid_tor_address, "Tor address too long");

      const uint16_t port{na.port()};
      const uint8_t len = length;
      a & port;
      a & len;
      a.save_binary(na.host_str(), length);
    }

    template <class Archive, class ver_type>
    inline void save(Archive& a, const net::i2p_address& na, const ver_type)
    {
      const size_t length = std::strlen(na.host_str());
      if (length > 255)
        MONERO_THROW(net::error::invalid_i2p_address, "i2p address too long");

      const uint16_t port{na.port()};
      const uint8_t len = length;
      a & port;
      a & len;
      a.save_binary(na.host_str(), length);
    }

    template <class Archive, class ver_type>
    inline void load(Archive& a, net::tor_address& na, const ver_type)
    {
      uint16_t port = 0;
      uint8_t length = 0;
      a & port;
      a & length;

      const size_t buffer_size = net::tor_address::buffer_size();
      if (length > buffer_size)
        MONERO_THROW(net::error::invalid_tor_address, "Tor address too long");

      char host[buffer_size] = {0};
      a.load_binary(host, length);
      host[sizeof(host) - 1] = 0;

      if (std::strcmp(host, net::tor_address::unknown_str()) == 0)
        na = net::tor_address::unknown();
      else
        na = MONERO_UNWRAP(net::tor_address::make(host, port));
    }

    template <class Archive, class ver_type>
    inline void load(Archive& a, net::i2p_address& na, const ver_type)
    {
      uint16_t port = 0;
      uint8_t length = 0;
      a & port;
      a & length;

      const size_t buffer_size = net::i2p_address::buffer_size();
      if (length > buffer_size)
        MONERO_THROW(net::error::invalid_i2p_address, "i2p address too long");

      char host[buffer_size] = {0};
      a.load_binary(host, length);
      host[sizeof(host) - 1] = 0;

      if (std::strcmp(host, net::i2p_address::unknown_str()) == 0)
        na = net::i2p_address::unknown();
      else
        na = MONERO_UNWRAP(net::i2p_address::make(host, port));
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, net::tor_address& na, const ver_type ver)
    {
      boost::serialization::split_free(a, na, ver);
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, net::i2p_address& na, const ver_type ver)
    {
      boost::serialization::split_free(a, na, ver);
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a,  nodetool::peerlist_entry& pl, const ver_type ver)
    {
      a & pl.adr;
      a & pl.id;
      a & pl.last_seen;
      if (ver < 1)
      {
        if (!typename Archive::is_saving())
          pl.pruning_seed = 0;
        return;
      }
      a & pl.pruning_seed;
      if (ver < 2)
      {
        if (!typename Archive::is_saving())
          pl.rpc_port = 0;
        return;
      }
      a & pl.rpc_port;
      if (ver < 3)
      {
        if (!typename Archive::is_saving())
          pl.rpc_credits_per_hash = 0;
        return;
      }
      a & pl.rpc_credits_per_hash;
    }

    template <class Archive, class ver_type>
    inline void serialize(Archive &a, nodetool::anchor_peerlist_entry& pl, const ver_type ver)
    {
      a & pl.adr;
      a & pl.id;
      a & pl.first_seen;
    }
  }
}
