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

#include "blockchain.h"
#include "serialization/serialization.h"
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"

namespace cryptonote
{
  class core;
};

namespace service_nodes
{
  struct proof_info
  {
      uint64_t timestamp;
      uint16_t version_major, version_minor, version_patch;
  };

  struct quorum_checkpointing
  {
    std::vector<crypto::public_key> quorum_nodes;
    BEGIN_SERIALIZE()
      FIELD(quorum_nodes)
    END_SERIALIZE()
  };

  struct quorum_uptime_proof
  {
    std::vector<crypto::public_key> quorum_nodes;
    std::vector<crypto::public_key> nodes_to_test;

    BEGIN_SERIALIZE()
      FIELD(quorum_nodes)
      FIELD(nodes_to_test)
    END_SERIALIZE()
  };

  struct quorum_manager
  {
    std::shared_ptr<const quorum_uptime_proof>  uptime_proof;
    std::shared_ptr<const quorum_checkpointing> checkpointing;
  };

  enum struct quorum_type
  {
    uptime_proof = 0,
    checkpointing,
    count,
  };

  class quorum_cop
    : public cryptonote::Blockchain::BlockAddedHook,
      public cryptonote::Blockchain::BlockchainDetachedHook,
      public cryptonote::Blockchain::InitHook
  {
  public:
    explicit quorum_cop(cryptonote::core& core);

    void init() override;
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
    void blockchain_detached(uint64_t height) override;

    void process_uptime_quorum    (cryptonote::block const &block);
    void process_checkpoint_quorum(cryptonote::block const &block);

    bool handle_uptime_proof(const cryptonote::NOTIFY_UPTIME_PROOF::request &proof);

    static const uint64_t REORG_SAFETY_BUFFER_IN_BLOCKS = 20;
    static_assert(REORG_SAFETY_BUFFER_IN_BLOCKS < deregister_vote::VOTE_LIFETIME_BY_HEIGHT,
                  "Safety buffer should always be less than the vote lifetime");
    bool prune_uptime_proof();

    proof_info get_uptime_proof(const crypto::public_key &pubkey) const;

    void generate_uptime_proof_request(cryptonote::NOTIFY_UPTIME_PROOF::request& req) const;

  private:

    cryptonote::core& m_core;
    uint64_t m_uptime_proof_height;

    std::unordered_map<crypto::public_key, proof_info> m_uptime_proof_seen;
    mutable epee::critical_section m_lock;
  };
}
