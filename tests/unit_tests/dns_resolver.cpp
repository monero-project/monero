#include "gtest/gtest.h"

#include "common/dns_utils.h"

TEST(DNSResolver, IPv4Success)
{
  tools::DNSResolver resolver;

  auto ips = resolver.get_ipv4("example.com");

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("93.184.216.119", ips[0].c_str());

  ips = tools::DNSResolver::instance().get_ipv4("example.com");

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("93.184.216.119", ips[0].c_str());
}

TEST(DNSResolver, IPv4Failure)
{
  // guaranteed by IANA/ICANN/RFC to be invalid
  tools::DNSResolver resolver;

  auto ips = resolver.get_ipv4("example.invalid");

  ASSERT_EQ(0, ips.size());

  ips = tools::DNSResolver::instance().get_ipv4("example.invalid");

  ASSERT_EQ(0, ips.size());
}

TEST(DNSResolver, IPv6Success)
{
  tools::DNSResolver resolver;

  auto ips = resolver.get_ipv6("example.com");

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("2606:2800:220:6d:26bf:1447:1097:aa7", ips[0].c_str());

  ips = tools::DNSResolver::instance().get_ipv6("example.com");

  ASSERT_EQ(1, ips.size());

  ASSERT_STREQ("2606:2800:220:6d:26bf:1447:1097:aa7", ips[0].c_str());
}

TEST(DNSResolver, IPv6Failure)
{
  // guaranteed by IANA/ICANN/RFC to be invalid
  tools::DNSResolver resolver;

  auto ips = resolver.get_ipv6("example.invalid");

  ASSERT_EQ(0, ips.size());

  ips = tools::DNSResolver::instance().get_ipv6("example.invalid");

  ASSERT_EQ(0, ips.size());
}
