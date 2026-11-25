// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials
//    provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific prior
//    written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Fuzzes the epee HTTP server connection handler: request line/header/body parsing
// (incl. multipart), pipelined and fragmented requests, and RFC 7616 digest
// authentication challenges and responses (http_auth), via the same
// handle_buff_in() entry the TCP server feeds received bytes through.

#include <algorithm>
#include <cstring>
#include <string>

#include "include_base_utils.h"
#include "net/http_auth.h"
#include "net/http_protocol_handler.h"
#include "net/net_utils_base.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

namespace
{
  class stub_endpoint final : public epee::net_utils::i_service_endpoint
  {
  public:
    stub_endpoint(): m_sent_bytes(0), m_io_context() {}

    bool do_send(epee::byte_slice message) override
    {
      m_sent_bytes += message.size();
      return true;
    }
    bool close(const bool /* wait_for_shutdown */) override { return true; }
    bool send_done() override { return true; }
    bool call_run_once_service_io() override { return true; }
    bool request_callback() override { return true; }
    boost::asio::io_context& get_io_context() override { return m_io_context; }

  private:
    size_t m_sent_bytes;
    boost::asio::io_context m_io_context;
  };

  struct stub_request_handler final
    : epee::net_utils::http::i_http_server_handler<epee::net_utils::connection_context_base>
  {
    bool handle_http_request(const epee::net_utils::http::http_request_info& /* query_info */,
        epee::net_utils::http::http_response_info& response_info,
        epee::net_utils::connection_context_base& /* conn_context */) override
    {
      response_info.m_mime_tipe = "text/plain";
      response_info.m_response_code = 200;
      response_info.m_response_comment = "OK";
      response_info.m_body = "ok";
      return true;
    }
  };
} // anonymous namespace

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  stub_endpoint ep;
  stub_request_handler ph;
  epee::net_utils::http::custum_handler_config<epee::net_utils::connection_context_base> cfg;
  cfg.m_phandler = &ph;
  cfg.m_disable_md5 = fdp.ConsumeBool();
  if (fdp.ConsumeBool())
  {
    epee::net_utils::http::login creds;
    creds.username = fdp.ConsumeBytesAsString(fdp.ConsumeIntegralInRange<size_t>(0, 16));
    creds.password = epee::wipeable_string(fdp.ConsumeBytesAsString(fdp.ConsumeIntegralInRange<size_t>(0, 16)));
    cfg.m_user = std::move(creds);
  }
  cfg.rng = [](size_t n, uint8_t *ptr) { memset(ptr, 0xA5, n); };

  epee::net_utils::connection_context_base conn_context;
  epee::net_utils::http::http_custom_handler<epee::net_utils::connection_context_base>
    handler(&ep, cfg, conn_context);

  // Feed the input the way the TCP server would: in up to 8 fuzz-sized chunks,
  // so fragmented and pipelined requests hit the buffering state machine too.
  static constexpr size_t kMaxChunks = 8;
  for (size_t i = 0; i < kMaxChunks && fdp.remaining_bytes() > 0; ++i)
  {
    const size_t max_chunk = std::min<size_t>(fdp.remaining_bytes(), 1024);
    const size_t chunk_len = fdp.ConsumeIntegralInRange<size_t>(1, max_chunk);
    std::string chunk = fdp.ConsumeBytesAsString(chunk_len);
    if (!handler.handle_recv(chunk.data(), chunk.size()))
      break;
  }
END_SIMPLE_FUZZER()
