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

#define PARSE_MULTI_URI(uri, expected) \
  std::vector<std::string> addresses; \
  std::vector<tools::wallet::tx_amount> amounts; \
  std::vector<std::string> recipient_names; \
  std::string description, error; \
  std::vector<std::string> unknown_parameters; \
  bool ret = tools::wallet::parse_uri(uri, addresses, amounts, recipient_names, \
                         description, unknown_parameters, error, cryptonote::TESTNET); \
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


TEST(uri, multiple_addresses_no_params)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&address=" TEST_ADDRESS, true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ(addresses[1], TEST_ADDRESS);
}

TEST(uri, multiple_addresses_with_amounts)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&address=" TEST_ADDRESS "&amount=0.2XMR", true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[0].amount, 500000000000ULL);
  ASSERT_EQ(addresses[1], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[1].amount, 200000000000ULL);
}

TEST(uri, multiple_address_with_btc_amount)
{
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description, error;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(std::string("monero:" TEST_ADDRESS "?version=2.0&amount=1BTC"), addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET);

  ASSERT_TRUE(ret);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ((uint64_t)amounts[0].amount, 100000000ULL);
  ASSERT_EQ(amounts[0].currency, std::string("BTC"));
}

TEST(uri, multiple_address_with_eth_amount)
{
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description, error;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(std::string("monero:" TEST_ADDRESS "?version=2.0&amount=100ETH"), addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET);

  ASSERT_TRUE(ret);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ(amounts[0].currency, std::string("ETH"));
  ASSERT_EQ(tools::wallet::to_string_u128(amounts[0].amount), std::string("100000000000000000000"));
}

TEST(uri, multiple_address_with_fractional_eth_amount)
{
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description, error;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(std::string("monero:" TEST_ADDRESS "?version=2.0&amount=12345.67890123456789ETH"), addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET);

  ASSERT_TRUE(ret);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ(amounts[0].currency, std::string("ETH"));
  ASSERT_EQ(tools::wallet::to_string_u128(amounts[0].amount), std::string("12345678901234567890000"));
}

TEST(uri, multiple_address_with_fractional_btc_amount)
{
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description, error;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(std::string("monero:" TEST_ADDRESS "?version=2.0&amount=0.5BTC"), addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ((uint64_t)amounts[0].amount, 50000000ULL);
  ASSERT_EQ(amounts[0].currency, std::string("BTC"));
}

TEST(uri, single_recipient_make_uri_with_btc_amount)
{
  std::vector<std::string> addresses = { TEST_ADDRESS };
  tools::wallet::tx_amount btc_amt;
  btc_amt.currency = "BTC";
  btc_amt.amount = (uint128)100000000ULL;
  std::vector<tools::wallet::tx_amount> amounts = { btc_amt };
  std::vector<std::string> names = { "Alice" };
  std::string err;
  std::string uri = tools::wallet::make_uri(addresses, amounts, names, "btc payment", err, cryptonote::TESTNET);

  ASSERT_FALSE(err.empty());
  ASSERT_TRUE(uri.empty());
}

TEST(uri, make_uri_with_btc_amount)
{
  std::vector<std::string> addresses = { TEST_ADDRESS, TEST_ADDRESS };
  tools::wallet::tx_amount btc_amt;
  btc_amt.currency = "BTC";
  btc_amt.amount = (uint128)100000000ULL; // 1 BTC -> satoshis in minor units table
  tools::wallet::tx_amount zero_xmr = { (uint128)0, std::string("XMR") };

  std::vector<tools::wallet::tx_amount> amounts = { btc_amt, zero_xmr };
  std::vector<std::string> names = { "Alice", "Bob" };
  std::string err;
  std::string uri = tools::wallet::make_uri(addresses, amounts, names, "btc multi payment", err, cryptonote::TESTNET);

  ASSERT_TRUE(err.empty()) << "make_uri returned error: " << err;
  ASSERT_FALSE(uri.empty());

  std::vector<std::string> out_addresses;
  std::vector<tools::wallet::tx_amount> out_amounts;
  std::vector<std::string> out_names;
  std::string description;
  std::vector<std::string> unknown_parameters;
  bool ret = tools::wallet::parse_uri(uri, out_addresses, out_amounts, out_names, description, unknown_parameters, err, cryptonote::TESTNET);

  ASSERT_TRUE(ret);
  ASSERT_TRUE(err.empty());
  ASSERT_EQ(out_addresses.size(), 2u);
  ASSERT_EQ(out_names.size(), 2u);
  ASSERT_EQ(out_names[0], std::string("Alice"));
  ASSERT_EQ(out_names[1], std::string("Bob"));
  ASSERT_EQ(description, std::string("btc multi payment"));
  ASSERT_EQ(out_amounts[0].currency, std::string("BTC"));
  ASSERT_EQ((uint64_t)out_amounts[0].amount, 100000000ULL);
  ASSERT_EQ(out_amounts[1].currency, std::string("XMR"));
  ASSERT_EQ((uint64_t)out_amounts[1].amount, 0ULL);
}

TEST(uri, multiple_address_with_fiat)
{
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description, error;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(std::string("monero:" TEST_ADDRESS "?version=2.0&amount=12.34EUR"), addresses, amounts, recipient_names, description, unknown_parameters, error, cryptonote::TESTNET);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(error.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ((uint64_t)amounts[0].amount, 1234ULL);
  ASSERT_EQ(amounts[0].currency, std::string("EUR"));
}

TEST(uri, multiple_addresses_with_recipient_names)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&label=Alice&address=" TEST_ADDRESS "&label=Bob", true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ(recipient_names[0], "Alice");
  ASSERT_EQ(addresses[1], TEST_ADDRESS);
  ASSERT_EQ(recipient_names[1], "Bob");
}

TEST(uri, multiple_addresses_with_mismatched_amounts)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&address=" TEST_ADDRESS, true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ((uint64_t)amounts[0].amount, 500000000000ULL);
  ASSERT_EQ((uint64_t)amounts[1].amount, 0ULL);
}

TEST(uri, multiple_integrated_addresses)
{
  PARSE_MULTI_URI("monero:" TEST_INTEGRATED_ADDRESS "?version=2.0&address=" TEST_INTEGRATED_ADDRESS2, false);
}

TEST(uri, multiple_addresses_with_mismatched_recipient_names)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&label=Alice&address=" TEST_ADDRESS, true);
  ASSERT_EQ(recipient_names.size(), 2);
  ASSERT_EQ(recipient_names[0], "Alice");
  ASSERT_EQ(recipient_names[1], "");
}

TEST(uri, multiple_addresses_with_partial_params)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&label=Alice&address=" TEST_ADDRESS "&amount=0", true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[0].amount, 500000000000ULL);
  ASSERT_EQ(recipient_names[0], "Alice");
  ASSERT_EQ(addresses[1], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[1].amount, 0ULL);
  ASSERT_EQ(recipient_names[1], "");
}

TEST(uri, multiple_addresses_with_unknown_params)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&address=" TEST_ADDRESS "&unknown_param=123;456", true);
  ASSERT_EQ(unknown_parameters.size(), 1);
  ASSERT_EQ(unknown_parameters[0], "unknown_param=123;456");
}

TEST(uri, multiple_addresses_with_description)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&address=" TEST_ADDRESS "&tx_description=Payment%20for%20services", true);
  ASSERT_EQ(description, "Payment for services");
}

TEST(uri, multiple_addresses_mismatched_params)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS TEST_ADDRESS "?tx_amount=0.5&recipient_name=Alice", false);
}

TEST(uri, multiple_addresses_all_params_correct)
{
  PARSE_MULTI_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&label=Alice&address=" TEST_ADDRESS "&amount=0.2XMR&label=Bob&tx_description=Payment%20for%20services", true);
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[0].amount, 500000000000ULL);
  ASSERT_EQ(recipient_names[0], "Alice");
  ASSERT_EQ(addresses[1], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[1].amount, 200000000000ULL);
  ASSERT_EQ(recipient_names[1], "Bob");
  ASSERT_EQ(description, "Payment for services");
}

TEST(uri, make_parse_uri_uint64_overload)
{
  std::vector<std::string> addresses = { TEST_ADDRESS };
  std::vector<uint64_t> amounts = { 250000000000ULL };
  std::vector<std::string> names = { "Dave" };
  std::string err;
  std::string uri = tools::wallet::make_uri(addresses, amounts, names, "quarter", err, cryptonote::TESTNET);
  ASSERT_TRUE(err.empty());
  ASSERT_FALSE(uri.empty());

  std::vector<std::string> out_addresses;
  std::vector<tools::wallet::tx_amount> out_amounts;
  std::vector<std::string> out_names;
  std::string description;
  std::vector<std::string> unknown_parameters;
  bool ret = tools::wallet::parse_uri(uri, out_addresses, out_amounts, out_names, description, unknown_parameters, err, cryptonote::TESTNET);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(err.empty());
  ASSERT_EQ(out_addresses.size(), 1);
  ASSERT_EQ((uint64_t)out_amounts[0].amount, 250000000000ULL);
  ASSERT_EQ(out_amounts[0].currency, std::string("XMR"));
  ASSERT_EQ(out_names[0], std::string("Dave"));
  ASSERT_EQ(description, std::string("quarter"));
}

TEST(uri, make_uri_single_recipient_compatibility)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string error_old, error_new;

  std::string old_uri = w.make_uri(TEST_ADDRESS, std::string(), 500000000000ULL, "Payment for services", "Alice", error_old);

  std::vector<std::string> addresses = { TEST_ADDRESS };
  std::vector<uint64_t> amounts = { 500000000000ULL };
  std::vector<std::string> names = { "Alice" };
  std::string new_uri = tools::wallet::make_uri(addresses, amounts, names, "Payment for services", error_new, cryptonote::TESTNET);

  ASSERT_TRUE(error_old.empty());
  ASSERT_TRUE(error_new.empty());
  ASSERT_EQ(old_uri, new_uri);
}

TEST(uri, wallet2_make_uri_new_parse_uri_compatibility)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string err;

  // generate a URI using the old wallet2::make_uri
  std::string uri = w.make_uri(TEST_ADDRESS, std::string(), 200000000000ULL, "desc", "Bob", err);
  ASSERT_TRUE(err.empty());

  // parse it with the new parse_uri (multi-recipient parser)
  std::vector<std::string> addresses;
  std::vector<tools::wallet::tx_amount> amounts;
  std::vector<std::string> recipient_names;
  std::string description;
  std::vector<std::string> unknown_parameters;

  bool ret = tools::wallet::parse_uri(uri, addresses, amounts, recipient_names, description, unknown_parameters, err, cryptonote::TESTNET);
  ASSERT_TRUE(ret);
  ASSERT_TRUE(err.empty());
  ASSERT_EQ(addresses.size(), 1);
  ASSERT_EQ(addresses[0], TEST_ADDRESS);
  ASSERT_EQ((uint64_t)amounts[0].amount, 200000000000ULL);
  ASSERT_EQ(amounts[0].currency, std::string("XMR"));
  ASSERT_EQ(recipient_names[0], std::string("Bob"));
  ASSERT_EQ(description, std::string("desc"));
  ASSERT_TRUE(unknown_parameters.empty());
}

TEST(uri, wallet2_parse_uri_rejects_v2)
{
  PARSE_URI("monero:" TEST_ADDRESS "?version=2.0&amount=0.5XMR&address=" TEST_ADDRESS, false);
}

TEST(uri, new_make_uri_wallet2_parse_uri_compatibility)
{
  tools::wallet2 w(cryptonote::TESTNET);
  std::string err;

  std::vector<std::string> addresses = { TEST_ADDRESS };
  std::vector<uint64_t> amounts = { 100000000000ULL }; // 0.1
  std::vector<std::string> names = { "Carol" };
  std::string new_uri = tools::wallet::make_uri(addresses, amounts, names, "note", err, cryptonote::TESTNET);
  ASSERT_TRUE(err.empty());

  std::string address, payment_id, recipient_name, description;
  uint64_t amount = 0;
  std::vector<std::string> unknown_parameters;
  bool ret = w.parse_uri(new_uri, address, payment_id, amount, description, recipient_name, unknown_parameters, err);

  ASSERT_TRUE(ret);
  ASSERT_TRUE(err.empty());
  ASSERT_EQ(address, TEST_ADDRESS);
  ASSERT_EQ(payment_id, std::string());
  ASSERT_EQ(amount, 100000000000ULL);
  ASSERT_EQ(recipient_name, std::string("Carol"));
  ASSERT_EQ(description, std::string("note"));
  ASSERT_TRUE(unknown_parameters.empty());
}
