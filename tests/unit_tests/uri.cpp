// Copyright (c) 2016-2024, The Monero Project
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

#include "gtest/gtest.h"
#include "string_tools.h"
#include "wallet/wallet2.h"

#define TEST_ADDRESS "9tTLtauaEKSj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5PaohYHC"
#define TEST_INTEGRATED_ADDRESS "A4A1uPj4qaxj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5acaPMJfMbn3ReTsBpp"
// included payment id: <f612cac0b6cb1cda>

#define PARSE_URI(uri, expected) \
  std::string address, payment_id, recipient_name, description, error; \
  uint64_t amount; \
  std::vector<std::string> unknown_parameters; \
  tools::wallet2 w(cryptonote::TESTNET); \
  bool ret = w.parse_uri(uri, address, payment_id, amount, description, recipient_name, unknown_parameters, error); \
  ASSERT_EQ(ret, expected);

TEST(uri, empty_string)
{
  PARSE_URI("", false);
}

TEST(uri, no_scheme)
{
  PARSE_URI("monero", false);
}

TEST(uri, bad_scheme)
{
  PARSE_URI("http://foo", false);
}

TEST(uri, scheme_not_first)
{
  PARSE_URI(" monero:", false);
}

TEST(uri, no_body)
{
  PARSE_URI("monero:", false);
}

TEST(uri, no_address)
{
  PARSE_URI("monero:?", false);
}

TEST(uri, bad_address)
{
  PARSE_URI("monero:44444", false);
}

TEST(uri, good_address)
{
  PARSE_URI("monero:" TEST_ADDRESS, true);
  ASSERT_EQ(address, TEST_ADDRESS);
}

TEST(uri, resets_outputs)
{
  std::string address = "old address";
  std::string payment_id = "old payment id";
  std::string recipient_name = "old recipient name";
  std::string description = "old description";
  std::string error = "old error";
  uint64_t amount = 1;
  std::vector<std::string> unknown_parameters{"old=parameter"};
  tools::wallet2 w(cryptonote::TESTNET);

  ASSERT_TRUE(w.parse_uri("monero:" TEST_ADDRESS, address, payment_id, amount, description, recipient_name, unknown_parameters, error));
  EXPECT_EQ(address, TEST_ADDRESS);
  EXPECT_EQ(amount, 0);
  EXPECT_TRUE(payment_id.empty());
  EXPECT_TRUE(description.empty());
  EXPECT_TRUE(recipient_name.empty());
  EXPECT_TRUE(unknown_parameters.empty());
  EXPECT_TRUE(error.empty());
}

TEST(uri, good_integrated_address)
{
  PARSE_URI("monero:" TEST_INTEGRATED_ADDRESS, true);
}

TEST(uri, parameter_without_inter)
{
  PARSE_URI("monero:" TEST_ADDRESS"&amount=1", false);
}

TEST(uri, parameter_without_equals)
{
  PARSE_URI("monero:" TEST_ADDRESS"?amount", false);
}

TEST(uri, parameter_without_value)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_amount=", false);
}

TEST(uri, negative_amount)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_amount=-1", false);
}

TEST(uri, bad_amount)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_amount=alphanumeric", false);
}

TEST(uri, duplicate_parameter)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_amount=1&tx_amount=1", false);
}

TEST(uri, unknown_parameter)
{
  PARSE_URI("monero:" TEST_ADDRESS"?unknown=1", true);
  ASSERT_EQ(unknown_parameters.size(), 1);
  ASSERT_EQ(unknown_parameters[0], "unknown=1");
}

TEST(uri, unknown_parameters)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_amount=1&unknown=1&tx_description=desc&foo=bar", true);
  ASSERT_EQ(unknown_parameters.size(), 2);
  ASSERT_EQ(unknown_parameters[0], "unknown=1");
  ASSERT_EQ(unknown_parameters[1], "foo=bar");
}

TEST(uri, empty_payment_id)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_payment_id=", false);
}

TEST(uri, bad_payment_id)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_payment_id=1234567890", false);
}

TEST(uri, short_payment_id)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_payment_id=1234567890123456", false);
}

TEST(uri, long_payment_id)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_payment_id=1234567890123456789012345678901234567890123456789012345678901234", true);
  ASSERT_EQ(address, TEST_ADDRESS);
  ASSERT_EQ(payment_id, "1234567890123456789012345678901234567890123456789012345678901234");
}

TEST(wallet2, parse_long_payment_id)
{
  const std::string payment_id_hex = "00112233445566778899aabbccddeeffffeeddccbbaa99887766554433221100";
  crypto::hash payment_id = crypto::null_hash;

  ASSERT_TRUE(tools::wallet2::parse_long_payment_id(payment_id_hex, payment_id));
  EXPECT_EQ(payment_id_hex, epee::string_tools::pod_to_hex(payment_id));

  const crypto::hash unchanged = payment_id;
  EXPECT_FALSE(tools::wallet2::parse_long_payment_id(payment_id_hex.substr(2), payment_id));
  EXPECT_EQ(unchanged, payment_id);
  EXPECT_FALSE(tools::wallet2::parse_long_payment_id(std::string(64, 'z'), payment_id));
  EXPECT_EQ(unchanged, payment_id);
}

TEST(wallet2, parse_short_payment_id)
{
  const std::string payment_id_hex = "0011223344556677";
  crypto::hash8 payment_id = crypto::null_hash8;

  ASSERT_TRUE(tools::wallet2::parse_short_payment_id(payment_id_hex, payment_id));
  EXPECT_EQ(payment_id_hex, epee::string_tools::pod_to_hex(payment_id));

  const crypto::hash8 unchanged = payment_id;
  EXPECT_FALSE(tools::wallet2::parse_short_payment_id(payment_id_hex.substr(2), payment_id));
  EXPECT_EQ(unchanged, payment_id);
  EXPECT_FALSE(tools::wallet2::parse_short_payment_id(std::string(16, 'z'), payment_id));
  EXPECT_EQ(unchanged, payment_id);
}

TEST(wallet2, parse_payment_id_pads_short_ids)
{
  const std::string payment_id_hex = "0011223344556677";
  crypto::hash payment_id = crypto::null_hash;

  ASSERT_TRUE(tools::wallet2::parse_payment_id(payment_id_hex, payment_id));
  EXPECT_EQ(payment_id_hex + std::string(48, '0'), epee::string_tools::pod_to_hex(payment_id));
}

TEST(uri, payment_id_with_integrated_address)
{
  PARSE_URI("monero:" TEST_INTEGRATED_ADDRESS"?tx_payment_id=1234567890123456", false);
}

TEST(uri, empty_description)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=", true);
  ASSERT_EQ(description, "");
}

TEST(uri, empty_recipient_name)
{
  PARSE_URI("monero:" TEST_ADDRESS"?recipient_name=", true);
  ASSERT_EQ(recipient_name, "");
}

TEST(uri, non_empty_description)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo", true);
  ASSERT_EQ(description, "foo");
}

TEST(uri, non_empty_recipient_name)
{
  PARSE_URI("monero:" TEST_ADDRESS"?recipient_name=foo", true);
  ASSERT_EQ(recipient_name, "foo");
}

TEST(uri, url_encoding)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo%20bar", true);
  ASSERT_EQ(description, "foo bar");
}

TEST(uri, non_alphanumeric_url_encoding)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo%2x", true);
  ASSERT_EQ(description, "foo%2x");
}

TEST(uri, truncated_url_encoding)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo%2", true);
  ASSERT_EQ(description, "foo%2");
}

TEST(uri, percent_without_url_encoding)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo%", true);
  ASSERT_EQ(description, "foo%");
}

TEST(uri, url_encoded_once)
{
  PARSE_URI("monero:" TEST_ADDRESS"?tx_description=foo%2020", true);
  ASSERT_EQ(description, "foo 20");
}

TEST(uri, make_uri_encodes_equals)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  const std::string uri = w.make_uri(TEST_ADDRESS, "", 0, "key=value", "name=value", error);

  ASSERT_TRUE(error.empty());
  ASSERT_EQ(uri, "monero:" TEST_ADDRESS"?recipient_name=name%3Dvalue&tx_description=key%3Dvalue");

  std::string address, payment_id, recipient_name, description;
  uint64_t amount;
  std::vector<std::string> unknown_parameters;
  ASSERT_TRUE(w.parse_uri(uri, address, payment_id, amount, description, recipient_name, unknown_parameters, error));
  EXPECT_EQ(recipient_name, "name=value");
  EXPECT_EQ(description, "key=value");
}

TEST(uri, make_multi_destination)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  std::vector<tools::wallet2::uri_destination> destinations;
  destinations.push_back({TEST_ADDRESS, 1500000000000});
  destinations.push_back({TEST_ADDRESS, 2000000000000});
  const std::string uri = w.make_uri(destinations, "desc", "names", error);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(uri, "monero:" TEST_ADDRESS ";" TEST_ADDRESS "?tx_amount=1.500000000000;2.000000000000&recipient_name=names&tx_description=desc");
}

TEST(uri, make_multi_destination_no_amounts)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  std::vector<tools::wallet2::uri_destination> destinations;
  destinations.push_back({TEST_ADDRESS, 0});
  destinations.push_back({TEST_ADDRESS, 0});
  const std::string uri = w.make_uri(destinations, "", "", error);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(uri, "monero:" TEST_ADDRESS ";" TEST_ADDRESS "?tx_amount=0.000000000000;0.000000000000");
}

TEST(uri, make_multi_destination_rejects_two_integrated)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  std::vector<tools::wallet2::uri_destination> destinations;
  destinations.push_back({TEST_INTEGRATED_ADDRESS, 1});
  destinations.push_back({TEST_INTEGRATED_ADDRESS, 2});
  ASSERT_TRUE(w.make_uri(destinations, "", "", error).empty());
  ASSERT_FALSE(error.empty());
}

TEST(uri, parse_multi_destination)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::vector<tools::wallet2::uri_destination> destinations;
  std::string payment_id, description, recipient_names, error;
  std::vector<std::string> unknown_parameters;
  ASSERT_TRUE(w.parse_uri("monero:" TEST_ADDRESS ";" TEST_ADDRESS "?tx_amount=1.5;2", destinations, payment_id, description, recipient_names, unknown_parameters, error));
  ASSERT_EQ(destinations.size(), 2);
  EXPECT_EQ(destinations[0].address, TEST_ADDRESS);
  EXPECT_EQ(destinations[0].amount, 1500000000000);
  EXPECT_EQ(destinations[1].address, TEST_ADDRESS);
  EXPECT_EQ(destinations[1].amount, 2000000000000);
  EXPECT_TRUE(payment_id.empty());
  EXPECT_TRUE(description.empty());
  EXPECT_TRUE(recipient_names.empty());
}

TEST(uri, parse_multi_destination_round_trip)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  std::vector<tools::wallet2::uri_destination> made;
  made.push_back({TEST_ADDRESS, 1000000000000});
  made.push_back({TEST_ADDRESS, 200000000000});
  const std::string uri = w.make_uri(made, "some desc", "a name", error);
  ASSERT_TRUE(error.empty());

  std::vector<tools::wallet2::uri_destination> parsed;
  std::string payment_id, description, recipient_names;
  std::vector<std::string> unknown_parameters;
  ASSERT_TRUE(w.parse_uri(uri, parsed, payment_id, description, recipient_names, unknown_parameters, error));
  ASSERT_EQ(parsed.size(), made.size());
  for (size_t i = 0; i < parsed.size(); ++i)
  {
    EXPECT_EQ(parsed[i].address, made[i].address);
    EXPECT_EQ(parsed[i].amount, made[i].amount);
  }
  EXPECT_EQ(description, "some desc");
  EXPECT_EQ(recipient_names, "a name");
}

TEST(uri, parse_multi_destination_amount_mismatch)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::vector<tools::wallet2::uri_destination> destinations;
  std::string payment_id, description, recipient_names, error;
  std::vector<std::string> unknown_parameters;
  ASSERT_FALSE(w.parse_uri("monero:" TEST_ADDRESS ";" TEST_ADDRESS "?tx_amount=1", destinations, payment_id, description, recipient_names, unknown_parameters, error));
  ASSERT_FALSE(w.parse_uri("monero:" TEST_ADDRESS ";?tx_amount=1;2", destinations, payment_id, description, recipient_names, unknown_parameters, error));
}

TEST(uri, parse_multi_destination_single_destination_rejects)
{
  // the deprecated single-destination parser refuses multi-destination URIs
  tools::wallet2 w(cryptonote::TESTNET);
  std::string address, payment_id, recipient_name, description, error;
  uint64_t amount;
  std::vector<std::string> unknown_parameters;
  ASSERT_FALSE(w.parse_uri("monero:" TEST_ADDRESS ";" TEST_ADDRESS, address, payment_id, amount, description, recipient_name, unknown_parameters, error));
}

TEST(uri, multi_destination_with_payment_id)
{
  // a payment id is accepted, but only if no destination uses an integrated address,
  // and it is exposed to callers which choose to reject it
  tools::wallet2 w(cryptonote::TESTNET);
  std::vector<tools::wallet2::uri_destination> destinations;
  std::string payment_id, description, recipient_names, error;
  std::vector<std::string> unknown_parameters;
  ASSERT_TRUE(w.parse_uri("monero:" TEST_ADDRESS ";" TEST_ADDRESS "?tx_payment_id=1234567890123456789012345678901234567890123456789012345678901234", destinations, payment_id, description, recipient_names, unknown_parameters, error));
  ASSERT_EQ(destinations.size(), 2);
  ASSERT_FALSE(payment_id.empty());

  ASSERT_FALSE(w.parse_uri("monero:" TEST_INTEGRATED_ADDRESS ";" TEST_ADDRESS "?tx_payment_id=1234567890123456789012345678901234567890123456789012345678901234", destinations, payment_id, description, recipient_names, unknown_parameters, error));
}
