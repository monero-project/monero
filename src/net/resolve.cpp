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

#include "net/resolve.h"

#include <boost/utility/string_ref.hpp>
#include "common/dns_utils.h"
#include "common/expect.h"
#include "net/error.h"

namespace net
{
namespace dnssec
{
  expect<service_response> resolve_hostname(const std::string& addr, const std::string& tlsa_port)
  {
    // use basic (blocking) unbound for now, possibly refactor later
    tools::DNSResolver& resolver = tools::DNSResolver::instance();

    bool dnssec_available = false;
    bool dnssec_valid = false;
    std::vector<std::string> ip_records = resolver.get_ipv4(addr, dnssec_available, dnssec_valid);

    if (dnssec_available && !dnssec_valid)
      return {net::error::bogus_dnssec};

    if (ip_records.empty())
    {
      ip_records = resolver.get_ipv6(addr, dnssec_available, dnssec_valid);
      if (dnssec_available && !dnssec_valid)
        return {net::error::bogus_dnssec};
      if (ip_records.empty())
        return {net::error::dns_query_failure};
    }

    std::vector<std::string> tlsa{};
    if (dnssec_available && !tlsa_port.empty())
    {
      tlsa = resolver.get_tlsa_tcp_record(addr, tlsa_port, dnssec_available, dnssec_valid);
      if (!dnssec_valid)
        return {net::error::bogus_dnssec};
    }
    return {{std::move(ip_records), std::move(tlsa)}};
  }
} // dnssec
} // net
