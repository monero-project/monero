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

#pragma once

#include <string>
#include <vector>
#include <map>
#include "gtest/gtest.h"

#include "blockchain_db/blockchain_db.h"

class BaseTestDB: public cryptonote::BlockchainDB {
public:
  BaseTestDB() {}
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
  virtual cryptonote::blobdata get_block_blob_from_height(const uint64_t& height) const { return cryptonote::t_serializable_object_to_blob(get_block_from_height(height)); }
  virtual cryptonote::blobdata get_block_blob(const crypto::hash& h) const { return cryptonote::blobdata(); }
  virtual bool get_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const { return false; }
  virtual bool get_pruned_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const { return false; }
  virtual bool get_prunable_tx_blob(const crypto::hash& h, cryptonote::blobdata &tx) const { return false; }
  virtual bool get_prunable_tx_hash(const crypto::hash& tx_hash, crypto::hash &prunable_hash) const { return false; }
  virtual uint64_t get_block_height(const crypto::hash& h) const { return 0; }
  virtual cryptonote::block_header get_block_header(const crypto::hash& h) const { return cryptonote::block_header(); }
  virtual uint64_t get_block_timestamp(const uint64_t& height) const { return 0; }
  virtual std::vector<uint64_t> get_block_cumulative_rct_outputs(const std::vector<uint64_t> &heights) const { return {}; }
  virtual uint64_t get_top_block_timestamp() const { return 0; }
  virtual size_t get_block_weight(const uint64_t& height) const { return 128; }
  virtual cryptonote::difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const { return 10; }
  virtual cryptonote::difficulty_type get_block_difficulty(const uint64_t& height) const { return 0; }
  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const { return 10000000000; }
  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const { return crypto::hash(); }
  virtual std::vector<cryptonote::block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const { return std::vector<cryptonote::block>(); }
  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const { return std::vector<crypto::hash>(); }
  virtual crypto::hash top_block_hash() const { return crypto::hash(); }
  virtual cryptonote::block get_top_block() const { return cryptonote::block(); }
  virtual uint64_t height() const { return 1; }
  virtual bool tx_exists(const crypto::hash& h) const { return false; }
  virtual bool tx_exists(const crypto::hash& h, uint64_t& tx_index) const { return false; }
  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const { return 0; }
  virtual cryptonote::transaction get_tx(const crypto::hash& h) const { return cryptonote::transaction(); }
  virtual bool get_tx(const crypto::hash& h, cryptonote::transaction &tx) const { return false; }
  virtual uint64_t get_tx_count() const { return 0; }
  virtual std::vector<cryptonote::transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const { return std::vector<cryptonote::transaction>(); }
  virtual uint64_t get_tx_block_height(const crypto::hash& h) const { return 0; }
  virtual uint64_t get_num_outputs(const uint64_t& amount) const { return 1; }
  virtual uint64_t get_indexing_base() const { return 0; }
  virtual cryptonote::output_data_t get_output_key(const uint64_t& amount, const uint64_t& index) { return cryptonote::output_data_t(); }
  virtual cryptonote::tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const { return cryptonote::tx_out_index(); }
  virtual cryptonote::tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index) const { return cryptonote::tx_out_index(); }
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<cryptonote::tx_out_index> &indices) const {}
  virtual void get_output_key(const epee::span<const uint64_t> &amounts, const std::vector<uint64_t> &offsets, std::vector<cryptonote::output_data_t> &outputs, bool allow_partial = false) {}
  virtual bool can_thread_bulk_indices() const { return false; }
  virtual std::vector<uint64_t> get_tx_output_indices(const crypto::hash& h) const { return std::vector<uint64_t>(); }
  virtual std::vector<uint64_t> get_tx_amount_output_indices(const uint64_t tx_index) const { return std::vector<uint64_t>(); }
  virtual bool has_key_image(const crypto::key_image& img) const { return false; }
  virtual void remove_block() { }
  virtual uint64_t add_transaction_data(const crypto::hash& blk_hash, const cryptonote::transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prunable_hash) {return 0;}
  virtual void remove_transaction_data(const crypto::hash& tx_hash, const cryptonote::transaction& tx) {}
  virtual uint64_t add_output(const crypto::hash& tx_hash, const cryptonote::tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time, const rct::key *commitment) {return 0;}
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

  virtual void add_txpool_tx(const crypto::hash &txid, const cryptonote::blobdata &blob, const cryptonote::txpool_tx_meta_t& details) {}
  virtual void update_txpool_tx(const crypto::hash &txid, const cryptonote::txpool_tx_meta_t& details) {}
  virtual uint64_t get_txpool_tx_count(bool include_unrelayed_txes = true) const { return 0; }
  virtual bool txpool_has_tx(const crypto::hash &txid) const { return false; }
  virtual void remove_txpool_tx(const crypto::hash& txid) {}
  virtual bool get_txpool_tx_meta(const crypto::hash& txid, cryptonote::txpool_tx_meta_t &meta) const { return false; }
  virtual bool get_txpool_tx_blob(const crypto::hash& txid, cryptonote::blobdata &bd) const { return false; }
  virtual uint64_t get_database_size() const { return 0; }
  virtual cryptonote::blobdata get_txpool_tx_blob(const crypto::hash& txid) const { return ""; }
  virtual bool for_all_txpool_txes(std::function<bool(const crypto::hash&, const cryptonote::txpool_tx_meta_t&, const cryptonote::blobdata*)>, bool include_blob = false, bool include_unrelayed_txes = false) const { return false; }

  virtual void add_block( const cryptonote::block& blk
                        , size_t block_weight
                        , const cryptonote::difficulty_type& cumulative_difficulty
                        , const uint64_t& coins_generated
                        , uint64_t num_rct_outs
                        , const crypto::hash& blk_hash
                        ) { }
  virtual cryptonote::block get_block_from_height(const uint64_t& height) const { return cryptonote::block(); }
  virtual void set_hard_fork_version(uint64_t height, uint8_t version) {}
  virtual uint8_t get_hard_fork_version(uint64_t height) const { return 0; }
  virtual void check_hard_fork_info() {}

  virtual uint32_t get_blockchain_pruning_seed() const { return 0; }
  virtual bool prune_blockchain(uint32_t pruning_seed = 0) { return true; }
  virtual bool update_pruning() { return true; }
  virtual bool check_pruning() { return true; }
};

