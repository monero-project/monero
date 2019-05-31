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

#pragma once

#include "serialization/serialization.h"
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/service_node_voting.h"

namespace cryptonote
{
  class core;
  struct vote_verification_context;
};

namespace service_nodes
{
  struct proof_info
  {
      uint64_t timestamp;
      uint16_t version_major, version_minor, version_patch;
  };

  struct testing_quorum
  {
    std::vector<crypto::public_key> validators;
    std::vector<crypto::public_key> workers;

    BEGIN_SERIALIZE()
      FIELD(validators)
      FIELD(workers)
    END_SERIALIZE()
  };

  struct quorum_manager
  {
    std::shared_ptr<const testing_quorum> deregister;
    std::shared_ptr<const testing_quorum> checkpointing;
  };

  class quorum_cop
    : public cryptonote::BlockAddedHook,
      public cryptonote::BlockchainDetachedHook,
      public cryptonote::InitHook
  {
  public:
    explicit quorum_cop(cryptonote::core& core);

    void init() override;
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
    void blockchain_detached(uint64_t height) override;

    void                       set_votes_relayed  (std::vector<quorum_vote_t> const &relayed_votes);
    std::vector<quorum_vote_t> get_relayable_votes();
    bool                       handle_vote        (quorum_vote_t const &vote, cryptonote::vote_verification_context &vvc);
    bool                       handle_uptime_proof(const cryptonote::NOTIFY_UPTIME_PROOF::request &proof);

    static const uint64_t REORG_SAFETY_BUFFER_IN_BLOCKS = 20;

    bool       prune_uptime_proof();
    proof_info get_uptime_proof(const crypto::public_key &pubkey) const;
    void       generate_uptime_proof_request(cryptonote::NOTIFY_UPTIME_PROOF::request& req) const;

  private:
    void process_quorums(cryptonote::block const &block);

    cryptonote::core& m_core;
    voting_pool       m_vote_pool;
    uint64_t          m_uptime_proof_height;
    uint64_t          m_last_checkpointed_height;

    std::unordered_map<crypto::public_key, proof_info> m_uptime_proof_seen;
    mutable epee::critical_section m_lock;
  };
}
