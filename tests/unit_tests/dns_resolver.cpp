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

#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "common/dns_utils.h"

TEST(DNSResolver, IPv4Success)
{
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv4("example.com", avail, valid);

  ASSERT_LE(1, ips.size());

  ips = tools::DNSResolver::instance().get_ipv4("example.com", avail, valid);

  ASSERT_LE(1, ips.size());
}

TEST(DNSResolver, IPv4Failure)
{
  // guaranteed by IANA/ICANN/RFC to be invalid
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv4("example.invalid", avail, valid);

  ASSERT_EQ(0, ips.size());

  ips = tools::DNSResolver::instance().get_ipv4("example.invalid", avail, valid);

  ASSERT_EQ(0, ips.size());
}

TEST(DNSResolver, DNSSECSuccess)
{
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv4("example.com", avail, valid);

  ASSERT_LE(1, ips.size());

  ASSERT_TRUE(avail);
  ASSERT_TRUE(valid);
}

TEST(DNSResolver, DNSSECFailure)
{
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv4("dnssec-failed.org", avail, valid);

  ASSERT_EQ(1, ips.size());

  //ASSERT_STREQ("93.184.216.119", ips[0].c_str());

  ASSERT_TRUE(avail);
  ASSERT_FALSE(valid);
}

// It would be great to include an IPv6 test and assume it'll pass, but not every ISP / resolver plays nicely with IPv6;)
/*TEST(DNSResolver, IPv6Success)
{
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv6("example.com", avail, valid);

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("2606:2800:220:6d:26bf:1447:1097:aa7", ips[0].c_str());

  ips = tools::DNSResolver::instance().get_ipv6("example.com", avail, valid);

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("2606:2800:220:6d:26bf:1447:1097:aa7", ips[0].c_str());
}*/

TEST(DNSResolver, IPv6Failure)
{
  // guaranteed by IANA/ICANN/RFC to be invalid
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;

  auto ips = resolver.get_ipv6("example.invalid", avail, valid);

  ASSERT_EQ(0, ips.size());

  ips = tools::DNSResolver::instance().get_ipv6("example.invalid", avail, valid);

  ASSERT_EQ(0, ips.size());
}

TEST(DNSResolver, GetTXTRecord)
{
  bool avail, valid;

  std::vector<std::string> records = tools::DNSResolver::instance().get_txt_record("donate.getmonero.org", avail, valid);

  EXPECT_NE(0, records.size());

  for (auto& rec : records)
  {
    std::cout << "TXT record for donate.getmonero.org: " << rec << std::endl;
  }

  // replace first @ with .
  std::string addr = tools::DNSResolver::instance().get_dns_format_from_oa_address("donate@getmonero.org");
  EXPECT_STREQ("donate.getmonero.org", addr.c_str());

  // no change
  addr = tools::DNSResolver::instance().get_dns_format_from_oa_address("donate.getmonero.org");
  EXPECT_STREQ("donate.getmonero.org", addr.c_str());
}

TEST(DNSResolver, Localhost)
{
  tools::DNSResolver resolver = tools::DNSResolver::create();

  bool avail, valid;
  std::vector<std::string> ips = resolver.get_ipv4("localhost", avail, valid);

  ASSERT_EQ(1, ips.size());
  ASSERT_EQ("127.0.0.1", ips[0]);
}

bool is_equal(const char *s, const std::vector<std::string> &v) { return v.size() == 1 && v[0] == s; }

TEST(DNS_PUBLIC, empty) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("").empty()); }
TEST(DNS_PUBLIC, default) { EXPECT_GT(tools::dns_utils::parse_dns_public("tcp").size(), 1); }
TEST(DNS_PUBLIC, invalid_scheme) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("invalid").empty()); }
TEST(DNS_PUBLIC, invalid_ip_alpha) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://invalid").empty()); }
TEST(DNS_PUBLIC, invalid_ip_num1) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3").empty()); }
TEST(DNS_PUBLIC, invalid_ip_num3) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5").empty()); }
TEST(DNS_PUBLIC, invalid_ip_num4_extra) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6x").empty()); }
TEST(DNS_PUBLIC, invalid_ip_num4_range) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.542.6").empty()); }
TEST(DNS_PUBLIC, invalid_ip_dot) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6.").empty()); }
TEST(DNS_PUBLIC, invalid_ip_num5) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6.7").empty()); }
TEST(DNS_PUBLIC, invalid_ip_4_missing) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4..7").empty()); }
TEST(DNS_PUBLIC, valid_ip_lo) { EXPECT_TRUE(is_equal("127.0.0.1", tools::dns_utils::parse_dns_public("tcp://127.0.0.1"))); }
TEST(DNS_PUBLIC, valid_ip) { EXPECT_TRUE(is_equal("3.4.5.6", tools::dns_utils::parse_dns_public("tcp://3.4.5.6"))); }
TEST(DNS_PUBLIC, valid_ip_port) { EXPECT_TRUE(is_equal("3.4.5.6@5353", tools::dns_utils::parse_dns_public("tcp://3.4.5.6:5353"))); }
TEST(DNS_PUBLIC, valid_ip_port_max) { EXPECT_TRUE(is_equal("3.4.5.6@65535", tools::dns_utils::parse_dns_public("tcp://3.4.5.6:65535"))); }
TEST(DNS_PUBLIC, invalid_ip_with_port) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.542.6:53").empty()); }
TEST(DNS_PUBLIC, invalid_port_zero) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6:0").empty()); }
TEST(DNS_PUBLIC, invalid_port_too_large) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6:65536").empty()); }
TEST(DNS_PUBLIC, invalid_port_alpha) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6:abc").empty()); }
TEST(DNS_PUBLIC, invalid_port_empty) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6:").empty()); }
TEST(DNS_PUBLIC, invalid_port_extra) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://3.4.5.6:53x").empty()); }
TEST(DNS_PUBLIC, invalid_ip_leading_zero_port) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tcp://127.0.0.01:5353").empty()); }

TEST(DNS_PUBLIC, default_tls) { bool tls = false; EXPECT_GT(tools::dns_utils::parse_dns_public("tls", &tls).size(), 1); EXPECT_TRUE(tls); }
TEST(DNS_PUBLIC, default_tcp_not_tls) { bool tls = true; EXPECT_GT(tools::dns_utils::parse_dns_public("tcp", &tls).size(), 1); EXPECT_FALSE(tls); }
TEST(DNS_PUBLIC, invalid_tls_ip_alpha) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://invalid").empty()); }
TEST(DNS_PUBLIC, invalid_tls_ip_num1) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3").empty()); }
TEST(DNS_PUBLIC, invalid_tls_ip_num3) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5").empty()); }
TEST(DNS_PUBLIC, invalid_tls_ip_num4_extra) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6x").empty()); }
TEST(DNS_PUBLIC, invalid_tls_ip_num4_range) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.542.6").empty()); }
TEST(DNS_PUBLIC, invalid_tls_ip_num5) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6.7").empty()); }
TEST(DNS_PUBLIC, invalid_tls_port_zero) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6:0").empty()); }
TEST(DNS_PUBLIC, invalid_tls_port_range) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6:65536").empty()); }
TEST(DNS_PUBLIC, invalid_tls_port_alpha) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6:x").empty()); }
TEST(DNS_PUBLIC, invalid_tls_port_extra) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6:853x").empty()); }
TEST(DNS_PUBLIC, invalid_tls_auth_empty) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6#").empty()); }
TEST(DNS_PUBLIC, invalid_tls_auth_char) { EXPECT_TRUE(tools::dns_utils::parse_dns_public("tls://3.4.5.6#bad!name").empty()); }
TEST(DNS_PUBLIC, valid_tls_ip) { bool tls = false; EXPECT_TRUE(is_equal("3.4.5.6@853", tools::dns_utils::parse_dns_public("tls://3.4.5.6", &tls))); EXPECT_TRUE(tls); }
TEST(DNS_PUBLIC, valid_tls_ip_port) { EXPECT_TRUE(is_equal("3.4.5.6@443", tools::dns_utils::parse_dns_public("tls://3.4.5.6:443"))); }
TEST(DNS_PUBLIC, valid_tls_ip_auth) { EXPECT_TRUE(is_equal("3.4.5.6@853#dns.quad9.net", tools::dns_utils::parse_dns_public("tls://3.4.5.6#dns.quad9.net"))); }
TEST(DNS_PUBLIC, valid_tls_ip_port_auth) { EXPECT_TRUE(is_equal("3.4.5.6@443#dns.quad9.net", tools::dns_utils::parse_dns_public("tls://3.4.5.6:443#dns.quad9.net"))); }
