// Copyright (c) 2019-2026, The Monero Project
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
#include <ctime>
#include "blockchain_db/locked_txn.h"
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
  , m_all_ki_count(0)
  , m_all_tx_count(0)
{
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_all_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, decrease_all_tx_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, increase_all_ki_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, decrease_all_ki_count);
  REGISTER_CALLBACK_METHOD(txpool_spend_key_public, check_txpool_spent_keys);
}

bool txpool_base::increase_all_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_all_tx_count;
  ++m_all_ki_count;
  return true;
}

bool txpool_base::decrease_all_tx_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  if (0 == m_all_tx_count || 0 == m_all_ki_count)
  {
    MERROR("txpool_base: cannot decrease tx/ki count past 0");
    return false;
  }
  --m_all_tx_count;
  --m_all_ki_count;
  return true;
}

bool txpool_base::increase_all_ki_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  ++m_all_ki_count;
  return true;
}

bool txpool_base::decrease_all_ki_count(cryptonote::core& /*c*/, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  if (0 == m_all_ki_count)
  {
    MERROR("txpool_base: cannot decrease ki count past 0");
    return false;
  }
  --m_all_ki_count;
  return true;
}

bool txpool_base::check_txpool_spent_keys(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& events)
{
  std::vector<cryptonote::tx_info> infos{};
  std::vector<cryptonote::spent_key_image_info> key_images{};
  if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images))
  {
    MERROR("Failed to fetch mempool info");
    return false;
  }

  if (infos.size() != m_all_tx_count)
  {
    MERROR("Failed mempool tx count: expected " << m_all_tx_count << " vs actual " << infos.size());
    return false;
  }

  if (key_images.size() != m_all_ki_count)
  {
    MERROR("Failed mempool spent keys count: expected " << m_all_ki_count << " vs actual " << key_images.size());
    return false;
  }

  return true;
}

bool txpool_spend_key_public::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  DO_CALLBACK(events, "check_txpool_spent_keys");
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);
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

bool txpool_drops_tx_into_blockchain::generate(std::vector<test_event_entry>& events) const
{
  INIT_MEMPOOL_TEST();

  // test 0 in mempool
  DO_CALLBACK(events, "check_txpool_spent_keys");

  // add 1 tx to mempool
  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);
  DO_CALLBACK(events, "increase_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  // mine block, which removes tx from mempool
  MAKE_NEXT_BLOCK_TX1(events, blk_1, blk_0r, miner_account, tx_0)
  DO_CALLBACK(events, "decrease_all_tx_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  return true;
}

txpool_cleanup_after_hf::txpool_cleanup_after_hf(): txpool_base(),
  m_main_chain_txid(), m_cleaned_txid(), m_post_v16_fork()
{
  REGISTER_CALLBACK_METHOD(txpool_cleanup_after_hf, set_txids_from_events)
  REGISTER_CALLBACK_METHOD(txpool_cleanup_after_hf, set_post_v16_fork)
  REGISTER_CALLBACK_METHOD(txpool_cleanup_after_hf, check_txid_in_chain)
  REGISTER_CALLBACK_METHOD(txpool_cleanup_after_hf, wait_until_txid_gone)
}

bool txpool_cleanup_after_hf::generate(std::vector<test_event_entry>& events) const
{
  uint64_t send_amount = 1000;
  uint64_t ts_start = 1338224400;
  GENERATE_ACCOUNT(miner_account);
  GENERATE_ACCOUNT(bob_account);
  MAKE_GENESIS_BLOCK(events, blk_0, miner_account, ts_start);

  static const event_replay_settings settings{
    .hard_forks = {{
        {1, 1},
        {2, 2},
        {3, 3},
        {4, 4},
        {5, 5},
        {6, 6},
        {7, 7},
        {8, 8},
        {9, 9},
        {10, 10},
        {11, 11},
        {12, 12},
        {13, 13},
        {14, 14},
        {15, 15},
        {16, 80}, //v16 is the softfork disallowing BP range proofs and no-view-tag txs
    }}
  };
  events.push_back(settings);
  const auto get_ideal_hf_version = [](size_t h) -> uint8_t {
    for (auto it = settings.hard_forks->crbegin(); it != settings.hard_forks->crend(); ++it)
      if (h >= it->second)
        return it->first;
    return 0;
  };

  // mine to 1 block before v16 soft fork
  const size_t height_target = settings.hard_forks->back().second - 1;
  size_t height = 1;
  cryptonote::block blk_0r = blk_0;
  for (; height < height_target; ++height)
  {
    const uint8_t hf_version = get_ideal_hf_version(height);
    if (!generator.construct_block(blk_0r, blk_0r, miner_account, {}, hf_version))
      return false;
    events.push_back(blk_0r);
  }

  const auto make_bp_tx = [&]() -> cryptonote::transaction {
    cryptonote::transaction tx;
    if (!construct_tx_to_key(events, tx, blk_0r, miner_account, bob_account,
        send_amount, TESTS_DEFAULT_FEE, /*nmix=*/15, /*rct=*/true,
        rct::RangeProofPaddedBulletproof, /*bp_version=*/3,
        /*check_unlock_time=*/true, /*fnc_tx_in_accept=*/{},
        /*use_view_tags=*/false))
      throw std::runtime_error("Could not make a BP tx");
    events.push_back(tx);
    return tx;
  };

  DO_CALLBACK(events, "check_txpool_spent_keys");

  // add BP / no-view-tag TX to mempool
  const cryptonote::transaction tx_0 = make_bp_tx();
  DO_CALLBACK(events, "increase_all_tx_count");
  for (size_t i = 1; i < tx_0.vin.size(); ++i)
    DO_CALLBACK(events, "increase_all_ki_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  // add another
  const cryptonote::transaction tx_1 = make_bp_tx();
  DO_CALLBACK(events, "increase_all_tx_count");
  for (size_t i = 1; i < tx_1.vin.size(); ++i)
    DO_CALLBACK(events, "increase_all_ki_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "set_txids_from_events");

  // mine last v15 block containing only first tx
  MAKE_NEXT_BLOCK_TX_LIST(events, last_bp_block, blk_0r, miner_account,
    {tx_0});
  DO_CALLBACK(events, "decrease_all_tx_count");
  for (size_t i = 1; i < tx_0.vin.size(); ++i)
    DO_CALLBACK(events, "decrease_all_ki_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "set_post_v16_fork");
  DO_CALLBACK(events, "check_txid_in_chain");

  // try add another tx (should fail, checked in our `check_tx_verification_context()`)
  make_bp_tx();
  DO_CALLBACK(events, "check_txpool_spent_keys");

  // wait until old tx is cleaned up
  DO_CALLBACK(events, "wait_until_txid_gone");
  DO_CALLBACK(events, "decrease_all_tx_count");
  for (size_t i = 1; i < tx_1.vin.size(); ++i)
    DO_CALLBACK(events, "decrease_all_ki_count");
  DO_CALLBACK(events, "check_txpool_spent_keys");

  return true;
}

bool txpool_cleanup_after_hf::check_tx_verification_context(const cryptonote::tx_verification_context& tvc, bool tx_added, size_t /*event_index*/, const cryptonote::transaction& tx)
{
  if (m_post_v16_fork && tx.version == 2 && tx.rct_signatures.type == rct::RCTTypeCLSAG)
    return !tx_added && tvc.m_verifivation_failed;
  return !tvc.m_verifivation_failed;
}

bool txpool_cleanup_after_hf::set_txids_from_events(cryptonote::core& c,
  size_t ev_index,
  const std::vector<test_event_entry>& events)
{
  // step back until right at v16 change
  auto it = events.crbegin();
  for (; it != events.crend(); ++it)
  {
    const callback_entry *pcb = boost::strict_get<callback_entry>(&*it);
    if (pcb && pcb->callback_name == "set_post_v16_fork")
      break;
  }
  // assign last TXID to `m_cleaned_txid` and 2nd last to `m_main_chain_txid`
  for (; it != events.crend(); ++it)
  {
    const cryptonote::transaction *ptx = boost::strict_get<cryptonote::transaction>(&*it);
    if (!ptx)
      continue;
    if (m_cleaned_txid == crypto::null_hash)
    {
      m_cleaned_txid = cryptonote::get_transaction_hash(*ptx);
    }
    else
    {
      m_main_chain_txid = cryptonote::get_transaction_hash(*ptx);
      break;
    }
  }
  return m_cleaned_txid != crypto::null_hash && m_main_chain_txid != crypto::null_hash;
}

bool txpool_cleanup_after_hf::set_post_v16_fork(cryptonote::core& c,
  size_t ev_index, const std::vector<test_event_entry>& events)
{
  m_post_v16_fork = true;
  return true;
}

bool txpool_cleanup_after_hf::check_txid_in_chain(cryptonote::core& c,
  size_t ev_index, const std::vector<test_event_entry>& events)
{
  CHECK_AND_ASSERT_MES(m_main_chain_txid != crypto::null_hash, false,
    "txpool_cleanup_after_hf main chain txid wasn't set");
  const bool in_chain = c.get_blockchain_storage().have_tx(m_main_chain_txid);
  CHECK_AND_ASSERT_MES(in_chain, false, "Missing expected TX in blockchain");
  return true;
}

bool txpool_cleanup_after_hf::wait_until_txid_gone(cryptonote::core& c,
  size_t ev_index, const std::vector<test_event_entry>& events)
{
  constexpr auto max_wait_time = std::chrono::seconds(32);
  const auto start_time = std::chrono::steady_clock::now();

  c.on_idle();

  CHECK_AND_ASSERT_MES(m_cleaned_txid != crypto::null_hash, false,
    "txpool_cleanup_after_hf cleaned txid wasn't set");

  while (std::chrono::steady_clock::now() - start_time < max_wait_time)
  {
    const bool in_chain = c.get_blockchain_storage().have_tx(m_cleaned_txid);
    CHECK_AND_ASSERT_MES(!in_chain, false,
      "TX to be cleaned is unexpectedly in blockchain");
    std::vector<crypto::hash> pool_txids;
    CHECK_AND_ASSERT_MES(c.get_pool_transaction_hashes(pool_txids), false,
      "Faild to fetch mempool TXIDs");
    if (pool_txids.empty())
    {
      const auto elapsed_ms = std::chrono::duration_cast
        <std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start_time).count();
      MINFO("Mempool was cleaned in " << elapsed_ms << "ms");
      return true;
    }
    CHECK_AND_ASSERT_MES(pool_txids.size() == 1, false,
      "Too many TXIDs in mempool");
    CHECK_AND_ASSERT_MES(pool_txids.at(0) == m_cleaned_txid, false,
      "Unexpected TX in mempool while waiting for cleanse");
    boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    c.on_idle();
  }

  MERROR("Timed out waiting for expired tx to exit mempool");
  return false;
}

txpool_future_receive_time::txpool_future_receive_time()
  : test_chain_unit_base()
  , m_future_txids{crypto::null_hash, crypto::null_hash}
  , m_expired_txid(crypto::null_hash)
{
  REGISTER_CALLBACK_METHOD(txpool_future_receive_time, set_future_receive_time);
  REGISTER_CALLBACK_METHOD(txpool_future_receive_time, check_txpool);
}

bool txpool_future_receive_time::generate(std::vector<test_event_entry>& events)
{
  INIT_MEMPOOL_TEST();
  GENERATE_ACCOUNT(alice_account);
  GENERATE_ACCOUNT(carol_account);

  MAKE_TX(events, tx_0, miner_account, bob_account, send_amount, blk_0r);
  MAKE_TX(events, tx_1, miner_account, alice_account, send_amount, blk_0r);
  MAKE_TX(events, tx_2, miner_account, carol_account, send_amount, blk_0r);
  DO_CALLBACK(events, "set_future_receive_time");
  DO_CALLBACK(events, "check_txpool");

  return true;
}

bool txpool_future_receive_time::set_future_receive_time(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry>& events)
{
  cryptonote::Blockchain& blockchain = c.get_blockchain_storage();
  m_future_txids[0] = cryptonote::get_transaction_hash(boost::get<cryptonote::transaction>(events.at(ev_index - 3)));
  m_future_txids[1] = cryptonote::get_transaction_hash(boost::get<cryptonote::transaction>(events.at(ev_index - 2)));
  m_expired_txid = cryptonote::get_transaction_hash(boost::get<cryptonote::transaction>(events.at(ev_index - 1)));

  const uint64_t now = std::time(nullptr);
  const auto set_receive_time = [&blockchain](const crypto::hash& txid, uint64_t receive_time)
  {
    cryptonote::txpool_tx_meta_t meta{};
    if (!blockchain.get_txpool_tx_meta(txid, meta))
      return false;
    meta.receive_time = receive_time;
    blockchain.update_txpool_tx(txid, meta);
    return true;
  };

  cryptonote::LockedTXN lock(blockchain.get_db());
  if (!set_receive_time(m_future_txids[0], now + 3600) ||
      !set_receive_time(m_future_txids[1], now + 3600) ||
      !set_receive_time(m_expired_txid, now - CRYPTONOTE_MEMPOOL_TX_LIVETIME - 1))
  {
    MERROR("Failed to update transaction pool metadata");
    return false;
  }
  lock.commit();
  return true;
}

bool txpool_future_receive_time::check_txpool(cryptonote::core& c, size_t /*ev_index*/, const std::vector<test_event_entry>& /*events*/)
{
  bool valid = true;
  cryptonote::txpool_stats stats{};
  if (!c.get_pool_transaction_stats(stats) || stats.txs_total != 3)
  {
    MERROR("Failed to get transaction pool statistics");
    valid = false;
  }

  uint32_t histogram_txs = 0;
  for (const cryptonote::txpool_histo& entry : stats.histo)
    histogram_txs += entry.txs;
  if (histogram_txs != stats.txs_total)
  {
    MERROR("Transaction pool histogram contains " << histogram_txs << " transactions, expected " << stats.txs_total);
    valid = false;
  }

  c.on_idle();
  for (const crypto::hash& txid : m_future_txids)
  {
    if (!c.pool_has_tx(txid))
    {
      MERROR("Transaction with a future receive time was removed from the pool");
      valid = false;
    }
  }
  if (c.pool_has_tx(m_expired_txid))
  {
    MERROR("Expired transaction was not removed from the pool");
    valid = false;
  }

  stats = {};
  if (!c.get_pool_transaction_stats(stats) || stats.txs_total != 2)
  {
    MERROR("Failed to get transaction pool statistics after cleanup");
    valid = false;
  }

  histogram_txs = 0;
  for (const cryptonote::txpool_histo& entry : stats.histo)
    histogram_txs += entry.txs;
  const uint64_t now = std::time(nullptr);
  if (histogram_txs != stats.txs_total || stats.oldest > now)
  {
    MERROR("Invalid transaction pool statistics after cleanup");
    valid = false;
  }

  return valid;
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
    if (!c.get_pool_transactions_and_spent_keys_info(infos, key_images) || infos.size() != m_all_hashes.size())
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
        MERROR("An unexpected transaction was returned from the public pool");
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
        MERROR("An unexpected transaction was returned from the public pool");
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
        MERROR("An unexpected transaction was returned from the all pool");
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
  DO_CALLBACK(events, "check_txpool_spent_keys");
  DO_CALLBACK(events, "mark_timestamp_change");
  DO_CALLBACK(events, "check_new_broadcasted");
  DO_CALLBACK(events, "timestamp_change_pause");
  DO_CALLBACK(events, "mark_no_new");
  events.push_back(tx_0);
  DO_CALLBACK(events, "check_unchanged");

  return true;
}
