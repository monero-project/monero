// Copyright (c) 2014-2018, The Monero Project
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

#include "chaingen.h"
#include "v2_tests.h"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_v2_tx_validation_base::generate_with(std::vector<test_event_entry>& events, const int *out_idx, int mixin, uint64_t amount_paid, bool valid) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  // create 4 miner accounts, and have them mine the next 4 blocks
  cryptonote::account_base miner_accounts[4];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[4];
  for (size_t n = 0; n < 4; ++n) {
    miner_accounts[n].generate();
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, miner_accounts[n],
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp,
        2, 2, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
  }

  // rewind
  cryptonote::block blk_r;
  {
    cryptonote::block blk_last = blocks[3];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp,
          2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0),
          false, "Failed to generate block");
      events.push_back(blk);
      blk_last = blk;
    }
    blk_r = blk_last;
  }

  // create a tx with the Nth outputs of miner's block reward
  std::vector<tx_source_entry> sources;
  for (size_t out_idx_idx = 0; out_idx[out_idx_idx] >= 0; ++out_idx_idx) {
    sources.resize(sources.size()+1);
    tx_source_entry& src = sources.back();

    src.amount = blocks[0].miner_tx.vout[out_idx[out_idx_idx]].amount;
  std::cout << "using " << print_money(src.amount) << " output at index " << out_idx[out_idx_idx] << std::endl;
    for (int m = 0; m <= mixin; ++m) {
      int idx;
      if (is_valid_decomposed_amount(src.amount))
        idx = m+1; // one out of that size per miner tx, including genesis
      else
        idx = 0; // dusty, no other output of that size
      src.push_output(idx, boost::get<txout_to_key>(blocks[m].miner_tx.vout[out_idx[out_idx_idx]].target).key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[0].miner_tx);
    src.real_output = 0;
    src.rct = false;
    src.real_output_in_tx_index = out_idx[out_idx_idx];
  }

  //fill outputs entry
  tx_destination_entry td;
  td.addr = miner_account.get_keys().m_account_address;
  td.amount = amount_paid;
  std::vector<tx_destination_entry> destinations;
  destinations.push_back(td);

  transaction tx;
  bool r = construct_tx(miner_accounts[0].get_keys(), sources, destinations, boost::none, std::vector<uint8_t>(), tx, 0);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");
  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);

  return true;
}

bool gen_v2_tx_mixable_0_mixin::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 0;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false);
}

bool gen_v2_tx_mixable_low_mixin::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 1;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false);
}

bool gen_v2_tx_unmixable_only::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 0;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, true);
}

bool gen_v2_tx_unmixable_one::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 0;
  const int out_idx[] = {0, 1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, true);
}

bool gen_v2_tx_unmixable_two::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 0;
  const int out_idx[] = {0, 1, 2, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false);
}

bool gen_v2_tx_dust::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10001;
  return generate_with(events, out_idx, mixin, amount_paid, false);
}
