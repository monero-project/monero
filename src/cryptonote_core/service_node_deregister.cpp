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

#include "service_node_deregister.h"
#include "service_node_list.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_core/service_node_list.h"
#include "cryptonote_core/blockchain.h"

#include "misc_log_ex.h"
#include "string_tools.h"

#include <random>
#include <string>
#include <vector>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "service_nodes"


namespace triton
{
  static crypto::hash make_hash_from(uint64_t block_height, uint32_t service_node_index)
  {
    const int buf_size = sizeof(block_height) + sizeof(service_node_index);
    char buf[buf_size];

    memcpy(buf, reinterpret_cast<void *>(&block_height), sizeof(block_height));
    memcpy(buf + sizeof(block_height), reinterpret_cast<void *>(&service_node_index), sizeof(service_node_index));

    crypto::hash result;
    crypto::cn_fast_hash(buf, buf_size, result);

    return result;
  }

  crypto::signature service_node_deregister::sign_vote(uint64_t block_height, uint32_t service_node_index, const crypto::public_key& pub, const crypto::secret_key& sec)
  {
    crypto::signature result;
    crypto::generate_signature(make_hash_from(block_height, service_node_index), pub, sec, result);
    return result;
  }

  bool service_node_deregister::verify_vote_signature(uint64_t block_height, uint32_t service_node_index, crypto::public_key p, crypto::signature s)
  {
    std::vector<std::pair<crypto::public_key, crypto::signature>> keys_and_sigs{ std::make_pair(p, s) };
    return verify_votes_signature(block_height, service_node_index, keys_and_sigs);
  }

  bool service_node_deregister::verify_votes_signature(uint64_t block_height, uint32_t service_node_index, const std::vector<std::pair<crypto::public_key, crypto::signature>>& keys_and_sigs)
  {
    crypto::hash hash = make_hash_from(block_height, service_node_index);
    for (auto& key_and_sig : keys_and_sigs)
    {
      if (!crypto::check_signature(hash, key_and_sig.first, key_and_sig.second))
      {
        return false;
      }
    }

    return true;
  }

  static bool verify_votes_helper(const cryptonote::tx_extra_service_node_deregister& deregister,
                                  cryptonote::vote_verification_context &vvc,
                                  const service_nodes::quorum_state &quorum_state)
  {
    if (deregister.service_node_index >= quorum_state.nodes_to_test.size())
    {
      vvc.m_service_node_index_out_of_bounds = true;
      LOG_PRINT_L1("Service node index in deregister vote was out of bounds:  " << deregister.service_node_index << ", expected to be in range of: [0, " << quorum_state.nodes_to_test.size() << "]");
      return false;
    }

    const std::vector<crypto::public_key>& quorum = quorum_state.quorum_nodes;
	std::vector<int8_t> quorum_set;

	std::vector<std::pair<crypto::public_key, crypto::signature>> keys_and_sigs;
	for (const cryptonote::tx_extra_service_node_deregister::vote& vote : deregister.votes)
    {
		if (vote.voters_quorum_index >= quorum.size())
      {
			vvc.m_voters_quorum_index_out_of_bounds = true;
			LOG_PRINT_L1("Voter's index in deregister vote was out of bounds:  " << vote.voters_quorum_index << ", expected to be in range of: [0, " << quorum.size() << "]");
			return false;
      }
		if (nettype == cryptonote::TESTNET && deregister.block_height < 8670)
		{
			// TODO(doyle): Remove on next testnet re-launch, also. Can remove nettype param until next fork I guess.
		}
		else
		{
			quorum_set.resize(quorum.size());
			if (++quorum_set[vote.voters_quorum_index] > 1)
			{
				vvc.m_duplicate_voters = true;
				LOG_PRINT_L1("Voter quorum index is duplicated:  " << vote.voters_quorum_index << ", expected to be in range of: [0, " << quorum.size() << "]");
				return false;
			}
		}
		keys_and_sigs.push_back(std::make_pair(quorum[vote.voters_quorum_index], vote.signature));
	}
  }

  bool r = service_node_deregister::verify_votes_signature(deregister.block_height, deregister.service_node_index, keys_and_sigs);
  if (!r)
  {
	  LOG_PRINT_L1("Invalid signatures for votes");
	  vvc.m_verification_failed = true;

	  return false;
  }
  return r;
  }

  bool service_node_deregister::verify_deregister(const cryptonote::tx_extra_service_node_deregister& deregister,
                                                  cryptonote::vote_verification_context &vvc,
                                                  const service_nodes::quorum_state &quorum_state)
  {
    if (deregister.votes.size() < service_nodes::MIN_VOTES_TO_KICK_SERVICE_NODE)
    {
	  LOG_PRINT_L1("Not enough votes");
      vvc.m_not_enough_votes = true;
      return false;
    }

    bool result = verify_votes_helper(deregister, vvc, quorum_state);
    return result;
  }

  bool service_node_deregister::verify_vote(const vote& v, cryptonote::vote_verification_context &vvc,
                                            const service_nodes::quorum_state &quorum_state)
  {
    cryptonote::tx_extra_service_node_deregister deregister;
    deregister.block_height = v.block_height;
    deregister.service_node_index = v.service_node_index;
    deregister.votes.push_back(cryptonote::tx_extra_service_node_deregister::vote{ v.signature, v.voters_quorum_index });
    return verify_votes_helper(deregister, vvc, quorum_state);
  }

  void deregister_vote_pool::set_relayed(const std::vector<service_node_deregister::vote>& votes)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    const time_t now = time(NULL);

    for (const service_node_deregister::vote &find_vote : votes)
    {
      deregister_group desired_group   = {};
      desired_group.block_height       = find_vote.block_height;
      desired_group.service_node_index = find_vote.service_node_index;

      auto deregister_entry = m_deregisters.find(desired_group);
      if (deregister_entry != m_deregisters.end())
      {
        std::vector<deregister> &deregister_vector = deregister_entry->second;
        for (auto &deregister : deregister_vector)
        {
          if (deregister.m_vote.voters_quorum_index == find_vote.voters_quorum_index)
          {
            deregister.m_time_last_sent_p2p = now;
            break;
          }
        }
      }
    }
  }

  std::vector<service_node_deregister::vote> deregister_vote_pool::get_relayable_votes() const
  {
    CRITICAL_REGION_LOCAL(m_lock);
    const cryptonote::cryptonote_connection_context fake_context = AUTO_VAL_INIT(fake_context);

    // TODO(doyle): Rate-limiting: A better threshold value that follows suite with transaction relay time back-off
    const time_t now       = time(NULL);
    const time_t THRESHOLD = 60 * 2;

    std::vector<service_node_deregister::vote> result;
    for (const auto &deregister_entry : m_deregisters)
    {
      const std::vector<deregister>& deregister_vector = deregister_entry.second;
      for (const deregister &entry : deregister_vector)
      {
        const time_t last_sent = now - entry.m_time_last_sent_p2p;
        if (last_sent > THRESHOLD)
        {
          result.push_back(entry.m_vote);
        }
      }
    }
    return result;
  }

  bool deregister_vote_pool::add_vote(const service_node_deregister::vote& new_vote,
                                      cryptonote::vote_verification_context& vvc,
                                      const service_nodes::quorum_state &quorum_state,
                                      cryptonote::transaction &tx)
  {
    if (!service_node_deregister::verify_vote(new_vote, vvc, quorum_state))
    {
      LOG_PRINT_L1("Signature verification failed for deregister vote");
      return false;
    }

    CRITICAL_REGION_LOCAL(m_lock);
    time_t const now = time(NULL);
    std::vector<deregister> *deregister_votes;
    {
      deregister_group desired_group   = {};
      desired_group.block_height       = new_vote.block_height;
      desired_group.service_node_index = new_vote.service_node_index;
      deregister_votes                 = &m_deregisters[desired_group];
    }

    bool new_deregister_is_unique = true;
    for (const auto &entry : *deregister_votes)
    {
      if (entry.m_vote.voters_quorum_index == new_vote.voters_quorum_index)
      {
        new_deregister_is_unique = false;
        break;
      }
    }

    if (new_deregister_is_unique)
    {
      vvc.m_added_to_pool = true;
      deregister_votes->emplace_back(deregister(0 /*time_last_sent_p2p*/, new_vote));

      if (deregister_votes->size() >= service_nodes::MIN_VOTES_TO_KICK_SERVICE_NODE)
      {
        cryptonote::tx_extra_service_node_deregister deregister;
        deregister.block_height       = new_vote.block_height;
        deregister.service_node_index = new_vote.service_node_index;
        deregister.votes.reserve(deregister_votes->size());

        for (const auto& entry : *deregister_votes)
        {
          cryptonote::tx_extra_service_node_deregister::vote tx_vote = {};
		  tx_vote.signature = entry.m_vote.signature;
		  tx_vote.voters_quorum_index = entry.m_vote.voters_quorum_index;
          deregister.votes.push_back(tx_vote);
        }


        vvc.m_full_tx_deregister_made = cryptonote::add_service_node_deregister_to_tx_extra(tx.extra, deregister);
        if (vvc.m_full_tx_deregister_made)
        {
          tx.version       = cryptonote::transaction::version_3_per_output_unlock_times;
         tx.is_deregister = true;
        }
        else
        {
          LOG_PRINT_L1("Could not create version 3 deregistration transaction from votes");
        }
      }
    }

    return true;
  }

  void deregister_vote_pool::remove_used_votes(std::vector<cryptonote::transaction> const &txs)
  {
    CRITICAL_REGION_LOCAL(m_lock);
    for (const cryptonote::transaction &tx : txs)
    {
      if (!tx.is_deregister_tx())
        continue;

      cryptonote::tx_extra_service_node_deregister deregister;
      if (!get_service_node_deregister_from_tx_extra(tx.extra, deregister))
      {
        LOG_ERROR("Could not get deregister from tx version 3, possibly corrupt tx");
        continue;
      }

      deregister_group desired_group   = {};
      desired_group.block_height       = deregister.block_height;
      desired_group.service_node_index = deregister.service_node_index;
      m_deregisters.erase(desired_group);
    }
  }

  void deregister_vote_pool::remove_expired_votes(uint64_t height)
  {
    if (height < service_node_deregister::VOTE_LIFETIME_BY_HEIGHT)
    {
      return;
    }

    CRITICAL_REGION_LOCAL(m_lock);
    uint64_t minimum_height = height - service_node_deregister::VOTE_LIFETIME_BY_HEIGHT;
    for (auto it = m_deregisters.begin(); it != m_deregisters.end();)
    {
      const deregister_group &deregister_for = it->first;
      if (deregister_for.block_height < minimum_height)
      {
        it = m_deregisters.erase(it);
      }
      else
      {
        it++;
      }
    }
  }
}; // namespace triton
