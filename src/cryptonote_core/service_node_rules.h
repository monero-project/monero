#pragma once

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "service_node_deregister.h"

namespace service_nodes {
  constexpr size_t   QUORUM_SIZE                      = 10;
  constexpr size_t   QUORUM_LIFETIME                  = (6 * deregister_vote::DEREGISTER_LIFETIME_BY_HEIGHT);
  constexpr size_t   MIN_VOTES_TO_KICK_SERVICE_NODE   = 7;
  constexpr size_t   NTH_OF_THE_NETWORK_TO_TEST       = 100;
  constexpr size_t   MIN_NODES_TO_TEST                = 50;
  constexpr size_t   MAX_SWARM_SIZE                   = 10;
  // We never create a new swarm unless there are SWARM_BUFFER extra nodes
  // available in the queue.
  constexpr size_t   SWARM_BUFFER                     = 5;
  // if a swarm has strictly less nodes than this, it is considered unhealthy
  // and nearby swarms will mirror it's data. It will disappear, and is already considered gone.
  constexpr size_t   MIN_SWARM_SIZE                   = 5;
  constexpr int      MAX_KEY_IMAGES_PER_CONTRIBUTOR   = 1;
  constexpr uint64_t QUEUE_SWARM_ID                   = 0;
  constexpr uint64_t KEY_IMAGE_AWAITING_UNLOCK_HEIGHT = 0;



inline uint64_t staking_num_lock_blocks(cryptonote::network_type nettype)
{
  switch(nettype)
  {
      case cryptonote::FAKECHAIN: return 30;
      case cryptonote::TESTNET:   return BLOCKS_EXPECTED_IN_DAYS(2);
      default:                    return BLOCKS_EXPECTED_IN_DAYS(30);
  }
}

uint64_t get_min_node_contribution            (uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions);
uint64_t get_min_node_contribution_in_portions(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions);

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
