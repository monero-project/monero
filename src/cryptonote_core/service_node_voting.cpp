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

#include "service_node_voting.h"
#include "service_node_list.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"

#include "misc_log_ex.h"
#include "string_tools.h"

#include <random>
#include <string>
#include <vector>

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{
  bool convert_deregister_vote_to_legacy(quorum_vote_t const &vote, legacy_deregister_vote &legacy_vote)
  {
    if (vote.type != quorum_type::deregister)
      return false;

    legacy_vote.block_height           = vote.block_height;
    legacy_vote.service_node_index     = vote.deregister.worker_index;
    legacy_vote.voters_quorum_index    = vote.index_in_group;
    legacy_vote.signature              = vote.signature;
    return true;
  }

  quorum_vote_t convert_legacy_deregister_vote(legacy_deregister_vote const &vote)
  {
    quorum_vote_t result           = {};
    result.type                    = quorum_type::deregister;
    result.block_height            = vote.block_height;
    result.signature               = vote.signature;
    result.group                   = quorum_group::validator;
    result.index_in_group          = vote.voters_quorum_index;
    result.deregister.worker_index = vote.service_node_index;
    return result;
  }

  static crypto::hash make_deregister_vote_hash(uint64_t block_height, uint32_t service_node_index)
  {
    const int buf_size = sizeof(block_height) + sizeof(service_node_index);
    char buf[buf_size];

    memcpy(buf, reinterpret_cast<void *>(&block_height), sizeof(block_height));
    memcpy(buf + sizeof(block_height), reinterpret_cast<void *>(&service_node_index), sizeof(service_node_index));

    crypto::hash result;
    crypto::cn_fast_hash(buf, buf_size, result);
    return result;
  }

  crypto::signature make_signature_from_vote(quorum_vote_t const &vote, const crypto::public_key& pub, const crypto::secret_key& sec)
  {
    crypto::signature result = {};
    switch(vote.type)
    {
      default:
      {
        LOG_PRINT_L1("Unhandled vote type with value: " << (int)vote.type);
        assert("Unhandled vote type" == 0);
        return result;
      };

      case quorum_type::deregister:
      {
        crypto::hash hash = make_deregister_vote_hash(vote.block_height, vote.deregister.worker_index);
        crypto::generate_signature(hash, pub, sec, result);
      }
      break;

      case quorum_type::checkpointing:
      {
        crypto::hash hash = vote.checkpoint.block_hash;
        crypto::generate_signature(hash, pub, sec, result);
      }
      break;
    }

    return result;
  }

  crypto::signature make_signature_from_tx_deregister(cryptonote::tx_extra_service_node_deregister const &deregister, crypto::public_key const &pub, crypto::secret_key const &sec)
  {
    crypto::signature result;
    crypto::hash hash = make_deregister_vote_hash(deregister.block_height, deregister.service_node_index);
    crypto::generate_signature(hash, pub, sec, result);
    return result;
  }

  static bool bounds_check_worker_index(service_nodes::testing_quorum const &quorum, size_t worker_index, cryptonote::vote_verification_context &vvc)
  {
    if (worker_index >= quorum.workers.size())
    {
      vvc.m_worker_index_out_of_bounds = true;
      LOG_PRINT_L1("Quorum worker index in was out of bounds: " << worker_index << ", expected to be in range of: [0, " << quorum.workers.size() << ")");
      return false;
    }
    return true;
  }

  static bool bounds_check_validator_index(service_nodes::testing_quorum const &quorum, size_t validator_index, cryptonote::vote_verification_context &vvc)
  {
    if (validator_index >= quorum.validators.size())
    {
      vvc.m_validator_index_out_of_bounds = true;
      LOG_PRINT_L1("Validator's index was out of bounds: " << validator_index << ", expected to be in range of: [0, " << quorum.validators.size() << ")");
      return false;
    }
    return true;
  }

  bool verify_tx_deregister(const cryptonote::tx_extra_service_node_deregister &deregister,
                            cryptonote::vote_verification_context &vvc,
                            const service_nodes::testing_quorum &quorum)
  {
    if (deregister.votes.size() < service_nodes::DEREGISTER_MIN_VOTES_TO_KICK_SERVICE_NODE)
    {
      LOG_PRINT_L1("Not enough votes");
      vvc.m_not_enough_votes = true;
      return false;
    }

    if (deregister.votes.size() > service_nodes::DEREGISTER_QUORUM_SIZE)
    {
      LOG_PRINT_L1("Too many votes");
      return false;
    }

    if (!bounds_check_worker_index(quorum, deregister.service_node_index, vvc))
      return false;

    crypto::hash const hash = make_deregister_vote_hash(deregister.block_height, deregister.service_node_index);
    std::array<int, service_nodes::DEREGISTER_QUORUM_SIZE> validator_set = {};
    for (const cryptonote::tx_extra_service_node_deregister::vote& vote : deregister.votes)
    {
      if (!bounds_check_validator_index(quorum, vote.validator_index, vvc))
        return false;

      if (++validator_set[vote.validator_index] > 1)
      {
        vvc.m_duplicate_voters = true;
        LOG_PRINT_L1("Voter quorum index is duplicated: " << vote.validator_index);
        return false;
      }

      crypto::public_key const &key = quorum.validators[vote.validator_index];
      if (!crypto::check_signature(hash, key, vote.signature))
      {
        LOG_PRINT_L1("Invalid signatures for votes");
        vvc.m_signature_not_valid = true;
        return false;
      }
    }

    return true;
  }

  quorum_vote_t make_deregister_vote(uint64_t block_height, uint16_t validator_index, uint16_t worker_index, crypto::public_key const &pub_key, crypto::secret_key const &sec_key)
  {
    quorum_vote_t result           = {};
    result.type                    = quorum_type::deregister;
    result.block_height            = block_height;
    result.group                   = quorum_group::validator;
    result.index_in_group          = validator_index;
    result.deregister.worker_index = worker_index;
    result.signature               = make_signature_from_vote(result, pub_key, sec_key);
    return result;
  }

  bool verify_vote(const quorum_vote_t& vote, uint64_t latest_height, cryptonote::vote_verification_context &vvc, const service_nodes::testing_quorum &quorum)
  {
    bool result = true;
    if (vote.group == quorum_group::invalid)
      result = false;
    else if (vote.group == quorum_group::validator)
      result = bounds_check_validator_index(quorum, vote.index_in_group, vvc);
    else
      result = bounds_check_worker_index(quorum, vote.index_in_group, vvc);

    if (!result)
      return result;

    uint64_t max_vote_age = 0;
    {
      crypto::public_key key = crypto::null_pkey;
      crypto::hash hash      = crypto::null_hash;

      switch(vote.type)
      {
        default:
        {
          LOG_PRINT_L1("Unhandled vote type with value: " << (int)vote.type);
          assert("Unhandled vote type" == 0);
          return false;
        };

        case quorum_type::deregister:
        {
          if (vote.group != quorum_group::validator)
          {
            LOG_PRINT_L1("Vote received specifies incorrect voting group, expected vote from validator");
            vvc.m_incorrect_voting_group = true;
            return false;
          }

          key          = quorum.validators[vote.index_in_group];
          max_vote_age = service_nodes::DEREGISTER_VOTE_LIFETIME;
          hash         = make_deregister_vote_hash(vote.block_height, vote.deregister.worker_index);

          bool result = bounds_check_worker_index(quorum, vote.deregister.worker_index, vvc);
          if (!result)
            return result;
        }
        break;

        case quorum_type::checkpointing:
        {
          if (vote.group != quorum_group::worker)
          {
            LOG_PRINT_L1("Vote received specifies incorrect voting group, expected vote from worker");
            vvc.m_incorrect_voting_group = true;
            return false;
          }

          key          = quorum.workers[vote.index_in_group];
          max_vote_age = service_nodes::CHECKPOINT_VOTE_LIFETIME;
          hash         = vote.checkpoint.block_hash;
        }
        break;
      }

      //
      // NOTE: Validate vote signature
      //
      result = crypto::check_signature(hash, key, vote.signature);
      if (!result)
      {
        vvc.m_signature_not_valid = true;
        return result;
      }
    }

    //
    // NOTE: Validate vote age
    //
    {
      uint64_t delta_height = latest_height - vote.block_height;
      if (vote.block_height < latest_height && delta_height >= max_vote_age)
      {
        LOG_PRINT_L1("Received vote for height: " << vote.block_height << ", is older than: " << max_vote_age
                                                  << " blocks and has been rejected.");
        vvc.m_invalid_block_height = true;
      }
      else if (vote.block_height > latest_height)
      {
        LOG_PRINT_L1("Received vote for height: " << vote.block_height << ", is newer than: " << latest_height
                                                  << " (latest block height) and has been rejected.");
        vvc.m_invalid_block_height = true;
      }

      if (vvc.m_invalid_block_height)
      {
        result                    = false;
        vvc.m_verification_failed = true;
        return result;
      }
    }

    result = true;
    return result;
  }

  void voting_pool::set_relayed(const std::vector<quorum_vote_t>& votes)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    const time_t now = time(NULL);

    for (const quorum_vote_t &find_vote : votes)
    {
      std::vector<pool_vote_entry> *vote_pool = nullptr;
      switch(find_vote.type)
      {
        default:
        {
          LOG_PRINT_L1("Unhandled find_vote type with value: " << (int)find_vote.type);
          assert("Unhandled find_vote type" == 0);
          break;
        };

        case quorum_type::deregister:
        {
          auto it = std::find_if(m_deregister_pool.begin(), m_deregister_pool.end(), [find_vote](deregister_pool_entry const &entry) {
              return (entry.height == find_vote.block_height &&
                      entry.worker_index == find_vote.deregister.worker_index);
          });

          if (it == m_deregister_pool.end())
            break;

          vote_pool = &it->votes;
        }
        break;

        case quorum_type::checkpointing:
        {
          auto it = std::find_if(m_checkpoint_pool.begin(), m_checkpoint_pool.end(), [find_vote](checkpoint_pool_entry const &entry) {
              return (entry.height == find_vote.block_height && entry.hash == find_vote.checkpoint.block_hash);
          });

          if (it == m_checkpoint_pool.end())
            break;

          vote_pool = &it->votes;
        }
        break;
      }

      if (vote_pool) // We found the group that this vote belongs to
      {
        auto vote = std::find_if(vote_pool->begin(), vote_pool->end(), [find_vote](pool_vote_entry const &entry) {
            return (find_vote.index_in_group == entry.vote.index_in_group);
        });

        if (vote != vote_pool->end())
        {
          vote->time_last_sent_p2p = now;
        }
      }
    }
  }

  std::vector<quorum_vote_t> voting_pool::get_relayable_votes() const
  {
    CRITICAL_REGION_LOCAL(m_lock);

    // TODO(doyle): Rate-limiting: A better threshold value that follows suite with transaction relay time back-off
#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
    const time_t TIME_BETWEEN_RELAY = 0;
#else
    const time_t TIME_BETWEEN_RELAY = 60 * 2;
#endif

    time_t const now = time(nullptr);
    std::vector<quorum_vote_t> result;
    for (deregister_pool_entry const &pool_entry : m_deregister_pool)
    {
      for (pool_vote_entry const &vote_entry : pool_entry.votes)
      {
        const time_t last_sent = now - vote_entry.time_last_sent_p2p;
        if (last_sent > TIME_BETWEEN_RELAY)
          result.push_back(vote_entry.vote);
      }
    }

    for (checkpoint_pool_entry const &pool_entry : m_checkpoint_pool)
    {
      for (pool_vote_entry const &vote_entry : pool_entry.votes)
      {
        const time_t last_sent = now - vote_entry.time_last_sent_p2p;
        if (last_sent > TIME_BETWEEN_RELAY)
          result.push_back(vote_entry.vote);
      }
    }

    return result;
  }

  // return: True if the vote was unique
  template <typename T>
  static bool add_vote_to_pool_if_unique(T &vote_pool, quorum_vote_t const &vote)
  {
    auto vote_it = std::find_if(vote_pool.votes.begin(), vote_pool.votes.end(), [&vote](pool_vote_entry const &pool_entry) {
        assert(pool_entry.vote.group == vote.group);
        return (pool_entry.vote.index_in_group == vote.index_in_group);
    });

    bool result = false;
    if (vote_it == vote_pool.votes.end())
    {
      pool_vote_entry entry = {};
      entry.vote            = vote;
      vote_pool.votes.push_back(entry);
      result                = true;
    }

    return result;
  }

  std::vector<pool_vote_entry> voting_pool::add_pool_vote_if_unique(uint64_t latest_height,
                                                                    const quorum_vote_t& vote,
                                                                    cryptonote::vote_verification_context& vvc,
                                                                    const service_nodes::testing_quorum &quorum)
  {
    std::vector<pool_vote_entry> result = {};

    if (!verify_vote(vote, latest_height, vvc, quorum))
    {
      LOG_PRINT_L1("Signature verification failed for deregister vote");
      return result;
    }

    CRITICAL_REGION_LOCAL(m_lock);
    switch(vote.type)
    {
      default:
      {
        LOG_PRINT_L1("Unhandled vote type with value: " << (int)vote.type);
        assert("Unhandled vote type" == 0);
        return result;
      };

      case quorum_type::deregister:
      {
        time_t const now = time(NULL);
        auto it = std::find_if(m_deregister_pool.begin(), m_deregister_pool.end(), [&vote](deregister_pool_entry const &entry) {
            return (entry.height == vote.block_height && entry.worker_index == vote.deregister.worker_index);
        });

        if (it == m_deregister_pool.end())
        {
          m_deregister_pool.emplace_back(vote.block_height, vote.deregister.worker_index);
          it = (m_deregister_pool.end() - 1);
        }

        deregister_pool_entry &pool_entry = (*it);
        vvc.m_added_to_pool               = add_vote_to_pool_if_unique(pool_entry, vote);
        result                            = pool_entry.votes;
      }
      break;

      case quorum_type::checkpointing:
      {
        // Get Matching Checkpoint
        auto it = std::find_if(m_checkpoint_pool.begin(), m_checkpoint_pool.end(), [&vote](checkpoint_pool_entry const &entry) {
            return (entry.height == vote.block_height && entry.hash == vote.checkpoint.block_hash);
        });

        if (it == m_checkpoint_pool.end())
        {
          m_checkpoint_pool.emplace_back(vote.block_height, vote.checkpoint.block_hash);
          it = (m_checkpoint_pool.end() - 1);
        }

        checkpoint_pool_entry &pool_entry = (*it);
        vvc.m_added_to_pool               = add_vote_to_pool_if_unique(pool_entry, vote);
        result                            = pool_entry.votes;
      }
      break;
    }

    return result;
  }

  void voting_pool::remove_used_votes(std::vector<cryptonote::transaction> const &txs)
  {
    // TODO(doyle): Cull checkpoint votes
    CRITICAL_REGION_LOCAL(m_lock);
    if (m_deregister_pool.empty())
      return;

    for (const cryptonote::transaction &tx : txs)
    {
      if (tx.get_type() != cryptonote::transaction::type_deregister)
        continue;

      cryptonote::tx_extra_service_node_deregister deregister;
      if (!get_service_node_deregister_from_tx_extra(tx.extra, deregister))
      {
        LOG_ERROR("Could not get deregister from tx, possibly corrupt tx");
        continue;
      }

      auto it = std::find_if(m_deregister_pool.begin(), m_deregister_pool.end(), [&deregister](deregister_pool_entry const &entry){
          return (entry.height == deregister.block_height) && (entry.worker_index == deregister.service_node_index);
      });

      if (it != m_deregister_pool.end())
        m_deregister_pool.erase(it);
    }
  }

  template <typename T>
  static void cull_votes(std::vector<T> &vote_pool, uint64_t min_height, uint64_t max_height)
  {
    for (auto it = vote_pool.begin(); it != vote_pool.end(); ++it)
    {
      const T &pool_entry = *it;
      if (pool_entry.height < min_height || pool_entry.height > max_height)
      {
        it = vote_pool.erase(it);
        it--;
      }
    }
  }

  void voting_pool::remove_expired_votes(uint64_t height)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    uint64_t deregister_min_height = (height < DEREGISTER_VOTE_LIFETIME) ? 0 : height - DEREGISTER_VOTE_LIFETIME;
    cull_votes(m_deregister_pool, deregister_min_height, height);

    uint64_t checkpoint_min_height = (height < CHECKPOINT_VOTE_LIFETIME) ? 0 : height - CHECKPOINT_VOTE_LIFETIME;
    cull_votes(m_checkpoint_pool, checkpoint_min_height, height);
  }
}; // namespace service_nodes

