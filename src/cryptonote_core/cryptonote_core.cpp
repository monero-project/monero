// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "include_base_utils.h"
using namespace epee;

#include <boost/foreach.hpp>
#include <unordered_set>
#include "cryptonote_core.h"
#include "common/command_line.h"
#include "common/util.h"
#include "warnings.h"
#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "cryptonote_format_utils.h"
#include "misc_language.h"

DISABLE_VS_WARNINGS(4355)

namespace cryptonote
{

  //-----------------------------------------------------------------------------------------------
  core::core(i_cryptonote_protocol* pprotocol):
              m_mempool(m_blockchain_storage),
              m_blockchain_storage(m_mempool),
              m_miner(this),
              m_miner_address(boost::value_initialized<account_public_address>()), 
              m_starter_message_showed(false)
  {
    set_cryptonote_protocol(pprotocol);
  }
  void core::set_cryptonote_protocol(i_cryptonote_protocol* pprotocol)
  {
    if(pprotocol)
      m_pprotocol = pprotocol;
    else
      m_pprotocol = &m_protocol_stub;
  }
  //-----------------------------------------------------------------------------------
  void core::set_checkpoints(checkpoints&& chk_pts)
  {
    m_blockchain_storage.set_checkpoints(std::move(chk_pts));
  }
  //-----------------------------------------------------------------------------------
  void core::init_options(boost::program_options::options_description& /*desc*/)
  {
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_command_line(const boost::program_options::variables_map& vm)
  {
    m_config_folder = command_line::get_arg(vm, command_line::arg_data_dir);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  uint64_t core::get_current_blockchain_height()
  {
    return m_blockchain_storage.get_current_blockchain_height();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blockchain_top(uint64_t& height, crypto::hash& top_id)
  {
    top_id = m_blockchain_storage.get_tail_id(height);
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks, std::list<transaction>& txs)
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks, txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_blocks(uint64_t start_offset, size_t count, std::list<block>& blocks)
  {
    return m_blockchain_storage.get_blocks(start_offset, count, blocks);
  }  //-----------------------------------------------------------------------------------------------
  bool core::get_transactions(const std::vector<crypto::hash>& txs_ids, std::list<transaction>& txs, std::list<crypto::hash>& missed_txs)
  {
    return m_blockchain_storage.get_transactions(txs_ids, txs, missed_txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_transaction(const crypto::hash &h, transaction &tx)
  {
    std::vector<crypto::hash> ids;
    ids.push_back(h);
    std::list<transaction> ltx;
    std::list<crypto::hash> missing;
    if (m_blockchain_storage.get_transactions(ids, ltx, missing))
    {
      if (ltx.size() > 0)
      {
        tx = *ltx.begin();
        return true;
      }
    }

    return false;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_alternative_blocks(std::list<block>& blocks)
  {
    return m_blockchain_storage.get_alternative_blocks(blocks);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_alternative_blocks_count()
  {
    return m_blockchain_storage.get_alternative_blocks_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::init(const boost::program_options::variables_map& vm)
  {
    bool r = handle_command_line(vm);

    r = m_mempool.init(m_config_folder);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize memory pool");

    r = m_blockchain_storage.init(m_config_folder);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    r = m_miner.init(vm);
    CHECK_AND_ASSERT_MES(r, false, "Failed to initialize blockchain storage");

    return load_state_data();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::set_genesis_block(const block& b)
  {
    return m_blockchain_storage.reset_and_set_genesis_block(b);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::load_state_data()
  {
    // may be some code later
    return true;
  }
  //-----------------------------------------------------------------------------------------------
    bool core::deinit()
  {
    m_miner.stop();
    m_mempool.deinit();
    m_blockchain_storage.deinit();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block)
  {
    tvc = boost::value_initialized<tx_verification_context>();
    //want to process all transactions sequentially
    CRITICAL_REGION_LOCAL(m_incoming_tx_lock);

    if(tx_blob.size() > get_max_tx_size())
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, too big size " << tx_blob.size() << ", rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }

    crypto::hash tx_hash = null_hash;
    crypto::hash tx_prefixt_hash = null_hash;
    transaction tx;

    if(!parse_tx_from_blob(tx, tx_hash, tx_prefixt_hash, tx_blob))
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, Failed to parse, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }
    //std::cout << "!"<< tx.vin.size() << std::endl;

    if(!check_tx_syntax(tx))
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " syntax, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }

    if(!check_tx_semantic(tx, keeped_by_block))
    {
      LOG_PRINT_L0("WRONG TRANSACTION BLOB, Failed to check tx " << tx_hash << " semantic, rejected");
      tvc.m_verifivation_failed = true;
      return false;
    }

    bool r = add_new_tx(tx, tx_hash, tx_prefixt_hash, tx_blob.size(), tvc, keeped_by_block);
    if(tvc.m_verifivation_failed)
    {LOG_PRINT_RED_L0("Transaction verification failed: " << tx_hash);}
    else if(tvc.m_verifivation_impossible)
    {LOG_PRINT_RED_L0("Transaction verification impossible: " << tx_hash);}

    if(tvc.m_added_to_pool)
      LOG_PRINT_L1("tx added: " << tx_hash);
    return r;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_stat_info(core_stat_info& st_inf)
  {
    st_inf.mining_speed = m_miner.get_speed();
    st_inf.alternative_blocks = m_blockchain_storage.get_alternative_blocks_count();
    st_inf.blockchain_height = m_blockchain_storage.get_current_blockchain_height();
    st_inf.tx_pool_size = m_mempool.get_transactions_count();
    st_inf.top_block_id_str = epee::string_tools::pod_to_hex(m_blockchain_storage.get_tail_id());
    return true;
  }

  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_semantic(const transaction& tx, bool keeped_by_block)
  {
    if(!tx.vin.size())
    {
      LOG_PRINT_RED_L0("tx with empty inputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!check_inputs_types_supported(tx))
    {
      LOG_PRINT_RED_L0("unsupported input types for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!check_outs_valid(tx))
    {
      LOG_PRINT_RED_L0("tx with invalid outputs, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!check_money_overflow(tx))
    {
      LOG_PRINT_RED_L0("tx have money overflow, rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    uint64_t amount_in = 0;
    get_inputs_money_amount(tx, amount_in);
    uint64_t amount_out = get_outs_money_amount(tx);

    if(amount_in <= amount_out)
    {
      LOG_PRINT_RED_L0("tx with wrong amounts: ins " << amount_in << ", outs " << amount_out << ", rejected for tx id= " << get_transaction_hash(tx));
      return false;
    }

    if(!keeped_by_block && get_object_blobsize(tx) >= m_blockchain_storage.get_current_comulative_blocksize_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE)
    {
      LOG_PRINT_RED_L0("tx have to big size " << get_object_blobsize(tx) << ", expected not bigger than " << m_blockchain_storage.get_current_comulative_blocksize_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
      return false;
    }

    //check if tx use different key images
    if(!check_tx_inputs_keyimages_diff(tx))
    {
      LOG_PRINT_RED_L0("tx have to big size " << get_object_blobsize(tx) << ", expected not bigger than " << m_blockchain_storage.get_current_comulative_blocksize_limit() - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
      return false;
    }


    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::check_tx_inputs_keyimages_diff(const transaction& tx)
  {
    std::unordered_set<crypto::key_image> ki;
    BOOST_FOREACH(const auto& in, tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, false);
      if(!ki.insert(tokey_in.k_image).second)
        return false;
    }
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(const transaction& tx, tx_verification_context& tvc, bool keeped_by_block)
  {
    crypto::hash tx_hash = get_transaction_hash(tx);
    crypto::hash tx_prefix_hash = get_transaction_prefix_hash(tx);
    blobdata bl;
    t_serializable_object_to_blob(tx, bl);
    return add_new_tx(tx, tx_hash, tx_prefix_hash, bl.size(), tvc, keeped_by_block);
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_blockchain_total_transactions()
  {
    return m_blockchain_storage.get_total_transactions();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys)
  {
    return m_blockchain_storage.get_outs(amount, pkeys);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_tx(const transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block)
  {
    if(m_mempool.have_tx(tx_hash))
    {
      LOG_PRINT_L2("tx " << tx_hash << "already have transaction in tx_pool");
      return true;
    }

    if(m_blockchain_storage.have_tx(tx_hash))
    {
      LOG_PRINT_L2("tx " << tx_hash << " already have transaction in blockchain");
      return true;
    }

    return m_mempool.add_tx(tx, tx_hash, blob_size, tvc, keeped_by_block);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, const blobdata& ex_nonce)
  {
    return m_blockchain_storage.create_block_template(b, adr, diffic, height, ex_nonce);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY::request& resp)
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, resp);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<block, std::list<transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count)
  {
    return m_blockchain_storage.find_blockchain_supplement(qblock_ids, blocks, total_height, start_height, max_count);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain(uint64_t start_index, uint64_t end_index)
  {
    m_blockchain_storage.print_blockchain(start_index, end_index);
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_index()
  {
    m_blockchain_storage.print_blockchain_index();
  }
  //-----------------------------------------------------------------------------------------------
  void core::print_blockchain_outs(const std::string& file)
  {
    m_blockchain_storage.print_blockchain_outs(file);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response& res)
  {
    return m_blockchain_storage.get_random_outs_for_amounts(req, res);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs)
  {
    return m_blockchain_storage.get_tx_outputs_gindexs(tx_id, indexs);
  }
  //-----------------------------------------------------------------------------------------------
  void core::pause_mine()
  {
    m_miner.pause();
  }
  //-----------------------------------------------------------------------------------------------
  void core::resume_mine()
  {
    m_miner.resume();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_block_found(block& b)
  {
    block_verification_context bvc = boost::value_initialized<block_verification_context>();
    m_miner.pause();
    m_blockchain_storage.add_new_block(b, bvc);
    //anyway - update miner template
    update_miner_block_template();
    m_miner.resume();


    CHECK_AND_ASSERT_MES(!bvc.m_verifivation_failed, false, "mined block failed verification");
    if(bvc.m_added_to_main_chain)
    {
      cryptonote_connection_context exclude_context = boost::value_initialized<cryptonote_connection_context>();
      NOTIFY_NEW_BLOCK::request arg = AUTO_VAL_INIT(arg);
      arg.hop = 0;
      arg.current_blockchain_height = m_blockchain_storage.get_current_blockchain_height();
      std::list<crypto::hash> missed_txs;
      std::list<transaction> txs;
      m_blockchain_storage.get_transactions(b.tx_hashes, txs, missed_txs);
      if(missed_txs.size() &&  m_blockchain_storage.get_block_id_by_height(get_block_height(b)) != get_block_hash(b))
      {
        LOG_PRINT_L0("Block found but, seems that reorganize just happened after that, do not relay this block");
        return true;
      }
      CHECK_AND_ASSERT_MES(txs.size() == b.tx_hashes.size() && !missed_txs.size(), false, "cant find some transactions in found block:" << get_block_hash(b) << " txs.size()=" << txs.size()
        << ", b.tx_hashes.size()=" << b.tx_hashes.size() << ", missed_txs.size()" << missed_txs.size());

      block_to_blob(b, arg.b.block);
      //pack transactions
      BOOST_FOREACH(auto& tx,  txs)
        arg.b.txs.push_back(t_serializable_object_to_blob(tx));

      m_pprotocol->relay_block(arg, exclude_context);
    }
    return bvc.m_added_to_main_chain;
  }
  //-----------------------------------------------------------------------------------------------
  void core::on_synchronized()
  {
    m_miner.on_synchronized();
  }
  bool core::get_backward_blocks_sizes(uint64_t from_height, std::vector<size_t>& sizes, size_t count)
  {
    return m_blockchain_storage.get_backward_blocks_sizes(from_height, sizes, count);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::add_new_block(const block& b, block_verification_context& bvc)
  {
    return m_blockchain_storage.add_new_block(b, bvc);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_incoming_block(const blobdata& block_blob, block_verification_context& bvc, bool update_miner_blocktemplate)
  {
    bvc = boost::value_initialized<block_verification_context>();
    if(block_blob.size() > get_max_block_size())
    {
      LOG_PRINT_L0("WRONG BLOCK BLOB, too big size " << block_blob.size() << ", rejected");
      bvc.m_verifivation_failed = true;
      return false;
    }


    block b = AUTO_VAL_INIT(b);
    if(!parse_and_validate_block_from_blob(block_blob, b))
    {
      LOG_PRINT_L0("Failed to parse and validate new block");
      bvc.m_verifivation_failed = true;
      return false;
    }
    add_new_block(b, bvc);
    if(update_miner_blocktemplate && bvc.m_added_to_main_chain)
       update_miner_block_template();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_tail_id()
  {
    return m_blockchain_storage.get_tail_id();
  }
  //-----------------------------------------------------------------------------------------------
  size_t core::get_pool_transactions_count()
  {
    return m_mempool.get_transactions_count();
  }
  //-----------------------------------------------------------------------------------------------
  bool core::have_block(const crypto::hash& id)
  {
    return m_blockchain_storage.have_block(id);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::parse_tx_from_blob(transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob)
  {
    return parse_and_validate_tx_from_blob(blob, tx, tx_hash, tx_prefix_hash);
  }
  //-----------------------------------------------------------------------------------------------
    bool core::check_tx_syntax(const transaction& tx)
  {
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_pool_transactions(std::list<transaction>& txs)
  {
    return m_mempool.get_transactions(txs);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_short_chain_history(std::list<crypto::hash>& ids)
  {
    return m_blockchain_storage.get_short_chain_history(ids);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS::request& arg, NOTIFY_RESPONSE_GET_OBJECTS::request& rsp, cryptonote_connection_context& context)
  {
    return m_blockchain_storage.handle_get_objects(arg, rsp);
  }
  //-----------------------------------------------------------------------------------------------
  crypto::hash core::get_block_id_by_height(uint64_t height)
  {
    return m_blockchain_storage.get_block_id_by_height(height);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::get_block_by_hash(const crypto::hash &h, block &blk) {
    return m_blockchain_storage.get_block_by_hash(h, blk);
  }
  //-----------------------------------------------------------------------------------------------
  void core::get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid) {
    m_blockchain_storage.get_all_known_block_ids(main, alt, invalid);
  }
  //-----------------------------------------------------------------------------------------------
  std::string core::print_pool(bool short_format)
  {
    return m_mempool.print_pool(short_format);
  }
  //-----------------------------------------------------------------------------------------------
  bool core::update_miner_block_template()
  {
    m_miner.on_block_chain_update();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
  bool core::on_idle()
  {
    if(!m_starter_message_showed)
    {
      LOG_PRINT_L0(ENDL << "**********************************************************************" << ENDL 
        << "The daemon will start synchronizing with the network. It may take up to several hours." << ENDL 
        << ENDL
        << "You can set the level of process detailization by using command \"set_log <level>\", where <level> is either 0 (no details), 1 (current block height synchronized), or 2 (all details)." << ENDL
        << ENDL
        << "Use \"help\" command to see the list of available commands." << ENDL
        << ENDL
        << "Note: in case you need to interrupt the process, use \"exit\" command. Otherwise, the current progress won't be saved." << ENDL 
        << "**********************************************************************");
      m_starter_message_showed = true;
    }

    m_store_blockchain_interval.do_call(boost::bind(&blockchain_storage::store_blockchain, &m_blockchain_storage));
    m_miner.on_idle();
    return true;
  }
  //-----------------------------------------------------------------------------------------------
}
