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
#include "cryptonote_core/service_node_rules.h"
#include "cryptonote_core/service_node_deregister.h"
// #include "eth_adapter/eth_adapter.h"
#include <list>

namespace cryptonote { class Blockchain; class BlockchainDB; }
namespace service_nodes
{
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

	struct contract
	{
		struct payment
		{
			uint64_t amount;
		};

		uint64_t creation_height;
		crypto::hash creation_hash;
		std::pair<uint64_t, crypto::hash> last_update;
		uint64_t rate;
	};

	struct service_node_info // registration information
	{
	  struct contribution_t
	  {
	    uint8_t version = version_3_lc;
	    crypto::public_key key_image_pub_key;
	    crypto::key_image key_image;
	    uint64_t amount;

	    BEGIN_SERIALIZE_OBJECT()
	      VARINT_FIELD(version)
	      FIELD(key_image_pub_key)
	      FIELD(key_image)
	      VARINT_FIELD(amount)
	    END_SERIALIZE()
	  };

		enum version
		{
			version_0,
			version_1_swarms,
			version_pool_upgrade,
			version_3_lc,
		};

		struct contributor_t
		{
		  uint8_t version;
			uint64_t amount;
			uint64_t reserved;
			cryptonote::account_public_address address;
			std::vector<contribution_t> locked_contributions;
			contributor_t() = default;
			contributor_t(uint64_t reserved_, const cryptonote::account_public_address& address_) : reserved(reserved_), address(address_)
			{
			  *this = {};
			  reserved = reserved_;
			  address = address_;
			}

			BEGIN_SERIALIZE_OBJECT()
			  VARINT_FIELD(version)
				VARINT_FIELD(amount)
				VARINT_FIELD(reserved)
				FIELD(address)
				if (version >= version_3_lc)
				  FIELD(locked_contributions)
			END_SERIALIZE()
		};

		uint8_t  version;
		uint64_t registration_height;
		uint64_t requested_unlock_height;
		uint64_t last_reward_block_height;
		uint32_t last_reward_transaction_index;
		std::vector<contributor_t> contributors;
		uint64_t total_contributed;
		uint64_t total_reserved;
		uint64_t staking_requirement;
		uint64_t portions_for_operator;
		swarm_id_t swarm_id;
		cryptonote::account_public_address operator_address;

		service_node_info() = default;

		bool is_valid() const { return total_contributed >= total_reserved; }
		bool is_fully_funded() const { return total_contributed >= staking_requirement; }
		size_t total_num_locked_contributions() const;

		BEGIN_SERIALIZE_OBJECT()
			VARINT_FIELD(version)
			VARINT_FIELD(registration_height)
			VARINT_FIELD(requested_unlock_height)
			VARINT_FIELD(last_reward_block_height)
			VARINT_FIELD(last_reward_transaction_index)
			FIELD(contributors)
			VARINT_FIELD(total_contributed)
			VARINT_FIELD(total_reserved)
			VARINT_FIELD(staking_requirement)
			VARINT_FIELD(portions_for_operator)
			FIELD(operator_address)
			VARINT_FIELD(swarm_id)
		END_SERIALIZE()
	};

	struct service_node_pubkey_info
	{
		crypto::public_key pubkey;
		service_node_info  info;

		BEGIN_SERIALIZE_OBJECT()
		  FIELD(pubkey)
		  FIELD(info)
		END_SERIALIZE()
	};

	struct key_image_blacklist_entry
	{
	  uint8_t version = service_node_info::version_3_lc;
	  crypto::key_image key_image;
	  uint64_t unlock_height;

	  BEGIN_SERIALIZE()
	    VARINT_FIELD(version)
	    FIELD(key_image)
	    VARINT_FIELD(unlock_height)
	  END_SERIALIZE()
	};

	template<typename T>
	void triton_shuffle(std::vector<T>& a, uint64_t seed)
	{
	  if (a.size() <= 1) return;
	  std::mt19937_64 mersenne_twister(seed);
	  for (size_t i = 1; i < a.size(); i++)
	  {
	    size_t j = (size_t)uniform_distribution_portable(mersenne_twister, i+1);
	    if (i != j)
	      std::swap(a[i], a[j]);
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
		~service_node_list();
		void block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs) override;
		void blockchain_detached(uint64_t height) override;
		void init() override;
		bool validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, uint8_t hard_fork_version, cryptonote::block_reward_parts const &reward_parts) const override;
		std::vector<std::pair<cryptonote::account_public_address, uint64_t>> get_winner_addresses_and_portions() const;
		crypto::public_key select_winner() const;

		std::vector<crypto::public_key> get_service_nodes_pubkeys() const;
		bool is_service_node(const crypto::public_key& pubkey) const;
		bool is_key_image_locked(crypto::key_image const &check_image, uint64_t *unlock_height = nullptr, service_node_info::contribution_t *the_locked_contribution = nullptr) const;
		void update_swarms(uint64_t height);

		/// Note(maxim): this should not affect thread-safety as the returned object is const
		const std::shared_ptr<const quorum_state> get_quorum_state(uint64_t height) const;
		std::vector<service_node_pubkey_info> get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const;
		const std::vector<key_image_blacklist_entry> &get_blacklisted_key_images() const { return m_key_image_blacklist; }

		void set_db_pointer(cryptonote::BlockchainDB* db);
		void set_my_service_node_keys(crypto::public_key const *pub_key);
		bool store();

		struct rollback_event
		{
			enum rollback_type
			{
				change_type,
				new_type,
				prevent_type,
				key_image_blacklist_type,
			};

			rollback_event() = default;
			rollback_event(uint64_t block_height, rollback_type type);
			virtual ~rollback_event() { }

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

			BEGIN_SERIALIZE()
				FIELDS(*static_cast<rollback_event *>(this))
				END_SERIALIZE()
		};

		struct rollback_key_image_blacklist : public rollback_event
		{
		  rollback_key_image_blacklist() { *this = {}; type = key_image_blacklist_type; }
		  rollback_key_image_blacklist(uint64_t block_height, key_image_blacklist_entry const &entry, bool is_adding_to_blacklist);

		  key_image_blacklist_entry m_entry;
		  bool m_was_adding_to_blacklist;

		  BEGIN_SERIALIZE()
		    FIELDS(*static_cast<rollback_event *>(this))
		    FIELD(m_entry)
		    FIELD(m_was_adding_to_blacklist)
		  END_SERIALIZE()
		};
		typedef boost::variant<rollback_change, rollback_new, prevent_rollback, rollback_key_image_blacklist> rollback_event_variant;

		struct quorum_state_for_serialization
		{
		  uint8_t version;
			uint64_t height;
			quorum_state state;

			BEGIN_SERIALIZE()
			  FIELD(version)
				FIELD(height)
				FIELD(state)
			END_SERIALIZE()
		};

		struct contract_info_for_serialization
		{
			uint64_t height; //register height
			crypto::hash registerHash; //register hash;
			uint64_t balance; //balance of contract for payments
			std::pair<uint64_t, std::string> last_data; //index of last data submission / data
			
			BEGIN_SERIALIZE()
				FIELD(height)
				FIELD(registerHash)
				FIELD(balance)
				FIELD(last_data)
			END_SERIALIZE()		
		};

		struct data_members_for_serialization
		{
		  uint8_t version;
		  uint64_t height;
			std::vector<quorum_state_for_serialization> quorum_states;
			std::vector<service_node_pubkey_info> infos;
			std::vector<rollback_event_variant> events;
			std::vector<key_image_blacklist_entry> key_image_blacklist;
			std::vector<contract_info_for_serialization> contracts;
			//std::vector<contract_event_variant> contract_events;

			BEGIN_SERIALIZE()
			  VARINT_FIELD(version)
				FIELD(quorum_states)
				FIELD(infos)
				FIELD(events)
				FIELD(contracts)
				FIELD(height)
				if (version >= service_node_info::version_3_lc)
				  FIELD(key_image_blacklist)
			END_SERIALIZE()
		};

	private:

		// Note(maxim): private methods don't have to be protected the mutex
		bool process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index);
		void process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
		bool process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height);
		bool process_swap_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index);
		void process_block(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs);

		bool contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const;

		void store_quorum_state_from_rewards_list(uint64_t height);

		bool is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const;
		std::vector<crypto::public_key> get_expired_nodes(uint64_t block_height) const;

		void clear(bool delete_db_entry = false);
		bool load();

		std::unordered_map<crypto::public_key, service_node_info> m_service_nodes_infos;
		std::list<std::unique_ptr<rollback_event>> m_rollback_events;
		cryptonote::Blockchain& m_blockchain;

		using block_height = uint64_t;
		block_height m_height;

    mutable boost::recursive_mutex m_sn_mutex;

		crypto::public_key const *m_service_node_pubkey;

		cryptonote::BlockchainDB* m_db;

		std::vector<key_image_blacklist_entry> m_key_image_blacklist;
		std::map<block_height, std::shared_ptr<const quorum_state>> m_quorum_states;

		std::vector<contract> m_contracts;
	};

	bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key);

	struct converted_registration_args
	{
	  bool success;
	  std::vector<cryptonote::account_public_address> addresses;
	  std::vector<uint64_t> portions;
	  uint64_t portions_for_operator;
	  std::string err_msg;
	};
	converted_registration_args convert_registration_args(cryptonote::network_type nettype, const std::vector<std::string>& args, uint64_t staking_requirement, uint8_t hard_fork_version);

	bool make_registration_cmd(cryptonote::network_type nettype, uint8_t hard_fork_version, uint64_t staking_requirement, const std::vector<std::string>& args, const crypto::public_key& service_node_pubkey, const crypto::secret_key &service_node_key, std::string &cmd, bool make_friendly, boost::optional<std::string&> err_msg);

	const static cryptonote::account_public_address null_address{ crypto::null_pkey, crypto::null_pkey };
	const static std::vector<std::pair<cryptonote::account_public_address, uint64_t>> null_winner = { std::pair<cryptonote::account_public_address, uint64_t>({ null_address, STAKING_PORTIONS }) };
}

VARIANT_TAG(binary_archive, service_nodes::service_node_list::data_members_for_serialization, 0xa0);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_change, 0xa1);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_new, 0xa2);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::prevent_rollback, 0xa3);
VARIANT_TAG(binary_archive, service_nodes::service_node_list::rollback_key_image_blacklist, 0xa4);