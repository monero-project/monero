// Copyright (c)      2018, The Loki Project
// Copyright (c)      2019, Project Triton
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
  const size_t QUORUM_SIZE                    = 10;
  const size_t MIN_VOTES_TO_KICK_SERVICE_NODE = 7;
  const size_t NTH_OF_THE_NETWORK_TO_TEST     = 100;
  const size_t MIN_NODES_TO_TEST              = 50;

  class quorum_cop;

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
    void register_hooks(service_nodes::quorum_cop &quorum_cop);
    void init();
    bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward);

    std::vector<crypto::public_key> get_expired_nodes(uint64_t block_height) const;

    std::vector<std::pair<cryptonote::account_public_address, uint32_t>> get_winner_addresses_and_portions(const crypto::hash& prev_id) const;
    crypto::public_key select_winner(const crypto::hash& prev_id) const;


    bool is_service_node(const crypto::public_key& pubkey) const;
    const std::shared_ptr<quorum_state> get_quorum_state(uint64_t height) const;


  private:
    struct service_node_info
   {
     struct contribution {
        uint64_t amount;
        cryptonote::account_public_address address;
      };
     // block_height and transaction_index are to record when the service node
     // is registered or when it last received a reward.
     //
     // set the winning service node as though it was re-registering at the
     // block height it wins on, with transaction index=-1
     // (hence transaction_index is signed)

     uint64_t last_reward_block_height;
     uint32_t last_reward_transaction_index;
     std::vector<cryptonote::account_public_address> addresses;
     std::vector<uint32_t> portions;
     std::vector<contribution> contributions;
      uint64_t total_contributions;
      uint64_t staking_requirement;

      bool is_fully_funded() const { return total_contributions >= staking_requirement; }
   };

   bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const;
   void process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index);
   void process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
   void process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height);


   std::vector<crypto::public_key> get_service_nodes_pubkeys() const;
    template<typename T>
    void block_added_generic(const cryptonote::block& block, const T& txs);

    bool contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const;
    bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint32_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key) const;
    uint64_t get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev) const;

    uint64_t get_min_contribution(uint64_t height) const;

    crypto::public_key find_service_node_from_miner_tx(const cryptonote::transaction& miner_tx, uint64_t block_height) const;
    void store_quorum_state_from_rewards_list(uint64_t height);

    class rollback_event
    {
    public:
      rollback_event(uint64_t block_height);
      virtual ~rollback_event() { }
      virtual bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const = 0;
      uint64_t m_block_height;
    };

    class rollback_change : public rollback_event
    {
    public:
      rollback_change(uint64_t block_height, const crypto::public_key& key, const service_node_info& info);
      bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const;
    private:
      crypto::public_key m_key;
      service_node_info m_info;
    };

    class rollback_new : public rollback_event
    {
    public:
      rollback_new(uint64_t block_height, cryptonote::account_public_address address);
      bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const;
    private:
      cryptonote::account_public_address m_address;
    };

    class prevent_rollback : public rollback_event
    {
    public:
      prevent_rollback(uint64_t block_height);
      bool apply(std::unordered_map<cryptonote::account_public_address, std::pair<uint64_t, size_t>>& service_nodes_last_reward, std::unordered_map<cryptonote::account_public_address, crypto::public_key>& service_nodes_keys) const;
    };

    std::unordered_map<crypto::public_key, service_node_info> m_service_nodes_infos;
    std::list<std::unique_ptr<rollback_event>> m_rollback_events;
    cryptonote::Blockchain& m_blockchain;
    bool m_hooks_registered;

    using block_height = uint64_t;
   std::map<block_height, std::shared_ptr<quorum_state>> m_quorum_states;

  };
  bool convert_registration_args(cryptonote::network_type nettype, const std::vector<std::string>& args, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint32_t>& portions, uint64_t& initial_contribution);
  bool make_registration_cmd(cryptonote::network_type nettype, const std::vector<std::string> args, const crypto::public_key& service_node_pubkey,
                           const crypto::secret_key service_node_key, std::string &cmd, bool make_friendly);
  const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
}
