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
#include "wallet/uri.h"

namespace
{
  const char* address = "9tTLtauaEKSj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5PaohYHC";
  const char* integrated = "A4A1uPj4qaxj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5acaPMJfMbn3ReTsBpp";
}

TEST(uri_multi_integration, validates_all_addresses_on_requested_network)
{
  tools::wallet::payment_uri input;
  input.recipients.push_back({address, "0.011", "XMR", "ordinary"});
  input.recipients.push_back({integrated, "", "XMR", "integrated"});
  std::string error = "stale";
  const auto uri = tools::wallet::make_uri_multi(input, cryptonote::TESTNET, error);
  ASSERT_FALSE(uri.empty()) << error;
  ASSERT_TRUE(error.empty());
  tools::wallet::payment_uri output;
  ASSERT_TRUE(tools::wallet::parse_uri_multi(uri, cryptonote::TESTNET, output, error)) << error;
  ASSERT_EQ(output.recipients.size(), 2);
  EXPECT_EQ(output.recipients[0].address, address);
  EXPECT_EQ(output.recipients[0].amount, "0.011");
  EXPECT_EQ(output.recipients[1].address, integrated);
  EXPECT_TRUE(output.recipients[1].amount.empty());

  EXPECT_FALSE(tools::wallet::parse_uri_multi(uri, cryptonote::MAINNET, output, error));
  EXPECT_TRUE(output.recipients.empty());
  EXPECT_FALSE(error.empty());
  EXPECT_TRUE(tools::wallet::make_uri_multi(input, cryptonote::UNDEFINED, error).empty());
}

TEST(uri_multi_integration, rejects_bad_second_checksum_and_resets_output)
{
  std::string bad = address;
  bad.back() = bad.back() == '1' ? '2' : '1';
  const std::string uri = std::string("monero:") + address + "?version=2.0&amount=1&address=" + bad;
  tools::wallet::payment_uri output;
  output.recipients.push_back({address, "9", "XMR", "stale"});
  output.tx_description = "stale";
  std::string error;
  EXPECT_FALSE(tools::wallet::parse_uri_multi(uri, cryptonote::TESTNET, output, error));
  EXPECT_TRUE(output.recipients.empty());
  EXPECT_TRUE(output.tx_description.empty());
  EXPECT_FALSE(error.empty());

  tools::wallet::payment_uri input;
  input.recipients.push_back({address, "1", "XMR", ""});
  input.recipients.push_back({bad, "2", "XMR", ""});
  EXPECT_TRUE(tools::wallet::make_uri_multi(input, cryptonote::TESTNET, error).empty());
}

TEST(uri_multi_integration, accepts_legacy_uri_without_opening_wallet)
{
  tools::wallet::payment_uri output;
  std::string error;
  ASSERT_TRUE(tools::wallet::parse_uri_multi(std::string("monero:") + address +
    "?tx_amount=0.011000000000&recipient_name=Name%3Dvalue&extra=one", cryptonote::TESTNET, output, error)) << error;
  ASSERT_EQ(output.recipients.size(), 1);
  EXPECT_EQ(output.recipients[0].amount, "0.011000000000");
  EXPECT_EQ(output.recipients[0].currency, "XMR");
  EXPECT_EQ(output.recipients[0].label, "Name=value");
  ASSERT_EQ(output.unknown_parameters.size(), 1);
  EXPECT_EQ(output.unknown_parameters[0], "extra=one");
}
