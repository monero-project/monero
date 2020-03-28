// Copyright (c) 2014-2020, The Monero Project
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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"

#include <vector>

#include "common/util.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/cryptonote_tx_utils.h"

namespace
{
  uint64_t const TEST_FEE = 5000000000; // 5 * 10^9
}

TEST(parse_tx_extra, handles_empty_extra)
{
  std::vector<uint8_t> extra;
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_TRUE(tx_extra_fields.empty());
}

TEST(parse_tx_extra, handles_padding_only_size_1)
{
  const uint8_t extra_arr[] = {0};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(1, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_padding), tx_extra_fields[0].type());
  ASSERT_EQ(1, boost::get<cryptonote::tx_extra_padding>(tx_extra_fields[0]).size);
}

TEST(parse_tx_extra, handles_padding_only_size_2)
{
  const uint8_t extra_arr[] = {0, 0};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(1, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_padding), tx_extra_fields[0].type());
  ASSERT_EQ(2, boost::get<cryptonote::tx_extra_padding>(tx_extra_fields[0]).size);
}

TEST(parse_tx_extra, handles_padding_only_max_size)
{
  std::vector<uint8_t> extra(TX_EXTRA_NONCE_MAX_COUNT, 0);
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(1, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_padding), tx_extra_fields[0].type());
  ASSERT_EQ(TX_EXTRA_NONCE_MAX_COUNT, boost::get<cryptonote::tx_extra_padding>(tx_extra_fields[0]).size);
}

TEST(parse_tx_extra, handles_padding_only_exceed_max_size)
{
  std::vector<uint8_t> extra(TX_EXTRA_NONCE_MAX_COUNT + 1, 0);
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_FALSE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
}

TEST(parse_tx_extra, handles_invalid_padding_only)
{
  std::vector<uint8_t> extra(2, 0);
  extra[1] = 42;
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_FALSE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
}

TEST(parse_tx_extra, handles_pub_key_only)
{
  const uint8_t extra_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(1, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_pub_key), tx_extra_fields[0].type());
}

TEST(parse_tx_extra, handles_extra_nonce_only)
{
  const uint8_t extra_arr[] = {2, 1, 42};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(1, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_nonce), tx_extra_fields[0].type());
  cryptonote::tx_extra_nonce extra_nonce = boost::get<cryptonote::tx_extra_nonce>(tx_extra_fields[0]);
  ASSERT_EQ(1, extra_nonce.nonce.size());
  ASSERT_EQ(42, extra_nonce.nonce[0]);
}

TEST(parse_tx_extra, handles_pub_key_and_padding)
{
  const uint8_t extra_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(extra, tx_extra_fields));
  ASSERT_EQ(2, tx_extra_fields.size());
  ASSERT_EQ(typeid(cryptonote::tx_extra_pub_key), tx_extra_fields[0].type());
  ASSERT_EQ(typeid(cryptonote::tx_extra_padding), tx_extra_fields[1].type());
}

TEST(parse_and_validate_tx_extra, is_valid_tx_extra_parsed)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  cryptonote::account_base acc;
  acc.generate();
  cryptonote::blobdata b = "dsdsdfsdfsf";
  ASSERT_TRUE(cryptonote::construct_miner_tx(0, 0, 10000000000000, 1000, TEST_FEE, acc.get_keys().m_account_address, tx, b, 1));
  crypto::public_key tx_pub_key = cryptonote::get_tx_pub_key_from_extra(tx);
  ASSERT_NE(tx_pub_key, crypto::null_pkey);
}
TEST(parse_and_validate_tx_extra, fails_on_big_extra_nonce)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  cryptonote::account_base acc;
  acc.generate();
  cryptonote::blobdata b(TX_EXTRA_NONCE_MAX_COUNT + 1, 0);
  ASSERT_FALSE(cryptonote::construct_miner_tx(0, 0, 10000000000000, 1000, TEST_FEE, acc.get_keys().m_account_address, tx, b, 1));
}
TEST(parse_and_validate_tx_extra, fails_on_wrong_size_in_extra_nonce)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  tx.extra[0] = TX_EXTRA_NONCE;
  tx.extra[1] = 255;
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  ASSERT_FALSE(cryptonote::parse_tx_extra(tx.extra, tx_extra_fields));
}
TEST(validate_parse_amount_case, validate_parse_amount)
{
  uint64_t res = 0;
  bool r = cryptonote::parse_amount(res, "0.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000);

  r = cryptonote::parse_amount(res, "100.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = cryptonote::parse_amount(res, "000.0000");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);

  r = cryptonote::parse_amount(res, "0");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);


  r = cryptonote::parse_amount(res, "   100.0001    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000100000000);

  r = cryptonote::parse_amount(res, "   100.0000    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 100000000000000);

  r = cryptonote::parse_amount(res, "   100. 0000    ");
  ASSERT_FALSE(r);

  r = cryptonote::parse_amount(res, "100. 0000");
  ASSERT_FALSE(r);

  r = cryptonote::parse_amount(res, "100 . 0000");
  ASSERT_FALSE(r);

  r = cryptonote::parse_amount(res, "100.00 00");
  ASSERT_FALSE(r);

  r = cryptonote::parse_amount(res, "1 00.00 00");
  ASSERT_FALSE(r);
}

TEST(sort_tx_extra, empty)
{
  std::vector<uint8_t> extra, sorted;
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted));
  ASSERT_EQ(extra, sorted);
}

TEST(sort_tx_extra, pubkey)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted));
  ASSERT_EQ(extra, sorted);
}

TEST(sort_tx_extra, two_pubkeys)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230,
    1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted));
  ASSERT_EQ(extra, sorted);
}

TEST(sort_tx_extra, keep_order)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230,
    2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted));
  ASSERT_EQ(extra, sorted);
}

TEST(sort_tx_extra, switch_order)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230};
  const uint8_t expected_arr[] = {1, 30, 208, 98, 162, 133, 64, 85, 83, 112, 91, 188, 89, 211, 24, 131, 39, 154, 22, 228,
    80, 63, 198, 141, 173, 111, 244, 183, 4, 149, 186, 140, 230,
    2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted));
  std::vector<uint8_t> expected(&expected_arr[0], &expected_arr[0] + sizeof(expected_arr));
  ASSERT_EQ(expected, sorted);
}

TEST(sort_tx_extra, invalid)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {1};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_FALSE(cryptonote::sort_tx_extra(extra, sorted));
}

TEST(sort_tx_extra, invalid_suffix_strict)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_FALSE(cryptonote::sort_tx_extra(extra, sorted));
}

TEST(sort_tx_extra, invalid_suffix_partial)
{
  std::vector<uint8_t> sorted;
  const uint8_t extra_arr[] = {2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  const uint8_t expected_arr[] = {2, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1};
  std::vector<uint8_t> extra(&extra_arr[0], &extra_arr[0] + sizeof(extra_arr));
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted, true));
  std::vector<uint8_t> expected(&expected_arr[0], &expected_arr[0] + sizeof(expected_arr));
  ASSERT_EQ(sorted, expected);
}

TEST(tx_extra_recipient_private_data, works)
{
  std::vector<uint8_t> extra;
  ASSERT_TRUE(cryptonote::add_recipient_private_data_to_tx_extra(extra, "foobar", false));
  const uint8_t expected_arr[] = {5, 0, 1, 6, 'f', 'o', 'o', 'b', 'a', 'r'};
  std::vector<uint8_t> expected(&expected_arr[0], &expected_arr[0] + sizeof(expected_arr));
  ASSERT_EQ(extra, expected);

  // encrypt
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  auto &hwdev = hw::get_device("default");
  cryptonote::keypair recipient_keys = cryptonote::keypair::generate(hwdev), tx_keys = cryptonote::keypair::generate(hwdev);
  ASSERT_TRUE(parse_tx_extra(extra, tx_extra_fields));
  cryptonote::tx_extra_recipient_private_data ec;
  ASSERT_TRUE(find_tx_extra_field_by_type(tx_extra_fields, ec));
  ASSERT_TRUE(cryptonote::encrypt_recipient_private_data(ec.data, recipient_keys.pub, tx_keys.sec, hwdev));
  cryptonote::remove_field_from_tx_extra(extra, typeid(cryptonote::tx_extra_recipient_private_data));
  ASSERT_TRUE(cryptonote::add_recipient_private_data_to_tx_extra(extra, ec.data, true));
  ASSERT_TRUE(memmem("foobar", 6, extra.data(), extra.size()) == NULL);
  ASSERT_EQ(extra[0], 5);
  ASSERT_EQ(extra[1], 1);

  // decrypt
  ASSERT_TRUE(parse_tx_extra(extra, tx_extra_fields));
  ASSERT_TRUE(find_tx_extra_field_by_type(tx_extra_fields, ec));
  ASSERT_TRUE(cryptonote::decrypt_recipient_private_data(ec.data, tx_keys.pub, recipient_keys.sec, hwdev));
  cryptonote::remove_field_from_tx_extra(extra, typeid(cryptonote::tx_extra_recipient_private_data));
  ASSERT_TRUE(cryptonote::add_recipient_private_data_to_tx_extra(extra, ec.data, false));
  ASSERT_EQ(extra, expected);

  auto mkstring = [](size_t n){ std::string s(n, 0); for (size_t i = 0; i < n; ++i) s[i] = i; return s; };

  // check we can use data of any size > 0 to max
  std::string Sempty;
  ASSERT_FALSE(cryptonote::encrypt_recipient_private_data(Sempty, recipient_keys.pub, tx_keys.sec, hwdev));
  for (uint32_t i = 0; i < 3 * TX_EXTRA_RECIPIENT_PRIVATE_DATA_GRANULARITY + 1; ++i)
  {
    uint32_t sz = i ? i : 255 * TX_EXTRA_RECIPIENT_PRIVATE_DATA_GRANULARITY;
    std::string S = mkstring(sz);
    ASSERT_TRUE(cryptonote::encrypt_recipient_private_data(S, recipient_keys.pub, tx_keys.sec, hwdev));
    cryptonote::remove_field_from_tx_extra(extra, typeid(cryptonote::tx_extra_recipient_private_data));
    ASSERT_TRUE(cryptonote::add_recipient_private_data_to_tx_extra(extra, S, true));
    ASSERT_TRUE(parse_tx_extra(extra, tx_extra_fields));
    ASSERT_TRUE(find_tx_extra_field_by_type(tx_extra_fields, ec));
    ASSERT_EQ(ec.data.size(), 1 + (sz + TX_EXTRA_RECIPIENT_PRIVATE_DATA_GRANULARITY - 1) / TX_EXTRA_RECIPIENT_PRIVATE_DATA_GRANULARITY * TX_EXTRA_RECIPIENT_PRIVATE_DATA_GRANULARITY);
    ASSERT_TRUE(cryptonote::decrypt_recipient_private_data(ec.data, tx_keys.pub, recipient_keys.sec, hwdev));
    ASSERT_EQ(ec.data, mkstring(sz));
  }
}
