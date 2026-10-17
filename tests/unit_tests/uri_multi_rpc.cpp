// Copyright (c) 2026, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
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

#include "gtest/gtest.h"
#include "wallet/uri_rpc.h"
#include "storages/portable_storage_template_helper.h"

namespace
{
  // Unlike the public request reader, this reader requires every response field.
  // It catches fields accidentally omitted by KV_SERIALIZE_OPT during storage.
  struct required_recipient_fields : tools::wallet_rpc::uri_multi_recipient
  {
    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(currency)
      KV_SERIALIZE(label)
    END_KV_SERIALIZE_MAP()
  };
}

TEST(uri_multi_rpc, response_emits_unspecified_defaults)
{
  tools::wallet_rpc::uri_multi_recipient recipient;
  recipient.address = "A";
  std::string encoded;
  ASSERT_TRUE(epee::serialization::store_t_to_json(recipient, encoded));
  required_recipient_fields required;
  ASSERT_TRUE(epee::serialization::load_t_from_json(required, encoded)) << encoded;
  EXPECT_TRUE(required.amount.empty());
  EXPECT_EQ(required.currency, "XMR");
  EXPECT_TRUE(required.label.empty());
}

TEST(uri_multi_rpc, request_preserves_omitted_and_zero_amounts)
{
  tools::wallet_rpc::COMMAND_RPC_MAKE_URI_MULTI::request request;
  ASSERT_TRUE(epee::serialization::load_t_from_json(request,
    R"({"recipients":[{"address":"A","amount":"0","currency":"USD","label":"zero"},{"address":"B"}]})"));
  EXPECT_EQ(request.network_type, "mainnet");
  ASSERT_EQ(request.recipients.size(), 2);
  EXPECT_EQ(request.recipients[0].amount, "0");
  EXPECT_EQ(request.recipients[0].currency, "USD");
  EXPECT_TRUE(request.recipients[1].amount.empty());
  EXPECT_EQ(request.recipients[1].currency, "XMR");
  EXPECT_TRUE(request.recipients[1].label.empty());

  std::string encoded;
  ASSERT_TRUE(epee::serialization::store_t_to_json(request, encoded));
  tools::wallet_rpc::COMMAND_RPC_MAKE_URI_MULTI::request restored;
  ASSERT_TRUE(epee::serialization::load_t_from_json(restored, encoded));
  ASSERT_EQ(restored.recipients.size(), 2);
  EXPECT_EQ(restored.recipients[0].amount, "0");
  EXPECT_TRUE(restored.recipients[1].amount.empty());
}

TEST(uri_multi_rpc, exact_decimal_is_not_limited_to_integer_width)
{
  tools::wallet_rpc::COMMAND_RPC_MAKE_URI_MULTI::request request;
  ASSERT_TRUE(epee::serialization::load_t_from_json(request,
    R"({"network_type":"testnet","recipients":[{"address":"A","amount":"20.000000000000000001","currency":"ETH"}],"tx_description":"x=y & z"})"));
  EXPECT_EQ(request.network_type, "testnet");
  ASSERT_EQ(request.recipients.size(), 1);
  EXPECT_EQ(request.recipients[0].amount, "20.000000000000000001");
  EXPECT_EQ(request.tx_description, "x=y & z");
}

TEST(uri_multi_rpc, parse_response_round_trips_recipient_order_and_unknowns)
{
  tools::wallet_rpc::COMMAND_RPC_PARSE_URI_MULTI::response response;
  response.uri.recipients.push_back({"A", "0.011", "XMR", "first"});
  response.uri.recipients.push_back({"B", "", "XMR", "second"});
  response.uri.tx_description = "two outputs";
  response.unknown_parameters = {"extra=value%20one"};
  std::string encoded;
  ASSERT_TRUE(epee::serialization::store_t_to_json(response, encoded));
  tools::wallet_rpc::COMMAND_RPC_PARSE_URI_MULTI::response restored;
  ASSERT_TRUE(epee::serialization::load_t_from_json(restored, encoded));
  ASSERT_EQ(restored.uri.recipients.size(), 2);
  EXPECT_EQ(restored.uri.recipients[0].address, "A");
  EXPECT_EQ(restored.uri.recipients[0].amount, "0.011");
  EXPECT_EQ(restored.uri.recipients[1].address, "B");
  EXPECT_TRUE(restored.uri.recipients[1].amount.empty());
  EXPECT_EQ(restored.uri.tx_description, "two outputs");
  EXPECT_EQ(restored.unknown_parameters, response.unknown_parameters);
}

TEST(uri_multi_rpc, parse_request_defaults_network)
{
  tools::wallet_rpc::COMMAND_RPC_PARSE_URI_MULTI::request request;
  ASSERT_TRUE(epee::serialization::load_t_from_json(request, R"({"uri":"monero:A?version=2.0"})"));
  EXPECT_EQ(request.network_type, "mainnet");
  EXPECT_EQ(request.uri, "monero:A?version=2.0");
}
