// Copyright (c) 2014, The Monero Project
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
using namespace epee;

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

  char empty_string = '\0';

  // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
  ub_ctx_resolvconf(m_data->m_ub_context, &empty_string);
  ub_ctx_hosts(m_data->m_ub_context, &empty_string);
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
