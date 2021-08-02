// Copyright (c) 2014-2019, The Monero Project
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
#include "ringct/bulletproofs.h"
#include "chaingen.h"
#include "triptych.h"
#include "device/device.hpp"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_triptych_tx_validation_base::generate_with_full(std::vector<test_event_entry>& events,
    const int *out_idx, int mixin, const uint64_t *amounts_paid, size_t second_rewind, uint8_t last_version, const rct::RCTConfig &rct_config, int tweak,  bool valid,
    const std::function<bool(std::vector<tx_source_entry> &sources, std::vector<tx_destination_entry> &destinations)> &pre_tx,
    const std::function<bool(transaction &tx)> &post_tx) const
{
  uint64_t ts_start = 1338224400;
  std::vector<cryptonote::block> blk_txes;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  blk_txes.push_back(blk_0);

  // height 1: block 0 contains the genesis coinbase tx

  // create 12 miner accounts, and have them mine the next 12 blocks
  cryptonote::account_base miner_accounts[12];
  const cryptonote::block *prev_block = &blk_0;
  cryptonote::block blocks[12 + TRIPTYCH_RING_SIZE + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW];
  for (size_t n = 0; n < 12; ++n) {
    miner_accounts[n].generate();
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[n], *prev_block, miner_accounts[n],
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
        2, 2, prev_block->timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
        false, "Failed to generate block");
    events.push_back(blocks[n]);
    prev_block = blocks + n;
    blk_txes.push_back(blocks[n]);
  }

  // height 13: blocks 1 to 12 now contain pre-triptych coinbase outputs

  // rewind
  size_t rewind_offset = blk_txes.size();
  cryptonote::block blk_r, blk_last;
  {
    blk_last = blocks[11];
    for (size_t i = 0; i < TRIPTYCH_RING_SIZE + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW; ++i)
    {
      cryptonote::block blk;
      CHECK_AND_ASSERT_MES(generator.construct_block_manually(blocks[12+i], blk_last, miner_account,
          test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_hf_version,
          2, 2, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
          crypto::hash(), 0, transaction(), std::vector<crypto::hash>(), 0, 0, 2),
          false, "Failed to generate block");
      events.push_back(blocks[12+i]);
      blk_last = blocks[12+i];
      blk_txes.push_back(blk_last);
    }
    blk_r = blk_last;
  }

  // height 201: blocks 1 to 12 now contain unlocked pre-triptych coinbase outputs, 13 - 200 contain coinbase outputs

  // create 4 txes from these miners in new blocks, to generate some rct outputs
  transaction rct_txes[4];
  rct::key rct_tx_masks[16];
  size_t rct_blk_offset = blk_txes.size();
  for (size_t n = 0; n < 4; ++n)
  {
    std::vector<crypto::hash> starting_rct_tx_hashes;
    std::vector<tx_source_entry> sources;
    uint64_t fees = 0;

    sources.resize(1);
    tx_source_entry& src = sources.back();

    const size_t index_in_tx = 5;
    src.amount = 30000000000000;
    for (int m = 0; m < 4; ++m) {
      src.push_output(m, boost::get<txout_to_key>(blocks[m].miner_tx.vout[index_in_tx].target).key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[n].miner_tx);
    src.real_output = n;
    src.real_output_in_tx_index = index_in_tx;
    src.mask = rct::identity();
    src.rct = false;
    src.triptych = false;

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
    boost::container::flat_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[miner_accounts[n].get_keys().m_account_address.m_spend_public_key] = {0,0};
    bool r = construct_tx_and_get_tx_key(miner_accounts[n].get_keys(), subaddresses, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), rct_txes[n], 0, tx_key, additional_tx_keys, NULL, true);
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
    fees += fee;

    blk_txes.push_back({});
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk_txes.back(), blk_last, miner_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
        4, 4, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0, 6, 4, fees),
        false, "Failed to generate block");
    events.push_back(blk_txes.back());
    blk_last = blk_txes.back();
  }

  const size_t num_pre_triptych_outputs = 40; // 4 coinbase outputs at 6 outputs per, 16 user tx outputs

  // create 128 txes from these miners in new blocks, to generate some triptych outputs
  transaction triptych_txes[TRIPTYCH_RING_SIZE];
  rct::key triptych_tx_masks[TRIPTYCH_RING_SIZE * 2];
  size_t triptych_blk_offset = blk_txes.size();
  size_t jump = 0; // to avoid mixing pre-triptych and triptych outputs in the same ring
  for (size_t n = 0; n < TRIPTYCH_RING_SIZE; ++n)
  {
    std::vector<crypto::hash> starting_rct_tx_hashes;
    std::vector<tx_source_entry> sources;
    uint64_t fees = 0;

    sources.resize(1);
    tx_source_entry& src = sources.back();

    src.amount = 30000000000000;
    for (int m = 0; m < 11; ++m) {
      if (blk_txes[rewind_offset + jump + n + m].miner_tx.vout.back().amount != src.amount)
      {
        MERROR("Unexpected amount: " << blk_txes[rewind_offset + jump + n + m].miner_tx.vout.back().amount);
        return false;
      }
      src.push_output(m + n + rewind_offset + jump - 1, boost::get<txout_to_key>(blk_txes[rewind_offset + jump + n + m].miner_tx.vout.back().target).key, src.amount);
    }
    src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blk_txes[rewind_offset + jump + n].miner_tx);
    src.real_output = 0;
    src.real_output_in_tx_index = blk_txes[rewind_offset + jump + n].miner_tx.vout.size() - 1;
    src.mask = rct::identity();
    src.rct = false;
    src.triptych = false;

    if (n == rewind_offset + 4)
      jump = 11;

    //fill outputs entry
    tx_destination_entry td;
    td.addr = miner_accounts[n & 3].get_keys().m_account_address;
    td.amount = 7390000000000 * 2;
    std::vector<tx_destination_entry> destinations;
    destinations.push_back(td);
    destinations.push_back(td); // 30 -> 7.39 * 2 * 2

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    boost::container::flat_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[miner_account.get_keys().m_account_address.m_spend_public_key] = {0,0};
    bool r = construct_tx_and_get_tx_key(miner_account.get_keys(), subaddresses, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), triptych_txes[n], 0, tx_key, additional_tx_keys, NULL, true, {rct::RangeProofPaddedBulletproof, 4});
    CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");
    events.push_back(triptych_txes[n]);
    starting_rct_tx_hashes.push_back(get_transaction_hash(triptych_txes[n]));

    for (size_t o = 0; o < 2; ++o)
    {
      crypto::key_derivation derivation;
      bool r = crypto::generate_key_derivation(destinations[o].addr.m_view_public_key, tx_key, derivation);
      CHECK_AND_ASSERT_MES(r, false, "Failed to generate key derivation");
      crypto::secret_key amount_key;
      crypto::derivation_to_scalar(derivation, o, amount_key);
      const uint8_t type = triptych_txes[n].rct_signatures.type;
      if (rct::is_rct_simple(type))
        rct::decodeRctSimple(triptych_txes[n].rct_signatures, rct::sk2rct(amount_key), o, triptych_tx_masks[o+n*2], hw::get_device("default"));
      else
        rct::decodeRct(triptych_txes[n].rct_signatures, rct::sk2rct(amount_key), o, triptych_tx_masks[o+n*2], hw::get_device("default"));
    }

    uint64_t fee = 0;
    get_tx_fee(triptych_txes[n], fee);
    fees += fee;

    blk_txes.push_back({});
    CHECK_AND_ASSERT_MES(generator.construct_block_manually(blk_txes.back(), blk_last, miner_account,
        test_generator::bf_major_ver | test_generator::bf_minor_ver | test_generator::bf_timestamp | test_generator::bf_tx_hashes | test_generator::bf_hf_version | test_generator::bf_max_outs | test_generator::bf_tx_fees,
        HF_VERSION_TRIPTYCH, HF_VERSION_TRIPTYCH, blk_last.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN * 2, // v2 has blocks twice as long
        crypto::hash(), 0, transaction(), starting_rct_tx_hashes, 0, 6, 4, fees),
        false, "Failed to generate block");
    events.push_back(blk_txes.back());
    blk_last = blk_txes.back();
  }

  // height 333: blocks 201 - 204 contain pre-triptych rct outputs, 205 - 332 contain triptych rct outputs

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

  // height 333 + second_rewind: blocks 201 - 204 contain pre-triptych rct outputs, 205 - 332 contain triptych rct outputs, 333-... contain triptych coinbase outputs

  const size_t n_txes = tweak == tweak_double_spend ? 2 : 1;
  for (size_t n_tx = 0; n_tx < n_txes; ++n_tx)
  {
    // create a tx from the requested ouputs
    std::vector<tx_source_entry> sources;
    size_t global_rct_idx = 6; // skip first coinbase (6 outputs)
    size_t rct_idx = 0;
    size_t pre_rct_idx = 0;
    size_t triptych_idx = 0;
    size_t ring_size;
    for (size_t out_idx_idx = 0; out_idx[out_idx_idx] >= 0; ++out_idx_idx) {
      sources.resize(sources.size()+1);
      tx_source_entry& src = sources.back();

      src.real_output = 0;
      switch (out_idx[out_idx_idx]) {
      case 0:
        // pre rct
        src.amount = 5000000000000;
        src.real_out_tx_key = cryptonote::get_tx_pub_key_from_extra(blocks[pre_rct_idx].miner_tx);
        src.real_output_in_tx_index = 4;
        src.mask = rct::identity();
        src.rct = false;
        src.triptych = false;
        for (int m = 0; m <= mixin; ++m) {
          size_t aidx;
          for (aidx = 0; aidx < blocks[pre_rct_idx].miner_tx.vout.size(); ++aidx)
            if (blocks[pre_rct_idx].miner_tx.vout[aidx].amount == src.amount)
              break;
          if (aidx == blocks[pre_rct_idx].miner_tx.vout.size())
          {
            MERROR("Amount not found");
            return false;
          }
          src.push_output(m, boost::get<txout_to_key>(blocks[pre_rct_idx].miner_tx.vout[aidx].target).key, src.amount);
          ++pre_rct_idx;
        }
        break;
      case 1: // pre-triptych rct
        src.amount = 7390000000000;
        src.real_out_tx_key = get_tx_pub_key_from_extra(rct_txes[rct_idx/4]);
        src.real_output_in_tx_index = rct_idx&3;
        src.mask = rct_tx_masks[rct_idx];
        src.rct = true;
        src.triptych = false;
        for (int m = 0; m <= mixin; ++m) {
          rct::ctkey ctkey;
          ctkey.dest = rct::pk2rct(boost::get<txout_to_key>(rct_txes[rct_idx/4].vout[rct_idx&3].target).key);
          ctkey.mask = rct_txes[rct_idx/4].rct_signatures.outPk[rct_idx&3].mask;
          src.outputs.push_back(std::make_pair(global_rct_idx, ctkey));
          if (tweak == tweak_mixed_rings && m == mixin)
          {
            // replace the last one with a triptych one
            src.outputs.back().first = num_pre_triptych_outputs + 6 /* first triptych coinbase */;
            src.outputs.back().second.dest = rct::pk2rct(boost::get<txout_to_key>(triptych_txes[0].vout[0].target).key);
            src.outputs.back().second.mask = triptych_txes[0].rct_signatures.outPk[0].mask;
          }
          ++rct_idx;
          ++global_rct_idx;
          if (global_rct_idx % 10 == 0)
            global_rct_idx += 6; // skip the coinbase
        }
        break;
      case 2: // triptych rct
        src.real_output = 4;
        src.amount = 7390000000000 * 2;
        src.real_out_tx_key = get_tx_pub_key_from_extra(triptych_txes[4]);
        src.real_output_in_tx_index = 0;
        src.mask = triptych_tx_masks[4 * 2];
        src.rct = true;
        src.triptych = true;
        ring_size = tweak == tweak_low_ring_size ? TRIPTYCH_RING_SIZE / 2 : TRIPTYCH_RING_SIZE;
        for (size_t m = 0; m < ring_size; ++m) {
          rct::ctkey ctkey;
          ctkey.dest = rct::pk2rct(boost::get<txout_to_key>(triptych_txes[m].vout[0].target).key);
          ctkey.mask = triptych_txes[m].rct_signatures.outPk[0].mask;
          src.outputs.push_back(std::make_pair(num_pre_triptych_outputs + m * 8 /* 6 coinbase, 2 user outputs */ + 6 /* first coinbase */, ctkey));
          if (tweak == tweak_mixed_rings && m == ring_size - 1)
          {
            // replace the first one with a pre-triptych one
            src.outputs[0].first = 6; // skip first coinbase
            src.outputs[0].second.dest = rct::pk2rct(boost::get<txout_to_key>(rct_txes[0].vout[0].target).key);
            src.outputs[0].second.mask = rct_txes[0].rct_signatures.outPk[0].mask;
          }
        }
        break;
      default:
        MERROR("Invalid out_idx: " << out_idx[out_idx_idx]);
        return false;
      }
    }

    //fill outputs entry
    tx_destination_entry td;
    td.addr = miner_account.get_keys().m_account_address;
    std::vector<tx_destination_entry> destinations;
    for (int o = 0; amounts_paid[o] != (uint64_t)-1; ++o)
    {
      td.amount = amounts_paid[o];
      destinations.push_back(td);
    }

    if (n_tx == n_txes - 1 && pre_tx)
      if (!pre_tx(sources, destinations))
        return false;

    transaction tx;
    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    boost::container::flat_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
    subaddresses[miner_accounts[0].get_keys().m_account_address.m_spend_public_key] = {0,0};
    bool r = construct_tx_and_get_tx_key(miner_accounts[0].get_keys(), subaddresses, sources, destinations, cryptonote::account_public_address{}, std::vector<uint8_t>(), tx, 0, tx_key, additional_tx_keys, NULL, true, rct_config);
    CHECK_AND_ASSERT_MES(r, false, "failed to construct transaction");

    if (n_tx == n_txes - 1 && post_tx)
      if (!post_tx(tx))
        return false;

    if (n_tx == n_txes - 1 && !valid)
      DO_CALLBACK(events, "mark_invalid_tx");

    events.push_back(tx);

    if (n_tx == n_txes - 1)
      LOG_PRINT_L0("Test tx: " << obj_to_json_str(tx));
  }

  return true;
}

bool gen_triptych_tx_valid_from_pre_rct::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {0, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, true, NULL, [&](transaction &tx) {
    if (tx.version < 2 || rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    for (const auto &in: tx.vin)
    {
      const auto &vin = boost::get<cryptonote::txin_to_key>(in);
      if (vin.key_offsets.size() != mixin + 1)
      {
        MERROR("Unexpected ring size: " << vin.key_offsets.size() << ", expected " << (mixin + 1));
        return false;
      }
    }
    for (const auto &e: tx.rct_signatures.p.triptych)
      if (e.type() != typeid(rct::clsag))
      {
        MERROR("Invalid signature type, expected CLSAG");
        return false;
      }
    return true;
  });
}

bool gen_triptych_tx_valid_from_pre_triptych::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, true, NULL, [&](transaction &tx) {
    if (tx.version < 2 || rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    for (const auto &in: tx.vin)
    {
      const auto &vin = boost::get<cryptonote::txin_to_key>(in);
      if (vin.key_offsets.size() != mixin + 1)
      {
        MERROR("Unexpected ring size: " << vin.key_offsets.size() << ", expected " << (mixin + 1));
        return false;
      }
    for (const auto &e: tx.rct_signatures.p.triptych)
      if (e.type() != typeid(rct::clsag))
      {
        MERROR("Invalid signature type, expected CLSAG");
        return false;
      }
    }
    return true;
  });
}

bool gen_triptych_tx_valid_from_post_triptych::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, true, NULL, [](transaction &tx) {
    if (tx.version < 2 || !rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    for (const auto &in: tx.vin)
    {
      const auto &vin = boost::get<cryptonote::txin_to_key>(in);
      if (vin.key_offsets.size() != TRIPTYCH_RING_SIZE)
      {
        MERROR("Unexpected key_offsets size: " << vin.key_offsets.size());
        return false;
      }
    for (const auto &e: tx.rct_signatures.p.triptych)
      if (e.type() != typeid(rct::TriptychProof))
      {
        MERROR("Unexpected triptych type");
        return false;
      }
    }
    return true;
  });
}

bool gen_triptych_tx_valid_mixed_inputs::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, 2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, true, NULL, [](transaction &tx) {
    if (tx.version < 2 || !rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    if (tx.rct_signatures.p.triptych.size() != 2)
    {
      MERROR("Unexpected triptych size: " << tx.rct_signatures.p.triptych.size());
      return false;
    }
    bool has_clsag = false, has_triptych = false;
    for (int i = 0; i < 2; ++i)
    {
      if (tx.rct_signatures.p.triptych[i].type() == typeid(rct::clsag))
        has_clsag = true;
      else if (tx.rct_signatures.p.triptych[i].type() == typeid(rct::TriptychProof))
        has_triptych = true;
    }
    if (!has_clsag || !has_triptych)
    {
      MERROR("Expected one each of CLSAG and Triptych");
      return false;
    }
    return true;
  });
}

bool gen_triptych_tx_invalid_bp::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 3 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, false, NULL, NULL);
}

bool gen_triptych_tx_invalid_mixed_rings_pre_triptych::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {1, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_mixed_rings, false, NULL, [](transaction &tx) {
    return tx.version >= 2 && !rct::is_rct_triptych(tx.rct_signatures.type);
  });
}

bool gen_triptych_tx_invalid_mixed_rings_post_triptych::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_mixed_rings, false, NULL, [](transaction &tx) {
    return tx.version >= 2 && rct::is_rct_triptych(tx.rct_signatures.type);
  });
}

bool gen_triptych_tx_invalid_double_spend::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_double_spend, false, NULL, [](transaction &tx) {
    return tx.version >= 2 && rct::is_rct_triptych(tx.rct_signatures.type);
  });
}

bool gen_triptych_tx_invalid_missing_signature::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {2, 2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_none, false, NULL, [](transaction &tx) {
    if (tx.version < 2 || !rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    if (tx.rct_signatures.p.triptych.size() != 2)
    {
      MERROR("Unexpected triptych size: " << tx.rct_signatures.p.triptych.size());
      return false;
    }
    tx.rct_signatures.p.triptych.pop_back();
    return true;
  });
}

bool gen_triptych_tx_invalid_low_ring_size::generate(std::vector<test_event_entry>& events) const
{
  const int mixin = 10;
  const int out_idx[] = {2, -1};
  const uint64_t amounts_paid[] = {5000, 5000, (uint64_t)-1};
  const rct::RCTConfig rct_config = { rct::RangeProofPaddedBulletproof, 5 };
  return generate_with_full(events, out_idx, mixin, amounts_paid, TRIPTYCH_RING_SIZE, HF_VERSION_TRIPTYCH, rct_config, tweak_low_ring_size, false, NULL, [](transaction &tx) {
    if (tx.version < 2 || !rct::is_rct_triptych(tx.rct_signatures.type))
    {
      MERROR("Unexpected tx type, version " << (unsigned)tx.version << ", type " << (unsigned)tx.rct_signatures.type);
      return false;
    }
    if (tx.vin.size() != 1)
    {
      MERROR("Unexpected vin size: " << tx.vin.size());
      return false;
    }
    if (boost::get<cryptonote::txin_to_key>(tx.vin[0]).key_offsets.size() != 64)
    {
      MERROR("Unexpected key_offsets size: " << boost::get<cryptonote::txin_to_key>(tx.vin[0]).key_offsets.size());
      return false;
    }
    return true;
  });
}
