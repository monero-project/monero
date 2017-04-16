// Copyright (c) 2014-2017, The Monero Project
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
#include "chaingen_tests_list.h"

using namespace epee;
using namespace crypto;
using namespace cryptonote;

namespace
{
  struct tx_builder
  {
    void step1_init(size_t version = 1, uint64_t unlock_time = 0)
    {
      m_tx.vin.clear();
      m_tx.vout.clear();
      m_tx.signatures.clear();

      m_tx.version = version;
      m_tx.unlock_time = unlock_time;

      m_tx_key = keypair::generate();
      add_tx_pub_key_to_extra(m_tx, m_tx_key.pub);
    }

    void step2_fill_inputs(const account_keys& sender_account_keys, const std::vector<tx_source_entry>& sources)
    {
      BOOST_FOREACH(const tx_source_entry& src_entr, sources)
      {
        m_in_contexts.push_back(keypair());
        keypair& in_ephemeral = m_in_contexts.back();
        crypto::key_image img;
        generate_key_image_helper(sender_account_keys, src_entr.real_out_tx_key, src_entr.real_output_in_tx_index, in_ephemeral, img);

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
    builder.step1_init(1, unlock_time);
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
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init(1 + 1, 0);
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_1, blk_0, miner_account, 10);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

  auto make_tx_with_unlock_time = [&](uint64_t unlock_time) -> transaction
  {
    return make_simple_tx_with_unlock_time(events, blk_1, miner_account, miner_account, MK_COINS(1), unlock_time);
  };

  std::list<transaction> txs_0;

  txs_0.push_back(make_tx_with_unlock_time(0));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) - 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r)));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) + 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(get_block_height(blk_1r) + 2));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(ts_start - 1));
  events.push_back(txs_0.back());

  txs_0.push_back(make_tx_with_unlock_time(time(0) + 60 * 60));
  events.push_back(txs_0.back());

  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  return true;
}

bool gen_tx_input_is_not_txin_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  MAKE_NEXT_BLOCK(events, blk_tmp, blk_0r, miner_account);
  events.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(blk_tmp.miner_tx);

  auto make_tx_with_input = [&](const txin_v& tx_input) -> transaction
  {
    std::vector<tx_source_entry> sources;
    std::vector<tx_destination_entry> destinations;
    fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

    tx_builder builder;
    builder.step1_init();
    builder.m_tx.vin.push_back(tx_input);
    builder.step3_fill_outputs(destinations);
    return builder.m_tx;
  };

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(make_tx_with_input(txin_to_script()));

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(make_tx_with_input(txin_to_scripthash()));

  return true;
}

bool gen_tx_no_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  tx_builder builder;
  builder.step1_init();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_no_inputs_has_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step3_fill_outputs(destinations);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_has_inputs_no_outputs::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  destinations.clear();

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  events.push_back(builder.m_tx);
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, builder.m_tx);

  return true;
}

bool gen_tx_invalid_input_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);
  sources.front().amount++;

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_input_wo_key_offsets::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  uint64_t key_offset = in_to_key.key_offsets.front();
  in_to_key.key_offsets.pop_back();
  CHECK_AND_ASSERT_MES(in_to_key.key_offsets.empty(), false, "txin contained more than one key_offset");
  builder.step4_calc_hash();
  in_to_key.key_offsets.push_back(key_offset);
  builder.step5_sign(sources);
  in_to_key.key_offsets.pop_back();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_key_offest_points_to_foreign_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);
  MAKE_ACCOUNT(events, alice_account);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, MK_COINS(15) + 1, blk_1);
  MAKE_TX_LIST(events, txs_0, miner_account, alice_account, MK_COINS(15) + 1, blk_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  std::vector<tx_source_entry> sources_bob;
  std::vector<tx_destination_entry> destinations_bob;
  fill_tx_sources_and_destinations(events, blk_2, bob_account, miner_account, MK_COINS(15) + 1 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources_bob, destinations_bob);

  std::vector<tx_source_entry> sources_alice;
  std::vector<tx_destination_entry> destinations_alice;
  fill_tx_sources_and_destinations(events, blk_2, alice_account, miner_account, MK_COINS(15) + 1 - TESTS_DEFAULT_FEE, TESTS_DEFAULT_FEE, 0, sources_alice, destinations_alice);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(bob_account.get_keys(), sources_bob);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  in_to_key.key_offsets.front() = sources_alice.front().outputs.front().first;
  builder.step3_fill_outputs(destinations_bob);
  builder.step4_calc_hash();
  builder.step5_sign(sources_bob);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_sender_key_offest_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  in_to_key.key_offsets.front() = std::numeric_limits<uint64_t>::max();
  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_mixed_key_offest_not_exist::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

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

  return true;
}

bool gen_tx_key_image_not_derive_from_tx_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  keypair kp = keypair::generate();
  key_image another_ki;
  crypto::generate_key_image(kp.pub, kp.sec, another_ki);
  in_to_key.k_image = another_ki;

  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();

  // Tx with invalid key image can't be subscribed, so create empty signature
  builder.m_tx.signatures.resize(1);
  builder.m_tx.signatures[0].resize(1);
  builder.m_tx.signatures[0][0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_key_image_is_invalid::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  txin_to_key& in_to_key = boost::get<txin_to_key>(builder.m_tx.vin.front());
  in_to_key.k_image = generate_invalid_key_image();

  builder.step3_fill_outputs(destinations);
  builder.step4_calc_hash();

  // Tx with invalid key image can't be subscribed, so create empty signature
  builder.m_tx.signatures.resize(1);
  builder.m_tx.signatures[0].resize(1);
  builder.m_tx.signatures[0][0] = boost::value_initialized<crypto::signature>();

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_check_input_unlock_time::generate(std::vector<test_event_entry>& events) const
{
  static const size_t tests_count = 6;

  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS_N(events, blk_1, blk_0, miner_account, tests_count - 1);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);

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

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);

  txout_to_key& out_to_key =  boost::get<txout_to_key>(builder.m_tx.vout.front().target);
  out_to_key.key = generate_invalid_pub_key();

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_output_with_zero_amount::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);
  builder.step3_fill_outputs(destinations);

  builder.m_tx.vout.front().amount = 0;

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_output_is_not_txout_to_key::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);

  std::vector<tx_source_entry> sources;
  std::vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_0, miner_account, miner_account, MK_COINS(1), TESTS_DEFAULT_FEE, 0, sources, destinations);

  tx_builder builder;
  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  builder.m_tx.vout.push_back(tx_out());
  builder.m_tx.vout.back().amount = 1;
  builder.m_tx.vout.back().target = txout_to_script();

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  builder.step1_init();
  builder.step2_fill_inputs(miner_account.get_keys(), sources);

  builder.m_tx.vout.push_back(tx_out());
  builder.m_tx.vout.back().amount = 1;
  builder.m_tx.vout.back().target = txout_to_scripthash();

  builder.step4_calc_hash();
  builder.step5_sign(sources);

  DO_CALLBACK(events, "mark_invalid_tx");
  events.push_back(builder.m_tx);

  return true;
}

bool gen_tx_signatures_are_invalid::generate(std::vector<test_event_entry>& events) const
{
  uint64_t ts_start = 1338224400;

  GENERATE_ACCOUNT(miner_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);
  MAKE_NEXT_BLOCK(events, blk_1, blk_0, miner_account);
  REWIND_BLOCKS(events, blk_1r, blk_1, miner_account);
  MAKE_ACCOUNT(events, alice_account);
  MAKE_ACCOUNT(events, bob_account);
  MAKE_TX_LIST_START(events, txs_0, miner_account, bob_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_1);
  MAKE_TX_LIST(events, txs_0, miner_account, alice_account, MK_COINS(1) + TESTS_DEFAULT_FEE, blk_1);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_2, blk_1r, miner_account, txs_0);

  MAKE_TX(events, tx_0, miner_account, miner_account, MK_COINS(60), blk_2);
  events.pop_back();

  MAKE_TX_MIX(events, tx_1, bob_account, miner_account, MK_COINS(1), 1, blk_2);
  events.pop_back();

  // Tx with nmix = 0 without signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  blobdata sr_tx = t_serializable_object_to_blob(static_cast<transaction_prefix>(tx_0));
  events.push_back(serialized_transaction(sr_tx));

  // Tx with nmix = 0 have a few inputs, and not enough signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(tx_0);
  sr_tx.resize(sr_tx.size() - sizeof(crypto::signature));
  events.push_back(serialized_transaction(sr_tx));

  // Tx with nmix = 0 have a few inputs, and too many signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(tx_0);
  sr_tx.insert(sr_tx.end(), sr_tx.end() - sizeof(crypto::signature), sr_tx.end());
  events.push_back(serialized_transaction(sr_tx));

  // Tx with nmix = 1 without signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(static_cast<transaction_prefix>(tx_1));
  events.push_back(serialized_transaction(sr_tx));

  // Tx with nmix = 1 have not enough signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(tx_1);
  sr_tx.resize(sr_tx.size() - sizeof(crypto::signature));
  events.push_back(serialized_transaction(sr_tx));

  // Tx with nmix = 1 have too many signatures
  DO_CALLBACK(events, "mark_invalid_tx");
  sr_tx = t_serializable_object_to_blob(tx_1);
  sr_tx.insert(sr_tx.end(), sr_tx.end() - sizeof(crypto::signature), sr_tx.end());
  events.push_back(serialized_transaction(sr_tx));

  return true;
}
