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

namespace service_nodes
{
	const size_t QUORUM_SIZE = 10;
	const size_t MIN_VOTES_TO_KICK_SERVICE_NODE = 7;
	const size_t NTH_OF_THE_NETWORK_TO_TEST = 100;
	const size_t MIN_NODES_TO_TEST = 50;

	class quorum_cop;

	struct quorum_state
	{
		void clear() { quorum_nodes.clear(); nodes_to_test.clear(); }
		std::vector<crypto::public_key> quorum_nodes;
		std::vector<crypto::public_key> nodes_to_test;

		BEGIN_SERIALIZE()
			FIELD(quorum_nodes)
			FIELD(nodes_to_test)
			END_SERIALIZE()
	};

	struct service_node_info // registration information
	{
		uint8_t version;

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

		uint64_t registration_height;

		// block_height and transaction_index are to record when the service node last received a reward.
		uint64_t last_reward_block_height;
		uint32_t last_reward_transaction_index;

		std::vector<contribution> contributors;
		uint64_t total_contributed;
		uint64_t total_reserved;
		uint64_t staking_requirement;
		uint64_t portions_for_operator;
		cryptonote::account_public_address operator_address;

		bool is_fully_funded() const { return total_contributed >= staking_requirement; }
		// the minimum contribution to start a new contributor
		uint64_t get_min_contribution() const { return std::min(staking_requirement - total_reserved, staking_requirement / MAX_NUMBER_OF_CONTRIBUTORS); }

		service_node_info() : version(0) {}

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
			FIELD(operator_address)
			END_SERIALIZE()
	};

	struct service_node_pubkey_info
	{
		crypto::public_key pubkey;
		service_node_info  info;
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

		std::vector<std::pair<cryptonote::account_public_address, uint64_t>> get_winner_addresses_and_portions(const crypto::hash& prev_id) const;
		crypto::public_key select_winner(const crypto::hash& prev_id) const;

		bool is_service_node(const crypto::public_key& pubkey) const;
		const std::shared_ptr<quorum_state> get_quorum_state(uint64_t height) const;
		std::vector<service_node_pubkey_info> get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const;

		void set_db_pointer(cryptonote::BlockchainDB* db) { m_db = db; }
		bool store();

		bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const;
		bool get_contribution(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address, uint64_t& transferred) const;

		void process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index);
		void process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
		void process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height);

		std::vector<crypto::public_key> get_service_nodes_pubkeys() const;

		template<typename T>
		void block_added_generic(const cryptonote::block& block, const T& txs);

		bool contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const;
		bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key) const;
		uint64_t get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev) const;

		crypto::public_key find_service_node_from_miner_tx(const cryptonote::transaction& miner_tx, uint64_t block_height) const;

		void store_quorum_state_from_rewards_list(uint64_t height);

	public:
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

		void clear(bool delete_db_entry = false);
		bool load();

		using block_height = uint64_t;

		std::unordered_map<crypto::public_key, service_node_info> m_service_nodes_infos;
		std::list<std::unique_ptr<rollback_event>> m_rollback_events;
		cryptonote::Blockchain& m_blockchain;
		bool m_hooks_registered;
		block_height m_height;

		cryptonote::BlockchainDB* m_db;

		std::map<block_height, std::shared_ptr<quorum_state>> m_quorum_states;
	};

	bool convert_registration_args(cryptonote::network_type nettype, std::vector<std::string> args, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint64_t>& portions, uint64_t& portions_for_operator, bool& autostake);
	bool make_registration_cmd(cryptonote::network_type nettype, const std::vector<std::string> args, const crypto::public_key& service_node_pubkey,
		const crypto::secret_key service_node_key, std::string &cmd, bool make_friendly);

	uint64_t get_staking_requirement_lock_blocks(cryptonote::network_type m_nettype);

	uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height);

	uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

	const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
}

VARIANT_TAG(binary_archive, service_nodes::service_node_list::data_members_for_serialization, 0xa0);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_change, 0xa1);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_new, 0xa2);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::prevent_rollback, 0xa3);