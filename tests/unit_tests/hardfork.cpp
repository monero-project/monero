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

#include <algorithm>
#include "gtest/gtest.h"

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/hardfork.h"

using namespace cryptonote;

#define BLOCKS_PER_YEAR 525960
#define SECONDS_PER_YEAR 31557600


class TestDB: public BlockchainDB {
public:
  TestDB() {};
  virtual void open(const std::string& filename, const int db_flags = 0) { }
  virtual void close() {}
  virtual void sync() {}
  virtual void safesyncmode(const bool onoff) {}
  virtual void reset() {}
  virtual std::vector<std::string> get_filenames() const { return std::vector<std::string>(); }
  virtual bool remove_data_file(const std::string& folder) const { return true; }
  virtual std::string get_db_name() const { return std::string(); }
  virtual bool lock() { return true; }
  virtual void unlock() { }
  virtual bool batch_start(uint64_t batch_num_blocks=0, uint64_t batch_bytes=0) { return true; }
  virtual void batch_stop() {}
  virtual void set_batch_transactions(bool) {}
  virtual void block_txn_start(bool readonly=false) {}
  virtual void block_txn_stop() {}
  virtual void block_txn_abort() {}
  virtual void drop_hard_fork_info() {}
  virtual bool block_exists(const crypto::hash& h, uint64_t *height) const { return false; }
  virtual blobdata get_block_blob_from_height(const uint64_t& height) const { return cryptonote::t_serializable_object_to_blob(get_block_from_height(height)); }
  virtual blobdata get_block_blob(const crypto::hash& h) const { return blobdata(); }
  virtual bool get_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const { return false; }
  virtual bool get_pruned_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const { return false; }
  virtual bool get_prunable_tx_hash(const crypto::hash& tx_hash, crypto::hash &prunable_hash) const { return false; }
  virtual uint64_t get_block_height(const crypto::hash& h) const { return 0; }
  virtual block_header get_block_header(const crypto::hash& h) const { return block_header(); }
  virtual uint64_t get_block_timestamp(const uint64_t& height) const { return 0; }
  virtual std::vector<uint64_t> get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const { return {}; }
  virtual uint64_t get_top_block_timestamp() const { return 0; }
  virtual size_t get_block_weight(const uint64_t& height) const { return 128; }
  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const { return 10; }
  virtual difficulty_type get_block_difficulty(const uint64_t& height) const { return 0; }
  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const { return 10000000000; }
  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const { return crypto::hash(); }
  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const { return std::vector<block>(); }
  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const { return std::vector<crypto::hash>(); }
  virtual crypto::hash top_block_hash() const { return crypto::hash(); }
  virtual block get_top_block() const { return block(); }
  virtual uint64_t height() const { return blocks.size(); }
  virtual bool tx_exists(const crypto::hash& h) const { return false; }
  virtual bool tx_exists(const crypto::hash& h, uint64_t& tx_index) const { return false; }
  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const { return 0; }
  virtual transaction get_tx(const crypto::hash& h) const { return transaction(); }
  virtual bool get_tx(const crypto::hash& h, transaction &tx) const { return false; }
  virtual uint64_t get_tx_count() const { return 0; }
  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const { return std::vector<transaction>(); }
  virtual uint64_t get_tx_block_height(const crypto::hash& h) const { return 0; }
  virtual uint64_t get_num_outputs(const uint64_t& amount) const { return 1; }
  virtual uint64_t get_indexing_base() const { return 0; }
  virtual output_data_t get_output_key(const uint64_t& amount, const uint64_t& index) { return output_data_t(); }
  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const { return tx_out_index(); }
  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const { return tx_out_index(); }
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices) const {}
  virtual bool can_thread_bulk_indices() const { return false; }
  virtual std::vector<uint64_t> get_tx_output_indices(const crypto::hash& h) const { return std::vector<uint64_t>(); }
  virtual std::vector<uint64_t> get_tx_amount_output_indices(const uint64_t tx_index) const { return std::vector<uint64_t>(); }
  virtual bool has_key_image(const crypto::key_image& img) const { return false; }
  virtual void remove_block() { blocks.pop_back(); }
  virtual uint64_t add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash) {return 0;}
  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx) {}
  virtual uint64_t add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time, const rct::key *commitment) {return 0;}
  virtual void add_tx_amount_output_indices(const uint64_t tx_index, const std::vector<uint64_t>& amount_output_indices) {}
  virtual void add_spent_key(const crypto::key_image& k_image) {}
  virtual void remove_spent_key(const crypto::key_image& k_image) {}

  virtual bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const { return true; }
  virtual bool for_blocks_range(const uint64_t&, const uint64_t&, std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const { return true; }
  virtual bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>, bool pruned) const { return true; }
  virtual bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, uint64_t height, size_t tx_idx)> f) const { return true; }
  virtual bool for_all_outputs(uint64_t amount, const std::function<bool(uint64_t height)> &f) const { return true; }
  virtual bool is_read_only() const { return false; }
  virtual std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>> get_output_histogram(const std::vector<uint64_t> &amounts, bool unlocked, uint64_t recent_cutoff, uint64_t min_count) const { return std::map<uint64_t, std::tuple<uint64_t, uint64_t, uint64_t>>(); }
  virtual bool get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, std::vector<uint64_t> &distribution, uint64_t &base) const { return false; }

  virtual void set_service_node_data(const std::string& data) {}
  virtual bool get_service_node_data(std::string& data) { return false; }
  virtual void clear_service_node_data() {}

  virtual void add_txpool_tx(const transaction &tx, const txpool_tx_meta_t& details) {}
  virtual void update_txpool_tx(const crypto::hash &txid, const txpool_tx_meta_t& details) {}
  virtual uint64_t get_txpool_tx_count(bool include_unrelayed_txes = true) const { return 0; }
  virtual bool txpool_has_tx(const crypto::hash &txid) const { return false; }
  virtual void remove_txpool_tx(const crypto::hash& txid) {}
  virtual bool get_txpool_tx_meta(const crypto::hash& txid, txpool_tx_meta_t &meta) const { return false; }
  virtual bool get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd) const { return false; }
  virtual uint64_t get_database_size() const { return 0; }
  virtual cryptonote::blobdata get_txpool_tx_blob(const crypto::hash& txid) const { return ""; }
  virtual bool for_all_txpool_txes(std::function<bool(const crypto::hash&, const txpool_tx_meta_t&, const cryptonote::blobdata*)>, bool include_blob = false, bool include_unrelayed_txes = false) const { return false; }
  virtual output_data_t get_output_key(const uint64_t& amount, const uint64_t& index) const { return output_data_t(); };
  virtual void get_output_key(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs, bool allow_partial = false) const {};

  virtual void add_block( const block& blk
                        , size_t block_weight
                        , const difficulty_type& cumulative_difficulty
                        , const uint64_t& coins_generated
                        , uint64_t num_rct_outs
                        , const crypto::hash& blk_hash
                        ) {
    blocks.push_back(blk);
  }
  virtual block get_block_from_height(const uint64_t& height) const {
    return blocks.at(height);
  }
  virtual void set_hard_fork_version(uint64_t height, uint8_t version) {
    if (versions.size() <= height) 
      versions.resize(height+1); 
    versions[height] = version;
  }
  virtual uint8_t get_hard_fork_version(uint64_t height) const {
    return versions.at(height);
  }
  virtual void check_hard_fork_info() {}

private:
  std::vector<block> blocks;
  std::deque<uint8_t> versions;
};

static cryptonote::block mkblock(uint8_t version, uint8_t vote)
{
  cryptonote::block b;
  b.major_version = version;
  b.minor_version = vote;
  return b;
}

static cryptonote::block mkblock(const HardFork &hf, uint64_t height, uint8_t vote)
{
  cryptonote::block b;
  b.major_version = hf.get(height);
  b.minor_version = vote;
  return b;
}

TEST(major, Only)
{
  TestDB db;
  HardFork hf(db, 1, 0, 0, 1, 0); // no voting

  //                      v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 2, 1));
  hf.init();

  // block height 0, only version 1 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 0));
  ASSERT_FALSE(hf.add(mkblock(2, 2), 0));
  ASSERT_TRUE(hf.add(mkblock(1, 2), 0));
  db.add_block(mkblock(1, 1), 0, 0, 0, 0, crypto::hash());

  // block height 1, only version 1 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 1));
  ASSERT_FALSE(hf.add(mkblock(2, 2), 1));
  ASSERT_TRUE(hf.add(mkblock(1, 2), 1));
  db.add_block(mkblock(1, 1), 0, 0, 0, 0, crypto::hash());

  // block height 2, only version 2 is accepted
  ASSERT_FALSE(hf.add(mkblock(0, 2), 2));
  ASSERT_FALSE(hf.add(mkblock(1, 2), 2));
  ASSERT_FALSE(hf.add(mkblock(3, 2), 2));
  ASSERT_TRUE(hf.add(mkblock(2, 2), 2));
  db.add_block(mkblock(2, 1), 0, 0, 0, 0, crypto::hash());
}

TEST(empty_hardforks, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  hf.init();
  ASSERT_TRUE(hf.get_state(time(NULL)) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(time(NULL) + 3600*24*400) == HardFork::Ready);

  for (uint64_t h = 0; h <= 10; ++h) {
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
  ASSERT_EQ(hf.get(0), 1);
  ASSERT_EQ(hf.get(1), 1);
  ASSERT_EQ(hf.get(10), 1);
}

TEST(ordering, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(2, 2, 1));
  ASSERT_FALSE(hf.add_fork(3, 3, 1));
  ASSERT_FALSE(hf.add_fork(3, 2, 2));
  ASSERT_FALSE(hf.add_fork(2, 3, 2));
  ASSERT_TRUE(hf.add_fork(3, 10, 2));
  ASSERT_TRUE(hf.add_fork(4, 20, 3));
  ASSERT_FALSE(hf.add_fork(5, 5, 4));
}

TEST(check_for_height, Success)
{
  TestDB db;
  HardFork hf(db, 1, 0, 0, 1, 0); // no voting

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 5, 1));
  hf.init();

  for (uint64_t h = 0; h <= 4; ++h) {
    ASSERT_TRUE(hf.check_for_height(mkblock(1, 1), h));
    ASSERT_FALSE(hf.check_for_height(mkblock(2, 2), h));  // block version is too high
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 5; h <= 10; ++h) {
    ASSERT_FALSE(hf.check_for_height(mkblock(1, 1), h));  // block version is too low
    ASSERT_TRUE(hf.check_for_height(mkblock(2, 2), h));
    db.add_block(mkblock(hf, h, 2), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(get, next_version)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, 5, 1));
  ASSERT_TRUE(hf.add_fork(4, 10, 2));
  hf.init();

  for (uint64_t h = 0; h <= 4; ++h) {
    ASSERT_EQ(2, hf.get_next_version());
    db.add_block(mkblock(hf, h, 1), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 5; h <= 9; ++h) {
    ASSERT_EQ(4, hf.get_next_version());
    db.add_block(mkblock(hf, h, 2), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 10; h <= 15; ++h) {
    ASSERT_EQ(4, hf.get_next_version());
    db.add_block(mkblock(hf, h, 4), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(states, Success)
{
  TestDB db;
  HardFork hf(db);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(2, BLOCKS_PER_YEAR, SECONDS_PER_YEAR));

  ASSERT_TRUE(hf.get_state(0) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_UPDATE_TIME / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + (HardFork::DEFAULT_UPDATE_TIME + HardFork::DEFAULT_FORKED_TIME) / 2) == HardFork::UpdateNeeded);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_FORKED_TIME * 2) == HardFork::LikelyForked);

  ASSERT_TRUE(hf.add_fork(3, BLOCKS_PER_YEAR * 5, SECONDS_PER_YEAR * 5));

  ASSERT_TRUE(hf.get_state(0) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_UPDATE_TIME / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + (HardFork::DEFAULT_UPDATE_TIME + HardFork::DEFAULT_FORKED_TIME) / 2) == HardFork::Ready);
  ASSERT_TRUE(hf.get_state(SECONDS_PER_YEAR + HardFork::DEFAULT_FORKED_TIME * 2) == HardFork::Ready);
}

TEST(steps_asap, Success)
{
  TestDB db;
  HardFork hf(db, 1,1,1,1);

  //                 v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(4, 2, 1));
  ASSERT_TRUE(hf.add_fork(7, 4, 2));
  ASSERT_TRUE(hf.add_fork(9, 6, 3));
  hf.init();

  for (uint64_t h = 0; h < 10; ++h) {
    db.add_block(mkblock(hf, h, 9), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  ASSERT_EQ(hf.get(0), 1);
  ASSERT_EQ(hf.get(1), 1);
  ASSERT_EQ(hf.get(2), 4);
  ASSERT_EQ(hf.get(3), 4);
  ASSERT_EQ(hf.get(4), 7);
  ASSERT_EQ(hf.get(5), 7);
  ASSERT_EQ(hf.get(6), 9);
  ASSERT_EQ(hf.get(7), 9);
  ASSERT_EQ(hf.get(8), 9);
  ASSERT_EQ(hf.get(9), 9);
}

TEST(steps_1, Success)
{
  TestDB db;
  HardFork hf(db, 1,1,1,1);

  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  for (int n = 1 ; n < 10; ++n)
    ASSERT_TRUE(hf.add_fork(n+1, n, n));
  hf.init();

  for (uint64_t h = 0 ; h < 10; ++h) {
    db.add_block(mkblock(hf, h, h+1), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t h = 0; h < 10; ++h) {
    ASSERT_EQ(hf.get(h), std::max(1,(int)h));
  }
}

TEST(reorganize, Same)
{
  for (int history = 1; history <= 12; ++history) {
    TestDB db;
    HardFork hf(db, 1, 1, 1, history, 100);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(4, 2, 1));
    ASSERT_TRUE(hf.add_fork(7, 4, 2));
    ASSERT_TRUE(hf.add_fork(9, 6, 3));
    hf.init();

    //                                 index  0  1  2  3  4  5  6  7  8  9
    static const uint8_t block_versions[] = { 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
    for (uint64_t h = 0; h < 20; ++h) {
      db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, crypto::hash());
      ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
    }

    for (uint64_t rh = 0; rh < 20; ++rh) {
      hf.reorganize_from_block_height(rh);
      for (int hh = 0; hh < 20; ++hh) {
        uint8_t version = hh >= history ? block_versions[hh - history] : 1;
        ASSERT_EQ(hf.get(hh), version);
      }
    }
  }
}

TEST(reorganize, Changed)
{
  TestDB db;
  HardFork hf(db, 1, 1, 1, 4, 100);

  //                 v  h  t
  ASSERT_TRUE(hf.add_fork(1, 0, 0));
  ASSERT_TRUE(hf.add_fork(4, 2, 1));
  ASSERT_TRUE(hf.add_fork(7, 4, 2));
  ASSERT_TRUE(hf.add_fork(9, 6, 3));
  hf.init();

  //                                    fork         4     7     9
  //                                    index  0  1  2  3  4  5  6  7  8  9
  static const uint8_t block_versions[] =    { 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9 };
  static const uint8_t expected_versions[] = { 1, 1, 1, 1, 1, 1, 4, 4, 7, 7, 9, 9, 9, 9, 9, 9 };
  for (uint64_t h = 0; h < 16; ++h) {
    db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE (hf.add(db.get_block_from_height(h), h));
  }

  for (uint64_t rh = 0; rh < 16; ++rh) {
    hf.reorganize_from_block_height(rh);
    for (int hh = 0; hh < 16; ++hh) {
      ASSERT_EQ(hf.get(hh), expected_versions[hh]);
    }
  }

  // delay a bit for 9, and go back to 1 to check it stays at 9
  static const uint8_t block_versions_new[] =    { 1, 1, 4, 4, 7, 7, 4, 7, 7, 7, 9, 9, 9, 9, 9, 1 };
  static const uint8_t expected_versions_new[] = { 1, 1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 7, 7, 7, 9, 9 };
  for (uint64_t h = 3; h < 16; ++h) {
    db.remove_block();
  }
  ASSERT_EQ(db.height(), 3);
  hf.reorganize_from_block_height(2);
  for (uint64_t h = 3; h < 16; ++h) {
    db.add_block(mkblock(hf, h, block_versions_new[h]), 0, 0, 0, 0, crypto::hash());
    bool ret = hf.add(db.get_block_from_height(h), h);
    ASSERT_EQ (ret, h < 15);
  }
  db.remove_block(); // last block added to the blockchain, but not hf
  ASSERT_EQ(db.height(), 15);
  for (int hh = 0; hh < 15; ++hh) {
    ASSERT_EQ(hf.get(hh), expected_versions_new[hh]);
  }
}

TEST(voting, threshold)
{
  for (int threshold = 87; threshold <= 88; ++threshold) {
    TestDB db;
    HardFork hf(db, 1, 1, 1, 8, threshold);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    hf.init();

    for (uint64_t h = 0; h <= 8; ++h) {
      uint8_t v = 1 + !!(h % 8);
      db.add_block(mkblock(hf, h, v), 0, 0, 0, 0, crypto::hash());
      bool ret = hf.add(db.get_block_from_height(h), h);
      if (h >= 8 && threshold == 87) {
        // for threshold 87, we reach the treshold at height 7, so from height 8, hard fork to version 2, but 8 tries to add 1
        ASSERT_FALSE(ret);
      }
      else {
        // for threshold 88, we never reach the threshold
        ASSERT_TRUE(ret);
        uint8_t expected = threshold == 88 ? 1 : h < 8 ? 1 : 2;
        ASSERT_EQ(hf.get(h), expected);
      }
    }
  }
}

TEST(voting, different_thresholds)
{
  for (int threshold = 87; threshold <= 88; ++threshold) {
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50); // window size 4

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 5, 0, 1)); // asap
    ASSERT_TRUE(hf.add_fork(3, 10, 100, 2)); // all votes
    ASSERT_TRUE(hf.add_fork(4, 15, 3)); // default 50% votes
    hf.init();

    //                                           0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9
    static const uint8_t block_versions[] =    { 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4 };
    static const uint8_t expected_versions[] = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };

    for (uint64_t h = 0; h < sizeof(block_versions) / sizeof(block_versions[0]); ++h) {
      db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, crypto::hash());
      bool ret = hf.add(db.get_block_from_height(h), h);
      ASSERT_EQ(ret, true);
    }
    for (uint64_t h = 0; h < sizeof(expected_versions) / sizeof(expected_versions[0]); ++h) {
      ASSERT_EQ(hf.get(h), expected_versions[h]);
    }
  }
}

TEST(voting, info)
{
  TestDB db;
  HardFork hf(db, 1, 1, 1, 4, 50); // window size 4, default threshold 50%

  //                      v  h  ts
  ASSERT_TRUE(hf.add_fork(1, 0,  0));
  //                      v  h   thr  ts
  ASSERT_TRUE(hf.add_fork(2, 5,    0,  1)); // asap
  ASSERT_TRUE(hf.add_fork(3, 10, 100,  2)); // all votes
  //                      v   h  ts
  ASSERT_TRUE(hf.add_fork(4, 15,  3)); // default 50% votes
  hf.init();

  //                                             0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9
  static const uint8_t block_versions[]      = { 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4 };
  static const uint8_t expected_versions[]   = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4 };
  static const uint8_t expected_thresholds[] = { 0, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 2, 2, 2, 2 };

  for (uint64_t h = 0; h < sizeof(block_versions) / sizeof(block_versions[0]); ++h) {
    uint32_t window, votes, threshold;
    uint64_t earliest_height;
    uint8_t voting;

    ASSERT_TRUE(hf.get_voting_info(1, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h, 4), votes);
    ASSERT_EQ(0, earliest_height);

    ASSERT_EQ(hf.get_current_version() >= 2, hf.get_voting_info(2, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 3 ? 0 : h - 3, 4), votes);
    ASSERT_EQ(5, earliest_height);

    ASSERT_EQ(hf.get_current_version() >= 3, hf.get_voting_info(3, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 8 ? 0 : h - 8, 4), votes);
    ASSERT_EQ(10, earliest_height);

    ASSERT_EQ(hf.get_current_version() == 4, hf.get_voting_info(4, window, votes, threshold, earliest_height, voting));
    ASSERT_EQ(std::min<uint64_t>(h <= 14 ? 0 : h - 14, 4), votes);
    ASSERT_EQ(15, earliest_height);

    ASSERT_EQ(std::min<uint64_t>(h, 4), window);
    ASSERT_EQ(expected_thresholds[h], threshold);
    ASSERT_EQ(4, voting);

    db.add_block(mkblock(hf, h, block_versions[h]), 0, 0, 0, 0, crypto::hash());
    ASSERT_TRUE(hf.add(db.get_block_from_height(h), h));
  }
}

TEST(new_blocks, denied)
{
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    hf.init();

    ASSERT_TRUE(hf.add(mkblock(1, 1), 0));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 1));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 2));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 3));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 4));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 5));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 6));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 7));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 8)); // we reach 50% of the last 4
    ASSERT_FALSE(hf.add(mkblock(2, 1), 9)); // so this one can't get added
    ASSERT_TRUE(hf.add(mkblock(2, 2), 9));
}

TEST(new_version, early)
{
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 4, 1));
    hf.init();

    ASSERT_TRUE(hf.add(mkblock(1, 2), 0));
    ASSERT_TRUE(hf.add(mkblock(1, 2), 1)); // we have enough votes already
    ASSERT_TRUE(hf.add(mkblock(1, 2), 2));
    ASSERT_TRUE(hf.add(mkblock(1, 1), 3)); // we accept a previous version because we did not switch, even with all the votes
    ASSERT_TRUE(hf.add(mkblock(2, 2), 4)); // but have to wait for the declared height anyway
    ASSERT_TRUE(hf.add(mkblock(2, 2), 5));
    ASSERT_FALSE(hf.add(mkblock(2, 1), 6)); // we don't accept 1 anymore
    ASSERT_TRUE(hf.add(mkblock(2, 2), 7)); // but we do accept 2
}

TEST(reorganize, changed)
{
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(3, 5, 2));
    ASSERT_TRUE(hf.add_fork(4, 555, 222));
    hf.init();

#define ADD(v, h, a) \
  do { \
    cryptonote::block b = mkblock(hf, h, v); \
    db.add_block(b, 0, 0, 0, 0, crypto::hash()); \
    ASSERT_##a(hf.add(b, h)); \
  } while(0)
#define ADD_TRUE(v, h) ADD(v, h, TRUE)
#define ADD_FALSE(v, h) ADD(v, h, FALSE)

    ADD_TRUE(1, 0);
    ADD_TRUE(1, 1);
    ADD_TRUE(2, 2);
    ADD_TRUE(2, 3); // switch to 2 here
    ADD_TRUE(2, 4);
    ADD_TRUE(2, 5);
    ADD_TRUE(2, 6);
    ASSERT_EQ(hf.get_current_version(), 2);
    ADD_TRUE(3, 7);
    ADD_TRUE(4, 8);
    ADD_TRUE(4, 9);
    ASSERT_EQ(hf.get_current_version(), 3);

    // pop a few blocks and check current version goes back down
    db.remove_block();
    hf.reorganize_from_block_height(8);
    ASSERT_EQ(hf.get_current_version(), 3);
    db.remove_block();
    hf.reorganize_from_block_height(7);
    ASSERT_EQ(hf.get_current_version(), 2);
    db.remove_block();
    ASSERT_EQ(hf.get_current_version(), 2);

    // add blocks again, but remaining at 2
    ADD_TRUE(2, 7);
    ADD_TRUE(2, 8);
    ADD_TRUE(2, 9);
    ASSERT_EQ(hf.get_current_version(), 2); // we did not bump to 3 this time
}

TEST(get, higher)
{
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50);

    //                 v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(3, 5, 2));
    hf.init();

    ASSERT_EQ(hf.get_ideal_version(0), 1);
    ASSERT_EQ(hf.get_ideal_version(1), 1);
    ASSERT_EQ(hf.get_ideal_version(2), 2);
    ASSERT_EQ(hf.get_ideal_version(3), 2);
    ASSERT_EQ(hf.get_ideal_version(4), 2);
    ASSERT_EQ(hf.get_ideal_version(5), 3);
    ASSERT_EQ(hf.get_ideal_version(6), 3);
    ASSERT_EQ(hf.get_ideal_version(7), 3);
}

TEST(get, earliest_ideal_height)
{
    TestDB db;
    HardFork hf(db, 1, 1, 1, 4, 50);

    //                      v  h  t
    ASSERT_TRUE(hf.add_fork(1, 0, 0));
    ASSERT_TRUE(hf.add_fork(2, 2, 1));
    ASSERT_TRUE(hf.add_fork(5, 5, 2));
    ASSERT_TRUE(hf.add_fork(6, 10, 3));
    ASSERT_TRUE(hf.add_fork(9, 15, 4));
    hf.init();

    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(1), 0);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(2), 2);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(3), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(4), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(5), 5);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(6), 10);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(7), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(8), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(9), 15);
    ASSERT_EQ(hf.get_earliest_ideal_height_for_version(10), std::numeric_limits<uint64_t>::max());
}

