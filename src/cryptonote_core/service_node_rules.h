#pragma once

namespace service_nodes {

inline uint64_t get_staking_requirement_lock_blocks(cryptonote::network_type nettype)
{
constexpr static uint32_t STAKING_REQUIREMENT_LOCK_BLOCKS         = 30*24*30;
constexpr static uint32_t STAKING_REQUIREMENT_LOCK_BLOCKS_TESTNET = 30*24*2;
constexpr static uint32_t STAKING_REQUIREMENT_LOCK_BLOCKS_FAKENET = 30;

switch(nettype) {
    case cryptonote::TESTNET: return STAKING_REQUIREMENT_LOCK_BLOCKS_TESTNET;
    case cryptonote::FAKECHAIN: return STAKING_REQUIREMENT_LOCK_BLOCKS_FAKENET;
    default: return STAKING_REQUIREMENT_LOCK_BLOCKS;
}
}

uint64_t get_min_node_contribution(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t contrib_count);

uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height);

uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

/// Check if portions are sufficiently large (provided the contributions
/// are made in the specified order) and don't exceed the required amount
bool check_service_node_portions(uint8_t version, const std::vector<uint64_t>& portions);

// Returns lowest x such that (staking_requirement * x/STAKING_PORTIONS) >= amount
uint64_t get_portions_to_make_amount(uint64_t staking_requirement, uint64_t amount);

bool get_portions_from_percent_str(std::string cut_str, uint64_t& portions);

}