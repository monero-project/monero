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

#include "common/loki.h"

namespace cryptonote
{
  class core;
  struct vote_verification_context;
};

namespace service_nodes
{
  struct service_node_info;

  LOKI_RPC_DOC_INTROSPECT
  struct testing_quorum
  {
    std::vector<crypto::public_key> validators; // Array of public keys identifying service nodes which are being tested for the queried height.
    std::vector<crypto::public_key> workers;    // Array of public keys identifying service nodes which are responsible for voting on the queried height.

    BEGIN_SERIALIZE()
      FIELD(validators)
      FIELD(workers)
    END_SERIALIZE()

    BEGIN_KV_SERIALIZE_MAP()
      std::vector<std::string> validators(this_ref.validators.size());
      for (size_t i = 0; i < this_ref.validators.size(); i++) validators[i] = epee::string_tools::pod_to_hex(this_ref.validators[i]);
      KV_SERIALIZE_VALUE(validators);

      std::vector<std::string> workers(this_ref.workers.size());
      for (size_t i = 0; i < this_ref.workers.size(); i++) workers[i] = epee::string_tools::pod_to_hex(this_ref.workers[i]);
      KV_SERIALIZE_VALUE(workers);
    END_KV_SERIALIZE_MAP()
  };

  struct quorum_manager
  {
    std::shared_ptr<const testing_quorum> obligations;
    // TODO(doyle): Validators aren't used, but I kept this as a testing_quorum
    // to avoid drastic changes for now to a lot of the service node API
    std::shared_ptr<const testing_quorum> checkpointing;
  };

  struct service_node_test_results {
    bool uptime_proved        = true;
    bool single_ip            = true;
    bool voted_in_checkpoints = true;
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
    std::vector<quorum_vote_t> get_relayable_votes(uint64_t current_height);
    bool                       handle_vote        (quorum_vote_t const &vote, cryptonote::vote_verification_context &vvc);

    static int64_t calculate_decommission_credit(const service_node_info &info, uint64_t current_height);

  private:
    void process_quorums(cryptonote::block const &block);
    service_node_test_results check_service_node(const crypto::public_key &pubkey, const service_node_info &info) const;

    cryptonote::core& m_core;
    voting_pool       m_vote_pool;
    uint64_t          m_obligations_height;
    uint64_t          m_last_checkpointed_height;
    mutable epee::critical_section m_lock;
  };
}
