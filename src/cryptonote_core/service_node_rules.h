#pragma once

#include "crypto/crypto.h"
#include "cryptonote_config.h"

namespace service_nodes {

inline uint64_t staking_initial_num_lock_blocks(cryptonote::network_type nettype)
{
  switch(nettype)
  {
      case cryptonote::FAKECHAIN: return 30;
      case cryptonote::TESTNET:   return BLOCKS_EXPECTED_IN_DAYS(2);
      default:                    return BLOCKS_EXPECTED_IN_DAYS(30);
  }
}

uint64_t get_min_node_contribution(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t contrib_count);

uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height);

uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

/// Check if portions are sufficiently large (provided the contributions
/// are made in the specified order) and don't exceed the required amount
bool check_service_node_portions(uint8_t version, const std::vector<uint64_t>& portions);

crypto::hash generate_request_stake_unlock_hash(uint32_t nonce);
uint64_t     get_locked_key_image_unlock_height(cryptonote::network_type nettype, uint64_t node_register_height, uint64_t curr_height);

// Returns lowest x such that (staking_requirement * x/STAKING_PORTIONS) >= amount
uint64_t get_portions_to_make_amount(uint64_t staking_requirement, uint64_t amount);

bool get_portions_from_percent_str(std::string cut_str, uint64_t& portions);

}
