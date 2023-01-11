// Copyright (c) 2014-2022, The Monero Project
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

#include "common/dns_utils.h"
// check local first (in the event of static or in-source compilation of libunbound)
#include "unbound.h"

#include <deque>
#include <set>
#include <stdlib.h>
#include "include_base_utils.h"
#include "common/threadpool.h"
#include "crypto/crypto.h"
#include <boost/thread/mutex.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>
using namespace epee;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.dns"

static const char *DEFAULT_DNS_PUBLIC_ADDR[] =
{
  "194.150.168.168",    // CCC (Germany)
  "80.67.169.40",       // FDN (France)
  "89.233.43.71",       // http://censurfridns.dk (Denmark)
  "109.69.8.51",        // punCAT (Spain)
  "193.58.251.251",     // SkyDNS (Russia)
};

static boost::mutex instance_lock;

namespace
{

/*
 * The following two functions were taken from unbound-anchor.c, from
 * the unbound library packaged with this source.  The license and source
 * can be found in $PROJECT_ROOT/external/unbound
 */

/* Cert builtin commented out until it's used, as the compiler complains

// return the built in root update certificate
static const char*
get_builtin_cert(void)
{
	return
// The ICANN CA fetched at 24 Sep 2010.  Valid to 2028
"-----BEGIN CERTIFICATE-----\n"
"MIIDdzCCAl+gAwIBAgIBATANBgkqhkiG9w0BAQsFADBdMQ4wDAYDVQQKEwVJQ0FO\n"
"TjEmMCQGA1UECxMdSUNBTk4gQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkxFjAUBgNV\n"
"BAMTDUlDQU5OIFJvb3QgQ0ExCzAJBgNVBAYTAlVTMB4XDTA5MTIyMzA0MTkxMloX\n"
"DTI5MTIxODA0MTkxMlowXTEOMAwGA1UEChMFSUNBTk4xJjAkBgNVBAsTHUlDQU5O\n"
"IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MRYwFAYDVQQDEw1JQ0FOTiBSb290IENB\n"
"MQswCQYDVQQGEwJVUzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKDb\n"
"cLhPNNqc1NB+u+oVvOnJESofYS9qub0/PXagmgr37pNublVThIzyLPGCJ8gPms9S\n"
"G1TaKNIsMI7d+5IgMy3WyPEOECGIcfqEIktdR1YWfJufXcMReZwU4v/AdKzdOdfg\n"
"ONiwc6r70duEr1IiqPbVm5T05l1e6D+HkAvHGnf1LtOPGs4CHQdpIUcy2kauAEy2\n"
"paKcOcHASvbTHK7TbbvHGPB+7faAztABLoneErruEcumetcNfPMIjXKdv1V1E3C7\n"
"MSJKy+jAqqQJqjZoQGB0necZgUMiUv7JK1IPQRM2CXJllcyJrm9WFxY0c1KjBO29\n"
"iIKK69fcglKcBuFShUECAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8B\n"
"Af8EBAMCAf4wHQYDVR0OBBYEFLpS6UmDJIZSL8eZzfyNa2kITcBQMA0GCSqGSIb3\n"
"DQEBCwUAA4IBAQAP8emCogqHny2UYFqywEuhLys7R9UKmYY4suzGO4nkbgfPFMfH\n"
"6M+Zj6owwxlwueZt1j/IaCayoKU3QsrYYoDRolpILh+FPwx7wseUEV8ZKpWsoDoD\n"
"2JFbLg2cfB8u/OlE4RYmcxxFSmXBg0yQ8/IoQt/bxOcEEhhiQ168H2yE5rxJMt9h\n"
"15nu5JBSewrCkYqYYmaxyOC3WrVGfHZxVI7MpIFcGdvSb2a1uyuua8l0BKgk3ujF\n"
"0/wsHNeP22qNyVO+XVBzrM8fk8BSUFuiT/6tZTYXRtEt5aKQZgXbKU5dUF3jT9qg\n"
"j/Br5BZw3X/zd325TvnswzMC1+ljLzHnQGGk\n"
"-----END CERTIFICATE-----\n"
		;
}
*/

/** return the built in root DS trust anchor */
static const char* const*
get_builtin_ds(void)
{
  static const char * const ds[] =
  {
    ". IN DS 19036 8 2 49AAC11D7B6F6446702E54A1607371607A1A41855200FD2CE1CDDE32F24E8FB5\n",
    ". IN DS 20326 8 2 E06D44B80B8F1D39A95C0B0D7C65D08458E880409BBC683457104237C7F8EC8D\n",
    NULL
  };
  return ds;
}

/************************************************************
 ************************************************************
 ***********************************************************/

} // anonymous namespace

namespace tools
{

static const char *get_record_name(int record_type)
{
  switch (record_type)
  {
    case DNS_TYPE_A: return "A";
    case DNS_TYPE_TXT: return "TXT";
    case DNS_TYPE_AAAA: return "AAAA";
    case DNS_TYPE_TLSA: return "TLSA";
    default: return "unknown";
  }
}

// fuck it, I'm tired of dealing with getnameinfo()/inet_ntop/etc
boost::optional<std::string> ipv4_to_string(const char* src, size_t len)
{
  if (len < 4)
  {
    MERROR("Invalid IPv4 address: " << std::string(src, len));
    return boost::none;
  }

  std::stringstream ss;
  unsigned int bytes[4];
  for (int i = 0; i < 4; i++)
  {
    unsigned char a = src[i];
    bytes[i] = a;
  }
  ss << bytes[0] << "."
     << bytes[1] << "."
     << bytes[2] << "."
     << bytes[3];
  return ss.str();
}

// this obviously will need to change, but is here to reflect the above
// stop-gap measure and to make the tests pass at least...
boost::optional<std::string> ipv6_to_string(const char* src, size_t len)
{
  if (len < 8)
  {
    MERROR("Invalid IPv4 address: " << std::string(src, len));
    return boost::none;
  }

  std::stringstream ss;
  unsigned int bytes[8];
  for (int i = 0; i < 8; i++)
  {
    unsigned char a = src[i];
    bytes[i] = a;
  }
  ss << bytes[0] << ":"
     << bytes[1] << ":"
     << bytes[2] << ":"
     << bytes[3] << ":"
     << bytes[4] << ":"
     << bytes[5] << ":"
     << bytes[6] << ":"
     << bytes[7];
  return ss.str();
}

boost::optional<std::string> txt_to_string(const char* src, size_t len)
{
  if (len == 0)
    return boost::none;
  return std::string(src+1, len-1);
}

boost::optional<std::string> tlsa_to_string(const char* src, size_t len)
{
  if (len < 4)
    return boost::none;
  return std::string(src, len);
}

// custom smart pointer.
// TODO: see if std::auto_ptr and the like support custom destructors
template<typename type, void (*freefunc)(type*)>
class scoped_ptr
{
public:
  scoped_ptr():
    ptr(nullptr)
  {
  }
  scoped_ptr(type *p):
    ptr(p)
  {
  }
  ~scoped_ptr()
  {
    freefunc(ptr);
  }
  operator type *() { return ptr; }
  type **operator &() { return &ptr; }
  type *operator->() { return ptr; }
  operator const type*() const { return &ptr; }

private:
  type* ptr;
};

typedef class scoped_ptr<ub_result,ub_resolve_free> ub_result_ptr;

struct DNSResolverData
{
  ub_ctx* m_ub_context;
};

// work around for bug https://www.nlnetlabs.nl/bugs-script/show_bug.cgi?id=515 needed for it to compile on e.g. Debian 7
class string_copy {
public:
    string_copy(const char *s): str(strdup(s)) {}
    ~string_copy() { free(str); }
    operator char*() { return str; }

public:
    char *str;
};

static void add_anchors(ub_ctx *ctx)
{
  const char * const *ds = ::get_builtin_ds();
  while (*ds)
  {
    MINFO("adding trust anchor: " << *ds);
    ub_ctx_add_ta(ctx, string_copy(*ds++));
  }
}

DNSResolver::DNSResolver() : m_data(new DNSResolverData())
{
  int use_dns_public = 0;
  std::vector<std::string> dns_public_addr;
  const char *DNS_PUBLIC = getenv("DNS_PUBLIC");
  if (DNS_PUBLIC)
  {
    dns_public_addr = tools::dns_utils::parse_dns_public(DNS_PUBLIC);
    if (!dns_public_addr.empty())
    {
      MGINFO("Using public DNS server(s): " << boost::join(dns_public_addr, ", ") << " (TCP)");
      use_dns_public = 1;
    }
    else
    {
      MERROR("Failed to parse DNS_PUBLIC");
    }
  }

  // init libunbound context
  m_data->m_ub_context = ub_ctx_create();

  if (use_dns_public)
  {
    for (const auto &ip: dns_public_addr)
      ub_ctx_set_fwd(m_data->m_ub_context, string_copy(ip.c_str()));
    ub_ctx_set_option(m_data->m_ub_context, string_copy("do-udp:"), string_copy("no"));
    ub_ctx_set_option(m_data->m_ub_context, string_copy("do-tcp:"), string_copy("yes"));
  }
  else {
    // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
    ub_ctx_resolvconf(m_data->m_ub_context, NULL);
    ub_ctx_hosts(m_data->m_ub_context, NULL);
  }

  add_anchors(m_data->m_ub_context);

  if (!DNS_PUBLIC)
  {
    // if no DNS_PUBLIC specified, we try a lookup to what we know
    // should be a valid DNSSEC record, and switch to known good
    // DNSSEC resolvers if verification fails
    bool available, valid;
    static const char *probe_hostname = "updates.moneropulse.org";
    auto records = get_txt_record(probe_hostname, available, valid);
    if (!valid)
    {
      MINFO("Failed to verify DNSSEC record from " << probe_hostname << ", falling back to TCP with well known DNSSEC resolvers");
      ub_ctx_delete(m_data->m_ub_context);
      m_data->m_ub_context = ub_ctx_create();
      add_anchors(m_data->m_ub_context);
      for (const auto &ip: DEFAULT_DNS_PUBLIC_ADDR)
        ub_ctx_set_fwd(m_data->m_ub_context, string_copy(ip));
      ub_ctx_set_option(m_data->m_ub_context, string_copy("do-udp:"), string_copy("no"));
      ub_ctx_set_option(m_data->m_ub_context, string_copy("do-tcp:"), string_copy("yes"));
    }
  }
}

DNSResolver::~DNSResolver()
{
  if (m_data)
  {
    if (m_data->m_ub_context != NULL)
    {
      ub_ctx_delete(m_data->m_ub_context);
    }
    delete m_data;
  }
}

std::vector<std::string> DNSResolver::get_record(const std::string& url, int record_type, boost::optional<std::string> (*reader)(const char *,size_t), bool& dnssec_available, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  dnssec_available = false;
  dnssec_valid = false;

  // destructor takes care of cleanup
  ub_result_ptr result;

  MDEBUG("Performing DNSSEC " << get_record_name(record_type) << " record query for " << url);

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, string_copy(url.c_str()), record_type, DNS_CLASS_IN, &result))
  {
    dnssec_available = (result->secure || result->bogus);
    dnssec_valid = result->secure && !result->bogus;
    if (dnssec_available && !dnssec_valid)
      MWARNING("Invalid DNSSEC " << get_record_name(record_type) << " record signature for " << url << ": " << result->why_bogus);
    if (result->havedata)
    {
      for (size_t i=0; result->data[i] != NULL; i++)
      {
        boost::optional<std::string> res = (*reader)(result->data[i], result->len[i]);
        if (res)
        {
          // do not dump dns record directly from dns into log
          MINFO("Found " << get_record_name(record_type) << " record for " << url);
          addresses.push_back(std::move(*res));
        }
      }
    }
  }

  return addresses;
}

std::vector<std::string> DNSResolver::get_ipv4(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  return get_record(url, DNS_TYPE_A, ipv4_to_string, dnssec_available, dnssec_valid);
}

std::vector<std::string> DNSResolver::get_ipv6(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  return get_record(url, DNS_TYPE_AAAA, ipv6_to_string, dnssec_available, dnssec_valid);
}

std::vector<std::string> DNSResolver::get_txt_record(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  return get_record(url, DNS_TYPE_TXT, txt_to_string, dnssec_available, dnssec_valid);
}

std::vector<std::string> DNSResolver::get_tlsa_tcp_record(const boost::string_ref url, const boost::string_ref port, bool& dnssec_available, bool& dnssec_valid)
{
  std::string service_addr;
  service_addr.reserve(url.size() + port.size() + 7);
  service_addr.push_back('_');
  service_addr.append(port.data(), port.size());
  service_addr.append("._tcp.");
  service_addr.append(url.data(), url.size());
  return get_record(service_addr, DNS_TYPE_TLSA, tlsa_to_string, dnssec_available, dnssec_valid);
}

std::string DNSResolver::get_dns_format_from_oa_address(const std::string& oa_addr)
{
  std::string addr(oa_addr);
  auto first_at = addr.find("@");
  if (first_at == std::string::npos)
    return addr;

  // convert name@domain.tld to name.domain.tld
  addr.replace(first_at, 1, ".");

  return addr;
}

DNSResolver& DNSResolver::instance()
{
  boost::lock_guard<boost::mutex> lock(instance_lock);

  static DNSResolver staticInstance;
  return staticInstance;
}

DNSResolver DNSResolver::create()
{
  return DNSResolver();
}

namespace dns_utils
{

//-----------------------------------------------------------------------
// TODO: parse the string in a less stupid way, probably with regex
std::string address_from_txt_record(const std::string& s)
{
  // make sure the txt record has "oa1:xmr" and find it
  auto pos = s.find("oa1:xmr");
  if (pos == std::string::npos)
    return {};
  // search from there to find "recipient_address="
  pos = s.find("recipient_address=", pos);
  if (pos == std::string::npos)
    return {};
  pos += 18; // move past "recipient_address="
  // find the next semicolon
  auto pos2 = s.find(";", pos);
  if (pos2 != std::string::npos)
  {
    // length of address == 95, we can at least validate that much here
    if (pos2 - pos == 95)
    {
      return s.substr(pos, 95);
    }
    else if (pos2 - pos == 106) // length of address == 106 --> integrated address
    {
      return s.substr(pos, 106);
    }
  }
  return {};
}
/**
 * @brief gets a monero address from the TXT record of a DNS entry
 *
 * gets the monero address from the TXT record of the DNS entry associated
 * with <url>.  If this lookup fails, or the TXT record does not contain an
 * XMR address in the correct format, returns an empty string.  <dnssec_valid>
 * will be set true or false according to whether or not the DNS query passes
 * DNSSEC validation.
 *
 * @param url the url to look up
 * @param dnssec_valid return-by-reference for DNSSEC status of query
 *
 * @return a monero address (as a string) or an empty string
 */
std::vector<std::string> addresses_from_url(const std::string& url, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  // get txt records
  bool dnssec_available, dnssec_isvalid;
  std::string oa_addr = DNSResolver::instance().get_dns_format_from_oa_address(url);
  auto records = DNSResolver::instance().get_txt_record(oa_addr, dnssec_available, dnssec_isvalid);

  // TODO: update this to allow for conveying that dnssec was not available
  if (dnssec_available && dnssec_isvalid)
  {
    dnssec_valid = true;
  }
  else dnssec_valid = false;

  // for each txt record, try to find a monero address in it.
  for (auto& rec : records)
  {
    std::string addr = address_from_txt_record(rec);
    if (addr.size())
    {
      addresses.push_back(addr);
    }
  }
  return addresses;
}

std::string get_account_address_as_str_from_url(const std::string& url, bool& dnssec_valid, std::function<std::string(const std::string&, const std::vector<std::string>&, bool)> dns_confirm)
{
  // attempt to get address from dns query
  auto addresses = addresses_from_url(url, dnssec_valid);
  if (addresses.empty())
  {
    LOG_ERROR("wrong address: " << url);
    return {};
  }
  return dns_confirm(url, addresses, dnssec_valid);
}

bool load_txt_records_from_dns(std::vector<std::string> &good_records, const std::vector<std::string> &dns_urls)
{
  // Prevent infinite recursion when distributing
  if (dns_urls.empty()) return false;

  std::vector<std::set<std::string> > records;
  records.resize(dns_urls.size());

  size_t first_index = crypto::rand_idx(dns_urls.size());

  // send all requests in parallel
  std::deque<bool> avail(dns_urls.size(), false), valid(dns_urls.size(), false);
  tools::threadpool& tpool = tools::threadpool::getInstanceForIO();
  tools::threadpool::waiter waiter(tpool);
  for (size_t n = 0; n < dns_urls.size(); ++n)
  {
    tpool.submit(&waiter,[n, dns_urls, &records, &avail, &valid](){
       const auto res = tools::DNSResolver::instance().get_txt_record(dns_urls[n], avail[n], valid[n]);
       for (const auto &s: res)
         records[n].insert(s);
    });
  }
  waiter.wait();

  size_t cur_index = first_index;
  do
  {
    const std::string &url = dns_urls[cur_index];
    if (!avail[cur_index])
    {
      records[cur_index].clear();
      LOG_PRINT_L2("DNSSEC not available for hostname: " << url << ", skipping.");
    }
    if (!valid[cur_index])
    {
      records[cur_index].clear();
      LOG_PRINT_L2("DNSSEC validation failed for hostname: " << url << ", skipping.");
    }

    cur_index++;
    if (cur_index == dns_urls.size())
    {
      cur_index = 0;
    }
  } while (cur_index != first_index);

  size_t num_valid_records = 0;

  for( const auto& record_set : records)
  {
    if (record_set.size() != 0)
    {
      num_valid_records++;
    }
  }

  if (num_valid_records < 2)
  {
    LOG_PRINT_L0("WARNING: no two valid DNS TXT records were received");
    return false;
  }

  typedef std::map<std::set<std::string>, uint32_t> map_t;
  map_t record_count;
  for (const auto &e: records)
  {
    if (!e.empty())
      ++record_count[e];
  }

  map_t::const_iterator good_record = record_count.end();
  for (map_t::const_iterator i = record_count.begin(); i != record_count.end(); ++i)
  {
    if (good_record == record_count.end() || i->second > good_record->second)
      good_record = i;
  }

  MDEBUG("Found " << (good_record == record_count.end() ? 0 : good_record->second) << "/" << dns_urls.size() << " matching records from " << num_valid_records << " valid records");
  if (good_record == record_count.end() || good_record->second < dns_urls.size() / 2 + 1)
  {
    LOG_PRINT_L0("WARNING: no majority of DNS TXT records matched (only " << good_record->second << "/" << dns_urls.size() << ")");
    return false;
  }

  good_records = {};
  for (const auto &s: good_record->first)
    good_records.push_back(s);
  return true;
}

std::vector<std::string> parse_dns_public(const char *s)
{
  unsigned ip0, ip1, ip2, ip3;
  char c;
  std::vector<std::string> dns_public_addr;
  if (!strcmp(s, "tcp"))
  {
    for (size_t i = 0; i < sizeof(DEFAULT_DNS_PUBLIC_ADDR) / sizeof(DEFAULT_DNS_PUBLIC_ADDR[0]); ++i)
      dns_public_addr.push_back(DEFAULT_DNS_PUBLIC_ADDR[i]);
    LOG_PRINT_L0("Using default public DNS server(s): " << boost::join(dns_public_addr, ", ") << " (TCP)");
  }
  else if (sscanf(s, "tcp://%u.%u.%u.%u%c", &ip0, &ip1, &ip2, &ip3, &c) == 4)
  {
    if (ip0 > 255 || ip1 > 255 || ip2 > 255 || ip3 > 255)
    {
      MERROR("Invalid IP: " << s << ", using default");
    }
    else
    {
      dns_public_addr.push_back(std::string(s + strlen("tcp://")));
    }
  }
  else
  {
    MERROR("Invalid DNS_PUBLIC contents, ignored");
  }
  return dns_public_addr;
}

}  // namespace tools::dns_utils

}  // namespace tools
