// Copyright (c) 2014-2017, The Monero Project
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
#include <boost/filesystem.hpp>
#include <unordered_set>
#include <vector>

#include "tx_pool.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_config.h"
#include "blockchain.h"
#include "common/boost_serialization_helper.h"
#include "common/int-util.h"
#include "misc_language.h"
#include "warnings.h"
#include "common/perf_timer.h"
#include "crypto/hash.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "txpool"

DISABLE_VS_WARNINGS(4244 4345 4503) //'boost::foreach_detail_::or_' : decorated name length exceeded, name was truncated

namespace cryptonote
{
  namespace
  {
    //TODO: constants such as these should at least be in the header,
    //      but probably somewhere more accessible to the rest of the
    //      codebase.  As it stands, it is at best nontrivial to test
    //      whether or not changing these parameters (or adding new)
    //      will work correctly.
    size_t const TRANSACTION_SIZE_LIMIT_V1 = (((CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V1 * 125) / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
    size_t const TRANSACTION_SIZE_LIMIT_V2 = (((CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE_V2 * 125) / 100) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE);
    time_t const MIN_RELAY_TIME = (60 * 5); // only start re-relaying transactions after that many seconds
    time_t const MAX_RELAY_TIME = (60 * 60 * 4); // at most that many seconds between resends
    float const ACCEPT_THRESHOLD = 1.0f;

    // a kind of increasing backoff within min/max bounds
    time_t get_relay_delay(time_t now, time_t received)
    {
      time_t d = (now - received + MIN_RELAY_TIME) / MIN_RELAY_TIME * MIN_RELAY_TIME;
      if (d > MAX_RELAY_TIME)
        d = MAX_RELAY_TIME;
      return d;
    }

    uint64_t template_accept_threshold(uint64_t amount)
    {
      return amount * ACCEPT_THRESHOLD;
    }
  }
  //---------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------
  tx_memory_pool::tx_memory_pool(Blockchain& bchs): m_blockchain(bchs)
  {

  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(const transaction &tx, /*const crypto::hash& tx_prefix_hash,*/ const crypto::hash &id, size_t blob_size, tx_verification_context& tvc, bool kept_by_block, bool relayed, bool do_not_relay, uint8_t version)
  {
    PERF_TIMER(add_tx);
    if (tx.version == 0)
    {
      // v0 never accepted
      LOG_PRINT_L1("transaction version 0 is invalid");
      tvc.m_verifivation_failed = true;
      return false;
    }

    // we do not accept transactions that timed out before, unless they're
    // kept_by_block
    if (!kept_by_block && m_timed_out_transactions.find(id) != m_timed_out_transactions.end())
    {
      // not clear if we should set that, since verifivation (sic) did not fail before, since
      // the tx was accepted before timing out.
      tvc.m_verifivation_failed = true;
      return false;
    }

    if(!check_inputs_types_supported(tx))
    {
      tvc.m_verifivation_failed = true;
      tvc.m_invalid_input = true;
      return false;
    }

    // fee per kilobyte, size rounded up.
    uint64_t fee;

    if (tx.version == 1)
    {
      uint64_t inputs_amount = 0;
      if(!get_inputs_money_amount(tx, inputs_amount))
      {
        tvc.m_verifivation_failed = true;
        return false;
      }

      uint64_t outputs_amount = get_outs_money_amount(tx);
      if(outputs_amount >= inputs_amount)
      {
        LOG_PRINT_L1("transaction use more money then it has: use " << print_money(outputs_amount) << ", have " << print_money(inputs_amount));
        tvc.m_verifivation_failed = true;
        tvc.m_overspend = true;
        return false;
      }

      fee = inputs_amount - outputs_amount;
    }
    else
    {
      fee = tx.rct_signatures.txnFee;
    }

    if (!kept_by_block && !m_blockchain.check_fee(blob_size, fee))
    {
      tvc.m_verifivation_failed = true;
      tvc.m_fee_too_low = true;
      return false;
    }

    size_t tx_size_limit = (version < 2 ? TRANSACTION_SIZE_LIMIT_V1 : TRANSACTION_SIZE_LIMIT_V2);
    if (!kept_by_block && blob_size >= tx_size_limit)
    {
      LOG_PRINT_L1("transaction is too big: " << blob_size << " bytes, maximum size: " << tx_size_limit);
      tvc.m_verifivation_failed = true;
      tvc.m_too_big = true;
      return false;
    }

    // if the transaction came from a block popped from the chain,
    // don't check if we have its key images as spent.
    // TODO: Investigate why not?
    if(!kept_by_block)
    {
      if(have_tx_keyimges_as_spent(tx))
      {
        LOG_PRINT_L1("Transaction with id= "<< id << " used already spent key images");
        tvc.m_verifivation_failed = true;
        tvc.m_double_spend = true;
        return false;
      }
    }

    if (!m_blockchain.check_tx_outputs(tx, tvc))
    {
      LOG_PRINT_L1("Transaction with id= "<< id << " has at least one invalid output");
      tvc.m_verifivation_failed = true;
      tvc.m_invalid_output = true;
      return false;
    }

    time_t receive_time = time(nullptr);

    crypto::hash max_used_block_id = null_hash;
    uint64_t max_used_block_height = 0;
    tx_details txd;
    txd.tx = tx;
    bool ch_inp_res = m_blockchain.check_tx_inputs(txd.tx, max_used_block_height, max_used_block_id, tvc, kept_by_block);
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    if(!ch_inp_res)
    {
      // if the transaction was valid before (kept_by_block), then it
      // may become valid again, so ignore the failed inputs check.
      if(kept_by_block)
      {
        auto txd_p = m_transactions.insert(transactions_container::value_type(id, txd));
        CHECK_AND_ASSERT_MES(txd_p.second, false, "transaction already exists at inserting in memory pool");
        txd_p.first->second.blob_size = blob_size;
        txd_p.first->second.fee = fee;
        txd_p.first->second.max_used_block_id = null_hash;
        txd_p.first->second.max_used_block_height = 0;
        txd_p.first->second.last_failed_height = 0;
        txd_p.first->second.last_failed_id = null_hash;
        txd_p.first->second.kept_by_block = kept_by_block;
        txd_p.first->second.receive_time = receive_time;
        txd_p.first->second.last_relayed_time = time(NULL);
        txd_p.first->second.relayed = relayed;
        txd_p.first->second.do_not_relay = do_not_relay;
        tvc.m_verifivation_impossible = true;
        tvc.m_added_to_pool = true;
      }else
      {
        LOG_PRINT_L1("tx used wrong inputs, rejected");
        tvc.m_verifivation_failed = true;
        return false;
      }
    }else
    {
      //update transactions container
      auto txd_p = m_transactions.insert(transactions_container::value_type(id, txd));
      CHECK_AND_ASSERT_MES(txd_p.second, false, "internal error: transaction already exists at inserting in memorypool");
      txd_p.first->second.blob_size = blob_size;
      txd_p.first->second.kept_by_block = kept_by_block;
      txd_p.first->second.fee = fee;
      txd_p.first->second.max_used_block_id = max_used_block_id;
      txd_p.first->second.max_used_block_height = max_used_block_height;
      txd_p.first->second.last_failed_height = 0;
      txd_p.first->second.last_failed_id = null_hash;
      txd_p.first->second.receive_time = receive_time;
      txd_p.first->second.last_relayed_time = time(NULL);
      txd_p.first->second.relayed = relayed;
      txd_p.first->second.do_not_relay = do_not_relay;
      tvc.m_added_to_pool = true;

      if(txd_p.first->second.fee > 0 && !do_not_relay)
        tvc.m_should_be_relayed = true;
    }

    // assume failure during verification steps until success is certain
    tvc.m_verifivation_failed = true;

    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, txin, false);
      std::unordered_set<crypto::hash>& kei_image_set = m_spent_key_images[txin.k_image];
      CHECK_AND_ASSERT_MES(kept_by_block || kei_image_set.size() == 0, false, "internal error: kept_by_block=" << kept_by_block
                                          << ",  kei_image_set.size()=" << kei_image_set.size() << ENDL << "txin.k_image=" << txin.k_image << ENDL
                                          << "tx_id=" << id );
      auto ins_res = kei_image_set.insert(id);
      CHECK_AND_ASSERT_MES(ins_res.second, false, "internal error: try to insert duplicate iterator in key_image set");
    }

    tvc.m_verifivation_failed = false;

    m_txs_by_fee_and_receive_time.emplace(std::pair<double, std::time_t>(fee / (double)blob_size, receive_time), id);

    MINFO("Transaction " << id << " added to pool");
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(const transaction &tx, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay, uint8_t version)
  {
    crypto::hash h = null_hash;
    size_t blob_size = 0;
    get_transaction_hash(tx, h, blob_size);
    return add_tx(tx, h, blob_size, tvc, keeped_by_block, relayed, do_not_relay, version);
  }
  //---------------------------------------------------------------------------------
  //FIXME: Can return early before removal of all of the key images.
  //       At the least, need to make sure that a false return here
  //       is treated properly.  Should probably not return early, however.
  bool tx_memory_pool::remove_transaction_keyimages(const transaction& tx)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    // ND: Speedup
    // 1. Move transaction hash calcuation outside of loop. ._.
    crypto::hash actual_hash = get_transaction_hash(tx);
    for(const txin_v& vi: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(vi, const txin_to_key, txin, false);
      auto it = m_spent_key_images.find(txin.k_image);
      CHECK_AND_ASSERT_MES(it != m_spent_key_images.end(), false, "failed to find transaction input in key images. img=" << txin.k_image << ENDL
                                    << "transaction id = " << get_transaction_hash(tx));
      std::unordered_set<crypto::hash>& key_image_set =  it->second;
      CHECK_AND_ASSERT_MES(key_image_set.size(), false, "empty key_image set, img=" << txin.k_image << ENDL
        << "transaction id = " << actual_hash);

      auto it_in_set = key_image_set.find(actual_hash);
      CHECK_AND_ASSERT_MES(it_in_set != key_image_set.end(), false, "transaction id not found in key_image set, img=" << txin.k_image << ENDL
        << "transaction id = " << actual_hash);
      key_image_set.erase(it_in_set);
      if(!key_image_set.size())
      {
        //it is now empty hash container for this key_image
        m_spent_key_images.erase(it);
      }

    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::take_tx(const crypto::hash &id, transaction &tx, size_t& blob_size, uint64_t& fee, bool &relayed, bool &do_not_relay)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    auto it = m_transactions.find(id);
    if(it == m_transactions.end())
      return false;

    auto sorted_it = find_tx_in_sorted_container(id);

    if (sorted_it == m_txs_by_fee_and_receive_time.end())
      return false;

    tx = it->second.tx;
    blob_size = it->second.blob_size;
    fee = it->second.fee;
    relayed = it->second.relayed;
    do_not_relay = it->second.do_not_relay;
    remove_transaction_keyimages(it->second.tx);
    m_transactions.erase(it);
    m_txs_by_fee_and_receive_time.erase(sorted_it);
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::on_idle()
  {
    m_remove_stuck_tx_interval.do_call([this](){return remove_stuck_transactions();});
  }
  //---------------------------------------------------------------------------------
  sorted_tx_container::iterator tx_memory_pool::find_tx_in_sorted_container(const crypto::hash& id) const
  {
    return std::find_if( m_txs_by_fee_and_receive_time.begin(), m_txs_by_fee_and_receive_time.end()
                       , [&](const sorted_tx_container::value_type& a){
                         return a.second == id;
                       }
    );
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::remove_stuck_transactions()
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for(auto it = m_transactions.begin(); it!= m_transactions.end();)
    {
      uint64_t tx_age = time(nullptr) - it->second.receive_time;

      if((tx_age > CRYPTONOTE_MEMPOOL_TX_LIVETIME && !it->second.kept_by_block) ||
         (tx_age > CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME && it->second.kept_by_block) )
      {
        LOG_PRINT_L1("Tx " << it->first << " removed from tx pool due to outdated, age: " << tx_age );
        remove_transaction_keyimages(it->second.tx);
        auto sorted_it = find_tx_in_sorted_container(it->first);
        if (sorted_it == m_txs_by_fee_and_receive_time.end())
        {
          LOG_PRINT_L1("Removing tx " << it->first << " from tx pool, but it was not found in the sorted txs container!");
        }
        else
        {
          m_txs_by_fee_and_receive_time.erase(sorted_it);
        }
        m_timed_out_transactions.insert(it->first);
        auto pit = it++;
        m_transactions.erase(pit);
      }else
        ++it;
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::get_relayable_transactions(std::list<std::pair<crypto::hash, cryptonote::transaction>> &txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    const time_t now = time(NULL);
    for(auto it = m_transactions.begin(); it!= m_transactions.end();)
    {
      // 0 fee transactions are never relayed
      if(it->second.fee > 0 && !it->second.do_not_relay && now - it->second.last_relayed_time > get_relay_delay(now, it->second.receive_time))
      {
        // if the tx is older than half the max lifetime, we don't re-relay it, to avoid a problem
        // mentioned by smooth where nodes would flush txes at slightly different times, causing
        // flushed txes to be re-added when received from a node which was just about to flush it
        time_t max_age = it->second.kept_by_block ? CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME : CRYPTONOTE_MEMPOOL_TX_LIVETIME;
        if (now - it->second.receive_time <= max_age / 2)
        {
          txs.push_back(std::make_pair(it->first, it->second.tx));
        }
      }
      ++it;
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::set_relayed(const std::list<std::pair<crypto::hash, cryptonote::transaction>> &txs)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    const time_t now = time(NULL);
    for (auto it = txs.begin(); it != txs.end(); ++it)
    {
      auto i = m_transactions.find(it->first);
      if (i != m_transactions.end())
      {
        i->second.relayed = true;
        i->second.last_relayed_time = now;
      }
    }
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::get_transactions_count() const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    return m_transactions.size();
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::get_transactions(std::list<transaction>& txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for(const auto& tx_vt: m_transactions)
      txs.push_back(tx_vt.second.tx);
  }
  //------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::get_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for (const auto& tx_vt : m_transactions)
    {
      tx_info txi;
      const tx_details& txd = tx_vt.second;
      txi.id_hash = epee::string_tools::pod_to_hex(tx_vt.first);
      txi.tx_json = obj_to_json_str(*const_cast<transaction*>(&txd.tx));
      txi.blob_size = txd.blob_size;
      txi.fee = txd.fee;
      txi.kept_by_block = txd.kept_by_block;
      txi.max_used_block_height = txd.max_used_block_height;
      txi.max_used_block_id_hash = epee::string_tools::pod_to_hex(txd.max_used_block_id);
      txi.last_failed_height = txd.last_failed_height;
      txi.last_failed_id_hash = epee::string_tools::pod_to_hex(txd.last_failed_id);
      txi.receive_time = txd.receive_time;
      txi.relayed = txd.relayed;
      txi.last_relayed_time = txd.last_relayed_time;
      txi.do_not_relay = txd.do_not_relay;
      tx_infos.push_back(txi);
    }

    for (const key_images_container::value_type& kee : m_spent_key_images) {
      const crypto::key_image& k_image = kee.first;
      const std::unordered_set<crypto::hash>& kei_image_set = kee.second;
      spent_key_image_info ki;
      ki.id_hash = epee::string_tools::pod_to_hex(k_image);
      for (const crypto::hash& tx_id_hash : kei_image_set)
      {
        ki.txs_hashes.push_back(epee::string_tools::pod_to_hex(tx_id_hash));
      }
      key_image_infos.push_back(ki);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, transaction& tx) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    auto it = m_transactions.find(id);
    if(it == m_transactions.end())
      return false;
    tx = it->second.tx;
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::on_blockchain_inc(uint64_t new_block_height, const crypto::hash& top_block_id)
  {
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::on_blockchain_dec(uint64_t new_block_height, const crypto::hash& top_block_id)
  {
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx(const crypto::hash &id) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    if(m_transactions.count(id))
      return true;
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimges_as_spent(const transaction& tx) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for(const auto& in: tx.vin)
    {
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, tokey_in, true);//should never fail
      if(have_tx_keyimg_as_spent(tokey_in.k_image))
         return true;
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimg_as_spent(const crypto::key_image& key_im) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    return m_spent_key_images.end() != m_spent_key_images.find(key_im);
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::lock() const
  {
    m_transactions_lock.lock();
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::unlock() const
  {
    m_transactions_lock.unlock();
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::is_transaction_ready_to_go(tx_details& txd) const
  {
    //not the best implementation at this time, sorry :(
    //check is ring_signature already checked ?
    if(txd.max_used_block_id == null_hash)
    {//not checked, lets try to check

      if(txd.last_failed_id != null_hash && m_blockchain.get_current_blockchain_height() > txd.last_failed_height && txd.last_failed_id == m_blockchain.get_block_id_by_height(txd.last_failed_height))
        return false;//we already sure that this tx is broken for this height

      tx_verification_context tvc;
      if(!m_blockchain.check_tx_inputs(txd.tx, txd.max_used_block_height, txd.max_used_block_id, tvc))
      {
        txd.last_failed_height = m_blockchain.get_current_blockchain_height()-1;
        txd.last_failed_id = m_blockchain.get_block_id_by_height(txd.last_failed_height);
        return false;
      }
    }else
    {
      if(txd.max_used_block_height >= m_blockchain.get_current_blockchain_height())
        return false;
      if(true)
      {
        //if we already failed on this height and id, skip actual ring signature check
        if(txd.last_failed_id == m_blockchain.get_block_id_by_height(txd.last_failed_height))
          return false;
        //check ring signature again, it is possible (with very small chance) that this transaction become again valid
        tx_verification_context tvc;
        if(!m_blockchain.check_tx_inputs(txd.tx, txd.max_used_block_height, txd.max_used_block_id, tvc))
        {
          txd.last_failed_height = m_blockchain.get_current_blockchain_height()-1;
          txd.last_failed_id = m_blockchain.get_block_id_by_height(txd.last_failed_height);
          return false;
        }
      }
    }
    //if we here, transaction seems valid, but, anyway, check for key_images collisions with blockchain, just to be sure
    if(m_blockchain.have_tx_keyimges_as_spent(txd.tx))
      return false;

    //transaction is ok.
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_key_images(const std::unordered_set<crypto::key_image>& k_images, const transaction& tx)
  {
    for(size_t i = 0; i!= tx.vin.size(); i++)
    {
      CHECKED_GET_SPECIFIC_VARIANT(tx.vin[i], const txin_to_key, itk, false);
      if(k_images.count(itk.k_image))
        return true;
    }
    return false;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::append_key_images(std::unordered_set<crypto::key_image>& k_images, const transaction& tx)
  {
    for(size_t i = 0; i!= tx.vin.size(); i++)
    {
      CHECKED_GET_SPECIFIC_VARIANT(tx.vin[i], const txin_to_key, itk, false);
      auto i_res = k_images.insert(itk.k_image);
      CHECK_AND_ASSERT_MES(i_res.second, false, "internal error: key images pool cache - inserted duplicate image in set: " << itk.k_image);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  std::string tx_memory_pool::print_pool(bool short_format) const
  {
    std::stringstream ss;
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    for (const transactions_container::value_type& txe : m_transactions) {
      const tx_details& txd = txe.second;
      ss << "id: " << txe.first << std::endl;
      if (!short_format) {
        ss << obj_to_json_str(*const_cast<transaction*>(&txd.tx)) << std::endl;
      }
      ss << "blob_size: " << txd.blob_size << std::endl
        << "fee: " << print_money(txd.fee) << std::endl
        << "kept_by_block: " << (txd.kept_by_block ? 'T' : 'F') << std::endl
        << "max_used_block_height: " << txd.max_used_block_height << std::endl
        << "max_used_block_id: " << txd.max_used_block_id << std::endl
        << "last_failed_height: " << txd.last_failed_height << std::endl
        << "last_failed_id: " << txd.last_failed_id << std::endl;
    }

    return ss.str();
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::fill_block_template(block &bl, size_t median_size, uint64_t already_generated_coins, size_t &total_size, uint64_t &fee, uint8_t version)
  {
    // Warning: This function takes already_generated_
    // coins as an argument and appears to do nothing
    // with it.

    CRITICAL_REGION_LOCAL(m_transactions_lock);

    uint64_t best_coinbase = 0;
    total_size = 0;
    fee = 0;
    
    //baseline empty block
    get_block_reward(median_size, total_size, already_generated_coins, best_coinbase, version);


#if 1
    size_t max_total_size = (130 * median_size) / 100 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
#else
    size_t max_total_size = 2 * median_size - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
#endif
    std::unordered_set<crypto::key_image> k_images;

    LOG_PRINT_L2("Filling block template, median size " << median_size << ", " << m_txs_by_fee_and_receive_time.size() << " txes in the pool");
    auto sorted_it = m_txs_by_fee_and_receive_time.begin();
    while (sorted_it != m_txs_by_fee_and_receive_time.end())
    {
      auto tx_it = m_transactions.find(sorted_it->second);
      LOG_PRINT_L2("Considering " << tx_it->first << ", size " << tx_it->second.blob_size << ", current block size " << total_size << "/" << max_total_size << ", current coinbase " << print_money(best_coinbase));

      // Can not exceed maximum block size
      if (max_total_size < total_size + tx_it->second.blob_size)
      {
        LOG_PRINT_L2("  would exceed maximum block size");
        sorted_it++;
        continue;
      }

#if 1
      // If we've exceeded the penalty free size,
      // stop including more tx
      if (total_size > median_size)
      {
        LOG_PRINT_L2("  would exceed median block size");
        break;
      }
#else
      // If we're getting lower coinbase tx,
      // stop including more tx
      uint64_t block_reward;
      if(!get_block_reward(median_size, total_size + tx_it->second.blob_size, already_generated_coins, block_reward, version))
      {
        LOG_PRINT_L2("  would exceed maximum block size");
        sorted_it++;
        continue;
      }
      uint64_t coinbase = block_reward + fee + tx_it->second.fee;
      if (coinbase < template_accept_threshold(best_coinbase))
      {
        LOG_PRINT_L2("  would decrease coinbase to " << print_money(coinbase));
        sorted_it++;
        continue;
      }
#endif

      // Skip transactions that are not ready to be
      // included into the blockchain or that are
      // missing key images
      if (!is_transaction_ready_to_go(tx_it->second) || have_key_images(k_images, tx_it->second.tx))
      {
        LOG_PRINT_L2("  not ready to go, or key images already seen");
        sorted_it++;
        continue;
      }

      bl.tx_hashes.push_back(tx_it->first);
      total_size += tx_it->second.blob_size;
      fee += tx_it->second.fee;
#if 0
      best_coinbase = coinbase;
#endif
      append_key_images(k_images, tx_it->second.tx);
      sorted_it++;
      LOG_PRINT_L2("  added, new block size " << total_size << "/" << max_total_size << ", coinbase " << print_money(best_coinbase));
    }

    LOG_PRINT_L2("Block template filled with " << bl.tx_hashes.size() << " txes, size "
        << total_size << "/" << max_total_size << ", coinbase " << print_money(best_coinbase)
        << " (including " << print_money(fee) << " in fees)");
    return true;
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::validate(uint8_t version)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    size_t n_removed = 0;
    size_t tx_size_limit = (version < 2 ? TRANSACTION_SIZE_LIMIT_V1 : TRANSACTION_SIZE_LIMIT_V2);
    for (auto it = m_transactions.begin(); it != m_transactions.end(); ) {
      if (it->second.blob_size >= tx_size_limit) {
        LOG_PRINT_L1("Transaction " << get_transaction_hash(it->second.tx) << " is too big (" << it->second.blob_size << " bytes), removing it from pool");
        remove_transaction_keyimages(it->second.tx);
        auto sorted_it = find_tx_in_sorted_container(it->first);
        if (sorted_it == m_txs_by_fee_and_receive_time.end())
        {
          LOG_PRINT_L1("Removing tx " << it->first << " from tx pool, but it was not found in the sorted txs container!");
        }
        else
        {
          m_txs_by_fee_and_receive_time.erase(sorted_it);
        }
        auto pit = it++;
        m_transactions.erase(pit);
        ++n_removed;
        continue;
      }
      it++;
    }
    return n_removed;
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether only ever returning true is correct
  bool tx_memory_pool::init(const std::string& config_folder)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);

    m_config_folder = config_folder;
    if (m_config_folder.empty())
      return true;

    std::string state_file_path = config_folder + "/" + CRYPTONOTE_POOLDATA_FILENAME;
    boost::system::error_code ec;
    if(!boost::filesystem::exists(state_file_path, ec))
      return true;
    bool res = tools::unserialize_obj_from_file(*this, state_file_path);
    if(!res)
    {
      LOG_ERROR("Failed to load memory pool from file " << state_file_path);

      m_transactions.clear();
      m_txs_by_fee_and_receive_time.clear();
      m_spent_key_images.clear();
    }

    // no need to store queue of sorted transactions, as it's easy to generate.
    for (const auto& tx : m_transactions)
    {
      m_txs_by_fee_and_receive_time.emplace(std::pair<double, time_t>(tx.second.fee / (double)tx.second.blob_size, tx.second.receive_time), tx.first);
    }

    // Ignore deserialization error
    return true;
  }

  //---------------------------------------------------------------------------------
  //TODO: investigate whether only ever returning true is correct
  bool tx_memory_pool::deinit()
  {
    LOG_PRINT_L1("Received signal to deactivate memory pool store");

    if (m_config_folder.empty())
    {
      LOG_PRINT_L1("Memory pool store already empty");
      return true;
    }

    if (!tools::create_directories_if_necessary(m_config_folder))
    {
      LOG_ERROR("Failed to create memory pool data directory: " << m_config_folder);
      return false;
    }

    std::string state_file_path = m_config_folder + "/" + CRYPTONOTE_POOLDATA_FILENAME;
    bool res = tools::serialize_obj_to_file(*this, state_file_path);
    if(!res)
    {
      LOG_ERROR("Failed to serialize memory pool to file " << state_file_path);
      return false;
    }
    else
    {
      LOG_PRINT_L1("Memory pool store deactivated successfully");
      return true;
    }

  }
}
