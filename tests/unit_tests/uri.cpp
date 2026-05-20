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
#include "wallet/wallet2.h"
#include "wallet/uri.hpp"

#define TEST_ADDRESS "9tTLtauaEKSj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5PaohYHC"
#define TEST_INTEGRATED_ADDRESS "A4A1uPj4qaxj7xoVXytVH32R1pLZBk4VV4mZFGEh4wkXhDWqw1soPyf3fGixf1kni31VznEZkWNEza9d5TvjWwq5acaPMJfMbn3ReTsBpp"
#define TEST_INTEGRATED_ADDRESS2 "48UktANa1g71SkdXhHJ72kp4GZf2tvKwBzXjRSe5SZbFxjrjDwpT7obRksYzYpy5KN5wUGagY7q2aqFUDDhYSnA5Z6J82B5XZQGkDox9a"
// included payment id: <f612cac0b6cb1cda>

#define PARSE_URI(uri, expected) \
  std::string address, payment_id, recipient_name, description, error; \
  uint64_t amount; \
  std::vector<std::string> unknown_parameters; \
  tools::wallet2 w(cryptonote::TESTNET); \
  bool ret = w.parse_uri(uri, address, payment_id, amount, description, recipient_name, unknown_parameters, error); \
  ASSERT_EQ(ret, expected);

// newer function that supersedes the parse_uri in wallet2.
#define PARSE_URI2(uri, expected) \
  std::vector<std::string> addresses, recipient_names, unknown_parameters; \
  std::vector<tools::wallet::tx_amount> amounts; \
  std::string description, error; \
  bool ret = tools::wallet::parse_uri(uri, addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET); \
  ASSERT_EQ(ret, expected);

#define MAKE_URI2(uri_var, addresses, amounts, recipient_names, description, expected) \
  std::string uri_var; std::string err_##uri_var; \
  uri_var = tools::wallet::make_uri(addresses, amounts, recipient_names, description, err_##uri_var, cryptonote::TESTNET); \
  if (expected) ASSERT_TRUE(err_##uri_var.empty()); \
  else ASSERT_FALSE(err_##uri_var.empty());

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


TEST(uri, test_basic_multiple_recipient_uri_parsing)
{
  PARSE_URI2("monero:" TEST_ADDRESS "?version=2.0&address=" TEST_ADDRESS "&amount=0.5XMR&label=Alice"
    "&address=" TEST_ADDRESS "&amount=0.2XMR&label=Bob"
    "&tx_description=Payment%20for%20services", true);

  ASSERT_EQ(addresses.size(), 3u);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ(addresses[2], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[1].amount, 500000000000ULL);
  ASSERT_EQ((uint64_t)amounts[2].amount, 200000000000ULL);
  ASSERT_EQ(recipient_names[1], "Alice");
  ASSERT_EQ(recipient_names[2], "Bob");
  ASSERT_EQ(description, "Payment for services");

  {
    PARSE_URI2("monero:" TEST_ADDRESS "?version=2.0&address=" TEST_ADDRESS "&unknown_param=123;456", true);
    ASSERT_EQ(unknown_parameters.size(), 1u);
    ASSERT_EQ(unknown_parameters[0], "unknown_param=123;456");
  }
}

TEST(uri, test_parse_uri_v2_with_currencies)
{
  struct test_case {
    const char* uri;
    uint128 amount;
    const char* currency;
  };

  const test_case cases[] = { 
    { "monero:" TEST_ADDRESS "?version=2.0&amount=0.5BTC", (uint128)50000000ULL, "BTC" },
    { "monero:" TEST_ADDRESS "?version=2.0&amount=12345.67890123456789ETH", uint128("12345678901234567890000"), "ETH" },
    { "monero:" TEST_ADDRESS "?version=2.0&amount=12.34EUR", (uint128)1234ULL, "EUR" },
  };

  for (const auto& c : cases) {
    PARSE_URI2(c.uri, true);
    ASSERT_EQ(addresses.size(), 1u);
    ASSERT_EQ(amounts[0].amount, c.amount);
    ASSERT_EQ(amounts[0].currency, c.currency);
  }
}

TEST(uri, test_make_uri_v2_with_currencies)
{  
  std::vector<tools::wallet::tx_amount> make_amounts = {
    { (uint128)100000000ULL, "BTC" },
    { (uint128)500000000000ULL, "XMR" }
  };
  std::vector<std::string> make_addresses = { TEST_ADDRESS, TEST_ADDRESS };
  std::vector<std::string> make_recipient_names = { "Alice", "Bob" };

  MAKE_URI2(uri, make_addresses, make_amounts, make_recipient_names, "multi-recipient payout",true);
  PARSE_URI2(uri, true);
  ASSERT_EQ(addresses.size(), 2u);
  ASSERT_EQ(amounts[0].currency, "BTC");
  ASSERT_EQ((uint64_t)amounts[0].amount, 100000000ULL);
  ASSERT_EQ(amounts[1].currency, "XMR");
  ASSERT_EQ((uint64_t)amounts[1].amount, 500000000000ULL);

  make_amounts.pop_back();
  make_addresses.pop_back();
  make_recipient_names.pop_back();
  MAKE_URI2(single_recipient_uri_with_currency, make_addresses, make_amounts, make_recipient_names, "btc payment", true);
  {
    PARSE_URI(single_recipient_uri_with_currency, false);
  }
}

TEST(uri, test_make_uri_v2_uint64_overload)
{
  std::vector<std::string> make_addresses = { TEST_ADDRESS }, make_recipient_names = { "Dave" };
  std::vector<uint64_t> make_amounts = { 250000000000ULL };
  MAKE_URI2(uri, make_addresses, make_amounts, make_recipient_names, "quarter", true);
  PARSE_URI2(uri, true);
  ASSERT_EQ((uint64_t)amounts[0].amount, 250000000000ULL);
  ASSERT_EQ(amounts[0].currency, std::string("XMR"));
  ASSERT_EQ(recipient_names[0], std::string("Dave"));
}

TEST(uri, test_legacy_and_v2_uri_compatibility)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error;
  std::string old_uri = w.make_uri(TEST_ADDRESS, std::string(), 200000000000ULL, "old style", "Bob", error);
  ASSERT_TRUE(error.empty());
  {
    PARSE_URI2(old_uri, true);
    ASSERT_EQ(addresses.size(), 1u);
    ASSERT_EQ((uint64_t)amounts[0].amount, 200000000000ULL);
    ASSERT_EQ(recipient_names[0], std::string("Bob"));
  }

  std::vector<std::string> addresses = { TEST_ADDRESS }, recipient_names = { "Carol" };
  std::vector<uint64_t> amounts = { 100000000000ULL };

  MAKE_URI2(new_uri, addresses, amounts, recipient_names, "donation", true);
  std::string address, payment_id, recipient_name, description;
  uint64_t amount = 0;
  std::vector<std::string> unknown_parameters;
  bool r = w.parse_uri(new_uri, address, payment_id, amount, description, recipient_name, unknown_parameters, error);
  ASSERT_TRUE(r);
  ASSERT_EQ(address, TEST_ADDRESS);
  ASSERT_EQ(amount, 100000000000ULL);
  ASSERT_EQ(recipient_name, "Carol");
}

TEST(uri, test_legacy_parse_uri_with_v2_uri)
{
  PARSE_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&address=" TEST_ADDRESS, false);
}

TEST(uri, test_parse_uri_v2_with_partial_params)
{
  {
    PARSE_URI2("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&address=" TEST_ADDRESS, true);
    ASSERT_EQ(addresses.size(), 2u);
    ASSERT_EQ((uint64_t)amounts[0].amount, 500000000000ULL);
    ASSERT_EQ((uint64_t)amounts[1].amount, 0ULL);
  }

  {
    PARSE_URI2("monero:" TEST_ADDRESS "?version=2.0&label=Alice&address=" TEST_ADDRESS, true);
    ASSERT_EQ(recipient_names.size(), 2u);
    ASSERT_EQ(recipient_names[0], "Alice");
    ASSERT_EQ(recipient_names[1], "");
  } 
}

TEST(uri, test_parse_uri_v2_with_malformed_uri)
{
  const char* bad_uris[] = {
    "monero:" TEST_INTEGRATED_ADDRESS "?version=2.0&address=" TEST_INTEGRATED_ADDRESS2,
    "monero:" TEST_ADDRESS TEST_ADDRESS "?tx_amount=0.5&recipient_name=Alice",
  };

  for (auto uri : bad_uris) {
    PARSE_URI2(uri, false);
  }
}
