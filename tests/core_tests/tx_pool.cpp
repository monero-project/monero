// Copyright (c) 2019-2024, The Monero Project
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

#include "tx_pool.h"

#include <boost/chrono/chrono.hpp>
#include <boost/thread/thread_only.hpp>
#include <limits>
#include "string_tools.h"

#define INIT_MEMPOOL_TEST()                                   \
  uint64_t send_amount = 1000;                                \
  uint64_t ts_start = 1338224400;                             \
  GENERATE_ACCOUNT(miner_account);                            \
  GENERATE_ACCOUNT(bob_account);                              \
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start); \
  REWIND_BLOCKS(events, blk_0r, blk_0, miner_account);        \


txpool_base::txpool_base()
  : test_chain_unit_base()
  , m_broadcasted_tx_count(0)
  , m_all_tx_count(0)
{
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_broadcasted_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_all_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, check_txpool_spent_keys);
}

bool txpool_base::increase_broadcasted_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_broadcasted_tx_count;
  return true;
}

bool txpool_base::increase_all_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_all_tx_count;
  return true;
}

bool txpool_base::check_txpool_spent_keys(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
  std::vector<cryptonote::tx_info> infos{};
  std::vector<cryptonote::spent_key_image_info> key_images{};
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images) || infos.size() != m_broadcasted_tx_count || key_images.size() != m_broadcasted_tx_count)
  {
    MERROR("Failed broadcasted spent keys retrieval - Expected Broadcasted Count: " << m_broadcasted_tx_count << " Actual Info Count: " << infos.size() << " Actual Key Image Count: " << key_images.size());
    return false;
  }

  infos.clear();
  key_images.clear();
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images, false) || infos.size() != m_broadcasted_tx_count || key_images.size() != m_broadcasted_tx_count)
  {
    MERROR("Failed broadcasted spent keys retrieval - Expected Broadcasted Count: " << m_broadcasted_tx_count << " Actual Info Count: " << infos.size() << " Actual Key Image Count: " << key_images.size());
    return false;
  }

  infos.clear();
  key_images.clear();
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images, true) || infos.size() != m_all_tx_count || key_images.size() != m_all_tx_count)
  {
    MERROR("Failed all spent keys retrieval - Expected All Count: " << m_all_tx_count << " Actual Info Count: " << infos.size() << " Actual Key Image Count: " << key_images.size());
    return false;
  }

  return true;
}

bool txpool_spend_key_public::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);
  DO_CALLBACK(events, "increase_broadcasted_tx_count");
  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  return true;
}

bool txpool_spend_key_all::generate(std::vector<test_event_entry>& events)
{
  INIT_MEMPOOL_TEST();
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_do_not_relay);

  DO_CALLBACK(events, "check_txpool_spent_keys");
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);
  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  return true;
}

txpool_double_spend_base::txpool_double_spend_base()
  : txpool_base()
  , m_broadcasted_hashes()
  , m_no_relay_hashes()
  , m_all_hashes()
  , m_no_new_index(0)
  , m_failed_index(0)
  , m_new_timestamp_index(0)
  , m_last_tx(crypto::hash{})
{
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, mark_no_new);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, mark_failed);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, mark_timestamp_change);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, timestamp_change_pause);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, check_unchanged);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, check_new_broadcasted);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, check_new_hidden);
  REGISTER_CALLBACK_METHOD(txpool_double_spend_base, check_new_no_relay);
}

bool txpool_double_spend_base::mark_no_new(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_no_new_index = ev_index + 1;
  return true;
}

bool txpool_double_spend_base::mark_failed(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_failed_index = ev_index + 1;
  return true;
}

bool txpool_double_spend_base::mark_timestamp_change(cryptonote::core& /*c*/, size_t ev_index, const std::vector<test_event_entry>& /*events*/)
{
  m_new_timestamp_index = ev_index + 1;
  return true;
}

bool txpool_double_spend_base::timestamp_change_pause(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  boost::this_thread::sleep_for(boost::chrono::seconds{1} + boost::chrono::milliseconds{100});
  return true;
}

bool txpool_double_spend_base::check_changed(cryptonote::core& c, const size_t ev_index, relay_test condition)
{
  const std::size_t new_broadcasted_hash_count = m_broadcasted_hashes.size() + unsigned(condition == relay_test::broadcasted);
  const std::size_t new_all_hash_count = m_all_hashes.size() + unsigned(condition == relay_test::hidden) + unsigned(condition == relay_test::no_relay);

  std::vector<crypto::hash> hashes{};
  if (!c.get_pool_transaction_hashes(hashes))
  {
    MERROR("Failed to get broadcasted transaction pool hashes");
    return false;
  }

  for (const crypto::hash& hash : hashes)
    m_broadcasted_hashes.insert(hash);

  if (new_broadcasted_hash_count != m_broadcasted_hashes.size())
  {
    MERROR("Expected " << new_broadcasted_hash_count << " broadcasted hashes but got " << m_broadcasted_hashes.size());
    return false;
  }

  if (m_broadcasted_hashes.size() != c.get_pool_transactions_count())
  {
    MERROR("Expected " << m_broadcasted_hashes.size() << " broadcasted hashes but got " << c.get_pool_transactions_count());
    return false;
  }

  hashes.clear();
  if (!c.get_pool_transaction_hashes(hashes, false))
  {
    MERROR("Failed to get broadcasted transaction pool hashes");
    return false;
  }

  for (const crypto::hash& hash : hashes)
    m_all_hashes.insert(std::make_pair(hash, 0));

  if (new_broadcasted_hash_count != m_broadcasted_hashes.size())
  {
    MERROR("Expected " << new_broadcasted_hash_count << " broadcasted hashes but got " << m_broadcasted_hashes.size());
    return false;
  }

  hashes.clear();
  if (!c.get_pool_transaction_hashes(hashes, true))
  {

    MERROR("Failed to get all transaction pool hashes");
    return false;
  }

  for (const crypto::hash& hash : hashes)
    m_all_hashes.insert(std::make_pair(hash, 0));

  if (new_all_hash_count != m_all_hashes.size())
  {
    MERROR("Expected " << new_all_hash_count << " all hashes but got " << m_all_hashes.size());
    return false;
  }

  if (condition == relay_test::no_relay)
  {
    if (!m_no_relay_hashes.insert(m_last_tx).second)
    {
      MERROR("Expected new no_relay tx but got a duplicate legacy tx");
      return false;
    }

    for (const crypto::hash& hash : m_no_relay_hashes)
    {
      if (!c.pool_has_tx(hash))
      {
        MERROR("Expected public tx " << hash << " to be listed in pool");
        return false;
      }
    }
  }

  // check receive time changes
  {
    std::vector<cryptonote::tx_info> infos{};
    std::vector<cryptonote::spent_key_image_info> key_images{};
    if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images, true) || infos.size() != m_all_hashes.size())
    {
      MERROR("Unable to retrieve all txpool metadata");
      return false;
    }

    for (const cryptonote::tx_info& info : infos)
    {
      crypto::hash tx_hash;
      if (!epee::string_tools::hex_to_pod(info.id_hash, tx_hash))
      {
        MERROR("Unable to convert tx_hash hex to binary");
        return false;
      }

      const auto entry = m_all_hashes.find(tx_hash);
      if (entry == m_all_hashes.end())
      {
        MERROR("Unable to find tx_hash in set of tracked hashes");
        return false;
      }

      if (m_new_timestamp_index == ev_index && m_last_tx == tx_hash)
      {
        if (entry->second >= info.receive_time)
        {
          MERROR("Last relay time did not change as expected - last at " << entry->second << " and current at " << info.receive_time);
          return false;
        }
        entry->second = info.receive_time;
      }
      else if (entry->second != info.receive_time)
      {
        MERROR("Last relayed time changed unexpectedly from " << entry->second << " to " << info.receive_time);
        return false;
      }
    }
  }

  {
    std::vector<cryptonote::transaction> txes{};
    if (!c.get_pool_transactions(txes))
    {
      MERROR("Failed to get broadcasted transactions from pool");
      return false;
    }

    hashes.clear();
    for (const cryptonote::transaction& tx : txes)
      hashes.push_back(cryptonote::get_transaction_hash(tx));

    std::unordered_set<crypto::hash> public_hashes = m_broadcasted_hashes;
    for (const crypto::hash& hash : hashes)
    {
      if (!c.pool_has_tx(hash))
      {
        MERROR("Expected broadcasted tx " << hash << " to be listed in pool");
        return false;
      }

      if (!public_hashes.erase(hash))
      {
        MERROR("An unexected transaction was returned from the public pool");
        return false;
      }
    }
    if (!public_hashes.empty())
    {
      MERROR(public_hashes.size() << " transaction(s) were missing from the public pool");
      return false;
    }
  }

  {
    std::vector<cryptonote::transaction> txes{};
    if (!c.get_pool_transactions(txes, false))
    {
      MERROR("Failed to get broadcasted transactions from pool");
      return false;
    }

    hashes.clear();
    for (const cryptonote::transaction& tx : txes)
      hashes.push_back(cryptonote::get_transaction_hash(tx));

    std::unordered_set<crypto::hash> public_hashes = m_broadcasted_hashes;
    for (const crypto::hash& hash : hashes)
    {

      if (!public_hashes.erase(hash))
      {
        MERROR("An unexected transaction was returned from the public pool");
        return false;
      }
    }
    if (!public_hashes.empty())
    {
      MERROR(public_hashes.size() << " transaction(s) were missing from the public pool");
      return false;
    }
  }

  {
    std::vector<cryptonote::transaction> txes{};
    if (!c.get_pool_transactions(txes, true))
    {
      MERROR("Failed to get all transactions from pool");
      return false;
    }

    hashes.clear();
    for (const cryptonote::transaction& tx : txes)
      hashes.push_back(cryptonote::get_transaction_hash(tx));

    std::unordered_map<crypto::hash, uint64_t> all_hashes = m_all_hashes;
    for (const crypto::hash& hash : hashes)
    {
      if (!all_hashes.erase(hash))
      {
        MERROR("An unexected transaction was returned from the all pool");
        return false;
      }
    }
    if (!all_hashes.empty())
    {
      MERROR(m_broadcasted_hashes.size() << " transaction(s) were missing from the all pool");
      return false;
    }
  }

  {
    std::vector<cryptonote::tx_backlog_entry> entries{};
    if (!c.get_txpool_backlog(entries))
    {
      MERROR("Failed to get broadcasted txpool backlog");
      return false;
    }

    if (m_broadcasted_hashes.size() != entries.size())
    {
      MERROR("Expected " << m_broadcasted_hashes.size() << " in the broadcasted txpool backlog but got " << entries.size());
      return false;
    }
  }

  for (const std::pair<const crypto::hash, uint64_t>& hash : m_all_hashes)
  {
    cryptonote::blobdata tx_blob{};
    if (!c.get_pool_transaction(hash.first, tx_blob, cryptonote::relay_category::all))
    {
      MERROR("Failed to retrieve tx expected to be in pool: " << hash.first);
      return false;
    }
  }

  {
    std::unordered_map<crypto::hash, uint64_t> difference = m_all_hashes;
    for (const crypto::hash& hash : m_broadcasted_hashes)
      difference.erase(hash);

    for (const crypto::hash& hash : m_no_relay_hashes)
      difference.erase(hash);

    for (const std::pair<const crypto::hash, uint64_t>& hash : difference)
    {
      if (c.pool_has_tx(hash.first))
      {
        MERROR("Did not expect private/hidden tx " << hash.first << " to be listed in pool");
        return false;
      }

      cryptonote::blobdata tx_blob{};
      if (c.get_pool_transaction(hash.first, tx_blob, cryptonote::relay_category::broadcasted))
      {
        MERROR("Tx " << hash.first << " is not supposed to be in broadcasted pool");
        return false;
      }

      if (!c.get_pool_transaction(hash.first, tx_blob, cryptonote::relay_category::all))
      {
        MERROR("Tx " << hash.first << " blob could not be retrieved from pool");
        return false;
      }
    }
  }

  {
    cryptonote::txpool_stats stats{};
    if (!c.get_pool_transaction_stats(stats) || stats.txs_total != m_broadcasted_hashes.size())
    {
      MERROR("Expected broadcasted stats to list " << m_broadcasted_hashes.size() << " txes but got " << stats.txs_total);
      return false;
    }

    if (!c.get_pool_transaction_stats(stats, false) || stats.txs_total != m_broadcasted_hashes.size())
    {
      MERROR("Expected broadcasted stats to list " << m_broadcasted_hashes.size() << " txes but got " << stats.txs_total);
      return false;
    }

    if (!c.get_pool_transaction_stats(stats, true) || stats.txs_total != m_all_hashes.size())
    {
      MERROR("Expected all stats to list " << m_all_hashes.size() << " txes but got " << stats.txs_total);
      return false;
    }
  }

  {
    std::vector<cryptonote::rpc::tx_in_pool> infos{};
    cryptonote::rpc::key_images_with_tx_hashes key_images{};
    if (!c.get_pool_for_rpc(infos, key_images) || infos.size() != m_broadcasted_hashes.size() || key_images.size() != m_broadcasted_hashes.size())
    {
      MERROR("Expected broadcasted rpc data to return " << m_broadcasted_hashes.size() << " but got " << infos.size() << " infos and " << key_images.size() << "key images");
      return false;
    }
  }
  return true;
}

bool txpool_double_spend_base::check_unchanged(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events */)
{
  return check_changed(c, ev_index, relay_test::no_change);
}

bool txpool_double_spend_base::check_new_broadcasted(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events */)
{
  return check_changed(c, ev_index, relay_test::broadcasted);
}

bool txpool_double_spend_base::check_new_hidden(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events */)
{
  return check_changed(c, ev_index, relay_test::hidden);
}
bool txpool_double_spend_base::check_new_no_relay(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& /*events */)
{
  return check_changed(c, ev_index, relay_test::no_relay);
}

bool txpool_double_spend_base::check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t event_idx, const cryptonote::transaction& tx) 
{
  m_last_tx = cryptonote::get_transaction_hash(tx);
  if (m_no_new_index == event_idx)
    return !tvc.m_verifivation_failed && !tx_added;
  else if (m_failed_index == event_idx)
    return tvc.m_verifivation_failed;// && !tx_added;
  else
    return !tvc.m_verifivation_failed && tx_added;
}

bool txpool_double_spend_norelay::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_do_not_relay);
  DO_CALLBACK(events, "mark_no_new");

  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);

  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_no_relay");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "check_unchanged");
  SET_EVENT_VISITOR_SETT(events, 0);
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "check_unchanged");

  // kepped by block currently does not change txpool status
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block);
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "check_unchanged");

  return true;
}

bool txpool_double_spend_local::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_local_relay);
  DO_CALLBACK(events, "mark_no_new");

  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);

  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_hidden");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "check_unchanged");
  SET_EVENT_VISITOR_SETT(events, 0);
  DO_CALLBACK(events, "timestamp_change_pause");
  events.push_back(tx_0);
  DO_CALLBACK(events, "increase_broadcasted_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_broadcasted");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_unchanged");

  return true;
}

bool txpool_double_spend_keyimage::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_local_relay);
  DO_CALLBACK(events, "mark_no_new");

  const std::size_t tx_index1 = events.size();
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);

  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_stem);
  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_hidden");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  const std::size_t tx_index2 = events.size();
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_unchanged");

  // use same key image with different id
  cryptonote::transaction tx_1;
  {
    auto events_copy = events;
    events_copy.erase(events_copy.begin() + tx_index1);
    events_copy.erase(events_copy.begin() + tx_index2 - 1);
    MAKE_TX(events_copy, tx_temp, miner_account, bob_account, send_amount, blk_0r);
    tx_1 = tx_temp;
  }

  // same key image
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_failed");
  events.push_back(tx_1);
  DO_CALLBACK(events, "check_unchanged");

  return true;
}

bool txpool_stem_loop::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_stem);
  DO_CALLBACK(events, "mark_no_new");

  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);

  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_hidden");
  DO_CALLBACK(events, "timestamp_change_pause");
  events.push_back(tx_0);
  DO_CALLBACK(events, "increase_broadcasted_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_broadcasted");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_unchanged");

  return true;
}
