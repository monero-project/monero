#pragma once

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "service_node_deregister.h"

#include <random>

namespace service_nodes {
  constexpr size_t   QUORUM_SIZE                                    = 10;
  constexpr size_t   MIN_VOTES_TO_KICK_SERVICE_NODE                 = 7;
  constexpr size_t   NTH_OF_THE_NETWORK_TO_TEST                     = 100;
  constexpr size_t   MIN_NODES_TO_TEST                              = 50;
  constexpr size_t   MAX_SWARM_SIZE                                 = 10;
  constexpr size_t   SWARM_BUFFER                                   = 5;
  constexpr size_t   MIN_SWARM_SIZE                                 = 5;
  constexpr size_t   IDEAL_SWARM_MARGIN                             = 2;
  constexpr size_t   IDEAL_SWARM_SIZE                               = MIN_SWARM_SIZE + IDEAL_SWARM_MARGIN;
  constexpr size_t   EXCESS_BASE                                    = MIN_SWARM_SIZE;
  constexpr size_t   NEW_SWARM_SIZE                                 = IDEAL_SWARM_SIZE;
  constexpr size_t   FILL_SWARM_LOWER_PERCENTILE                    = 25;
  constexpr size_t   DECOMMISSIONED_REDISTRIBUTION_LOWER_PERCENTILE = 0;
  constexpr size_t   STEALING_SWARM_UPPER_PERCENTILE                = 75;
  constexpr uint64_t QUEUE_SWARM_ID                                 = 0;
  constexpr int      MAX_KEY_IMAGES_PER_CONTRIBUTOR                 = 1;

  using swarm_id_t = uint64_t;
  constexpr swarm_id_t UNASSIGNED_SWARM_ID          = UINT64_MAX;

inline uint64_t staking_num_lock_blocks(cryptonote::network_type nettype)
{
  switch(nettype)
  {
    case cryptonote::TESTNET: return BLOCKS_EXPECTED_IN_DAYS(2);
    case cryptonote::FAKECHAIN: return 30;
    default: return BLOCKS_EXPECTED_IN_DAYS(30);
  }
}

uint64_t get_min_node_contribution(uint8_t hard_fork_version, uint64_t staking_requirement, uint64_t total_reserved);

uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height);

uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

/// Check if portions are sufficiently large (except for the last) and add up to the required amount
bool check_service_node_portions(const std::vector<uint64_t>& portions, const uint64_t min_portions = MIN_PORTIONS);

// Returns lowest x such that (staking_requirement * x/STAKING_PORTIONS) >= amount
uint64_t get_portions_to_make_amount(uint64_t staking_requirement, uint64_t amount);

bool get_portions_from_percent_str(std::string cut_str, uint64_t& portions);

uint64_t uniform_distribution_portable(std::mt19937_64& mersenne_twister, uint64_t n);
}
