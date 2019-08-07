// Copyright (c) 2018, The Loki Project
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

#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_basic/tx_extra.h"

#include "string_tools.h"
#include "math_helper.h"
#include "syncobj.h"

namespace cryptonote
{
  struct tx_verification_context;
  struct vote_verification_context;
  struct checkpoint_t;
};

namespace service_nodes
{
  struct testing_quorum;

  struct voter_to_signature
  {
    uint16_t          voter_index;
    crypto::signature signature;

    BEGIN_SERIALIZE()
      FIELD(voter_index)
      FIELD(signature)
    END_SERIALIZE()
  };

  struct checkpoint_vote { crypto::hash block_hash; };
  struct state_change_vote { uint16_t worker_index; new_state state; };

  enum struct quorum_type : uint8_t
  {
    obligations = 0,
    checkpointing,
    count,
    rpc_request_all_quorums_sentinel_value = 255, // Only valid for get_quorum_state RPC call
  };

  inline char const *quorum_type_to_string(quorum_type v)
  {
    switch(v)
    {
      case quorum_type::obligations:   return "obligation";
      case quorum_type::checkpointing: return "checkpointing";
      default: assert(false);          return "xx_unhandled_type";
    }
  }

  enum struct quorum_group : uint8_t { invalid, validator, worker };
  struct quorum_vote_t
  {
    uint8_t           version = 0;
    quorum_type       type;
    uint64_t          block_height;
    quorum_group      group;
    uint16_t          index_in_group;
    crypto::signature signature;

    union
    {
      state_change_vote state_change;
      checkpoint_vote   checkpoint;
    };
  };

  quorum_vote_t     make_state_change_vote(uint64_t block_height, uint16_t index_in_group, uint16_t worker_index, new_state state, crypto::public_key const &pub_key, crypto::secret_key const &secret_key);

  bool              verify_checkpoint                  (cryptonote::checkpoint_t const &checkpoint, service_nodes::testing_quorum const &quorum);
  bool              verify_tx_state_change             (const cryptonote::tx_extra_service_node_state_change& state_change, uint64_t latest_height, cryptonote::tx_verification_context& vvc, const service_nodes::testing_quorum &quorum, uint8_t hf_version);
  bool              verify_vote_age                    (const quorum_vote_t& vote, uint64_t latest_height, cryptonote::vote_verification_context &vvc);
  bool              verify_vote_against_quorum         (const quorum_vote_t& vote, cryptonote::vote_verification_context &vvc, const service_nodes::testing_quorum &quorum);
  crypto::signature make_signature_from_vote           (quorum_vote_t const &vote, const crypto::public_key& pub, const crypto::secret_key& sec);
  crypto::signature make_signature_from_tx_state_change(cryptonote::tx_extra_service_node_state_change const &state_change, crypto::public_key const &pub, crypto::secret_key const &sec);

  // NOTE: This preserves the deregister vote format pre-checkpointing so that
  // up to the hardfork, we can still deserialize and serialize until we switch
  // over to the new format
  struct legacy_deregister_vote
  {
    uint64_t          block_height;
    uint32_t          service_node_index;
    uint32_t          voters_quorum_index;
    crypto::signature signature;
  };

  bool           convert_deregister_vote_to_legacy(quorum_vote_t const &vote, legacy_deregister_vote &legacy);
  quorum_vote_t  convert_legacy_deregister_vote   (legacy_deregister_vote const &vote);

  struct pool_vote_entry
  {
    quorum_vote_t vote;
    uint64_t      time_last_sent_p2p;
  };

  struct voting_pool
  {
    // return: The vector of votes if the vote is valid (and even if it is not unique) otherwise nullptr
    std::vector<pool_vote_entry> add_pool_vote_if_unique(const quorum_vote_t &vote, cryptonote::vote_verification_context &vvc);

    // TODO(loki): Review relay behaviour and all the cases when it should be triggered
    void                         set_relayed         (const std::vector<quorum_vote_t>& votes);
    void                         remove_expired_votes(uint64_t height);
    void                         remove_used_votes   (std::vector<cryptonote::transaction> const &txs, uint8_t hard_fork_version);
    std::vector<quorum_vote_t>   get_relayable_votes (uint64_t height) const;
    bool                         received_checkpoint_vote(uint64_t height, size_t index_in_quorum) const;

  private:
    std::vector<pool_vote_entry> *find_vote_pool(const quorum_vote_t &vote, bool create_if_not_found = false);

    struct obligations_pool_entry
    {
      explicit obligations_pool_entry(const quorum_vote_t &vote)
          : height{vote.block_height}, worker_index{vote.state_change.worker_index}, state{vote.state_change.state} {}
      obligations_pool_entry(const cryptonote::tx_extra_service_node_state_change &sc)
          : height{sc.block_height}, worker_index{sc.service_node_index}, state{sc.state} {}

      uint64_t                     height;
      uint32_t                     worker_index;
      new_state                    state;
      std::vector<pool_vote_entry> votes;

      bool operator==(const obligations_pool_entry &e) const { return height == e.height && worker_index == e.worker_index && state == e.state; }
    };
    std::vector<obligations_pool_entry> m_obligations_pool;

    struct checkpoint_pool_entry
    {
      explicit checkpoint_pool_entry(const quorum_vote_t &vote) : height{vote.block_height}, hash{vote.checkpoint.block_hash} {}
      checkpoint_pool_entry(uint64_t height, crypto::hash const &hash): height(height), hash(hash) {}
      uint64_t                     height;
      crypto::hash                 hash;
      std::vector<pool_vote_entry> votes;

      bool operator==(const checkpoint_pool_entry &e) const { return height == e.height && hash == e.hash; }
    };
    std::vector<checkpoint_pool_entry> m_checkpoint_pool;

    mutable epee::critical_section m_lock;
  };
}; // namespace service_nodes

