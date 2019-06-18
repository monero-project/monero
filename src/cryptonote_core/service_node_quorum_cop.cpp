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

#include "service_node_quorum_cop.h"
#include "service_node_voting.h"
#include "service_node_list.h"
#include "cryptonote_config.h"
#include "cryptonote_core.h"
#include "version.h"
#include "common/loki.h"
#include "net/local_ip.h"
#include <boost/endian/conversion.hpp>

#include "common/loki_integration_test_hooks.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "quorum_cop"

namespace service_nodes
{
  static_assert(quorum_cop::REORG_SAFETY_BUFFER_IN_BLOCKS < STATE_CHANGE_VOTE_LIFETIME, "Safety buffer should always be less than the vote lifetime");

  quorum_cop::quorum_cop(cryptonote::core& core)
    : m_core(core), m_obligations_height(0), m_last_checkpointed_height(0)
  {
  }

  void quorum_cop::init()
  {
    m_obligations_height       = 0;
    m_last_checkpointed_height = 0;
    m_uptime_proof_seen.clear();

    uint64_t top_height;
    crypto::hash top_hash;
    m_core.get_blockchain_top(top_height, top_hash);

    cryptonote::block blk;
    if (m_core.get_block_by_hash(top_hash, blk))
      process_quorums(blk);
  }

  // Perform service node tests -- this returns true is the server node is in a good state, that is,
  // has submitted uptime proofs, participated in required quorums, etc.
  bool quorum_cop::check_service_node(const crypto::public_key &pubkey, const service_node_info &info) const
  {
      if (!m_uptime_proof_seen.count(pubkey))
        return false;

      // TODO: check for missing checkpoint quorum votes

      return true;
  }

  void quorum_cop::blockchain_detached(uint64_t height)
  {
    // TODO(doyle): Assumes large reorgs that are no longer possible with checkpointing
    if (m_obligations_height >= height)
    {
      LOG_ERROR("The blockchain was detached to height: " << height << ", but quorum cop has already processed votes up to " << m_obligations_height);
      LOG_ERROR("This implies a reorg occured that was over " << REORG_SAFETY_BUFFER_IN_BLOCKS << ". This should never happen! Please report this to the devs.");
      m_obligations_height = height;
    }

    if (m_last_checkpointed_height >= height)
    {
      LOG_ERROR("The blockchain was detached to height: " << height << ", but quorum cop has already processed votes up to " << m_last_checkpointed_height);
      LOG_ERROR("This implies a reorg occured that was over " << REORG_SAFETY_BUFFER_IN_BLOCKS << ". This should never happen! Please report this to the devs.");
      m_last_checkpointed_height = height;
    }
    m_vote_pool.remove_expired_votes(height);
  }

  void quorum_cop::set_votes_relayed(std::vector<quorum_vote_t> const &relayed_votes)
  {
    m_vote_pool.set_relayed(relayed_votes);
  }

  std::vector<quorum_vote_t> quorum_cop::get_relayable_votes()
  {
    std::vector<quorum_vote_t> result = m_vote_pool.get_relayable_votes();
    return result;
  }

  static int find_index_in_quorum_group(std::vector<crypto::public_key> const &group, crypto::public_key const &my_pubkey)
  {
    int result = -1;
    auto it = std::find(group.begin(), group.end(), my_pubkey);
    if (it == group.end()) return result;
    result = std::distance(group.begin(), it);
    return result;
  }

  void quorum_cop::process_quorums(cryptonote::block const &block)
  {
    crypto::public_key my_pubkey;
    crypto::secret_key my_seckey;
    if (!m_core.get_service_node_keys(my_pubkey, my_seckey))
      return;

    if (!m_core.is_service_node(my_pubkey, /*require_active=*/ true))
      return;

    uint64_t const height = cryptonote::get_block_height(block);
    for (int i = 0; i < (int)quorum_type::count; i++)
    {
      quorum_type const type       = static_cast<quorum_type>(i);
      uint64_t const vote_lifetime = service_nodes::quorum_vote_lifetime(type);

      uint64_t const latest_height = std::max(m_core.get_current_blockchain_height(), m_core.get_target_blockchain_height());
      if (latest_height < vote_lifetime)
        continue;

      uint64_t const start_voting_from_height = latest_height - vote_lifetime;
      if (height < start_voting_from_height)
        continue;

      int const hf_version = block.major_version;
      switch(type)
      {
        default:
        {
          assert("Unhandled quorum type " == 0);
          LOG_ERROR("Unhandled quorum type with value: " << (int)type);
        } break;

        case quorum_type::obligations:
        {
          if (hf_version >= cryptonote::network_version_9_service_nodes)
          {
            // NOTE: Wait atleast 2 hours before we're allowed to vote so that we collect uptimes from everyone on the network
            time_t const now = time(nullptr);
#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
            constexpr time_t min_lifetime = 0;
#else
            constexpr time_t min_lifetime = UPTIME_PROOF_MAX_TIME_IN_SECONDS;
#endif
            bool alive_for_min_time = (now - m_core.get_start_time()) >= min_lifetime;
            if (!alive_for_min_time)
            {
              continue;
            }

            if (m_obligations_height < start_voting_from_height)
              m_obligations_height = start_voting_from_height;

            for (; m_obligations_height < (height - REORG_SAFETY_BUFFER_IN_BLOCKS); m_obligations_height++)
            {
              if (m_core.get_hard_fork_version(m_obligations_height) < cryptonote::network_version_9_service_nodes) continue;

              const std::shared_ptr<const testing_quorum> quorum =
                  m_core.get_testing_quorum(quorum_type::obligations, m_obligations_height);
              if (!quorum)
              {
                // TODO(loki): Fatal error
                LOG_ERROR("Obligations quorum for height: " << m_obligations_height << " was not cached in daemon!");
                continue;
              }

              if (quorum->workers.empty()) continue;

              int index_in_group = find_index_in_quorum_group(quorum->validators, my_pubkey);
              if (index_in_group <= -1) continue;

              //
              // NOTE: I am in the quorum
              //
              auto worker_states = m_core.get_service_node_list_state(quorum->workers);
              auto worker_it = worker_states.begin();
              CRITICAL_REGION_LOCAL(m_lock);
              int good = 0, total = 0;
              for (size_t node_index = 0; node_index < quorum->workers.size(); ++worker_it, ++node_index)
              {
                // If the SN no longer exists then it'll be omitted from the worker_states vector,
                // so if the elements don't line up skip ahead.
                while (worker_it->pubkey != quorum->workers[node_index] && node_index < quorum->workers.size())
                  node_index++;
                if (node_index == quorum->workers.size())
                  break;
                total++;

                const auto &node_key = worker_it->pubkey;
                const auto &info  = worker_it->info;

                bool checks_passed = check_service_node(node_key, info);

                new_state vote_for_state;
                if (checks_passed) {
                  if (!info.is_decommissioned()) {
                    good++;
                    continue;
                  }

                  vote_for_state = new_state::recommission;
                  LOG_PRINT_L2("Decommissioned service node " << quorum->workers[node_index] << " is now passing required checks; voting to recommission");
                }
                else {
                  int64_t credit = calculate_decommission_credit(info, latest_height);

                  if (info.is_decommissioned()) {
                    if (credit >= 0) {
                      LOG_PRINT_L2("Decommissioned service node " << quorum->workers[node_index] << " is still not passing required checks, but has remaining credit (" <<
                          credit << " blocks); abstaining (to leave decommissioned)");
                      continue;
                    }

                    LOG_PRINT_L2("Decommissioned service node " << quorum->workers[node_index] << " has no remaining credit; voting to deregister");
                    vote_for_state = new_state::deregister; // Credit ran out!
                  } else {
                    if (credit >= DECOMMISSION_MINIMUM) {
                      vote_for_state = new_state::decommission;
                      LOG_PRINT_L2("Service node " << quorum->workers[node_index] << " has stopped passing required checks, but has sufficient earned credit (" <<
                          credit << " blocks) to avoid deregistration; voting to decommission");
                    } else {
                      vote_for_state = new_state::deregister;
                      LOG_PRINT_L2("Service node " << quorum->workers[node_index] << " has stopped passing required checks, but does not have sufficient earned credit (" <<
                          credit << " blocks, " << DECOMMISSION_MINIMUM << " required) to decommission; voting to deregister");
                    }
                  }
                }

                quorum_vote_t vote = service_nodes::make_state_change_vote(
                    m_obligations_height, static_cast<uint16_t>(index_in_group), node_index, vote_for_state, my_pubkey, my_seckey);
                cryptonote::vote_verification_context vvc;
                if (!handle_vote(vote, vvc))
                  LOG_ERROR("Failed to add uptime check_state vote; reason: " << print_vote_verification_context(vvc, nullptr));
              }
              if (good > 0)
                LOG_PRINT_L2(good << " of " << total << " service nodes are active and passing checks; no state change votes required");
            }
          }
        }
        break;

        case quorum_type::checkpointing:
        {
          if (hf_version >= cryptonote::network_version_12_checkpointing)
          {
            if (m_last_checkpointed_height < start_voting_from_height)
              m_last_checkpointed_height = start_voting_from_height;

            for (m_last_checkpointed_height += (m_last_checkpointed_height % CHECKPOINT_INTERVAL);
                 m_last_checkpointed_height <= height;
                 m_last_checkpointed_height += CHECKPOINT_INTERVAL)
            {
              if (m_core.get_hard_fork_version(m_last_checkpointed_height) < cryptonote::network_version_12_checkpointing)
                continue;

              const std::shared_ptr<const testing_quorum> quorum =
                  m_core.get_testing_quorum(quorum_type::checkpointing, m_last_checkpointed_height);
              if (!quorum)
              {
                // TODO(loki): Fatal error
                LOG_ERROR("Checkpoint quorum for height: " << m_last_checkpointed_height
                                                           << " was not cached in daemon!");
                continue;
              }

              int index_in_group = find_index_in_quorum_group(quorum->workers, my_pubkey);
              if (index_in_group <= -1) continue;

              //
              // NOTE: I am in the quorum, handle checkpointing
              //
              quorum_vote_t vote         = {};
              vote.type                  = quorum_type::checkpointing;
              vote.checkpoint.block_hash = m_core.get_block_id_by_height(m_last_checkpointed_height);

              if (vote.checkpoint.block_hash == crypto::null_hash)
              {
                // TODO(loki): Fatal error
                LOG_ERROR("Could not get block hash for block on height: " << m_last_checkpointed_height);
                continue;
              }

              vote.block_height   = m_last_checkpointed_height;
              vote.group          = quorum_group::worker;
              vote.index_in_group = static_cast<uint16_t>(index_in_group);
              vote.signature      = make_signature_from_vote(vote, my_pubkey, my_seckey);

              cryptonote::vote_verification_context vvc = {};
              if (!handle_vote(vote, vvc))
                LOG_ERROR("Failed to add checkpoint vote reason: " << print_vote_verification_context(vvc, nullptr));
            }
          }

        }
        break;
      }
    }
  }

  void quorum_cop::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    process_quorums(block);

    // Since our age checks for state change votes is now (age >=
    // STATE_CHANGE_VOTE_LIFETIME_IN_BLOCKS) where age is
    // get_current_blockchain_height() which gives you the height that you are
    // currently mining for, i.e. (height + 1).

    // Otherwise peers will silently drop connection from each other when they
    // go around P2Ping votes due to passing around old votes
    uint64_t const height = cryptonote::get_block_height(block) + 1;
    m_vote_pool.remove_expired_votes(height);
    m_vote_pool.remove_used_votes(txs, block.major_version);
  }

  bool quorum_cop::handle_vote(quorum_vote_t const &vote, cryptonote::vote_verification_context &vvc)
  {
    vvc = {};
    switch(vote.type)
    {
      default:
      {
        LOG_PRINT_L1("Unhandled vote type with value: " << (int)vote.type);
        assert("Unhandled vote type" == 0);
        return false;
      };

      case quorum_type::obligations:
      break;
      case quorum_type::checkpointing:
      {
        cryptonote::block block;
        if (!m_core.get_block_by_hash(vote.checkpoint.block_hash, block)) // Does vote reference a valid block hash?
        {
          LOG_PRINT_L1("Vote does not reference valid block hash: " << vote.checkpoint.block_hash);
          return false;
        }
      }
      break;
    }

    // NOTE: Only do validation that relies on access cryptonote::core here in quorum cop, the rest goes in voting pool
    std::shared_ptr<const testing_quorum> quorum = m_core.get_testing_quorum(vote.type, vote.block_height);
    if (!quorum)
    {
      // TODO(loki): Fatal error
      LOG_ERROR("Quorum state for height: " << vote.block_height << " was not cached in daemon!");
      return false;
    }

    uint64_t latest_height             = std::max(m_core.get_current_blockchain_height(), m_core.get_target_blockchain_height());
    std::vector<pool_vote_entry> votes = m_vote_pool.add_pool_vote_if_unique(latest_height, vote, vvc, *quorum);
    bool result                        = !vvc.m_verification_failed;

    if (!vvc.m_added_to_pool) // NOTE: Not unique vote
      return result;

    switch(vote.type)
    {
      default:
      {
        LOG_PRINT_L1("Unhandled vote type with value: " << (int)vote.type);
        assert("Unhandled vote type" == 0);
        return false;
      };

      case quorum_type::obligations:
      {
        if (votes.size() >= STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE)
        {
          cryptonote::tx_extra_service_node_state_change state_change{
            vote.state_change.state, vote.block_height, vote.state_change.worker_index};
          state_change.votes.reserve(votes.size());

          std::transform(votes.begin(), votes.end(), std::back_inserter(state_change.votes), [](pool_vote_entry const &pool_vote) {
            return cryptonote::tx_extra_service_node_state_change::vote{pool_vote.vote.signature, pool_vote.vote.index_in_group};
          });

          cryptonote::transaction state_change_tx = {};
          int hf_version = m_core.get_blockchain_storage().get_current_hard_fork_version();
          if (cryptonote::add_service_node_state_change_to_tx_extra(state_change_tx.extra, state_change, hf_version))
          {
            state_change_tx.version = cryptonote::transaction::get_max_version_for_hf(hf_version, m_core.get_nettype());
            state_change_tx.type    = cryptonote::txtype::state_change;

            cryptonote::tx_verification_context tvc = {};
            cryptonote::blobdata const tx_blob      = cryptonote::tx_to_blob(state_change_tx);

            result &= m_core.handle_incoming_tx(tx_blob, tvc, false /*keeped_by_block*/, false /*relayed*/, false /*do_not_relay*/);
            if (!result || tvc.m_verifivation_failed)
            {
              LOG_PRINT_L1("A full state change tx for height: " << vote.block_height <<
                           " and service node: " << vote.state_change.worker_index <<
                           " could not be verified and was not added to the memory pool, reason: " <<
                           print_tx_verification_context(tvc, &state_change_tx));
            }
          }
          else
          {
            LOG_PRINT_L1("Failed to add state change to tx extra for height: "
                         << vote.block_height << " and service node: " << vote.state_change.worker_index);
          }
        }
        else
        {
          LOG_PRINT_L2("Don't have enough votes yet to submit a state change transaction: have " << votes.size() << " of " << STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE << " required");
        }
      }
      break;

      case quorum_type::checkpointing:
      {
        if (votes.size() >= CHECKPOINT_MIN_VOTES)
        {
          cryptonote::checkpoint_t checkpoint = {};
          checkpoint.type                     = cryptonote::checkpoint_type::service_node;
          checkpoint.height                   = vote.block_height;
          checkpoint.block_hash               = vote.checkpoint.block_hash;
          checkpoint.signatures.reserve(votes.size());

          for (pool_vote_entry const &pool_vote : votes)
          {
            voter_to_signature vts = {};
            vts.voter_index        = pool_vote.vote.index_in_group;
            vts.signature          = pool_vote.vote.signature;
            checkpoint.signatures.push_back(vts);
          }

          m_core.get_blockchain_storage().update_checkpoint(checkpoint);
        }
        {
          LOG_PRINT_L2("Don't have enough votes yet to submit a checkpoint: have " << votes.size() << " of " << CHECKPOINT_MIN_VOTES << " required");
        }
      }
      break;
    }
    return result;
  }

  /// NOTE(maxim): we can remove this after hardfork
  static crypto::hash make_hash(crypto::public_key const &pubkey, uint64_t timestamp)
  {
    boost::endian::native_to_little(timestamp);
    char buf[44] = "SUP"; // Meaningless magic bytes
    crypto::hash result;
    memcpy(buf + 4, reinterpret_cast<const void *>(&pubkey), sizeof(pubkey));
    memcpy(buf + 4 + sizeof(pubkey), reinterpret_cast<const void *>(&timestamp), sizeof(timestamp));
    crypto::cn_fast_hash(buf, sizeof(buf), result);

    return result;
  }

  static crypto::hash make_hash_v2(crypto::public_key const& pubkey,
                                   uint64_t timestamp,
                                   uint32_t pub_ip,
                                   uint16_t storage_port)
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

  bool quorum_cop::handle_uptime_proof(const cryptonote::NOTIFY_UPTIME_PROOF::request &proof)
  {
    uint64_t now = time(nullptr);

    uint64_t timestamp               = proof.timestamp;
    const crypto::public_key& pubkey = proof.pubkey;
    const crypto::signature& sig     = proof.sig;
    const uint32_t public_ip         = proof.public_ip;
    const uint16_t storage_port      = proof.storage_port;

    if ((timestamp < now - UPTIME_PROOF_BUFFER_IN_SECONDS) || (timestamp > now + UPTIME_PROOF_BUFFER_IN_SECONDS)) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": timestamp is too far from now");
      return false;
    }

    if (!m_core.is_service_node(pubkey, /*require_active=*/ false)) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": no such service node is currently registered");
      return false;
    }

    uint64_t height = m_core.get_current_blockchain_height();
    int version     = m_core.get_hard_fork_version(height);

    // NOTE: Only care about major version for now
    // FIXME(Jason): remove this `false` before release!
    if (false && version >= cryptonote::network_version_12_checkpointing && proof.snode_version_major < 4) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": v4+ loki version is required for v12+ network proofs");
      return false;
    }
    else if (version >= cryptonote::network_version_11_infinite_staking && proof.snode_version_major < 3) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": v3+ loki version is required for v11+ network proofs");
      return false;
    }
    else if (version >= cryptonote::network_version_10_bulletproofs && proof.snode_version_major < 2) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": v2+ loki version is required for v10+ network proofs");
      return false;
    }

    CRITICAL_REGION_LOCAL(m_lock);
    if (m_uptime_proof_seen[pubkey].timestamp >= now - (UPTIME_PROOF_FREQUENCY_IN_SECONDS / 2)) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": already received one uptime proof for this node recently");
      return false;
    }

    const uint64_t hf12_height = m_core.get_earliest_ideal_height_for_version(cryptonote::network_version_12_checkpointing);

    /// Accept both old and new uptime proofs in a small window of 2 blocks
    /// after switching to hf 12; (for simplicity accept new signatures before hf 12 too)
    const bool enforce_v2 = (hf12_height != std::numeric_limits<uint64_t>::max() && height >= hf12_height + 2);

    crypto::hash hash;
    bool signature_ok = false;
    if (!enforce_v2) {

      hash = make_hash(pubkey, timestamp);

      signature_ok = crypto::check_signature(hash, pubkey, sig);

      if (!signature_ok) {
        hash = make_hash_v2(pubkey, timestamp, public_ip, storage_port);
        signature_ok = crypto::check_signature(hash, pubkey, sig);
      }

    } else {
      hash = make_hash_v2(pubkey, timestamp, public_ip, storage_port);
      signature_ok = crypto::check_signature(hash, pubkey, sig);

      /// Sanity check; we do the same on lokid startup
      if (epee::net_utils::is_ip_local(public_ip) || epee::net_utils::is_ip_loopback(public_ip)) return false;
    }

    if (!signature_ok) {
      LOG_PRINT_L2("Rejecting uptime proof from " << pubkey << ": signature validation failed");
      return false;
    }

    m_uptime_proof_seen[pubkey] = {now, proof.snode_version_major, proof.snode_version_minor, proof.snode_version_patch};
    LOG_PRINT_L2("Accepted uptime proof from " << pubkey);
    return true;
  }

  void quorum_cop::generate_uptime_proof_request(cryptonote::NOTIFY_UPTIME_PROOF::request& req) const
  {
    req.snode_version_major = static_cast<uint16_t>(LOKI_VERSION_MAJOR);
    req.snode_version_minor = static_cast<uint16_t>(LOKI_VERSION_MINOR);
    req.snode_version_patch = static_cast<uint16_t>(LOKI_VERSION_PATCH);

    crypto::public_key pubkey;
    crypto::secret_key seckey;
    m_core.get_service_node_keys(pubkey, seckey);

    req.timestamp           = time(nullptr);
    req.pubkey              = pubkey;
    req.public_ip           = m_core.get_service_node_public_ip();
    req.storage_port        = m_core.get_storage_port();

    crypto::hash hash;

    const uint8_t version = m_core.get_blockchain_storage().get_current_hard_fork_version();

    if (version < cryptonote::network_version_12_checkpointing) {
      hash = make_hash(req.pubkey, req.timestamp);
    } else {
      hash = make_hash_v2(req.pubkey, req.timestamp, req.public_ip, req.storage_port);
    }

    crypto::generate_signature(hash, pubkey, seckey, req.sig);
  }

  bool quorum_cop::prune_uptime_proof()
  {
    uint64_t now = time(nullptr);
    const uint64_t prune_from_timestamp = now - UPTIME_PROOF_MAX_TIME_IN_SECONDS;
    CRITICAL_REGION_LOCAL(m_lock);

    std::vector<crypto::public_key> to_remove;
    for (const auto &proof : m_uptime_proof_seen)
    {
      if (proof.second.timestamp < prune_from_timestamp)
        to_remove.push_back(proof.first);
    }
    for (const auto &pk : to_remove)
      m_uptime_proof_seen.erase(pk);

    return true;
  }

  proof_info quorum_cop::get_uptime_proof(const crypto::public_key &pubkey) const
  {

    CRITICAL_REGION_LOCAL(m_lock);
    const auto it = m_uptime_proof_seen.find(pubkey);
    if (it == m_uptime_proof_seen.end())
      return {};

    return it->second;
  }

  // Calculate the decommission credit for a service node.  If the SN is current decommissioned this
  // returns the number of blocks remaining in the credit; otherwise this is the number of currently
  // accumulated blocks.
  int64_t quorum_cop::calculate_decommission_credit(const service_node_info &info, uint64_t current_height)
  {
    // If currently decommissioned, we need to know how long it was up before being decommissioned;
    // otherwise we need to know how long since it last become active until now.
    int64_t blocks_up;
    if (info.is_decommissioned()) // decommissioned; the negative of active_since_height tells us when the period leading up to the current decommission started
      blocks_up = int64_t(info.last_decommission_height) - (-info.active_since_height);
    else
      blocks_up = int64_t(current_height) - int64_t(info.active_since_height);

    // Now we calculate the credit earned from being up for `blocks_up` blocks
    int64_t credit = 0;
    if (blocks_up >= 0) {
      credit = blocks_up * DECOMMISSION_CREDIT_PER_DAY / BLOCKS_EXPECTED_IN_HOURS(24);

      if (info.decommission_count <= info.is_decommissioned()) // Has never been decommissioned (or is currently in the first decommission), so add initial starting credit
        credit += DECOMMISSION_INITIAL_CREDIT;
      if (credit > DECOMMISSION_MAX_CREDIT)
        credit = DECOMMISSION_MAX_CREDIT; // Cap the available decommission credit blocks if above the max
    }

    // If currently decommissioned, remove any used credits used for the current downtime
    if (info.is_decommissioned())
      credit -= int64_t(current_height) - int64_t(info.last_decommission_height);

    return credit;
  }
}
