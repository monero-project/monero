// Copyright (c) 2018, The Loki Project
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

#include <vector>
#include <unordered_map>
#include <utility>

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"

#include "math_helper.h"
#include "syncobj.h"

namespace cryptonote
{
	struct vote_verification_context;
};

namespace service_nodes
{
	struct quorum_state;
};

namespace triton
{
	namespace service_node_deregister
	{
		const uint64_t VOTE_LIFETIME_BY_HEIGHT = (60 * 60 * 2) / DIFFICULTY_TARGET_V2;
		const uint64_t DEREGISTER_LIFETIME_BY_HEIGHT = VOTE_LIFETIME_BY_HEIGHT;

		struct vote
		{
			uint64_t          block_height;
			uint32_t          service_node_index;
			uint32_t          voters_quorum_index;
			crypto::signature signature;
		};

		crypto::signature sign_vote(uint64_t block_height, uint32_t service_node_index, const crypto::public_key& pub, const crypto::secret_key& sec);
		bool verify_vote_signature(uint64_t block_height, uint32_t service_node_index, crypto::public_key p, crypto::signature s);
		bool verify_votes_signature(uint64_t block_height, uint32_t service_node_index, const std::vector<std::pair<crypto::public_key, crypto::signature>>& keys_and_sigs);

		bool verify_deregister(cryptonote::network_type nettype, const cryptonote::tx_extra_service_node_deregister& deregister,
			cryptonote::vote_verification_context& vvc,
			const service_nodes::quorum_state &quorum);
		bool verify_vote(cryptonote::network_type nettype, const vote& v, cryptonote::vote_verification_context &vvc,
			const service_nodes::quorum_state &quorum);
	};

	class deregister_vote_pool
	{
	public:
		/**
		*  @return True if vote was valid and in the pool already or just added (check vote verfication for specific case).
		*/
		bool add_vote(const service_node_deregister::vote& new_vote,
			cryptonote::vote_verification_context& vvc,
			const service_nodes::quorum_state &quorum_state,
			cryptonote::transaction &tx);

		// TODO(triton): Review relay behaviour and all the cases when it should be triggered
		void                                       set_relayed(const std::vector<service_node_deregister::vote>& votes);
		void                                       remove_expired_votes(uint64_t height);
		void                                       remove_used_votes(std::vector<cryptonote::transaction> const &txs);
		std::vector<service_node_deregister::vote> get_relayable_votes() const;

		cryptonote::network_type m_nettype = cryptonote::UNDEFINED;

	private:
		struct deregister
		{
			deregister(uint64_t time_last_sent_p2p, service_node_deregister::vote vote)
				: m_time_last_sent_p2p(time_last_sent_p2p), m_vote(vote) {}

			uint64_t m_time_last_sent_p2p;
			service_node_deregister::vote m_vote;
		};

		struct deregister_group
		{
			uint64_t block_height;
			uint32_t service_node_index;
			time_t   time_group_created;

			bool operator==(const deregister_group &other) const
			{
				bool result = (block_height == other.block_height) && (service_node_index == other.service_node_index);
				return result;
			}
		};

		struct deregister_group_hasher
		{
			size_t operator()(const deregister_group& deregister) const
			{
				size_t res = 17;
				res = res * 31 + std::hash<uint64_t>()(deregister.block_height);
				res = res * 31 + std::hash<uint32_t>()(deregister.service_node_index);
				res = res * 31 + std::hash<uint32_t>()(deregister.time_group_created);
				return res;
			}
		};

		std::unordered_map<deregister_group, std::vector<deregister>, deregister_group_hasher> m_deregisters;
		mutable epee::critical_section m_lock;
	};
}; // namespace triton
