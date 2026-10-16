// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list
//    of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
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
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "net/abstract_http_client.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "storages/portable_storage_template_helper.h"
#include "wallet/wallet2.h"

namespace
{
  class recording_http_client final : public epee::net_utils::http::abstract_http_client
  {
  public:
    std::vector<std::string> events;
    bool connected = true;
    bool auto_connect = true;

    void set_server(std::string, std::string, boost::optional<epee::net_utils::http::login>, epee::net_utils::ssl_options_t) override
    {}

    void set_auto_connect(bool auto_connect_) override
    {
      auto_connect = auto_connect_;
    }

    bool connect(std::chrono::milliseconds) override
    {
      events.push_back("connect");
      connected = true;
      return true;
    }

    bool disconnect() override
    {
      events.push_back("disconnect");
      connected = false;
      return true;
    }

    bool is_connected(bool *ssl = NULL) override
    {
      if (ssl)
        *ssl = false;
      return connected;
    }

    bool invoke(const boost::string_ref uri, const boost::string_ref, const boost::string_ref, std::chrono::milliseconds timeout,
        const epee::net_utils::http::http_response_info** ppresponse_info = NULL,
        const epee::net_utils::http::fields_list& = epee::net_utils::http::fields_list()) override
    {
      if (!connected)
      {
        if (!auto_connect || !connect(timeout))
          return false;
      }

      events.push_back("invoke:" + std::string{uri.data(), uri.size()});
      cryptonote::COMMAND_RPC_SEND_RAW_TX::response res{};
      res.status = CORE_RPC_STATUS_OK;
      response.m_response_code = 200;
      EXPECT_TRUE(epee::serialization::store_t_to_json(res, response.m_body));
      if (ppresponse_info)
        *ppresponse_info = &response;
      return true;
    }

    bool invoke_get(const boost::string_ref, std::chrono::milliseconds, const std::string&,
        const epee::net_utils::http::http_response_info**, const epee::net_utils::http::fields_list&) override
    {
      return false;
    }

    uint64_t get_bytes_sent() const override { return 0; }
    uint64_t get_bytes_received() const override { return 0; }

  private:
    epee::net_utils::http::http_response_info response;
  };

  class recording_http_client_factory final : public epee::net_utils::http::http_client_factory
  {
  public:
    explicit recording_http_client_factory(std::vector<recording_http_client*> &clients)
      : clients(clients)
    {}

    std::unique_ptr<epee::net_utils::http::abstract_http_client> create() override
    {
      auto client = std::make_unique<recording_http_client>();
      clients.push_back(client.get());
      return client;
    }

  private:
    std::vector<recording_http_client*> &clients;
  };
}

TEST(wallet_daemon_connection, commit_tx_reconnects_before_submit)
{
  std::vector<recording_http_client*> clients;
  tools::wallet2 wallet{
    cryptonote::TESTNET,
    1,
    true,
    std::make_unique<recording_http_client_factory>(clients)
  };

  ASSERT_GE(clients.size(), 1u);
  recording_http_client &daemon_client = *clients.front();
  tools::wallet2::pending_tx ptx{};

  wallet.store_tx_info(false);
  wallet.commit_tx(ptx);

  ASSERT_EQ(3u, daemon_client.events.size());
  EXPECT_EQ("disconnect", daemon_client.events[0]);
  EXPECT_EQ("connect", daemon_client.events[1]);
  EXPECT_EQ("invoke:/sendrawtransaction", daemon_client.events[2]);
}

TEST(wallet_daemon_connection, commit_tx_vector_reconnects_once)
{
  std::vector<recording_http_client*> clients;
  tools::wallet2 wallet{
    cryptonote::TESTNET,
    1,
    true,
    std::make_unique<recording_http_client_factory>(clients)
  };

  ASSERT_GE(clients.size(), 1u);
  recording_http_client &daemon_client = *clients.front();
  std::vector<tools::wallet2::pending_tx> ptx_vector(2);

  wallet.store_tx_info(false);
  wallet.commit_tx(ptx_vector);

  ASSERT_EQ(4u, daemon_client.events.size());
  EXPECT_EQ("disconnect", daemon_client.events[0]);
  EXPECT_EQ("connect", daemon_client.events[1]);
  EXPECT_EQ("invoke:/sendrawtransaction", daemon_client.events[2]);
  EXPECT_EQ("invoke:/sendrawtransaction", daemon_client.events[3]);
}
