// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

#include "tx_pool.h"
#include "cryptonote_basic.h"
#include "common/util.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "difficulty.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "verification_context.h"
#include "crypto/hash.h"
#include "checkpoints.h"

namespace cryptonote
{

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class blockchain_storage
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

    blockchain_storage(tx_memory_pool& tx_pool):m_tx_pool(tx_pool), m_current_block_cumul_sz_limit(0), m_is_in_checkpoint_zone(false)
    {};

    bool init() { return init(tools::get_default_data_dir()); }
    bool init(const std::string& config_folder);
    bool deinit();

    void set_checkpoints(checkpoints&& chk_pts) { m_checkpoints = chk_pts; }

    //bool push_new_block();
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs);
    bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks);
    bool get_alternative_blocks(std::list<block>& blocks);
    size_t get_alternative_blocks_count();
    crypto::hash get_block_id_by_height(uint64_t height);
    bool get_block_by_hash(const crypto::hash &h, block &blk);
    void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

    template<class archive_t>
    void serialize(archive_t & ar, const unsigned int version);

    bool have_tx(const crypto::hash &id);
    bool have_tx_keyimges_as_spent(const transaction &tx);
    bool have_tx_keyimg_as_spent(const crypto::key_image &key_im);
    transaction *get_tx(const crypto::hash &id);

    template<class visitor_t>
    bool scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height = NULL);

    uint64_t get_current_blockchain_height();
    crypto::hash get_tail_id();
    crypto::hash get_tail_id(uint64_t& height);
    difficulty_type get_difficulty_for_next_block();
    bool add_new_block(const block& bl_, block_verification_context& bvc);
    bool reset_and_set_genesis_block(const block& b);
    bool create_block_template(block& b, const account_public_address& miner_address, difficulty_type& di, uint64_t& height, const blobdata& ex_nonce);
    bool have_block(const crypto::hash& id);
    size_t get_total_transactions();
    bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
    bool get_short_chain_history(std::list<crypto::hash>& ids);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, uint64_t& starter_offset);
    bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count);
    bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp);
    bool handle_get_objects(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res);
    bool get_backward_blocks_sizes(size_t from_height, std::vector<size_t>& sz, size_t count);
    bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs);
    bool store_blockchain();
    bool check_tx_input(const txin_to_key& txin, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig, uint64_t* pmax_related_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, const crypto::hash& tx_prefix_hash, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t* pmax_used_block_height = NULL);
    bool check_tx_inputs(const transaction& tx, uint64_t& pmax_used_block_height, crypto::hash& max_used_block_id);
    uint64_t get_current_comulative_blocksize_limit();
    bool is_storing_blockchain(){return m_is_blockchain_storing;}
    uint64_t block_difficulty(size_t i);

    template<class t_ids_container, class t_blocks_container, class t_missed_container>
    bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& bl_id, block_ids)
      {
        auto it = m_blocks_index.find(bl_id);
        if(it == m_blocks_index.end())
          missed_bs.push_back(bl_id);
        else
        {
          CHECK_AND_ASSERT_MES(it->second < m_blocks.size(), false, "Internal error: bl_id=" << string_tools::pod_to_hex(bl_id)
            << " have index record with offset="<<it->second<< ", bigger then m_blocks.size()=" << m_blocks.size());
            blocks.push_back(m_blocks[it->second].bl);
        }
      }
      return true;
    }

    template<class t_ids_container, class t_tx_container, class t_missed_container>
    bool get_transactions(const t_ids_container& txs_ids, t_tx_container& txs, t_missed_container& missed_txs)
    {
      CRITICAL_REGION_LOCAL(m_blockchain_lock);

      BOOST_FOREACH(const auto& tx_id, txs_ids)
      {
        auto it = m_transactions.find(tx_id);
        if(it == m_transactions.end())
        {
          transaction tx;
          if(!m_tx_pool.get_transaction(tx_id, tx))
            missed_txs.push_back(tx_id);
          else
            txs.push_back(tx);
        }
        else
          txs.push_back(it->second.tx);
      }
      return true;
    }
    //debug functions
    void print_blockchain(uint64_t start_index, uint64_t end_index);
    void print_blockchain_index();
    void print_blockchain_outs(const std::string& file);

  private:
    typedef std::unordered_map<crypto::hash, size_t> blocks_by_id_index;
    typedef std::unordered_map<crypto::hash, transaction_chain_entry> transactions_container;
    typedef std::unordered_set<crypto::key_image> key_images_container;
    typedef std::vector<block_extended_info> blocks_container;
    typedef std::unordered_map<crypto::hash, block_extended_info> blocks_ext_by_hash;
    typedef std::unordered_map<crypto::hash, block> blocks_by_hash;
    typedef std::map<uint64_t, std::vector<std::pair<crypto::hash, size_t>>> outputs_container; //crypto::hash - tx hash, size_t - index of out in transaction

    tx_memory_pool& m_tx_pool;
    critical_section m_blockchain_lock; // TODO: add here reader/writer lock

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

    bool switch_to_alternative_blockchain(std::list<blocks_ext_by_hash::iterator>& alt_chain);
    bool pop_block_from_blockchain();
    bool purge_block_data_from_blockchain(const block& b, size_t processed_tx_count);
    bool purge_transaction_from_blockchain(const crypto::hash& tx_id);
    bool purge_transaction_keyimages_from_blockchain(const transaction& tx, bool strict_check);

    bool handle_block_to_main_chain(const block& bl, block_verification_context& bvc);
    bool handle_block_to_main_chain(const block& bl, const crypto::hash& id, block_verification_context& bvc);
    bool handle_alternative_block(const block& b, const crypto::hash& id, block_verification_context& bvc);
    difficulty_type get_next_difficulty_for_alternative_chain(const std::list<blocks_ext_by_hash::iterator>& alt_chain, block_extended_info& bei);
    bool prevalidate_miner_transaction(const block& b, uint64_t height);
    bool validate_miner_transaction(const block& b, size_t cumulative_block_size, uint64_t fee, uint64_t& base_reward, uint64_t already_generated_coins);
    bool validate_transaction(const block& b, uint64_t height, const transaction& tx);
    bool rollback_blockchain_switching(std::list<block>& original_chain, size_t rollback_height);
    bool add_transaction_from_block(const transaction& tx, const crypto::hash& tx_id, const crypto::hash& bl_id, uint64_t bl_height);
    bool push_transaction_to_global_outs_index(const transaction& tx, const crypto::hash& tx_id, std::vector<uint64_t>& global_indexes);
    bool pop_transaction_from_global_index(const transaction& tx, const crypto::hash& tx_id);
    bool get_last_n_blocks_sizes(std::vector<size_t>& sz, size_t count);
    bool add_out_to_get_random_outs(std::vector<std::pair<crypto::hash, size_t> >& amount_outs, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& result_outs, uint64_t amount, size_t i);
    bool is_tx_spendtime_unlocked(uint64_t unlock_time);
    bool add_block_as_invalid(const block& bl, const crypto::hash& h);
    bool add_block_as_invalid(const block_extended_info& bei, const crypto::hash& h);
    size_t find_end_of_allowed_index(const std::vector<std::pair<crypto::hash, size_t> >& amount_outs);
    bool check_block_timestamp_main(const block& b);
    bool check_block_timestamp(std::vector<uint64_t> timestamps, const block& b);
    uint64_t get_adjusted_time();
    bool complete_timestamps_vector(uint64_t start_height, std::vector<uint64_t>& timestamps);
    bool update_next_comulative_size_limit();
  };


  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

  #define CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER    12

  template<class archive_t>
  void blockchain_storage::serialize(archive_t & ar, const unsigned int version)
  {
    if(version < 11)
      return;
    CRITICAL_REGION_LOCAL(m_blockchain_lock);
    ar & m_blocks;
    ar & m_blocks_index;
    ar & m_transactions;
    ar & m_spent_keys;
    ar & m_alternative_chains;
    ar & m_outputs;
    ar & m_invalid_blocks;
    ar & m_current_block_cumul_sz_limit;
    /*serialization bug workaround*/
    if(version > 11)
    {
      uint64_t total_check_count = m_blocks.size() + m_blocks_index.size() + m_transactions.size() + m_spent_keys.size() + m_alternative_chains.size() + m_outputs.size() + m_invalid_blocks.size() + m_current_block_cumul_sz_limit;
      if(archive_t::is_saving::value)
      {        
        ar & total_check_count;
      }else
      {
        uint64_t total_check_count_loaded = 0;
        ar & total_check_count_loaded;
        if(total_check_count != total_check_count_loaded)
        {
          LOG_ERROR("Blockchain storage data corruption detected. total_count loaded from file = " << total_check_count_loaded << ", expected = " << total_check_count);

          LOG_PRINT_L0("Blockchain storage:" << ENDL << 
            "m_blocks: " << m_blocks.size() << ENDL  << 
            "m_blocks_index: " << m_blocks_index.size() << ENDL  << 
            "m_transactions: " << m_transactions.size() << ENDL  << 
            "m_spent_keys: " << m_spent_keys.size() << ENDL  << 
            "m_alternative_chains: " << m_alternative_chains.size() << ENDL  << 
            "m_outputs: " << m_outputs.size() << ENDL  << 
            "m_invalid_blocks: " << m_invalid_blocks.size() << ENDL  << 
            "m_current_block_cumul_sz_limit: " << m_current_block_cumul_sz_limit);

          throw std::runtime_error("Blockchain data corruption");
        }
      }
    }


    LOG_PRINT_L2("Blockchain storage:" << ENDL << 
        "m_blocks: " << m_blocks.size() << ENDL  << 
        "m_blocks_index: " << m_blocks_index.size() << ENDL  << 
        "m_transactions: " << m_transactions.size() << ENDL  << 
        "m_spent_keys: " << m_spent_keys.size() << ENDL  << 
        "m_alternative_chains: " << m_alternative_chains.size() << ENDL  << 
        "m_outputs: " << m_outputs.size() << ENDL  << 
        "m_invalid_blocks: " << m_invalid_blocks.size() << ENDL  << 
        "m_current_block_cumul_sz_limit: " << m_current_block_cumul_sz_limit);
  }

  //------------------------------------------------------------------
  template<class visitor_t>
  bool blockchain_storage::scan_outputkeys_for_indexes(const txin_to_key& tx_in_to_key, visitor_t& vis, uint64_t* pmax_related_block_height)
  {
    CRITICAL_REGION_LOCAL(m_blockchain_lock);
    auto it = m_outputs.find(tx_in_to_key.amount);
    if(it == m_outputs.end() || !tx_in_to_key.key_offsets.size())
      return false;

    std::vector<uint64_t> absolute_offsets = relative_output_offsets_to_absolute(tx_in_to_key.key_offsets);


    std::vector<std::pair<crypto::hash, size_t> >& amount_outs_vec = it->second;
    size_t count = 0;
    BOOST_FOREACH(uint64_t i, absolute_offsets)
    {
      if(i >= amount_outs_vec.size() )
      {
        LOG_PRINT_L0("Wrong index in transaction inputs: " << i << ", expected maximum " << amount_outs_vec.size() - 1);
        return false;
      }
      transactions_container::iterator tx_it = m_transactions.find(amount_outs_vec[i].first);
      CHECK_AND_ASSERT_MES(tx_it != m_transactions.end(), false, "Wrong transaction id in output indexes: " <<string_tools::pod_to_hex(amount_outs_vec[i].first));
      CHECK_AND_ASSERT_MES(amount_outs_vec[i].second < tx_it->second.tx.vout.size(), false,
        "Wrong index in transaction outputs: " << amount_outs_vec[i].second << ", expected less then " << tx_it->second.tx.vout.size());
      if(!vis.handle_output(tx_it->second.tx, tx_it->second.tx.vout[amount_outs_vec[i].second]))
      {
        LOG_PRINT_L0("Failed to handle_output for output no = " << count << ", with absolute offset " << i);
        return false;
      }
      if(count++ == absolute_offsets.size()-1 && pmax_related_block_height)
      {
        if(*pmax_related_block_height < tx_it->second.m_keeper_block_height)
          *pmax_related_block_height = tx_it->second.m_keeper_block_height;
      }
    }

    return true;
  }
}



BOOST_CLASS_VERSION(cryptonote::blockchain_storage, CURRENT_BLOCKCHAIN_STORAGE_ARCHIVE_VER)
