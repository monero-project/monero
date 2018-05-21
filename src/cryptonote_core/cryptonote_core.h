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

#include <ctime>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"
#include "storages/portable_storage_template_helper.h"
#include "common/download.h"
#include "common/threadpool.h"
#include "common/command_line.h"
#include "tx_pool.h"
#include "blockchain.h"
#include "cryptonote_basic/miner.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_basic/cryptonote_stat_info.h"
#include "warnings.h"
#include "crypto/hash.h"

PUSH_WARNINGS
DISABLE_VS_WARNINGS(4355)

namespace cryptonote
{
   struct test_options {
     const std::pair<uint8_t, uint64_t> *hard_forks;
   };

  extern const command_line::arg_descriptor<std::string, false, true, 2> arg_data_dir;
  extern const command_line::arg_descriptor<bool, false> arg_testnet_on;
  extern const command_line::arg_descriptor<bool, false> arg_stagenet_on;
  extern const command_line::arg_descriptor<bool> arg_offline;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/

   /**
    * @brief handles core cryptonote functionality
    *
    * This class coordinates cryptonote functionality including, but not
    * limited to, communication among the Blockchain, the transaction pool,
    * any miners, and the network.
    */
   class core: public i_miner_handler
   {
   public:

      /**
       * @brief constructor
       *
       * sets member variables into a usable state
       *
       * @param pprotocol pre-constructed protocol object to store and use
       */
     core(i_cryptonote_protocol* pprotocol);

    /**
     * @copydoc Blockchain::handle_get_objects
     *
     * @note see Blockchain::handle_get_objects()
     * @param context connection context associated with the request
     */
     bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote_connection_context& context);

     /**
      * @brief calls various idle routines
      *
      * @note see miner::on_idle and tx_memory_pool::on_idle
      *
      * @return true
      */
     bool on_idle();

     /**
      * @brief handles an incoming transaction
      *
      * Parses an incoming transaction and, if nothing is obviously wrong,
      * passes it along to the transaction pool
      *
      * @param tx_blob the tx to handle
      * @param tvc metadata about the transaction's validity
      * @param keeped_by_block if the transaction has been in a block
      * @param relayed whether or not the transaction was relayed to us
      * @param do_not_relay whether to prevent the transaction from being relayed
      *
      * @return true if the transaction made it to the transaction pool, otherwise false
      */
     bool handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay);

     /**
      * @brief handles a list of incoming transactions
      *
      * Parses incoming transactions and, if nothing is obviously wrong,
      * passes them along to the transaction pool
      *
      * @param tx_blobs the txs to handle
      * @param tvc metadata about the transactions' validity
      * @param keeped_by_block if the transactions have been in a block
      * @param relayed whether or not the transactions were relayed to us
      * @param do_not_relay whether to prevent the transactions from being relayed
      *
      * @return true if the transactions made it to the transaction pool, otherwise false
      */
     bool handle_incoming_txs(const std::list<blobdata>& tx_blobs, std::vector<tx_verification_context>& tvc, bool keeped_by_block, bool relayed, bool do_not_relay);

     /**
      * @brief handles an incoming block
      *
      * periodic update to checkpoints is triggered here
      * Attempts to add the block to the Blockchain and, on success,
      * optionally updates the miner's block template.
      *
      * @param block_blob the block to be added
      * @param bvc return-by-reference metadata context about the block's validity
      * @param update_miner_blocktemplate whether or not to update the miner's block template
      *
      * @return false if loading new checkpoints fails, or the block is not
      * added, otherwise true
      */
     bool handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate = true);

     /**
      * @copydoc Blockchain::prepare_handle_incoming_blocks
      *
      * @note see Blockchain::prepare_handle_incoming_blocks
      */
     bool prepare_handle_incoming_blocks(const std::list<block_complete_entry>  &blocks);

     /**
      * @copydoc Blockchain::cleanup_handle_incoming_blocks
      *
      * @note see Blockchain::cleanup_handle_incoming_blocks
      */
     bool cleanup_handle_incoming_blocks(bool force_sync = false);
     	     	
     /**
      * @brief check the size of a block against the current maximum
      *
      * @param block_blob the block to check
      *
      * @return whether or not the block is too big
      */
     bool check_incoming_block_size(const blobdata& block_blob) const;

     /**
      * @brief get the cryptonote protocol instance
      *
      * @return the instance
      */
     i_cryptonote_protocol* get_protocol(){return m_pprotocol;}

     //-------------------- i_miner_handler -----------------------

     /**
      * @brief stores and relays a block found by a miner
      *
      * Updates the miner's target block, attempts to store the found
      * block in Blockchain, and -- on success -- relays that block to
      * the network.
      *
      * @param b the block found
      *
      * @return true if the block was added to the main chain, otherwise false
      */
     virtual bool handle_block_found( block& b);

     /**
      * @copydoc Blockchain::create_block_template
      *
      * @note see Blockchain::create_block_template
      */
     virtual bool get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce);

     /**
      * @brief called when a transaction is relayed
      */
     virtual void on_transaction_relayed(const cryptonote::blobdata& tx);


     /**
      * @brief gets the miner instance
      *
      * @return a reference to the miner instance
      */
     miner& get_miner(){return m_miner;}

     /**
      * @brief gets the miner instance (const)
      *
      * @return a const reference to the miner instance
      */
     const miner& get_miner()const{return m_miner;}

     /**
      * @brief adds command line options to the given options set
      *
      * As of now, there are no command line options specific to core,
      * so this function simply returns.
      *
      * @param desc return-by-reference the command line options set to add to
      */
     static void init_options(boost::program_options::options_description& desc);

     /**
      * @brief initializes the core as needed
      *
      * This function initializes the transaction pool, the Blockchain, and
      * a miner instance with parameters given on the command line (or defaults)
      *
      * @param vm command line parameters
      * @param config_subdir subdirectory for config storage
      * @param test_options configuration options for testing
      *
      * @return false if one of the init steps fails, otherwise true
      */
     bool init(const boost::program_options::variables_map& vm, const char *config_subdir = NULL, const test_options *test_options = NULL);

     /**
      * @copydoc Blockchain::reset_and_set_genesis_block
      *
      * @note see Blockchain::reset_and_set_genesis_block
      */
     bool set_genesis_block(const block& b);

     /**
      * @brief performs safe shutdown steps for core and core components
      *
      * Uninitializes the miner instance, transaction pool, and Blockchain
      *
      * @return true
      */
     bool deinit();

     /**
      * @brief sets to drop blocks downloaded (for testing)
      */
     void test_drop_download();

     /**
      * @brief sets to drop blocks downloaded below a certain height
      *
      * @param height height below which to drop blocks
      */
     void test_drop_download_height(uint64_t height);

     /**
      * @brief gets whether or not to drop blocks (for testing)
      *
      * @return whether or not to drop blocks
      */
     bool get_test_drop_download() const;

     /**
      * @brief gets whether or not to drop blocks
      *
      * If the current blockchain height <= our block drop threshold
      * and test drop blocks is set, return true
      *
      * @return see above
      */
     bool get_test_drop_download_height() const;

     /**
      * @copydoc Blockchain::get_current_blockchain_height
      *
      * @note see Blockchain::get_current_blockchain_height()
      */
     uint64_t get_current_blockchain_height() const;

     /**
      * @brief get the hash and height of the most recent block
      *
      * @param height return-by-reference height of the block
      * @param top_id return-by-reference hash of the block
      */
     void get_blockchain_top(uint64_t& height, crypto::hash& top_id) const;

     /**
      * @copydoc Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&, std::list<transaction>&) const
      *
      * @note see Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&, std::list<transaction>&) const
      */
     bool get_blocks(uint64_t start_offset, size_t count, std::list<std::pair<cryptonote::blobdata,block>>& blocks, std::list<cryptonote::blobdata>& txs) const;

     /**
      * @copydoc Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&) const
      *
      * @note see Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&) const
      */
     bool get_blocks(uint64_t start_offset, size_t count, std::list<std::pair<cryptonote::blobdata,block>>& blocks) const;

     /**
      * @copydoc Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&) const
      *
      * @note see Blockchain::get_blocks(uint64_t, size_t, std::list<std::pair<cryptonote::blobdata,block>>&) const
      */
     bool get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks) const;

     /**
      * @copydoc Blockchain::get_blocks(const t_ids_container&, t_blocks_container&, t_missed_container&) const
      *
      * @note see Blockchain::get_blocks(const t_ids_container&, t_blocks_container&, t_missed_container&) const
      */
     template<class t_ids_container, class t_blocks_container, class t_missed_container>
     bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs) const
     {
       return m_blockchain_storage.get_blocks(block_ids, blocks, missed_bs);
     }

     /**
      * @copydoc Blockchain::get_block_id_by_height
      *
      * @note see Blockchain::get_block_id_by_height
      */
     crypto::hash get_block_id_by_height(uint64_t height) const;

     /**
      * @copydoc Blockchain::get_transactions
      *
      * @note see Blockchain::get_transactions
      */
     bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<cryptonote::blobdata>& txs, std::list<crypto::hash>& missed_txs) const;

     /**
      * @copydoc Blockchain::get_transactions
      *
      * @note see Blockchain::get_transactions
      */
     bool get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs) const;

     /**
      * @copydoc Blockchain::get_block_by_hash
      *
      * @note see Blockchain::get_block_by_hash
      */
     bool get_block_by_hash(const crypto::hash &h, block &blk, bool *orphan = NULL) const;

     /**
      * @copydoc Blockchain::get_alternative_blocks
      *
      * @note see Blockchain::get_alternative_blocks(std::list<block>&) const
      */
     bool get_alternative_blocks(std::list<block>& blocks) const;

     /**
      * @copydoc Blockchain::get_alternative_blocks_count
      *
      * @note see Blockchain::get_alternative_blocks_count() const
      */
     size_t get_alternative_blocks_count() const;

     /**
      * @brief set the pointer to the cryptonote protocol object to use
      *
      * @param pprotocol the pointer to set ours as
      */
     void set_cryptonote_protocol(i_cryptonote_protocol* pprotocol);

     /**
      * @copydoc Blockchain::set_checkpoints
      *
      * @note see Blockchain::set_checkpoints()
      */
     void set_checkpoints(checkpoints&& chk_pts);

     /**
      * @brief set the file path to read from when loading checkpoints
      *
      * @param path the path to set ours as
      */
     void set_checkpoints_file_path(const std::string& path);

     /**
      * @brief set whether or not we enforce DNS checkpoints
      *
      * @param enforce_dns enforce DNS checkpoints or not
      */
     void set_enforce_dns_checkpoints(bool enforce_dns);

     /**
      * @brief set whether or not to enable or disable DNS checkpoints
      *
      * @param disble whether to disable DNS checkpoints
      */
     void disable_dns_checkpoints(bool disable = true) { m_disable_dns_checkpoints = disable; }

     /**
      * @copydoc tx_memory_pool::have_tx
      *
      * @note see tx_memory_pool::have_tx
      */
     bool pool_has_tx(const crypto::hash &txid) const;

     /**
      * @copydoc tx_memory_pool::get_transactions
      * @param include_unrelayed_txes include unrelayed txes in result
      *
      * @note see tx_memory_pool::get_transactions
      */
     bool get_pool_transactions(std::list<transaction>& txs, bool include_unrelayed_txes = true) const;

     /**
      * @copydoc tx_memory_pool::get_txpool_backlog
      *
      * @note see tx_memory_pool::get_txpool_backlog
      */
     bool get_txpool_backlog(std::vector<tx_backlog_entry>& backlog) const;
     
     /**
      * @copydoc tx_memory_pool::get_transactions
      * @param include_unrelayed_txes include unrelayed txes in result
      *
      * @note see tx_memory_pool::get_transactions
      */
     bool get_pool_transaction_hashes(std::vector<crypto::hash>& txs, bool include_unrelayed_txes = true) const;

     /**
      * @copydoc tx_memory_pool::get_transactions
      * @param include_unrelayed_txes include unrelayed txes in result
      *
      * @note see tx_memory_pool::get_transactions
      */
     bool get_pool_transaction_stats(struct txpool_stats& stats, bool include_unrelayed_txes = true) const;

     /**
      * @copydoc tx_memory_pool::get_transaction
      *
      * @note see tx_memory_pool::get_transaction
      */
     bool get_pool_transaction(const crypto::hash& id, cryptonote::blobdata& tx) const;

     /**
      * @copydoc tx_memory_pool::get_pool_transactions_and_spent_keys_info
      * @param include_unrelayed_txes include unrelayed txes in result
      *
      * @note see tx_memory_pool::get_pool_transactions_and_spent_keys_info
      */
     bool get_pool_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos, bool include_unrelayed_txes = true) const;

     /**
      * @copydoc tx_memory_pool::get_pool_for_rpc
      *
      * @note see tx_memory_pool::get_pool_for_rpc
      */
     bool get_pool_for_rpc(std::vector<cryptonote::rpc::tx_in_pool>& tx_infos, cryptonote::rpc::key_images_with_tx_hashes& key_image_infos) const;

     /**
      * @copydoc tx_memory_pool::get_transactions_count
      *
      * @note see tx_memory_pool::get_transactions_count
      */
     size_t get_pool_transactions_count() const;

     /**
      * @copydoc Blockchain::get_total_transactions
      *
      * @note see Blockchain::get_total_transactions
      */
     size_t get_blockchain_total_transactions() const;

     /**
      * @copydoc Blockchain::have_block
      *
      * @note see Blockchain::have_block
      */
     bool have_block(const crypto::hash& id) const;

     /**
      * @copydoc Blockchain::get_short_chain_history
      *
      * @note see Blockchain::get_short_chain_history
      */
     bool get_short_chain_history(std::list<crypto::hash>& ids) const;

     /**
      * @copydoc Blockchain::find_blockchain_supplement(const std::list<crypto::hash>&, NOTIFY_RESPONSE_CHAIN_ENTRY::request&) const
      *
      * @note see Blockchain::find_blockchain_supplement(const std::list<crypto::hash>&, NOTIFY_RESPONSE_CHAIN_ENTRY::request&) const
      */
     bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp) const;

     /**
      * @copydoc Blockchain::find_blockchain_supplement(const uint64_t, const std::list<crypto::hash>&, std::list<std::pair<cryptonote::blobdata, std::list<cryptonote::blobdata> > >&, uint64_t&, uint64_t&, size_t) const
      *
      * @note see Blockchain::find_blockchain_supplement(const uint64_t, const std::list<crypto::hash>&, std::list<std::pair<cryptonote::blobdata, std::list<transaction> > >&, uint64_t&, uint64_t&, size_t) const
      */
     bool find_blockchain_supplement(const uint64_t req_start_block, const std::list<crypto::hash>& qblock_ids, std::list<std::pair<cryptonote::blobdata, std::list<cryptonote::blobdata> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count) const;

     /**
      * @brief gets some stats about the daemon
      *
      * @param st_inf return-by-reference container for the stats requested
      *
      * @return true
      */
     bool get_stat_info(core_stat_info& st_inf) const;

     /**
      * @copydoc Blockchain::get_tx_outputs_gindexs
      *
      * @note see Blockchain::get_tx_outputs_gindexs
      */
     bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs) const;

     /**
      * @copydoc Blockchain::get_tail_id
      *
      * @note see Blockchain::get_tail_id
      */
     crypto::hash get_tail_id() const;

     /**
      * @copydoc Blockchain::get_block_cumulative_difficulty
      *
      * @note see Blockchain::get_block_cumulative_difficulty
      */
     difficulty_type get_block_cumulative_difficulty(uint64_t height) const;

     /**
      * @copydoc Blockchain::get_random_outs_for_amounts
      *
      * @note see Blockchain::get_random_outs_for_amounts
      */
     bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res) const;

     /**
      * @copydoc Blockchain::get_outs
      *
      * @note see Blockchain::get_outs
      */
     bool get_outs(const COMMAND_RPC_GET_OUTPUTS_BIN::request& req, COMMAND_RPC_GET_OUTPUTS_BIN::response& res) const;

     /**
      *
      * @copydoc Blockchain::get_random_rct_outs
      *
      * @note see Blockchain::get_random_rct_outs
      */
     bool get_random_rct_outs(const COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::request& req, COMMAND_RPC_GET_RANDOM_RCT_OUTPUTS::response& res) const;

     /**
      * @copydoc Blockchain::get_output_distribution
      *
      * @brief get per block distribution of outputs of a given amount
      */
     bool get_output_distribution(uint64_t amount, uint64_t from_height, uint64_t to_height, uint64_t &start_height, std::vector<uint64_t> &distribution, uint64_t &base) const;

     /**
      * @copydoc miner::pause
      *
      * @note see miner::pause
      */
     void pause_mine();

     /**
      * @copydoc miner::resume
      *
      * @note see miner::resume
      */
     void resume_mine();

     /**
      * @brief gets the Blockchain instance
      *
      * @return a reference to the Blockchain instance
      */
     Blockchain& get_blockchain_storage(){return m_blockchain_storage;}

     /**
      * @brief gets the Blockchain instance (const)
      *
      * @return a const reference to the Blockchain instance
      */
     const Blockchain& get_blockchain_storage()const{return m_blockchain_storage;}

     /**
      * @copydoc tx_memory_pool::print_pool
      *
      * @note see tx_memory_pool::print_pool
      */
     std::string print_pool(bool short_format) const;

     /**
      * @copydoc miner::on_synchronized
      *
      * @note see miner::on_synchronized
      */
     void on_synchronized();

     /**
      * @copydoc Blockchain::safesyncmode
      *
      * 2note see Blockchain::safesyncmode
      */
     void safesyncmode(const bool onoff);

     /**
      * @brief sets the target blockchain height
      *
      * @param target_blockchain_height the height to set
      */
     void set_target_blockchain_height(uint64_t target_blockchain_height);

     /**
      * @brief gets the target blockchain height
      *
      * @param target_blockchain_height the target height
      */
     uint64_t get_target_blockchain_height() const;

     /**
      * @brief returns the newest hardfork version known to the blockchain
      *
      * @return the version
      */
     uint8_t get_ideal_hard_fork_version() const;

     /**
      * @brief return the ideal hard fork version for a given block height
      *
      * @return what it says above
      */
     uint8_t get_ideal_hard_fork_version(uint64_t height) const;

     /**
      * @brief return the hard fork version for a given block height
      *
      * @return what it says above
      */
     uint8_t get_hard_fork_version(uint64_t height) const;

     /**
      * @brief gets start_time
      *
      */
     std::time_t get_start_time() const;

     /**
      * @brief tells the Blockchain to update its checkpoints
      *
      * This function will check if enough time has passed since the last
      * time checkpoints were updated and tell the Blockchain to update
      * its checkpoints if it is time.  If updating checkpoints fails,
      * the daemon is told to shut down.
      *
      * @note see Blockchain::update_checkpoints()
      */
     bool update_checkpoints();

     /**
      * @brief tells the daemon to wind down operations and stop running
      *
      * Currently this function raises SIGTERM, allowing the installed signal
      * handlers to do the actual stopping.
      */
     void graceful_exit();

     /**
      * @brief stops the daemon running
      *
      * @note see graceful_exit()
      */
     void stop();

     /**
      * @copydoc Blockchain::have_tx_keyimg_as_spent
      *
      * @note see Blockchain::have_tx_keyimg_as_spent
      */
     bool is_key_image_spent(const crypto::key_image& key_im) const;

     /**
      * @brief check if multiple key images are spent
      *
      * plural version of is_key_image_spent()
      *
      * @param key_im list of key images to check
      * @param spent return-by-reference result for each image checked
      *
      * @return true
      */
     bool are_key_images_spent(const std::vector<crypto::key_image>& key_im, std::vector<bool> &spent) const;

     /**
      * @brief check if multiple key images are spent in the transaction pool
      *
      * @param key_im list of key images to check
      * @param spent return-by-reference result for each image checked
      *
      * @return true
      */
     bool are_key_images_spent_in_pool(const std::vector<crypto::key_image>& key_im, std::vector<bool> &spent) const;

     /**
      * @brief get the number of blocks to sync in one go
      *
      * @return the number of blocks to sync in one go
      */
     size_t get_block_sync_size(uint64_t height) const;

     /**
      * @brief get the sum of coinbase tx amounts between blocks
      *
      * @return the number of blocks to sync in one go
      */
     std::pair<uint64_t, uint64_t> get_coinbase_tx_sum(const uint64_t start_offset, const size_t count);
     
     /**
      * @brief get the network type we're on
      *
      * @return which network are we on?
      */     
     network_type get_nettype() const { return m_nettype; };

     /**
      * @brief get whether fluffy blocks are enabled
      *
      * @return whether fluffy blocks are enabled
      */
     bool fluffy_blocks_enabled() const { return m_fluffy_blocks_enabled; }

     /**
      * @brief check a set of hashes against the precompiled hash set
      *
      * @return number of usable blocks
      */
     uint64_t prevalidate_block_hashes(uint64_t height, const std::list<crypto::hash> &hashes);

     /**
      * @brief get free disk space on the blockchain partition
      *
      * @return free space in bytes
      */
     uint64_t get_free_space() const;

     /**
      * @brief get whether the core is running offline
      *
      * @return whether the core is running offline
      */
     bool offline() const { return m_offline; }

   private:

     /**
      * @copydoc add_new_tx(transaction&, tx_verification_context&, bool)
      *
      * @param tx_hash the transaction's hash
      * @param tx_prefix_hash the transaction prefix' hash
      * @param blob_size the size of the transaction
      * @param relayed whether or not the transaction was relayed to us
      * @param do_not_relay whether to prevent the transaction from being relayed
      *
      */
     bool add_new_tx(transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay);

     /**
      * @brief add a new transaction to the transaction pool
      *
      * Adds a new transaction to the transaction pool.
      *
      * @param tx the transaction to add
      * @param tvc return-by-reference metadata about the transaction's verification process
      * @param keeped_by_block whether or not the transaction has been in a block
      * @param relayed whether or not the transaction was relayed to us
      * @param do_not_relay whether to prevent the transaction from being relayed
      *
      * @return true if the transaction is already in the transaction pool,
      * is already in a block on the Blockchain, or is successfully added
      * to the transaction pool
      */
     bool add_new_tx(transaction& tx, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay);

     /**
      * @copydoc Blockchain::add_new_block
      *
      * @note see Blockchain::add_new_block
      */
     bool add_new_block(const block& b, block_verification_context& bvc);

     /**
      * @brief load any core state stored on disk
      *
      * currently does nothing, but may have state to load in the future.
      *
      * @return true
      */
     bool load_state_data();

     /**
      * @copydoc parse_tx_from_blob(transaction&, crypto::hash&, crypto::hash&, const blobdata&) const
      *
      * @note see parse_tx_from_blob(transaction&, crypto::hash&, crypto::hash&, const blobdata&) const
      */
     bool parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob) const;

     /**
      * @brief check a transaction's syntax
      *
      * For now this does nothing, but it may check something about the tx
      * in the future.
      *
      * @param tx the transaction to check
      *
      * @return true
      */
     bool check_tx_syntax(const transaction& tx) const;

     /**
      * @brief validates some simple properties of a transaction
      *
      * Currently checks: tx has inputs,
      *                   tx inputs all of supported type(s),
      *                   tx outputs valid (type, key, amount),
      *                   input and output total amounts don't overflow,
      *                   output amount <= input amount,
      *                   tx not too large,
      *                   each input has a different key image.
      *
      * @param tx the transaction to check
      * @param keeped_by_block if the transaction has been in a block
      *
      * @return true if all the checks pass, otherwise false
      */
     bool check_tx_semantic(const transaction& tx, bool keeped_by_block) const;

     bool handle_incoming_tx_pre(const blobdata& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash, crypto::hash &tx_prefixt_hash, bool keeped_by_block, bool relayed, bool do_not_relay);
     bool handle_incoming_tx_post(const blobdata& tx_blob, tx_verification_context& tvc, cryptonote::transaction &tx, crypto::hash &tx_hash, crypto::hash &tx_prefixt_hash, bool keeped_by_block, bool relayed, bool do_not_relay);

     /**
      * @copydoc miner::on_block_chain_update
      *
      * @note see miner::on_block_chain_update
      *
      * @return true
      */
     bool update_miner_block_template();

     /**
      * @brief act on a set of command line options given
      *
      * @param vm the command line options
      *
      * @return true
      */
     bool handle_command_line(const boost::program_options::variables_map& vm);

     /**
      * @brief verify that each input key image in a transaction is unique
      *
      * @param tx the transaction to check
      *
      * @return false if any key image is repeated, otherwise true
      */
     bool check_tx_inputs_keyimages_diff(const transaction& tx) const;

     /**
      * @brief verify that each ring uses distinct members
      *
      * @param tx the transaction to check
      *
      * @return false if any ring uses duplicate members, true otherwise
      */
     bool check_tx_inputs_ring_members_diff(const transaction& tx) const;

     /**
      * @brief verify that each input key image in a transaction is in
      * the valid domain
      *
      * @param tx the transaction to check
      *
      * @return false if any key image is not in the valid domain, otherwise true
      */
     bool check_tx_inputs_keyimages_domain(const transaction& tx) const;

     /**
      * @brief checks HardFork status and prints messages about it
      *
      * Checks the status of HardFork and logs/prints if an update to
      * the daemon is necessary.
      *
      * @note see Blockchain::get_hard_fork_state and HardFork::State
      *
      * @return true
      */
     bool check_fork_time();

     /**
      * @brief attempts to relay any transactions in the mempool which need it
      *
      * @return true
      */
     bool relay_txpool_transactions();

     /**
      * @brief checks DNS versions
      *
      * @return true on success, false otherwise
      */
     bool check_updates();

     /**
      * @brief checks free disk space
      *
      * @return true on success, false otherwise
      */
     bool check_disk_space();

     bool m_test_drop_download = true; //!< whether or not to drop incoming blocks (for testing)

     uint64_t m_test_drop_download_height = 0; //!< height under which to drop incoming blocks, if doing so

     tx_memory_pool m_mempool; //!< transaction pool instance
     Blockchain m_blockchain_storage; //!< Blockchain instance

     i_cryptonote_protocol* m_pprotocol; //!< cryptonote protocol instance

     epee::critical_section m_incoming_tx_lock; //!< incoming transaction lock

     //m_miner and m_miner_addres are probably temporary here
     miner m_miner; //!< miner instance
     account_public_address m_miner_address; //!< address to mine to (for miner instance)

     std::string m_config_folder; //!< folder to look in for configs and other files

     cryptonote_protocol_stub m_protocol_stub; //!< cryptonote protocol stub instance

     epee::math_helper::once_a_time_seconds<60*60*12, false> m_store_blockchain_interval; //!< interval for manual storing of Blockchain, if enabled
     epee::math_helper::once_a_time_seconds<60*60*2, true> m_fork_moaner; //!< interval for checking HardFork status
     epee::math_helper::once_a_time_seconds<60*2, false> m_txpool_auto_relayer; //!< interval for checking re-relaying txpool transactions
     epee::math_helper::once_a_time_seconds<60*60*12, true> m_check_updates_interval; //!< interval for checking for new versions
     epee::math_helper::once_a_time_seconds<60*10, true> m_check_disk_space_interval; //!< interval for checking for disk space

     std::atomic<bool> m_starter_message_showed; //!< has the "daemon will sync now" message been shown?

     uint64_t m_target_blockchain_height; //!< blockchain height target

     network_type m_nettype; //!< which network are we on?

     std::string m_checkpoints_path; //!< path to json checkpoints file
     time_t m_last_dns_checkpoints_update; //!< time when dns checkpoints were last updated
     time_t m_last_json_checkpoints_update; //!< time when json checkpoints were last updated

     std::atomic_flag m_checkpoints_updating; //!< set if checkpoints are currently updating to avoid multiple threads attempting to update at once
     bool m_disable_dns_checkpoints;

     size_t block_sync_size;

     time_t start_time;

     std::unordered_set<crypto::hash> bad_semantics_txes[2];
     boost::mutex bad_semantics_txes_lock;

     tools::threadpool& m_threadpool;

     enum {
       UPDATES_DISABLED,
       UPDATES_NOTIFY,
       UPDATES_DOWNLOAD,
       UPDATES_UPDATE,
     } check_updates_level;

     tools::download_async_handle m_update_download;
     size_t m_last_update_length;
     boost::mutex m_update_mutex;

     bool m_fluffy_blocks_enabled;
     bool m_offline;
   };
}

POP_WARNINGS
