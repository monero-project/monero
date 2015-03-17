// Copyright (c) 2014, The Monero Project
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
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/list.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/foreach.hpp>
#include <atomic>

#include "syncobj.h"
#include "string_tools.h"
#include "cryptonote_basic.h"
#include "common/util.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "difficulty.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "checkpoints.h"
#include "blockchain_db/blockchain_db.h"

namespace cryptonote
{
  class tx_memory_pool;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class Blockchain
  {
  public:
    struct transaction_chain_entry
    {
      transaction tx;
      uint64_t m_keeper_block_height;
      size_t m_blob_size;
      std::vector<uint64_t> m_global_output_indexes;
    };

    struct block_extended_info
    {
      block   bl;
      uint64_t height;
      size_t block_cumulative_size;
      difficulty_type cumulative_difficulty;
      uint64_t already_generated_coins;
    };

    Blockchain(tx_memory_pool& tx_pool);

    bool init(const std::string& config_folder, const bool testnet = false, const int db_flags = 0);
    bool deinit();

    void set_checkpoints(checkpoints&& chk_pts) { m_checkpoints = chk_pts; }

    //bool push_new_block();
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs) const;
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks) const;
    bool get_alternative_blocks(std::list<block>& blocks) const;
    size_t get_alternative_blocks_count() const;
    crypto::hash get_block_id_by_height(uint64_t height) const;
    bool get_block_by_hash(const crypto::hash &h, block &blk) const;
    void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid) const;

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);

    bool have_tx(const crypto::hash &id) const;
    bool have_tx_keyimges_as_spent(const transaction &tx) const;
    bool have_tx_keyimg_as_spent(const crypto::key_image &key_im) const;

    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height = NULL) const;

    uint64_t get_current_blockchain_height() const;
    crypto::hash get_tail_id() const;
    crypto::hash get_tail_id(uint64_t& height) const;
    difficulty_type get_difficulty_for_next_block() const;
    bool add_new_block(const block& bl_, block_verification_context& bvc);
    bool reset_and_set_genesis_block(const block& b);
    bool create_block_template(block& b, const account_public_address& miner_address, difficulty_type& di, uint64_t& height, const blobdata& ex_nonce) const;
    bool have_block(const crypto::hash& id) const;
    size_t get_total_transactions() const;
    bool get_short_chain_history(std::list<crypto::hash>& ids) const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const;
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset) const;
    bool find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count) const;
    bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp);
    bool handle_get_objects(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) const;
    bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const;
    bool store_blockchain();
    bool check_tx_input(const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t* pmax_related_block_height = NULL) const;
    bool check_tx_inputs(const transaction& tx, uint64_t* pmax_used_block_height = NULL) const;
    bool check_tx_inputs(const transaction& tx, uint64_t& pmax_used_block_height, crypto::hash& max_used_block_id) const;
    uint64_t get_current_cumulative_blocksize_limit() const;
    bool is_storing_blockchain()const{return m_is_blockchain_storing;}
    uint64_t block_difficulty(uint64_t i) const;

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs) const;

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs) const;

    //debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string& file);

    void check_against_checkpoints(const checkpoints& points, bool enforce);
    void set_enforce_dns_checkpoints(bool enforce);
    bool update_checkpoints(const std::string& file_path, bool check_dns);

    BlockchainDB* get_db()
    {
      return m_db;
    }

  private:
    typedef std::unordered_map<crypto::hash, size_t> blocks_by_id_index;
    typedef std::unordered_map<crypto::hash, transaction_chain_entry> transactions_container;
    typedef std::unordered_set<crypto::key_image> key_images_container;
    typedef std::vector<block_extended_info> blocks_container;
    typedef std::unordered_map<crypto::hash, block_extended_info> blocks_ext_by_hash;
    typedef std::unordered_map<crypto::hash, block> blocks_by_hash;
    typedef std::map<uint64_t, std::vector<std::pair<crypto::hash, size_t>>> outputs_container; //crypto::hash - tx hash, size_t - index of out in transaction

    BlockchainDB* m_db;

    tx_memory_pool& m_tx_pool;
    mutable epee::critical_section m_blockchain_lock; // TODO: add here reader/writer lock

    // main chain
    blocks_container m_blocks;               // height  -> block_extended_info
    blocks_by_id_index m_blocks_index;       // crypto::hash -> height
    transactions_container m_transactions;
    key_images_container m_spent_keys;
    size_t m_current_block_cumul_sz_limit;


    // all alternative chains
    blocks_ext_by_hash m_alternative_chains; // crypto::hash -> block_extended_info

    // some invalid blocks
    blocks_ext_by_hash m_invalid_blocks;     // crypto::hash -> block_extended_info
    outputs_container m_outputs;


    std::string m_config_folder;
    checkpoints m_checkpoints;
    std::atomic<bool> m_is_in_checkpoint_zone;
    std::atomic<bool> m_is_blockchain_storing;
    bool m_testnet;
    bool m_enforce_dns_checkpoints;

    bool switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain, bool discard_disconnected_chain);
    block pop_block_from_blockchain();
    bool purge_transaction_from_blockchain(const crypto::hash& tx_id);
    bool purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check);

    bool handle_block_to_main_chain(const block& bl, block_verification_context& bvc);
    bool handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc);
    bool handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc);
    difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei) const;
    bool prevalidate_miner_transaction(const block& b, uint64_t height);
    bool validate_miner_transaction(const block& b, size_t cumulative_block_size, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins);
    bool validate_transaction(const block& b, uint64_t height, const transaction& tx);
    bool rollback_blockchain_switching(std::list<block>& original_chain, uint64_t rollback_height);
    bool add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height);
    bool push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes);
    bool pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id);
    void get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count) const;
    void add_out_to_get_random_outs(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i) const;
    bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
    bool add_block_as_invalid(const block& bl, const crypto::hash& h);
    bool add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h);
    bool check_block_timestamp(const block& b) const;
    bool check_block_timestamp(std::vector<uint64_t>& timestamps, const block& b) const;
    uint64_t get_adjusted_time() const;
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool update_next_cumulative_size_limit();

    bool check_for_double_spend(const transaction& tx, key_images_container& keys_this_block) const;
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  #define CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER    12

  //------------------------------------------------------------------

}  // namespace cryptonote

BOOST_CLASS_VERSION(cryptonote::Blockchain, CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER)
