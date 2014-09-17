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
#include <ldns/rr.h> // for RR type and class defs
#include <unbound.h>
#include <arpa/inet.h> // for inet_ntoa (bytes to text for IPs)

namespace tools
{

struct DNSResolverData
{
  ub_ctx* m_ub_context;
};

DNSResolver::DNSResolver() : m_data(new DNSResolverData())
{
  // init libunbound context
  m_data->m_ub_context = ub_ctx_create();

  // look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
  ub_ctx_resolvconf(m_data->m_ub_context, "");
  ub_ctx_hosts(m_data->m_ub_context, "");
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

std::vector<std::string> DNSResolver::get_ipv4(const std::string& url)
{
  ub_result* result = NULL;
  std::vector<std::string> retval;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, url.c_str(), LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN, &result))
  {
    if (result->havedata)
    {
      for (int i=0; result->data[i] != NULL; i++)
      {
        char as_str[INET_ADDRSTRLEN];

        // convert bytes to string, append if no error
        if (inet_ntop(AF_INET, result->data[i], as_str, sizeof(as_str)))
        {
          retval.push_back(as_str);
        }
      }
    }
  }

  // cleanup
  ub_resolve_free(result);
  return retval;
}

std::vector<std::string> DNSResolver::get_ipv6(const std::string& url)
{
  ub_result* result = NULL;
  std::vector<std::string> retval;

  // call DNS resolver, blocking.  if return value not zero, something went wrong
  if (!ub_resolve(m_data->m_ub_context, url.c_str(), LDNS_RR_TYPE_AAAA, LDNS_RR_CLASS_IN, &result))
  {
    if (result->havedata)
    {
      for (int i=0; result->data[i] != NULL; i++)
      {
        char as_str[INET6_ADDRSTRLEN];

        // convert bytes to string, append if no error
        if (inet_ntop(AF_INET6, result->data[i], as_str, sizeof(as_str)))
        {
          retval.push_back(as_str);
        }
      }
    }
  }

  // cleanup
  ub_resolve_free(result);
  return retval;
}

std::string DNSResolver::get_payment_address(const std::string& url)
{
  std::string retval;
  return retval;
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

}  // namespace tools
