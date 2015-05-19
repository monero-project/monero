// Copyright (c) 2014-2015, The Monero Project
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
std::string ipv4_to_string(const char* src)
{
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
std::string ipv6_to_string(const char* src)
{
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

// custom smart pointer.
// TODO: see if std::auto_ptr and the like support custom destructors
class ub_result_ptr
{
public:
  ub_result_ptr()
  {
    ptr = nullptr;
  }
  ~ub_result_ptr()
  {
    ub_resolve_free(ptr);
  }
  ub_result* ptr;
};

struct DNSResolverData
{
  ub_ctx* m_ub_context;
};

DNSResolver::DNSResolver() : m_data(new DNSResolverData())
{
  // init libunbound context
  m_data->m_ub_context = ub_ctx_create();

  // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
  ub_ctx_resolvconf(m_data->m_ub_context, NULL);
  ub_ctx_hosts(m_data->m_ub_context, NULL);

	#ifdef DEVELOPER_LIBUNBOUND_OLD
		#pragma message "Using the work around for old libunbound"
		{ // work around for bug https://www.nlnetlabs.nl/bugs-script/show_bug.cgi?id=515 needed for it to compile on e.g. Debian 7
			char * ds_copy = NULL; // this will be the writable copy of string that bugged version of libunbound requires
			try {
				char * ds_copy = strdup( ::get_builtin_ds() );
				ub_ctx_add_ta(m_data->m_ub_context, ds_copy);
			} catch(...) { // probably not needed but to work correctly in every case...
				if (ds_copy) { free(ds_copy); ds_copy=NULL; } // for the strdup
				throw ;
			}
			if (ds_copy) { free(ds_copy); ds_copy=NULL; } // for the strdup
		}
	#else
		// normal version for fixed libunbound
		ub_ctx_add_ta(m_data->m_ub_context, ::get_builtin_ds() );
	#endif

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

std::vector<std::string> DNSResolver::get_ipv4(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  dnssec_available = false;
  dnssec_valid = false;
  char urlC[1000];  // waaaay too big, but just in case...

  strncpy(urlC, url.c_str(), 999);
  urlC[999] = '\0';
  if (!check_address_syntax(urlC))
  {
    return addresses;
  }

  // destructor takes care of cleanup
  ub_result_ptr result;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, urlC, DNS_TYPE_A, DNS_CLASS_IN, &(result.ptr)))
  {
    dnssec_available = (result.ptr->secure || (!result.ptr->secure && result.ptr->bogus));
    dnssec_valid = !result.ptr->bogus;
    if (result.ptr->havedata)
    {
      for (size_t i=0; result.ptr->data[i] != NULL; i++)
      {
        addresses.push_back(ipv4_to_string(result.ptr->data[i]));
      }
    }
  }

  return addresses;
}

std::vector<std::string> DNSResolver::get_ipv6(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  std::vector<std::string> addresses;
  dnssec_available = false;
  dnssec_valid = false;
  char urlC[1000];  // waaaay too big, but just in case...

  strncpy(urlC, url.c_str(), 999);
  urlC[999] = '\0';

  if (!check_address_syntax(urlC))
  {
    return addresses;
  }

  ub_result_ptr result;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, urlC, DNS_TYPE_AAAA, DNS_CLASS_IN, &(result.ptr)))
  {
    dnssec_available = (result.ptr->secure || (!result.ptr->secure && result.ptr->bogus));
    dnssec_valid = !result.ptr->bogus;
    if (result.ptr->havedata)
    {
      for (size_t i=0; result.ptr->data[i] != NULL; i++)
      {
        addresses.push_back(ipv6_to_string(result.ptr->data[i]));
      }
    }
  }

  return addresses;
}

std::vector<std::string> DNSResolver::get_txt_record(const std::string& url, bool& dnssec_available, bool& dnssec_valid)
{
  std::vector<std::string> records;
  dnssec_available = false;
  dnssec_valid = false;
  char urlC[1000];  // waaaay too big, but just in case...

  strncpy(urlC, url.c_str(), 999);
  urlC[999] = '\0';

  if (!check_address_syntax(urlC))
  {
    return records;
  }

  ub_result_ptr result;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, urlC, DNS_TYPE_TXT, DNS_CLASS_IN, &(result.ptr)))
  {
    dnssec_available = (result.ptr->secure || (!result.ptr->secure && result.ptr->bogus));
    dnssec_valid = !result.ptr->bogus;
    if (result.ptr->havedata)
    {
      for (size_t i=0; result.ptr->data[i] != NULL; i++)
      {
      	// plz fix this, but this does NOT work and spills over into parts of memory it shouldn't: records.push_back(result.ptr->data[i]);
        char *restxt;
        restxt = (char*) calloc(result.ptr->len[i]+1, 1);
        memcpy(restxt, result.ptr->data[i]+1, result.ptr->len[i]-1);
        records.push_back(restxt);
      }
    }
  }

  return records;
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
  static DNSResolver* staticInstance = NULL;
  if (staticInstance == NULL)
  {
    staticInstance = new DNSResolver();
  }
  return *staticInstance;
}

bool DNSResolver::check_address_syntax(const std::string& addr)
{
  // if string doesn't contain a dot, we won't consider it a url for now.
  if (addr.find(".") == std::string::npos)
  {
    return false;
  }
  return true;
}

}  // namespace tools
