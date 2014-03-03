// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "p2p/net_node_common.h"
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"
#include "storages/portable_storage_template_helper.h"
#include "tx_pool.h"
#include "blockchain_storage.h"
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
     bool handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block);
     bool handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate = true);
     i_cryptonote_protocol* get_protocol(){return m_pprotocol;}

     //-------------------- i_miner_handler -----------------------
     virtual bool handle_block_found( block& b);
     virtual bool get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce);


     miner& get_miner(){return m_miner;}
     static void init_options(boost::program_options::options_description& desc);
     bool init(const boost::program_options::variables_map& vm);
     bool set_genesis_block(const block& b);
     bool deinit();
     uint64_t get_current_blockchain_height();
     bool get_blockchain_top(uint64_t& heeight, crypto::hash& top_id);
     bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs);
     bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks);
     template<class t_ids_container, class t_blocks_container, class t_missed_container>
     bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)
     {
       return m_blockchain_storage.get_blocks(block_ids, blocks, missed_bs);
     }
     crypto::hash get_block_id_by_height(uint64_t height);
     bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs);
     bool get_transaction(const crypto::hash &h, transaction &tx);
     bool get_block_by_hash(const crypto::hash &h, block &blk);
     void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

     bool get_alternative_blocks(std::list<block>& blocks);
     size_t get_alternative_blocks_count();

     void set_cryptonote_protocol(i_cryptonote_protocol* pprotocol);
     void set_checkpoints(checkpoints&& chk_pts);

     bool get_pool_transactions(std::list<transaction>& txs);
     size_t get_pool_transactions_count();
     size_t get_blockchain_total_transactions();
     bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
     bool have_block(const crypto::hash& id);
     bool get_short_chain_history(std::list<crypto::hash>& ids);
     bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp);
     bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count);
     bool get_stat_info(core_stat_info& st_inf);
     bool get_backward_blocks_sizes(uint64_t from_height, std::vector<size_t>& sizes, size_t count);
     bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs);
     crypto::hash get_tail_id();
     bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
     void pause_mine();
     void resume_mine();
     blockchain_storage& get_blockchain_storage(){return m_blockchain_storage;}
     //debug functions
     void print_blockchain(uint64_t start_index, uint64_t end_index);
     void print_blockchain_index();
     std::string print_pool(bool short_format);
     void print_blockchain_outs(const std::string& file);
     void on_synchronized();

   private:
     bool add_new_tx(const transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block);
     bool add_new_tx(const transaction& tx, tx_verification_context& tvc, bool keeped_by_block);
     bool add_new_block(const block& b, block_verification_context& bvc);
     bool load_state_data();
     bool parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob);

     bool check_tx_syntax(const transaction& tx);
     //check correct values, amounts and all lightweight checks not related with database
     bool check_tx_semantic(const transaction& tx, bool keeped_by_block);
     //check if tx already in memory pool or in main blockchain

     bool is_key_image_spent(const crypto::key_image& key_im);

     bool check_tx_ring_signature(const txin_to_key& tx, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig);
     bool is_tx_spendtime_unlocked(uint64_t unlock_time);
     bool update_miner_block_template();
     bool handle_command_line(const boost::program_options::variables_map& vm);
     bool on_update_blocktemplate_interval();
     bool check_tx_inputs_keyimages_diff(const transaction& tx);


     tx_memory_pool m_mempool;
     blockchain_storage m_blockchain_storage;
     i_cryptonote_protocol* m_pprotocol;
     critical_section m_incoming_tx_lock;
     //m_miner and m_miner_addres are probably temporary here
     miner m_miner;
     account_public_address m_miner_address;
     std::string m_config_folder;
     cryptonote_protocol_stub m_protocol_stub;
     math_helper::once_a_time_seconds<60*60*12, false> m_store_blockchain_interval;
     friend class tx_validate_inputs;
     std::atomic<bool> m_starter_message_showed;
   };
}

POP_WARNINGS
