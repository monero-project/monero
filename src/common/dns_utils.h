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

#include <vector>
#include <string>

namespace tools
{

struct DNSResolverData;

/**
 * @brief Provides high-level access to DNS resolution
 *
 * This class is designed to provide a high-level abstraction to DNS resolution
 * functionality, including access to TXT records and such.  It will also
 * handle DNSSEC validation of the results.
 */
class DNSResolver
{
public:

  /**
   * @brief Constructs an instance of DNSResolver
   *
   * Constructs a class instance and does setup stuff for the backend resolver.
   */
  DNSResolver();

  /**
   * @brief takes care of freeing C pointers and such
   */
  ~DNSResolver();

  /**
   * @brief gets ipv4 addresses from DNS query of a URL
   *
   * returns a vector of all IPv4 "A" records for given URL.
   * If no "A" records found, returns an empty vector.
   *
   * @param url A string containing a URL to query for
   *
   * @return vector of strings containing ipv4 addresses
   */
  std::vector<std::string> get_ipv4(const std::string& url);

  /**
   * @brief gets ipv6 addresses from DNS query
   *
   * returns a vector of all IPv6 "A" records for given URL.
   * If no "A" records found, returns an empty vector.
   *
   * @param url A string containing a URL to query for
   *
   * @return vector of strings containing ipv6 addresses
   */
   std::vector<std::string> get_ipv6(const std::string& url);

  /**
   * @brief gets a TXT record from a DNS query for the supplied URL;
   * if no TXT record present returns an empty string.
   *
   * @param url A string containing a URL to query for
   *
   * @return A string containing a TXT record; or an empty string
   */
  // TODO: modify this to accomodate DNSSEC
  std::string get_txt_record(const std::string& url);

  /**
   * @brief Gets the singleton instance of DNSResolver
   *
   * @return returns a pointer to the singleton
   */
  static DNSResolver& instance();

private:

  DNSResolverData *m_data;
}; // class DNSResolver

}  // namespace tools
