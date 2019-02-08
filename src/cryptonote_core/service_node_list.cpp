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


#include "ringct/rctSigs.h"
#include "wallet/wallet2.h"
#include "cryptonote_tx_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "common/int-util.h"
#include "common/scoped_message_writer.h"
#include "common/i18n.h"
#include "quorum_cop.h"

#include "service_node_list.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "service_nodes"

namespace service_nodes
{

	service_node_list::service_node_list(cryptonote::Blockchain& blockchain)
		: m_blockchain(blockchain), m_hooks_registered(false), m_height(0), m_db(nullptr)
	{
	}

	void service_node_list::register_hooks(service_nodes::quorum_cop &quorum_cop)
	{
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
		uint64_t current_height = m_blockchain.get_current_blockchain_height();
		bool loaded = load();

		if (loaded && m_height == current_height) return;

		if (!loaded || m_height > current_height) clear(true);

		LOG_PRINT_L0("Recalculating service nodes list, scanning blockchain from height " << m_height);
		LOG_PRINT_L0("This may take some time...");

		while (m_height < current_height)
		{
			std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
			if (!m_blockchain.get_blocks(m_height, 1000, blocks))
			{
				LOG_ERROR("Unable to initialize service nodes list");
				return;
			}

			for (const auto& block_pair : blocks)
			{
				const cryptonote::block& block = block_pair.second;
				std::list<cryptonote::transaction> txs;
				std::list<crypto::hash> missed_txs;
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

	const std::shared_ptr<quorum_state> service_node_list::get_quorum_state(uint64_t height) const
	{
		std::shared_ptr<service_nodes::quorum_state> result;
		const auto &it = m_quorum_states.find(height);
		if (it == m_quorum_states.end())
		{
			// TODO(triton): Not being able to find the quorum is going to be a fatal error.
		}
		else
		{
			result = it->second;
		}

		return result;
	}

	std::vector<service_node_pubkey_info> service_node_list::get_service_node_list_state(const std::vector<crypto::public_key> &service_node_pubkeys) const
	{
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

	bool service_node_list::is_service_node(const crypto::public_key& pubkey) const
	{
		return m_service_nodes_infos.find(pubkey) != m_service_nodes_infos.end();
	}

	bool service_node_list::contribution_tx_output_has_correct_unlock_time(const cryptonote::transaction& tx, size_t i, uint64_t block_height) const
	{
		uint64_t unlock_time = tx.unlock_time;

		if (tx.version >= cryptonote::transaction::version_3_per_output_unlock_times)
			unlock_time = tx.output_unlock_times[i];

		return unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER && unlock_time >= block_height + get_staking_requirement_lock_blocks(m_blockchain.nettype());
	}

	bool service_node_list::reg_tx_extract_fields(const cryptonote::transaction& tx, std::vector<cryptonote::account_public_address>& addresses, uint64_t& portions_for_operator, std::vector<uint64_t>& portions, uint64_t& expiration_timestamp, crypto::public_key& service_node_key, crypto::signature& signature, crypto::public_key& tx_pub_key) const
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

	uint64_t service_node_list::get_reg_tx_staking_output_contribution(const cryptonote::transaction& tx, int i, crypto::key_derivation derivation, hw::device& hwdev) const
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
			case rct::RCTTypeFull:
			case rct::RCTTypeBulletproof:
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

	void service_node_list::process_deregistration_tx(const cryptonote::transaction& tx, uint64_t block_height)
	{
		if (!tx.is_deregister_tx())
			return;

		cryptonote::tx_extra_service_node_deregister deregister;
		if (!cryptonote::get_service_node_deregister_from_tx_extra(tx.extra, deregister))
		{
			LOG_ERROR("Transaction deregister did not have deregister data in tx extra, possibly corrupt tx in blockchain");
			return;
		}

		const std::shared_ptr<quorum_state> state = get_quorum_state(deregister.block_height);

		if (!state)
		{
			// TODO(triton): Not being able to find a quorum is fatal! We want better caching abilities.
			LOG_ERROR("Quorum state for height: " << deregister.block_height << ", was not stored by the daemon");
			return;
		}

		if (deregister.service_node_index >= state->nodes_to_test.size())
		{
			LOG_ERROR("Service node index to vote off has become invalid, quorum rules have changed without a hardfork.");
			return;
		}

		const crypto::public_key& key = state->nodes_to_test[deregister.service_node_index];

		auto iter = m_service_nodes_infos.find(key);
		if (iter == m_service_nodes_infos.end())
			return;

		LOG_PRINT_L1("Deregistration for service node: " << key);

		m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, key, iter->second)));
		m_service_nodes_infos.erase(iter);
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

		uint64_t portions_left = STAKING_PORTIONS;
		for (size_t i = 0; i < service_node_portions.size(); i++)
		{
			uint64_t min_portions = std::min(portions_left, MIN_PORTIONS);
			if (service_node_portions[i] < min_portions || service_node_portions[i] > portions_left)
				return false;
			portions_left -= service_node_portions[i];
		}

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

	void service_node_list::process_registration_tx(const cryptonote::transaction& tx, uint64_t block_timestamp, uint64_t block_height, uint32_t index)
	{
		crypto::public_key key;
		service_node_info info;
		if (!is_registration_tx(tx, block_timestamp, block_height, index, key, info))
			return;

		auto iter = m_service_nodes_infos.find(key);
		if (iter != m_service_nodes_infos.end())
			return;

		LOG_PRINT_L1("New service node registered: " << key << " at block height: " << block_height);

		m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_new(block_height, key)));
		m_service_nodes_infos[key] = info;
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
		block_added_generic(block, txs);
		store();
	}


	template<typename T>
	void service_node_list::block_added_generic(const cryptonote::block& block, const T& txs)
	{
		uint64_t block_height = cryptonote::get_block_height(block);
		int hard_fork_version = m_blockchain.get_hard_fork_version(block_height);

		assert(m_height == block_height);
		m_height++;

		if (hard_fork_version < 9)
			return;

		{
			const size_t ROLLBACK_EVENT_EXPIRATION_BLOCKS = 30;
			uint64_t cull_height = (block_height < ROLLBACK_EVENT_EXPIRATION_BLOCKS) ? block_height : block_height - ROLLBACK_EVENT_EXPIRATION_BLOCKS;

			while (!m_rollback_events.empty() && m_rollback_events.front()->m_block_height < cull_height)
			{
				m_rollback_events.pop_front();
			}
			m_rollback_events.push_front(std::unique_ptr<rollback_event>(new prevent_rollback(cull_height)));
		}

		for (const crypto::public_key& pubkey : get_expired_nodes(block_height))
		{
			auto i = m_service_nodes_infos.find(pubkey);
			if (i != m_service_nodes_infos.end())
			{
				m_rollback_events.push_back(std::unique_ptr<rollback_event>(new rollback_change(block_height, pubkey, i->second)));
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

		uint32_t index = 0;
		for (const cryptonote::transaction& tx : txs)
		{
			crypto::public_key key;
			service_node_info info;
			cryptonote::account_public_address address;
			process_registration_tx(tx, block.timestamp, block_height, index);
			process_contribution_tx(tx, block_height, index);
			process_deregistration_tx(tx, block_height);
			index++;
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

		const uint64_t lock_blocks = get_staking_requirement_lock_blocks(m_blockchain.nettype());

		if (block_height < lock_blocks)
			return expired_nodes;

		const uint64_t expired_nodes_block_height = block_height - lock_blocks;

		std::list<std::pair<cryptonote::blobdata, cryptonote::block>> blocks;
		if (!m_blockchain.get_blocks(expired_nodes_block_height, 1, blocks))
		{
			LOG_ERROR("Unable to get historical blocks");
			return expired_nodes;
		}

		const cryptonote::block& block = blocks.begin()->second;
		std::list<cryptonote::transaction> txs;
		std::list<crypto::hash> missed_txs;
		if (!m_blockchain.get_transactions(block.tx_hashes, txs, missed_txs))
		{
			LOG_ERROR("Unable to get transactions for block " << block.hash);
			return expired_nodes;
		}

		uint32_t index = 0;
		for (const cryptonote::transaction& tx : txs)
		{
			crypto::public_key key;
			service_node_info info;
			if (is_registration_tx(tx, block.timestamp, expired_nodes_block_height, index, key, info))
			{
				expired_nodes.push_back(key);
			}
			index++;
		}

		return expired_nodes;
	}

	std::vector<std::pair<cryptonote::account_public_address, uint64_t>> service_node_list::get_winner_addresses_and_portions(const crypto::hash& prev_id) const
	{
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
	bool service_node_list::validate_miner_tx(const crypto::hash& prev_id, const cryptonote::transaction& miner_tx, uint64_t height, int hard_fork_version, uint64_t base_reward)
	{
		if (hard_fork_version < 9)
			return true;

		uint64_t total_service_node_reward = cryptonote::get_service_node_reward(height, base_reward, hard_fork_version);

		crypto::public_key winner = select_winner(prev_id);

		crypto::public_key check_winner_pubkey = cryptonote::get_service_node_winner_from_tx_extra(miner_tx.extra);
		if (check_winner_pubkey != winner)
			return false;

		const std::vector<std::pair<cryptonote::account_public_address, uint64_t>> addresses_and_portions = get_winner_addresses_and_portions(prev_id);

		if (miner_tx.vout.size() - 1 < addresses_and_portions.size())
			return false;

		for (size_t i = 0; i < addresses_and_portions.size(); i++)
		{
			size_t vout_index = miner_tx.vout.size() - 1 /* governance */ - addresses_and_portions.size() + i;

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

	static uint64_t uniform_distribution_portable(std::mt19937_64& mersenne_twister, uint64_t n)
	{
		uint64_t secureMax = mersenne_twister.max() - mersenne_twister.max() % n;
		uint64_t x;
		do x = mersenne_twister(); while (x >= secureMax);
		return  x / (secureMax / n);
	}

	static void triton_shuffle(std::vector<size_t>& a, uint64_t seed)
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
		if (!m_quorum_states[height])
			m_quorum_states[height] = std::shared_ptr<quorum_state>(new quorum_state());

		std::shared_ptr<quorum_state> state = m_quorum_states[height];
		state->clear();
		{
			std::vector<crypto::public_key>& quorum = state->quorum_nodes;
			{
				quorum.clear();
				quorum.resize(std::min(full_node_list.size(), QUORUM_SIZE));
				for (size_t i = 0; i < quorum.size(); i++)
				{
					size_t node_index = pub_keys_indexes[i];
					const crypto::public_key &key = full_node_list[node_index];
					quorum[i] = key;
				}
			}

			std::vector<crypto::public_key>& nodes_to_test = state->nodes_to_test;
			{
				size_t num_remaining_nodes = pub_keys_indexes.size() - quorum.size();
				size_t num_nodes_to_test = std::max(num_remaining_nodes / NTH_OF_THE_NETWORK_TO_TEST,
					std::min(MIN_NODES_TO_TEST, num_remaining_nodes));

				nodes_to_test.clear();
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
			m_quorum_states[quorum.height] = std::shared_ptr<quorum_state>(new quorum_state());
			*m_quorum_states[quorum.height] = quorum.state;
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

		LOG_PRINT_L0("Service node data loaded successfully, m_height: " << m_height);
		LOG_PRINT_L0(m_service_nodes_infos.size() << " nodes and " << m_rollback_events.size() << " rollback events loaded.");

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
		m_height = 0;
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

	uint64_t get_staking_requirement_lock_blocks(cryptonote::network_type nettype)
	{
		return (nettype == cryptonote::MAINNET) ? 30 * 24 * 30 : 30 * 24 * 2;
	}

	uint64_t get_staking_requirement(cryptonote::network_type m_nettype, uint64_t height)
	{
		if (m_nettype == cryptonote::TESTNET || m_nettype == cryptonote::MAINNET)
			return COIN * 100000;
	}

	uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement)
	{
		uint64_t hi, lo, resulthi, resultlo;
		lo = mul128(staking_requirement, portions, &hi);
		div128_64(hi, lo, STAKING_PORTIONS, &resulthi, &resultlo);
		return resultlo;
	}
}
