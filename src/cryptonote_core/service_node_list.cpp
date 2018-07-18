// Copyright (c)      2018, The Loki Project
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

#include <functional>
#include <random>

#include "ringct/rctSigs.h"
#include "wallet/wallet2.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/tx_extra.h"

#include "service_node_list.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{

  service_node_list::service_node_list(cryptonote::Blockchain& blockchain)
    : m_blockchain(blockchain)
  {
    blockchain.hook_block_added(*this);
    blockchain.hook_blockchain_detached(*this);
    blockchain.hook_init(*this);
    blockchain.hook_validate_miner_tx(*this);
  }

  void service_node_list::init()
  {
    // TODO: Save this calculation, only do it if it's not here.
    LOG_PRINT_L0("Recalculating service nodes list, scanning last 30 days");

    m_service_nodes_last_reward.clear();
    m_service_nodes_keys.clear();

    while (!m_rollback_events.empty())
    {
      m_rollback_events.pop_front();
    }

    uint64_t current_height = m_blockchain.get_current_blockchain_height();
    uint64_t start_height = 0;
    if (current_height >= STAKING_REQUIREMENT_LOCK_BLOCKS + STAKING_RELOCK_WINDOW_BLOCKS)
    {
      start_height = current_height - STAKING_REQUIREMENT_LOCK_BLOCKS - STAKING_RELOCK_WINDOW_BLOCKS;
    }

    for (uint64_t height = start_height; height <= current_height; height += 1000)
    {
      std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
      if (!m_blockchain.get_blocks(height, 1000, blocks))
      {
        LOG_ERROR("Unable to initialize service nodes list");
        return;
      }

      for (const auto& block_pair : blocks)
      {
        const cryptonote::block& block = block_pair.second;
        std::list<cryptonote::transaction> txs;
        std::list<crypto::hash> missed_txs;
        if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
        {
          LOG_ERROR("Unable to get transactions for block " << block.hash);
          return;
        }

        block_added_generic(block, txs);
      }
    }

    m_rollback_events.push_back(std::unique_ptr<rollback_event>(new prevent_rollback(current_height)));
  }

  std::vector<crypto::public_key> service_node_list::get_service_node_pubkeys() const
  {
    std::vector<crypto::public_key> result;
    for (const auto& iter : m_service_nodes_keys)
      result.push_back(iter.second);

    std::sort(result.begin(), result.end(),
      [](const crypto::public_key &a, const crypto::public_key &b) {
        return memcmp(reinterpret_cast<const void*>(&a), reinterpret_cast<const void*>(&b), sizeof(a)) < 0;
      });
    return result;
  }

  const std::shared_ptr<quorum_state> service_node_list::get_quorum_state(uint64_t height) const
  {
    std::shared_ptr<service_nodes::quorum_state> result;
    const auto &it = m_quorum_states.find(height);
    if (it == m_quorum_states.end())
    {
      // TODO(loki): Not being able to find the quorum is going to be a fatal error.
    }
    else
    {
      result = it->second;
    }

    return result;
  }

  bool service_node_list::is_service_node(const crypto::public_key& pubkey) const
  {
    // TODO: speed this up, it should just be a single lookup.
    for (const auto& iter : m_service_nodes_keys)
      if (iter.second == pubkey)
        return true;
    return false;
  }

  bool service_node_list::reg_tx_has_correct_unlock_time(const cryptonote::transaction& tx, uint64_t block_height) const
  {
    return tx.unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && tx.unlock_time >= block_height + STAKING_REQUIREMENT_LOCK_BLOCKS;
  }

  bool service_node_list::reg_tx_extract_fields(const cryptonote::transaction& tx, cryptonote::account_public_address& address, crypto::public_key& service_node_key, crypto::public_key& tx_pub_key) const
  {
    cryptonote::tx_extra_service_node_register register_;
    if (cryptonote::get_service_node_register_from_tx_extra(tx.extra, register_))
    {
      address.m_spend_public_key = register_.public_spend_key;
      address.m_view_public_key = register_.public_view_key;
      service_node_key = register_.service_node_key;
    }
    else
    {
      address.m_spend_public_key = crypto::null_pkey;
      address.m_view_public_key = crypto::null_pkey;
      service_node_key = crypto::null_pkey;
    }

    tx_pub_key = cryptonote::get_tx_pub_key_from_extra(tx.extra);
    return address.m_spend_public_key != crypto::null_pkey &&
      address.m_view_public_key != crypto::null_pkey &&
      tx_pub_key != crypto::null_pkey &&
      service_node_key != crypto::null_pkey;
  }

  bool service_node_list::is_reg_tx_staking_output(const cryptonote::transaction& tx, int i, uint64_t block_height, crypto::key_derivation derivation, hw::device& hwdev) const
  {
    if (tx.vout[i].target.type() != typeid(cryptonote::txout_to_key))
    {
      return false;
    }

    rct::key mask;
    uint64_t money_transferred = 0;

    crypto::secret_key scalar1;
    hwdev.derivation_to_scalar(derivation, i, scalar1);
    try
    {
      switch (tx.rct_signatures.type)
      {
      case rct::RCTTypeSimple:
      case rct::RCTTypeSimpleBulletproof:
        money_transferred = rct::decodeRctSimple(tx.rct_signatures, rct::sk2rct(scalar1), i, mask, hwdev);
        break;
      case rct::RCTTypeFull:
      case rct::RCTTypeFullBulletproof:
        money_transferred = rct::decodeRct(tx.rct_signatures, rct::sk2rct(scalar1), i, mask, hwdev);
        break;
      default:
        LOG_ERROR("Unsupported rct type: " << tx.rct_signatures.type);
        return false;
      }
    }
    catch (const std::exception &e)
    {
      LOG_ERROR("Failed to decode input " << i);
      return false;
    }

    return money_transferred >= m_blockchain.get_staking_requirement(block_height);
  }

  bool service_node_list::is_deregistration_tx(const cryptonote::transaction& tx, cryptonote::account_public_address &address) const
  {
    if (tx.version != cryptonote::transaction::version_3_deregister_tx)
    {
        return false;
    }

    cryptonote::tx_extra_service_node_deregister deregister;
    if (!cryptonote::get_service_node_deregister_from_tx_extra(tx.extra, deregister))
    {
      LOG_ERROR("Transaction deregister did not have deregister data in tx extra, possibly corrupt tx in blockchain");
      return false;
    }

    if (const std::shared_ptr<quorum_state> state = get_quorum_state(deregister.block_height))
    {
      if (deregister.service_node_index >= state->nodes_to_test.size())
      {
          LOG_ERROR("Service node index to vote off has become invalid, quorum rules have changed without a hardfork.");
          return false;
      }

      const crypto::public_key &snode_pubkey = state->nodes_to_test[deregister.service_node_index];
      for (const auto& key_it : m_service_nodes_keys)
      {
        if (key_it.second == snode_pubkey)
        {
          address = key_it.first;
          return true;
        }
      }
    }
    else
    {
      // TODO(loki): Not being able to find a quorum is fatal! We want better caching abilities.
      LOG_ERROR("Quorum state for height: " << deregister.block_height << ", was not stored by the daemon");
    }

    return false;
  }

  // This function takes a tx and returns true if it is a staking transaction.
  // It also sets the address argument to the public spendkey and pub viewkey of
  // the transaction.
  //
  bool service_node_list::is_registration_tx(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address, crypto::public_key& key) const
  {
    if (!reg_tx_has_correct_unlock_time(tx, block_height))
    {
      return false;
    }

    crypto::public_key tx_pub_key, service_node_key;
    cryptonote::account_public_address service_node_address;
    
    if (!reg_tx_extract_fields(tx, service_node_address, service_node_key, tx_pub_key))
    {
      return false;
    }

    if (is_service_node(service_node_key))
      return false;

    cryptonote::keypair gov_key = cryptonote::get_deterministic_keypair_from_height(1);

    crypto::key_derivation derivation;
    crypto::generate_key_derivation(service_node_address.m_view_public_key, gov_key.sec, derivation);

    hw::device& hwdev = hw::get_device("default");

    for (size_t i = 0; i < tx.vout.size(); ++i)
    {
      if (is_reg_tx_staking_output(tx, i, block_height, derivation, hwdev))
      {
        address = service_node_address;
        key = service_node_key;
        return true;
      }
    }
    return false;
  }

  void service_node_list::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    block_added_generic(block, txs);
  }

  cryptonote::account_public_address service_node_list::find_service_node_from_miner_tx(const cryptonote::transaction& miner_tx, uint64_t height) const
  {
    if (miner_tx.vout.size() != 3)
    {
      MERROR("Miner tx should have 3 outputs");
      return null_address;
    }

    if (miner_tx.vout[1].target.type() != typeid(cryptonote::txout_to_key))
    {
      MERROR("Service node output target type should be txout_to_key");
      return null_address;
    }

    cryptonote::keypair gov_key = cryptonote::get_deterministic_keypair_from_height(height);

    for (const auto& address_blockheight : m_service_nodes_last_reward)
    {
      const cryptonote::account_public_address address = address_blockheight.first;
      const crypto::public_key& pub_spendkey = address.m_spend_public_key;
      const crypto::public_key& pub_viewkey = address.m_view_public_key;

      crypto::key_derivation derivation;
      crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
      bool r = crypto::generate_key_derivation(pub_viewkey, gov_key.sec, derivation);
      CHECK_AND_ASSERT_MES(r, null_address, "while creating outs: failed to generate_key_derivation(" << pub_viewkey << ", " << gov_key.sec << ")");
      r = crypto::derive_public_key(derivation, 1, pub_spendkey, out_eph_public_key);
      CHECK_AND_ASSERT_MES(r, null_address, "while creating outs: failed to derive_public_key(" << derivation << ", " << 1 << ", "<< pub_spendkey << ")");

      if (boost::get<cryptonote::txout_to_key>(miner_tx.vout[1].target).key == out_eph_public_key)
      {
        return address;
      }
    }

    return null_address;
  }


  template<typename T>
  void service_node_list::block_added_generic(const cryptonote::block& block, const T& txs)
  {
    uint64_t block_height = cryptonote::get_block_height(block);
    int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

    if (hard_fork_version < 8)
      return;

    assert(block.miner_tx.vout.size() == 3);

    while (!m_rollback_events.empty() && m_rollback_events.front()->m_block_height < block_height - ROLLBACK_EVENT_EXPIRATION_BLOCKS)
    {
      m_rollback_events.pop_front();
    }

    cryptonote::account_public_address winner_address = find_service_node_from_miner_tx(block.miner_tx, block_height);
    if (m_service_nodes_last_reward.count(winner_address) == 1)
    {
      m_rollback_events.push_back(
        std::unique_ptr<rollback_event>(
          new rollback_change(block_height, winner_address, m_service_nodes_last_reward[winner_address], m_service_nodes_keys[winner_address])
        )
      );
      m_service_nodes_last_reward[winner_address] = std::pair<uint64_t, size_t>(block_height, 0);
    }

    for (const cryptonote::account_public_address address : get_expired_nodes(block_height))
    {
      auto i = m_service_nodes_last_reward.find(address);
      if (i != m_service_nodes_last_reward.end())
      {
        m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, address, i->second, m_service_nodes_keys[winner_address])));
        m_service_nodes_last_reward.erase(i);
        m_service_nodes_keys.erase(address);
      }
    }

    size_t index = 1;
    for (const cryptonote::transaction& tx : txs)
    {
      cryptonote::account_public_address address;
      crypto::public_key key;
      if (is_registration_tx(tx, block_height, address, key))
      {
        auto iter = m_service_nodes_last_reward.find(address);
        if (iter == m_service_nodes_last_reward.end())
        {
          m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_new(block_height, address)));
          m_service_nodes_last_reward[address] = std::pair<uint64_t, size_t>(block_height, index);
          m_service_nodes_keys[address] = key;
        }
        else
        {
          crypto::public_key& service_node_key = m_service_nodes_keys[address];
          m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, address, iter->second, service_node_key)));
          iter->second = std::pair<uint64_t, size_t>(block_height, index);
          service_node_key = key;
        }
      }
      else if (is_deregistration_tx(tx, address))
      {
        auto iter = m_service_nodes_last_reward.find(address);
        if (iter != m_service_nodes_last_reward.end())
        {
          crypto::public_key& service_node_key = m_service_nodes_keys[address];
          m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, address, iter->second, service_node_key)));
          m_service_nodes_last_reward.erase(iter);
          m_service_nodes_keys.erase(address);
        }
        else
        {
          MWARNING("Tried to kick off a service node that is no longer registered");
        }
      }
      index++;
    }

    const uint64_t curr_height           = m_blockchain.get_current_blockchain_height();

    const size_t QUORUM_LIFETIME         = loki::service_node_deregister::DEREGISTER_LIFETIME_BY_HEIGHT;
    const size_t cache_state_from_height = (curr_height < QUORUM_LIFETIME) ? 0 : curr_height - QUORUM_LIFETIME;

    if (block_height >= cache_state_from_height)
    {
      store_quorum_state_from_rewards_list(block_height);

      while (!m_quorum_states.empty() && m_quorum_states.begin()->first < cache_state_from_height)
      {
        m_quorum_states.erase(m_quorum_states.begin());
      }
    }
  }

  void service_node_list::blockchain_detached(uint64_t height)
  {
    while (!m_rollback_events.empty() && m_rollback_events.back()->m_block_height >= height)
    {
      if (!m_rollback_events.back()->apply(m_service_nodes_last_reward, m_service_nodes_keys))
      {
        init();
        break;
      }
      m_rollback_events.pop_back();
    }

    while (!m_quorum_states.empty() && (--m_quorum_states.end())->first >= height)
    {
      m_quorum_states.erase(--m_quorum_states.end());
    }
  }

  std::vector<cryptonote::account_public_address> service_node_list::get_expired_nodes(uint64_t block_height) const
  {
    std::vector<cryptonote::account_public_address> expired_nodes;

    if (block_height < STAKING_REQUIREMENT_LOCK_BLOCKS + STAKING_RELOCK_WINDOW_BLOCKS)
      return expired_nodes;

    const uint64_t expired_nodes_block_height = block_height - STAKING_REQUIREMENT_LOCK_BLOCKS - STAKING_RELOCK_WINDOW_BLOCKS;

    std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
    if (!m_blockchain.get_blocks(expired_nodes_block_height, 1, blocks))
    {
      LOG_ERROR("Unable to get historical blocks");
      return expired_nodes;
    }

    const cryptonote::block& block = blocks.begin()->second;
    std::list<cryptonote::transaction> txs;
    std::list<crypto::hash> missed_txs;
    if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
    {
      LOG_ERROR("Unable to get transactions for block " << block.hash);
      return expired_nodes;
    }

    for (const cryptonote::transaction& tx : txs)
    {
      cryptonote::account_public_address address;
      crypto::public_key unused_key;
      if (is_registration_tx(tx, expired_nodes_block_height, address, unused_key))
      {
        expired_nodes.push_back(address);
      }
    }

    return expired_nodes;
  }

  cryptonote::account_public_address service_node_list::select_winner(const crypto::hash& prev_id)
  {
    auto lowest_height = std::pair<uint64_t, size_t>(std::numeric_limits<uint64_t>::max(), std::numeric_limits<size_t>::max());
    cryptonote::account_public_address address = null_address;
    for (const auto& spendkey_blockheight : m_service_nodes_last_reward)
    {
      if (spendkey_blockheight.second < lowest_height)
      {
        lowest_height = spendkey_blockheight.second;
        address = spendkey_blockheight.first;
      }
    }
    return address;
  }

  /// validates the miner TX for the next block
  //
  bool service_node_list::validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward)
  {
    if (hard_fork_version < 8)
      return true;

    uint64_t service_node_reward = cryptonote::get_service_node_reward(height, base_reward, hard_fork_version);

    if (miner_tx.vout.size() != 3)
    {
      MERROR("Miner TX should have exactly 3 outputs");
      return false;
    }

    if (miner_tx.vout[1].amount != service_node_reward)
    {
      MERROR("Service node reward amount incorrect. Should be " << cryptonote::print_money(service_node_reward) << ", is: " << cryptonote::print_money(miner_tx.vout[1].amount));
      return false;
    }

    if (miner_tx.vout[1].target.type() != typeid(cryptonote::txout_to_key))
    {
      MERROR("Service node output target type should be txout_to_key");
      return false;
    }

    crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);;
    crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
    cryptonote::account_public_address address = select_winner(prev_id);
    cryptonote::keypair gov_key = cryptonote::get_deterministic_keypair_from_height(height);

    bool r = crypto::generate_key_derivation(address.m_view_public_key, gov_key.sec, derivation);
    CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << address.m_view_public_key << ", " << gov_key.sec << ")");
    r = crypto::derive_public_key(derivation, 1, address.m_spend_public_key, out_eph_public_key);
    CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << 1 << ", "<< address.m_spend_public_key << ")");

    if (boost::get<cryptonote::txout_to_key>(miner_tx.vout[1].target).key != out_eph_public_key)
    {
      MERROR("Invalid service node reward output");
      return false;
    }

    return true;
  }

  void service_node_list::store_quorum_state_from_rewards_list(uint64_t height)
  {
    const crypto::hash block_hash = m_blockchain.get_block_id_by_height(height);
    if (block_hash == crypto::null_hash)
    {
      MERROR("Block height: " << height << " returned null hash");
      return;
    }

    std::vector<crypto::public_key> full_node_list = get_service_node_pubkeys();
    std::vector<size_t>                              pub_keys_indexes(full_node_list.size());
    {
      size_t index = 0;
      for (size_t i = 0; i < full_node_list.size(); i++) { pub_keys_indexes[i] = i; }

      // Shuffle indexes
      uint64_t seed = 0;
      std::memcpy(&seed, block_hash.data, std::min(sizeof(seed), sizeof(block_hash.data)));

      std::mt19937_64 mersenne_twister(seed);
      std::shuffle(pub_keys_indexes.begin(), pub_keys_indexes.end(), mersenne_twister);
    }

    // Assign indexes from shuffled list into quorum and list of nodes to test
    if (!m_quorum_states[height])
      m_quorum_states[height] = std::shared_ptr<quorum_state>(new quorum_state());

    std::shared_ptr<quorum_state> state = m_quorum_states[height];
    state->clear();
    {
      std::vector<crypto::public_key>& quorum = state->quorum_nodes;
      {
        quorum.clear();
        quorum.resize(std::min(full_node_list.size(), QUORUM_SIZE));
        for (size_t i = 0; i < quorum.size(); i++)
        {
          size_t node_index                   = pub_keys_indexes[i];
          const crypto::public_key &key = full_node_list[node_index];
          quorum[i] = key;
        }
      }

      std::vector<crypto::public_key>& nodes_to_test = state->nodes_to_test;
      {
        size_t num_remaining_nodes = pub_keys_indexes.size() - quorum.size();
        size_t num_nodes_to_test   = std::max(num_remaining_nodes/NTH_OF_THE_NETWORK_TO_TEST,
                                              std::min(MIN_NODES_TO_TEST, num_remaining_nodes));

        nodes_to_test.clear();
        nodes_to_test.resize(num_nodes_to_test);

        const int pub_keys_offset = quorum.size();
        for (size_t i = 0; i < nodes_to_test.size(); i++)
        {
          size_t node_index             = pub_keys_indexes[pub_keys_offset + i];
          const crypto::public_key &key = full_node_list[node_index];
          nodes_to_test[i]              = key;
        }
      }
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  service_node_list::rollback_event::rollback_event(uint64_t block_height) : m_block_height(block_height)
  {
  }

  service_node_list::rollback_change::rollback_change(uint64_t block_height, const cryptonote::account_public_address& address, const std::pair<uint64_t, size_t>& height_index, const crypto::public_key& key)
    : service_node_list::rollback_event(block_height), m_address(address), m_height_index(height_index), m_key(key)
  {
  }

  bool service_node_list::rollback_change::apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const
  {
    auto iter = service_nodes_last_reward.find(m_address);
    if (iter == service_nodes_last_reward.end())
    {
      MERROR("Could not find service node address in rollback change");
      return false;
    }
    service_nodes_last_reward[m_address] = m_height_index;
    service_nodes_keys[m_address] = m_key;
    return true;
  }

  service_node_list::rollback_new::rollback_new(uint64_t block_height, cryptonote::account_public_address address)
    : service_node_list::rollback_event(block_height), m_address(address)
  {
  }

  bool service_node_list::rollback_new::apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const
  {
    auto iter = service_nodes_last_reward.find(m_address);
    auto iter2 = service_nodes_keys.find(m_address);
    if (iter == service_nodes_last_reward.end())
    {
      MERROR("Could not find service node address in rollback new");
      return false;
    }
    if (iter2 == service_nodes_keys.end())
    {
      MERROR("Could not find service node pubkey in rollback new");
      return false;
    }
    service_nodes_last_reward.erase(iter);
    service_nodes_keys.erase(iter2);
    return true;
  }

  service_node_list::prevent_rollback::prevent_rollback(uint64_t block_height) : service_node_list::rollback_event(block_height)
  {
  }

  bool service_node_list::prevent_rollback::apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const
  {
    MERROR("Unable to rollback any further!");
    return false;
  }
}
