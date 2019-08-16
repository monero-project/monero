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

#include <boost/variant.hpp>
#include "serialization/serialization.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_core/service_node_rules.h"
#include "cryptonote_core/service_node_voting.h"
#include "cryptonote_core/service_node_quorum_cop.h"

namespace cryptonote { class Blockchain; class BlockchainDB; }

namespace service_nodes
{
  struct proof_info
  {
    uint64_t timestamp     = 0;
    uint16_t version_major = 0, version_minor = 0, version_patch = 0;
    std::array<bool, CHECKPOINT_MIN_QUORUMS_NODE_MUST_VOTE_IN_BEFORE_DEREGISTER_CHECK> votes;
    uint8_t vote_index = 0;
    std::array<std::pair<uint32_t, uint64_t>, 2> public_ips = {}; // (not serialized)
    proof_info() { votes.fill(true); }

    // Unlike the above, these two *do* get serialized, but directly from state_t rather than as a subobject:
    uint32_t public_ip;
    uint16_t storage_port;
  };

  struct service_node_info // registration information
  {
    enum version
    {
      version_0_checkpointing, // versioning reset in 4.0.0 (data structure storage changed)
    };

    struct contribution_t
    {
      uint8_t            version{version_0_checkpointing};
      crypto::public_key key_image_pub_key;
      crypto::key_image  key_image;
      uint64_t           amount;

      contribution_t() = default;
      contribution_t(uint8_t version, const crypto::public_key &pubkey, const crypto::key_image &key_image, uint64_t amount)
        : version{version}, key_image_pub_key{pubkey}, key_image{key_image}, amount{amount} {}

      BEGIN_SERIALIZE_OBJECT()
        VARINT_FIELD(version)
        FIELD(key_image_pub_key)
        FIELD(key_image)
        VARINT_FIELD(amount)
      END_SERIALIZE()
    };

    struct contributor_t
    {
      uint8_t  version;
      uint64_t amount;
      uint64_t reserved;
      cryptonote::account_public_address address;
      std::vector<contribution_t> locked_contributions;

      contributor_t() = default;
      contributor_t(uint64_t reserved_, const cryptonote::account_public_address& address_) : reserved(reserved_), address(address_)
      {
        *this    = {};
        reserved = reserved_;
        address  = address_;
      }

      BEGIN_SERIALIZE_OBJECT()
        VARINT_FIELD(version)
        VARINT_FIELD(amount)
        VARINT_FIELD(reserved)
        FIELD(address)
        FIELD(locked_contributions)
      END_SERIALIZE()
    };

    uint8_t                            version;
    uint64_t                           registration_height;
    uint64_t                           requested_unlock_height;
    // block_height and transaction_index are to record when the service node last received a reward.
    uint64_t                           last_reward_block_height;
    uint32_t                           last_reward_transaction_index;
    uint32_t                           decommission_count; // How many times this service node has been decommissioned
    int64_t                            active_since_height; // if decommissioned: equal to the *negative* height at which you became active before the decommission
    uint64_t                           last_decommission_height; // The height at which the last (or current!) decommissioning started, or 0 if never decommissioned
    std::vector<contributor_t>         contributors;
    uint64_t                           total_contributed;
    uint64_t                           total_reserved;
    uint64_t                           staking_requirement;
    uint64_t                           portions_for_operator;
    swarm_id_t                         swarm_id;
    cryptonote::account_public_address operator_address;
    uint64_t                           last_ip_change_height; // The height of the last quorum penalty for changing IPs

    // The data in `proof_info` are shared across states because we don't want to roll them back in
    // the event of a reorg: we always want them to contain the latest received info.  They are also
    // deliberately mutable (via the dereferenced non-const type) so that they can be updated
    // without needing to duplicate state.  Only public_ip and storage_port are serialized; the rest
    // of proof info is transient.
    std::shared_ptr<proof_info> proof = std::make_shared<proof_info>();

    service_node_info() = default;
    bool is_fully_funded() const { return total_contributed >= staking_requirement; }
    bool is_decommissioned() const { return active_since_height < 0; }
    bool is_active() const { return is_fully_funded() && !is_decommissioned(); }

    bool can_transition_to_state(uint8_t hf_version, uint64_t block_height, new_state proposed_state) const;
    bool can_be_voted_on        (uint64_t block_height) const;
    size_t total_num_locked_contributions() const;

    BEGIN_SERIALIZE_OBJECT()
      VARINT_FIELD(version)
      VARINT_FIELD(registration_height)
      VARINT_FIELD(requested_unlock_height)
      VARINT_FIELD(last_reward_block_height)
      VARINT_FIELD(last_reward_transaction_index)
      VARINT_FIELD(decommission_count)
      VARINT_FIELD(active_since_height)
      VARINT_FIELD(last_decommission_height)
      FIELD(contributors)
      VARINT_FIELD(total_contributed)
      VARINT_FIELD(total_reserved)
      VARINT_FIELD(staking_requirement)
      VARINT_FIELD(portions_for_operator)
      FIELD(operator_address)
      VARINT_FIELD(swarm_id)
      VARINT_FIELD_N("public_ip", proof->public_ip)
      VARINT_FIELD_N("storage_port", proof->storage_port)
      VARINT_FIELD(last_ip_change_height)
    END_SERIALIZE()
  };

  using pubkey_and_sninfo     =          std::pair<crypto::public_key, std::shared_ptr<const service_node_info>>;
  using service_nodes_infos_t = std::unordered_map<crypto::public_key, std::shared_ptr<const service_node_info>>;

  struct service_node_pubkey_info
  {
    crypto::public_key pubkey;
    std::shared_ptr<const service_node_info> info;

    service_node_pubkey_info() = default;
    service_node_pubkey_info(const pubkey_and_sninfo &pair) : pubkey{pair.first}, info{pair.second} {}

    BEGIN_SERIALIZE_OBJECT()
      FIELD(pubkey)
      if (!W)
        info = std::make_shared<service_node_info>();
      FIELD_N("info", const_cast<service_node_info &>(*info))
    END_SERIALIZE()
  };

  struct key_image_blacklist_entry
  {
    uint8_t           version{0};
    crypto::key_image key_image;
    uint64_t          unlock_height;

    key_image_blacklist_entry() = default;
    key_image_blacklist_entry(uint8_t version, const crypto::key_image &key_image, uint64_t unlock_height)
        : version{version}, key_image{key_image}, unlock_height{unlock_height} {}

    bool operator==(const key_image_blacklist_entry &other) const { return key_image == other.key_image; }
    bool operator==(const crypto::key_image &image) const { return key_image == image; }

    BEGIN_SERIALIZE()
      VARINT_FIELD(version)
      FIELD(key_image)
      VARINT_FIELD(unlock_height)
    END_SERIALIZE()
  };


  template<typename RandomIt>
  void loki_shuffle(RandomIt begin, RandomIt end, uint64_t seed)
  {
    if (end <= begin + 1) return;
    const size_t size = std::distance(begin, end);
    std::mt19937_64 mersenne_twister(seed);
    for (size_t i = 1; i < size; i++)
    {
      size_t j = (size_t)uniform_distribution_portable(mersenne_twister, i+1);
      using std::swap;
      swap(begin[i], begin[j]);
    }
  }

  class service_node_list
    : public cryptonote::BlockAddedHook,
      public cryptonote::BlockchainDetachedHook,
      public cryptonote::InitHook,
      public cryptonote::ValidateMinerTxHook
  {
  public:
    service_node_list(cryptonote::Blockchain& blockchain);
    void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
    void blockchain_detached(uint64_t height) override;
    void init() override;
    bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, cryptonote::block_reward_parts const &base_reward) const override;
    std::vector<std::pair<cryptonote::account_public_address, uint64_t>> get_winner_addresses_and_portions() const;
    crypto::public_key select_winner() const;

    bool is_service_node(const crypto::public_key& pubkey, bool require_active = true) const;
    bool is_key_image_locked(crypto::key_image const &check_image, uint64_t *unlock_height = nullptr, service_node_info::contribution_t *the_locked_contribution = nullptr) const;

    /// Note(maxim): this should not affect thread-safety as the returned object is const
    ///
    /// For checkpointing, quorums are only generated when height % CHECKPOINT_INTERVAL == 0 (and
    /// the actual internal quorum used is for `height - REORG_SAFETY_BUFFER_BLOCKS_POST_HF12`, i.e.
    /// do no subtract off the buffer in advance)
    /// return: nullptr if the quorum is not cached in memory (pruned from memory).
    std::shared_ptr<const testing_quorum> get_testing_quorum(quorum_type type, uint64_t height, bool include_old = false) const;
    quorum_manager                        get_quorum_manager(uint64_t height, bool include_old = false) const;
    bool                                  get_quorum_pubkey(quorum_type type, quorum_group group, uint64_t height, size_t quorum_index, crypto::public_key &key) const;

    std::vector<service_node_pubkey_info> get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const;
    const std::vector<key_image_blacklist_entry> &get_blacklisted_key_images() const { return m_state.key_image_blacklist; }

    void set_db_pointer(cryptonote::BlockchainDB* db);
    void set_my_service_node_keys(crypto::public_key const *pub_key);
    void set_quorum_history_storage(uint64_t hist_size); // 0 = none (default), 1 = unlimited, N = # of blocks
    bool store();

    void get_all_service_nodes_public_keys(std::vector<crypto::public_key>& keys, bool require_active) const;

    /// Record public ip and storage port and add them to the service node list
    cryptonote::NOTIFY_UPTIME_PROOF::request generate_uptime_proof(crypto::public_key const &pubkey, crypto::secret_key const &key, uint32_t public_ip, uint16_t storage_port) const;
    bool handle_uptime_proof        (cryptonote::NOTIFY_UPTIME_PROOF::request const &proof);
    void record_checkpoint_vote     (crypto::public_key const &pubkey, bool voted);

    struct quorum_for_serialization
    {
      uint8_t        version;
      uint64_t       height;
      testing_quorum quorums[static_cast<uint8_t>(quorum_type::count)];

      BEGIN_SERIALIZE()
        FIELD(version)
        FIELD(height)
        FIELD_N("obligations_quorum", quorums[static_cast<uint8_t>(quorum_type::obligations)])
        FIELD_N("checkpointing_quorum", quorums[static_cast<uint8_t>(quorum_type::checkpointing)])
      END_SERIALIZE()
    };

    struct state_serialized
    {
      enum struct version_t { version_0, count, };
      static version_t get_version(uint8_t /*hf_version*/) { return version_t::version_0; }

      version_t                              version;
      uint64_t                               height;
      std::vector<service_node_pubkey_info>  infos;
      std::vector<key_image_blacklist_entry> key_image_blacklist;
      quorum_for_serialization               quorums;
      bool                                   only_stored_quorums;

      BEGIN_SERIALIZE()
        ENUM_FIELD(version, version < version_t::count)
        VARINT_FIELD(height)
        FIELD(infos)
        FIELD(key_image_blacklist)
        FIELD(quorums)
        FIELD(only_stored_quorums)
      END_SERIALIZE()
    };

    struct data_for_serialization
    {
      enum struct version_t { version_0, count, };
      static version_t get_version(uint8_t /*hf_version*/) { return version_t::version_0; }

      version_t version;
      std::vector<quorum_for_serialization> quorum_states;
      std::vector<state_serialized>         states;
      void clear() { quorum_states.clear(); states.clear(); version = {}; }

      BEGIN_SERIALIZE()
        ENUM_FIELD(version, version < version_t::count)
        FIELD(quorum_states)
        FIELD(states)
      END_SERIALIZE()
    };

    using block_height = uint64_t;
    struct state_t
    {
      bool                                   only_loaded_quorums;
      service_nodes_infos_t                  service_nodes_infos;
      std::vector<key_image_blacklist_entry> key_image_blacklist;
      block_height                           height{0};
      mutable quorum_manager                 quorums;          // Mutable because we are allowed to (and need to) change it via std::set iterator

      state_t() = default;
      state_t(block_height height) : height{height} {}
      state_t(state_serialized &&state);

      // Returns a filtered, pubkey-sorted vector of service nodes that are active (fully funded and
      // *not* decommissioned).
      std::vector<pubkey_and_sninfo> active_service_nodes_infos() const;
      // Similar to the above, but returns all nodes that are fully funded *and* decommissioned.
      std::vector<pubkey_and_sninfo> decommissioned_service_nodes_infos() const;
    };

  private:

    // Note(maxim): private methods don't have to be protected the mutex
    // Returns true if there was a registration:
    void rescan_starting_from_curr_state(bool store_to_disk);
    bool process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index);
    // Returns true if there was a successful contribution that fully funded a service node:
    bool process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
    // Returns true if a service node changed state (deregistered, decommissioned, or recommissioned)
    bool process_state_change_tx(const cryptonote::transaction& tx, uint64_t block_height);
    void process_block(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs);
    void update_swarms(uint64_t height);

    bool contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const;

    bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const;
    std::vector<crypto::public_key> update_and_get_expired_nodes(const std::vector<cryptonote::transaction> &txs, uint64_t block_height);

    void reset(bool delete_db_entry = false);
    bool load(uint64_t current_height);

    mutable boost::recursive_mutex m_sn_mutex;
    cryptonote::Blockchain&        m_blockchain;
    crypto::public_key const      *m_service_node_pubkey;
    cryptonote::BlockchainDB      *m_db;
    uint64_t                       m_store_quorum_history;

    struct quorums_by_height
    {
      quorums_by_height() = default;
      quorums_by_height(uint64_t height, quorum_manager quorums) : height(height), quorums(std::move(quorums)) {}
      uint64_t       height;
      quorum_manager quorums;
    };

    struct state_t_less {
        constexpr bool operator()(const state_t &lhs, const state_t &rhs) const { return lhs.height < rhs.height; }
    };

    std::deque<quorums_by_height>          m_old_quorum_states; // Store all old quorum history only if run with --store-full-quorum-history
    std::set<state_t, state_t_less>        m_state_history;
    state_t                                m_state;
  };

  bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key);

  struct converted_registration_args
  {
    bool                                            success;
    std::vector<cryptonote::account_public_address> addresses;
    std::vector<uint64_t>                           portions;
    uint64_t                                        portions_for_operator;
    std::string                                     err_msg; // if (success == false), this is set to the err msg otherwise empty
  };
  converted_registration_args convert_registration_args(cryptonote::network_type nettype,
                                                        const std::vector<std::string>& args,
                                                        uint64_t staking_requirement,
                                                        uint8_t hf_version);

  bool make_registration_cmd(cryptonote::network_type nettype,
      uint8_t hf_version,
      uint64_t staking_requirement,
      const std::vector<std::string>& args,
      const crypto::public_key& service_node_pubkey,
      const crypto::secret_key &service_node_key,
      std::string &cmd,
      bool make_friendly,
      boost::optional<std::string&> err_msg);

  const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
  const static std::vector<std::pair<cryptonote::account_public_address, uint64_t>> null_winner =
    {std::pair<cryptonote::account_public_address, uint64_t>({null_address, STAKING_PORTIONS})};
}
