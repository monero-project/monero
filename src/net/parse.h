// Copyright (c) 2018-2019, The Monero Project
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

#include <boost/utility/string_ref.hpp>
#include <cstdint>

#include "common/expect.h"
#include "net/net_utils_base.h"

#if defined(SEKRETA)
#include "net/sekreta.h"
#endif

namespace net
{
    void get_network_address_host_and_port(const std::string& address, std::string& host, std::string& port);

    /*!
      Identifies onion, i2p and IPv4 addresses and returns them as a generic
      `network_address`. If the type is unsupported, it might be a hostname,
      and `error() == net::error::kUnsupportedAddress` is returned.

      \param address An onion address, i2p address, ipv4 address or hostname. Hostname
          will return an error.
      \param default_port If `address` does not specify a port, this value
          will be used.

      \return A tor or IPv4 address, else error.
    */
    expect<epee::net_utils::network_address>
        get_network_address(boost::string_ref address, std::uint16_t default_port);

    /*!
      Identifies an IPv4 subnet in CIDR notatioa and returns it as a generic
      `network_address`. If the type is unsupported, it might be a hostname,
      and `error() == net::error::kUnsupportedAddress` is returned.

      \param address An ipv4 address.
      \param allow_implicit_32 whether to accept "raw" IPv4 addresses, with CIDR notation

      \return A tor or IPv4 address, else error.
    */
    expect<epee::net_utils::ipv4_network_subnet>
        get_ipv4_subnet_address(boost::string_ref address, bool allow_implicit_32 = false);
}

#if defined(SEKRETA)
namespace net::sekreta
{
//! \brief CLI system argument parser for Sekreta
//!
//! \details Offers extensibility and future-proofing for arg-caller
//!   implementations
//!
//! \tparam t_address Returned daemon address type
//! \tparam t_value Interface value type
//!
//! \param daemon Pair: daemon host address and port
//! \param systems Requested system type to parse
//!
//! \note Currently limited to parsing for a single-system implementation and
//!   for a single address
//!
//! \note Sekreta's impl helper design can serve as a framework-replacement for
//!   CN copypasta
//!
//! \todo epee + serialization needs std::string_view support
//!
//! \todo network address parsing (any anon network parsing) should be used
//!   through Sekreta when implemented
template <
    typename t_address = expect<epee::net_utils::network_address>,
    typename t_value = std::string>
std::pair<std::optional<t_address>, std::optional<t_value>> parse_cli_arg(
    const std::pair<std::string, uint16_t>& daemon,
    const std::vector<t_value>& systems)
{
  namespace impl = ::sekreta::api::impl_helper;
  namespace type = impl::type;

  using t_args = impl::Args<t_value>;

  auto const cli_error = [&](const type::kError error) {
    return std::make_pair(
        std::nullopt, impl::args_error<t_args, t_value>(error).second);
  };

  bool is_garlic{false}, is_onion{false}, is_loki{false};
  for (const auto& system : systems)
    {
      std::optional<type::kSystem> const arg =
          t_args::template get_key<type::kSystem>(system);

      if (!arg)
        return cli_error(type::kError::System);

      // TODO(unassigned): see below regarding SEK-key for more networks
      switch (arg.value())
        {
          case type::kSystem::Kovri:
            //case type::kSystem::Ire:
            //case type::kSystem::I2P:
            is_garlic = true;
            break;
          //case type::kSystem::Tor:
          //  is_onion = true;
          //  break;
          //case type::kSystem::Loki:
          //  is_loki = true;
          //  break;
          default:
            // Prevents lack of expected implementation
            return cli_error(type::kError::System);
        }
    }

  // TODO(anonimal): SEK-key support for daemon-address will allow multiple daemons/networks
  // TODO(unassigned): get_network_address does not allow for addressbook-based garlic addresses
  t_address const address =
      net::get_network_address(daemon.first, daemon.second);

  if (!address)
    return cli_error(type::kError::Address);

  // TODO(anonimal): Sekreta should provide checks/info for all address types
  // TODO(anonimal): when multiple daemon-addresses, do this better
  bool is_valid_address{false};
  using t_zone = epee::net_utils::zone;
  switch (address->get_zone())
    {
      // TODO(anonimal): implementation-specific zones: not all garlics are alike.
      case t_zone::i2p:
        {
          if (is_garlic)
            is_valid_address = true;
        }
        break;
      case t_zone::tor:
        {
          if (is_onion)
            is_valid_address = true;
        }
        break;
      // TODO(unassigned): support this zone
      //case t_zone::loki:
      //  {
      //    if (is_loki)
      //      is_valid_address = true;
      //  }
      //  break;
      default:
        return cli_error(type::kError::Network);
    }

  if (!is_valid_address)
    return cli_error(type::kError::Address);

  return {address, std::nullopt};
}
}  // namespace net::sekreta
#endif
