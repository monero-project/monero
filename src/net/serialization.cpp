// Copyright (c) 2021-2022, The Monero Project
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

#include "serialization.h"

#include <algorithm>
#include <boost/core/demangle.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include "net/i2p_address.h"
#include "net/enums.h"
#include "net/net_utils_base.h"
#include "net/tor_address.h"
#include "serialization/wire/adapted/asio.h"
#include "serialization/wire/adapted/static_vector.h"
#include "serialization/wire/epee.h"
#include "serialization/wire/error.h"

namespace net
{
  namespace
  {
    constexpr const std::size_t host_buffer_size =
      std::max(tor_address::buffer_size(), i2p_address::buffer_size());
    using host_buffer = boost::container::static_vector<char, host_buffer_size>;

    template<typename T>
    T check_address(expect<T> value)
    {
      if (value.has_error())
        WIRE_DLOG_THROW(wire::error::schema::string, "Host field is invalid format for " << boost::core::demangle(typeid(T).name()) << ": " << value.error().message());
      return *value;
    }

    template<typename T>
    T make_address_fields(const host_buffer& host, const std::optional<std::uint16_t> port)
    {
      if constexpr (std::is_same_v<T, net::i2p_address>)
        return check_address(net::i2p_address::make({host.data(), host.size()}));
      else if constexpr (!std::is_same_v<T, net::i2p_address>)
      {
        if (!port)
          WIRE_DLOG_THROW(wire::error::schema::missing_key, "Expected 'port' key for " << boost::core::demangle(typeid(T).name()));
        return check_address(T::make({host.data(), host.size()}, *port));
      }
    }

    template<typename T>
    T make_address_fields(const std::optional<host_buffer>& host, const std::optional<std::uint16_t> port)
    {
      if (!host)
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "Expected 'host' key for " << boost::core::demangle(typeid(T).name()));
      return make_address_fields<T>(*host, port);
    }

    template<typename T>
    T read_address(wire::epee_reader& source)
    {
      host_buffer host{};
      std::optional<std::uint16_t> port{};
      wire::object(source, wire::field("host", std::ref(host)), wire::optional_field("port", std::ref(port)));
      if (boost::string_ref{host.data(), host.size()} == T::unknown_str())
        return T::unknown();
      return make_address_fields<T>(host, port);
    }

    void write_address(wire::epee_writer& dest, const boost::string_ref host, const std::uint16_t port)
    {
      wire::object(dest, wire::field("host", host), wire::field("port", port));
    }
  } // anonymous

  void read_bytes(wire::epee_reader& source, i2p_address& dest)
  {
    dest = read_address<i2p_address>(source);
  }
  void write_bytes(wire::epee_writer& dest, const i2p_address& source)
  {
    write_address(dest, source.host_str(), source.port());
  }

  void read_bytes(wire::epee_reader& source, tor_address& dest)
  {
    dest = read_address<tor_address>(source);
  }
  void write_bytes(wire::epee_writer& dest, const tor_address& source)
  {
    write_address(dest, source.host_str(), source.port());
  }
} // net

namespace epee
{
namespace net_utils
{
  namespace
  {
    struct address_values
    {
      std::optional<net::host_buffer> host;
      std::optional<boost::asio::ip::address_v6> addr;
      std::optional<std::uint32_t> m_ip;
      std::optional<std::uint16_t> port;
      std::optional<std::uint16_t> m_port;
    };

    void read_bytes(wire::epee_reader& source, address_values& self)
    {
      wire::object(source,
        WIRE_OPTIONAL_FIELD(host),
        WIRE_OPTIONAL_FIELD(addr),
        WIRE_OPTIONAL_FIELD(m_ip),
        WIRE_OPTIONAL_FIELD(port),
        WIRE_OPTIONAL_FIELD(m_port)
      );
    }

    epee::net_utils::ipv4_network_address make_ipv4(const std::optional<std::uint32_t>& ip, const std::optional<std::uint16_t>& port)
    {
      if (!ip)
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "missing m_ip");
      if (!port)
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "missing m_port");
      return epee::net_utils::ipv4_network_address{SWAP32LE(*ip), *port};
    }

    epee::net_utils::ipv6_network_address make_ipv6(const std::optional<boost::asio::ip::address_v6>& ip, const std::optional<std::uint16_t>& port)
    {
      if (!ip)
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "missing m_ip");
      if (!port)
        WIRE_DLOG_THROW(wire::error::schema::missing_key, "missing m_port");
      return epee::net_utils::ipv6_network_address{*ip, *port};
    }
  } // anonymous

  //! read/write definitions for `epee::net_utils::address_type`
  WIRE_AS_INTEGER(address_type);

  void read_bytes(wire::epee_reader& source, network_address& dest)
  {
    address_type type{};
    address_values values{};
    wire::object(source, wire::field("type", std::ref(type)), wire::field("addr", std::ref(values)));

    switch (type)
    {
    case address_type::ipv4:
      dest = make_ipv4(values.m_ip, values.m_port);
      break;
    case address_type::ipv6:
      dest = make_ipv6(values.addr, values.m_port);
      break;
    case address_type::i2p:
      dest = net::make_address_fields<net::i2p_address>(values.host, values.port);
      break;
    case address_type::tor:
      dest = net::make_address_fields<net::tor_address>(values.host, values.port);
      break;
    case address_type::invalid:
      dest = network_address{};
      break;
    default:
      WIRE_DLOG_THROW(wire::error::schema::enumeration, "Unknown address_type type: " << std::uintmax_t(type));
    }
  }

  static void write_bytes(wire::epee_writer& dest, std::pair<const network_address&, address_type> source)
  {
    // write the fields for the type directly in the same "namespace" (legacy)
    switch (source.second)
    {
    case address_type::ipv4:
      wire_write::bytes(dest, source.first.as<ipv4_network_address>());
      break;
    case address_type::ipv6:
      wire_write::bytes(dest, source.first.as<ipv6_network_address>());
      break;
    case address_type::i2p:
      wire_write::bytes(dest, source.first.as<net::i2p_address>());
      break;
    case address_type::tor:
      wire_write::bytes(dest, source.first.as<net::tor_address>());
      break;
    default:
    case address_type::invalid:
      MERROR("Invalid network_address type: " << std::uintmax_t(source.second));
      break;
    }
  }

  void write_bytes_explicit(wire::epee_writer& dest, const network_address& source)
  {
    const auto type_id = source.get_type_id();
    wire::object(dest, wire::field("type", type_id), wire::field("addr", std::make_pair(std::ref(source), type_id)));
  }
} // net_utils
} // epee
