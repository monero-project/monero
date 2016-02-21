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

#include "common/dns_utils.h"
#include <cstring>
#include <sstream>
// check local first (in the event of static or in-source compilation of libunbound)
#include "unbound.h"

#include <stdlib.h>
#include "include_base_utils.h"
#include <boost/filesystem/fstream.hpp>
using namespace epee;
namespace bf = boost::filesystem;

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
static const char*
get_builtin_ds(void)
{
  return
". IN DS 19036 8 2 49AAC11D7B6F6446702E54A1607371607A1A41855200FD2CE1CDDE32F24E8FB5\n";
}

/************************************************************
 ************************************************************
 ***********************************************************/

} // anonymous namespace

namespace tools
{

// fuck it, I'm tired of dealing with getnameinfo()/inet_ntop/etc
std::string ipv4_to_string(const char* src, size_t len)
{
  assert(len >= 4);

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
std::string ipv6_to_string(const char* src, size_t len)
{
  assert(len >= 8);

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

std::string txt_to_string(const char* src, size_t len)
{
  return std::string(src+1, len-1);
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

DNSResolver::DNSResolver() : m_data(new DNSResolverData())
{
  int use_dns_public = 0;
  const char* dns_public_addr = "8.8.4.4";
  if (auto res = getenv("DNS_PUBLIC"))
  {
    std::string dns_public(res);
    // TODO: could allow parsing of IP and protocol: e.g. DNS_PUBLIC=tcp:8.8.8.8
    if (dns_public == "tcp")
    {
      LOG_PRINT_L0("Using public DNS server: " << dns_public_addr << " (TCP)");
      use_dns_public = 1;
    }
  }

  // init libunbound context
  m_data->m_ub_context = ub_ctx_create();

  if (use_dns_public)
  {
    ub_ctx_set_fwd(m_data->m_ub_context, string_copy(dns_public_addr));
    ub_ctx_set_option(m_data->m_ub_context, string_copy("do-udp:"), string_copy("no"));
    ub_ctx_set_option(m_data->m_ub_context, string_copy("do-tcp:"), string_copy("yes"));
  }
  else {
    // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
    ub_ctx_resolvconf(m_data->m_ub_context, NULL);
    ub_ctx_hosts(m_data->m_ub_context, NULL);
  }

  ub_ctx_add_ta(m_data->m_ub_context, string_copy(::get_builtin_ds()));
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

std::vector<std::string> DNSResolver::get_record(const std::string& url, int record_type, std::string (*reader)(const char *,size_t), bool& dnssec_available, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  dnssec_available = false;
  dnssec_valid = false;

  if (!check_address_syntax(url.c_str()))
  {
    return addresses;
  }

  // destructor takes care of cleanup
  ub_result_ptr result;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, string_copy(url.c_str()), record_type, DNS_CLASS_IN, &result))
  {
    dnssec_available = (result->secure || (!result->secure && result->bogus));
    dnssec_valid = result->secure && !result->bogus;
    if (result->havedata)
    {
      for (size_t i=0; result->data[i] != NULL; i++)
      {
        addresses.push_back((*reader)(result->data[i], result->len[i]));
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

  static DNSResolver* staticInstance = NULL;
  if (staticInstance == NULL)
  {
    staticInstance = new DNSResolver();
  }
  return *staticInstance;
}

DNSResolver DNSResolver::create()
{
  return DNSResolver();
}

bool DNSResolver::check_address_syntax(const char *addr) const
{
  // if string doesn't contain a dot, we won't consider it a url for now.
  if (strchr(addr,'.') == NULL)
  {
    return false;
  }
  return true;
}

}  // namespace tools
