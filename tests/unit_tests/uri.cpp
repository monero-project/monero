// Copyright (c) 2016-2018, The Monero Project
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
  PARSE_URI("monero:" TEST_ADDRESS"?tx_payment_id=1234567890123456", true);
  ASSERT_EQ(address, TEST_ADDRESS);
  ASSERT_EQ(payment_id, "1234567890123456");
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

