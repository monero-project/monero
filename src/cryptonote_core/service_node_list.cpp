// Copyright (c)      2018, The Loki Project
// Copyright (c)      2018, Project Triton
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
#include "common/int-util.h"
#include "common/scoped_message_writer.h"
#include "common/i18n.h"
#include "service_node_list.h"
#include "quorum_cop.h"


#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{
  service_node_list::service_node_list(cryptonote::Blockchain& blockchain)
    : m_blockchain(blockchain), m_hooks_registered(false)
  {
  }
  void service_node_list::register_hooks(service_nodes::quorum_cop &quorum_cop)
 {
   if (!m_hooks_registered)
   {
     m_hooks_registered = true;
     m_blockchain.hook_block_added(*this);
     m_blockchain.hook_blockchain_detached(*this);
     m_blockchain.hook_init(*this);
     m_blockchain.hook_validate_miner_tx(*this);

     // NOTE: There is an implicit dependency on service node lists hooks
     m_blockchain.hook_block_added(quorum_cop);
     m_blockchain.hook_blockchain_detached(quorum_cop);
   }

  void service_node_list::init()
  {
    // TODO: Save this calculation, only do it if it's not here.
    LOG_PRINT_L0("Recalculating service nodes list, scanning last 30 days");

    m_service_nodes_infos.clear();


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

  std::vector<crypto::public_key> service_node_list::get_service_nodes_pubkeys() const
  {
    std::vector<crypto::public_key> result;
    for (const auto& iter : m_service_nodes_infos)
    if (iter.second.is_fully_funded())
            result.push_back(iter.first);
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
      // TODO(triton): Not being able to find the quorum is going to be a fatal error.
    }
    else
    {
      result = it->second;
    }

    return result;
  }
  bool service_node_list::is_service_node(const crypto::public_key& pubkey) const
 {
    return m_service_nodes_infos.find(pubkey) != m_service_nodes_infos.end();
 }
 bool service_node_list::contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const
  {
    uint64_t unlock_time = tx.unlock_time;

    if (tx.version >= cryptonote::transaction::version_3_per_output_unlock_times)
      unlock_time = tx.output_unlock_times[i];

    return unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && unlock_time >= block_height + STAKING_REQUIREMENT_LOCK_BLOCKS;
  }

  bool service_node_list::reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint32_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key) const
  {
    cryptonote::tx_extra_service_node_register registration;
    if (!get_service_node_register_from_tx_extra(tx.extra, registration))
      return false;
    if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, service_node_key))
      return false;
   addresses.clear();
   addresses.reserve(registration.m_public_spend_keys.size());
   for (size_t i = 0; i < registration.m_public_spend_keys.size(); i++)
     addresses.push_back(cryptonote::account_public_address{ registration.m_public_spend_keys[i], registration.m_public_view_keys[i] });
   portions = registration.m_portions;
   expiration_timestamp = registration.m_expiration_timestamp;
   signature = registration.m_service_node_signature;
    tx_pub_key = cryptonote::get_tx_pub_key_from_extra(tx.extra);
     return true;
  }

  uint64_t service_node_list::get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev) const
  {
    if (tx.vout[i].target.type() != typeid(cryptonote::txout_to_key))
    {
      return 0;
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
      LOG_PRINT_L0("Unsupported rct type: " << tx.rct_signatures.type);
        return 0;
      }
    }
    catch (const std::exception &e)
    {
      LOG_PRINT_L0("Failed to decode input " << i);
      return 0;
    }

    return money_transferred;
  }
  void service_node_list::process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height)
 {
   if (!tx.is_deregister_tx())
    return;

   cryptonote::tx_extra_service_node_deregister deregister;
   if (!cryptonote::get_service_node_deregister_from_tx_extra(tx.extra, deregister))
   {
     LOG_ERROR("Transaction deregister did not have deregister data in tx extra, possibly corrupt tx in blockchain");
     return ;
   }

   const std::shared_ptr<quorum_state> state = get_quorum_state(deregister.block_height);

   if (!state)
   {
     // TODO(triton): Not being able to find a quorum is fatal! We want better caching abilities.
     LOG_ERROR("Quorum state for height: " << deregister.block_height << ", was not stored by the daemon");
     return ;
   }

   const crypto::public_key& key = state->nodes_to_test[deregister.service_node_index];
   auto iter = m_service_nodes_infos.find(key);
    if (iter == m_service_nodes_infos.end())
      return;

    MGINFO("Deregistration for service node: " << key);

    m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, key, iter->second)));
    m_service_nodes_infos.erase(iter);
 }
  // This function takes a tx and returns true if it is a staking transaction.
  // It also sets the address argument to the public spendkey and pub viewkey of
  // the transaction.
  //
  bool service_node_list::is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const
  {
    crypto::public_key tx_pub_key, service_node_key;
    std::vector<cryptonote::account_public_address> service_node_addresses;
  std::vector<uint32_t> service_node_portions;
  uint64_t expiration_timestamp;
    crypto::signature signature;

    if (!reg_tx_extract_fields(tx, service_node_addresses, service_node_portions, expiration_timestamp, service_node_key, signature, tx_pub_key))
      return false;

      assert(service_node_portions.size() == service_node_addresses.size());


  uint64_t total = 0;
  for (size_t i = 0; i < service_node_portions.size(); i++)
    total += service_node_portions[i];
  if (total > STAKING_PORTIONS)
    return false;

    crypto::hash hash;
   if (!get_registration_hash(service_node_addresses, service_node_portions, expiration_timestamp, hash))
     return false;
   if (!crypto::check_signature(hash, service_node_key, signature))
     return false;
   if (expiration_timestamp < block_timestamp)
      return false;

      if (service_node_addresses.size() > MAX_NUMBER_OF_SHAREHOLDERS)
         return false;

       key = service_node_key;

       info.last_reward_block_height = block_height;
       info.last_reward_transaction_index = index;
       info.addresses = service_node_addresses;
       info.portions = service_node_portions;
       info.total_contributions = 0;
       info.staking_requirement = m_blockchain.get_staking_requirement(block_height);

       return true;
     }

     void service_node_list::process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index)
     {
       crypto::public_key key;
       service_node_info info;
       if (!is_registration_tx(tx, block_timestamp, block_height, index, key, info))
         return;

       auto iter = m_service_nodes_infos.find(key);
       if (iter != m_service_nodes_infos.end())
         return;

       MGINFO("New service node registered: " << key);

       m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_new(block_height, key)));
       m_service_nodes_infos[key] = info;
     }

     void service_node_list::process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index)
     {
       // TODO: move unlock time check from here to below, when unlock time is done per output.
       crypto::public_key pubkey;
       cryptonote::account_public_address address;

       if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, pubkey))
         return;
       if (!cryptonote::get_service_node_contributor_from_tx_extra(tx.extra, address))
         return;

       auto iter = m_service_nodes_infos.find(pubkey);
       if (iter == m_service_nodes_infos.end())
         return;

       if (iter->second.is_fully_funded())
         return;



    crypto::key_derivation derivation;

      uint64_t transferred = 0;
      for (size_t i = 0; i < tx.vout.size(); i++)
        if (contribution_tx_output_has_correct_unlock_time(tx, i, block_height))
          transferred += get_reg_tx_staking_output_contribution(tx, i, derivation, hwdev);

    if (transferred < get_min_contribution(iter->second.staking_requirement))
      return;

   m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, pubkey, iter->second)));
   iter->second.contributions.push_back(service_node_info::contribution{ transferred, address });
   iter->second.total_contributions += transferred;
   iter->second.last_reward_block_height = block_height;
   iter->second.last_reward_transaction_index = index;

   MGINFO("Contribution of " << transferred << " received for service node " << pubkey);
   return;
  }
  void service_node_list::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
 {
   block_added_generic(block, txs);
 }
  template<typename T>
  void service_node_list::block_added_generic(const cryptonote::block& block, const T& txs)
  {
    uint64_t block_height = cryptonote::get_block_height(block);
    int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

    if (hard_fork_version < 5)
      return;

    while (!m_rollback_events.empty() && m_rollback_events.front()->m_block_height < block_height - ROLLBACK_EVENT_EXPIRATION_BLOCKS)
    {
      m_rollback_events.pop_front();
    }
    for (const crypto::public_key& pubkey : get_expired_nodes(block_height))
    {
      auto i = m_service_nodes_infos.find(pubkey);
      if (i != m_service_nodes_infos.end())
      {
        m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, pubkey, i->second)));
        m_service_nodes_infos.erase(i);
      }
      // Service nodes may expire early if they double staked by accident, so
      // expiration doesn't mean the node is in the list.
    }
    crypto::public_key winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(block.miner_tx.extra);
    if (m_service_nodes_infos.count(winner_pubkey) == 1)
    {
      m_rollback_events.push_back(
        std::unique_ptr<rollback_event>(
          new rollback_change(block_height, winner_pubkey, m_service_nodes_infos[winner_pubkey])
        )
      );
      // set the winner as though it was re-registering at transaction index=-1 for this block
      m_service_nodes_infos[winner_pubkey].last_reward_block_height = block_height;
      m_service_nodes_infos[winner_pubkey].last_reward_transaction_index = UINT32_MAX;
    }

    

    uint32_t index = 0;
    for (const cryptonote::transaction& tx : txs)
    {
      crypto::public_key key;
      service_node_info info;
      cryptonote::account_public_address address;
     process_registration_tx(tx, block.timestamp, block_height, index);
     process_contribution_tx(tx, block_height, index);
     process_deregistration_tx(tx, block_height);
      index++;
    }
    const uint64_t curr_height           = m_blockchain.get_current_blockchain_height();

   const size_t QUORUM_LIFETIME         = triton::service_node_deregister::DEREGISTER_LIFETIME_BY_HEIGHT;
   const size_t cache_state_from_height = (curr_height < QUORUM_LIFETIME) ? 0 : curr_height - QUORUM_LIFETIME;

   if (block_height >= cache_state_from_height)
   {
     store_quorum_state_from_rewards_list(block_height);

     while (!m_quorum_states.empty() && m_quorum_states.begin()->first < cache_state_from_height)
     {
       m_quorum_states.erase(m_quorum_states.begin());
     }
   }

    // store in the db service node state for height block_height;
  }

  void service_node_list::blockchain_detached(uint64_t height)
  {
    while (!m_rollback_events.empty() && m_rollback_events.back()->m_block_height >= height)
    {
      if (!m_rollback_events.back()->apply(m_service_nodes_infos))
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

  std::vector<crypto::public_key> service_node_list::get_expired_nodes(uint64_t block_height) const
  {
    std::vector<crypto::public_key> expired_nodes;

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
    uint32_t index = 0;
    for (const cryptonote::transaction& tx : txs)
    {
      crypto::public_key key;
        service_node_info info;
        if (is_registration_tx(tx, block.timestamp, expired_nodes_block_height, index, key, info))
      {
        expired_nodes.push_back(key);
      }
      index++;
    }

    return expired_nodes;
  }
  uint64_t service_node_list::get_min_contribution(uint64_t staking_requirement) const
  {
    return staking_requirement / MAX_NUMBER_OF_CONTRIBUTORS;
  }
  std::vector<std::pair<cryptonote::account_public_address, uint32_t>> service_node_list::get_winner_addresses_and_portions(const crypto::hash& prev_id) const
  {
    crypto::public_key key = select_winner(prev_id);
    if (key == crypto::null_pkey)
      return { std::make_pair(null_address, STAKING_PORTIONS) };
    std::vector<std::pair<cryptonote::account_public_address, uint32_t>> winners;
    uint64_t remaining_portions = STAKING_PORTIONS;
   auto add_to_winners = [&winners](cryptonote::account_public_address address, uint64_t amount_of_portions) {
     if (amount_of_portions == 0) return;
     auto iter = std::find_if(std::begin(winners), std::end(winners),
         [&address](const std::pair<cryptonote::account_public_address, uint32_t>& winner) {
           return winner.first == address;
         });
     if (iter == winners.end())
       winners.push_back(std::make_pair(address, amount_of_portions));
     else
       iter->second += amount_of_portions;
   };
    for (size_t i = 0; i < m_service_nodes_infos.at(key).addresses.size(); i++)
    {
   add_to_winners(m_service_nodes_infos.at(key).addresses[i], m_service_nodes_infos.at(key).portions[i]);
   remaining_portions -= m_service_nodes_infos.at(key).portions[i];
 }
 uint64_t remaining_staking_requirement = m_service_nodes_infos.at(key).staking_requirement;
 for (const auto& contribution : m_service_nodes_infos.at(key).contributions)
 {
   uint64_t actual_contribution = std::min(remaining_staking_requirement, contribution.amount);
   remaining_staking_requirement -= actual_contribution;

   uint64_t hi, lo, resulthi, resultlo;
   lo = mul128(actual_contribution, remaining_portions, &hi);
   div128_64(hi, lo, m_service_nodes_infos.at(key).staking_requirement, &resulthi, &resultlo);

   add_to_winners(contribution.address, resultlo);
 }
    return winners;
  }
  cryptonote::account_public_address service_node_list::select_winner(const crypto::hash& prev_id)
  {
    auto oldest_waiting = std::pair<uint64_t, uint32_t>(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint32_t>::max());
     crypto::public_key key = crypto::null_pkey;
     for (const auto& info : m_service_nodes_infos)
      if (info.second.is_fully_funded())
      {
        auto waiting_since = std::make_pair(info.second.last_reward_block_height, info.second.last_reward_transaction_index);
       if (waiting_since < oldest_waiting)
       {
         oldest_waiting = waiting_since;
         key = info.first;
       }
      }
    return key;
  }

  /// validates the miner TX for the next block
  //
  bool service_node_list::validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward)
  {
    if (hard_fork_version < 5)
      return true;

      uint64_t total_service_node_reward = cryptonote::get_service_node_reward(height, base_reward, hard_fork_version);

      crypto::public_key winner = select_winner(prev_id);

      crypto::public_key check_winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(miner_tx.extra);
      if (check_winner_pubkey != winner)
        return false;

        const std::vector<std::pair<cryptonote::account_public_address, uint32_t>> addresses_and_portions = get_winner_addresses_and_portions(prev_id);

     if (miner_tx.vout.size() - 1 < addresses_and_portions.size())
      return false;
      for (size_t i = 0; i < addresses_and_portions.size(); i++)
     {

       size_t vout_index = miner_tx.vout.size() - 1 /* governance */ - addresses_and_portions.size() + i;

       uint64_t reward = cryptonote::get_share_of_reward(addresses_and_portions[i].second, total_service_node_reward);


       if (miner_tx.vout[vout_index].amount != reward)
         {
           MERROR("Service node reward amount incorrect. Should be " << cryptonote::print_money(reward) << ", is: " << cryptonote::print_money(miner_tx.vout[vout_index].amount));
           return false;
         }

         if (miner_tx.vout[vout_index].target.type() != typeid(cryptonote::txout_to_key))
         {
           MERROR("Service node output target type should be txout_to_key");
           return false;
         }
         crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);;
     crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
     cryptonote::keypair gov_key = cryptonote::get_deterministic_keypair_from_height(height);

     bool r = crypto::generate_key_derivation(addresses_and_portions[i].first.m_view_public_key, gov_key.sec, derivation);
     CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << addresses_and_portions[i].first.m_view_public_key << ", " << gov_key.sec << ")");
     r = crypto::derive_public_key(derivation, vout_index, addresses_and_portions[i].first.m_spend_public_key, out_eph_public_key);
     CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << vout_index << ", "<< addresses_and_portions[i].first.m_spend_public_key << ")");

     if (boost::get<cryptonote::txout_to_key>(miner_tx.vout[vout_index].target).key != out_eph_public_key)
     {
       MERROR("Invalid service node reward output");
       return false;
     }
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

   std::vector<crypto::public_key> full_node_list = get_service_nodes_pubkeys();
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

  service_node_list::rollback_change::rollback_change(uint64_t block_height, const crypto::public_key& key, const service_node_info& info)
    : service_node_list::rollback_event(block_height), m_key(key), m_info(info)
  {
  }

  bool service_node_list::rollback_change::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
  {
    service_nodes_infos[m_key] = m_info;
    return true;
  }

  service_node_list::rollback_new::rollback_new(uint64_t block_height, const crypto::public_key& key)
    : service_node_list::rollback_event(block_height), m_key(key)
  {
  }

  bool service_node_list::rollback_new::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
  {
    auto iter = service_nodes_infos.find(m_key);
    if (iter == service_nodes_infos.end())
   {
     MERROR("Could not find service node pubkey in rollback new");
     return false;
   }
   service_nodes_infos.erase(iter);
    return true;
  }

  service_node_list::prevent_rollback::prevent_rollback(uint64_t block_height) : service_node_list::rollback_event(block_height)
  {
  }

  bool service_node_list::prevent_rollback::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
  {
    MERROR("Unable to rollback any further!");
    return false;
  }

  bool convert_registration_args(cryptonote::network_type nettype, const std::vector<std::string>& args, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint32_t>& portions, uint64_t& initial_contribution)
  {
    if (args.size() % 2 == 0)
    {
      MERROR(tr("Expected an odd number of arguments: [<address> <fraction> [<address> <fraction> [...]]] <initial contribution>"));
     return false;
   }
   if (args.size() < 1)
   {
     MERROR(tr("Missing <initial contribution>"));
     return false;
   }
   if ((args.size()-1)/ 2 > MAX_NUMBER_OF_CONTRIBUTORS)
   {
     MERROR(tr("Exceeds the maximum number of contributors, which is ") << MAX_NUMBER_OF_CONTRIBUTORS);
      return false;
    }
    addresses.clear();
    portions.clear();
    uint64_t total_portions = 0;
    for (size_t i = 0; i < args.size()-1; i += 2)
    {
      cryptonote::address_parse_info info;
      if (!cryptonote::get_account_address_from_str(info, nettype, args[i]))
      {
        MERROR(tr("failed to parse address"));
        return false;
      }

      if (info.has_payment_id)
      {
        MERROR(tr("can't use a payment id for staking tx"));
        return false;
      }
      if (info.is_subaddress)
      {
        MERROR(tr("can't use a subaddress for staking tx"));
        return false;
      }

      addresses.push_back(info.address);

      try
      {
        double share_fraction = boost::lexical_cast<double>(args[i+1]);
        if (share_fraction <= 0 || share_fraction > 1)
        {
          MERROR(tr("Invalid share amount: ") << args[i+1] << tr(". ") << tr("Must be more than 0 and no greater than 1"));
          return false;
        }
        uint32_t num_portions = STAKING_PORTIONS * share_fraction;
        portions.push_back(num_portions);
        total_portions += num_portions;
      }
      catch (const std::exception &e)
      {
        MERROR(tr("Invalid share amount: ") << args[i+1] << tr(". ") << tr("Must be more than 0 and no greater than 1"));
        return false;
      }
    }
    if (!cryptonote::parse_amount(initial_contribution, args.back()) || initial_contribution == 0)
   {
     MERROR(tr("amount is wrong: ") << args.back() <<
       ", " << tr("expected number from ") << cryptonote::print_money(1) <<
       " to " << cryptonote::print_money(std::numeric_limits<uint64_t>::max()));
     return true;
   }
    if (total_portions > (uint64_t)STAKING_PORTIONS)
    {
      MERROR(tr("Invalid portion amounts, portions must sum to at most 1."));
      MERROR(tr("If it looks correct,  this may be because of rounding. Try reducing one of the portionholders portions by a very tiny amount"));
      return false;
    }
    return true;
  }
  bool make_registration_cmd(cryptonote::network_type nettype, const std::vector<std::string> args, const crypto::public_key& service_node_pubkey,
                            const crypto::secret_key service_node_key, std::string &cmd, bool make_friendly)
 {

   std::vector<cryptonote::account_public_address> addresses;
   std::vector<uint32_t> shares;
   uint64_t initial_contribution;
    if (!convert_registration_args(nettype, args, addresses, shares, initial_contribution))
   {
     MERROR(tr("Could not convert registration args"));
     return false;
   }

   uint64_t exp_timestamp = time(nullptr) + STAKING_AUTHORIZATION_EXPIRATION_WINDOW;

   crypto::hash hash;
   bool hashed = cryptonote::get_registration_hash(addresses, shares, exp_timestamp, hash);
   if (!hashed)
   {
     MERROR(tr("Could not make registration hash from addresses and shares"));
     return false;
   }

   crypto::signature signature;
   crypto::generate_signature(hash, service_node_pubkey, service_node_key, signature);

   std::stringstream stream;
   if (make_friendly)
   {
     stream << tr("Run this command in the wallet that will fund this registration:\n\n");
   }

   stream << "register_service_node";
   for (size_t i = 0; i < args.size(); ++i)
   {
     stream << " " << args[i];
   }

   stream << " " << exp_timestamp << " ";
   stream << epee::string_tools::pod_to_hex(service_node_pubkey) << " ";
   stream << epee::string_tools::pod_to_hex(signature);


   if (make_friendly)
   {
     stream << "\n\n";
     time_t tt = exp_timestamp;
     struct tm tm;
#ifdef WIN32
     gmtime_s(&tm, &tt);
#else
     gmtime_r(&tt, &tm);
#endif

     char buffer[128];
     strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M:%S %p", &tm);
     stream << tr("This registration expires at ") << buffer << tr(". This should be in about 2 weeks. If it isn't, check this computer's clock") << std::endl;
     stream << tr("Please submit your registration into the blockchain before this time or it will be invalid.");
   }

   cmd = stream.str();
   return true;
 }
}
