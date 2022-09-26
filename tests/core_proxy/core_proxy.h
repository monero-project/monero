// Copyright (c) 2014-2022, The Monero Project
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

#include <boost/program_options/variables_map.hpp>

#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_core/i_core_events.h"
#include <unordered_map>

namespace tests
{
  struct block_index {
      size_t height;
      crypto::hash id;
      crypto::hash longhash;
      cryptonote::block blk;
      cryptonote::blobdata blob;
      std::list<cryptonote::transaction> txes;

      block_index() : height(0), id(crypto::null_hash), longhash(crypto::null_hash) { }
      block_index(size_t _height, const crypto::hash &_id, const crypto::hash &_longhash, const cryptonote::block &_blk, const cryptonote::blobdata &_blob, const std::list<cryptonote::transaction> &_txes)
          : height(_height), id(_id), longhash(_longhash), blk(_blk), blob(_blob), txes(_txes) { }
  };

  class proxy_core : public cryptonote::i_core_events
  {
      cryptonote::block m_genesis;
      std::list<crypto::hash> m_known_block_list;
      std::unordered_map<crypto::hash, block_index> m_hash2blkidx;

      crypto::hash m_lastblk;
      std::list<cryptonote::transaction> txes;

      bool add_block(const crypto::hash &_id, const crypto::hash &_longhash, const cryptonote::block &_blk, const cryptonote::blobdata &_blob);
      void build_short_history(std::list<crypto::hash> &m_history, const crypto::hash &m_start);
      

  public:
    virtual bool is_synchronized() const final { return true; }
    void on_synchronized(){}
    void safesyncmode(const bool){}
    virtual uint64_t get_current_blockchain_height() const final {return 1;}
    void set_target_blockchain_height(uint64_t) {}
    bool init(const boost::program_options::variables_map& vm);
    bool deinit(){return true;}
    bool get_short_chain_history(std::list<crypto::hash>& ids);
    bool have_block(const crypto::hash& id, int *where = NULL);
    bool have_block_unlocked(const crypto::hash& id, int *where = NULL);
    void get_blockchain_top(uint64_t& height, crypto::hash& top_id);
    bool handle_incoming_tx(const cryptonote::tx_blob_entry& tx_blob, cryptonote::tx_verification_context& tvc, cryptonote::relay_method tx_relay, bool relayed);
    bool handle_incoming_txs(const std::vector<cryptonote::tx_blob_entry>& tx_blobs, std::vector<cryptonote::tx_verification_context>& tvc, cryptonote::relay_method tx_relay, bool relayed);
    bool handle_incoming_block(const cryptonote::blobdata& block_blob, const cryptonote::block *block, cryptonote::block_verification_context& bvc, bool update_miner_blocktemplate = true);
    void pause_mine(){}
    void resume_mine(){}
    bool on_idle(){return true;}
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, bool clip_pruned, cryptonote::NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp){return true;}
    bool handle_get_objects(cryptonote::NOTIFY_REQUEST_GET_OBJECTS::request& arg, cryptonote::NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote::cryptonote_connection_context& context){return true;}
    cryptonote::Blockchain &get_blockchain_storage() { throw std::runtime_error("Called invalid member function: please never call get_blockchain_storage on the TESTING class proxy_core."); }
    bool get_test_drop_download() {return true;}
    bool get_test_drop_download_height() {return true;}
    bool prepare_handle_incoming_blocks(const std::vector<cryptonote::block_complete_entry>  &blocks_entry, std::vector<cryptonote::block> &blocks) { return true; }
    bool cleanup_handle_incoming_blocks(bool force_sync = false) { return true; }
    bool update_checkpoints(const bool skip_dns = false) { return true; }
    uint64_t get_target_blockchain_height() const { return 1; }
    size_t get_block_sync_size(uint64_t height) const { return BLOCKS_SYNCHRONIZING_DEFAULT_COUNT; }
    virtual void on_transactions_relayed(epee::span<const cryptonote::blobdata> tx_blobs, cryptonote::relay_method tx_relay) {}
    cryptonote::network_type get_nettype() const { return cryptonote::MAINNET; }
    bool get_pool_transaction(const crypto::hash& id, cryptonote::blobdata& tx_blob, cryptonote::relay_category tx_category) const { return false; }
    bool pool_has_tx(const crypto::hash &txid) const { return false; }
    bool get_blocks(uint64_t start_offset, size_t count, std::vector<std::pair<cryptonote::blobdata, cryptonote::block>>& blocks, std::vector<cryptonote::blobdata>& txs) const { return false; }
    bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::vector<cryptonote::transaction>& txs, std::vector<crypto::hash>& missed_txs) const { return false; }
    bool get_block_by_hash(const crypto::hash &h, cryptonote::block &blk, bool *orphan = NULL) const { return false; }
    uint8_t get_ideal_hard_fork_version() const { return 0; }
    uint8_t get_ideal_hard_fork_version(uint64_t height) const { return 0; }
    uint8_t get_hard_fork_version(uint64_t height) const { return 0; }
    uint64_t get_earliest_ideal_height_for_version(uint8_t version) const { return 0; }
    cryptonote::difficulty_type get_block_cumulative_difficulty(uint64_t height) const { return 0; }
    bool fluffy_blocks_enabled() const { return false; }
    uint64_t prevalidate_block_hashes(uint64_t height, const std::vector<crypto::hash> &hashes, const std::vector<uint64_t> &weights) { return 0; }
    bool has_block_weights(uint64_t height, uint64_t nblocks) const { return false; }
    bool is_within_compiled_block_hash_area(uint64_t height) const { return false; }
    bool pad_transactions() const { return false; }
    uint32_t get_blockchain_pruning_seed() const { return 0; }
    bool prune_blockchain(uint32_t pruning_seed) const { return true; }
    bool get_txpool_complement(const std::vector<crypto::hash> &hashes, std::vector<cryptonote::blobdata> &txes) { return false; }
    bool get_pool_transaction_hashes(std::vector<crypto::hash>& txs, bool include_unrelayed_txes = true) const { return false; }
    crypto::hash get_block_id_by_height(uint64_t height) const { return crypto::null_hash; }
  };
}
