#pragma once

namespace service_nodes {

	inline uint64_t get_staking_requirement_lock_blocks(cryptonote::network_type nettype)
	{	
		switch (nettype) {
		case cryptonote::TESTNET: return 1440;
		case cryptonote::FAKECHAIN: return 30;
		default: return 44640;
		}
	}

	inline uint64_t get_min_node_contribution(uint64_t staking_requirement, uint64_t total_reserved)
	{
		return std::min(staking_requirement - total_reserved, staking_requirement / MAX_NUMBER_OF_CONTRIBUTORS);
	}

	uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height);

	uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

	/// Check if portions are sufficiently large (except for the last) and add up to the required amount
	bool check_service_node_portions(const std::vector<uint64_t>& portions);

}