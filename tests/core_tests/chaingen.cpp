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

#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <array>
#include <random>
#include <sstream>
#include <fstream>

#include "include_base_utils.h"

#include "console_handler.h"

#include "p2p/net_node.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/miner.h"

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/blockchain.h"
#include "blockchain_db/testdb.h"

#include "chaingen.h"
#include "device/device.hpp"
using namespace std;

using namespace epee;
using namespace crypto;
using namespace cryptonote;

namespace
{
  /**
   * Dummy TestDB to store height -> (block, hash) information
   * for the use only in the test_generator::fill_nonce() function,
   * which requires blockchain object to correctly compute PoW on HF12+ blocks
   * as the mining function requires it to obtain a valid seedhash.
   */
  class TestDB: public cryptonote::BaseTestDB
  {
  private:
    struct block_t
    {
      cryptonote::block bl;
      crypto::hash hash;
    };

  public:
    TestDB() { m_open = true; }

    virtual void add_block( const cryptonote::block& blk
        , size_t block_weight
        , uint64_t long_term_block_weight
        , const cryptonote::difficulty_type& cumulative_difficulty
        , const uint64_t& coins_generated
        , uint64_t num_rct_outs
        , const crypto::hash& blk_hash
        , const fcmp_pp::curve_trees::OutsByLastLockedBlock& outs_by_last_locked_block
        , const std::unordered_map<uint64_t/*output_id*/, uint64_t/*last locked block_id*/>& timelocked_outputs
    ) override
    {
      blocks.push_back({blk, blk_hash});
    }

    virtual uint64_t height() const override { return blocks.empty() ? 0 : blocks.size() - 1; }

    // Required for randomx
    virtual crypto::hash get_block_hash_from_height(const uint64_t &height) const override
    {
      if (height < blocks.size())
      {
        MDEBUG("Get hash for block height: " << height << " hash: " << blocks[height].hash);
        return blocks[height].hash;
      }

      MDEBUG("Get hash for block height: " << height << " zero-hash");
      crypto::hash hash = crypto::null_hash;
      *(uint64_t*)&hash = height;
      return hash;
    }

    virtual crypto::hash top_block_hash(uint64_t *block_height = NULL) const override
    {
      const uint64_t h = height();
      if (block_height != nullptr)
      {
        *block_height = h;
      }

      return get_block_hash_from_height(h);
    }

    virtual cryptonote::block get_top_block() const override
    {
      if (blocks.empty())
      {
        cryptonote::block b;
        return b;
      }

      return blocks[blocks.size()-1].bl;
    }

    virtual void pop_block(cryptonote::block &blk, std::vector<cryptonote::transaction> &txs) override { if (!blocks.empty()) blocks.pop_back(); }
    virtual void set_hard_fork_version(uint64_t height, uint8_t version) override { if (height >= hf.size()) hf.resize(height + 1); hf[height] = version; }
    virtual uint8_t get_hard_fork_version(uint64_t height) const override { if (height >= hf.size()) return 255; return hf[height]; }

  private:
    std::vector<block_t> blocks;
    std::vector<uint8_t> hf;
  };

}

static std::unique_ptr<cryptonote::BlockchainAndPool> init_blockchain(const std::vector<test_event_entry> & events, cryptonote::network_type nettype)
{
  v_hardforks_t hardforks;
  cryptonote::test_options test_options_tmp{nullptr, 0};
  const cryptonote::test_options * test_options = &test_options_tmp;
  if (!extract_hard_forks(events, hardforks))
  {
    MDEBUG("Extracting hard-forks from blocks");
    extract_hard_forks_from_blocks(events, hardforks);
  }

  hardforks.push_back(std::make_pair((uint8_t)0, (uint64_t)0));  // terminator
  test_options_tmp.hard_forks = hardforks.data();
  test_options = &test_options_tmp;

  std::unique_ptr<cryptonote::BlockchainAndPool> bap(new BlockchainAndPool());

  auto bdb = new TestDB();

  BOOST_FOREACH(const test_event_entry &ev, events)
  {
    if (typeid(block) != ev.type())
    {
      continue;
    }

    const block *blk = &boost::get<block>(ev);
    auto blk_hash = get_block_hash(*blk);
    bdb->add_block(*blk, 1, 1, 1, 0, 0, blk_hash, {}, {});
  }

  bool r = bap->blockchain.init(bdb, nettype, true, test_options, 2, nullptr);
  CHECK_AND_ASSERT_THROW_MES(r, "could not init blockchain from events");
  return bap;
}

void test_generator::get_block_chain(std::vector<block_info>& blockchain, const crypto::hash& head, size_t n) const
{
  crypto::hash curr = head;
  while (null_hash != curr && blockchain.size() < n)
  {
    auto it = m_blocks_info.find(curr);
    if (m_blocks_info.end() == it)
    {
      throw std::runtime_error("block hash wasn't found");
    }

    blockchain.push_back(it->second);
    curr = it->second.prev_id;
  }

  std::reverse(blockchain.begin(), blockchain.end());
}

void test_generator::get_last_n_block_weights(std::vector<size_t>& block_weights, const crypto::hash& head, size_t n) const
{
  std::vector<block_info> blockchain;
  get_block_chain(blockchain, head, n);
  BOOST_FOREACH(auto& bi, blockchain)
  {
    block_weights.push_back(bi.block_weight);
  }
}

uint64_t test_generator::get_already_generated_coins(const crypto::hash& blk_id) const
{
  auto it = m_blocks_info.find(blk_id);
  if (it == m_blocks_info.end())
    throw std::runtime_error("block hash wasn't found");

  return it->second.already_generated_coins;
}

uint64_t test_generator::get_already_generated_coins(const cryptonote::block& blk) const
{
  crypto::hash blk_hash;
  get_block_hash(blk, blk_hash);
  return get_already_generated_coins(blk_hash);
}

void test_generator::add_block(const cryptonote::block& blk, size_t txs_weight, std::vector<size_t>& block_weights, uint64_t already_generated_coins, uint64_t block_reward, uint8_t hf_version)
{
  const size_t block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
  m_blocks_info[get_block_hash(blk)] = block_info(blk.prev_id, already_generated_coins + block_reward, block_weight);
}

bool test_generator::construct_block(cryptonote::block& blk, uint64_t height, const crypto::hash& prev_id,
                                     const cryptonote::account_base& miner_acc, uint64_t timestamp, uint64_t already_generated_coins,
                                     std::vector<size_t>& block_weights, const std::list<cryptonote::transaction>& tx_list,
                                     const boost::optional<uint8_t>& hf_ver)
{
  blk.major_version = hf_ver ? hf_ver.get() : CURRENT_BLOCK_MAJOR_VERSION;
  blk.minor_version = hf_ver ? hf_ver.get() : CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp = timestamp;
  blk.prev_id = prev_id;

  blk.tx_hashes.reserve(tx_list.size());
  BOOST_FOREACH(const transaction &tx, tx_list)
  {
    crypto::hash tx_hash;
    get_transaction_hash(tx, tx_hash);
    blk.tx_hashes.push_back(tx_hash);
  }

  uint64_t total_fee = 0;
  size_t txs_weight = 0;
  BOOST_FOREACH(auto& tx, tx_list)
  {
    uint64_t fee = 0;
    bool r = get_tx_fee(tx, fee);
    CHECK_AND_ASSERT_MES(r, false, "wrong transaction passed to construct_block");
    total_fee += fee;
    txs_weight += get_transaction_weight(tx);
  }

  blk.miner_tx = AUTO_VAL_INIT(blk.miner_tx);
  size_t target_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
  while (true)
  {
    if (!construct_miner_tx(height, misc_utils::median(block_weights), already_generated_coins, target_block_weight, total_fee, miner_acc.get_keys().m_account_address, blk.miner_tx, blobdata(), 10, hf_ver ? hf_ver.get() : 1))
      return false;

    size_t actual_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
    if (target_block_weight < actual_block_weight)
    {
      target_block_weight = actual_block_weight;
    }
    else if (actual_block_weight < target_block_weight)
    {
      size_t delta = target_block_weight - actual_block_weight;
      blk.miner_tx.extra.resize(blk.miner_tx.extra.size() + delta, 0);
      actual_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
      if (actual_block_weight == target_block_weight)
      {
        break;
      }
      else
      {
        CHECK_AND_ASSERT_MES(target_block_weight < actual_block_weight, false, "Unexpected block size");
        delta = actual_block_weight - target_block_weight;
        blk.miner_tx.extra.resize(blk.miner_tx.extra.size() - delta);
        actual_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
        if (actual_block_weight == target_block_weight)
        {
          break;
        }
        else
        {
          CHECK_AND_ASSERT_MES(actual_block_weight < target_block_weight, false, "Unexpected block size");
          blk.miner_tx.extra.resize(blk.miner_tx.extra.size() + delta, 0);
          target_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
        }
      }
    }
    else
    {
      break;
    }
  }

  //blk.tree_root_hash = get_tree_hash(blk);

  fill_nonce(blk, get_test_difficulty(hf_ver), height);
  const uint64_t block_reward = get_outs_money_amount(blk.miner_tx) - total_fee;
  add_block(blk, txs_weight, block_weights, already_generated_coins, block_reward, hf_ver ? hf_ver.get() : 1);

  return true;
}

bool test_generator::construct_block(cryptonote::block& blk, const cryptonote::account_base& miner_acc, uint64_t timestamp)
{
  std::vector<size_t> block_weights;
  std::list<cryptonote::transaction> tx_list;
  return construct_block(blk, 0, null_hash, miner_acc, timestamp, 0, block_weights, tx_list);
}

bool test_generator::construct_block(cryptonote::block& blk, const cryptonote::block& blk_prev,
                                     const cryptonote::account_base& miner_acc,
                                     const std::list<cryptonote::transaction>& tx_list/* = std::list<cryptonote::transaction>()*/,
                                     const boost::optional<uint8_t>& hf_ver)
{
  uint64_t height = boost::get<txin_gen>(blk_prev.miner_tx.vin.front()).height + 1;
  crypto::hash prev_id = get_block_hash(blk_prev);
  // Keep difficulty unchanged
  uint64_t timestamp = blk_prev.timestamp + current_difficulty_window(hf_ver); // DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN;
  uint64_t already_generated_coins = get_already_generated_coins(prev_id);
  std::vector<size_t> block_weights;
  get_last_n_block_weights(block_weights, prev_id, CRYPTONOTE_REWARD_BLOCKS_WINDOW);

  return construct_block(blk, height, prev_id, miner_acc, timestamp, already_generated_coins, block_weights, tx_list, hf_ver);
}

bool test_generator::construct_block_manually(block& blk, const block& prev_block, const account_base& miner_acc,
                                              int actual_params/* = bf_none*/, uint8_t major_ver/* = 0*/,
                                              uint8_t minor_ver/* = 0*/, uint64_t timestamp/* = 0*/,
                                              const crypto::hash& prev_id/* = crypto::hash()*/, const difficulty_type& diffic/* = 1*/,
                                              const transaction& miner_tx/* = transaction()*/,
                                              const std::vector<crypto::hash>& tx_hashes/* = std::vector<crypto::hash>()*/,
                                              size_t txs_weight/* = 0*/, size_t max_outs/* = 0*/, uint8_t hf_version/* = 1*/,
                                              uint64_t fees/* = 0*/, const crypto::ec_point& fcmp_pp_tree_root/* = crypto::ec_point{}*/)
{
  blk.major_version = actual_params & bf_major_ver ? major_ver : CURRENT_BLOCK_MAJOR_VERSION;
  blk.minor_version = actual_params & bf_minor_ver ? minor_ver : CURRENT_BLOCK_MINOR_VERSION;
  blk.timestamp     = actual_params & bf_timestamp ? timestamp : prev_block.timestamp + DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN; // Keep difficulty unchanged
  blk.prev_id       = actual_params & bf_prev_id   ? prev_id   : get_block_hash(prev_block);
  blk.tx_hashes     = actual_params & bf_tx_hashes ? tx_hashes : std::vector<crypto::hash>();
  max_outs          = actual_params & bf_max_outs ? max_outs : 9999;
  hf_version        = actual_params & bf_hf_version ? hf_version : 1;
  fees              = actual_params & bf_tx_fees ? fees : 0;

  size_t height = get_block_height(prev_block) + 1;
  uint64_t already_generated_coins = get_already_generated_coins(prev_block);
  std::vector<size_t> block_weights;
  get_last_n_block_weights(block_weights, get_block_hash(prev_block), CRYPTONOTE_REWARD_BLOCKS_WINDOW);
  if (actual_params & bf_miner_tx)
  {
    blk.miner_tx = miner_tx;
  }
  else
  {
    size_t current_block_weight = txs_weight + get_transaction_weight(blk.miner_tx);
    // TODO: This will work, until size of constructed block is less then CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE
    if (!construct_miner_tx(height, misc_utils::median(block_weights), already_generated_coins, current_block_weight, fees, miner_acc.get_keys().m_account_address, blk.miner_tx, blobdata(), max_outs, hf_version))
      return false;
  }

  //blk.tree_root_hash = get_tree_hash(blk);
  if (blk.major_version >= HF_VERSION_FCMP_PLUS_PLUS)
    blk.fcmp_pp_tree_root = fcmp_pp_tree_root;

  difficulty_type a_diffic = actual_params & bf_diffic ? diffic : get_test_difficulty(hf_version);
  fill_nonce(blk, a_diffic, height);

  const uint64_t block_reward = get_outs_money_amount(blk.miner_tx) - fees;
  add_block(blk, txs_weight, block_weights, already_generated_coins, block_reward, hf_version);

  return true;
}

bool test_generator::construct_block_manually_tx(cryptonote::block& blk, const cryptonote::block& prev_block,
                                                 const cryptonote::account_base& miner_acc,
                                                 const std::vector<crypto::hash>& tx_hashes, size_t txs_weight)
{
  return construct_block_manually(blk, prev_block, miner_acc, bf_tx_hashes, 0, 0, 0, crypto::hash(), 0, transaction(), tx_hashes, txs_weight);
}

void test_generator::fill_nonce(cryptonote::block& blk, const difficulty_type& diffic, uint64_t height)
{
  const cryptonote::Blockchain *blockchain = nullptr;
  std::unique_ptr<cryptonote::BlockchainAndPool> bap;

  if (blk.major_version >= RX_BLOCK_VERSION && diffic > 1)
  {
    if (m_events == nullptr)
    {
      MDEBUG("events not set, RandomX PoW can fail due to zero seed hash");
    }
    else
    {
      bap = init_blockchain(*m_events, m_nettype);
      blockchain = &bap->blockchain;
    }
  }

  blk.nonce = 0;
  while (!miner::find_nonce_for_given_block([blockchain](const cryptonote::block &b, uint64_t height, const crypto::hash *seed_hash, unsigned int threads, crypto::hash &hash){
    return cryptonote::get_block_longhash(blockchain, b, hash, height, seed_hash, threads);
  }, blk, diffic, height, NULL)) {
    blk.timestamp++;
  }
}

namespace
{
  uint64_t get_inputs_amount(const vector<tx_source_entry> &s)
  {
    uint64_t r = 0;
    BOOST_FOREACH(const tx_source_entry &e, s)
    {
      r += e.amount;
    }

    return r;
  }
}

bool init_output_indices(map_output_idx_t& outs, std::map<uint64_t, std::vector<size_t> >& outs_mine, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, const cryptonote::account_base& from) {

    BOOST_FOREACH (const block& blk, blockchain) {
        vector<const transaction*> vtx;
        vtx.push_back(&blk.miner_tx);

        BOOST_FOREACH(const crypto::hash &h, blk.tx_hashes) {
            const map_hash2tx_t::const_iterator cit = mtx.find(h);
            if (mtx.end() == cit)
                throw std::runtime_error("block contains an unknown tx hash");

            vtx.push_back(cit->second);
        }

        //vtx.insert(vtx.end(), blk.);
        // TODO: add all other txes
        for (size_t i = 0; i < vtx.size(); i++) {
            const transaction &tx = *vtx[i];

            for (size_t j = 0; j < tx.vout.size(); ++j) {
                const tx_out &out = tx.vout[j];

                output_index oi(out.target, out.amount, boost::get<txin_gen>(*blk.miner_tx.vin.begin()).height, i, j, &blk, vtx[i]);
                oi.set_rct(tx.version == 2);
                oi.unlock_time = tx.unlock_time;
                oi.is_coin_base = i == 0;

                if (2 == out.target.which()) { // out_to_key
                    outs[out.amount].push_back(oi);
                    size_t tx_global_idx = outs[out.amount].size() - 1;
                    outs[out.amount][tx_global_idx].idx = tx_global_idx;
                    // Is out to me?
                    crypto::public_key output_public_key;
                    cryptonote::get_output_public_key(out, output_public_key);
                    if (is_out_to_acc(from.get_keys(), output_public_key, get_tx_pub_key_from_extra(tx), get_additional_tx_pub_keys_from_extra(tx), j)) {
                        outs_mine[out.amount].push_back(tx_global_idx);
                    }
                }
            }
        }
    }

    return true;
}

bool init_spent_output_indices(map_output_idx_t& outs, map_output_t& outs_mine, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, const cryptonote::account_base& from) {

    BOOST_FOREACH (const map_output_t::value_type &o, outs_mine) {
        for (size_t i = 0; i < o.second.size(); ++i) {
            output_index &oi = outs[o.first][o.second[i]];

            // construct key image for this output
            crypto::key_image img;
            keypair in_ephemeral;
            crypto::public_key out_key = boost::get<txout_to_key>(oi.out).key;
            std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
            subaddresses[from.get_keys().m_account_address.m_spend_public_key] = {0,0};
            generate_key_image_helper(from.get_keys(), subaddresses, out_key, get_tx_pub_key_from_extra(*oi.p_tx), get_additional_tx_pub_keys_from_extra(*oi.p_tx), oi.out_no, in_ephemeral, img, hw::get_device(("default")));

            // lookup for this key image in the events vector
            BOOST_FOREACH(auto& tx_pair, mtx) {
                const transaction& tx = *tx_pair.second;
                BOOST_FOREACH(const txin_v &in, tx.vin) {
                    if (typeid(txin_to_key) == in.type()) {
                        const txin_to_key &itk = boost::get<txin_to_key>(in);
                        if (itk.k_image == img) {
                            oi.spent = true;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool fill_output_entries(std::vector<output_index>& out_indices, size_t sender_out, size_t nmix, size_t& real_entry_idx, std::vector<tx_source_entry::output_entry>& output_entries, uint64_t cur_height, const boost::optional<fnc_accept_output_t>& fnc_accept = boost::none)
{
  if (out_indices.size() <= nmix)
    return false;

  bool sender_out_found = false;
  size_t rest = nmix;
  for (size_t i = 0; i < out_indices.size() && (0 < rest || !sender_out_found); ++i)
  {
    const output_index& oi = out_indices[i];
    if (oi.spent)
      continue;
    if (oi.unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && oi.unlock_time > cur_height + 1)
      continue;
    if (fnc_accept && !(fnc_accept.get())({.oi=oi, .cur_height=cur_height}))
      continue;

    bool append = false;
    if (i == sender_out)
    {
      append = true;
      sender_out_found = true;
      real_entry_idx = output_entries.size();
    }
    else if (0 < rest)
    {
      --rest;
      append = true;
    }

    if (append)
    {
      rct::key comm = oi.commitment();
      const txout_to_key& otk = boost::get<txout_to_key>(oi.out);
      output_entries.push_back(tx_source_entry::output_entry(oi.idx, rct::ctkey({rct::pk2rct(otk.key), comm})));
    }
  }

  return 0 == rest && sender_out_found;
}

bool fill_tx_sources(std::vector<tx_source_entry>& sources, const std::vector<test_event_entry>& events,
                     const block& blk_head, const cryptonote::account_base& from, uint64_t amount, size_t nmix,
                     bool check_unlock_time, const boost::optional<fnc_accept_output_t>& fnc_accept)
{
    map_output_idx_t outs;
    map_output_t outs_mine;

    std::vector<cryptonote::block> blockchain;
    map_hash2tx_t mtx;
    if (!find_block_chain(events, blockchain, mtx, get_block_hash(blk_head)))
        return false;

    if (!init_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;

    if (!init_spent_output_indices(outs, outs_mine, blockchain, mtx, from))
        return false;

    // Iterate in reverse is more efficiency
    uint64_t head_height = check_unlock_time ? get_block_height(blk_head) : std::numeric_limits<uint64_t>::max() - 1;
    uint64_t sources_amount = 0;
    bool sources_found = false;
    BOOST_REVERSE_FOREACH(const map_output_t::value_type o, outs_mine)
    {
        for (size_t i = 0; i < o.second.size() && !sources_found; ++i)
        {
            size_t sender_out = o.second[i];
            const output_index& oi = outs[o.first][sender_out];
            if (oi.spent)
                continue;
            if (oi.rct)
                continue;
            if (oi.unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && oi.unlock_time > head_height + 1)
                continue;
            if (fnc_accept && !(fnc_accept.get())({.oi=oi, .cur_height=head_height}))
                continue;

            cryptonote::tx_source_entry ts;
            ts.amount = oi.amount;
            ts.real_output_in_tx_index = oi.out_no;
            ts.real_out_tx_key = get_tx_pub_key_from_extra(*oi.p_tx); // incoming tx public key
            size_t realOutput;
            if (!fill_output_entries(outs[o.first], sender_out, nmix, realOutput, ts.outputs, head_height, fnc_accept))
              continue;

            ts.real_output = realOutput;
            ts.rct = false;
            ts.mask = rct::identity();  // non-rct has identity mask by definition

            rct::key comm = rct::zeroCommitVartime(ts.amount);
            for(auto & ot : ts.outputs)
              ot.second.mask = comm;

            sources.push_back(ts);

            sources_amount += ts.amount;
            sources_found = amount <= sources_amount;
        }

        if (sources_found)
            break;
    }

    return sources_found;
}

bool fill_tx_destination(tx_destination_entry &de, const cryptonote::account_public_address &to, uint64_t amount) {
    de.addr = to;
    de.amount = amount;
    return true;
}

map_txid_output_t::iterator block_tracker::find_out(const crypto::hash &txid, size_t out)
{
  return find_out(std::make_pair(txid, out));
}

map_txid_output_t::iterator block_tracker::find_out(const output_hasher &id)
{
  return m_map_outs.find(id);
}

void block_tracker::process(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx)
{
  std::vector<const cryptonote::block*> blks;
  blks.reserve(blockchain.size());

  BOOST_FOREACH (const block& blk, blockchain) {
    auto hsh = get_block_hash(blk);
    auto it = m_blocks.find(hsh);
    if (it == m_blocks.end()){
      m_blocks[hsh] = blk;
    }

    blks.push_back(&m_blocks[hsh]);
  }

  process(blks, mtx);
}

void block_tracker::process(const std::vector<const cryptonote::block*>& blockchain, const map_hash2tx_t& mtx)
{
  BOOST_FOREACH (const block* blk, blockchain) {
    vector<const transaction*> vtx;
    vtx.push_back(&(blk->miner_tx));

    BOOST_FOREACH(const crypto::hash &h, blk->tx_hashes) {
      const map_hash2tx_t::const_iterator cit = mtx.find(h);
      CHECK_AND_ASSERT_THROW_MES(mtx.end() != cit, "block contains an unknown tx hash");
      vtx.push_back(cit->second);
    }

    for (size_t i = 0; i < vtx.size(); i++) {
      process(blk, vtx[i], i);
    }
  }
}

void block_tracker::process(const block* blk, const transaction * tx, size_t i)
{
  for (size_t j = 0; j < tx->vout.size(); ++j) {
    const tx_out &out = tx->vout[j];

    if (typeid(cryptonote::txout_to_key) != out.target.type() && typeid(cryptonote::txout_to_tagged_key) != out.target.type()) {
      continue;
    }

    const uint64_t rct_amount = tx->version == 2 ? 0 : out.amount;
    const output_hasher hid = std::make_pair(tx->hash, j);
    auto it = find_out(hid);
    if (it != m_map_outs.end()){
      continue;
    }

    output_index oi(out.target, out.amount, boost::get<txin_gen>(blk->miner_tx.vin.front()).height, i, j, blk, tx);
    oi.set_rct(tx->version == 2);
    oi.idx = m_outs[rct_amount].size();
    oi.unlock_time = tx->unlock_time;
    oi.is_coin_base = tx->vin.size() == 1 && tx->vin.back().type() == typeid(cryptonote::txin_gen);

    m_outs[rct_amount].push_back(oi);
    m_map_outs.insert({hid, oi});
  }
}

void block_tracker::global_indices(const cryptonote::transaction *tx, std::vector<uint64_t> &indices)
{
  indices.clear();

  for(size_t j=0; j < tx->vout.size(); ++j){
    auto it = find_out(tx->hash, j);
    if (it != m_map_outs.end()){
      indices.push_back(it->second.idx);
    }
  }
}

void block_tracker::get_fake_outs(size_t num_outs, uint64_t amount, uint64_t global_index, uint64_t cur_height, std::vector<get_outs_entry> &outs, const boost::optional<fnc_accept_output_t>& fnc_accept){
  auto & vct = m_outs[amount];
  const size_t n_outs = vct.size();
  CHECK_AND_ASSERT_THROW_MES(n_outs > 0, "n_outs is 0");

  std::set<size_t> used;
  std::vector<size_t> choices;
  choices.resize(n_outs);
  for(size_t i=0; i < n_outs; ++i) choices[i] = i;
  shuffle(choices.begin(), choices.end(), std::default_random_engine(crypto::rand<unsigned>()));

  size_t n_iters = 0;
  ssize_t idx = -1;
  outs.reserve(num_outs);
  while(outs.size() < num_outs){
    n_iters += 1;
    idx = (idx + 1) % n_outs;
    size_t oi_idx = choices[(size_t)idx];
    CHECK_AND_ASSERT_THROW_MES((n_iters / n_outs) <= outs.size(), "Fake out pick selection problem");

    auto & oi = vct[oi_idx];
    if (oi.idx == global_index)
      continue;
    if (oi.out.type() != typeid(cryptonote::txout_to_key))
      continue;
    if (oi.unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && oi.unlock_time > cur_height + 1)
      continue;
    if (used.find(oi_idx) != used.end())
      continue;
    if (fnc_accept && !(fnc_accept.get())({.oi=oi, .cur_height=cur_height}))
      continue;

    rct::key comm = oi.commitment();
    auto out = boost::get<txout_to_key>(oi.out);
    outs.emplace_back(oi.idx, out.key, comm);
    used.insert(oi_idx);
  }
}

std::string block_tracker::dump_data()
{
  ostringstream ss;
  for (auto &m_out : m_outs)
  {
    auto & vct = m_out.second;
    ss << m_out.first << " => |vector| = " << vct.size() << '\n';

    for (const auto & oi : vct)
    {
      auto out = boost::get<txout_to_key>(oi.out);

      ss << "    idx: " << oi.idx
      << ", rct: " << oi.rct
      << ", xmr: " << oi.amount
      << ", key: " << dump_keys(out.key.data)
      << ", msk: " << dump_keys(oi.comm.bytes)
      << ", txid: " << dump_keys(oi.p_tx->hash.data)
      << '\n';
    }
  }

  return ss.str();
}

void block_tracker::dump_data(const std::string & fname)
{
  ofstream myfile;
  myfile.open (fname);
  myfile << dump_data();
  myfile.close();
}

std::string dump_data(const cryptonote::transaction &tx)
{
  ostringstream ss;
  ss << "msg: " << dump_keys(tx.rct_signatures.message.bytes)
     << ", vin: ";

  for(auto & in : tx.vin){
    if (typeid(txin_to_key) == in.type()){
      auto tk = boost::get<txin_to_key>(in);
      std::vector<uint64_t> full_off;
      int64_t last = -1;

      ss << " i: " << tk.amount << " [";
      for(auto ix : tk.key_offsets){
        ss << ix << ", ";
        if (last == -1){
          last = ix;
          full_off.push_back(ix);
        } else {
          last += ix;
          full_off.push_back((uint64_t)last);
        }
      }

      ss << "], full: [";
      for(auto ix : full_off){
        ss << ix << ", ";
      }
      ss << "]; ";

    } else if (typeid(txin_gen) == in.type()){
      ss << " h: " << boost::get<txin_gen>(in).height << ", ";
    } else {
      ss << " ?, ";
    }
  }

  ss << ", mixring: \n";
  for (const auto & row : tx.rct_signatures.mixRing){
    for(auto cur : row){
      ss << "    (" << dump_keys(cur.dest.bytes) << ", " << dump_keys(cur.mask.bytes) << ")\n ";
    }
    ss << "; ";
  }

  return ss.str();
}

cryptonote::account_public_address get_address(const var_addr_t& inp)
{
  if (typeid(cryptonote::account_public_address) == inp.type()){
    return boost::get<cryptonote::account_public_address>(inp);
  } else if(typeid(cryptonote::account_keys) == inp.type()){
    return boost::get<cryptonote::account_keys>(inp).m_account_address;
  } else if (typeid(cryptonote::account_base) == inp.type()){
    return boost::get<cryptonote::account_base>(inp).get_keys().m_account_address;
  } else if (typeid(cryptonote::tx_destination_entry) == inp.type()){
    return boost::get<cryptonote::tx_destination_entry>(inp).addr;
  } else {
    throw std::runtime_error("Unexpected type");
  }
}

cryptonote::account_public_address get_address(const cryptonote::account_public_address& inp)
{
  return inp;
}

cryptonote::account_public_address get_address(const cryptonote::account_keys& inp)
{
  return inp.m_account_address;
}

cryptonote::account_public_address get_address(const cryptonote::account_base& inp)
{
  return inp.get_keys().m_account_address;
}

cryptonote::account_public_address get_address(const cryptonote::tx_destination_entry& inp)
{
  return inp.addr;
}

uint64_t sum_amount(const std::vector<tx_destination_entry>& destinations)
{
  uint64_t amount = 0;
  for(auto & cur : destinations){
    amount += cur.amount;
  }

  return amount;
}

uint64_t sum_amount(const std::vector<cryptonote::tx_source_entry>& sources)
{
  uint64_t amount = 0;
  for(auto & cur : sources){
    amount += cur.amount;
  }

  return amount;
}

void fill_tx_destinations(const var_addr_t& from, const std::vector<tx_destination_entry>& dests,
                          uint64_t fee,
                          const std::vector<tx_source_entry> &sources,
                          std::vector<tx_destination_entry>& destinations,
                          bool always_change)

{
  destinations.clear();
  uint64_t amount = sum_amount(dests);
  std::copy(dests.begin(), dests.end(), std::back_inserter(destinations));

  tx_destination_entry de_change;
  uint64_t cache_back = get_inputs_amount(sources) - (amount + fee);

  if (cache_back > 0 || always_change) {
    if (!fill_tx_destination(de_change, get_address(from), cache_back <= 0 ? 0 : cache_back))
      throw std::runtime_error("couldn't fill transaction cache back destination");
    destinations.push_back(de_change);
  }
}

void fill_tx_destinations(const var_addr_t& from, const cryptonote::account_public_address& to,
                          uint64_t amount, uint64_t fee,
                          const std::vector<tx_source_entry> &sources,
                          std::vector<tx_destination_entry>& destinations,
                          std::vector<tx_destination_entry>& destinations_pure,
                          bool always_change)
{
  destinations.clear();

  tx_destination_entry de;
  if (!fill_tx_destination(de, to, amount))
    throw std::runtime_error("couldn't fill transaction destination");
  destinations.push_back(de);
  destinations_pure.push_back(de);

  tx_destination_entry de_change;
  uint64_t cache_back = get_inputs_amount(sources) - (amount + fee);

  if (cache_back > 0 || always_change) {
    if (!fill_tx_destination(de_change, get_address(from), cache_back <= 0 ? 0 : cache_back))
      throw std::runtime_error("couldn't fill transaction cache back destination");
    destinations.push_back(de_change);
  }
}

void fill_tx_destinations(const var_addr_t& from, const cryptonote::account_public_address& to,
                          uint64_t amount, uint64_t fee,
                          const std::vector<tx_source_entry> &sources,
                          std::vector<tx_destination_entry>& destinations, bool always_change)
{
  std::vector<tx_destination_entry> destinations_pure;
  fill_tx_destinations(from, to, amount, fee, sources, destinations, destinations_pure, always_change);
}

void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_public_address& to,
                                      uint64_t amount, uint64_t fee, size_t nmix, std::vector<tx_source_entry>& sources,
                                      std::vector<tx_destination_entry>& destinations, bool check_unlock_time,
                                      const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept)
{
  sources.clear();
  destinations.clear();

  if (!fill_tx_sources(sources, events, blk_head, from, amount + fee, nmix, check_unlock_time, fnc_tx_in_accept))
    throw tx_construct_tx_fill_error();

  fill_tx_destinations(from, to, amount, fee, sources, destinations, false);
}

void fill_tx_sources_and_destinations(const std::vector<test_event_entry>& events, const block& blk_head,
                                      const cryptonote::account_base& from, const cryptonote::account_base& to,
                                      uint64_t amount, uint64_t fee, size_t nmix, std::vector<tx_source_entry>& sources,
                                      std::vector<tx_destination_entry>& destinations, bool check_unlock_time,
                                      const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept)
{
  fill_tx_sources_and_destinations(events, blk_head, from, to.get_keys().m_account_address, amount, fee, nmix, sources, destinations, check_unlock_time, fnc_tx_in_accept);
}

cryptonote::tx_destination_entry build_dst(const var_addr_t& to, bool is_subaddr, uint64_t amount)
{
  tx_destination_entry de;
  de.amount = amount;
  de.addr = get_address(to);
  de.is_subaddress = is_subaddr;
  return de;
}

std::vector<cryptonote::tx_destination_entry> build_dsts(const var_addr_t& to1, bool sub1, uint64_t am1, size_t repeat)
{
  std::vector<cryptonote::tx_destination_entry> res;
  res.reserve(repeat);
  for(size_t i = 0; i < repeat; ++i)
  {
    res.emplace_back(build_dst(to1, sub1, am1));
  }
  return res;
}

std::vector<cryptonote::tx_destination_entry> build_dsts(std::initializer_list<dest_wrapper_t> inps)
{
  std::vector<cryptonote::tx_destination_entry> res;
  res.reserve(inps.size());
  for(auto & c : inps){
    res.push_back(build_dst(c.addr, c.is_subaddr, c.amount));
  }
  return res;
}

bool construct_miner_tx_manually(size_t height, uint64_t already_generated_coins,
                                 const account_public_address& miner_address, transaction& tx, uint64_t fee,
                                 uint8_t hf_version/* = 1*/, keypair* p_txkey/* = 0*/)
{
  keypair txkey;
  txkey = keypair::generate(hw::get_device("default"));
  add_tx_pub_key_to_extra(tx, txkey.pub);

  if (0 != p_txkey)
    *p_txkey = txkey;

  txin_gen in;
  in.height = height;
  tx.vin.push_back(in);

  // This will work, until size of constructed block is less then CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE
  uint64_t block_reward;
  if (!get_block_reward(0, 0, already_generated_coins, block_reward, hf_version))
  {
    LOG_PRINT_L0("Block is too big");
    return false;
  }
  block_reward += fee;

  crypto::key_derivation derivation;
  crypto::public_key out_eph_public_key;
  crypto::generate_key_derivation(miner_address.m_view_public_key, txkey.sec, derivation);
  crypto::derive_public_key(derivation, 0, miner_address.m_spend_public_key, out_eph_public_key);

  bool use_view_tags = hf_version >= HF_VERSION_VIEW_TAGS;
  crypto::view_tag view_tag;
  if (use_view_tags)
    crypto::derive_view_tag(derivation, 0, view_tag);

  tx_out out;
  cryptonote::set_tx_out(block_reward, out_eph_public_key, use_view_tags, view_tag, out);

  tx.vout.push_back(out);

  if (hf_version >= HF_VERSION_DYNAMIC_FEE)
    tx.version = 2;
  else
    tx.version = 1;
  tx.unlock_time = height + CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW;

  return true;
}

bool construct_tx_to_key(const std::vector<test_event_entry>& events, cryptonote::transaction& tx, const cryptonote::block& blk_head,
                         const cryptonote::account_base& from, const var_addr_t& to, uint64_t amount,
                         uint64_t fee, size_t nmix, bool rct, rct::RangeProofType range_proof_type, int bp_version,
                         bool check_unlock_time, const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept)
{
  vector<tx_source_entry> sources;
  vector<tx_destination_entry> destinations;
  fill_tx_sources_and_destinations(events, blk_head, from, get_address(to), amount, fee, nmix, sources, destinations, check_unlock_time, fnc_tx_in_accept);

  return construct_tx_rct(from.get_keys(), sources, destinations, from.get_keys().m_account_address, std::vector<uint8_t>(), tx, rct, range_proof_type, bp_version);
}

bool construct_tx_to_key(const std::vector<test_event_entry>& events, cryptonote::transaction& tx, const cryptonote::block& blk_head,
                         const cryptonote::account_base& from, const std::vector<cryptonote::tx_destination_entry>& destinations,
                         uint64_t fee, size_t nmix, bool rct, rct::RangeProofType range_proof_type, int bp_version,
                         bool check_unlock_time, const boost::optional<fnc_accept_output_t>& fnc_tx_in_accept)
{
  vector<tx_source_entry> sources;
  vector<tx_destination_entry> destinations_all;
  uint64_t amount = sum_amount(destinations);

  if (!fill_tx_sources(sources, events, blk_head, from, amount + fee, nmix, check_unlock_time, fnc_tx_in_accept))
    throw tx_construct_tx_fill_error();

  fill_tx_destinations(from, destinations, fee, sources, destinations_all, false);

  return construct_tx_rct(from.get_keys(), sources, destinations_all, get_address(from), std::vector<uint8_t>(), tx, rct, range_proof_type, bp_version);
}

bool construct_tx_to_key(cryptonote::transaction& tx,
                         const cryptonote::account_base& from, const var_addr_t& to, uint64_t amount,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct, rct::RangeProofType range_proof_type, int bp_version)
{
  vector<tx_destination_entry> destinations;
  fill_tx_destinations(from, get_address(to), amount, fee, sources, destinations, rct);
  return construct_tx_rct(from.get_keys(), sources, destinations, get_address(from), std::vector<uint8_t>(), tx, rct, range_proof_type, bp_version);
}

bool construct_tx_to_key(cryptonote::transaction& tx,
                         const cryptonote::account_base& from,
                         const std::vector<cryptonote::tx_destination_entry>& destinations,
                         std::vector<cryptonote::tx_source_entry> &sources,
                         uint64_t fee, bool rct, rct::RangeProofType range_proof_type, int bp_version)
{
  vector<tx_destination_entry> all_destinations;
  fill_tx_destinations(from, destinations, fee, sources, all_destinations, rct);
  return construct_tx_rct(from.get_keys(), sources, all_destinations, get_address(from), std::vector<uint8_t>(), tx, rct, range_proof_type, bp_version);
}

bool construct_tx_rct(const cryptonote::account_keys& sender_account_keys, std::vector<cryptonote::tx_source_entry>& sources, const std::vector<cryptonote::tx_destination_entry>& destinations, const boost::optional<cryptonote::account_public_address>& change_addr, std::vector<uint8_t> extra, cryptonote::transaction& tx, bool rct, rct::RangeProofType range_proof_type, int bp_version)
{
  std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
  subaddresses[sender_account_keys.m_account_address.m_spend_public_key] = {0, 0};
  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  std::vector<tx_destination_entry> destinations_copy = destinations;
  rct::RCTConfig rct_config = {range_proof_type, bp_version};
  fcmp_pp::ProofParams fcmp_pp_params = {};
  return construct_tx_and_get_tx_key(sender_account_keys, subaddresses, sources, destinations_copy, change_addr, extra, tx, tx_key, additional_tx_keys, fcmp_pp_params, rct, rct_config);
}

transaction construct_tx_with_fee(std::vector<test_event_entry>& events, const block& blk_head,
                                  const account_base& acc_from, const var_addr_t& to, uint64_t amount, uint64_t fee)
{
  transaction tx;
  construct_tx_to_key(events, tx, blk_head, acc_from, to, amount, fee, 0);
  events.push_back(tx);
  return tx;
}

uint64_t get_balance(const cryptonote::account_base& addr, const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx) {
    uint64_t res = 0;
    std::map<uint64_t, std::vector<output_index> > outs;
    std::map<uint64_t, std::vector<size_t> > outs_mine;

    map_hash2tx_t confirmed_txs;
    get_confirmed_txs(blockchain, mtx, confirmed_txs);

    if (!init_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    if (!init_spent_output_indices(outs, outs_mine, blockchain, confirmed_txs, addr))
        return false;

    BOOST_FOREACH (const map_output_t::value_type &o, outs_mine) {
        for (size_t i = 0; i < o.second.size(); ++i) {
            if (outs[o.first][o.second[i]].spent)
                continue;

            res += outs[o.first][o.second[i]].amount;
        }
    }

    return res;
}

bool extract_hard_forks(const std::vector<test_event_entry>& events, v_hardforks_t& hard_forks)
{
  for(auto & ev : events)
  {
    if (typeid(event_replay_settings) == ev.type())
    {
      const auto & rep_settings = boost::get<event_replay_settings>(ev);
      if (rep_settings.hard_forks)
      {
        const auto & hf = rep_settings.hard_forks.get();
        std::copy(hf.begin(), hf.end(), std::back_inserter(hard_forks));
      }
    }
  }

  return !hard_forks.empty();
}

bool extract_hard_forks_from_blocks(const std::vector<test_event_entry>& events, v_hardforks_t& hard_forks)
{
  int hf = -1;
  int64_t height = 0;

  for(auto & ev : events)
  {
    if (typeid(block) != ev.type())
    {
      continue;
    }

    const block *blk = &boost::get<block>(ev);
    if (blk->major_version != hf)
    {
      hf = blk->major_version;
      hard_forks.push_back(std::make_pair(blk->major_version, (uint64_t)height));
    }

    height += 1;
  }

  return !hard_forks.empty();
}

void get_confirmed_txs(const std::vector<cryptonote::block>& blockchain, const map_hash2tx_t& mtx, map_hash2tx_t& confirmed_txs)
{
  std::unordered_set<crypto::hash> confirmed_hashes;
  BOOST_FOREACH(const block& blk, blockchain)
  {
    BOOST_FOREACH(const crypto::hash& tx_hash, blk.tx_hashes)
    {
      confirmed_hashes.insert(tx_hash);
    }
  }

  BOOST_FOREACH(const auto& tx_pair, mtx)
  {
    if (0 != confirmed_hashes.count(tx_pair.first))
    {
      confirmed_txs.insert(tx_pair);
    }
  }
}

bool trim_block_chain(std::vector<cryptonote::block>& blockchain, const crypto::hash& tail){
  size_t cut = 0;
  bool found = true;

  for(size_t i = 0; i < blockchain.size(); ++i){
    crypto::hash chash = get_block_hash(blockchain[i]);
    if (chash == tail){
      cut = i;
      found = true;
      break;
    }
  }

  if (found && cut > 0){
    blockchain.erase(blockchain.begin(), blockchain.begin() + cut);
  }

  return found;
}

bool trim_block_chain(std::vector<const cryptonote::block*>& blockchain, const crypto::hash& tail){
  size_t cut = 0;
  bool found = true;

  for(size_t i = 0; i < blockchain.size(); ++i){
    crypto::hash chash = get_block_hash(*blockchain[i]);
    if (chash == tail){
      cut = i;
      found = true;
      break;
    }
  }

  if (found && cut > 0){
    blockchain.erase(blockchain.begin(), blockchain.begin() + cut);
  }

  return found;
}

uint64_t num_blocks(const std::vector<test_event_entry>& events)
{
  uint64_t res = 0;
  BOOST_FOREACH(const test_event_entry& ev, events)
  {
    if (typeid(block) == ev.type())
    {
      res += 1;
    }
  }

  return res;
}

cryptonote::block get_head_block(const std::vector<test_event_entry>& events)
{
  for(auto it = events.rbegin(); it != events.rend(); ++it)
  {
    auto &ev = *it;
    if (typeid(block) == ev.type())
    {
      return boost::get<block>(ev);
    }
  }

  throw std::runtime_error("No block event");
}

bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<cryptonote::block>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head) {
    std::unordered_map<crypto::hash, const block*> block_index;
    BOOST_FOREACH(const test_event_entry& ev, events)
    {
        if (typeid(block) == ev.type())
        {
            const block* blk = &boost::get<block>(ev);
            block_index[get_block_hash(*blk)] = blk;
        }
        else if (typeid(transaction) == ev.type())
        {
            const transaction& tx = boost::get<transaction>(ev);
            mtx[get_transaction_hash(tx)] = &tx;
        }
    }

    bool b_success = false;
    crypto::hash id = head;
    for (auto it = block_index.find(id); block_index.end() != it; it = block_index.find(id))
    {
        blockchain.push_back(*it->second);
        id = it->second->prev_id;
        if (null_hash == id)
        {
            b_success = true;
            break;
        }
    }
    reverse(blockchain.begin(), blockchain.end());

    return b_success;
}

bool find_block_chain(const std::vector<test_event_entry>& events, std::vector<const cryptonote::block*>& blockchain, map_hash2tx_t& mtx, const crypto::hash& head) {
    std::unordered_map<crypto::hash, const block*> block_index;
    BOOST_FOREACH(const test_event_entry& ev, events)
    {
        if (typeid(block) == ev.type())
        {
            const block* blk = &boost::get<block>(ev);
            block_index[get_block_hash(*blk)] = blk;
        }
        else if (typeid(transaction) == ev.type())
        {
            const transaction& tx = boost::get<transaction>(ev);
            mtx[get_transaction_hash(tx)] = &tx;
        }
    }

    bool b_success = false;
    crypto::hash id = head;
    for (auto it = block_index.find(id); block_index.end() != it; it = block_index.find(id))
    {
        blockchain.push_back(it->second);
        id = it->second->prev_id;
        if (null_hash == id)
        {
            b_success = true;
            break;
        }
    }
    reverse(blockchain.begin(), blockchain.end());
    return b_success;
}


void test_chain_unit_base::register_callback(const std::string& cb_name, verify_callback cb)
{
  m_callbacks[cb_name] = cb;
}
bool test_chain_unit_base::verify(const std::string& cb_name, cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  auto cb_it = m_callbacks.find(cb_name);
  if(cb_it == m_callbacks.end())
  {
    LOG_ERROR("Failed to find callback " << cb_name);
    return false;
  }
  return cb_it->second(c, ev_index, events);
}

bool test_chain_unit_base::check_block_verification_context(const cryptonote::block_verification_context& bvc, size_t event_idx, const cryptonote::block& /*blk*/)
{
  return !bvc.m_verifivation_failed;
}

bool test_chain_unit_base::check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool /*tx_added*/, size_t /*event_index*/, const cryptonote::transaction& /*tx*/)
{
  return !tvc.m_verifivation_failed;
}

bool test_chain_unit_base::check_tx_verification_context_array(const std::vector<cryptonote::tx_verification_context>& tvcs, size_t /*tx_added*/, size_t /*event_index*/, const std::vector<cryptonote::transaction>& /*txs*/)
{
  for (const cryptonote::tx_verification_context &tvc: tvcs)
    if (tvc.m_verifivation_failed)
      return false;
  return true;
}
