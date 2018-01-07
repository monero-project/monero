// Copyright (c) 2017-2018, The Monero Project
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

#include "include_base_utils.h"
#include "file_io_utils.h"
#include "net/http_client.h"
#include "fuzzer.h"

class dummy_client
{
public:
  bool connect(const std::string& addr, int port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0") { return true; }
  bool connect(const std::string& addr, const std::string& port, std::chrono::milliseconds timeout, bool ssl = false, const std::string& bind_ip = "0.0.0.0") { return true; }
  bool disconnect() { return true; }
  bool send(const std::string& buff, std::chrono::milliseconds timeout) { return true; }
  bool send(const void* data, size_t sz) { return true; }
  bool is_connected() { return true; }
  bool recv(std::string& buff, std::chrono::milliseconds timeout)
  {
    buff = data;
    data.clear();
    return true;
  }

  void set_test_data(const std::string &s) { data = s; }

private:
  std::string data;
};

class HTTPClientFuzzer: public Fuzzer
{
public:
  HTTPClientFuzzer() {}
  virtual int init();
  virtual int run(const std::string &filename);

private:
  epee::net_utils::http::http_simple_client_template<dummy_client> client;
};

int HTTPClientFuzzer::init()
{
  return 0;
}

int HTTPClientFuzzer::run(const std::string &filename)
{
  std::string s;

  if (!epee::file_io_utils::load_file_to_string(filename, s))
  {
    std::cout << "Error: failed to load file " << filename << std::endl;
    return 1;
  }
  try
  {
    client.test(s, std::chrono::milliseconds(1000));
  }
  catch (const std::exception &e)
  {
    std::cerr << "Failed to test http client: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, const char **argv)
{
  HTTPClientFuzzer fuzzer;
  return run_fuzzer(argc, argv, fuzzer);
}

