// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"

#include "common/util.h"
#include "cryptonote_core/cryptonote_format_utils.h"

TEST(parse_and_validate_tx_extra, is_correct_parse_and_validate_tx_extra)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  cryptonote::account_base acc;
  acc.generate();
  cryptonote::blobdata b = "dsdsdfsdfsf";
  bool r = cryptonote::construct_miner_tx(0, 0, 10000000000000, 1000, DEFAULT_FEE, acc.get_keys().m_account_address, tx, b, 1);
  ASSERT_TRUE(r);
  crypto::public_key tx_pub_key;
  r = cryptonote::parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_TRUE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_extranonce_too_big)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  cryptonote::account_base acc;
  acc.generate();
  cryptonote::blobdata b(260, 0);
  bool r = cryptonote::construct_miner_tx(0, 0, 10000000000000, 1000, DEFAULT_FEE, acc.get_keys().m_account_address, tx, b, 1);
  ASSERT_FALSE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_wrong_extra_couner_too_big)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  tx.extra[0] = TX_EXTRA_NONCE;
  tx.extra[1] = 255;
  crypto::public_key tx_pub_key;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_FALSE(r);
}
TEST(parse_and_validate_tx_extra, is_correct_wrong_extra_nonce_double_entry)
{
  cryptonote::transaction tx = AUTO_VAL_INIT(tx);
  tx.extra.resize(20, 0);
  cryptonote::blobdata v = "asasdasd";
  cryptonote::add_tx_extra_nonce(tx, v);
  cryptonote::add_tx_extra_nonce(tx, v);
  crypto::public_key tx_pub_key;
  bool r = parse_and_validate_tx_extra(tx, tx_pub_key);
  ASSERT_FALSE(r);
}

TEST(validate_parse_amount_case, validate_parse_amount)
{
  uint64_t res = 0;
  bool r = cryptonote::parse_amount(res, "0.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 10000);

  r = cryptonote::parse_amount(res, "100.0001");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 10000010000);

  r = cryptonote::parse_amount(res, "000.0000");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);

  r = cryptonote::parse_amount(res, "0");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 0);


  r = cryptonote::parse_amount(res, "   100.0001    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 10000010000);

  r = cryptonote::parse_amount(res, "   100.0000    ");
  ASSERT_TRUE(r);
  ASSERT_EQ(res, 10000000000);

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
