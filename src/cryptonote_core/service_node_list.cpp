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
#include <algorithm>
#include <chrono>

#include <boost/endian/conversion.hpp>

#include "ringct/rctSigs.h"
#include "wallet/wallet2.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "int-util.h"
#include "common/scoped_message_writer.h"
#include "common/i18n.h"
#include "common/util.h"
#include "blockchain.h"
#include "service_node_quorum_cop.h"

#include "service_node_list.h"
#include "service_node_rules.h"
#include "service_node_swarm.h"
#include "version.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{
  size_t constexpr MAX_SHORT_TERM_STATE_HISTORY   = 6 * STATE_CHANGE_TX_LIFETIME_IN_BLOCKS;
  size_t constexpr STORE_LONG_TERM_STATE_INTERVAL = 10000;

  static int get_min_service_node_info_version_for_hf(uint8_t hf_version)
  {
    return service_node_info::version_0_checkpointing; // Versioning reset with the full SN rescan in 4.0.0
  }

  service_node_list::service_node_list(cryptonote::Blockchain& blockchain)
    : m_blockchain(blockchain), m_db(nullptr), m_service_node_pubkey(nullptr), m_store_quorum_history(0) { }

  void service_node_list::rescan_starting_from_curr_state(bool store_to_disk)
  {
    if (m_blockchain.get_current_hard_fork_version() < 9)
      return;

    auto scan_start         = std::chrono::high_resolution_clock::now();
    uint64_t current_height = m_blockchain.get_current_blockchain_height();
    if (m_state.height == current_height)
      return;

    MGINFO("Recalculating service nodes list, scanning blockchain from height: " << m_state.height << " to: " << current_height);
    std::vector<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
    std::vector<cryptonote::transaction> txs;
    std::vector<crypto::hash> missed_txs;
    auto work_start = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; m_state.height < current_height; i++)
    {
      if (i > 0 && i % 10 == 0)
      {
        if (store_to_disk) store();
        auto work_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(work_end - work_start);
        MGINFO("... scanning height " << m_state.height << " (" << duration.count() / 1000.f << "s)");
        work_start = std::chrono::high_resolution_clock::now();
      }

      blocks.clear();
      if (!m_blockchain.get_blocks(m_state.height, 1000, blocks))
      {
        MERROR("Unable to initialize service nodes list");
        return;
      }
      if (blocks.empty())
        break;

      for (const auto& block_pair : blocks)
      {
        txs.clear();
        missed_txs.clear();

        const cryptonote::block& block = block_pair.second;
        if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
        {
          MERROR("Unable to get transactions for block " << block.hash);
          return;
        }

        process_block(block, txs);
      }
    }

    auto scan_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(scan_end - scan_start);
    MGINFO("Done recalculating service nodes list (" << duration.count() / 1000.f << "s)");
    if (store_to_disk) store();
  }

  void service_node_list::init()
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    if (m_blockchain.get_current_hard_fork_version() < 9)
    {
      reset(true);
      return;
    }

    uint64_t current_height = m_blockchain.get_current_blockchain_height();
    bool loaded = load(current_height);
    if (loaded && m_old_quorum_states.size() < std::min(m_store_quorum_history, uint64_t{10})) {
      LOG_PRINT_L0("Full history storage requested, but " << m_old_quorum_states.size() << " old quorum states found");
      loaded = false; // Either we don't have stored history or the history is very short, so recalculation is necessary or cheap.
    }

    bool store_to_disk = false;
    if (!loaded || m_state.height > current_height)
    {
      reset(true);
      store_to_disk = true;
    }

    rescan_starting_from_curr_state(store_to_disk);
  }

  template <typename UnaryPredicate>
  static std::vector<service_nodes::pubkey_and_sninfo> sort_and_filter(const service_nodes_infos_t &sns_infos, UnaryPredicate p, bool reserve = true) {
    std::vector<pubkey_and_sninfo> result;
    if (reserve) result.reserve(sns_infos.size());
    for (const pubkey_and_sninfo &key_info : sns_infos)
      if (p(*key_info.second))
        result.push_back(key_info);

    std::sort(result.begin(), result.end(),
      [](const pubkey_and_sninfo &a, const pubkey_and_sninfo &b) {
        return memcmp(reinterpret_cast<const void*>(&a), reinterpret_cast<const void*>(&b), sizeof(a)) < 0;
      });
    return result;
  }

  std::vector<pubkey_and_sninfo> service_node_list::state_t::active_service_nodes_infos() const {
    return sort_and_filter(service_nodes_infos, [](const service_node_info &info) { return info.is_active(); });
  }

  std::vector<pubkey_and_sninfo> service_node_list::state_t::decommissioned_service_nodes_infos() const {
    return sort_and_filter(service_nodes_infos, [](const service_node_info &info) { return info.is_decommissioned() && info.is_fully_funded(); }, /*reserve=*/ false);
  }

  quorum_manager service_node_list::get_quorum_manager(uint64_t height, bool include_old) const
  {
    quorum_manager const *result_ptr = nullptr;

    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    if (height == m_state.height)
      result_ptr = &m_state.quorums;
    else // NOTE: Search m_state_history
    {
      auto it = m_state_history.find(height);
      if (it != m_state_history.end())
        result_ptr = &it->quorums;
    }

    if (!result_ptr && include_old) // NOTE: Search m_old_quorum_states
    {
      auto it =
          std::lower_bound(m_old_quorum_states.begin(),
                           m_old_quorum_states.end(),
                           height,
                           [](quorums_by_height const &entry, uint64_t height) { return entry.height < height; });

      if (it != m_old_quorum_states.end() && it->height == height)
        result_ptr = &it->quorums;
    }

    if (!quorums)
        return nullptr;

    quorum_manager quorums = get_quorum_manager(height, include_old);
    if (type == quorum_type::obligations)
      return quorums.obligations;
    else if (type == quorum_type::checkpointing)
      return quorums.checkpointing;

    MERROR("Developer error: Unhandled quorum enum with value: " << (size_t)type);
    assert(!"Developer error: Unhandled quorum enum");
    return nullptr;
  }

  static bool get_quorum_pubkey_from_quorum(testing_quorum const &quorum, quorum_group group, size_t quorum_index, crypto::public_key &key)
  {
    std::vector<crypto::public_key> const *array = nullptr;
    if      (group == quorum_group::validator) array = &quorum.validators;
    else if (group == quorum_group::worker)    array = &quorum.workers;
    else
    {
      MERROR("Invalid quorum group specified");
      return false;
    }

    if (quorum_index >= array->size())
    {
      MERROR("Quorum indexing out of bounds: " << quorum_index << ", quorum_size: " << array->size());
      return false;
    }

    key = (*array)[quorum_index];
    return true;
  }

  bool service_node_list::get_quorum_pubkey(quorum_type type, quorum_group group, uint64_t height, size_t quorum_index, crypto::public_key &key) const
  {
    std::shared_ptr<const testing_quorum> quorum = get_testing_quorum(type, height);
    if (!quorum)
    {
      LOG_PRINT_L1("Quorum for height: " << height << ", was not stored by the daemon");
      return false;
    }

    bool result = get_quorum_pubkey_from_quorum(*quorum, group, quorum_index, key);
    return result;
  }

  std::vector<service_node_pubkey_info> service_node_list::get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    std::vector<service_node_pubkey_info> result;

    if (service_node_pubkeys.empty())
    {
      result.reserve(m_state.service_nodes_infos.size());

      for (const auto &info : m_state.service_nodes_infos)
        result.emplace_back(info);
    }
    else
    {
      result.reserve(service_node_pubkeys.size());
      for (const auto &it : service_node_pubkeys)
      {
        auto find_it = m_state.service_nodes_infos.find(it);
        if (find_it != m_state.service_nodes_infos.end())
          result.emplace_back(*find_it);
      }
    }

    return result;
  }

  void service_node_list::set_db_pointer(cryptonote::BlockchainDB* db)
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    m_db = db;
  }

  void service_node_list::set_my_service_node_keys(crypto::public_key const *pub_key)
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    m_service_node_pubkey = pub_key;
  }

  void service_node_list::set_quorum_history_storage(uint64_t hist_size) {
    if (hist_size == 1)
      hist_size = std::numeric_limits<uint64_t>::max();
    m_store_quorum_history = hist_size;
  }

  bool service_node_list::is_service_node(const crypto::public_key& pubkey, bool require_active) const
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    auto it = m_state.service_nodes_infos.find(pubkey);
    return it != m_state.service_nodes_infos.end() && (!require_active || it->second->is_active());
  }

  bool service_node_list::is_key_image_locked(crypto::key_image const &check_image, uint64_t *unlock_height, service_node_info::contribution_t *the_locked_contribution) const
  {
    for (const auto& pubkey_info : m_state.service_nodes_infos)
    {
      const service_node_info &info = *pubkey_info.second;
      for (const service_node_info::contributor_t &contributor : info.contributors)
      {
        for (const service_node_info::contribution_t &contribution : contributor.locked_contributions)
        {
          if (check_image == contribution.key_image)
          {
            if (the_locked_contribution) *the_locked_contribution = contribution;
            if (unlock_height) *unlock_height = info.requested_unlock_height;
            return true;
          }
        }
      }
    }
    return false;
  }

  bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature)
  {
    cryptonote::tx_extra_service_node_register registration;
    if (!get_service_node_register_from_tx_extra(tx.extra, registration))
      return false;
    if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, service_node_key))
      return false;

    addresses.clear();
    addresses.reserve(registration.m_public_spend_keys.size());
    for (size_t i = 0; i < registration.m_public_spend_keys.size(); i++) {
      addresses.emplace_back();
      addresses.back().m_spend_public_key = registration.m_public_spend_keys[i];
      addresses.back().m_view_public_key = registration.m_public_view_keys[i];
    }

    portions_for_operator = registration.m_portions_for_operator;
    portions = registration.m_portions;
    expiration_timestamp = registration.m_expiration_timestamp;
    signature = registration.m_service_node_signature;
    return true;
  }

  struct parsed_tx_contribution
  {
    cryptonote::account_public_address address;
    uint64_t transferred;
    crypto::secret_key tx_key;
    std::vector<service_node_info::contribution_t> locked_contributions;
  };

  static uint64_t get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation const &derivation, hw::device& hwdev)
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
      case rct::RCTTypeBulletproof:
      case rct::RCTTypeBulletproof2:
        money_transferred = rct::decodeRctSimple(tx.rct_signatures, rct::sk2rct(scalar1), i, mask, hwdev);
        break;
      case rct::RCTTypeFull:
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

  /// Makes a copy of the given service_node_info and replaces the shared_ptr with a pointer to the copy.
  /// Returns the non-const service_node_info (which is now held by the passed-in shared_ptr lvalue ref).
  static service_node_info &duplicate_info(std::shared_ptr<const service_node_info> &info_ptr) {
    auto new_ptr = std::make_shared<service_node_info>(*info_ptr);
    info_ptr = new_ptr;
    return *new_ptr;
  }

  bool service_node_list::process_state_change_tx(const cryptonote::transaction& tx, uint64_t block_height)
  {
    if (tx.type != cryptonote::txtype::state_change)
      return false;

    uint8_t hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

    cryptonote::tx_extra_service_node_state_change state_change;
    if (!cryptonote::get_service_node_state_change_from_tx_extra(tx.extra, state_change, hard_fork_version))
    {
      MERROR("Transaction did not have valid state change data in tx extra, possibly corrupt tx in blockchain");
      return false;
    }

    crypto::public_key key;
    quorum_manager quorums = get_quorum_manager(state_change.block_height);
    if (!quorums.obligations || !get_quorum_pubkey_from_quorum(*quorums.obligations, quorum_group::worker, state_change.service_node_index, key))
      return false;

    auto iter = m_state.service_nodes_infos.find(key);
    if (iter == m_state.service_nodes_infos.end()) {
      LOG_PRINT_L2("Received state change tx for non-registered service node " << key << " (perhaps a delayed tx?)");
      return false;
    }

    auto &info = duplicate_info(iter->second);
    bool is_me = m_service_node_pubkey && *m_service_node_pubkey == key;

    switch (state_change.state) {
      case new_state::deregister:
        if (is_me)
          MGINFO_RED("Deregistration for service node (yours): " << key);
        else
          LOG_PRINT_L1("Deregistration for service node: " << key);

        if (hard_fork_version >= cryptonote::network_version_11_infinite_staking)
        {
          for (const auto &contributor : info.contributors)
          {
            for (const auto &contribution : contributor.locked_contributions)
            {
              m_state.key_image_blacklist.emplace_back(
                  get_min_service_node_info_version_for_hf(hard_fork_version),
                  contribution.key_image,
                  block_height + staking_num_lock_blocks(m_blockchain.nettype()));
            }
          }
        }

        m_state.service_nodes_infos.erase(iter);
        return true;

      case new_state::decommission:
        if (hard_fork_version < cryptonote::network_version_12_checkpointing) {
          MERROR("Invalid decommission transaction seen before network v12");
          return false;
        }

        if (info.is_decommissioned()) {
          LOG_PRINT_L2("Received decommission tx for already-decommissioned service node " << key << "; ignoring");
          return false;
        }

        if (is_me)
          MGINFO_RED("Temporary decommission for service node (yours): " << key);
        else
          LOG_PRINT_L1("Temporary decommission for service node: " << key);

        info.active_since_height = -info.active_since_height;
        info.last_decommission_height = block_height;
        info.decommission_count++;

        info.proof->timestamp = 0;
        return true;

      case new_state::recommission:
        if (hard_fork_version < cryptonote::network_version_12_checkpointing) {
          MERROR("Invalid recommission transaction seen before network v12");
          return false;
        }

        if (!info.is_decommissioned()) {
          LOG_PRINT_L2("Received recommission tx for already-active service node " << key << "; ignoring");
          return false;
        }

        if (is_me)
          MGINFO_GREEN("Recommission for service node (yours): " << key);
        else
          LOG_PRINT_L1("Recommission for service node: " << key);


        info.active_since_height = block_height;
        // Move the SN at the back of the list as if it had just registered (or just won)
        info.last_reward_block_height = block_height;
        info.last_reward_transaction_index = std::numeric_limits<uint32_t>::max();

        // NOTE: Only the quorum deciding on this node agrees that the service
        // node has a recent uptime atleast for it to be recommissioned not
        // necessarily the entire network. Ensure the entire network agrees
        // simultaneously they are online if we are recommissioning by resetting
        // the failure conditions.
        info.proof->timestamp = time(nullptr);
        info.proof->votes.fill(true);
        return true;

      case new_state::ip_change_penalty:
        if (hard_fork_version < cryptonote::network_version_12_checkpointing) {
          MERROR("Invalid ip_change_penalty transaction seen before network v12");
          return false;
        }

        if (info.is_decommissioned()) {
          LOG_PRINT_L2("Received reset position tx for service node " << key << " but it is already decommissioned; ignoring");
          return false;
        }

        if (is_me)
          MGINFO_RED("Reward position reset for service node (yours): " << key);
        else
          LOG_PRINT_L1("Reward position reset for service node: " << key);


        // Move the SN at the back of the list as if it had just registered (or just won)
        info.last_reward_block_height = block_height;
        info.last_reward_transaction_index = std::numeric_limits<uint32_t>::max();
        info.last_ip_change_height = block_height;
        return true;

      default:
        // dev bug!
        MERROR("BUG: Service node state change tx has unknown state " << static_cast<uint16_t>(state_change.state));
        return false;
    }
  }

  void service_node_list::update_swarms(uint64_t height) {

    crypto::hash hash = m_blockchain.get_block_id_by_height(height);
    uint64_t seed = 0;
    std::memcpy(&seed, hash.data, sizeof(seed));

    /// Gather existing swarms from infos
    swarm_snode_map_t existing_swarms;

    for (const auto &key_info : m_state.active_service_nodes_infos())
      existing_swarms[key_info.second->swarm_id].push_back(key_info.first);

    calc_swarm_changes(existing_swarms, seed);

    /// Apply changes
    for (const auto entry : existing_swarms) {

      const swarm_id_t swarm_id = entry.first;
      const std::vector<crypto::public_key>& snodes = entry.second;

      for (const auto &snode : snodes) {

        auto &sn_info_ptr = m_state.service_nodes_infos.at(snode);
        if (sn_info_ptr->swarm_id == swarm_id) continue; /// nothing changed for this snode
        duplicate_info(sn_info_ptr).swarm_id = swarm_id;
      }
    }
  }

  static bool get_contribution(cryptonote::network_type nettype, int hard_fork_version, const cryptonote::transaction& tx, uint64_t block_height, parsed_tx_contribution &parsed_contribution)
  {
    if (!cryptonote::get_service_node_contributor_from_tx_extra(tx.extra, parsed_contribution.address))
      return false;

    if (!cryptonote::get_tx_secret_key_from_tx_extra(tx.extra, parsed_contribution.tx_key))
    {
      LOG_PRINT_L1("Contribution TX: There was a service node contributor but no secret key in the tx extra on height: " << block_height << " for tx: " << get_transaction_hash(tx));
      return false;
    }

    crypto::key_derivation derivation;
    if (!crypto::generate_key_derivation(parsed_contribution.address.m_view_public_key, parsed_contribution.tx_key, derivation))
    {
      LOG_PRINT_L1("Contribution TX: Failed to generate key derivation on height: " << block_height << " for tx: " << get_transaction_hash(tx));
      return false;
    }

    hw::device& hwdev               = hw::get_device("default");
    parsed_contribution.transferred = 0;

    if (hard_fork_version >= cryptonote::network_version_11_infinite_staking)
    {
      cryptonote::tx_extra_tx_key_image_proofs key_image_proofs;
      if (!get_tx_key_image_proofs_from_tx_extra(tx.extra, key_image_proofs))
      {
        LOG_PRINT_L1("Contribution TX: Didn't have key image proofs in the tx_extra, rejected on height: " << block_height << " for tx: " << get_transaction_hash(tx));
        return false;
      }

      for (size_t output_index = 0; output_index < tx.vout.size(); ++output_index)
      {
        uint64_t transferred = get_reg_tx_staking_output_contribution(tx, output_index, derivation, hwdev);
        if (transferred == 0)
          continue;

        crypto::public_key ephemeral_pub_key;
        {
          if (!hwdev.derive_public_key(derivation, output_index, parsed_contribution.address.m_spend_public_key, ephemeral_pub_key))
          {
            LOG_PRINT_L1("Contribution TX: Could not derive TX ephemeral key on height: " << block_height << " for tx: " << get_transaction_hash(tx) << " for output: " << output_index);
            continue;
          }

          const auto& out_to_key = boost::get<cryptonote::txout_to_key>(tx.vout[output_index].target);
          if (out_to_key.key != ephemeral_pub_key)
          {
            LOG_PRINT_L1("Contribution TX: Derived TX ephemeral key did not match tx stored key on height: " << block_height << " for tx: " << get_transaction_hash(tx) << " for output: " << output_index);
            continue;
          }
        }

        crypto::public_key const *ephemeral_pub_key_ptr = &ephemeral_pub_key;
        for (auto proof = key_image_proofs.proofs.begin(); proof != key_image_proofs.proofs.end(); proof++)
        {
          if (!crypto::check_ring_signature((const crypto::hash &)(proof->key_image), proof->key_image, &ephemeral_pub_key_ptr, 1, &proof->signature))
            continue;

          parsed_contribution.locked_contributions.emplace_back(
              service_node_info::version_0_checkpointing,
              ephemeral_pub_key,
              proof->key_image,
              transferred
          );

          parsed_contribution.transferred += transferred;
          key_image_proofs.proofs.erase(proof);
          break;
        }
      }
    }
    else
    {
      for (size_t i = 0; i < tx.vout.size(); i++)
      {
        bool has_correct_unlock_time = false;
        {
          uint64_t unlock_time = tx.unlock_time;
          if (tx.version >= cryptonote::txversion::v3_per_output_unlock_times)
            unlock_time = tx.output_unlock_times[i];

          uint64_t min_height = block_height + staking_num_lock_blocks(nettype);
          has_correct_unlock_time = unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && unlock_time >= min_height;
        }

        if (has_correct_unlock_time)
          parsed_contribution.transferred += get_reg_tx_staking_output_contribution(tx, i, derivation, hwdev);
      }
    }

    return true;
  }

  bool service_node_list::is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const
  {
    crypto::public_key service_node_key;
    std::vector<cryptonote::account_public_address> service_node_addresses;
    std::vector<uint64_t> service_node_portions;
    uint64_t portions_for_operator;
    uint64_t expiration_timestamp;
    crypto::signature signature;

    if (!reg_tx_extract_fields(tx, service_node_addresses, portions_for_operator, service_node_portions, expiration_timestamp, service_node_key, signature))
      return false;

    if (service_node_portions.size() != service_node_addresses.size() || service_node_portions.empty())
    {
      LOG_PRINT_L1("Register TX: Extracted portions size: (" << service_node_portions.size() <<
                   ") was empty or did not match address size: (" << service_node_addresses.size() <<
                   ") on height: " << block_height <<
                   " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    uint8_t hf_version = m_blockchain.get_hard_fork_version(block_height);

    if (!check_service_node_portions(hf_version, service_node_portions)) return false;

    if (portions_for_operator > STAKING_PORTIONS)
    {
      LOG_PRINT_L1("Register TX: Operator portions: " << portions_for_operator <<
                   " exceeded staking portions: " << STAKING_PORTIONS <<
                   " on height: " << block_height <<
                   " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    // check the signature is all good

    crypto::hash hash;
    if (!get_registration_hash(service_node_addresses, portions_for_operator, service_node_portions, expiration_timestamp, hash))
    {
      LOG_PRINT_L1("Register TX: Failed to extract registration hash, on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    if (!crypto::check_key(service_node_key) || !crypto::check_signature(hash, service_node_key, signature))
    {
      LOG_PRINT_L1("Register TX: Has invalid key and/or signature, on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    if (expiration_timestamp < block_timestamp)
    {
      LOG_PRINT_L1("Register TX: Has expired. The block timestamp: " << block_timestamp <<
                   " is greater than the expiration timestamp: " << expiration_timestamp <<
                   " on height: " << block_height <<
                   " for tx:" << cryptonote::get_transaction_hash(tx));
      return false;
    }

    // check the initial contribution exists

    info.staking_requirement = get_staking_requirement(m_blockchain.nettype(), block_height, hf_version);

    cryptonote::account_public_address address;

    parsed_tx_contribution parsed_contribution = {};
    if (!get_contribution(m_blockchain.nettype(), hf_version, tx, block_height, parsed_contribution))
    {
      LOG_PRINT_L1("Register TX: Had service node registration fields, but could not decode contribution on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    const uint64_t min_transfer = get_min_node_contribution(hf_version, info.staking_requirement, info.total_reserved, info.total_num_locked_contributions());
    if (parsed_contribution.transferred < min_transfer)
    {
      LOG_PRINT_L1("Register TX: Contribution transferred: " << parsed_contribution.transferred << " didn't meet the minimum transfer requirement: " << min_transfer << " on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    size_t total_num_of_addr = service_node_addresses.size();
    if (std::find(service_node_addresses.begin(), service_node_addresses.end(), parsed_contribution.address) == service_node_addresses.end())
      total_num_of_addr++;

    if (total_num_of_addr > MAX_NUMBER_OF_CONTRIBUTORS)
    {
      LOG_PRINT_L1("Register TX: Number of participants: " << total_num_of_addr <<
                   " exceeded the max number of contributors: " << MAX_NUMBER_OF_CONTRIBUTORS <<
                   " on height: " << block_height <<
                   " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    // don't actually process this contribution now, do it when we fall through later.

    key = service_node_key;

    info.operator_address = service_node_addresses[0];
    info.portions_for_operator = portions_for_operator;
    info.registration_height = block_height;
    info.last_reward_block_height = block_height;
    info.last_reward_transaction_index = index;
    info.active_since_height = 0;
    info.last_decommission_height = 0;
    info.decommission_count = 0;
    info.total_contributed = 0;
    info.total_reserved = 0;
    info.swarm_id = UNASSIGNED_SWARM_ID;
    info.proof->public_ip = 0;
    info.proof->storage_port = 0;
    info.last_ip_change_height = block_height;
    info.version = get_min_service_node_info_version_for_hf(hf_version);

    info.contributors.clear();

    for (size_t i = 0; i < service_node_addresses.size(); i++)
    {
      // Check for duplicates
      auto iter = std::find(service_node_addresses.begin(), service_node_addresses.begin() + i, service_node_addresses[i]);
      if (iter != service_node_addresses.begin() + i)
      {
        LOG_PRINT_L1("Register TX: There was a duplicate participant for service node on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
        return false;
      }

      uint64_t hi, lo, resulthi, resultlo;
      lo = mul128(info.staking_requirement, service_node_portions[i], &hi);
      div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);

      info.contributors.emplace_back();
      auto &contributor = info.contributors.back();
      contributor.version                          = get_min_service_node_info_version_for_hf(hf_version);
      contributor.reserved                         = resultlo;
      contributor.address                          = service_node_addresses[i];
      info.total_reserved += resultlo;
    }

    return true;
  }

  bool service_node_list::process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index)
  {
    crypto::public_key key;
    auto info_ptr = std::make_shared<service_node_info>();
    service_node_info &info = *info_ptr;
    if (!is_registration_tx(tx, block_timestamp, block_height, index, key, info))
      return false;

    int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);
    if (hard_fork_version >= cryptonote::network_version_11_infinite_staking)
    {
      // NOTE(loki): Grace period is not used anymore with infinite staking. So, if someone somehow reregisters, we just ignore it
      const auto iter = m_state.service_nodes_infos.find(key);
      if (iter != m_state.service_nodes_infos.end())
        return false;

      if (m_service_node_pubkey && *m_service_node_pubkey == key) MGINFO_GREEN("Service node registered (yours): " << key << " on height: " << block_height);
      else                                                        LOG_PRINT_L1("New service node registered: "     << key << " on height: " << block_height);
    }
    else
    {
      // NOTE: A node doesn't expire until registration_height + lock blocks excess now which acts as the grace period
      // So it is possible to find the node still in our list.
      bool registered_during_grace_period = false;
      const auto iter = m_state.service_nodes_infos.find(key);
      if (iter != m_state.service_nodes_infos.end())
      {
        if (hard_fork_version >= cryptonote::network_version_10_bulletproofs)
        {
          service_node_info const &old_info = *iter->second;
          uint64_t expiry_height = old_info.registration_height + staking_num_lock_blocks(m_blockchain.nettype());
          if (block_height < expiry_height)
            return false;

          // NOTE: Node preserves its position in list if it reregisters during grace period.
          registered_during_grace_period = true;
          info.last_reward_block_height = old_info.last_reward_block_height;
          info.last_reward_transaction_index = old_info.last_reward_transaction_index;
        }
        else
        {
          return false;
        }
      }

      if (m_service_node_pubkey && *m_service_node_pubkey == key)
      {
        if (registered_during_grace_period)
        {
          MGINFO_GREEN("Service node re-registered (yours): " << key << " at block height: " << block_height);
        }
        else
        {
          MGINFO_GREEN("Service node registered (yours): " << key << " at block height: " << block_height);
        }
      }
      else
      {
        LOG_PRINT_L1("New service node registered: " << key << " at block height: " << block_height);
      }
    }

    m_state.service_nodes_infos[key] = std::move(info_ptr);
    return true;
  }

  bool service_node_list::process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index)
  {
    crypto::public_key pubkey;

    if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, pubkey))
      return false; // Is not a contribution TX don't need to check it.

    parsed_tx_contribution parsed_contribution = {};
    const uint8_t hf_version = m_blockchain.get_hard_fork_version(block_height);
    if (!get_contribution(m_blockchain.nettype(), hf_version, tx, block_height, parsed_contribution))
    {
      LOG_PRINT_L1("Contribution TX: Could not decode contribution for service node: " << pubkey << " on height: " << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    /// Service node must be registered
    auto iter = m_state.service_nodes_infos.find(pubkey);
    if (iter == m_state.service_nodes_infos.end())
    {
      LOG_PRINT_L1("Contribution TX: Contribution received for service node: " << pubkey <<
                   ", but could not be found in the service node list on height: " << block_height <<
                   " for tx: " << cryptonote::get_transaction_hash(tx )<< "\n"
                   "This could mean that the service node was deregistered before the contribution was processed.");
      return false;
    }

    const service_node_info& curinfo = *iter->second;
    if (curinfo.is_fully_funded())
    {
      LOG_PRINT_L1("Contribution TX: Service node: " << pubkey <<
                   " is already fully funded, but contribution received on height: "  << block_height <<
                   " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    if (!cryptonote::get_tx_secret_key_from_tx_extra(tx.extra, parsed_contribution.tx_key))
    {
      LOG_PRINT_L1("Contribution TX: Failed to get tx secret key from contribution received on height: "  << block_height << " for tx: " << cryptonote::get_transaction_hash(tx));
      return false;
    }

    auto &info = duplicate_info(iter->second);
    auto &contributors = info.contributors;

    auto contrib_iter = std::find_if(contributors.begin(), contributors.end(),
        [&parsed_contribution](const service_node_info::contributor_t& contributor) { return contributor.address == parsed_contribution.address; });
    const bool new_contributor = (contrib_iter == contributors.end());

    if (new_contributor)
    {
      if (contributors.size() >= MAX_NUMBER_OF_CONTRIBUTORS)
      {
        LOG_PRINT_L1("Contribution TX: Node is full with max contributors: " << MAX_NUMBER_OF_CONTRIBUTORS <<
                     " for service node: " << pubkey <<
                     " on height: "  << block_height <<
                     " for tx: " << cryptonote::get_transaction_hash(tx));
        return false;
      }

      /// Check that the contribution is large enough
      const uint8_t hf_version = m_blockchain.get_hard_fork_version(block_height);
      const uint64_t min_contribution = get_min_node_contribution(hf_version, info.staking_requirement, info.total_reserved, info.total_num_locked_contributions());
      if (parsed_contribution.transferred < min_contribution)
      {
        LOG_PRINT_L1("Contribution TX: Amount " << parsed_contribution.transferred <<
                     " did not meet min " << min_contribution <<
                     " for service node: " << pubkey <<
                     " on height: "  << block_height <<
                     " for tx: " << cryptonote::get_transaction_hash(tx));
        return false;
      }

      //
      // Successfully Validated
      //
      contrib_iter = info.contributors.emplace(contributors.end());
      contrib_iter->version = get_min_service_node_info_version_for_hf(hf_version);
      contrib_iter->address = parsed_contribution.address;
    }

    service_node_info::contributor_t& contributor = *contrib_iter;

    // In this action, we cannot
    // increase total_reserved so much that it is >= staking_requirement
    uint64_t can_increase_reserved_by = info.staking_requirement - info.total_reserved;
    uint64_t max_amount               = contributor.reserved + can_increase_reserved_by;
    parsed_contribution.transferred = std::min(max_amount - contributor.amount, parsed_contribution.transferred);

    contributor.amount     += parsed_contribution.transferred;
    info.total_contributed += parsed_contribution.transferred;

    if (contributor.amount > contributor.reserved)
    {
      info.total_reserved += contributor.amount - contributor.reserved;
      contributor.reserved = contributor.amount;
    }

    info.last_reward_block_height = block_height;
    info.last_reward_transaction_index = index;

    const size_t max_contributions_per_node = service_nodes::MAX_KEY_IMAGES_PER_CONTRIBUTOR * MAX_NUMBER_OF_CONTRIBUTORS;
    if (hf_version >= cryptonote::network_version_11_infinite_staking)
    {
      for (const service_node_info::contribution_t &contribution : parsed_contribution.locked_contributions)
      {
        if (info.total_num_locked_contributions() < max_contributions_per_node)
          contributor.locked_contributions.push_back(contribution);
        else
        {
          LOG_PRINT_L1("Contribution TX: Already hit the max number of contributions: " << max_contributions_per_node <<
                       " for contributor: " << cryptonote::get_account_address_as_str(m_blockchain.nettype(), false, contributor.address) <<
                       " on height: "  << block_height <<
                       " for tx: " << cryptonote::get_transaction_hash(tx));
          break;
        }
      }
    }

    LOG_PRINT_L1("Contribution of " << parsed_contribution.transferred << " received for service node " << pubkey);
    if (info.is_fully_funded()) {
      info.active_since_height = block_height;
      return true;
    }
    return false;
  }

  void service_node_list::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    process_block(block, txs);
    store();
  }

  static std::vector<size_t> generate_shuffled_service_node_index_list(
      size_t list_size,
      crypto::hash const &block_hash,
      quorum_type type,
      size_t sublist_size = 0,
      size_t sublist_up_to = 0)
  {
    std::vector<size_t> result(list_size);
    std::iota(result.begin(), result.end(), 0);

    uint64_t seed = 0;
    std::memcpy(&seed, block_hash.data, std::min(sizeof(seed), sizeof(block_hash.data)));
    boost::endian::little_to_native_inplace(seed);

    seed += static_cast<uint64_t>(type);

    //       Shuffle 2
    //       |=================================|
    //       |                                 |
    // Shuffle 1                               |
    // |==============|                        |
    // |     |        |                        |
    // |sublist_size  |                        |
    // |     |    sublist_up_to                |
    // 0     N        Y                        Z
    // [.......................................]

    // If we have a list [0,Z) but we need a shuffled sublist of the first N values that only
    // includes values from [0,Y) then we do this using two shuffles: first of the [0,Y) sublist,
    // then of the [N,Z) sublist (which is already partially shuffled, but that doesn't matter).  We
    // reuse the same seed for both partial shuffles, but again, that isn't an issue.
    if ((0 < sublist_size && sublist_size < list_size) && (0 < sublist_up_to && sublist_up_to < list_size)) {
      assert(sublist_size <= sublist_up_to); // Can't select N random items from M items when M < N
      loki_shuffle(result.begin(), result.begin() + sublist_up_to, seed);
      loki_shuffle(result.begin() + sublist_size, result.end(), seed);
    }
    else {
      loki_shuffle(result.begin(), result.end(), seed);
    }
    return result;
  }

  static quorum_manager generate_quorums(cryptonote::network_type nettype, service_node_list::state_t const &state, cryptonote::block const &block)
  {
    quorum_manager result = {};
    crypto::hash block_hash;

    // NOTE: The quorum for a particular height is derived from the service node
    // list state that's been updated from the next block. This is an
    // unfortunate design decision, that we locked ourselves into from the start.

    // The alternative is to subtract a 1 from the height in get_testing_quorum.
    uint64_t const height = cryptonote::get_block_height(block);
    assert(state.height == height + 1);

    int const hf_version  = block.major_version;
    if (!cryptonote::get_block_hash(block, block_hash))
    {
      MERROR("Block height: " << height << " returned null hash");
      return result;
    }

    // The two quorums here have different selection criteria: the entire checkpoint quorum and the
    // state change *validators* want only active service nodes, but the state change *workers*
    // (i.e. the nodes to be tested) also include decommissioned service nodes.  (Prior to v12 there
    // are no decommissioned nodes, so this distinction is irrelevant for network concensus).
    auto active_snode_list = state.active_service_nodes_infos();
    decltype(active_snode_list) decomm_snode_list;
    if (hf_version >= cryptonote::network_version_12_checkpointing)
      decomm_snode_list = state.decommissioned_service_nodes_infos();

    quorum_type const max_quorum_type = max_quorum_type_for_hf(hf_version);
    for (int type_int = 0; type_int <= (int)max_quorum_type; type_int++)
    {
      auto type             = static_cast<quorum_type>(type_int);
      size_t num_validators = 0, num_workers = 0;
      auto quorum           = std::make_shared<testing_quorum>();
      std::vector<size_t> pub_keys_indexes;

      if (type == quorum_type::obligations)
      {
        size_t total_nodes         = active_snode_list.size() + decomm_snode_list.size();
        num_validators             = std::min(active_snode_list.size(), STATE_CHANGE_QUORUM_SIZE);
        pub_keys_indexes           = generate_shuffled_service_node_index_list(total_nodes, block_hash, type, num_validators, active_snode_list.size());
        result.obligations         = quorum;
        size_t num_remaining_nodes = total_nodes - num_validators;
        num_workers                = std::min(num_remaining_nodes, std::max(STATE_CHANGE_MIN_NODES_TO_TEST, num_remaining_nodes/STATE_CHANGE_NTH_OF_THE_NETWORK_TO_TEST));
      }
      else if (type == quorum_type::checkpointing)
      {
        // Checkpoint quorums only exist every CHECKPOINT_INTERVAL blocks, but the height that gets
        // used to generate the quorum (i.e. the `height` variable here) is actually `H -
        // REORG_SAFETY_BUFFER_BLOCKS_POST_HF12`, where H is divisible by CHECKPOINT_INTERVAL, but
        // REORG_SAFETY_BUFFER_BLOCKS_POST_HF12 is not (it equals 11).  Hence the addition here to
        // "undo" the lag before checking to see if we're on an interval multiple:
        if ((height + REORG_SAFETY_BUFFER_BLOCKS_POST_HF12) % CHECKPOINT_INTERVAL != 0)
          continue; // Not on an interval multiple: no checkpointing quorum is defined.

        size_t total_nodes = active_snode_list.size();

        // TODO(loki): Soft fork, remove when testnet gets reset
        if (nettype == cryptonote::TESTNET && height < 85357)
          total_nodes = active_snode_list.size() + decomm_snode_list.size();

        pub_keys_indexes     = generate_shuffled_service_node_index_list(total_nodes, block_hash, type);
        result.checkpointing = quorum;
        num_workers          = std::min(pub_keys_indexes.size(), CHECKPOINT_QUORUM_SIZE);
      }
      else
      {
        MERROR("Unhandled quorum type enum with value: " << type_int);
        continue;
      }

      quorum->validators.reserve(num_validators);
      quorum->workers.reserve(num_workers);

      size_t i = 0;
      for (; i < num_validators; i++)
      {
        quorum->validators.push_back(active_snode_list[pub_keys_indexes[i]].first);
      }

      for (; i < num_validators + num_workers; i++)
      {
        size_t j = pub_keys_indexes[i];
        if (j < active_snode_list.size())
          quorum->workers.push_back(active_snode_list[j].first);
        else
          quorum->workers.push_back(decomm_snode_list[j - active_snode_list.size()].first);
      }
    }

    return result;
  }

  void service_node_list::process_block(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    uint64_t block_height = cryptonote::get_block_height(block);
    int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

    if (hard_fork_version < 9)
      return;

    assert(m_state.height == block_height);
    bool need_swarm_update = false;
    m_state_history.insert(m_state_history.end(), m_state);
    m_state.quorums = {};
    ++m_state.height;

    //
    // Cull old history
    //
    {
      uint64_t cull_height = (block_height < MAX_SHORT_TERM_STATE_HISTORY) ? 0 : block_height - MAX_SHORT_TERM_STATE_HISTORY;
      auto it = m_state_history.find(cull_height);
      if (it != m_state_history.end())
      {
        uint64_t next_long_term_state         = ((it->height / STORE_LONG_TERM_STATE_INTERVAL) + 1) * STORE_LONG_TERM_STATE_INTERVAL;
        uint64_t dist_to_next_long_term_state = next_long_term_state - it->height;
        bool need_quorum_for_future_states    = (dist_to_next_long_term_state <= VOTE_LIFETIME + VOTE_OR_TX_VERIFY_HEIGHT_BUFFER);

        if (it->height % STORE_LONG_TERM_STATE_INTERVAL == 0)
        {
          // Preserve everything
          m_long_term_states_added_to = true;
        }
        else if (need_quorum_for_future_states)
        {
          // Preserve just quorum
          state_t &state              = const_cast<state_t &>(*it); // safe: set order only depends on state_t.height
          state.service_nodes_infos   = {};
          state.key_image_blacklist   = {};
          state.only_loaded_quorums   = true;
          m_long_term_states_added_to = true;
        }
        else
        {
          if (m_store_quorum_history)
            m_old_quorum_states.emplace_back(it->height, it->quorums);
          it = m_state_history.erase(it);
        }
      }

      if (m_old_quorum_states.size() > m_store_quorum_history)
        m_old_quorum_states.erase(m_old_quorum_states.begin(), m_old_quorum_states.begin() + (m_old_quorum_states.size() -  m_store_quorum_history));
    }

    //
    // Remove expired blacklisted key images
    //
    for (auto entry = m_state.key_image_blacklist.begin(); entry != m_state.key_image_blacklist.end();)
    {
      if (block_height >= entry->unlock_height)
        entry = m_state.key_image_blacklist.erase(entry);
      else
        entry++;
    }

    //
    // Expire Nodes
    //
    for (const crypto::public_key& pubkey : update_and_get_expired_nodes(txs, block_height))
    {
      auto i = m_state.service_nodes_infos.find(pubkey);
      if (i != m_state.service_nodes_infos.end())
      {
        if (m_service_node_pubkey && *m_service_node_pubkey == pubkey)
        {
          MGINFO_GREEN("Service node expired (yours): " << pubkey << " at block height: " << block_height);
        }
        else
        {
          LOG_PRINT_L1("Service node expired: " << pubkey << " at block height: " << block_height);
        }

        need_swarm_update += i->second->is_active();
        m_state.service_nodes_infos.erase(i);
      }
    }

    //
    // Advance the list to the next candidate for a reward
    //
    {
      crypto::public_key winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(block.miner_tx.extra);
      auto it = m_state.service_nodes_infos.find(winner_pubkey);
      if (it != m_state.service_nodes_infos.end())
      {
        // set the winner as though it was re-registering at transaction index=UINT32_MAX for this block
        auto &info = duplicate_info(it->second);
        info.last_reward_block_height = block_height;
        info.last_reward_transaction_index = UINT32_MAX;
      }
    }

    //
    // Process TXs in the Block
    //
    for (uint32_t index = 0; index < txs.size(); ++index)
    {
      const cryptonote::transaction& tx = txs[index];
      if (tx.type == cryptonote::txtype::standard)
      {
        process_registration_tx(tx, block.timestamp, block_height, index);
        need_swarm_update += process_contribution_tx(tx, block_height, index);
      }
      else if (tx.type == cryptonote::txtype::state_change)
      {
        need_swarm_update += process_state_change_tx(tx, block_height);
      }
      else if (tx.type == cryptonote::txtype::key_image_unlock)
      {
        crypto::public_key snode_key;
        if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, snode_key))
          continue;

        auto it = m_state.service_nodes_infos.find(snode_key);
        if (it == m_state.service_nodes_infos.end())
          continue;

        const service_node_info &node_info = *it->second;
        if (node_info.requested_unlock_height != KEY_IMAGE_AWAITING_UNLOCK_HEIGHT)
        {
          LOG_PRINT_L1("Unlock TX: Node already requested an unlock at height: " << node_info.requested_unlock_height << " rejected on height: " << block_height << " for tx: " << get_transaction_hash(tx));
          continue;
        }

        cryptonote::tx_extra_tx_key_image_unlock unlock;
        if (!cryptonote::get_tx_key_image_unlock_from_tx_extra(tx.extra, unlock))
        {
          LOG_PRINT_L1("Unlock TX: Didn't have key image unlock in the tx_extra, rejected on height: " << block_height << " for tx: " << get_transaction_hash(tx));
          continue;
        }

        uint64_t unlock_height = get_locked_key_image_unlock_height(m_blockchain.nettype(), node_info.registration_height, block_height);

        for (const auto &contributor : node_info.contributors)
        {
          auto cit = std::find_if(contributor.locked_contributions.begin(), contributor.locked_contributions.end(),
              [&unlock](const service_node_info::contribution_t &contribution) { return unlock.key_image == contribution.key_image; });
          if (cit != contributor.locked_contributions.end())
          {
            // NOTE(loki): This should be checked in blockchain check_tx_inputs already
            crypto::hash const hash = service_nodes::generate_request_stake_unlock_hash(unlock.nonce);
            if (crypto::check_signature(hash, cit->key_image_pub_key, unlock.signature)) {
              duplicate_info(it->second).requested_unlock_height = unlock_height;
            }
            else
              LOG_PRINT_L1("Unlock TX: Couldn't verify key image unlock in the tx_extra, rejected on height: " << block_height << " for tx: " << get_transaction_hash(tx));

            break;
          }
        }
      }
    }

    m_state_history.rbegin()->quorums = generate_quorums(m_blockchain.nettype(), m_state, block);
    if (need_swarm_update)
      update_swarms(block_height);
  }

  uint64_t constexpr MIN_DERIVABLE_SHORT_TERM_STATE_OFFSET =
      MAX_SHORT_TERM_STATE_HISTORY - VOTE_LIFETIME - VOTE_OR_TX_VERIFY_HEIGHT_BUFFER;
  void service_node_list::blockchain_detached(uint64_t height)
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);

    if (m_state.height == height)
      return;

    // NOTE: The first (VOTE_LIFETIME + VOTE_OR_TX_VERIFY_HEIGHT_BUFFER) of the
    // recent states we store, don't have quorum information preceeding it.  So
    // if we pop to within that range, we can't restore the service node list to
    // its original state since we might not be able to verify state changes
    // relying on
    // quorums from before it.

    bool reinitialise = (height < STORE_LONG_TERM_STATE_INTERVAL || height < MIN_DERIVABLE_SHORT_TERM_STATE_OFFSET);
    if (!reinitialise)
    {
      uint64_t height_delta = m_state.height - height;
      if (height_delta <= MIN_DERIVABLE_SHORT_TERM_STATE_OFFSET) // Detaching to recent state, which we store all necessary information for
      {
        auto it = m_state_history.find(height);
        if (it == m_state_history.end()) reinitialise = true;
        else                             m_state_history.erase(it, m_state_history.end());
      }
      else // Detach large amount, we only store complete state every STORE_LONG_TERM_STATE_INTERVAL blocks.
      {
        uint64_t prev_interval = height - (height % STORE_LONG_TERM_STATE_INTERVAL);
        auto it                = m_state_history.upper_bound(prev_interval);
        if (it == m_state_history.end()) reinitialise = true;
        else                             m_state_history.erase(it, m_state_history.end());
      }
    }

    // NOTE: only_loaded_quorums should not be true here should be handled above, but sanity check anyway
    auto it = std::prev(m_state_history.end());
    if (m_state_history.empty() || reinitialise || it->only_loaded_quorums)
    {
      m_state_history.clear();
      init();
      return;
    }

    m_state = std::move(*it);
    m_state_history.erase(it);

    if (m_state.height != height)
      rescan_starting_from_curr_state(false /*store_to_disk*/);
    store();
  }

  std::vector<crypto::public_key> service_node_list::update_and_get_expired_nodes(const std::vector<cryptonote::transaction> &txs, uint64_t block_height)
  {
    std::vector<crypto::public_key> expired_nodes;
    uint64_t const lock_blocks = staking_num_lock_blocks(m_blockchain.nettype());

    // TODO(loki): This should really use the registration height instead of getting the block and expiring nodes.
    // But there's something subtly off when using registration height causing syncing problems.
    if (m_blockchain.get_hard_fork_version(block_height) == cryptonote::network_version_9_service_nodes)
    {
      if (block_height < lock_blocks)
        return expired_nodes;

      const uint64_t expired_nodes_block_height = block_height - lock_blocks;
      std::vector<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
      if (!m_blockchain.get_blocks(expired_nodes_block_height, 1, blocks))
      {
        LOG_ERROR("Unable to get historical blocks");
        return expired_nodes;
      }

      const cryptonote::block& block = blocks.begin()->second;
      std::vector<cryptonote::transaction> txs;
      std::vector<crypto::hash> missed_txs;
      if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
      {
        LOG_ERROR("Unable to get transactions for block " << block.hash);
        return expired_nodes;
      }

      uint32_t index = 0;
      for (const cryptonote::transaction& tx : txs)
      {
        crypto::public_key key;
        service_node_info info = {};
        if (is_registration_tx(tx, block.timestamp, expired_nodes_block_height, index, key, info))
        {
          expired_nodes.push_back(key);
        }
        index++;
      }
    }
    else
    {
      const uint64_t hf11_height = m_blockchain.get_earliest_ideal_height_for_version(cryptonote::network_version_11_infinite_staking);
      for (auto it = m_state.service_nodes_infos.begin(); it != m_state.service_nodes_infos.end(); it++)
      {
        crypto::public_key const &snode_key = it->first;
        const service_node_info &info       = *it->second;
        if (info.registration_height >= hf11_height)
        {
          if (info.requested_unlock_height != KEY_IMAGE_AWAITING_UNLOCK_HEIGHT && block_height > info.requested_unlock_height)
            expired_nodes.push_back(snode_key);
        }
        else // Version 10 Bulletproofs
        {
          /// Note: this code exhibits a sublte unintended behaviour: a snode that
          /// registered in hardfork 9 and was scheduled for deregistration in hardfork 10
          /// will have its life is slightly prolonged by the "grace period", although it might
          /// look like we use the registration height to determine the expiry height.
          uint64_t node_expiry_height = info.registration_height + lock_blocks + STAKING_REQUIREMENT_LOCK_BLOCKS_EXCESS;
          if (block_height > node_expiry_height)
            expired_nodes.push_back(snode_key);
        }
      }
    }

    return expired_nodes;
  }

  std::vector<std::pair<cryptonote::account_public_address, uint64_t>> service_node_list::get_winner_addresses_and_portions() const
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    crypto::public_key key = select_winner();
    if (key == crypto::null_pkey)
      return { std::make_pair(null_address, STAKING_PORTIONS) };

    std::vector<std::pair<cryptonote::account_public_address, uint64_t>> winners;

    const service_node_info& info = *m_state.service_nodes_infos.at(key);

    const uint64_t remaining_portions = STAKING_PORTIONS - info.portions_for_operator;

    // Add contributors and their portions to winners.
    for (const auto& contributor : info.contributors)
    {
      uint64_t hi, lo, resulthi, resultlo;
      lo = mul128(contributor.amount, remaining_portions, &hi);
      div128_64(hi, lo, info.staking_requirement, &resulthi, &resultlo);

      if (contributor.address == info.operator_address)
        resultlo += info.portions_for_operator;

      winners.emplace_back(contributor.address, resultlo);
    }
    return winners;
  }

  crypto::public_key service_node_list::select_winner() const
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    auto oldest_waiting = std::make_tuple(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint32_t>::max(), crypto::null_pkey);
    for (const auto& info : m_state.service_nodes_infos)
    {
      const auto &sninfo = *info.second;
      if (sninfo.is_active())
      {
        auto waiting_since = std::make_tuple(sninfo.last_reward_block_height, sninfo.last_reward_transaction_index, info.first);
        if (waiting_since < oldest_waiting)
        {
          oldest_waiting = waiting_since;
        }
      }
    }
    return std::get<2>(oldest_waiting);
  }

  bool service_node_list::validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, cryptonote::block_reward_parts const &reward_parts) const
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    if (hard_fork_version < 9)
      return true;

    // NOTE(loki): Service node reward distribution is calculated from the
    // original amount, i.e. 50% of the original base reward goes to service
    // nodes not 50% of the reward after removing the governance component (the
    // adjusted base reward post hardfork 10).
    uint64_t base_reward = reward_parts.original_base_reward;
    uint64_t total_service_node_reward = cryptonote::service_node_reward_formula(base_reward, hard_fork_version);

    crypto::public_key winner = select_winner();

    crypto::public_key check_winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(miner_tx.extra);
    if (check_winner_pubkey != winner) {
      MERROR("Service node reward winner is incorrect");
      return false;
    }

    const std::vector<std::pair<cryptonote::account_public_address, uint64_t>> addresses_and_portions = get_winner_addresses_and_portions();
    
    if (miner_tx.vout.size() - 1 < addresses_and_portions.size())
      return false;

    for (size_t i = 0; i < addresses_and_portions.size(); i++)
    {
      size_t vout_index = i + 1;
      uint64_t reward = cryptonote::get_portion_of_reward(addresses_and_portions[i].second, total_service_node_reward);

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

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  static service_node_list::quorum_for_serialization serialize_quorum_state(uint8_t hf_version, uint64_t height, quorum_manager const &quorums)
  {
    service_node_list::quorum_for_serialization result = {};
    result.height                                      = height;
    if (quorums.obligations)   result.quorums[static_cast<uint8_t>(quorum_type::obligations)] = *quorums.obligations;
    if (quorums.checkpointing) result.quorums[static_cast<uint8_t>(quorum_type::checkpointing)] = *quorums.checkpointing;
    return result;
  }

  static service_node_list::state_serialized serialize_service_node_state_object(uint8_t hf_version, service_node_list::state_t const &state)
  {
    service_node_list::state_serialized result = {};
    result.version                             = service_node_list::state_serialized::get_version(hf_version);

    result.infos.reserve(state.service_nodes_infos.size());
    for (const auto &kv_pair : state.service_nodes_infos)
      result.infos.emplace_back(kv_pair);

    result.key_image_blacklist = state.key_image_blacklist;
    result.height              = state.height;
    result.quorums             = serialize_quorum_state(hf_version, state.height, state.quorums);
    result.only_stored_quorums = state.only_loaded_quorums;
    return result;
  }

  bool service_node_list::store()
  {
    if (!m_db)
        return false; // Haven't been initialized yet

    uint8_t hf_version = m_blockchain.get_current_hard_fork_version();
    if (hf_version < cryptonote::network_version_9_service_nodes)
      return true;

    static data_for_serialization long_term_data  = {}; // NOTE: Static to avoid constant reallocation
    static data_for_serialization short_term_data = {};
    data_for_serialization *data[]                = {&long_term_data, &short_term_data};
    auto const serialize_version                  = data_for_serialization::get_version(hf_version);

    for (data_for_serialization *serialize_entry : data)
    {
      if (serialize_entry->version != serialize_version) m_long_term_states_added_to = true;
      serialize_entry->version = serialize_version;
      serialize_entry->clear();
    }

    {
      std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
      short_term_data.quorum_states.reserve(m_old_quorum_states.size());
      for (const quorums_by_height &entry : m_old_quorum_states)
        short_term_data.quorum_states.push_back(serialize_quorum_state(hf_version, entry.height, entry.quorums));

      uint64_t const min_short_term_height = m_state.height - MAX_SHORT_TERM_STATE_HISTORY;
      uint64_t const max_short_term_height = m_state.height - MIN_DERIVABLE_SHORT_TERM_STATE_OFFSET;
      if (m_long_term_states_added_to)
      {
        for (auto it : m_state_history)
        {
          if (it.height >= min_short_term_height) break;
          long_term_data.states.push_back(serialize_service_node_state_object(hf_version, it));
        }
      }

      for (auto it = m_state_history.lower_bound(min_short_term_height);
           it != m_state_history.end();
           it++)
      {
        if (it->height > max_short_term_height) break;
        short_term_data.states.push_back(serialize_service_node_state_object(hf_version, *it));
      }
    }

    static std::string blob;
    blob.clear();
    if (m_long_term_states_added_to)
    {
      std::stringstream ss;
      binary_archive<true> ba(ss);
      bool r = ::serialization::serialize(ba, long_term_data);
      CHECK_AND_ASSERT_MES(r, false, "Failed to store service node info: failed to serialize long term data");
      blob.append(ss.str());
      {
        cryptonote::db_wtxn_guard txn_guard(m_db);
        m_db->set_service_node_data(blob, true /*long_term*/);
      }
    }

    blob.clear();
    {
      std::stringstream ss;
      binary_archive<true> ba(ss);
      bool r = ::serialization::serialize(ba, short_term_data);
      CHECK_AND_ASSERT_MES(r, false, "Failed to store service node info: failed to serialize short term data data");
      blob.append(ss.str());
      {
        cryptonote::db_wtxn_guard txn_guard(m_db);
        m_db->set_service_node_data(blob, false /*long_term*/);
      }
    }

    m_long_term_states_added_to = false;
    return true;
  }

  void service_node_list::get_all_service_nodes_public_keys(std::vector<crypto::public_key>& keys, bool require_active) const
  {
    keys.clear();
    keys.reserve(m_state.service_nodes_infos.size());

    if (require_active) {
      for (const auto &key_info : m_state.service_nodes_infos)
        if (key_info.second->is_active())
          keys.push_back(key_info.first);
    }
    else {
      for (const auto &key_info : m_state.service_nodes_infos)
        keys.push_back(key_info.first);
    }
  }

  static crypto::hash make_uptime_proof_hash(crypto::public_key const &pubkey, uint64_t timestamp, uint32_t pub_ip, uint16_t storage_port)
  {
    constexpr size_t BUFFER_SIZE = sizeof(pubkey) + sizeof(timestamp) + sizeof(pub_ip) + sizeof(storage_port);

    boost::endian::native_to_little_inplace(timestamp);
    boost::endian::native_to_little_inplace(pub_ip);
    boost::endian::native_to_little_inplace(storage_port);

    char buf[BUFFER_SIZE];
    crypto::hash result;
    memcpy(buf, reinterpret_cast<const void *>(&pubkey), sizeof(pubkey));
    memcpy(buf + sizeof(pubkey), reinterpret_cast<const void *>(&timestamp), sizeof(timestamp));
    memcpy(buf + sizeof(pubkey) + sizeof(timestamp), reinterpret_cast<const void *>(&pub_ip), sizeof(pub_ip));
    memcpy(buf + sizeof(pubkey) + sizeof(timestamp) + sizeof(pub_ip), reinterpret_cast<const void *>(&storage_port), sizeof(storage_port));

    crypto::cn_fast_hash(buf, sizeof(buf), result);

    return result;
  }

  cryptonote::NOTIFY_UPTIME_PROOF::request service_node_list::generate_uptime_proof(crypto::public_key const &pubkey,
                                                                                    crypto::secret_key const &key,
                                                                                    uint32_t public_ip,
                                                                                    uint16_t storage_port) const
  {
    cryptonote::NOTIFY_UPTIME_PROOF::request result = {};
    result.snode_version_major                      = static_cast<uint16_t>(LOKI_VERSION_MAJOR);
    result.snode_version_minor                      = static_cast<uint16_t>(LOKI_VERSION_MINOR);
    result.snode_version_patch                      = static_cast<uint16_t>(LOKI_VERSION_PATCH);
    result.timestamp                                = time(nullptr);
    result.pubkey                                   = pubkey;
    result.public_ip                                = public_ip;
    result.storage_port                             = storage_port;

    crypto::hash hash = make_uptime_proof_hash(pubkey, result.timestamp, public_ip, storage_port);
    crypto::generate_signature(hash, pubkey, key, result.sig);
    return result;
  }

  bool service_node_list::handle_uptime_proof(cryptonote::NOTIFY_UPTIME_PROOF::request const &proof)
  {
    uint8_t const hf_version = m_blockchain.get_current_hard_fork_version();
    uint64_t const now       = time(nullptr);

    // NOTE: Validate proof version, timestamp range,
    {
      if ((proof.timestamp < now - UPTIME_PROOF_BUFFER_IN_SECONDS) || (proof.timestamp > now + UPTIME_PROOF_BUFFER_IN_SECONDS))
      {
        LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey << ": timestamp is too far from now");
        return false;
      }

      // NOTE: Only care about major version for now
      if (hf_version >= cryptonote::network_version_12_checkpointing && proof.snode_version_major < 4)
      {
        LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey
                                                    << ": v4+ loki version is required for v12+ network proofs");
        return false;
      }
      else if (hf_version >= cryptonote::network_version_11_infinite_staking && proof.snode_version_major < 3)
      {
        LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey << ": v3+ loki version is required for v11+ network proofs");
        return false;
      }
      else if (hf_version >= cryptonote::network_version_10_bulletproofs && proof.snode_version_major < 2)
      {
        LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey << ": v2+ loki version is required for v10+ network proofs");
        return false;
      }
    }

    //
    // NOTE: Validate proof signature
    //
    {
      crypto::hash hash = make_uptime_proof_hash(proof.pubkey, proof.timestamp, proof.public_ip, proof.storage_port);
      bool signature_ok = crypto::check_signature(hash, proof.pubkey, proof.sig);
      if (epee::net_utils::is_ip_local(proof.public_ip) || epee::net_utils::is_ip_loopback(proof.public_ip)) return false; // Sanity check; we do the same on lokid startup

      if (!signature_ok)
      {
        LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey << ": signature validation failed");
        return false;
      }
    }

    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    auto it = m_state.service_nodes_infos.find(proof.pubkey);
    if (it == m_state.service_nodes_infos.end())
    {
      LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey << ": no such service node is currently registered");
      return false;
    }

    const service_node_info &info = *it->second;
    if (info.proof->timestamp >= now - (UPTIME_PROOF_FREQUENCY_IN_SECONDS / 2))
    {
      LOG_PRINT_L2("Rejecting uptime proof from " << proof.pubkey
                                                  << ": already received one uptime proof for this node recently");
      return false;
    }

    LOG_PRINT_L2("Accepted uptime proof from " << proof.pubkey);
    auto &iproof = *info.proof;
    iproof.timestamp     = now;
    iproof.version_major = proof.snode_version_major;
    iproof.version_minor = proof.snode_version_minor;
    iproof.version_patch = proof.snode_version_patch;
    iproof.public_ip     = proof.public_ip;
    iproof.storage_port  = proof.storage_port;

    // Track any IP changes (so that the obligations quorum can penalize for IP changes)
    // First prune any stale (>1w) ip info.  1 week is probably excessive, but IP switches should be
    // rare and this could, in theory, be useful for diagnostics.
    auto &ips = info.proof->public_ips;
    // If we already know about the IP, update its timestamp:
    if (ips[0].first && ips[0].first == proof.public_ip)
        ips[0].second = now;
    else if (ips[1].first && ips[1].first == proof.public_ip)
        ips[1].second = now;
    // Otherwise replace whichever IP has the older timestamp
    else if (ips[0].second > ips[1].second)
        ips[1] = {proof.public_ip, now};
    else
        ips[0] = {proof.public_ip, now};

    return true;
  }

  void service_node_list::record_checkpoint_vote(crypto::public_key const &pubkey, bool voted)
  {
    std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
    auto it = m_state.service_nodes_infos.find(pubkey);
    if (it == m_state.service_nodes_infos.end())
      return;

    proof_info &info            = *it->second->proof;
    info.votes[info.vote_index] = voted;
    info.vote_index             = (info.vote_index + 1) % info.votes.size();
  }

  static quorum_manager quorum_for_serialization_to_quorum_manager(service_node_list::quorum_for_serialization const &source)
  {
    quorum_manager result = {};
    {
      auto quorum        = std::make_shared<testing_quorum>(source.quorums[static_cast<uint8_t>(quorum_type::obligations)]);
      result.obligations = quorum;
    }

    // Don't load any checkpoints that shouldn't exist (see the comment in generate_quorums as to why the `+BUFFER` term is here).
    if ((source.height + REORG_SAFETY_BUFFER_BLOCKS_POST_HF12) % CHECKPOINT_INTERVAL == 0)
    {
      auto quorum = std::make_shared<testing_quorum>(source.quorums[static_cast<uint8_t>(quorum_type::checkpointing)]);
      result.checkpointing = quorum;
    }

    return result;
  }

  service_node_list::state_t::state_t(state_serialized &&state)
  : height{state.height}
  , key_image_blacklist{std::move(state.key_image_blacklist)}
  , only_loaded_quorums{state.only_stored_quorums}
  {
    for (auto &pubkey_info : state.infos)
      service_nodes_infos.emplace(std::move(pubkey_info.pubkey), std::move(pubkey_info.info));
    
    quorums = quorum_for_serialization_to_quorum_manager(state.quorums);
  }

  bool service_node_list::load(const uint64_t current_height)
  {
    LOG_PRINT_L1("service_node_list::load()");
    reset(false);
    if (!m_db)
    {
      return false;
    }

    // NOTE: Deserialize long term state history
    uint64_t bytes_loaded = 0;
    cryptonote::db_rtxn_guard txn_guard(m_db);
    std::string blob;
    if (m_db->get_service_node_data(blob, true /*long_term*/))
    {
      bytes_loaded += blob.size();
      LOKI_DEFER { blob.clear(); };
      std::stringstream ss;
      ss << blob;
      binary_archive<false> ba(ss);

      data_for_serialization data_in = {};
      if (::serialization::serialize(ba, data_in) && data_in.states.size())
      {
        for (state_serialized &entry : data_in.states)
          m_state_history.emplace_hint(m_state_history.end(), std::move(entry));
      }
    }

    // NOTE: Deserialize short term state history
    if (!m_db->get_service_node_data(blob, false))
      return false;

    bytes_loaded += blob.size();
    std::stringstream ss;
    ss << blob;
    binary_archive<false> ba(ss);

    data_for_serialization data_in = {};
    bool deserialized              = ::serialization::serialize(ba, data_in);
    CHECK_AND_ASSERT_MES(deserialized, false, "Failed to parse service node data from blob");

    if (data_in.states.empty())
      return false;

    {
      const uint64_t hist_state_from_height = current_height - m_store_quorum_history;
      uint64_t last_loaded_height = 0;
      for (auto &states : data_in.quorum_states)
      {
        if (states.height < hist_state_from_height)
          continue;

        quorums_by_height entry = {};
        entry.height            = states.height;
        entry.quorums           = quorum_for_serialization_to_quorum_manager(states);

        if (states.height <= last_loaded_height)
        {
          LOG_PRINT_L0("Serialised quorums is not stored in ascending order by height in DB, failed to load from DB");
          return false;
        }
        last_loaded_height = states.height;
        m_old_quorum_states.push_back(entry);
      }
    }

    {
      assert(data_in.states.size() > 0);
      size_t const last_index  = data_in.states.size() - 1;
      for (size_t i = 0; i < last_index; i++)
        m_state_history.emplace_hint(m_state_history.end(), std::move(data_in.states[i]));
      m_state = std::move(data_in.states[last_index]);
    }

    MGINFO("Service node data loaded successfully, height: " << m_state.height);
    MGINFO(m_state.service_nodes_infos.size()
           << " nodes and " << m_state_history.size() << " historical states loaded ("
           << tools::get_human_readable_bytes(bytes_loaded) << ")");

    LOG_PRINT_L1("service_node_list::load() returning success");
    return true;
  }

  void service_node_list::reset(bool delete_db_entry)
  {
    m_state_history.clear();
    m_old_quorum_states.clear();
    m_state = {};

    if (m_db && delete_db_entry)
    {
      cryptonote::db_wtxn_guard txn_guard(m_db);
      m_db->clear_service_node_data();
    }

    uint64_t hardfork_9_from_height = 0;
    {
      uint32_t window, votes, threshold;
      uint8_t voting;
      m_blockchain.get_hard_fork_voting_info(9, window, votes, threshold, hardfork_9_from_height, voting);
    }
    m_state.height = hardfork_9_from_height;
  }

  size_t service_node_info::total_num_locked_contributions() const
  {
    size_t result = 0;
    for (service_node_info::contributor_t const &contributor : this->contributors)
      result += contributor.locked_contributions.size();
    return result;
  }

  converted_registration_args convert_registration_args(cryptonote::network_type nettype,
                                                        const std::vector<std::string>& args,
                                                        uint64_t staking_requirement,
                                                        uint8_t hf_version)
  {
    converted_registration_args result = {};
    if (args.size() % 2 == 0 || args.size() < 3)
    {
      result.err_msg = tr("Usage: <operator cut> <address> <fraction> [<address> <fraction> [...]]]");
      return result;
    }

    if ((args.size()-1)/ 2 > MAX_NUMBER_OF_CONTRIBUTORS)
    {
      result.err_msg = tr("Exceeds the maximum number of contributors, which is ") + std::to_string(MAX_NUMBER_OF_CONTRIBUTORS);
      return result;
    }

    try
    {
      result.portions_for_operator = boost::lexical_cast<uint64_t>(args[0]);
      if (result.portions_for_operator > STAKING_PORTIONS)
      {
        result.err_msg = tr("Invalid portion amount: ") + args[0] + tr(". Must be between 0 and ") + std::to_string(STAKING_PORTIONS);
        return result;
      }
    }
    catch (const std::exception &e)
    {
      result.err_msg = tr("Invalid portion amount: ") + args[0] + tr(". Must be between 0 and ") + std::to_string(STAKING_PORTIONS);
      return result;
    }

    struct addr_to_portion_t
    {
      cryptonote::address_parse_info info;
      uint64_t portions;
    };

    std::vector<addr_to_portion_t> addr_to_portions;
    size_t const OPERATOR_ARG_INDEX     = 1;
    for (size_t i = OPERATOR_ARG_INDEX, num_contributions = 0;
         i < args.size();
         i += 2, ++num_contributions)
    {
      cryptonote::address_parse_info info;
      if (!cryptonote::get_account_address_from_str(info, nettype, args[i]))
      {
        result.err_msg = tr("Failed to parse address: ") + args[i];
        return result;
      }

      if (info.has_payment_id)
      {
        result.err_msg = tr("Can't use a payment id for staking tx");
        return result;
      }

      if (info.is_subaddress)
      {
        result.err_msg = tr("Can't use a subaddress for staking tx");
        return result;
      }

      try
      {
        uint64_t num_portions = boost::lexical_cast<uint64_t>(args[i+1]);
        addr_to_portions.push_back({info, num_portions});
      }
      catch (const std::exception &e)
      {
        result.err_msg = tr("Invalid amount for contributor: ") + args[i] + tr(", with portion amount that could not be converted to a number: ") + args[i+1];
        return result;
      }
    }

    //
    // FIXME(doyle): FIXME(loki) !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // This is temporary code to redistribute the insufficient portion dust
    // amounts between contributors. It should be removed in HF12.
    //
    std::array<uint64_t, MAX_NUMBER_OF_CONTRIBUTORS * service_nodes::MAX_KEY_IMAGES_PER_CONTRIBUTOR> excess_portions;
    std::array<uint64_t, MAX_NUMBER_OF_CONTRIBUTORS * service_nodes::MAX_KEY_IMAGES_PER_CONTRIBUTOR> min_contributions;
    {
      // NOTE: Calculate excess portions from each contributor
      uint64_t loki_reserved = 0;
      for (size_t index = 0; index < addr_to_portions.size(); ++index)
      {
        addr_to_portion_t const &addr_to_portion = addr_to_portions[index];
        uint64_t min_contribution_portions       = service_nodes::get_min_node_contribution_in_portions(hf_version, staking_requirement, loki_reserved, index);
        uint64_t loki_amount                     = service_nodes::portions_to_amount(staking_requirement, addr_to_portion.portions);
        loki_reserved                           += loki_amount;

        uint64_t excess = 0;
        if (addr_to_portion.portions > min_contribution_portions)
          excess = addr_to_portion.portions - min_contribution_portions;

        min_contributions[index] = min_contribution_portions;
        excess_portions[index]   = excess;
      }
    }

    uint64_t portions_left  = STAKING_PORTIONS;
    uint64_t total_reserved = 0;
    for (size_t i = 0; i < addr_to_portions.size(); ++i)
    {
      addr_to_portion_t &addr_to_portion = addr_to_portions[i];
      uint64_t min_portions = get_min_node_contribution_in_portions(hf_version, staking_requirement, total_reserved, i);

      uint64_t portions_to_steal = 0;
      if (addr_to_portion.portions < min_portions)
      {
          // NOTE: Steal dust portions from other contributor if we fall below
          // the minimum by a dust amount.
          uint64_t needed             = min_portions - addr_to_portion.portions;
          const uint64_t FUDGE_FACTOR = 10;
          const uint64_t DUST_UNIT    = (STAKING_PORTIONS / staking_requirement);
          const uint64_t DUST         = DUST_UNIT * FUDGE_FACTOR;
          if (needed > DUST)
            continue;

          for (size_t sub_index = 0; sub_index < addr_to_portions.size(); sub_index++)
          {
            if (i == sub_index) continue;
            uint64_t &contributor_excess = excess_portions[sub_index];
            if (contributor_excess > 0)
            {
              portions_to_steal = std::min(needed, contributor_excess);
              addr_to_portion.portions += portions_to_steal;
              contributor_excess -= portions_to_steal;
              needed -= portions_to_steal;
              result.portions[sub_index] -= portions_to_steal;

              if (needed == 0)
                break;
            }
          }

          // NOTE: Operator is sending in the minimum amount and it falls below
          // the minimum by dust, just increase the portions so it passes
          if (needed > 0 && addr_to_portions.size() < MAX_NUMBER_OF_CONTRIBUTORS * service_nodes::MAX_KEY_IMAGES_PER_CONTRIBUTOR)
            addr_to_portion.portions += needed;
      }

      if (addr_to_portion.portions < min_portions || (addr_to_portion.portions - portions_to_steal) > portions_left)
      {
        result.err_msg = tr("Invalid amount for contributor: ") + args[i] + tr(", with portion amount: ") + args[i+1] + tr(". The contributors must each have at least 25%, except for the last contributor which may have the remaining amount");
        return result;
      }

      if (min_portions == UINT64_MAX)
      {
        result.err_msg = tr("Too many contributors specified, you can only split a node with up to: ") + std::to_string(MAX_NUMBER_OF_CONTRIBUTORS) + tr(" people.");
        return result;
      }

      portions_left -= addr_to_portion.portions;
      portions_left += portions_to_steal;
      result.addresses.push_back(addr_to_portion.info.address);
      result.portions.push_back(addr_to_portion.portions);
      uint64_t loki_amount = service_nodes::portions_to_amount(addr_to_portion.portions, staking_requirement);
      total_reserved      += loki_amount;
    }

    result.success = true;
    return result;
  }

  bool make_registration_cmd(cryptonote::network_type nettype,
      uint8_t hf_version,
      uint64_t staking_requirement,
      const std::vector<std::string>& args,
      const crypto::public_key& service_node_pubkey,
      const crypto::secret_key &service_node_key,
      std::string &cmd,
      bool make_friendly,
      boost::optional<std::string&> err_msg)
  {

    converted_registration_args converted_args = convert_registration_args(nettype, args, staking_requirement, hf_version);
    if (!converted_args.success)
    {
      MERROR(tr("Could not convert registration args, reason: ") << converted_args.err_msg);
      return false;
    }

    uint64_t exp_timestamp = time(nullptr) + STAKING_AUTHORIZATION_EXPIRATION_WINDOW;

    crypto::hash hash;
    bool hashed = cryptonote::get_registration_hash(converted_args.addresses, converted_args.portions_for_operator, converted_args.portions, exp_timestamp, hash);
    if (!hashed)
    {
      MERROR(tr("Could not make registration hash from addresses and portions"));
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
      epee::misc_utils::get_gmt_time(tt, tm);

      char buffer[128];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M:%S %p", &tm);
      stream << tr("This registration expires at ") << buffer << tr(".\n");
      stream << tr("This should be in about 2 weeks, if it isn't, check this computer's clock.\n");
      stream << tr("Please submit your registration into the blockchain before this time or it will be invalid.");
    }

    cmd = stream.str();
    return true;
  }

  bool service_node_info::can_be_voted_on(uint64_t height) const
  {
    // If the SN expired and was reregistered since the height we'll be voting on it prematurely
    if (!this->is_fully_funded() || this->registration_height >= height) return false;
    if (this->is_decommissioned() && this->last_decommission_height >= height) return false;

    if (this->is_active())
    {
      // NOTE: This cast is safe. The definition of is_active() is that active_since_height >= 0
      assert(this->active_since_height >= 0);
      if (static_cast<uint64_t>(this->active_since_height) >= height) return false;
    }

    return true;
  }

  bool service_node_info::can_transition_to_state(uint8_t hf_version, uint64_t height, new_state proposed_state) const
  {
    if (hf_version >= cryptonote::network_version_13 && !can_be_voted_on(height))
      return false;

    if (proposed_state == new_state::deregister)
    {
      if (height < this->registration_height)
        return false;
    }

    if (this->is_decommissioned())
    {
      return proposed_state != new_state::decommission &&
             proposed_state != new_state::ip_change_penalty;
    }
    else
    {
      return (proposed_state != new_state::recommission);
    }
  }

}

