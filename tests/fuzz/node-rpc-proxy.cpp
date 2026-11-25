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

// Fuzzes the wallet's daemon RPC response handling (NodeRPCProxy): a stub HTTP client
// feeds attacker-controlled JSON-RPC response bodies into invoke_http_json_rpc, driving
// the typed deserialization of daemon replies (get_version, get_info, hard_fork_info,
// fee estimates, get_transactions, block headers, ...) exactly as a hostile daemon
// would deliver them.

#include <deque>
#include <string>

#include "include_base_utils.h"
#include "net/abstract_http_client.h"
#include "wallet/node_rpc_proxy.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "fuzzer.h"

namespace
{
  class stub_http_client final : public epee::net_utils::http::abstract_http_client
  {
  public:
    std::deque<std::string> responses;

    bool set_proxy(const std::string &/*address*/) override { return true; }
    void set_server(std::string /*host*/, std::string /*port*/,
        boost::optional<epee::net_utils::http::login> /*user*/,
        epee::net_utils::ssl_options_t /*ssl_options*/) override {}
    void set_auto_connect(bool /*auto_connect*/) override {}
    bool connect(std::chrono::milliseconds /*timeout*/) override { return true; }
    bool disconnect() override { return true; }
    bool is_connected(bool * /*ssl*/ = NULL) override { return true; }
    bool invoke(const boost::string_ref /*uri*/, const boost::string_ref /*method*/,
        const boost::string_ref /*body*/, std::chrono::milliseconds /*timeout*/,
        const epee::net_utils::http::http_response_info **ppresponse_info = NULL,
        const epee::net_utils::http::fields_list & /*additional_params*/ =
            epee::net_utils::http::fields_list()) override
    {
      if (ppresponse_info)
      {
        m_ri.clear();
        m_ri.m_response_code = 200;
        m_ri.m_mime_tipe = "application/json";
        if (!responses.empty())
        {
          m_ri.m_body = responses.front();
          responses.pop_front();
        }
        *ppresponse_info = &m_ri;
      }
      return true;
    }
    bool invoke_get(const boost::string_ref uri, std::chrono::milliseconds timeout,
        const std::string &body = std::string(),
        const epee::net_utils::http::http_response_info **ppresponse_info = NULL,
        const epee::net_utils::http::fields_list &additional_params =
            epee::net_utils::http::fields_list()) override
    {
      return invoke(uri, "GET", body, timeout, ppresponse_info, additional_params);
    }
    uint64_t get_bytes_sent() const override { return 0; }
    uint64_t get_bytes_received() const override { return 0; }

  private:
    epee::net_utils::http::http_response_info m_ri;
  };
} // anonymous namespace

BEGIN_INIT_SIMPLE_FUZZER()
END_INIT_SIMPLE_FUZZER()

BEGIN_SIMPLE_FUZZER()
  FuzzedDataProvider fdp(buf, len);

  // Configuration from the back: up to 6 fuzzed JSON-RPC replies.
  const std::size_t n_replies = 1 + fdp.ConsumeIntegralInRange<std::size_t>(0, 5);

  boost::recursive_mutex mutex;
  stub_http_client client;
  for (std::size_t i = 0; i < n_replies; ++i)
    client.responses.push_back(fdp.ConsumeBytesAsString(
        std::min<std::size_t>(fdp.remaining_bytes() / 2 + 1, 256)));

  tools::NodeRPCProxy proxy(client, mutex);

  // Drive the getters; each one runs invoke_http_json_rpc with the stub's attacker
  // body and deserializes the typed response. Errors are reported as optional
  // strings (documented behavior) and simply consumed here.
  try
  {
    uint32_t rpc_version = 0;
    std::vector<std::pair<uint8_t, uint64_t>> hard_forks;
    uint64_t height = 0, target_height = 0;
    boost::optional<std::string> err = proxy.get_rpc_version(rpc_version, hard_forks, height, target_height);
    err = proxy.get_height(height);
    err = proxy.get_target_height(height);
    uint64_t weight_limit = 0;
    err = proxy.get_block_weight_limit(weight_limit);
    uint64_t adjusted_time = 0;
    err = proxy.get_adjusted_time(adjusted_time);
    uint64_t earliest_height = 0;
    for (uint8_t v = 1; v <= 2; ++v)
      err = proxy.get_earliest_height(v, earliest_height);
    uint64_t fee = 0;
    err = proxy.get_dynamic_base_fee_estimate(4, fee);
    std::vector<uint64_t> fees;
    err = proxy.get_dynamic_base_fee_estimate_2021_scaling(4, fees);
    uint64_t fee_quantization_mask = 0;
    err = proxy.get_fee_quantization_mask(fee_quantization_mask);
    std::vector<crypto::hash> txids(2);
    err = proxy.get_transactions(txids,
        [](const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request &,
           const cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response &, bool) {});
    cryptonote::block_header_response block_header;
    err = proxy.get_block_header_by_height(1, block_header);
    (void)err;
  }
  catch (...) {}
END_SIMPLE_FUZZER()
