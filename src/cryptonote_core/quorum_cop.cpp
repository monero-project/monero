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
#include "cryptonote_config.h"
#include "cryptonote_core.h"
#include "quorum_cop.h"

namespace service_nodes
{
  quorum_cop::quorum_cop(cryptonote::core& core, service_nodes::service_node_list& service_node_list)
    : m_core(core), m_service_node_list(service_node_list), m_last_height(0)
  {
  }

  void quorum_cop::blockchain_detached(uint64_t height)
  {
    if (m_last_height >= height)
    {
      LOG_ERROR("The blockchain was detached to height: " << height << ", but quorum cop has already processed votes up to " << m_last_height);
      LOG_ERROR("This implies a reorg occured that was over " << REORG_SAFETY_BUFFER_IN_BLOCKS << ". This should never happen! Please report this to the devs.");
      m_last_height = height;
    }
  }

  void quorum_cop::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
  {
    crypto::public_key my_pubkey;
    crypto::secret_key my_seckey;
    if (!m_core.get_service_node_keys(my_pubkey, my_seckey))
      return;

    time_t const now          = time(nullptr);
    time_t const min_lifetime = 60 * 60 * 2;
    bool alive_for_min_time   = (now - m_core.get_start_time()) >= min_lifetime;
    if (!alive_for_min_time)
    {
      return;
    }

    uint64_t const height        = cryptonote::get_block_height(block);
    uint64_t const latest_height = m_core.get_current_blockchain_height();

    if (latest_height < triton::service_node_deregister::VOTE_LIFETIME_BY_HEIGHT)
      return;

    uint64_t const execute_justice_from_height = latest_height - triton::service_node_deregister::VOTE_LIFETIME_BY_HEIGHT;
    if (height < execute_justice_from_height)
      return;

    if (m_last_height < execute_justice_from_height)
      m_last_height = execute_justice_from_height;


    for (;m_last_height < (height - REORG_SAFETY_BUFFER_IN_BLOCKS); m_last_height++)
    {
      const std::shared_ptr<quorum_state> state = m_core.get_quorum_state(m_last_height);
      if (!state)
      {
        // TODO(triton): Fatal error
        LOG_ERROR("Quorum state for height: " << m_last_height << "was not cached in daemon!");
        continue;
      }

      auto it = std::find(state->quorum_nodes.begin(), state->quorum_nodes.end(), my_pubkey);
      if (it == state->quorum_nodes.end())
        continue;

      size_t my_index_in_quorum = it - state->quorum_nodes.begin();
      for (size_t node_index = 0; node_index < state->nodes_to_test.size(); ++node_index)
      {
        const crypto::public_key &node_key = state->nodes_to_test[node_index];

        CRITICAL_REGION_LOCAL(m_lock);
        bool vote_off_node = (m_uptime_proof_seen.find(node_key) != m_uptime_proof_seen.end());

        if (!vote_off_node)
          continue;

        triton::service_node_deregister::vote vote = {};
        vote.block_height        = m_last_height;
        vote.service_node_index  = node_index;
        vote.voters_quorum_index = my_index_in_quorum;
        vote.signature           = triton::service_node_deregister::sign_vote(vote.block_height, vote.service_node_index, my_pubkey, my_seckey);

        cryptonote::vote_verification_context vvc = {};
        if (!m_core.add_deregister_vote(vote, vvc))
        {
          if (vvc.m_invalid_block_height)
            LOG_ERROR("block height was invalid: " << vote.block_height);

          if (vvc.m_voters_quorum_index_out_of_bounds)
            LOG_ERROR("voters quorum index specified out of bounds: " << vote.voters_quorum_index);

          if (vvc.m_service_node_index_out_of_bounds)
            LOG_ERROR("service node index specified out of bounds: " << vote.service_node_index);

          if (vvc.m_signature_not_valid)
            LOG_ERROR("signature was not valid, was the signature signed properly?");
        }
      }
    }
  }

  static crypto::hash make_hash(crypto::public_key const &pubkey, uint64_t timestamp)
  {
    char buf[44] = "SUP"; // Meaningless magic bytes
    crypto::hash result;
    memcpy(buf + 4, reinterpret_cast<const void *>(&pubkey), sizeof(pubkey));
    memcpy(buf + 4 + sizeof(pubkey), reinterpret_cast<const void *>(&timestamp), sizeof(timestamp));
    crypto::cn_fast_hash(buf, sizeof(buf), result);

    return result;
  }

  bool quorum_cop::handle_uptime_proof(uint64_t timestamp, const crypto::public_key& pubkey, const crypto::signature& sig)
  {
    uint64_t now = time(nullptr);

    if ((timestamp < now - UPTIME_PROOF_BUFFER_IN_SECONDS) || (timestamp > now + UPTIME_PROOF_BUFFER_IN_SECONDS))
      return false;

    // TODO(doyle): the only dependency on m_service_node_lists which could be
    // replaced by the lists stored in db when that is implemented - 2018-07-24
    if (!m_service_node_list.is_service_node(pubkey))
      return false;

    CRITICAL_REGION_LOCAL(m_lock);
    if (m_uptime_proof_seen[pubkey] >= now - (UPTIME_PROOF_FREQUENCY_IN_SECONDS / 2))
      return false; // already received one uptime proof for this node recently.

    crypto::hash hash = make_hash(pubkey, timestamp);
    if (!crypto::check_signature(hash, pubkey, sig))
      return false;

    m_uptime_proof_seen[pubkey] = timestamp;
    return true;
  }

  void quorum_cop::generate_uptime_proof_request(const crypto::public_key& pubkey, const crypto::secret_key& seckey, cryptonote::NOTIFY_UPTIME_PROOF::request& req) const
  {
    req.timestamp     = time(nullptr);
    req.pubkey        = pubkey;

    crypto::hash hash = make_hash(req.pubkey, req.timestamp);
    crypto::generate_signature(hash, pubkey, seckey, req.sig);
  }

  bool quorum_cop::prune_uptime_proof()
  {
    uint64_t now = time(nullptr);
    const uint64_t prune_from_timestamp = now - UPTIME_PROOF_MAX_TIME_IN_SECONDS;
    CRITICAL_REGION_LOCAL(m_lock);

    for (auto it = m_uptime_proof_seen.begin(); it != m_uptime_proof_seen.end();)
    {
      if (it->second < prune_from_timestamp)
        it = m_uptime_proof_seen.erase(it);
      else
        it++;
    }

    return true;
  }
}
