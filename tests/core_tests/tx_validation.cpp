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
#include "tx_validation.h"
#include "device/device.hpp"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

namespace
{
  struct tx_builder
  {
    void step1_init(size_t version = 7, uint64_t unlock_time = 0)
    {
      m_tx.vin.clear();
      m_tx.vout.clear();
      m_tx.signatures.clear();

      m_tx.version = version;
      m_tx.unlock_time = unlock_time;

      m_tx_key = keypair::generate(hw::get_device("default"));
      add_tx_pub_key_to_extra(m_tx, m_tx_key.pub);
    }

    void step2_fill_inputs(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources)
    {
      BOOST_FOREACH(const tx_source_entry& src_entr, sources)
      {
        m_in_contexts.push_back(keypair());
        keypair& in_ephemeral = m_in_contexts.back();
        crypto::key_image img;
        std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
        subaddresses[sender_account_keys.m_account_address.m_spend_public_key] = {0,0};
        auto& out_key = reinterpret_cast<const crypto::public_key&>(src_entr.outputs[src_entr.real_output].second.dest);
        generate_key_image_helper(sender_account_keys, subaddresses, out_key, src_entr.real_out_tx_key, src_entr.real_out_additional_tx_keys, src_entr.real_output_in_tx_index, in_ephemeral, img, hw::get_device(("default")));

        // put key image into tx input
        txin_to_key input_to_key;
        input_to_key.amount = src_entr.amount;
        input_to_key.k_image = img;

        // fill outputs array and use relative offsets
        BOOST_FOREACH(const tx_source_entry::output_entry& out_entry, src_entr.outputs)
          input_to_key.key_offsets.push_back(out_entry.first);

        input_to_key.key_offsets = absolute_output_offsets_to_relative(input_to_key.key_offsets);
        m_tx.vin.push_back(input_to_key);
      }
    }

    void step3_fill_outputs(const std::vector<tx_destination_entry>& destinations)
    {
      size_t output_index = 0;
      BOOST_FOREACH(const tx_destination_entry& dst_entr, destinations)
      {
        crypto::key_derivation derivation;
        crypto::public_key out_eph_public_key;
        crypto::generate_key_derivation(dst_entr.addr.m_view_public_key, m_tx_key.sec, derivation);
        crypto::derive_public_key(derivation, output_index, dst_entr.addr.m_spend_public_key, out_eph_public_key);

        tx_out out;
        out.amount = dst_entr.amount;
        txout_to_key tk;
        tk.key = out_eph_public_key;
        out.target = tk;
        m_tx.vout.push_back(out);
        output_index++;
      }
    }

    void step4_calc_hash()
    {
      get_transaction_prefix_hash(m_tx, m_tx_prefix_hash);
    }

    void step5_sign(const std::vector<tx_source_entry>& sources)
    {
      m_tx.signatures.clear();

      size_t i = 0;
      BOOST_FOREACH(const tx_source_entry& src_entr, sources)
      {
        std::vector<const crypto::public_key*> keys_ptrs;
        std::vector<crypto::public_key> keys(src_entr.outputs.size());
        size_t j = 0;
        BOOST_FOREACH(const tx_source_entry::output_entry& o, src_entr.outputs)
        {
          keys[j] = rct::rct2pk(o.second.dest);
          keys_ptrs.push_back(&keys[j]);
          ++j;
        }

        m_tx.signatures.push_back(std::vector<crypto::signature>());
        std::vector<crypto::signature>& sigs = m_tx.signatures.back();
        sigs.resize(src_entr.outputs.size());
        generate_ring_signature(m_tx_prefix_hash, boost::get<txin_to_key>(m_tx.vin[i]).k_image, keys_ptrs, m_in_contexts[i].sec, src_entr.real_output, sigs.data());
        i++;
      }
    }

    transaction m_tx;
    keypair m_tx_key;
    std::vector<keypair> m_in_contexts;
    crypto::hash m_tx_prefix_hash;
  };

  transaction make_simple_tx_with_unlock_time(const std::vector<test_event_entry>& events,
    const cryptonote::block& blk_head, const cryptonote::account_base& from, const cryptonote::account_base& to,
    uint64_t amount, uint64_t unlock_time)
  {
    std::vector<tx_source_entry> sources;
    std::vector<tx_destination_entry> destinations;
    fill_tx_sources_and_destinations(events, blk_head, from, to, amount, TESTS_DEFAULT_FEE, 0, sources, destinations);

    tx_builder builder;
    builder.step1_init(cryptonote::network_version_7, unlock_time);
    builder.step2_fill_inputs(from.get_keys(), sources);
    builder.step3_fill_outputs(destinations);
    builder.step4_calc_hash();
    builder.step5_sign(sources);
    return builder.m_tx;
  };

  crypto::public_key generate_invalid_pub_key()
  {
    for (int i = 0; i <= 0xFF; ++i)
    {
      crypto::public_key key;
      memset(&key, i, sizeof(crypto::public_key));
      if (!crypto::check_key(key))
      {
        return key;
      }
    }

    throw std::runtime_error("invalid public key wasn't found");
    return crypto::public_key();
  }

  crypto::key_image generate_invalid_key_image()
  {
    crypto::key_image key_image;
    // a random key image plucked from the blockchain
    if (!epee::string_tools::hex_to_pod("6b9f5d1be7c950dc6e4e258c6ef75509412ba9ecaaf90e6886140151d1365b5e", key_image))
      throw std::runtime_error("invalid key image wasn't found");
    return key_image;
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Tests

bool gen_tx_big_version::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_money_unlocked, blk_tail, miner_account, 30);
  REWIND_BLOCKS(events, blk_head, blk_money_unlocked, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), -1).build();
  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);

  return true;
}

bool gen_tx_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  std::list<transaction> txs_0;

  transaction tx       = {};
  uint64_t unlock_time = 0;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = get_block_height(blk_money_unlocked) - 1;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = get_block_height(blk_money_unlocked);
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = get_block_height(blk_money_unlocked) + 1;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = get_block_height(blk_money_unlocked) + 2;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = ts_start - 1;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  tx          = {};
  unlock_time = time(0) + 60 * 60;
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_unlock_time(unlock_time).build();
  events.push_back(tx);
  txs_0.push_back(tx);

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_tmp, blk_money_unlocked, miner_account, txs_0);
  return true;
}

bool gen_tx_input_is_not_txin_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  MAKE_NEXT_BLOCK(events, blk_tmp, blk_head, miner_account);
  events.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(blk_tmp.miner_tx);

  DO_CALLBACK(events, "mark_invalid_tx");
  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vin.push_back(txin_to_script());
  events.push_back(tx);

  DO_CALLBACK(events, "mark_invalid_tx");
  tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vin.push_back(txin_to_scripthash());
  events.push_back(tx);

  return true;
}

bool gen_tx_no_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  transaction tx = {};
  tx.version     = cryptonote::network_version_7;
  add_tx_pub_key_to_extra(tx, keypair::generate(hw::get_device("default")).pub);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_no_inputs_has_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vin.clear();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_has_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vout.clear();

  DO_CALLBACK(events, "mark_invalid_tx"); // NOTE(loki): This used to be valid in Monero pre RCT, but not anymore with our transactions because we start with RCT type TXs
  events.push_back(tx);
  return true;
}

bool gen_tx_invalid_input_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  std::vector<tx_source_entry>      sources;
  std::vector<tx_destination_entry> destinations;
  uint64_t                          change_amount;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, CRYPTONOTE_DEFAULT_TX_MIXIN, sources, destinations, &change_amount);
  sources.front().amount++;

  transaction tx = {};
  cryptonote::tx_destination_entry change_addr{ change_amount, miner_account.get_keys().m_account_address, false /*is_subaddress*/ };
  assert(cryptonote::construct_tx(miner_account.get_keys(), sources, destinations, change_addr, {} /*tx_extra*/, tx, 0 /*unlock_time*/, cryptonote::network_version_7, false /*is_staking*/));

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_input_wo_key_offsets::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, CRYPTONOTE_DEFAULT_TX_MIXIN, sources, destinations);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  txin_to_key& in_to_key = boost::get<txin_to_key>(tx.vin.front());
  while (!in_to_key.key_offsets.empty())
    in_to_key.key_offsets.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_key_offset_points_to_foreign_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_1, blk_tail, miner_account, 40);
  REWIND_BLOCKS     (events, blk_2, blk_1,    miner_account);

  MAKE_ACCOUNT      (events, alice_account);
  MAKE_ACCOUNT      (events, bob_account);

  MAKE_TX_LIST_START     (events, txs_0, miner_account, bob_account,   MK_COINS(15) + 1, blk_2);
  MAKE_TX_LIST           (events, txs_0, miner_account, alice_account, MK_COINS(15) + 1, blk_2);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_money_unlocked, blk_2,              miner_account, txs_0);
  REWIND_BLOCKS          (events, blk_head,           blk_money_unlocked, miner_account);

  transaction bob_tx = {};
  TxBuilder(events, bob_tx, blk_money_unlocked, bob_account, miner_account, MK_COINS(15) + 1 - TESTS_DEFAULT_FEE, cryptonote::network_version_7).with_fee(TESTS_DEFAULT_FEE).build();

  std::vector<tx_source_entry>      sources_alice;
  std::vector<tx_destination_entry> destinations_alice;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, alice_account, miner_account, MK_COINS(15) + 1 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, CRYPTONOTE_DEFAULT_TX_MIXIN, sources_alice, destinations_alice);

  txin_to_key& bob_in_to_key        = boost::get<txin_to_key>(bob_tx.vin.front());
  bob_in_to_key.key_offsets.front() = sources_alice.front().outputs.back().first;

  // TODO(loki): This used to be first(), but in the debugger bob's front() is
  // 0 and alice's front() is 0 .. sooo ??  Reassigning the first offset
  // wouldn't change the test.  Now this test returns the same error as
  // gen_tx_sender_key_offset_not_exist so I don't think this test is correct
  // using back().
  // bob_in_to_key.key_offsets.front() = sources_alice.front().outputs.first().first;

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(bob_tx);
  return true;
}

bool gen_tx_sender_key_offset_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  txin_to_key& in_to_key        = boost::get<txin_to_key>(tx.vin.front());
  in_to_key.key_offsets.front() = std::numeric_limits<uint64_t>::max();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_mixed_key_offset_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  // TODO(loki): This test looks broken. step2_fill_inputs calls
  // generate_key_image_helper() which returns false and doesn't write to the TX
  // if it fails. This test fails and early outs before the the key image is
  // even made so, we can't really test putting this onto the chain? This would
  // be more like a unit test.

  // Monero version
#if 0
  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);
  MAKE_ACCOUNT(events, alice_account);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_1);
  MAKE_TX_LIST(events, txs_0, miner_account, alice_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_2, bob_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 1, sources, destinations);

  sources.front().outputs[(sources.front().real_output + 1) % 2].first = std::numeric_limits<uint64_t>::max();

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(bob_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);
#endif

  // Loki version
#if 0
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_1, blk_tail, miner_account, 40);
  REWIND_BLOCKS     (events, blk_2, blk_1,    miner_account);

  MAKE_ACCOUNT      (events, alice_account);
  MAKE_ACCOUNT      (events, bob_account);

  MAKE_TX_LIST_START     (events, txs_0, miner_account, bob_account,   MK_COINS(1) + TESTS_DEFAULT_FEE, blk_2);
  MAKE_TX_LIST           (events, txs_0, miner_account, alice_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_2);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_money_unlocked, blk_2,              miner_account, txs_0);
  REWIND_BLOCKS          (events, blk_head,           blk_money_unlocked, miner_account);

  std::vector<tx_source_entry>      sources;
  std::vector<tx_destination_entry> destinations;
  uint64_t                          change_amount;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, bob_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, CRYPTONOTE_DEFAULT_TX_MIXIN, sources, destinations, &change_amount);
  sources.front().outputs[(sources.front().real_output + 1) % 2].first = std::numeric_limits<uint64_t>::max();

  transaction tx = {};
  cryptonote::tx_destination_entry change_addr{ change_amount, miner_account.get_keys().m_account_address, false /*is_subaddress*/ };
  assert(cryptonote::construct_tx(miner_account.get_keys(), sources, destinations, change_addr, {} /*tx_extra*/, tx, 0 /*unlock_time*/, cryptonote::network_version_7, false /*is_staking*/));

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
#endif
  return true;
}

bool gen_tx_key_image_not_derive_from_tx_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  txin_to_key& in_to_key        = boost::get<txin_to_key>(tx.vin.front());

  // Use fake key image
  keypair keys = keypair::generate(hw::get_device("default"));
  key_image fake_key_image;
  crypto::generate_key_image(keys.pub, keys.sec, fake_key_image);
  in_to_key.k_image = fake_key_image;

  // Tx with invalid key image can't be subscribed, so create empty signature
  tx.signatures.resize(1);
  tx.signatures[0].resize(1);
  tx.signatures[0][0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_key_image_is_invalid::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  txin_to_key& in_to_key = boost::get<txin_to_key>(tx.vin.front());
  in_to_key.k_image      = generate_invalid_key_image();

  // Tx with invalid key image can't be subscribed, so create empty signature
  tx.signatures.resize(1);
  tx.signatures[0].resize(1);
  tx.signatures[0][0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_check_input_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  static const size_t tests_count = 6;

  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_1, blk_0, miner_account, tests_count - 1);
  REWIND_BLOCKS     (events, blk_1r, blk_1, miner_account);

  std::array<account_base, tests_count> accounts;
  for (size_t i = 0; i < tests_count; ++i)
  {
    MAKE_ACCOUNT(events, acc);
    accounts[i] = acc;
  }

  std::list<transaction> txs_0;
  auto make_tx_to_acc = [&](size_t acc_idx, uint64_t unlock_time)
  {
    txs_0.push_back(make_simple_tx_with_unlock_time(events, blk_1, miner_account, accounts[acc_idx],
      MK_COINS(1) + TESTS_DEFAULT_FEE, unlock_time));
    events.push_back(txs_0.back());
  };

  uint64_t blk_3_height = get_block_height(blk_1r) + 2;
  make_tx_to_acc(0, 0);
  make_tx_to_acc(1, blk_3_height - 1);
  make_tx_to_acc(2, blk_3_height);
  make_tx_to_acc(3, blk_3_height + 1);
  make_tx_to_acc(4, time(0) - 1);
  make_tx_to_acc(5, time(0) + 60 * 60);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  std::list<transaction> txs_1;
  auto make_tx_from_acc = [&](size_t acc_idx, bool invalid)
  {
    transaction tx = make_simple_tx_with_unlock_time(events, blk_2, accounts[acc_idx], miner_account, MK_COINS(1), 0);
    if (invalid)
    {
      DO_CALLBACK(events, "mark_invalid_tx");
    }
    else
    {
      txs_1.push_back(tx);
    }
    events.push_back(tx);
  };

  make_tx_from_acc(0, false);
  make_tx_from_acc(1, false);
  make_tx_from_acc(2, false);
  make_tx_from_acc(3, true);
  make_tx_from_acc(4, false);
  make_tx_from_acc(5, true);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_3, blk_2, miner_account, txs_1);

  return true;
}

bool gen_tx_txout_to_key_has_invalid_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx           = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  txout_to_key& out_to_key = boost::get<txout_to_key>(tx.vout.front().target);
  out_to_key.key           = generate_invalid_pub_key();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_output_with_zero_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  // TODO(loki): Hmm. Can't use TxBuilder approach because RCT masks amounts
  // after it's constructed, so vout amounts is already zero. It seems to be
  // valid to be able to send a transaction whos output is zero, so this test
  // might not be valid anymore post RCT.
#if 1
  std::vector<tx_source_entry>      sources;
  std::vector<tx_destination_entry> destinations;
  uint64_t                          change_amount;
  fill_tx_sources_and_destinations(events, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, CRYPTONOTE_DEFAULT_TX_MIXIN, sources, destinations, &change_amount);

  for (tx_destination_entry &entry : destinations)
    entry.amount = 0;

  transaction tx = {};
  cryptonote::tx_destination_entry change_addr{ change_amount, miner_account.get_keys().m_account_address, false /*is_subaddress*/ };
  assert(cryptonote::construct_tx(miner_account.get_keys(), sources, destinations, change_addr, {} /*tx_extra*/, tx, 0 /*unlock_time*/, cryptonote::network_version_7, false /*is_staking*/));

#else
  transaction tx           = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vout.front().amount = 0;

#endif

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_output_is_not_txout_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_money_unlocked, blk_tail,           miner_account, 40);
  REWIND_BLOCKS     (events, blk_head,           blk_money_unlocked, miner_account);

  transaction tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vout.back().target = txout_to_script();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);

  tx = {};
  TxBuilder(events, tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(1), cryptonote::network_version_7).build();
  tx.vout.back().target = txout_to_scripthash();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(tx);
  return true;
}

bool gen_tx_signatures_are_invalid::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT  (miner_account);
  MAKE_GENESIS_BLOCK(events, blk_tail, miner_account, ts_start);
  REWIND_BLOCKS_N   (events, blk_1, blk_tail, miner_account, 40);
  REWIND_BLOCKS     (events, blk_2, blk_1,    miner_account);

  MAKE_ACCOUNT      (events, alice_account);
  MAKE_ACCOUNT      (events, bob_account);

  MAKE_TX_LIST_START     (events, txs_0, miner_account, bob_account,   MK_COINS(1) + TESTS_DEFAULT_FEE, blk_2);
  MAKE_TX_LIST           (events, txs_0, miner_account, alice_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_2);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_money_unlocked, blk_2,              miner_account, txs_0);
  REWIND_BLOCKS          (events, blk_head,           blk_money_unlocked, miner_account);

  transaction miner_tx = {};
  TxBuilder(events, miner_tx, blk_money_unlocked, miner_account, miner_account, MK_COINS(60), cryptonote::network_version_7).with_fee(TESTS_DEFAULT_FEE).build();

  // TX without signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  blobdata sr_tx = t_serializable_object_to_blob(static_cast<transaction_prefix>(miner_tx));
  events.push_back(serialized_transaction(sr_tx));

  // TX have a few inputs, and not enough signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(miner_tx);
  sr_tx.resize(sr_tx.size() - sizeof(crypto::signature));
  events.push_back(serialized_transaction(sr_tx));

  // TX have a few inputs, and too many signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(miner_tx);
  sr_tx.insert(sr_tx.end(), sr_tx.end() - sizeof(crypto::signature), sr_tx.end());
  events.push_back(serialized_transaction(sr_tx));

  transaction bob_tx = {};
  TxBuilder(events, bob_tx, blk_money_unlocked, bob_account, miner_account, MK_COINS(1), cryptonote::network_version_7).with_fee(TESTS_DEFAULT_FEE).build();

  // TX without signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(static_cast<transaction_prefix>(bob_tx));
  events.push_back(serialized_transaction(sr_tx));

  // TX have not enough signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(bob_tx);
  sr_tx.resize(sr_tx.size() - sizeof(crypto::signature));
  events.push_back(serialized_transaction(sr_tx));

  // TX have too many signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(bob_tx);
  sr_tx.insert(sr_tx.end(), sr_tx.end() - sizeof(crypto::signature), sr_tx.end());
  events.push_back(serialized_transaction(sr_tx));
  return true;
}
