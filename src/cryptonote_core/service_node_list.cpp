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

#include <functional>
#include <random>
#include <algorithm>

#include "ringct/rctSigs.h"
#include "wallet/wallet2.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "common/int-util.h"
#include "common/scoped_message_writer.h"
#include "common/i18n.h"
#include "quorum_cop.h"
#include "common/exp2.h"

#include "service_node_list.h"
#include "service_node_rules.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{
	static uint64_t uniform_distribution_portable(std::mt19937_64& mersenne_twister, uint64_t n)
	{
		uint64_t secureMax = mersenne_twister.max() - mersenne_twister.max() % n;
		uint64_t x;
		do x = mersenne_twister(); while (x >= secureMax);
		return  x / (secureMax / n);
	}

	uint64_t service_node_info::get_min_contribution() const
	{
		uint64_t result = get_min_node_contribution(staking_requirement, total_reserved);
		return result;
	}

	service_node_list::service_node_list(cryptonote::Blockchain& blockchain)
		: m_blockchain(blockchain), m_hooks_registered(false), m_height(0), m_db(nullptr), m_service_node_pubkey(nullptr)
	{
	}

	void service_node_list::register_hooks(service_nodes::quorum_cop &quorum_cop)
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		if (!m_hooks_registered)
		{
			m_hooks_registered = true;
			m_blockchain.hook_block_added(*this);
			m_blockchain.hook_blockchain_detached(*this);
			m_blockchain.hook_init(*this);
			m_blockchain.hook_validate_miner_tx(*this);

			// NOTE: There is an implicit dependency on service node lists hooks
			m_blockchain.hook_init(quorum_cop);
			m_blockchain.hook_block_added(quorum_cop);
			m_blockchain.hook_blockchain_detached(quorum_cop);
		}
	}

	void service_node_list::init()
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		if (m_blockchain.get_current_hard_fork_version() < 5)
		{
			clear(true);
			return;
		}

		uint64_t current_height = m_blockchain.get_current_blockchain_height();
		bool loaded = load();

		if (loaded && m_height == current_height) return;

		if (!loaded || m_height > current_height) clear(true);

		LOG_PRINT_L0("Recalculating service nodes list, scanning blockchain from height " << m_height);
		LOG_PRINT_L0("This may take some time...");

		std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
		while (m_height < current_height)
		{
			blocks.clear();
			if (!m_blockchain.get_blocks(m_height, 1000, blocks))
			{
				LOG_ERROR("Unable to initialize service nodes list");
				return;
			}

			std::vector<cryptonote::transaction> txs;
			std::vector<crypto::hash> missed_txs;
			for (const auto& block_pair : blocks)
			{
				txs.clear();
				missed_txs.clear();

				const cryptonote::block& block = block_pair.second;
				std::vector<cryptonote::transaction> txs;
				std::vector<crypto::hash> missed_txs;
				if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
				{
					LOG_ERROR("Unable to get transactions for block " << block.hash);
					return;
				}

				block_added_generic(block, txs);
			}
		}
	}

	std::vector<crypto::public_key> service_node_list::get_service_nodes_pubkeys() const
	{
		std::vector<crypto::public_key> result;
		for (const auto& iter : m_service_nodes_infos)
			if (iter.second.is_fully_funded())
				result.push_back(iter.first);

		std::sort(result.begin(), result.end(),
			[](const crypto::public_key &a, const crypto::public_key &b) {
			return memcmp(reinterpret_cast<const void*>(&a), reinterpret_cast<const void*>(&b), sizeof(a)) < 0;
		});
		return result;
	}

	const std::shared_ptr<const quorum_state> service_node_list::get_quorum_state(uint64_t height) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		const auto &it = m_quorum_states.find(height);
		if (it == m_quorum_states.end())
		{
			// TODO(triton): Not being able to find the quorum is going to be a fatal error.
		}
		else
		{
			return it->second;
		}

		return std::make_shared<quorum_state>();
	}

	std::vector<service_node_pubkey_info> service_node_list::get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		std::vector<service_node_pubkey_info> result;

		if (service_node_pubkeys.empty())
		{
			result.reserve(m_service_nodes_infos.size());

			for (const auto &it : m_service_nodes_infos)
			{
				service_node_pubkey_info entry = {};
				entry.pubkey = it.first;
				entry.info = it.second;
				result.push_back(entry);
			}
		}
		else
		{
			result.reserve(service_node_pubkeys.size());
			for (const auto &it : service_node_pubkeys)
			{
				const auto &find_it = m_service_nodes_infos.find(it);
				if (find_it == m_service_nodes_infos.end())
					continue;

				service_node_pubkey_info entry = {};
				entry.pubkey = (*find_it).first;
				entry.info = (*find_it).second;
				result.push_back(entry);
			}
		}

		return result;
	}

	void service_node_list::set_db_pointer(cryptonote::BlockchainDB* db)
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		m_db = db;
	}

	void service_node_list::set_my_service_node_keys(crypto::public_key const *pub_key)
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		m_service_node_pubkey = pub_key;
	}

	bool service_node_list::is_service_node(const crypto::public_key& pubkey) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		return m_service_nodes_infos.find(pubkey) != m_service_nodes_infos.end();
	}

	bool service_node_list::contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const
	{
		uint64_t unlock_time = tx.unlock_time;

		if (tx.version >= cryptonote::transaction::version_3_per_output_unlock_times)
			unlock_time = tx.output_unlock_times[i];

		return unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && unlock_time >= block_height + get_staking_requirement_lock_blocks(m_blockchain.nettype());
	}

	bool reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key)
	{
		cryptonote::tx_extra_service_node_register registration;
		if (!get_service_node_register_from_tx_extra(tx.extra, registration))
			return false;
		if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, service_node_key))
			return false;

		addresses.clear();
		addresses.reserve(registration.m_public_spend_keys.size());
		for (size_t i = 0; i < registration.m_public_spend_keys.size(); i++)
			addresses.push_back(cryptonote::account_public_address{ registration.m_public_spend_keys[i], registration.m_public_view_keys[i] });

		portions_for_operator = registration.m_portions_for_operator;
		portions = registration.m_portions;
		expiration_timestamp = registration.m_expiration_timestamp;
		signature = registration.m_service_node_signature;
		tx_pub_key = cryptonote::get_tx_pub_key_from_extra(tx.extra);

		return true;
	}

	uint64_t get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev)
	{
		if (tx.vout[i].target.type() != typeid(cryptonote::txout_to_key))
		{
			return 0;
		}

		rct::key mask;
		uint64_t money_transferred = 0;

		crypto::secret_key scalar1;
		hwdev.derivation_to_scalar(derivation, i, scalar1);
		try
		{
			switch (tx.rct_signatures.type)
			{
			case rct::RCTTypeSimple:
			case rct::RCTTypeBulletproof:
				money_transferred = rct::decodeRctSimple(tx.rct_signatures, rct::sk2rct(scalar1), i, mask, hwdev);
				break;
			case rct::RCTTypeFull:
				money_transferred = rct::decodeRct(tx.rct_signatures, rct::sk2rct(scalar1), i, mask, hwdev);
				break;
			default:
				LOG_PRINT_L0("Unsupported rct type: " << tx.rct_signatures.type);
				return 0;
			}
		}
		catch (const std::exception &e)
		{
			LOG_PRINT_L0("Failed to decode input " << i);
			return 0;
		}

		return money_transferred;
	}

	bool service_node_list::process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height)
	{
		if (!tx.is_deregister_tx())
			return false;

		cryptonote::tx_extra_service_node_deregister deregister;
		if (!cryptonote::get_service_node_deregister_from_tx_extra(tx.extra, deregister))
		{
			LOG_ERROR("Transaction deregister did not have deregister data in tx extra, possibly corrupt tx in blockchain");
			return false;
		}

		const auto state = get_quorum_state(deregister.block_height);

		if (!state)
		{
			// TODO(triton): Not being able to find a quorum is fatal! We want better caching abilities.
			LOG_ERROR("Quorum state for height: " << deregister.block_height << ", was not stored by the daemon");
			return false;
		}

		if (deregister.service_node_index >= state->nodes_to_test.size())
		{
			LOG_ERROR("Service node index to vote off has become invalid, quorum rules have changed without a hardfork.");
			return false;
		}

		const crypto::public_key& key = state->nodes_to_test[deregister.service_node_index];

		auto iter = m_service_nodes_infos.find(key);
		if (iter == m_service_nodes_infos.end())
			return false;

		if (m_service_node_pubkey && *m_service_node_pubkey == key)
		{
			MGINFO_RED("Deregistration for service node (yours): " << key);
		}
		else
		{
			LOG_PRINT_L1("Deregistration for service node: " << key);
		}

		m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, key, iter->second)));
		m_service_nodes_infos.erase(iter);

		return true;
	}

	static uint64_t get_new_swarm_id(std::mt19937_64& mt, const std::vector<swarm_id_t>& ids)
	{
		uint64_t id_new = QUEUE_SWARM_ID;

		while (id_new == QUEUE_SWARM_ID || (std::find(ids.begin(), ids.end(), id_new) != ids.end())) {
			id_new = uniform_distribution_portable(mt, UINT64_MAX);
		}

		return id_new;
	}

	static std::vector<swarm_id_t> get_all_swarms(const std::map<swarm_id_t, std::vector<crypto::public_key>>& swarm_to_snodes)
	{
		std::vector<swarm_id_t> all_swarms;
		all_swarms.reserve(swarm_to_snodes.size());
		for (const auto& entry : swarm_to_snodes) {
			all_swarms.push_back(entry.first);
		}
		return all_swarms;
	}

	static crypto::public_key pop_random_snode(std::mt19937_64& mt, std::vector<crypto::public_key>& vec)
	{
		const auto idx = uniform_distribution_portable(mt, vec.size());
		const auto sn_pk = vec.at(idx);
		auto it = vec.begin();
		std::advance(it, idx);
		vec.erase(it);
		return sn_pk;
	}


	static void calc_swarm_changes(std::map<swarm_id_t, std::vector<crypto::public_key>>& swarm_to_snodes, uint64_t seed)
	{
		std::mt19937_64 mersenne_twister(seed);

		std::vector<crypto::public_key> swarm_buffer = swarm_to_snodes[QUEUE_SWARM_ID];
		swarm_to_snodes.erase(QUEUE_SWARM_ID);

		auto all_swarms = get_all_swarms(swarm_to_snodes);
		std::sort(all_swarms.begin(), all_swarms.end());

		triton_shuffle(all_swarms, seed);

		const auto cmp_swarm_sizes =
			[&swarm_to_snodes](swarm_id_t lhs, swarm_id_t rhs) {
			return swarm_to_snodes.at(lhs).size() < swarm_to_snodes.at(rhs).size();
		};

		/// 1. If there are any swarms that are about to dissapear -> try to fill them first
		std::vector<swarm_id_t> starving_swarms;
		{
			std::copy_if(all_swarms.begin(),
				all_swarms.end(),
				std::back_inserter(starving_swarms),
				[&swarm_to_snodes](swarm_id_t id) { return swarm_to_snodes.at(id).size() < MIN_SWARM_SIZE; });

			for (const auto swarm_id : starving_swarms) {

				const size_t needed = MIN_SWARM_SIZE - swarm_to_snodes.at(swarm_id).size();

				for (auto j = 0u; j < needed && !swarm_buffer.empty(); ++j) {
					const auto sn_pk = pop_random_snode(mersenne_twister, swarm_buffer);
					swarm_to_snodes.at(swarm_id).push_back(sn_pk);
				}

				if (swarm_buffer.empty()) break;
			}
		}

		/// 2. Any starving swarms still left? If yes, steal nodes from larger swarms
		{
			bool can_continue = true; /// whether there are still large swarms to steal from
			for (const auto swarm_id : starving_swarms) {

				if (swarm_to_snodes.at(swarm_id).size() == MIN_SWARM_SIZE) continue;

				const auto needed = MIN_SWARM_SIZE - swarm_to_snodes.at(swarm_id).size();

				for (auto i = 0u; i < needed; ++i) {

					const auto large_swarm =
						*std::max_element(all_swarms.begin(), all_swarms.end(), cmp_swarm_sizes);

					if (swarm_to_snodes.at(large_swarm).size() <= MIN_SWARM_SIZE) {
						can_continue = false;
						break;
					}

					const crypto::public_key sn_pk = pop_random_snode(mersenne_twister, swarm_to_snodes.at(large_swarm));
					swarm_to_snodes.at(swarm_id).push_back(sn_pk);
				}

				if (!can_continue) break;
			}

		}

		/// 3. Fill in "unsaturated" swarms (with fewer than max nodes) starting from smallest
		{
			while (!swarm_buffer.empty() && !all_swarms.empty()) {

				const swarm_id_t smallest_swarm = *std::min_element(all_swarms.begin(), all_swarms.end(), cmp_swarm_sizes);

				std::vector<crypto::public_key>& swarm = swarm_to_snodes.at(smallest_swarm);

				if (swarm.size() == MAX_SWARM_SIZE) break;

				const auto sn_pk = pop_random_snode(mersenne_twister, swarm_buffer);
				swarm.push_back(sn_pk);
			}
		}

		/// 4. If there are still enough nodes for MAX_SWARM_SIZE + some safety buffer, create a new swarm
		while (swarm_buffer.size() >= MAX_SWARM_SIZE + SWARM_BUFFER) {

			/// shuffle the queue and select MAX_SWARM_SIZE last elements
			const auto new_swarm_id = get_new_swarm_id(mersenne_twister, all_swarms);

			triton_shuffle(swarm_buffer, seed + new_swarm_id);

			std::vector<crypto::public_key> selected_snodes;

			for (auto i = 0u; i < MAX_SWARM_SIZE; ++i) {

				/// get next node from the buffer
				const crypto::public_key fresh_snode = swarm_buffer.back();
				swarm_buffer.pop_back();

				/// Try replacing nodes in existing swarms
				if (swarm_to_snodes.size() > 0) {
					/// a. Select a random swarm
					const uint64_t swarm_idx = uniform_distribution_portable(mersenne_twister, swarm_to_snodes.size());
					auto it = swarm_to_snodes.begin();
					std::advance(it, swarm_idx);
					std::vector<crypto::public_key>& selected_swarm = it->second;

					/// b. Select a random snode
					const crypto::public_key snode = pop_random_snode(mersenne_twister, selected_swarm);

					/// c. Swap that node with a node in the queue, the old node will form a new swarm
					selected_snodes.push_back(snode);
					selected_swarm.push_back(fresh_snode);
				}
				else {
					/// If there are no existing swarms, create the first swarm directly from the queue
					selected_snodes.push_back(fresh_snode);
				}
			}

			swarm_to_snodes.insert({ new_swarm_id, std::move(selected_snodes) });
		}

		/// 5. If there is a swarm with less than MIN_SWARM_SIZE, decommission that swarm (should almost never happen due to the safety buffer).
		for (auto entry : swarm_to_snodes) {
			if (entry.second.size() < MIN_SWARM_SIZE) {
				LOG_PRINT_L1("swarm " << entry.first << " is DECOMMISSIONED");
				/// TODO: move data to other swarms, then put snodes back in the queue
			}
		}

		/// 6. Put nodes from the buffer back to the "buffer agnostic" data structure
		swarm_to_snodes.insert({ QUEUE_SWARM_ID, std::move(swarm_buffer) });
	}

	void service_node_list::update_swarms(uint64_t height) {

		crypto::hash hash = m_blockchain.get_block_id_by_height(height);
		uint64_t seed = 0;
		std::memcpy(&seed, hash.data, sizeof(seed));

		/// Gather existing swarms from infos
		std::map<swarm_id_t, std::vector<crypto::public_key>> existing_swarms;

		for (const auto& entry : m_service_nodes_infos) {
			const auto id = entry.second.swarm_id;
			existing_swarms[id].push_back(entry.first);
		}

		calc_swarm_changes(existing_swarms, seed);

		/// Apply changes
		for (const auto entry : existing_swarms) {

			const swarm_id_t swarm_id = entry.first;
			const std::vector<crypto::public_key>& snodes = entry.second;

			for (const auto snode : snodes) {

				auto& sn_info = m_service_nodes_infos.at(snode);
				if (sn_info.swarm_id == swarm_id) continue; /// nothing changed for this snode

															/// modify info and record the change
				m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(height, snode, sn_info)));
				sn_info.swarm_id = swarm_id;
			}

		}

	}

	bool service_node_list::is_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index, crypto::public_key& key, service_node_info& info) const
	{
		crypto::public_key tx_pub_key, service_node_key;
		std::vector<cryptonote::account_public_address> service_node_addresses;
		std::vector<uint64_t> service_node_portions;
		uint64_t portions_for_operator;
		uint64_t expiration_timestamp;
		crypto::signature signature;

		if (!reg_tx_extract_fields(tx, service_node_addresses, portions_for_operator, service_node_portions, expiration_timestamp, service_node_key, signature, tx_pub_key))
			return false;

		if (service_node_portions.size() != service_node_addresses.size() || service_node_portions.empty())
			return false;

		// check the portions
		if (!check_service_node_portions(service_node_portions)) return false;

		if (portions_for_operator > STAKING_PORTIONS)
			return false;

		// check the signature is all good

		crypto::hash hash;
		if (!get_registration_hash(service_node_addresses, portions_for_operator, service_node_portions, expiration_timestamp, hash))
			return false;
		if (!crypto::check_key(service_node_key) || !crypto::check_signature(hash, service_node_key, signature))
			return false;
		if (expiration_timestamp < block_timestamp)
			return false;

		// check the initial contribution exists

		info.staking_requirement = get_staking_requirement(m_blockchain.nettype(), block_height);

		cryptonote::account_public_address address;
		uint64_t transferred = 0;
		if (!get_contribution(tx, block_height, address, transferred))
			return false;
		if (transferred < info.staking_requirement / MAX_NUMBER_OF_CONTRIBUTORS)
			return false;
		int is_this_a_new_address = 0;
		if (std::find(service_node_addresses.begin(), service_node_addresses.end(), address) == service_node_addresses.end())
			is_this_a_new_address = 1;
		if (service_node_addresses.size() + is_this_a_new_address > MAX_NUMBER_OF_CONTRIBUTORS)
			return false;

		// don't actually process this contribution now, do it when we fall through later.

		key = service_node_key;

		info.operator_address = service_node_addresses[0];
		info.portions_for_operator = portions_for_operator;
		info.registration_height = block_height;
		info.last_reward_block_height = block_height;
		info.last_reward_transaction_index = index;
		info.total_contributed = 0;
		info.total_reserved = 0;

		const auto hf_version = m_blockchain.get_hard_fork_version(block_height);

		if (hf_version >= 5) {
			info.version = service_node_info::version_1_swarms;
			info.swarm_id = QUEUE_SWARM_ID; /// new nodes go into a "queue swarm"
		}

		info.contributors.clear();

		for (size_t i = 0; i < service_node_addresses.size(); i++)
		{
			// Check for duplicates
			auto iter = std::find(service_node_addresses.begin(), service_node_addresses.begin() + i, service_node_addresses[i]);
			if (iter != service_node_addresses.begin() + i)
				return false;
			uint64_t hi, lo, resulthi, resultlo;
			lo = mul128(info.staking_requirement, service_node_portions[i], &hi);
			div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
			info.contributors.push_back(service_node_info::contribution(resultlo, service_node_addresses[i]));
			info.total_reserved += resultlo;
		}

		return true;
	}

	bool service_node_list::process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index)
	{
		crypto::public_key key;
		service_node_info info = {};
		if (!is_registration_tx(tx, block_timestamp, block_height, index, key, info))
			return false;

		// NOTE: A node doesn't expire until registration_height + lock blocks excess now which acts as the grace period
		// So it is possible to find the node still in our list.
		bool registered_during_grace_period = false;
		const auto iter = m_service_nodes_infos.find(key);
		if (iter != m_service_nodes_infos.end())
		{
			int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);
			if (hard_fork_version >= 5)
			{
				service_node_info const &old_info = iter->second;
				uint64_t expiry_height = old_info.registration_height + get_staking_requirement_lock_blocks(m_blockchain.nettype());
				if (block_height < expiry_height)
					return false;

				// NOTE: Node preserves its position in list if it reregisters during grace period.
				registered_during_grace_period = true;
				info.last_reward_block_height = old_info.last_reward_block_height;
				info.last_reward_transaction_index = old_info.last_reward_transaction_index;
			}
			else
			{
				return false;
			}
		}

		if (m_service_node_pubkey && *m_service_node_pubkey == key)
		{
			if (registered_during_grace_period)
			{
				MGINFO_GREEN("Service node re-registered (yours): " << key << " at block height: " << block_height);
			}
			else
			{
				MGINFO_GREEN("Service node registered (yours): " << key << " at block height: " << block_height);
			}
		}
		else
		{
			LOG_PRINT_L1("New service node registered: " << key << " at block height: " << block_height);
		}

		m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_new(block_height, key)));
		m_service_nodes_infos[key] = info;

		return true;
	}

	bool service_node_list::get_contribution(const cryptonote::transaction& tx, uint64_t block_height, cryptonote::account_public_address& address, uint64_t& transferred) const
	{
		crypto::secret_key tx_key;

		if (!cryptonote::get_service_node_contributor_from_tx_extra(tx.extra, address))
			return false;
		if (!cryptonote::get_tx_secret_key_from_tx_extra(tx.extra, tx_key))
			return false;

		crypto::key_derivation derivation;
		if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
			return false;

		hw::device& hwdev = hw::get_device("default");

		transferred = 0;
		for (size_t i = 0; i < tx.vout.size(); i++)
			if (contribution_tx_output_has_correct_unlock_time(tx, i, block_height))
				transferred += get_reg_tx_staking_output_contribution(tx, i, derivation, hwdev);

		return true;
	}

	void service_node_list::process_contribution_tx(const cryptonote::transaction& tx, uint64_t block_height, uint32_t index)
	{
		crypto::public_key pubkey;
		cryptonote::account_public_address address;
		uint64_t transferred;

		if (!cryptonote::get_service_node_pubkey_from_tx_extra(tx.extra, pubkey))
			return;
		if (!get_contribution(tx, block_height, address, transferred))
			return;

		auto iter = m_service_nodes_infos.find(pubkey);
		if (iter == m_service_nodes_infos.end())
			return;

		service_node_info& info = iter->second;

		if (info.is_fully_funded())
			return;

		auto& contributors = info.contributors;

		// Only create a new contributor if they stake at least a quarter
		// and if we don't already have the maximum
		auto contrib_iter = std::find_if(contributors.begin(), contributors.end(),
			[&address](const service_node_info::contribution& contributor) { return contributor.address == address; });
		if (contrib_iter == contributors.end())
		{
			if (contributors.size() >= MAX_NUMBER_OF_CONTRIBUTORS || transferred < info.get_min_contribution())
				return;
		}

		m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, pubkey, info)));

		if (contrib_iter == contributors.end())
		{
			contributors.push_back(service_node_info::contribution(0, address));
			contrib_iter = --contributors.end();
		}

		service_node_info::contribution& contributor = *contrib_iter;

		// In this action, we cannot
		// increase total_reserved so much that it is >= staking_requirement
		uint64_t can_increase_reserved_by = info.staking_requirement - info.total_reserved;
		uint64_t max_amount = contributor.reserved + can_increase_reserved_by;
		transferred = std::min(max_amount - contributor.amount, transferred);

		contributor.amount += transferred;
		info.total_contributed += transferred;

		if (contributor.amount > contributor.reserved)
		{
			info.total_reserved += contributor.amount - contributor.reserved;
			contributor.reserved = contributor.amount;
		}

		iter->second.last_reward_block_height = block_height;
		iter->second.last_reward_transaction_index = index;

		LOG_PRINT_L1("Contribution of " << transferred << " received for service node " << pubkey);

		return;
	}

	void service_node_list::block_added(const cryptonote::block& block, const std::vector<cryptonote::transaction>& txs)
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		block_added_generic(block, txs);
		store();
	}


	template<typename T>
	void service_node_list::block_added_generic(const cryptonote::block& block, const T& txs)
	{
		uint64_t block_height = cryptonote::get_block_height(block);
		int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

		if (hard_fork_version < 5)
			return;

		assert(m_height == block_height);
		m_height++;
		{
			const size_t ROLLBACK_EVENT_EXPIRATION_BLOCKS = 30;
			uint64_t cull_height = (block_height < ROLLBACK_EVENT_EXPIRATION_BLOCKS) ? block_height : block_height - ROLLBACK_EVENT_EXPIRATION_BLOCKS;

			while (!m_rollback_events.empty() && m_rollback_events.front()->m_block_height < cull_height)
			{
				m_rollback_events.pop_front();
			}
			m_rollback_events.push_front(std::unique_ptr<rollback_event>(new prevent_rollback(cull_height)));
		}

		size_t expired_count = 0;

		for (const crypto::public_key& pubkey : get_expired_nodes(block_height))
		{
			auto i = m_service_nodes_infos.find(pubkey);
			if (i != m_service_nodes_infos.end())
			{
				if (m_service_node_pubkey && *m_service_node_pubkey == pubkey)
				{
					MGINFO_GREEN("Service node expired (yours): " << pubkey << " at block height: " << block_height);
				}
				else
				{
					LOG_PRINT_L1("Service node expired: " << pubkey << " at block height: " << block_height);
				}

				m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, pubkey, i->second)));

				expired_count++;
				m_service_nodes_infos.erase(i);
			}
			// Service nodes may expire early if they double staked by accident, so
			// expiration doesn't mean the node is in the list.
		}

		crypto::public_key winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(block.miner_tx.extra);
		if (m_service_nodes_infos.count(winner_pubkey) == 1)
		{
			m_rollback_events.push_back(
				std::unique_ptr<rollback_event>(
					new rollback_change(block_height, winner_pubkey, m_service_nodes_infos[winner_pubkey])
					)
			);
			// set the winner as though it was re-registering at transaction index=UINT32_MAX for this block
			m_service_nodes_infos[winner_pubkey].last_reward_block_height = block_height;
			m_service_nodes_infos[winner_pubkey].last_reward_transaction_index = UINT32_MAX;
		}

		size_t registrations = 0;
		size_t deregistrations = 0;

		uint32_t index = 0;
		for (const cryptonote::transaction& tx : txs)
		{
			crypto::public_key key;
			service_node_info info;
			cryptonote::account_public_address address;
			if (process_registration_tx(tx, block.timestamp, block_height, index)) {
				registrations++;
			}
			process_contribution_tx(tx, block_height, index);
			if (process_deregistration_tx(tx, block_height)) {
				deregistrations++;
			}
			index++;
		}

		if (registrations || deregistrations || expired_count) {
			update_swarms(block_height);
		}

		const size_t QUORUM_LIFETIME = (6 * triton::service_node_deregister::DEREGISTER_LIFETIME_BY_HEIGHT);
		// save six times the quorum lifetime, to be sure. also to help with debugging.
		const size_t cache_state_from_height = (block_height < QUORUM_LIFETIME) ? 0 : block_height - QUORUM_LIFETIME;

		store_quorum_state_from_rewards_list(block_height);

		while (!m_quorum_states.empty() && m_quorum_states.begin()->first < cache_state_from_height)
		{
			m_quorum_states.erase(m_quorum_states.begin());
		}
	}

	void service_node_list::blockchain_detached(uint64_t height)
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		while (!m_rollback_events.empty() && m_rollback_events.back()->m_block_height >= height)
		{
			if (!m_rollback_events.back()->apply(m_service_nodes_infos))
			{
				init();
				break;
			}
			m_rollback_events.pop_back();
		}

		while (!m_quorum_states.empty() && (--m_quorum_states.end())->first >= height)
		{
			m_quorum_states.erase(--m_quorum_states.end());
		}

		m_height = height;

		store();
	}

	std::vector<crypto::public_key> service_node_list::get_expired_nodes(uint64_t block_height) const
	{
		std::vector<crypto::public_key> expired_nodes;
		int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

		uint64_t lock_blocks = get_staking_requirement_lock_blocks(m_blockchain.nettype());
		if (hard_fork_version >= 5)
			lock_blocks += STAKING_REQUIREMENT_LOCK_BLOCKS_EXCESS;

		if (block_height < lock_blocks)
			return expired_nodes;

		if (hard_fork_version >= 5)
		{
			for (auto &it : m_service_nodes_infos)
			{
				crypto::public_key const &pubkey = it.first;
				service_node_info  const &info = it.second;

				uint64_t node_expiry_height = info.registration_height + lock_blocks;
				if (block_height > node_expiry_height)
				{
					expired_nodes.push_back(pubkey);
				}
			}
		}
		else
		{
			const uint64_t expired_nodes_block_height = block_height - lock_blocks;
			std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
			if (!m_blockchain.get_blocks(expired_nodes_block_height, 1, blocks))
			{
				LOG_ERROR("Unable to get historical blocks");
				return expired_nodes;
			}

			const cryptonote::block& block = blocks.begin()->second;
			std::vector<cryptonote::transaction> txs;
			std::vector<crypto::hash> missed_txs;
			if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
			{
				LOG_ERROR("Unable to get transactions for block " << block.hash);
				return expired_nodes;
			}

			uint32_t index = 0;
			for (const cryptonote::transaction& tx : txs)
			{
				crypto::public_key key;
				service_node_info info = {};
				if (is_registration_tx(tx, block.timestamp, expired_nodes_block_height, index, key, info))
				{
					expired_nodes.push_back(key);
				}
				index++;
			}
		}

		return expired_nodes;
	}

	std::vector<std::pair<cryptonote::account_public_address, uint64_t>> service_node_list::get_winner_addresses_and_portions(const crypto::hash& prev_id) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		crypto::public_key key = select_winner(prev_id);
		if (key == crypto::null_pkey)
			return { std::make_pair(null_address, STAKING_PORTIONS) };

		std::vector<std::pair<cryptonote::account_public_address, uint64_t>> winners;

		const service_node_info& info = m_service_nodes_infos.at(key);

		const uint64_t remaining_portions = STAKING_PORTIONS - info.portions_for_operator;

		// Add contributors and their portions to winners.
		for (const auto& contributor : info.contributors)
		{
			uint64_t hi, lo, resulthi, resultlo;
			lo = mul128(contributor.amount, remaining_portions, &hi);
			div128_64(hi, lo, info.staking_requirement, &resulthi, &resultlo);

			if (contributor.address == info.operator_address)
				resultlo += info.portions_for_operator;

			winners.push_back(std::make_pair(contributor.address, resultlo));
		}
		return winners;
	}

	crypto::public_key service_node_list::select_winner(const crypto::hash& prev_id) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		auto oldest_waiting = std::pair<uint64_t, uint32_t>(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint32_t>::max());
		crypto::public_key key = crypto::null_pkey;
		for (const auto& info : m_service_nodes_infos)
			if (info.second.is_fully_funded())
			{
				auto waiting_since = std::make_pair(info.second.last_reward_block_height, info.second.last_reward_transaction_index);
				if (waiting_since < oldest_waiting)
				{
					oldest_waiting = waiting_since;
					key = info.first;
				}
			}
		return key;
	}

	/// validates the miner TX for the next block
	//
	bool service_node_list::validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, cryptonote::block_reward_parts const &reward_parts) const
	{
		std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);
		if (hard_fork_version < 5)
			return true;

		// NOTE(triton): Service node reward distribution is calculated from the
		// original amount, i.e. 50% of the original base reward goes to service
		// nodes not 50% of the reward after removing the governance component (the
		// adjusted base reward post hardfork 10).
		uint64_t base_reward = reward_parts.original_base_reward;
		uint64_t total_service_node_reward = cryptonote::service_node_reward_formula(base_reward, hard_fork_version);

		crypto::public_key winner = select_winner(prev_id);

		crypto::public_key check_winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(miner_tx.extra);
		if (check_winner_pubkey != winner)
			return false;

		const std::vector<std::pair<cryptonote::account_public_address, uint64_t>> addresses_and_portions = get_winner_addresses_and_portions(prev_id);

		if (miner_tx.vout.size() - 1 < addresses_and_portions.size())
			return false;

		for (size_t i = 0; i < addresses_and_portions.size(); i++)
		{
			size_t vout_index = i + 1;
			uint64_t reward = cryptonote::get_portion_of_reward(addresses_and_portions[i].second, total_service_node_reward);

			if (miner_tx.vout[vout_index].amount != reward)
			{
				MERROR("Service node reward amount incorrect. Should be " << cryptonote::print_money(reward) << ", is: " << cryptonote::print_money(miner_tx.vout[vout_index].amount));
				return false;
			}

			if (miner_tx.vout[vout_index].target.type() != typeid(cryptonote::txout_to_key))
			{
				MERROR("Service node output target type should be txout_to_key");
				return false;
			}

			crypto::key_derivation derivation = AUTO_VAL_INIT(derivation);;
			crypto::public_key out_eph_public_key = AUTO_VAL_INIT(out_eph_public_key);
			cryptonote::keypair gov_key = cryptonote::get_deterministic_keypair_from_height(height);

			bool r = crypto::generate_key_derivation(addresses_and_portions[i].first.m_view_public_key, gov_key.sec, derivation);
			CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to generate_key_derivation(" << addresses_and_portions[i].first.m_view_public_key << ", " << gov_key.sec << ")");
			r = crypto::derive_public_key(derivation, vout_index, addresses_and_portions[i].first.m_spend_public_key, out_eph_public_key);
			CHECK_AND_ASSERT_MES(r, false, "while creating outs: failed to derive_public_key(" << derivation << ", " << vout_index << ", " << addresses_and_portions[i].first.m_spend_public_key << ")");

			if (boost::get<cryptonote::txout_to_key>(miner_tx.vout[vout_index].target).key != out_eph_public_key)
			{
				MERROR("Invalid service node reward output");
				return false;
			}
		}

		return true;
	}

	template<typename T>
	void triton_shuffle(std::vector<T>& a, uint64_t seed)
	{
		if (a.size() <= 1) return;
		std::mt19937_64 mersenne_twister(seed);
		for (size_t i = 1; i < a.size(); i++)
		{
			size_t j = (size_t)uniform_distribution_portable(mersenne_twister, i + 1);
			if (i != j)
				std::swap(a[i], a[j]);
		}
	}

	void service_node_list::store_quorum_state_from_rewards_list(uint64_t height)
	{
		const crypto::hash block_hash = m_blockchain.get_block_id_by_height(height);
		if (block_hash == crypto::null_hash)
		{
			MERROR("Block height: " << height << " returned null hash");
			return;
		}

		std::vector<crypto::public_key> full_node_list = get_service_nodes_pubkeys();
		std::vector<size_t>                              pub_keys_indexes(full_node_list.size());
		{
			size_t index = 0;
			for (size_t i = 0; i < full_node_list.size(); i++) { pub_keys_indexes[i] = i; }

			// Shuffle indexes
			uint64_t seed = 0;
			std::memcpy(&seed, block_hash.data, std::min(sizeof(seed), sizeof(block_hash.data)));

			triton_shuffle(pub_keys_indexes, seed);
		}

		// Assign indexes from shuffled list into quorum and list of nodes to test

		auto new_state = std::make_shared<quorum_state>();

		{
			std::vector<crypto::public_key>& quorum = new_state->quorum_nodes;
			{
				quorum.resize(std::min(full_node_list.size(), QUORUM_SIZE));
				for (size_t i = 0; i < quorum.size(); i++)
				{
					size_t node_index = pub_keys_indexes[i];
					const crypto::public_key &key = full_node_list[node_index];
					quorum[i] = key;
				}
			}

			std::vector<crypto::public_key>& nodes_to_test = new_state->nodes_to_test;
			{
				size_t num_remaining_nodes = pub_keys_indexes.size() - quorum.size();
				size_t num_nodes_to_test = std::max(num_remaining_nodes / NTH_OF_THE_NETWORK_TO_TEST,
					std::min(MIN_NODES_TO_TEST, num_remaining_nodes));

				nodes_to_test.resize(num_nodes_to_test);

				const int pub_keys_offset = quorum.size();
				for (size_t i = 0; i < nodes_to_test.size(); i++)
				{
					size_t node_index = pub_keys_indexes[pub_keys_offset + i];
					const crypto::public_key &key = full_node_list[node_index];
					nodes_to_test[i] = key;
				}
			}
		}

		m_quorum_states[height] = new_state;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	service_node_list::rollback_event::rollback_event(uint64_t block_height, rollback_type type) : m_block_height(block_height), type(type)
	{
	}

	service_node_list::rollback_change::rollback_change(uint64_t block_height, const crypto::public_key& key, const service_node_info& info)
		: service_node_list::rollback_event(block_height, change_type), m_key(key), m_info(info)
	{
	}

	bool service_node_list::rollback_change::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
	{
		service_nodes_infos[m_key] = m_info;
		return true;
	}

	service_node_list::rollback_new::rollback_new(uint64_t block_height, const crypto::public_key& key)
		: service_node_list::rollback_event(block_height, new_type), m_key(key)
	{
	}

	bool service_node_list::rollback_new::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
	{
		auto iter = service_nodes_infos.find(m_key);
		if (iter == service_nodes_infos.end())
		{
			MERROR("Could not find service node pubkey in rollback new");
			return false;
		}
		service_nodes_infos.erase(iter);
		return true;
	}

	service_node_list::prevent_rollback::prevent_rollback(uint64_t block_height) : service_node_list::rollback_event(block_height, prevent_type)
	{
	}

	bool service_node_list::prevent_rollback::apply(std::unordered_map<crypto::public_key, service_node_info>& service_nodes_infos) const
	{
		MERROR("Unable to rollback any further!");
		return false;
	}

	bool service_node_list::store()
	{

		CHECK_AND_ASSERT_MES(m_db != nullptr, false, "Failed to store service node info, m_db == nullptr");
		data_members_for_serialization data_to_store;
		{
			std::lock_guard<boost::recursive_mutex> lock(m_sn_mutex);

			quorum_state_for_serialization quorum;
			for (const auto& kv_pair : m_quorum_states)
			{
				quorum.height = kv_pair.first;
				quorum.state = *kv_pair.second;
				data_to_store.quorum_states.push_back(quorum);
			}

			node_info_for_serialization info;
			for (const auto& kv_pair : m_service_nodes_infos)
			{
				info.key = kv_pair.first;
				info.info = kv_pair.second;
				data_to_store.infos.push_back(info);
			}

			rollback_event_variant event;
			for (const auto& event_ptr : m_rollback_events)
			{
				switch (event_ptr->type)
				{
				case rollback_event::change_type:
					event = *reinterpret_cast<rollback_change *>(event_ptr.get());
					data_to_store.events.push_back(*reinterpret_cast<rollback_change *>(event_ptr.get()));
					break;
				case rollback_event::new_type:
					event = *reinterpret_cast<rollback_new *>(event_ptr.get());
					data_to_store.events.push_back(*reinterpret_cast<rollback_new *>(event_ptr.get()));
					break;
				case rollback_event::prevent_type:
					event = *reinterpret_cast<prevent_rollback *>(event_ptr.get());
					data_to_store.events.push_back(*reinterpret_cast<prevent_rollback *>(event_ptr.get()));
					break;
				default:
					MERROR("On storing service node data, unknown rollback event type encountered");
					return false;
				}
			}

		}

		data_to_store.height = m_height;

		std::stringstream ss;
		binary_archive<true> ba(ss);

		bool r = ::serialization::serialize(ba, data_to_store);
		CHECK_AND_ASSERT_MES(r, false, "Failed to store service node info: failed to serialize data");

		std::string blob = ss.str();
		m_db->block_txn_start(false/*readonly*/);
		m_db->set_service_node_data(blob);
		m_db->block_txn_stop();

		return true;
	}

	bool service_node_list::load()
	{
		LOG_PRINT_L1("service_node_list::load()");
		clear(false);
		if (!m_db)
		{
			return false;
		}
		std::stringstream ss;

		data_members_for_serialization data_in;
		std::string blob;

		m_db->block_txn_start(true/*readonly*/);
		if (!m_db->get_service_node_data(blob))
		{
			m_db->block_txn_stop();
			return false;
		}
		m_db->block_txn_stop();

		ss << blob;
		binary_archive<false> ba(ss);
		bool r = ::serialization::serialize(ba, data_in);
		CHECK_AND_ASSERT_MES(r, false, "Failed to parse service node data from blob");

		m_height = data_in.height;

		for (const auto& quorum : data_in.quorum_states)
		{
			m_quorum_states[quorum.height] = std::make_shared<quorum_state>(quorum.state);
		}

		for (const auto& info : data_in.infos)
		{
			m_service_nodes_infos[info.key] = info.info;
		}

		for (const auto& event : data_in.events)
		{
			if (event.type() == typeid(rollback_change))
			{
				rollback_change *i = new rollback_change();
				const rollback_change& from = boost::get<rollback_change>(event);
				i->m_block_height = from.m_block_height;
				i->m_key = from.m_key;
				i->m_info = from.m_info;
				i->type = rollback_event::change_type;
				m_rollback_events.push_back(std::unique_ptr<rollback_event>(i));
			}
			else if (event.type() == typeid(rollback_new))
			{
				rollback_new *i = new rollback_new();
				const rollback_new& from = boost::get<rollback_new>(event);
				i->m_block_height = from.m_block_height;
				i->m_key = from.m_key;
				i->type = rollback_event::new_type;
				m_rollback_events.push_back(std::unique_ptr<rollback_event>(i));
			}
			else if (event.type() == typeid(prevent_rollback))
			{
				prevent_rollback *i = new prevent_rollback();
				const prevent_rollback& from = boost::get<prevent_rollback>(event);
				i->m_block_height = from.m_block_height;
				i->type = rollback_event::prevent_type;
				m_rollback_events.push_back(std::unique_ptr<rollback_event>(i));
			}
			else
			{
				MERROR("Unhandled rollback event type in restoring data to service node list.");
				return false;
			}
		}

		MGINFO("Service node data loaded successfully, m_height: " << m_height);
		MGINFO(m_service_nodes_infos.size() << " nodes and " << m_rollback_events.size() << " rollback events loaded.");

		LOG_PRINT_L1("service_node_list::load() returning success");
		return true;
	}

	void service_node_list::clear(bool delete_db_entry)
	{
		m_service_nodes_infos.clear();
		m_rollback_events.clear();

		if (m_db && delete_db_entry)
		{
			m_db->block_txn_start(false/*readonly*/);
			m_db->clear_service_node_data();
			m_db->block_txn_stop();
		}

		m_quorum_states.clear();

		uint64_t hardfork_5_from_height = 0;
		{
			uint32_t window, votes, threshold;
			uint8_t voting;
			m_blockchain.get_hard_fork_voting_info(9, window, votes, threshold, hardfork_5_from_height, voting);
		}
		m_height = hardfork_5_from_height;
	}

	bool convert_registration_args(cryptonote::network_type nettype, std::vector<std::string> args, std::vector<cryptonote::account_public_address>& addresses, std::vector<uint64_t>& portions, uint64_t& portions_for_operator, bool& autostake)
	{
		autostake = false;
		if (!args.empty() && args[0] == "auto")
		{
			autostake = true;
			args.erase(args.begin());
		}

		if (args.size() % 2 == 0 || args.size() < 3)
		{
			MERROR(tr("Usage: [auto] <operator cut> <address> <fraction> [<address> <fraction> [...]]]"));
			return false;
		}
		if ((args.size() - 1) / 2 > MAX_NUMBER_OF_CONTRIBUTORS)
		{
			MERROR(tr("Exceeds the maximum number of contributors, which is ") << MAX_NUMBER_OF_CONTRIBUTORS);
			return false;
		}
		addresses.clear();
		portions.clear();
		try
		{
			portions_for_operator = boost::lexical_cast<uint64_t>(args[0]);
			if (portions_for_operator > STAKING_PORTIONS)
			{
				MERROR(tr("Invalid portion amount: ") << args[0] << tr(". ") << tr("Must be between 0 and ") << STAKING_PORTIONS);
				return false;
			}
		}
		catch (const std::exception &e)
		{
			MERROR(tr("Invalid portion amount: ") << args[0] << tr(". ") << tr("Must be between 0 and ") << STAKING_PORTIONS);
			return false;
		}
		uint64_t portions_left = STAKING_PORTIONS;
		for (size_t i = 1; i < args.size(); i += 2)
		{
			cryptonote::address_parse_info info;
			if (!cryptonote::get_account_address_from_str(info, nettype, args[i]))
			{
				MERROR(tr("failed to parse address"));
				return false;
			}

			if (info.has_payment_id)
			{
				MERROR(tr("can't use a payment id for staking tx"));
				return false;
			}

			if (info.is_subaddress)
			{
				MERROR(tr("can't use a subaddress for staking tx"));
				return false;
			}

			addresses.push_back(info.address);

			try
			{
				uint64_t num_portions = boost::lexical_cast<uint64_t>(args[i + 1]);
				uint64_t min_portions = std::min(portions_left, MIN_PORTIONS);
				if (num_portions < min_portions || num_portions > portions_left)
				{
					MERROR(tr("Invalid portion amount: ") << args[i + 1] << tr(". ") << tr("The contributors must each have at least 25%, except for the last contributor which may have the remaining amount"));
					return false;
				}
				portions_left -= num_portions;
				portions.push_back(num_portions);
			}
			catch (const std::exception &e)
			{
				MERROR(tr("Invalid portion amount: ") << args[i + 1] << tr(". ") << tr("The contributors must each have at least 25%, except for the last contributor which may have the remaining amount"));
				return false;
			}
		}
		return true;
	}

	bool make_registration_cmd(cryptonote::network_type nettype, const std::vector<std::string> args, const crypto::public_key& service_node_pubkey,
		const crypto::secret_key service_node_key, std::string &cmd, bool make_friendly)
	{

		std::vector<cryptonote::account_public_address> addresses;
		std::vector<uint64_t> portions;
		uint64_t operator_portions;
		bool autostake;
		if (!convert_registration_args(nettype, args, addresses, portions, operator_portions, autostake))
		{
			MERROR(tr("Could not convert registration args"));
			return false;
		}

		uint64_t exp_timestamp = time(nullptr) + (autostake ? STAKING_AUTHORIZATION_EXPIRATION_AUTOSTAKE : STAKING_AUTHORIZATION_EXPIRATION_WINDOW);

		crypto::hash hash;
		bool hashed = cryptonote::get_registration_hash(addresses, operator_portions, portions, exp_timestamp, hash);
		if (!hashed)
		{
			MERROR(tr("Could not make registration hash from addresses and portions"));
			return false;
		}

		crypto::signature signature;
		crypto::generate_signature(hash, service_node_pubkey, service_node_key, signature);

		std::stringstream stream;
		if (make_friendly)
		{
			stream << tr("Run this command in the wallet that will fund this registration:\n\n");
		}

		stream << "register_service_node";
		for (size_t i = 0; i < args.size(); ++i)
		{
			stream << " " << args[i];
		}

		stream << " " << exp_timestamp << " ";
		stream << epee::string_tools::pod_to_hex(service_node_pubkey) << " ";
		stream << epee::string_tools::pod_to_hex(signature);

		if (make_friendly)
		{
			stream << "\n\n";
			time_t tt = exp_timestamp;
			struct tm tm;
#ifdef WIN32
			gmtime_s(&tm, &tt);
#else
			gmtime_r(&tt, &tm);
#endif

			char buffer[128];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M:%S %p", &tm);
			stream << tr("This registration expires at ") << buffer << tr(".\n");
			stream << tr("This should be in about 2 weeks, or two years for autostaking.\n");
			stream << tr("If it isn't, check this computer's clock.\n");
			stream << tr("Please submit your registration into the blockchain before this time or it will be invalid.");
		}

		cmd = stream.str();
		return true;
	}
}
