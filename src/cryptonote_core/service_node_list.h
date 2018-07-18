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

#define ROLLBACK_EVENT_EXPIRATION_BLOCKS 30

namespace service_nodes
{
  const size_t QUORUM_SIZE                    = 3;
  const size_t MIN_VOTES_TO_KICK_SERVICE_NODE = 3;
  const size_t NTH_OF_THE_NETWORK_TO_TEST     = 100;
  const size_t MIN_NODES_TO_TEST              = 50;

  struct quorum_state
  {
    void clear() { quorum_nodes.clear(); nodes_to_test.clear(); }
    std::vector<crypto::public_key> quorum_nodes;
    std::vector<crypto::public_key> nodes_to_test;
  };

  class service_node_list
    : public cryptonote::Blockchain::BlockAddedHook,
      public cryptonote::Blockchain::BlockchainDetachedHook,
      public cryptonote::Blockchain::InitHook,
      public cryptonote::Blockchain::ValidateMinerTxHook
  {
  public:
    service_node_list(cryptonote::Blockchain& blockchain);
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs);
    void blockchain_detached(uint64_t height);
    void init();
    bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward);

    std::vector<cryptonote::account_public_address> get_expired_nodes(uint64_t block_height) const;
    cryptonote::account_public_address select_winner(const crypto::hash& prev_id);

    bool is_service_node(const crypto::public_key& pubkey) const;
    const std::shared_ptr<quorum_state> get_quorum_state(uint64_t height) const;

  private:

    bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address, crypto::public_key& key) const;
    bool is_deregistration_tx(const cryptonote::transaction& tx, cryptonote::account_public_address& address) const;

    std::vector<crypto::public_key> get_service_node_pubkeys() const;
    template<typename T>
    void block_added_generic(const cryptonote::block& block, const T& txs);

    bool reg_tx_has_correct_unlock_time(const cryptonote::transaction& tx, uint64_t block_height) const;
    bool reg_tx_extract_fields(const cryptonote::transaction& tx, cryptonote::account_public_address& address, crypto::public_key& service_node_key, crypto::public_key& tx_pub_key) const;
    bool is_reg_tx_staking_output(const cryptonote::transaction& tx, int i, uint64_t block_height, crypto::key_derivation derivation, hw::device& hwdev) const;

    cryptonote::account_public_address find_service_node_from_miner_tx(const cryptonote::transaction& miner_tx, uint64_t block_height) const;

    void store_quorum_state_from_rewards_list(uint64_t height);

    class rollback_event
    {
    public:
      rollback_event(uint64_t block_height);
      virtual ~rollback_event() { }
      virtual bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const = 0;
      uint64_t m_block_height;
    };

    class rollback_change : public rollback_event
    {
    public:
      rollback_change(uint64_t block_height, const cryptonote::account_public_address& address, const std::pair<uint64_t, size_t>& height_index, const crypto::public_key& key);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const;
    private:
      cryptonote::account_public_address m_address;
      std::pair<uint64_t, size_t> m_height_index;
      crypto::public_key m_key;
    };

    class rollback_new : public rollback_event
    {
    public:
      rollback_new(uint64_t block_height, cryptonote::account_public_address address);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const;
    private:
      cryptonote::account_public_address m_address;
    };

    class prevent_rollback : public rollback_event
    {
    public:
      prevent_rollback(uint64_t block_height);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const;
    };

    // Service nodes are organized by time since last reward or registration
    // This value is given by block height, and differentiated by transaction index for
    // registrations that occured in the same block. index = 0 for block reward, 1 for first transaction, etc.
    // hence a std::pair<uint64_t, size_t> is used here for this value.
    std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>> m_service_nodes_last_reward;
    std::unordered_map<cryptonote::account_public_address, crypto::public_key> m_service_nodes_keys;
    std::list<std::unique_ptr<rollback_event>> m_rollback_events;
    cryptonote::Blockchain& m_blockchain;

    using block_height = uint64_t;
    std::map<block_height, std::shared_ptr<quorum_state>> m_quorum_states;
  };

  const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
}
