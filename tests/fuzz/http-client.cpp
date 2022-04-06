// Copyright (c) 2017-2022, The Monero Project
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

#include <boost/utility/string_ref.hpp>
#include "include_base_utils.h"
#include "file_io_utils.h"
#include "net/http_client.h"
#include "net/net_ssl.h"
#include "fuzzer.h"

class dummy_client
{
public:
  bool connect(const std::string& addr, int port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0") { return true; }
  bool connect(const std::string& addr, const std::string& port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0") { return true; }
  bool disconnect() { return true; }
  bool send(const boost::string_ref buff, std::chrono::milliseconds timeout) { return true; }
  bool send(const void* data, size_t sz) { return true; }
  bool is_connected() { return true; }
  bool recv(std::string& buff, std::chrono::milliseconds timeout)
  {
    buff = data;
    data.clear();
    return true;
  }
  void set_ssl(epee::net_utils::ssl_options_t ssl_options) { }
  bool is_connected(bool *ssl = NULL) { return true; }
  uint64_t get_bytes_sent() const  { return 1; }
  uint64_t get_bytes_received() const { return 1; }

  void set_test_data(const std::string &s) { data = s; }

private:
  std::string data;
};

static epee::net_utils::http::http_simple_client_template<dummy_client> client;

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  client.test(std::string((const char*)buf, len), std::chrono::milliseconds(1000));
END_SIMPLE_FUZZER()
