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
#include "wallet/uri_syntax.h"

namespace
{
  using tools::wallet::payment_uri;
  using tools::wallet::uri_recipient;
  using tools::wallet::parse_uri_syntax;
  using tools::wallet::make_uri_syntax;

  uri_recipient recipient(const std::string& address, const std::string& amount = "",
    const std::string& currency = "XMR", const std::string& label = "")
  {
    uri_recipient result;
    result.address = address;
    result.amount = amount;
    result.currency = currency;
    result.label = label;
    return result;
  }

  void expect_same(const payment_uri& expected, const payment_uri& actual)
  {
    ASSERT_EQ(expected.recipients.size(), actual.recipients.size());
    for (std::size_t i = 0; i < expected.recipients.size(); ++i)
    {
      SCOPED_TRACE(i);
      EXPECT_EQ(expected.recipients[i].address, actual.recipients[i].address);
      EXPECT_EQ(expected.recipients[i].amount, actual.recipients[i].amount);
      EXPECT_EQ(expected.recipients[i].currency, actual.recipients[i].currency);
      EXPECT_EQ(expected.recipients[i].label, actual.recipients[i].label);
    }
    EXPECT_EQ(expected.tx_description, actual.tx_description);
    EXPECT_EQ(expected.unknown_parameters, actual.unknown_parameters);
  }
}

TEST(uri_multi, three_recipients_preserve_missing_zero_units_and_text)
{
  payment_uri original;
  original.recipients.push_back(recipient("A1", "", "XMR", "amount left open"));
  original.recipients.push_back(recipient("B2", "0", "ETH", "a+b & caf\xc3\xa9%26"));
  original.recipients.push_back(recipient("C3", "00012345678901234567890.0000000000000001", "EUR", "name=value"));
  original.tx_description = "description & = % +";
  original.unknown_parameters = {"extension=%26%3d%2526", "empty="};
  std::string error = "old error";
  const std::string encoded = make_uri_syntax(original, error);
  ASSERT_FALSE(encoded.empty()) << error;
  EXPECT_TRUE(error.empty());
  EXPECT_EQ(encoded, "monero:A1?version=2.0&label=amount%20left%20open"
    "&address=B2&amount=0ETH&label=a%2Bb%20%26%20caf%C3%A9%2526"
    "&address=C3&amount=00012345678901234567890.0000000000000001EUR&label=name%3Dvalue"
    "&tx_description=description%20%26%20%3D%20%25%20%2B&extension=%26%3d%2526&empty=");
  payment_uri parsed;
  ASSERT_TRUE(parse_uri_syntax(encoded, parsed, error)) << error;
  expect_same(original, parsed);
}

TEST(uri_multi, legacy_single_recipient_and_unknown_parameters)
{
  payment_uri parsed;
  std::string error;
  ASSERT_TRUE(parse_uri_syntax("monero:A1?tx_amount=000.011000&recipient_name=Alice%20Example"
    "&tx_description=Invoice%3D1&extension=%2526&empty=", parsed, error)) << error;
  ASSERT_EQ(1u, parsed.recipients.size());
  EXPECT_EQ("A1", parsed.recipients[0].address);
  EXPECT_EQ("000.011000", parsed.recipients[0].amount);
  EXPECT_EQ("XMR", parsed.recipients[0].currency);
  EXPECT_EQ("Alice Example", parsed.recipients[0].label);
  EXPECT_EQ("Invoice=1", parsed.tx_description);
  EXPECT_EQ((std::vector<std::string>{"extension=%2526", "empty="}), parsed.unknown_parameters);
  EXPECT_EQ("monero:A1?version=2.0&amount=000.011000XMR&label=Alice%20Example"
    "&tx_description=Invoice%3D1&extension=%2526&empty=", make_uri_syntax(parsed, error));
}

TEST(uri_multi, syntax_layer_does_not_claim_address_validation)
{
  payment_uri parsed;
  std::string error;
  // This is a token, deliberately not a real address. The network adapter must reject it.
  ASSERT_TRUE(parse_uri_syntax("monero:NotARealAddress0", parsed, error)) << error;
  ASSERT_EQ(1u, parsed.recipients.size());
  EXPECT_TRUE(parsed.recipients[0].amount.empty());
  EXPECT_EQ("monero:NotARealAddress0?version=2.0", make_uri_syntax(parsed, error));
  ASSERT_TRUE(parse_uri_syntax("monero:A1?", parsed, error)) << error;
}

TEST(uri_multi, decimal_spelling_and_supported_currencies)
{
  const char* values[] = {".5XMR", "1.BTC", "000.0100ETH", "0USD", "12.34EUR", "5"};
  const char* amounts[] = {".5", "1.", "000.0100", "0", "12.34", "5"};
  const char* currencies[] = {"XMR", "BTC", "ETH", "USD", "EUR", "XMR"};
  for (std::size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
  {
    SCOPED_TRACE(values[i]);
    payment_uri parsed;
    std::string error;
    ASSERT_TRUE(parse_uri_syntax(std::string("monero:A1?version=2.0&amount=") + values[i], parsed, error)) << error;
    ASSERT_EQ(1u, parsed.recipients.size());
    EXPECT_EQ(amounts[i], parsed.recipients[0].amount);
    EXPECT_EQ(currencies[i], parsed.recipients[0].currency);
  }
}

TEST(uri_multi, aliases_and_repeated_attributes_on_distinct_recipients)
{
  payment_uri parsed;
  std::string error;
  ASSERT_TRUE(parse_uri_syntax("monero:A1?version=2.0&tx_amount=1BTC&recipient_name=Alice"
    "&address=B2&amount=2USD&label=Bob&tx_description=shared&address=C3&amount=0&label=Carol",
    parsed, error)) << error;
  ASSERT_EQ(3u, parsed.recipients.size());
  EXPECT_EQ("BTC", parsed.recipients[0].currency);
  EXPECT_EQ("USD", parsed.recipients[1].currency);
  EXPECT_EQ("XMR", parsed.recipients[2].currency);
  EXPECT_EQ("Carol", parsed.recipients[2].label);
  EXPECT_EQ("shared", parsed.tx_description);
}

TEST(uri_multi, text_is_decoded_once_and_plus_is_literal)
{
  payment_uri parsed;
  std::string error;
  ASSERT_TRUE(parse_uri_syntax("monero:A1?version=2.0&label=a+b%26%3d%2526"
    "&tx_description=%F0%9F%8C%8D&extension=%2525", parsed, error)) << error;
  EXPECT_EQ("a+b&=%26", parsed.recipients[0].label);
  EXPECT_EQ("\xf0\x9f\x8c\x8d", parsed.tx_description);
  EXPECT_EQ("monero:A1?version=2.0&label=a%2Bb%26%3D%2526"
    "&tx_description=%F0%9F%8C%8D&extension=%2525", make_uri_syntax(parsed, error));
}

TEST(uri_multi, recipient_does_not_inherit_previous_attributes)
{
  payment_uri parsed;
  std::string error;
  const std::string encoded = "monero:A1?version=2.0&amount=1BTC&label=first&address=B2";
  ASSERT_TRUE(parse_uri_syntax(encoded, parsed, error)) << error;
  ASSERT_EQ(2u, parsed.recipients.size());
  EXPECT_EQ("1", parsed.recipients[0].amount);
  EXPECT_EQ("BTC", parsed.recipients[0].currency);
  EXPECT_EQ("first", parsed.recipients[0].label);
  EXPECT_EQ("B2", parsed.recipients[1].address);
  EXPECT_TRUE(parsed.recipients[1].amount.empty());
  EXPECT_EQ("XMR", parsed.recipients[1].currency);
  EXPECT_TRUE(parsed.recipients[1].label.empty());
  EXPECT_EQ(encoded, make_uri_syntax(parsed, error));
}

TEST(uri_multi, encoded_text_bytes_roundtrip_without_unicode_normalization)
{
  payment_uri parsed;
  std::string error;
  const std::string encoded = "monero:A1?version=2.0&label=%00%0A%FF";
  ASSERT_TRUE(parse_uri_syntax(encoded, parsed, error)) << error;
  EXPECT_EQ(std::string("\0\n\xff", 3), parsed.recipients[0].label);
  EXPECT_EQ(encoded, make_uri_syntax(parsed, error));
  const unsigned char raw_bytes[] = {0, 10, 255};
  for (unsigned char byte : raw_bytes)
  {
    const std::string raw = std::string("monero:A1?version=2.0&label=") + static_cast<char>(byte);
    EXPECT_FALSE(parse_uri_syntax(raw, parsed, error));
    EXPECT_TRUE(parsed.recipients.empty());
    EXPECT_FALSE(error.empty());
  }
}

TEST(uri_multi, failure_clears_partial_and_previous_results)
{
  payment_uri parsed;
  std::string error = "old error";
  ASSERT_TRUE(parse_uri_syntax("monero:A1?version=2.0&amount=1XMR&extension=old", parsed, error));
  ASSERT_TRUE(error.empty());
  EXPECT_FALSE(parse_uri_syntax("monero:A1?version=2.0&amount=1XMR&tx_description=old"
    "&extension=old&address=B2&label=%GG", parsed, error));
  EXPECT_TRUE(parsed.recipients.empty());
  EXPECT_TRUE(parsed.tx_description.empty());
  EXPECT_TRUE(parsed.unknown_parameters.empty());
  EXPECT_FALSE(error.empty());
  ASSERT_TRUE(parse_uri_syntax("monero:C3", parsed, error)) << error;
  EXPECT_TRUE(error.empty());
  ASSERT_EQ(1u, parsed.recipients.size());
  EXPECT_EQ("C3", parsed.recipients[0].address);
  EXPECT_TRUE(parsed.recipients[0].amount.empty());
}

TEST(uri_multi, rejects_invalid_syntax_and_conflicting_fields)
{
  const char* invalid[] = {
    "", "MONERO:A1", " monero:A1", "monero:", "monero:?version=2.0", "monero:A1;B2",
    "monero:A1#fragment", "monero:A%31", "monero:A 1", "monero:A1/B2",
    "monero:A1?version=1.0", "monero:A1?version=2", "monero:A1?version=2%2E0",
    "monero:A1?foo=bar&version=2.0", "monero:A1?version=2.0&version=2.0",
    "monero:A1?amount=1", "monero:A1?label=Alice", "monero:A1?address=B2",
    "monero:A1?tx_amount=1XMR", "monero:A1?tx_amount=", "monero:A1?tx_payment_id=123",
    "monero:A1?version=2.0&tx_payment_id=123", "monero:A1?tx%5Fpayment_id=123",
    "monero:A1?version=2.0&address=", "monero:A1?version=2.0&address=B%32",
    "monero:A1?version=2.0&amount=1&amount=2", "monero:A1?version=2.0&amount=1&tx_amount=2",
    "monero:A1?version=2.0&tx_amount=1&amount=2", "monero:A1?version=2.0&label=&label=two",
    "monero:A1?version=2.0&label=one&recipient_name=two",
    "monero:A1?version=2.0&recipient_name=one&label=two",
    "monero:A1?version=2.0&tx_description=&address=B2&tx_description=again",
    "monero:A1?unknown=1&unknown=2", "monero:A1?version=2.0&x=1&address=B2&x=2",
    "monero:A1?version=2.0&label=%", "monero:A1?version=2.0&label=%0",
    "monero:A1?version=2.0&label=%GG", "monero:A1?version=2.0&label=raw space",
    "monero:A1?version=2.0&label=raw\nnewline", "monero:A1?version=2.0&label=raw\xc3\xa9",
    "monero:A1?version=2.0&label=a=b", "monero:A1?version=2.0&label=a#b",
    "monero:A1?bad=%0", "monero:A1?=value", "monero:A1?flag", "monero:A1?x=y&",
    "monero:A1?&x=y", "monero:A1?x=y&&z=q"
  };
  for (const char* value : invalid)
  {
    SCOPED_TRACE(value);
    payment_uri parsed;
    parsed.recipients.push_back(recipient("old", "1"));
    parsed.tx_description = "old";
    parsed.unknown_parameters.push_back("old=value");
    std::string error = "stale";
    EXPECT_FALSE(parse_uri_syntax(value, parsed, error));
    EXPECT_TRUE(parsed.recipients.empty());
    EXPECT_TRUE(parsed.tx_description.empty());
    EXPECT_TRUE(parsed.unknown_parameters.empty());
    EXPECT_FALSE(error.empty());
    EXPECT_NE("stale", error);
  }
}

TEST(uri_multi, rejects_malformed_amounts_without_numeric_conversion)
{
  const char* invalid[] = {"", ".", "XMR", ".XMR", "-1XMR", "+1XMR", "1e2XMR", "1.2.3XMR",
    "1xmr", "1USDC", "1 XMR", "1%2EXMR", "NaNXMR", "InfinityXMR", "1XMRextra", "\xd9\xa1XMR"};
  for (const char* amount : invalid)
  {
    SCOPED_TRACE(amount);
    payment_uri parsed;
    std::string error;
    EXPECT_FALSE(parse_uri_syntax(std::string("monero:A1?version=2.0&amount=") + amount, parsed, error));
    EXPECT_TRUE(parsed.recipients.empty());
    EXPECT_FALSE(error.empty());
  }
}

TEST(uri_multi, generator_rejects_invalid_models_and_parameter_collisions)
{
  std::string error;
  payment_uri model;
  EXPECT_TRUE(make_uri_syntax(model, error).empty());
  EXPECT_FALSE(error.empty());
  const uri_recipient invalid[] = {
    recipient(""), recipient("A1&address=B2"), recipient("A1", "."), recipient("A1", "1XMR"),
    recipient("A1", "1", "USDC"), recipient("A1", "", "BTC"), recipient("A1", "-1")
  };
  for (const uri_recipient& value : invalid)
  {
    model.recipients = {recipient("A1"), value};
    EXPECT_TRUE(make_uri_syntax(model, error).empty());
    EXPECT_FALSE(error.empty());
  }
  const char* invalid_unknown[] = {"version=2.0", "address=B2", "amount=1", "tx_amount=1",
    "label=other", "recipient_name=other", "tx_description=other", "tx_payment_id=123",
    "tx%5Fpayment_id=123", "x=value&address=B2", "x=a=b", "x=%", "=value", "flag"};
  model.recipients = {recipient("A1", "0")};
  for (const char* value : invalid_unknown)
  {
    SCOPED_TRACE(value);
    model.unknown_parameters = {value};
    EXPECT_TRUE(make_uri_syntax(model, error).empty());
    EXPECT_FALSE(error.empty());
  }
  model.unknown_parameters = {"x=1", "x=2"};
  EXPECT_TRUE(make_uri_syntax(model, error).empty());
  EXPECT_FALSE(error.empty());
  model.unknown_parameters.clear();
  EXPECT_EQ("monero:A1?version=2.0&amount=0XMR", make_uri_syntax(model, error));
  EXPECT_TRUE(error.empty());
}
