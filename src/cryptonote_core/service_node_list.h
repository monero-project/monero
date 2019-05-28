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
#include <boost/variant.hpp>
#include "serialization/serialization.h"
#include "cryptonote_core/service_node_rules.h"

namespace service_nodes
{
	constexpr size_t QUORUM_SIZE = 10;
	constexpr size_t MIN_VOTES_TO_KICK_SERVICE_NODE = 7;
	constexpr size_t NTH_OF_THE_NETWORK_TO_TEST = 100;
	constexpr size_t MIN_NODES_TO_TEST = 50;

	constexpr size_t MAX_SWARM_SIZE = 10;
	// We never create a new swarm unless there are SWARM_BUFFER extra nodes
	// available in the queue.
	constexpr size_t SWARM_BUFFER = 5;
	// if a swarm has strictly less nodes than this, it is considered unhealthy
	// and nearby swarms will mirror it's data. It will disappear, and is already considered gone.
	constexpr size_t MIN_SWARM_SIZE = 5;

	class quorum_cop;

	struct quorum_state
	{
		std::vector<crypto::public_key> quorum_nodes;
		std::vector<crypto::public_key> nodes_to_test;

		BEGIN_SERIALIZE()
			FIELD(quorum_nodes)
			FIELD(nodes_to_test)
			END_SERIALIZE()
	};

	using swarm_id_t = uint64_t;

	struct service_node_info // registration information
	{
		enum version
		{
			version_0,
			version_1_swarms
		};

		struct contribution
		{
			uint64_t amount;
			uint64_t reserved;
			cryptonote::account_public_address address;
			contribution() {}
			contribution(uint64_t _reserved, const cryptonote::account_public_address& _address)
				: amount(0), reserved(_reserved), address(_address) { }

			BEGIN_SERIALIZE()
				VARINT_FIELD(amount)
				VARINT_FIELD(reserved)
				FIELD(address)
				END_SERIALIZE()
		};

		uint8_t  version = service_node_info::version_0;
		uint64_t registration_height;

		// block_height and transaction_index are to record when the service node last received a reward.
		uint64_t last_reward_block_height;
		uint32_t last_reward_transaction_index;

		std::vector<contribution> contributors;
		uint64_t total_contributed;
		uint64_t total_reserved;
		uint64_t staking_requirement;
		uint64_t portions_for_operator;
		swarm_id_t swarm_id;
		cryptonote::account_public_address operator_address;

		bool is_fully_funded() const { return total_contributed >= staking_requirement; }
		// the minimum contribution to start a new contributor
		uint64_t get_min_contribution() const;

		service_node_info() = default;

		BEGIN_SERIALIZE()
			VARINT_FIELD(version)
			VARINT_FIELD(registration_height)
			VARINT_FIELD(last_reward_block_height)
			VARINT_FIELD(last_reward_transaction_index)
			FIELD(contributors)
			VARINT_FIELD(total_contributed)
			VARINT_FIELD(total_reserved)
			VARINT_FIELD(staking_requirement)
			VARINT_FIELD(portions_for_operator)
			if (version >= service_node_info::version_1_swarms) {
				VARINT_FIELD(swarm_id)
			}
		FIELD(operator_address)
			END_SERIALIZE()
	};

	struct service_node_pubkey_info
	{
		crypto::public_key pubkey;
		service_node_info  info;
	};

	template<typename T>
	void triton_shuffle(std::vector<T>& a, uint64_t seed);

	static constexpr uint64_t QUEUE_SWARM_ID = 0;

	class service_node_list
		: public cryptonote::Blockchain::BlockAddedHook,
		public cryptonote::Blockchain::BlockchainDetachedHook,
		public cryptonote::Blockchain::InitHook,
		public cryptonote::Blockchain::ValidateMinerTxHook
	{
	public:
		service_node_list(cryptonote::Blockchain& blockchain);
		void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
		void blockchain_detached(uint64_t height) override;
		void register_hooks(service_nodes::quorum_cop &quorum_cop);
		void init() override;
		bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, cryptonote::block_reward_parts const &reward_parts) const override;
		std::vector<std::pair<cryptonote::account_public_address, uint64_t>> get_winner_addresses_and_portions(const crypto::hash& prev_id) const;
		crypto::public_key select_winner(const crypto::hash& prev_id) const;

		bool is_service_node(const crypto::public_key& pubkey) const;

		void update_swarms(uint64_t height);

		/// Note(maxim): this should not affect thread-safety as the returned object is const
		const std::shared_ptr<const quorum_state> get_quorum_state(uint64_t height) const;
		std::vector<service_node_pubkey_info> get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const;

		void set_db_pointer(cryptonote::BlockchainDB* db);
		void set_my_service_node_keys(crypto::public_key const *pub_key);
		bool store();

		struct rollback_event
		{
			enum rollback_type
			{
				change_type,
				new_type,
				prevent_type
			};

			rollback_event() = default;
			rollback_event(uint64_t block_height, rollback_type type);
			virtual ~rollback_event() { }
			virtual bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const = 0;

			rollback_type type;

			uint64_t m_block_height;

			BEGIN_SERIALIZE()
				VARINT_FIELD(m_block_height)
				END_SERIALIZE()
		};

		struct rollback_change : public rollback_event
		{
			rollback_change() { type = change_type; }
			rollback_change(uint64_t block_height, const crypto::public_key& key, const service_node_info& info);
			bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const;
			crypto::public_key m_key;
			service_node_info m_info;

			BEGIN_SERIALIZE()
				FIELDS(*static_cast<rollback_event *>(this))
				FIELD(m_key)
				FIELD(m_info)
				END_SERIALIZE()
		};

		struct rollback_new : public rollback_event
		{
			rollback_new() { type = new_type; }
			rollback_new(uint64_t block_height, const crypto::public_key& key);
			bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const;
			crypto::public_key m_key;

			BEGIN_SERIALIZE()
				FIELDS(*static_cast<rollback_event *>(this))
				FIELD(m_key)
				END_SERIALIZE()
		};

		struct prevent_rollback : public rollback_event
		{
			prevent_rollback() { type = prevent_type; }
			prevent_rollback(uint64_t block_height);
			bool apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const;

			BEGIN_SERIALIZE()
				FIELDS(*static_cast<rollback_event *>(this))
				END_SERIALIZE()
		};

		typedef boost::variant<rollback_change, rollback_new, prevent_rollback> rollback_event_variant;

		struct node_info_for_serialization
		{
			crypto::public_key key;
			service_node_info info;

			BEGIN_SERIALIZE()
				FIELD(key)
				FIELD(info)
				END_SERIALIZE()
		};

		struct quorum_state_for_serialization
		{
			uint64_t height;
			quorum_state state;

			BEGIN_SERIALIZE()
				FIELD(height)
				FIELD(state)
				END_SERIALIZE()
		};

		struct data_members_for_serialization
		{
			std::vector<quorum_state_for_serialization> quorum_states;
			std::vector<node_info_for_serialization> infos;
			std::vector<rollback_event_variant> events;
			uint64_t height;

			BEGIN_SERIALIZE()
				FIELD(quorum_states)
				FIELD(infos)
				FIELD(events)
				FIELD(height)
				END_SERIALIZE()
		};

	private:

		// Note(maxim): private methods don't have to be protected the mutex
		bool get_contribution(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address, uint64_t& transferred) const;

		bool process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index);
		void process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
		bool process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height);

		std::vector<crypto::public_key> get_service_nodes_pubkeys() const;

		template<typename T>
		void block_added_generic(const cryptonote::block& block, const T& txs);

		bool contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const;

		void store_quorum_state_from_rewards_list(uint64_t height);

		bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const;
		std::vector<crypto::public_key> get_expired_nodes(uint64_t block_height) const;

		void clear(bool delete_db_entry = false);
		bool load();

		mutable boost::recursive_mutex m_sn_mutex;

		using block_height = uint64_t;

		std::unordered_map<crypto::public_key, service_node_info> m_service_nodes_infos;
		std::list<std::unique_ptr<rollback_event>> m_rollback_events;
		cryptonote::Blockchain& m_blockchain;
		bool m_hooks_registered;
		block_height m_height;

		crypto::public_key const *m_service_node_pubkey;

		cryptonote::BlockchainDB* m_db;

		std::map<block_height, std::shared_ptr<const quorum_state>> m_quorum_states;
	};

	uint64_t get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev);
	bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key);

	bool convert_registration_args(cryptonote::network_type nettype, std::vector<std::string> args, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint64_t>& portions, uint64_t& portions_for_operator, bool& autostake);
	bool make_registration_cmd(cryptonote::network_type nettype, const std::vector<std::string> args, const crypto::public_key& service_node_pubkey,
		const crypto::secret_key service_node_key, std::string &cmd, bool make_friendly);

	const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
	const static std::vector<std::pair<cryptonote::account_public_address, uint64_t>> null_winner =
	{ std::pair<cryptonote::account_public_address, uint64_t>({ null_address, STAKING_PORTIONS }) };
}

VARIANT_TAG(binary_archive, service_nodes::service_node_list::data_members_for_serialization, 0xa0);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_change, 0xa1);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_new, 0xa2);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::prevent_rollback, 0xa3);