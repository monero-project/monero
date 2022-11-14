// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <string>
#include <boost/optional/optional.hpp>
#include "http_auth.h"
#include "net/net_ssl.h"

namespace epee
{
namespace net_utils
{
  inline const char* get_hex_vals()
  {
    static constexpr const char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    return hexVals;
  }

  inline const char* get_unsave_chars()
  {
    //static constexpr char unsave_chars[] = "\"<>%\\^[]`+$,@:;/!#?=&";
    static constexpr const char unsave_chars[] = "\"<>%\\^[]`+$,@:;!#&";
    return unsave_chars;
  }

  bool is_unsafe(unsigned char compare_char);
  std::string dec_to_hex(char num, int radix);
  int get_index(const char *s, char c);
  std::string hex_to_dec_2bytes(const char *s);
  std::string convert(char val);
  std::string conver_to_url_format(const std::string& uri);
  std::string convert_from_url_format(const std::string& uri);
  std::string convert_to_url_format_force_all(const std::string& uri);

namespace http
{
  class abstract_http_client
  {
  public:
    abstract_http_client() {}
    virtual ~abstract_http_client() {}
    bool set_server(const std::string& address, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect, const std::string& virtual_host = {});
    virtual bool set_proxy(const std::string& address);
    virtual void set_server(std::string host, std::string port, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect, const std::string& virtual_host = {}) = 0;
    virtual void set_auto_connect(bool auto_connect) = 0;
    virtual bool connect(std::chrono::milliseconds timeout) = 0;
    virtual bool disconnect() = 0;
    virtual bool is_connected(bool *ssl = NULL) = 0;
    virtual bool invoke(const boost::string_ref uri, const boost::string_ref method, const boost::string_ref body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
    virtual bool invoke_get(const boost::string_ref uri, std::chrono::milliseconds timeout, const std::string& body = std::string(), const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
    virtual bool invoke_post(const boost::string_ref uri, const std::string& body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
    virtual uint64_t get_bytes_sent() const = 0;
    virtual uint64_t get_bytes_received() const = 0;
  };

  class http_client_factory
  {
  public:
    virtual ~http_client_factory() {}
    virtual std::unique_ptr<abstract_http_client> create() = 0;
  };
}
}
}
