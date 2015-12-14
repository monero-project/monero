// Copyright (c) 2014-2015, The Monero Project
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

#include <ctime>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "p2p/net_node_common.h"
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"
#include "storages/portable_storage_template_helper.h"
#include "tx_pool.h"
#if BLOCKCHAIN_DB == DB_LMDB
#include "blockchain.h"
#else
#include "blockchain_storage.h"
#endif
#include "miner.h"
#include "connection_context.h"
#include "cryptonote_core/cryptonote_stat_info.h"
#include "warnings.h"
#include "crypto/hash.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4355)

namespace cryptonote
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
   class core: public i_miner_handler
   {
   public:
     core(i_cryptonote_protocol* pprotocol);
     bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote_connection_context& context);
     bool on_idle();
     bool handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block, bool relayed);
     bool handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate = true);
     bool prepare_handle_incoming_blocks(const std::list<block_complete_entry>  &blocks);
     bool cleanup_handle_incoming_blocks(bool force_sync = false);

     bool check_incoming_block_size(const blobdata& block_blob) const;
     i_cryptonote_protocol* get_protocol(){return m_pprotocol;}

     //-------------------- i_miner_handler -----------------------
     virtual bool handle_block_found( block& b);
     virtual bool get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce);


     miner& get_miner(){return m_miner;}
     const miner& get_miner()const{return m_miner;}
     static void init_options(boost::program_options::options_description& desc);
     bool init(const boost::program_options::variables_map& vm);
     bool set_genesis_block(const block& b);
     bool deinit();
     static void set_fast_exit();
     static bool get_fast_exit();
     void test_drop_download();
     void test_drop_download_height(uint64_t height);
     bool get_test_drop_download() const;
     bool get_test_drop_download_height() const;
     uint64_t get_current_blockchain_height() const;
     bool get_blockchain_top(uint64_t& heeight, crypto::hash& top_id) const;
     bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs) const;
     bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks) const;
     template<class t_ids_container, class t_blocks_container, class t_missed_container>
     bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs) const
     {
       return m_blockchain_storage.get_blocks(block_ids, blocks, missed_bs);
     }
     crypto::hash get_block_id_by_height(uint64_t height) const;
     bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs) const;
     bool get_block_by_hash(const crypto::hash &h, block &blk) const;
     //void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

     bool get_alternative_blocks(std::list<block>& blocks) const;
     size_t get_alternative_blocks_count() const;

     void set_cryptonote_protocol(i_cryptonote_protocol* pprotocol);
     void set_checkpoints(checkpoints&& chk_pts);
     void set_checkpoints_file_path(const std::string& path);
     void set_enforce_dns_checkpoints(bool enforce_dns);

     bool get_pool_transactions(std::list<transaction>& txs) const;
     bool get_pool_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos) const;
     size_t get_pool_transactions_count() const;
     size_t get_blockchain_total_transactions() const;
     //bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
     bool have_block(const crypto::hash& id) const;
     bool get_short_chain_history(std::list<crypto::hash>& ids) const;
     bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const;
     bool find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count) const;
     bool get_stat_info(core_stat_info& st_inf) const;
     //bool get_backward_blocks_sizes(uint64_t from_height, std::vector<size_t>& sizes, size_t count);
     bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const;
     crypto::hash get_tail_id() const;
     bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) const;
     void pause_mine();
     void resume_mine();
#if BLOCKCHAIN_DB == DB_LMDB
     Blockchain& get_blockchain_storage(){return m_blockchain_storage;}
     const Blockchain& get_blockchain_storage()const{return m_blockchain_storage;}
#else
     blockchain_storage& get_blockchain_storage(){return m_blockchain_storage;}
     const blockchain_storage& get_blockchain_storage()const{return m_blockchain_storage;}
#endif
     //debug functions
     void print_blockchain(uint64_t start_index, uint64_t end_index) const;
     void print_blockchain_index() const;
     std::string print_pool(bool short_format) const;
     void print_blockchain_outs(const std::string& file);
     void on_synchronized();

     void set_target_blockchain_height(uint64_t target_blockchain_height);
     uint64_t get_target_blockchain_height() const;

     bool update_checkpoints();

     void stop();

     bool is_key_image_spent(const crypto::key_image& key_im) const;
     bool are_key_images_spent(const std::vector<crypto::key_image>& key_im, std::vector<bool> &spent) const;

   private:
     bool add_new_tx(const transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block, bool relayed);
     bool add_new_tx(const transaction& tx, tx_verification_context& tvc, bool keeped_by_block, bool relayed);
     bool add_new_block(const block& b, block_verification_context& bvc);
     bool load_state_data();
     bool parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob) const;

     bool check_tx_syntax(const transaction& tx) const;
     //check correct values, amounts and all lightweight checks not related with database
     bool check_tx_semantic(const transaction& tx, bool keeped_by_block) const;
     //check if tx already in memory pool or in main blockchain

     bool check_tx_ring_signature(const txin_to_key& tx, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig) const;
     bool is_tx_spendtime_unlocked(uint64_t unlock_time) const;
     bool update_miner_block_template();
     bool handle_command_line(const boost::program_options::variables_map& vm);
     bool on_update_blocktemplate_interval();
     bool check_tx_inputs_keyimages_diff(const transaction& tx) const;
     void graceful_exit();
     bool check_fork_time();
     bool relay_txpool_transactions();

     static std::atomic<bool> m_fast_exit;
     bool m_test_drop_download = true;
     uint64_t m_test_drop_download_height = 0;

     tx_memory_pool m_mempool;
#if BLOCKCHAIN_DB == DB_LMDB
     Blockchain m_blockchain_storage;
#else
     blockchain_storage m_blockchain_storage;
#endif
     i_cryptonote_protocol* m_pprotocol;
     epee::critical_section m_incoming_tx_lock;
     //m_miner and m_miner_addres are probably temporary here
     miner m_miner;
     account_public_address m_miner_address;
     std::string m_config_folder;
     cryptonote_protocol_stub m_protocol_stub;
     epee::math_helper::once_a_time_seconds<60*60*12, false> m_store_blockchain_interval;
     epee::math_helper::once_a_time_seconds<60*60*2, false> m_fork_moaner;
     epee::math_helper::once_a_time_seconds<60*2, false> m_txpool_auto_relayer; //!< interval for checking re-relaying txpool transactions
     friend class tx_validate_inputs;
     std::atomic<bool> m_starter_message_showed;

     uint64_t m_target_blockchain_height;

     bool m_testnet;
     bool m_fakechain;
     std::string m_checkpoints_path;
     time_t m_last_dns_checkpoints_update;
     time_t m_last_json_checkpoints_update;

     std::atomic_flag m_checkpoints_updating;
   };
}

POP_WARNINGS
