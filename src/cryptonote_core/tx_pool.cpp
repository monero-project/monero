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
#include <boost/filesystem.hpp>
#include <unordered_set>
#include <vector>

#include "tx_pool.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/cryptonote_boost_serialization.h"
#include "cryptonote_config.h"
#include "blockchain.h"
#include "blockchain_db/blockchain_db.h"
#include "common/boost_serialization_helper.h"
#include "common/int-util.h"
#include "misc_language.h"
#include "warnings.h"
#include "common/perf_timer.h"
#include "crypto/hash.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "txpool"

DISABLE_VS_WARNINGS(4244 4345 4503) //'boost::foreach_detail_::or_' : decorated name length exceeded, name was truncated

using namespace crypto;

namespace cryptonote
{
  namespace
  {
    //TODO: constants such as these should at least be in the header,
    //      but probably somewhere more accessible to the rest of the
    //      codebase.  As it stands, it is at best nontrivial to test
    //      whether or not changing these parameters (or adding new)
    //      will work correctly.
    time_t const MIN_RELAY_TIME = (60 * 5); // only start re-relaying transactions after that many seconds
    time_t const MAX_RELAY_TIME = (60 * 60 * 4); // at most that many seconds between resends
    float const ACCEPT_THRESHOLD = 1.0f;

    // a kind of increasing backoff within min/max bounds
    uint64_t get_relay_delay(time_t now, time_t received)
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

    uint64_t get_transaction_size_limit(uint8_t version)
    {
      return get_min_block_size(version) - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    }

    // This class is meant to create a batch when none currently exists.
    // If a batch exists, it can't be from another thread, since we can
    // only be called with the txpool lock taken, and it is held during
    // the whole prepare/handle/cleanup incoming block sequence.
    class LockedTXN {
    public:
      LockedTXN(Blockchain &b): m_blockchain(b), m_batch(false) {
        m_batch = m_blockchain.get_db().batch_start();
      }
      ~LockedTXN() { try { if (m_batch) { m_blockchain.get_db().batch_stop(); } } catch (const std::exception &e) { MWARNING("LockedTXN dtor filtering exception: " << e.what()); } }
    private:
      Blockchain &m_blockchain;
      bool m_batch;
    };
  }
  //---------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------
  tx_memory_pool::tx_memory_pool(Blockchain& bchs): m_blockchain(bchs), m_txpool_max_size(DEFAULT_TXPOOL_MAX_SIZE), m_txpool_size(0)
  {

  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(transaction &tx, /*const crypto::hash& tx_prefix_hash,*/ const crypto::hash &id, size_t blob_size, tx_verification_context& tvc, bool kept_by_block, bool relayed, bool do_not_relay, uint8_t version)
  {
    // this should already be called with that lock, but let's make it explicit for clarity
    CRITICAL_REGION_LOCAL(m_transactions_lock);

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
      if(outputs_amount > inputs_amount)
      {
        LOG_PRINT_L1("transaction use more money than it has: use " << print_money(outputs_amount) << ", have " << print_money(inputs_amount));
        tvc.m_verifivation_failed = true;
        tvc.m_overspend = true;
        return false;
      }
      else if(outputs_amount == inputs_amount)
      {
        LOG_PRINT_L1("transaction fee is zero: outputs_amount == inputs_amount, rejecting.");
        tvc.m_verifivation_failed = true;
        tvc.m_fee_too_low = true;
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

    size_t tx_size_limit = get_transaction_size_limit(version);
    if (!kept_by_block && blob_size > tx_size_limit)
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
        mark_double_spend(tx);
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

    // assume failure during verification steps until success is certain
    tvc.m_verifivation_failed = true;

    time_t receive_time = time(nullptr);

    crypto::hash max_used_block_id = null_hash;
    uint64_t max_used_block_height = 0;
    cryptonote::txpool_tx_meta_t meta;
    bool ch_inp_res = m_blockchain.check_tx_inputs(tx, max_used_block_height, max_used_block_id, tvc, kept_by_block);
    if(!ch_inp_res)
    {
      // if the transaction was valid before (kept_by_block), then it
      // may become valid again, so ignore the failed inputs check.
      if(kept_by_block)
      {
        meta.blob_size = blob_size;
        meta.fee = fee;
        meta.max_used_block_id = null_hash;
        meta.max_used_block_height = 0;
        meta.last_failed_height = 0;
        meta.last_failed_id = null_hash;
        meta.kept_by_block = kept_by_block;
        meta.receive_time = receive_time;
        meta.last_relayed_time = time(NULL);
        meta.relayed = relayed;
        meta.do_not_relay = do_not_relay;
        meta.double_spend_seen = have_tx_keyimges_as_spent(tx);
        meta.bf_padding = 0;
        memset(meta.padding, 0, sizeof(meta.padding));
        try
        {
          CRITICAL_REGION_LOCAL1(m_blockchain);
          LockedTXN lock(m_blockchain);
          m_blockchain.add_txpool_tx(tx, meta);
          if (!insert_key_images(tx, kept_by_block))
            return false;
          m_txs_by_fee_and_receive_time.emplace(std::pair<double, std::time_t>(fee / (double)blob_size, receive_time), id);
        }
        catch (const std::exception &e)
        {
          MERROR("transaction already exists at inserting in memory pool: " << e.what());
          return false;
        }
        tvc.m_verifivation_impossible = true;
        tvc.m_added_to_pool = true;
      }else
      {
        LOG_PRINT_L1("tx used wrong inputs, rejected");
        tvc.m_verifivation_failed = true;
        tvc.m_invalid_input = true;
        return false;
      }
    }else
    {
      //update transactions container
      meta.blob_size = blob_size;
      meta.kept_by_block = kept_by_block;
      meta.fee = fee;
      meta.max_used_block_id = max_used_block_id;
      meta.max_used_block_height = max_used_block_height;
      meta.last_failed_height = 0;
      meta.last_failed_id = null_hash;
      meta.receive_time = receive_time;
      meta.last_relayed_time = time(NULL);
      meta.relayed = relayed;
      meta.do_not_relay = do_not_relay;
      meta.double_spend_seen = false;
      meta.bf_padding = 0;
      memset(meta.padding, 0, sizeof(meta.padding));

      try
      {
        CRITICAL_REGION_LOCAL1(m_blockchain);
        LockedTXN lock(m_blockchain);
        m_blockchain.remove_txpool_tx(get_transaction_hash(tx));
        m_blockchain.add_txpool_tx(tx, meta);
        if (!insert_key_images(tx, kept_by_block))
          return false;
        m_txs_by_fee_and_receive_time.emplace(std::pair<double, std::time_t>(fee / (double)blob_size, receive_time), id);
      }
      catch (const std::exception &e)
      {
        MERROR("internal error: transaction already exists at inserting in memory pool: " << e.what());
        return false;
      }
      tvc.m_added_to_pool = true;

      if(meta.fee > 0 && !do_not_relay)
        tvc.m_should_be_relayed = true;
    }

    tvc.m_verifivation_failed = false;
    m_txpool_size += blob_size;

    MINFO("Transaction added to pool: txid " << id << " bytes: " << blob_size << " fee/byte: " << (fee / (double)blob_size));

    prune(m_txpool_max_size);

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::add_tx(transaction &tx, tx_verification_context& tvc, bool keeped_by_block, bool relayed, bool do_not_relay, uint8_t version)
  {
    crypto::hash h = null_hash;
    size_t blob_size = 0;
    if (!get_transaction_hash(tx, h, blob_size) || blob_size == 0)
      return false;
    return add_tx(tx, h, blob_size, tvc, keeped_by_block, relayed, do_not_relay, version);
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::get_txpool_size() const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    return m_txpool_size;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::set_txpool_max_size(size_t bytes)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    m_txpool_max_size = bytes;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::prune(size_t bytes)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    if (bytes == 0)
      bytes = m_txpool_max_size;
    CRITICAL_REGION_LOCAL1(m_blockchain);
    LockedTXN lock(m_blockchain);

    // this will never remove the first one, but we don't care
    auto it = --m_txs_by_fee_and_receive_time.end();
    while (it != m_txs_by_fee_and_receive_time.begin())
    {
      if (m_txpool_size <= bytes)
        break;
      try
      {
        const crypto::hash &txid = it->second;
        txpool_tx_meta_t meta;
        if (!m_blockchain.get_txpool_tx_meta(txid, meta))
        {
          MERROR("Failed to find tx in txpool");
          return;
        }
        // don't prune the kept_by_block ones, they're likely added because we're adding a block with those
        if (meta.kept_by_block)
        {
          --it;
          continue;
        }
        cryptonote::blobdata txblob = m_blockchain.get_txpool_tx_blob(txid);
        cryptonote::transaction tx;
        if (!parse_and_validate_tx_from_blob(txblob, tx))
        {
          MERROR("Failed to parse tx from txpool");
          return;
        }
        // remove first, in case this throws, so key images aren't removed
        MINFO("Pruning tx " << txid << " from txpool: size: " << it->first.second << ", fee/byte: " << it->first.first);
        m_blockchain.remove_txpool_tx(txid);
        m_txpool_size -= txblob.size();
        remove_transaction_keyimages(tx);
        MINFO("Pruned tx " << txid << " from txpool: size: " << it->first.second << ", fee/byte: " << it->first.first);
        m_txs_by_fee_and_receive_time.erase(it--);
      }
      catch (const std::exception &e)
      {
        MERROR("Error while pruning txpool: " << e.what());
        return;
      }
    }
    if (m_txpool_size > bytes)
      MINFO("Pool size after pruning is larger than limit: " << m_txpool_size << "/" << bytes);
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::insert_key_images(const transaction &tx, bool kept_by_block)
  {
    for(const auto& in: tx.vin)
    {
      const crypto::hash id = get_transaction_hash(tx);
      CHECKED_GET_SPECIFIC_VARIANT(in, const txin_to_key, txin, false);
      std::unordered_set<crypto::hash>& kei_image_set = m_spent_key_images[txin.k_image];
      CHECK_AND_ASSERT_MES(kept_by_block || kei_image_set.size() == 0, false, "internal error: kept_by_block=" << kept_by_block
                                          << ",  kei_image_set.size()=" << kei_image_set.size() << ENDL << "txin.k_image=" << txin.k_image << ENDL
                                          << "tx_id=" << id );
      auto ins_res = kei_image_set.insert(id);
      CHECK_AND_ASSERT_MES(ins_res.second, false, "internal error: try to insert duplicate iterator in key_image set");
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  //FIXME: Can return early before removal of all of the key images.
  //       At the least, need to make sure that a false return here
  //       is treated properly.  Should probably not return early, however.
  bool tx_memory_pool::remove_transaction_keyimages(const transaction& tx)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
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
  bool tx_memory_pool::take_tx(const crypto::hash &id, transaction &tx, size_t& blob_size, uint64_t& fee, bool &relayed, bool &do_not_relay, bool &double_spend_seen)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);

    auto sorted_it = find_tx_in_sorted_container(id);
    if (sorted_it == m_txs_by_fee_and_receive_time.end())
      return false;

    try
    {
      LockedTXN lock(m_blockchain);
      txpool_tx_meta_t meta;
      if (!m_blockchain.get_txpool_tx_meta(id, meta))
      {
        MERROR("Failed to find tx in txpool");
        return false;
      }
      cryptonote::blobdata txblob = m_blockchain.get_txpool_tx_blob(id);
      if (!parse_and_validate_tx_from_blob(txblob, tx))
      {
        MERROR("Failed to parse tx from txpool");
        return false;
      }
      blob_size = meta.blob_size;
      fee = meta.fee;
      relayed = meta.relayed;
      do_not_relay = meta.do_not_relay;
      double_spend_seen = meta.double_spend_seen;

      // remove first, in case this throws, so key images aren't removed
      m_blockchain.remove_txpool_tx(id);
      m_txpool_size -= blob_size;
      remove_transaction_keyimages(tx);
    }
    catch (const std::exception &e)
    {
      MERROR("Failed to remove tx from txpool: " << e.what());
      return false;
    }

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
    CRITICAL_REGION_LOCAL1(m_blockchain);
    std::unordered_set<crypto::hash> remove;
    m_blockchain.for_all_txpool_txes([this, &remove](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata*) {
      uint64_t tx_age = time(nullptr) - meta.receive_time;

      if((tx_age > CRYPTONOTE_MEMPOOL_TX_LIVETIME && !meta.kept_by_block) ||
         (tx_age > CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME && meta.kept_by_block) )
      {
        LOG_PRINT_L1("Tx " << txid << " removed from tx pool due to outdated, age: " << tx_age );
        auto sorted_it = find_tx_in_sorted_container(txid);
        if (sorted_it == m_txs_by_fee_and_receive_time.end())
        {
          LOG_PRINT_L1("Removing tx " << txid << " from tx pool, but it was not found in the sorted txs container!");
        }
        else
        {
          m_txs_by_fee_and_receive_time.erase(sorted_it);
        }
        m_timed_out_transactions.insert(txid);
        remove.insert(txid);
      }
      return true;
    }, false);

    if (!remove.empty())
    {
      LockedTXN lock(m_blockchain);
      for (const crypto::hash &txid: remove)
      {
        try
        {
          cryptonote::blobdata bd = m_blockchain.get_txpool_tx_blob(txid);
          cryptonote::transaction tx;
          if (!parse_and_validate_tx_from_blob(bd, tx))
          {
            MERROR("Failed to parse tx from txpool");
            // continue
          }
          else
          {
            // remove first, so we only remove key images if the tx removal succeeds
            m_blockchain.remove_txpool_tx(txid);
            m_txpool_size -= bd.size();
            remove_transaction_keyimages(tx);
          }
        }
        catch (const std::exception &e)
        {
          MWARNING("Failed to remove stuck transaction: " << txid);
          // ignore error
        }
      }
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::get_relayable_transactions(std::list<std::pair<crypto::hash, cryptonote::blobdata>> &txs) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    const uint64_t now = time(NULL);
    m_blockchain.for_all_txpool_txes([this, now, &txs](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *){
      // 0 fee transactions are never relayed
      if(meta.fee > 0 && !meta.do_not_relay && now - meta.last_relayed_time > get_relay_delay(now, meta.receive_time))
      {
        // if the tx is older than half the max lifetime, we don't re-relay it, to avoid a problem
        // mentioned by smooth where nodes would flush txes at slightly different times, causing
        // flushed txes to be re-added when received from a node which was just about to flush it
        uint64_t max_age = meta.kept_by_block ? CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME : CRYPTONOTE_MEMPOOL_TX_LIVETIME;
        if (now - meta.receive_time <= max_age / 2)
        {
          try
          {
            cryptonote::blobdata bd = m_blockchain.get_txpool_tx_blob(txid);
            txs.push_back(std::make_pair(txid, bd));
          }
          catch (const std::exception &e)
          {
            MERROR("Failed to get transaction blob from db");
            // ignore error
          }
        }
      }
      return true;
    }, false);
    return true;
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::set_relayed(const std::list<std::pair<crypto::hash, cryptonote::blobdata>> &txs)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    const time_t now = time(NULL);
    LockedTXN lock(m_blockchain);
    for (auto it = txs.begin(); it != txs.end(); ++it)
    {
      try
      {
        txpool_tx_meta_t meta;
        if (m_blockchain.get_txpool_tx_meta(it->first, meta))
        {
          meta.relayed = true;
          meta.last_relayed_time = now;
          m_blockchain.update_txpool_tx(it->first, meta);
        }
      }
      catch (const std::exception &e)
      {
        MERROR("Failed to update txpool transaction metadata: " << e.what());
        // continue
      }
    }
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::get_transactions_count(bool include_unrelayed_txes) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    return m_blockchain.get_txpool_tx_count(include_unrelayed_txes);
  }
  //---------------------------------------------------------------------------------
  void tx_memory_pool::get_transactions(std::list<transaction>& txs, bool include_unrelayed_txes) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    m_blockchain.for_all_txpool_txes([&txs](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      transaction tx;
      if (!parse_and_validate_tx_from_blob(*bd, tx))
      {
        MERROR("Failed to parse tx from txpool");
        // continue
        return true;
      }
      txs.push_back(tx);
      return true;
    }, true, include_unrelayed_txes);
  }
  //------------------------------------------------------------------
  void tx_memory_pool::get_transaction_hashes(std::vector<crypto::hash>& txs, bool include_unrelayed_txes) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    m_blockchain.for_all_txpool_txes([&txs](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      txs.push_back(txid);
      return true;
    }, false, include_unrelayed_txes);
  }
  //------------------------------------------------------------------
  void tx_memory_pool::get_transaction_backlog(std::vector<tx_backlog_entry>& backlog, bool include_unrelayed_txes) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    const uint64_t now = time(NULL);
    m_blockchain.for_all_txpool_txes([&backlog, now](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      backlog.push_back({meta.blob_size, meta.fee, meta.receive_time - now});
      return true;
    }, false, include_unrelayed_txes);
  }
  //------------------------------------------------------------------
  void tx_memory_pool::get_transaction_stats(struct txpool_stats& stats, bool include_unrelayed_txes) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    const uint64_t now = time(NULL);
    std::map<uint64_t, txpool_histo> agebytes;
    stats.txs_total = m_blockchain.get_txpool_tx_count(include_unrelayed_txes);
    std::vector<uint32_t> sizes;
    sizes.reserve(stats.txs_total);
    m_blockchain.for_all_txpool_txes([&stats, &sizes, now, &agebytes](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      sizes.push_back(meta.blob_size);
      stats.bytes_total += meta.blob_size;
      if (!stats.bytes_min || meta.blob_size < stats.bytes_min)
        stats.bytes_min = meta.blob_size;
      if (meta.blob_size > stats.bytes_max)
        stats.bytes_max = meta.blob_size;
      if (!meta.relayed)
        stats.num_not_relayed++;
      stats.fee_total += meta.fee;
      if (!stats.oldest || meta.receive_time < stats.oldest)
        stats.oldest = meta.receive_time;
      if (meta.receive_time < now - 600)
        stats.num_10m++;
      if (meta.last_failed_height)
        stats.num_failing++;
      uint64_t age = now - meta.receive_time + (now == meta.receive_time);
      agebytes[age].txs++;
      agebytes[age].bytes += meta.blob_size;
      if (meta.double_spend_seen)
        ++stats.num_double_spends;
      return true;
      }, false, include_unrelayed_txes);
    stats.bytes_med = epee::misc_utils::median(sizes);
    if (stats.txs_total > 1)
    {
      /* looking for 98th percentile */
      size_t end = stats.txs_total * 0.02;
      uint64_t delta, factor;
      std::map<uint64_t, txpool_histo>::iterator it, i2;
      if (end)
      {
        /* If enough txs, spread the first 98% of results across
         * the first 9 bins, drop final 2% in last bin.
         */
        it=agebytes.end();
        for (size_t n=0; n <= end; n++, it--);
        stats.histo_98pc = it->first;
        factor = 9;
        delta = it->first;
        stats.histo.resize(10);
      } else
      {
        /* If not enough txs, don't reserve the last slot;
         * spread evenly across all 10 bins.
         */
        stats.histo_98pc = 0;
        it = agebytes.end();
        factor = stats.txs_total > 9 ? 10 : stats.txs_total;
        delta = now - stats.oldest;
        stats.histo.resize(factor);
      }
      if (!delta)
        delta = 1;
      for (i2 = agebytes.begin(); i2 != it; i2++)
      {
        size_t i = (i2->first * factor - 1) / delta;
        stats.histo[i].txs += i2->second.txs;
        stats.histo[i].bytes += i2->second.bytes;
      }
      for (; i2 != agebytes.end(); i2++)
      {
        stats.histo[factor].txs += i2->second.txs;
        stats.histo[factor].bytes += i2->second.bytes;
      }
    }
  }
  //------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::get_transactions_and_spent_keys_info(std::vector<tx_info>& tx_infos, std::vector<spent_key_image_info>& key_image_infos, bool include_sensitive_data) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    m_blockchain.for_all_txpool_txes([&tx_infos, key_image_infos, include_sensitive_data](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      tx_info txi;
      txi.id_hash = epee::string_tools::pod_to_hex(txid);
      txi.tx_blob = *bd;
      transaction tx;
      if (!parse_and_validate_tx_from_blob(*bd, tx))
      {
        MERROR("Failed to parse tx from txpool");
        // continue
        return true;
      }
      txi.tx_json = obj_to_json_str(tx);
      txi.blob_size = meta.blob_size;
      txi.fee = meta.fee;
      txi.kept_by_block = meta.kept_by_block;
      txi.max_used_block_height = meta.max_used_block_height;
      txi.max_used_block_id_hash = epee::string_tools::pod_to_hex(meta.max_used_block_id);
      txi.last_failed_height = meta.last_failed_height;
      txi.last_failed_id_hash = epee::string_tools::pod_to_hex(meta.last_failed_id);
      // In restricted mode we do not include this data:
      txi.receive_time = include_sensitive_data ? meta.receive_time : 0;
      txi.relayed = meta.relayed;
      // In restricted mode we do not include this data:
      txi.last_relayed_time = include_sensitive_data ? meta.last_relayed_time : 0;
      txi.do_not_relay = meta.do_not_relay;
      txi.double_spend_seen = meta.double_spend_seen;
      tx_infos.push_back(txi);
      return true;
    }, true, include_sensitive_data);

    txpool_tx_meta_t meta;
    for (const key_images_container::value_type& kee : m_spent_key_images) {
      const crypto::key_image& k_image = kee.first;
      const std::unordered_set<crypto::hash>& kei_image_set = kee.second;
      spent_key_image_info ki;
      ki.id_hash = epee::string_tools::pod_to_hex(k_image);
      for (const crypto::hash& tx_id_hash : kei_image_set)
      {
        if (!include_sensitive_data)
        {
          try
          {
            if (!m_blockchain.get_txpool_tx_meta(tx_id_hash, meta))
            {
              MERROR("Failed to get tx meta from txpool");
              return false;
            }
            if (!meta.relayed)
              // Do not include that transaction if in restricted mode and it's not relayed
              continue;
          }
          catch (const std::exception &e)
          {
            MERROR("Failed to get tx meta from txpool: " << e.what());
            return false;
          }
        }
        ki.txs_hashes.push_back(epee::string_tools::pod_to_hex(tx_id_hash));
      }
      // Only return key images for which we have at least one tx that we can show for them
      if (!ki.txs_hashes.empty())
        key_image_infos.push_back(ki);
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_pool_for_rpc(std::vector<cryptonote::rpc::tx_in_pool>& tx_infos, cryptonote::rpc::key_images_with_tx_hashes& key_image_infos) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    m_blockchain.for_all_txpool_txes([&tx_infos, key_image_infos](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd){
      cryptonote::rpc::tx_in_pool txi;
      txi.tx_hash = txid;
      transaction tx;
      if (!parse_and_validate_tx_from_blob(*bd, tx))
      {
        MERROR("Failed to parse tx from txpool");
        // continue
        return true;
      }
      txi.tx = tx;
      txi.blob_size = meta.blob_size;
      txi.fee = meta.fee;
      txi.kept_by_block = meta.kept_by_block;
      txi.max_used_block_height = meta.max_used_block_height;
      txi.max_used_block_hash = meta.max_used_block_id;
      txi.last_failed_block_height = meta.last_failed_height;
      txi.last_failed_block_hash = meta.last_failed_id;
      txi.receive_time = meta.receive_time;
      txi.relayed = meta.relayed;
      txi.last_relayed_time = meta.last_relayed_time;
      txi.do_not_relay = meta.do_not_relay;
      txi.double_spend_seen = meta.double_spend_seen;
      tx_infos.push_back(txi);
      return true;
    }, true, false);

    for (const key_images_container::value_type& kee : m_spent_key_images) {
      std::vector<crypto::hash> tx_hashes;
      const std::unordered_set<crypto::hash>& kei_image_set = kee.second;
      for (const crypto::hash& tx_id_hash : kei_image_set)
      {
        tx_hashes.push_back(tx_id_hash);
      }

      const crypto::key_image& k_image = kee.first;
      key_image_infos[k_image] = tx_hashes;
    }
    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::check_for_key_images(const std::vector<crypto::key_image>& key_images, std::vector<bool> spent) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);

    spent.clear();

    for (const auto& image : key_images)
    {
      spent.push_back(m_spent_key_images.find(image) == m_spent_key_images.end() ? false : true);
    }

    return true;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::get_transaction(const crypto::hash& id, cryptonote::blobdata& txblob) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    try
    {
      return m_blockchain.get_txpool_tx_blob(id, txblob);
    }
    catch (const std::exception &e)
    {
      return false;
    }
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
    CRITICAL_REGION_LOCAL1(m_blockchain);
    return m_blockchain.get_db().txpool_has_tx(id);
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::have_tx_keyimges_as_spent(const transaction& tx) const
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
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
  bool tx_memory_pool::is_transaction_ready_to_go(txpool_tx_meta_t& txd, transaction &tx) const
  {
    //not the best implementation at this time, sorry :(
    //check is ring_signature already checked ?
    if(txd.max_used_block_id == null_hash)
    {//not checked, lets try to check

      if(txd.last_failed_id != null_hash && m_blockchain.get_current_blockchain_height() > txd.last_failed_height && txd.last_failed_id == m_blockchain.get_block_id_by_height(txd.last_failed_height))
        return false;//we already sure that this tx is broken for this height

      tx_verification_context tvc;
      if(!m_blockchain.check_tx_inputs(tx, txd.max_used_block_height, txd.max_used_block_id, tvc))
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
        if(!m_blockchain.check_tx_inputs(tx, txd.max_used_block_height, txd.max_used_block_id, tvc))
        {
          txd.last_failed_height = m_blockchain.get_current_blockchain_height()-1;
          txd.last_failed_id = m_blockchain.get_block_id_by_height(txd.last_failed_height);
          return false;
        }
      }
    }
    //if we here, transaction seems valid, but, anyway, check for key_images collisions with blockchain, just to be sure
    if(m_blockchain.have_tx_keyimges_as_spent(tx))
    {
      txd.double_spend_seen = true;
      return false;
    }

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
  void tx_memory_pool::mark_double_spend(const transaction &tx)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    LockedTXN lock(m_blockchain);
    for(size_t i = 0; i!= tx.vin.size(); i++)
    {
      CHECKED_GET_SPECIFIC_VARIANT(tx.vin[i], const txin_to_key, itk, void());
      const key_images_container::const_iterator it = m_spent_key_images.find(itk.k_image);
      if (it != m_spent_key_images.end())
      {
        for (const crypto::hash &txid: it->second)
        {
          txpool_tx_meta_t meta;
          if (!m_blockchain.get_txpool_tx_meta(txid, meta))
          {
            MERROR("Failed to find tx meta in txpool");
            // continue, not fatal
            continue;
          }
          if (!meta.double_spend_seen)
          {
            MDEBUG("Marking " << txid << " as double spending " << itk.k_image);
            meta.double_spend_seen = true;
            try
            {
              m_blockchain.update_txpool_tx(txid, meta);
            }
            catch (const std::exception &e)
            {
              MERROR("Failed to update tx meta: " << e.what());
              // continue, not fatal
            }
          }
        }
      }
    }
  }
  //---------------------------------------------------------------------------------
  std::string tx_memory_pool::print_pool(bool short_format) const
  {
    std::stringstream ss;
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    m_blockchain.for_all_txpool_txes([&ss, short_format](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *txblob) {
      ss << "id: " << txid << std::endl;
      if (!short_format) {
        cryptonote::transaction tx;
        if (!parse_and_validate_tx_from_blob(*txblob, tx))
        {
          MERROR("Failed to parse tx from txpool");
          return true; // continue
        }
        ss << obj_to_json_str(tx) << std::endl;
      }
      ss << "blob_size: " << meta.blob_size << std::endl
        << "fee: " << print_money(meta.fee) << std::endl
        << "kept_by_block: " << (meta.kept_by_block ? 'T' : 'F') << std::endl
        << "double_spend_seen: " << (meta.double_spend_seen ? 'T' : 'F') << std::endl
        << "max_used_block_height: " << meta.max_used_block_height << std::endl
        << "max_used_block_id: " << meta.max_used_block_id << std::endl
        << "last_failed_height: " << meta.last_failed_height << std::endl
        << "last_failed_id: " << meta.last_failed_id << std::endl;
      return true;
    }, !short_format);

    return ss.str();
  }
  //---------------------------------------------------------------------------------
  //TODO: investigate whether boolean return is appropriate
  bool tx_memory_pool::fill_block_template(block &bl, size_t median_size, uint64_t already_generated_coins, size_t &total_size, uint64_t &fee, uint64_t &expected_reward, uint8_t version)
  {
    // Warning: This function takes already_generated_
    // coins as an argument and appears to do nothing
    // with it.

    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);

    uint64_t best_coinbase = 0, coinbase = 0;
    total_size = 0;
    fee = 0;
    
    //baseline empty block
    get_block_reward(median_size, total_size, already_generated_coins, best_coinbase, version);


    size_t max_total_size_pre_v5 = (130 * median_size) / 100 - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    size_t max_total_size_v5 = 2 * median_size - CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE;
    size_t max_total_size = version >= 5 ? max_total_size_v5 : max_total_size_pre_v5;
    std::unordered_set<crypto::key_image> k_images;

    LOG_PRINT_L2("Filling block template, median size " << median_size << ", " << m_txs_by_fee_and_receive_time.size() << " txes in the pool");

    LockedTXN lock(m_blockchain);

    auto sorted_it = m_txs_by_fee_and_receive_time.begin();
    while (sorted_it != m_txs_by_fee_and_receive_time.end())
    {
      txpool_tx_meta_t meta;
      if (!m_blockchain.get_txpool_tx_meta(sorted_it->second, meta))
      {
        MERROR("  failed to find tx meta");
        continue;
      }
      LOG_PRINT_L2("Considering " << sorted_it->second << ", size " << meta.blob_size << ", current block size " << total_size << "/" << max_total_size << ", current coinbase " << print_money(best_coinbase));

      // Can not exceed maximum block size
      if (max_total_size < total_size + meta.blob_size)
      {
        LOG_PRINT_L2("  would exceed maximum block size");
        sorted_it++;
        continue;
      }

      // start using the optimal filling algorithm from v5
      if (version >= 5)
      {
        // If we're getting lower coinbase tx,
        // stop including more tx
        uint64_t block_reward;
        if(!get_block_reward(median_size, total_size + meta.blob_size, already_generated_coins, block_reward, version))
        {
          LOG_PRINT_L2("  would exceed maximum block size");
          sorted_it++;
          continue;
        }
        coinbase = block_reward + fee + meta.fee;
        if (coinbase < template_accept_threshold(best_coinbase))
        {
          LOG_PRINT_L2("  would decrease coinbase to " << print_money(coinbase));
          sorted_it++;
          continue;
        }
      }
      else
      {
        // If we've exceeded the penalty free size,
        // stop including more tx
        if (total_size > median_size)
        {
          LOG_PRINT_L2("  would exceed median block size");
          break;
        }
      }

      cryptonote::blobdata txblob = m_blockchain.get_txpool_tx_blob(sorted_it->second);
      cryptonote::transaction tx;
      if (!parse_and_validate_tx_from_blob(txblob, tx))
      {
        MERROR("Failed to parse tx from txpool");
        sorted_it++;
        continue;
      }

      // Skip transactions that are not ready to be
      // included into the blockchain or that are
      // missing key images
      const cryptonote::txpool_tx_meta_t original_meta = meta;
      bool ready = is_transaction_ready_to_go(meta, tx);
      if (memcmp(&original_meta, &meta, sizeof(meta)))
      {
        try
	{
	  m_blockchain.update_txpool_tx(sorted_it->second, meta);
	}
        catch (const std::exception &e)
	{
	  MERROR("Failed to update tx meta: " << e.what());
	  // continue, not fatal
	}
      }
      if (!ready)
      {
        LOG_PRINT_L2("  not ready to go");
        sorted_it++;
        continue;
      }
      if (have_key_images(k_images, tx))
      {
        LOG_PRINT_L2("  key images already seen");
        sorted_it++;
        continue;
      }

      bl.tx_hashes.push_back(sorted_it->second);
      total_size += meta.blob_size;
      fee += meta.fee;
      best_coinbase = coinbase;
      append_key_images(k_images, tx);
      sorted_it++;
      LOG_PRINT_L2("  added, new block size " << total_size << "/" << max_total_size << ", coinbase " << print_money(best_coinbase));
    }

    expected_reward = best_coinbase;
    LOG_PRINT_L2("Block template filled with " << bl.tx_hashes.size() << " txes, size "
        << total_size << "/" << max_total_size << ", coinbase " << print_money(best_coinbase)
        << " (including " << print_money(fee) << " in fees)");
    return true;
  }
  //---------------------------------------------------------------------------------
  size_t tx_memory_pool::validate(uint8_t version)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);
    size_t tx_size_limit = get_transaction_size_limit(version);
    std::unordered_set<crypto::hash> remove;

    m_txpool_size = 0;
    m_blockchain.for_all_txpool_txes([this, &remove, tx_size_limit](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata*) {
      m_txpool_size += meta.blob_size;
      if (meta.blob_size > tx_size_limit) {
        LOG_PRINT_L1("Transaction " << txid << " is too big (" << meta.blob_size << " bytes), removing it from pool");
        remove.insert(txid);
      }
      else if (m_blockchain.have_tx(txid)) {
        LOG_PRINT_L1("Transaction " << txid << " is in the blockchain, removing it from pool");
        remove.insert(txid);
      }
      return true;
    }, false);

    size_t n_removed = 0;
    if (!remove.empty())
    {
      LockedTXN lock(m_blockchain);
      for (const crypto::hash &txid: remove)
      {
        try
        {
          cryptonote::blobdata txblob = m_blockchain.get_txpool_tx_blob(txid);
          cryptonote::transaction tx;
          if (!parse_and_validate_tx_from_blob(txblob, tx))
          {
            MERROR("Failed to parse tx from txpool");
            continue;
          }
          // remove tx from db first
          m_blockchain.remove_txpool_tx(txid);
          m_txpool_size -= txblob.size();
          remove_transaction_keyimages(tx);
          auto sorted_it = find_tx_in_sorted_container(txid);
          if (sorted_it == m_txs_by_fee_and_receive_time.end())
          {
            LOG_PRINT_L1("Removing tx " << txid << " from tx pool, but it was not found in the sorted txs container!");
          }
          else
          {
            m_txs_by_fee_and_receive_time.erase(sorted_it);
          }
          ++n_removed;
        }
        catch (const std::exception &e)
        {
          MERROR("Failed to remove invalid tx from pool");
          // continue
        }
      }
    }
    return n_removed;
  }
  //---------------------------------------------------------------------------------
  bool tx_memory_pool::init(size_t max_txpool_size)
  {
    CRITICAL_REGION_LOCAL(m_transactions_lock);
    CRITICAL_REGION_LOCAL1(m_blockchain);

    m_txpool_max_size = max_txpool_size ? max_txpool_size : DEFAULT_TXPOOL_MAX_SIZE;
    m_txs_by_fee_and_receive_time.clear();
    m_spent_key_images.clear();
    m_txpool_size = 0;
    std::vector<crypto::hash> remove;

    // first add the not kept by block, then the kept by block,
    // to avoid rejection due to key image collision
    for (int pass = 0; pass < 2; ++pass)
    {
      const bool kept = pass == 1;
      bool r = m_blockchain.for_all_txpool_txes([this, &remove, kept](const crypto::hash &txid, const txpool_tx_meta_t &meta, const cryptonote::blobdata *bd) {
        if (!!kept != !!meta.kept_by_block)
          return true;
        cryptonote::transaction tx;
        if (!parse_and_validate_tx_from_blob(*bd, tx))
        {
          MWARNING("Failed to parse tx from txpool, removing");
          remove.push_back(txid);
        }
        if (!insert_key_images(tx, meta.kept_by_block))
        {
          MFATAL("Failed to insert key images from txpool tx");
          return false;
        }
        m_txs_by_fee_and_receive_time.emplace(std::pair<double, time_t>(meta.fee / (double)meta.blob_size, meta.receive_time), txid);
        m_txpool_size += meta.blob_size;
        return true;
      }, true);
      if (!r)
        return false;
    }
    if (!remove.empty())
    {
      LockedTXN lock(m_blockchain);
      for (const auto &txid: remove)
      {
        try
        {
          m_blockchain.remove_txpool_tx(txid);
        }
        catch (const std::exception &e)
        {
          MWARNING("Failed to remove corrupt transaction: " << txid);
          // ignore error
        }
      }
    }
    return true;
  }

  //---------------------------------------------------------------------------------
  bool tx_memory_pool::deinit()
  {
    return true;
  }
}
