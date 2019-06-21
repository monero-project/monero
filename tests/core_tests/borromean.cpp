// Copyright (c) 2014-2019, AEON, The Monero Project
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
#include "borromean.h"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_borromean_tx_validation_base::generate_with(std::vector<test_event_entry>& events, size_t num_rewind, bool use_borromean, bool valid) const
{
  const int mixin = 2;
  const int out_idx[] = {1, 2, 3, 4, 5, -1};
  const std::vector<uint64_t> paid_amounts = { 10000000000000, 2000000000000, 300000000000, 40000000000, 5000000000 };

  uint64_t ts_start = 1338224400;
  uint64_t bc_height = 1;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  // create 4 miner accounts, and have them mine the next 4 v1 blocks
  cryptonote::account_base miner_accounts[4];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[4];
  for (size_t n = 0; n < 4; ++n) {
    miner_accounts[n].generate();
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, miner_accounts[n],
        test_generator::bf_timestamp,
        0, 0, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
    ++bc_height;
  }

  // fork to v8 and rewind
  cryptonote::block blk_r;
  {
    cryptonote::block blk_last = blocks[3];
    for (size_t i = 0; i < num_rewind; ++i)
    {
      uint8_t blk_ver = bc_height < 70 ? 7 : 8;
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          blk_ver, blk_ver, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * (bc_height < 5 ? 1 : 4), // v2 has blocks four times as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, blk_ver),
          false, "Failed to generate block");
      events.push_back(blk);
      blk_last = blk;
      ++bc_height;
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
  std::vector<tx_destination_entry> destinations;
  for (uint64_t amount : paid_amounts)
  {
    td.amount = amount;
    destinations.push_back(td);
  }

  transaction tx;
  bool r = construct_tx(miner_accounts[0].get_keys(), sources, destinations, boost::none, std::vector<uint8_t>(), tx, 0, use_borromean);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");
  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  LOG_PRINT_L0("Tx size: " << get_object_blobsize(tx));

  return true;
}

bool gen_borromean_tx_valid_pre_fork_without::generate(std::vector<test_event_entry>& events) const
{
  return generate_with(events, 60, false, true);
}

bool gen_borromean_tx_invalid_pre_fork_with::generate(std::vector<test_event_entry>& events) const
{
  return generate_with(events, 60, true, false);
}

bool gen_borromean_tx_valid_post_fork_without::generate(std::vector<test_event_entry>& events) const
{
  return generate_with(events, 70, false, true);
}

bool gen_borromean_tx_valid_post_fork_with::generate(std::vector<test_event_entry>& events) const
{
  return generate_with(events, 70, true, true);
}
