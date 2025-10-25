// Copyright (c) 2014-2024, The Monero Project
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

#include "ringct/rctSigs.h"
#include "chaingen.h"
#include "rct.h"
#include "device/device.hpp"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_rct_tx_validation_base::generate_with_full(std::vector<test_event_entry>& events,
    const int *out_idx, int mixin, uint64_t amount_paid, size_t second_rewind, uint8_t last_version, const rct::RCTConfig &rct_config, bool use_view_tags, bool valid,
    const std::function<void(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations)> &pre_tx,
    const std::function<void(transaction &tx)> &post_tx) const
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
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        2, 2, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
  }

  // rewind
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[3];
    for (size_t i = 0; i < CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
          false, "Failed to generate block");
      events.push_back(blk);
      blk_last = blk;
    }
    blk_r = blk_last;
  }

  // create 4 txes from these miners in another block, to generate some rct outputs
  transaction rct_txes[4];
  rct::key rct_tx_masks[16];
  cryptonote::block blk_txes[4];
  for (size_t n = 0; n < 4; ++n)
  {
    std::vector<crypto::hash> starting_rct_tx_hashes;
    std::vector<tx_source_entry> sources;

    sources.resize(1);
    tx_source_entry& src = sources.back();

    const size_t index_in_tx = 5;
    src.amount = 30000000000000;
    for (int m = 0; m < 4; ++m) {
      crypto::public_key output_public_key;
      cryptonote::get_output_public_key(blocks[m].miner_tx.vout[index_in_tx], output_public_key);
      src.push_output(m, output_public_key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[n].miner_tx);
    src.real_output = n;
    src.real_output_in_tx_index = index_in_tx;
    src.mask = rct::identity();
    src.rct = false;

    //fill outputs entry
    tx_destination_entry td;
    td.addr = miner_accounts[n].get_keys().m_account_address;
    td.amount = 7390000000000;
    std::vector<tx_destination_entry> destinations;
    destinations.push_back(td);
    destinations.push_back(td);
    destinations.push_back(td);
    destinations.push_back(td); // 30 -> 7.39 * 4

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[miner_accounts[n].get_keys().m_account_address.m_spend_public_key] = {0,0};
    bool r = construct_tx_and_get_tx_key(miner_accounts[n].get_keys(), subaddresses, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), rct_txes[n], tx_key, additional_tx_keys, true);
    CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");
    events.push_back(rct_txes[n]);
    starting_rct_tx_hashes.push_back(get_transaction_hash(rct_txes[n]));

    for (size_t o = 0; o < 4; ++o)
    {
      crypto::key_derivation derivation;
      bool r = crypto::generate_key_derivation(destinations[o].addr.m_view_public_key, tx_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      crypto::secret_key amount_key;
      crypto::derivation_to_scalar(derivation, o, amount_key);
      const uint8_t type = rct_txes[n].rct_signatures.type;
      if (rct::is_rct_simple(type))
        rct::decodeRctSimple(rct_txes[n].rct_signatures, rct::sk2rct(amount_key), o, rct_tx_masks[o+n*4], hw::get_device("default"));
      else
        rct::decodeRct(rct_txes[n].rct_signatures, rct::sk2rct(amount_key), o, rct_tx_masks[o+n*4], hw::get_device("default"));
    }

    uint64_t fee = 0;
    get_tx_fee(rct_txes[n], fee);

    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk_txes[n], blk_last, miner_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
        4, 4, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0, 6, 4, fee),
        false, "Failed to generate block");
    events.push_back(blk_txes[n]);
    blk_last = blk_txes[n];
  }

  // rewind
  {
    for (size_t i = 0; i < second_rewind; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk, blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version | test_generator::bf_max_outs,
          last_version, last_version, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 6, last_version),
          false, "Failed to generate block");
      events.push_back(blk);
      blk_last = blk;
    }
    blk_r = blk_last;
  }

  // create a tx from the requested ouputs
  std::vector<tx_source_entry> sources;
  size_t global_rct_idx = 6; // skip first coinbase (6 outputs)
  size_t rct_idx = 0;
  size_t pre_rct_idx = 0;
  for (size_t out_idx_idx = 0; out_idx[out_idx_idx] >= 0; ++out_idx_idx) {
    sources.resize(sources.size()+1);
    tx_source_entry& src = sources.back();

    src.real_output = 0;
    if (out_idx[out_idx_idx]) {
      // rct
      src.amount = 7390000000000;
      src.real_out_tx_key = get_tx_pub_key_from_extra(rct_txes[rct_idx/4]);
      src.real_output_in_tx_index = rct_idx&3;
      src.mask = rct_tx_masks[rct_idx];
      src.rct = true;
      for (int m = 0; m <= mixin; ++m) {
        rct::ctkey ctkey;
        ctkey.dest = rct::pk2rct(boost::get<txout_to_key>(rct_txes[rct_idx/4].vout[rct_idx&3].target).key);
        ctkey.mask = rct_txes[rct_idx/4].rct_signatures.outPk[rct_idx&3].mask;
        src.outputs.push_back(std::make_pair(global_rct_idx, ctkey));
        ++rct_idx;
        ++global_rct_idx;
        if (global_rct_idx % 10 == 0)
          global_rct_idx += 6; // skip the coinbase
      }
    }
    else
    {
      // pre rct
      src.amount = 5000000000000;
      src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[pre_rct_idx].miner_tx);
      src.real_output_in_tx_index = 4;
      src.mask = rct::identity();
      src.rct = false;
      for (int m = 0; m <= mixin; ++m) {
        src.push_output(m, boost::get<txout_to_key>(blocks[pre_rct_idx].miner_tx.vout[4].target).key, src.amount);
        ++pre_rct_idx;
      }
    }
  }

  //fill outputs entry
  tx_destination_entry td;
  td.addr = miner_account.get_keys().m_account_address;
  td.amount = amount_paid;
  std::vector<tx_destination_entry> destinations;
  // from v12, we need two outputs at least
  destinations.push_back(td);
  destinations.push_back(td);

  if (pre_tx)
    pre_tx(sources, destinations);

  transaction tx;
  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  subaddresses[miner_accounts[0].get_keys().m_account_address.m_spend_public_key] = {0,0};
  bool r = construct_tx_and_get_tx_key(miner_accounts[0].get_keys(), subaddresses, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), tx, tx_key, additional_tx_keys, true, rct_config, use_view_tags);
  CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");

  if (post_tx)
    post_tx(tx);

  if (!valid)
    DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  LOG_PRINT_L0("Test tx: " << obj_to_json_str(tx));

  return true;
}

bool gen_rct_tx_validation_base::generate_with(std::vector<test_event_entry>& events,
    const int *out_idx, int mixin, uint64_t amount_paid, bool valid,
    const std::function<void(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations)> &pre_tx,
    const std::function<void(transaction &tx)> &post_tx) const
{
  const rct::RCTConfig rct_config { rct::RangeProofBorromean, 0 };
  bool use_view_tags = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, 4, rct_config, use_view_tags, valid, pre_tx, post_tx);
}

bool gen_rct_tx_valid_from_pre_rct::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, true, NULL, NULL);
}

bool gen_rct_tx_valid_from_rct::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, true, NULL, NULL);
}

bool gen_rct_tx_valid_from_mixed::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, 0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, true, NULL, NULL);
}

bool gen_rct_tx_pre_rct_bad_real_dest::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  bool tx_creation_succeeded = false;
  // in the case, the tx will fail to create, due to mismatched sk/pk
  bool ret = generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {rct::key sk; rct::skpkGen(sk, sources[0].outputs[0].second.dest);},
    [&tx_creation_succeeded](const transaction &tx){tx_creation_succeeded=true;});
  return !ret && !tx_creation_succeeded;
}

bool gen_rct_tx_pre_rct_bad_real_mask::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {sources[0].outputs[0].second.mask = rct::zeroCommitVartime(99999);},
    NULL);
}

bool gen_rct_tx_pre_rct_bad_fake_dest::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {rct::key sk; rct::skpkGen(sk, sources[0].outputs[1].second.dest);},
    NULL);
}

bool gen_rct_tx_pre_rct_bad_fake_mask::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {sources[0].outputs[1].second.mask = rct::zeroCommitVartime(99999);},
    NULL);
}

bool gen_rct_tx_rct_bad_real_dest::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  bool tx_creation_succeeded = false;
  // in the case, the tx will fail to create, due to mismatched sk/pk
  bool ret = generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {rct::key sk; rct::skpkGen(sk, sources[0].outputs[0].second.dest);},
    [&tx_creation_succeeded](const transaction &tx){tx_creation_succeeded=true;});
  return !ret && !tx_creation_succeeded;
}

bool gen_rct_tx_rct_bad_real_mask::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {sources[0].outputs[0].second.mask = rct::zeroCommitVartime(99999);},
    NULL);
}

bool gen_rct_tx_rct_bad_fake_dest::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {rct::key sk; rct::skpkGen(sk, sources[0].outputs[1].second.dest);},
    NULL);
}

bool gen_rct_tx_rct_bad_fake_mask::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {sources[0].outputs[1].second.mask = rct::zeroCommitVartime(99999);},
    NULL);
}

bool gen_rct_tx_rct_spend_with_zero_commit::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    [](std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations) {sources[0].outputs[0].second.mask = rct::zeroCommitVartime(sources[0].amount); sources[0].mask = rct::identity();},
    [](transaction &tx){boost::get<txin_to_key>(tx.vin[0]).amount = 0;});
}

bool gen_rct_tx_pre_rct_zero_vin_amount::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {boost::get<txin_to_key>(tx.vin[0]).amount = 0;});
}

bool gen_rct_tx_rct_non_zero_vin_amount::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {boost::get<txin_to_key>(tx.vin[0]).amount = 5000000000000;}); // one that we know exists
}

bool gen_rct_tx_non_zero_vout_amount::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.vout[0].amount = 5000000000000;}); // one that we know exists
}

bool gen_rct_tx_pre_rct_duplicate_key_image::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [&events](transaction &tx) {boost::get<txin_to_key>(tx.vin[0]).k_image = boost::get<txin_to_key>(boost::get<transaction>(events[67]).vin[0]).k_image;});
}

bool gen_rct_tx_rct_duplicate_key_image::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [&events](transaction &tx) {boost::get<txin_to_key>(tx.vin[0]).k_image = boost::get<txin_to_key>(boost::get<transaction>(events[67]).vin[0]).k_image;});
}

bool gen_rct_tx_pre_rct_wrong_key_image::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  // some random key image from the monero blockchain, so we get something that is a valid key image
  static const uint8_t k_image[33] = "\x49\x3b\x56\x16\x54\x76\xa8\x75\xb7\xf4\xa8\x51\xf5\x55\xd3\x44\xe7\x3e\xea\x73\xee\xc1\x06\x7c\x7d\xb6\x57\x28\x46\x85\xe1\x07";
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {memcpy(&boost::get<txin_to_key>(tx.vin[0]).k_image, k_image, 32);});
}

bool gen_rct_tx_rct_wrong_key_image::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  // some random key image from the monero blockchain, so we get something that is a valid key image
  static const uint8_t k_image[33] = "\x49\x3b\x56\x16\x54\x76\xa8\x75\xb7\xf4\xa8\x51\xf5\x55\xd3\x44\xe7\x3e\xea\x73\xee\xc1\x06\x7c\x7d\xb6\x57\x28\x46\x85\xe1\x07";
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {memcpy(&boost::get<txin_to_key>(tx.vin[0]).k_image, k_image, 32);});
}

bool gen_rct_tx_pre_rct_wrong_fee::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.rct_signatures.txnFee++;});
}

bool gen_rct_tx_rct_wrong_fee::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.rct_signatures.txnFee++;});
}

bool gen_rct_tx_pre_rct_increase_vin_and_fee::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {boost::get<txin_to_key>(tx.vin[0]).amount++;tx.rct_signatures.txnFee++;});
}

bool gen_rct_tx_pre_rct_remove_vin::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.vin.pop_back();});
}

bool gen_rct_tx_rct_remove_vin::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.vin.pop_back();});
}

bool gen_rct_tx_pre_rct_add_vout::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.vout.push_back(tx.vout.back());});
}

bool gen_rct_tx_rct_add_vout::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [](transaction &tx) {tx.vout.push_back(tx.vout.back());});
}

bool gen_rct_tx_pre_rct_altered_extra::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  bool failed = false;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [&failed](transaction &tx) {std::string extra_nonce; crypto::hash pid = crypto::null_hash; set_payment_id_to_tx_extra_nonce(extra_nonce, pid); if (!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce)) failed = true; }) && !failed;
}

bool gen_rct_tx_rct_altered_extra::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  bool failed = false;
  return generate_with(events, out_idx, mixin, amount_paid, false,
    NULL, [&failed](transaction &tx) {std::string extra_nonce; crypto::hash pid = crypto::null_hash; set_payment_id_to_tx_extra_nonce(extra_nonce, pid); if (!add_extra_nonce_to_tx_extra(tx.extra, extra_nonce)) failed = true; }) && !failed;
}

bool gen_rct_tx_pre_rct_has_no_view_tag_before_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  bool use_view_tags = false;
  bool valid = true;
  return generate_with_full(events, out_idx, mixin, amount_paid, 0, 0, {}, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_pre_rct_has_no_view_tag_from_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofPaddedBulletproof, 3 };
  bool use_view_tags = false;
  bool valid = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, HF_VERSION_VIEW_TAGS, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_pre_rct_has_view_tag_before_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  bool use_view_tags = true;
  bool valid = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, 0, 0, {}, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_pre_rct_has_view_tag_from_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {0, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofPaddedBulletproof, 3 };
  bool use_view_tags = true;
  bool valid = true;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, HF_VERSION_VIEW_TAGS, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_rct_has_no_view_tag_before_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofBorromean, 0 };
  bool use_view_tags = false;
  bool valid = true;
  return generate_with_full(events, out_idx, mixin, amount_paid, 0, 0, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_rct_has_no_view_tag_from_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofPaddedBulletproof, 3 };
  bool use_view_tags = false;
  bool valid = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, HF_VERSION_VIEW_TAGS+1, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_rct_has_view_tag_before_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 2;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofBorromean, 0 };
  bool use_view_tags = true;
  bool valid = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, 0, 0, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_rct_has_view_tag_from_hf_view_tags::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofPaddedBulletproof, 3 };
  bool use_view_tags = true;
  bool valid = true;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE, HF_VERSION_VIEW_TAGS, rct_config, use_view_tags, valid, NULL, NULL);
}

bool gen_rct_tx_uses_output_too_early::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amount_paid = 10000;
  const rct::RCTConfig rct_config { rct::RangeProofPaddedBulletproof, 2 };
  bool use_view_tags = false;
  bool valid = false;
  return generate_with_full(events, out_idx, mixin, amount_paid, CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE-3, HF_VERSION_ENFORCE_MIN_AGE, rct_config, use_view_tags, valid, NULL, NULL);
}
