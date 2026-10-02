// Copyright (c) 2026, The Monero Project
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

#include <string_view>

#include "include_base_utils.h"
#include "net/tor_address.h"
#include "net/i2p_address.h"
#include "net/parse.h"
#include "fuzzer.h"

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  const std::string input((const char*)buf, len);

  // Onion addresses (Tor)
  const auto tor = net::tor_address::make(input);
  if (tor)
  {
    const std::string rendered = tor->str();
    net::tor_address::make(rendered);
  }

  const auto onion_decoded = net::from_onion_v3(std::string_view{input});
  net::validate_v3_onion_checksum(onion_decoded);

  // I2P addresses
  const auto i2p = net::i2p_address::make(input);
  if (i2p)
  {
    const std::string rendered = i2p->str();
    net::i2p_address::make(rendered);
  }

  // Generic host/port splitting, used for IPv4, IPv6, Tor, I2P, and hostnames
  std::string host;
  std::string port;
  net::get_network_address_host_and_port(input, host, port);

  // Generic address parsing (IPv4, IPv6, Tor, I2P)
  net::get_network_address(input, 18080);

  // IPv4 CIDR subnet parsing
  net::get_ipv4_subnet_address(input, true);
  net::get_ipv4_subnet_address(input, false);

  // TODO add IPv6 CIDR subnet parsing

  // TCP endpoint resolution (IPv4/IPv6 host/port)
  net::get_tcp_endpoint(input);

  // URI parsing (scheme, userinfo, host/port) and SOCKS endpoint parsing
  net::uri_components::get(input);
  net::socks::endpoint::get(input);
END_SIMPLE_FUZZER()
