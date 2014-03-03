// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once 
#include "chaingen.h"

template<size_t invalid_block_idx = 0>
class gen_block_verification_base : public test_chain_unit_base
{
public:
  gen_block_verification_base()
  {
    REGISTER_CALLBACK("check_block_purged", gen_block_verification_base<invalid_block_idx>::check_block_purged);
  }

  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/)
  {
    if (invalid_block_idx == event_idx)
      return bvc.m_verifivation_failed;
    else
      return !bvc.m_verifivation_failed;
  }

  bool check_block_purged(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
  {
    DEFINE_TESTS_ERROR_CONTEXT("gen_block_verification_base::check_block_purged");

    CHECK_TEST_CONDITION(invalid_block_idx < ev_index);
    CHECK_EQ(0, c.get_pool_transactions_count());
    CHECK_EQ(invalid_block_idx, c.get_current_blockchain_height());

    return true;
  }
};

template<size_t expected_blockchain_height>
struct gen_block_accepted_base : public test_chain_unit_base
{
  gen_block_accepted_base()
  {
    REGISTER_CALLBACK("check_block_accepted", gen_block_accepted_base::check_block_accepted);
  }

  bool check_block_accepted(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
  {
    DEFINE_TESTS_ERROR_CONTEXT("gen_block_accepted_base::check_block_accepted");

    CHECK_EQ(0, c.get_pool_transactions_count());
    CHECK_EQ(expected_blockchain_height, c.get_current_blockchain_height());

    return true;
  }
};

struct gen_block_big_major_version : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_big_minor_version : public gen_block_accepted_base<2>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_not_checked : public gen_block_accepted_base<BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_in_past : public gen_block_verification_base<BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_ts_in_future : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_invalid_prev_id : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/);
};

struct gen_block_invalid_nonce : public gen_block_verification_base<3>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_no_miner_tx : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_low : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_high : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_timestamp_in_past : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_unlock_time_is_timestamp_in_future : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_height_is_low : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_height_is_high : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_2_tx_gen_in : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_2_in : public gen_block_verification_base<CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW + 1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_with_txin_to_key : public gen_block_verification_base<CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW + 2>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_out_is_small : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_out_is_big : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_no_out : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_miner_tx_has_out_to_alice : public gen_block_accepted_base<2>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_has_invalid_tx : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_is_too_big : public gen_block_verification_base<1>
{
  bool generate(std::vector<test_event_entry>& events) const;
};

struct gen_block_invalid_binary_format : public test_chain_unit_base
{
  gen_block_invalid_binary_format();
  bool generate(std::vector<test_event_entry>& events) const;
  bool check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/);
  bool check_all_blocks_purged(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);
  bool corrupt_blocks_boundary(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events);

private:
  size_t m_corrupt_blocks_begin_idx;
};
