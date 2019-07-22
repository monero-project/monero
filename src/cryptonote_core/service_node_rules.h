#pragma once

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "service_node_voting.h"

namespace service_nodes {
  // Service node decommissioning: as service nodes stay up they earn "credits" (measured in blocks)
  // towards a future outage.  A new service node starts out with INITIAL_CREDIT, and then builds up
  // CREDIT_PER_DAY for each day the service node remains active up to a maximum of
  // DECOMMISSION_MAX_CREDIT.
  //
  // If a service node stops sending uptime proofs, a quorum will consider whether the service node
  // has built up enough credits (at least MINIMUM): if so, instead of submitting a deregistration,
  // it instead submits a decommission.  This removes the service node from the list of active
  // service nodes both for rewards and for any active network duties.  If the service node comes
  // back online (i.e. starts sending the required performance proofs again) before the credits run
  // out then a quorum will reinstate the service node using a recommission transaction, which adds
  // the service node back to the bottom of the service node reward list, and resets its accumulated
  // credits to 0.  If it does not come back online within the required number of blocks (i.e. the
  // accumulated credit at the point of decommissioning) then a quorum will send a permanent
  // deregistration transaction to the network, starting a 30-day deregistration count down.
  constexpr int64_t DECOMMISSION_CREDIT_PER_DAY = BLOCKS_EXPECTED_IN_HOURS(24) / 30;
  constexpr int64_t DECOMMISSION_INITIAL_CREDIT = BLOCKS_EXPECTED_IN_HOURS(2);
  constexpr int64_t DECOMMISSION_MAX_CREDIT     = BLOCKS_EXPECTED_IN_HOURS(24);
  constexpr int64_t DECOMMISSION_MINIMUM        = BLOCKS_EXPECTED_IN_HOURS(2);

  static_assert(DECOMMISSION_INITIAL_CREDIT <= DECOMMISSION_MAX_CREDIT, "Initial registration decommission credit cannot be larger than the maximum decommission credit");

  constexpr uint64_t  CHECKPOINT_NUM_CHECKPOINTS_FOR_CHAIN_FINALITY = 2;
  constexpr uint64_t  CHECKPOINT_INTERVAL                           = 4;  // Checkpoint every 4 blocks and prune when too old except if (height % CHECKPOINT_STORE_PERSISTENTLY_INTERVAL == 0)
  constexpr uint64_t  CHECKPOINT_STORE_PERSISTENTLY_INTERVAL        = 60; // Persistently store the checkpoints at these intervals
  constexpr uint64_t  CHECKPOINT_VOTE_LIFETIME                      = CHECKPOINT_STORE_PERSISTENTLY_INTERVAL; // Keep the last 60 blocks worth of votes

  constexpr int16_t CHECKPOINT_MIN_QUORUMS_NODE_MUST_VOTE_IN_BEFORE_DEREGISTER_CHECK = 8;
  constexpr int16_t CHECKPOINT_MAX_MISSABLE_VOTES                                    = 4;
  static_assert(CHECKPOINT_MAX_MISSABLE_VOTES < CHECKPOINT_MIN_QUORUMS_NODE_MUST_VOTE_IN_BEFORE_DEREGISTER_CHECK,
                "The maximum number of votes a service node can miss cannot be greater than the amount of checkpoint "
                "quorums they must participate in before we check if they should be deregistered or not.");

  // State change quorums are in charge of policing the network by changing the state of a service
  // node on the network: temporary decommissioning, recommissioning, and permanent deregistration.
  constexpr size_t   STATE_CHANGE_NTH_OF_THE_NETWORK_TO_TEST = 100;
  constexpr size_t   STATE_CHANGE_MIN_NODES_TO_TEST          = 50;
  constexpr uint64_t VOTE_LIFETIME                           = BLOCKS_EXPECTED_IN_HOURS(2);

#if defined(LOKI_ENABLE_INTEGRATION_TEST_HOOKS)
  constexpr size_t    STATE_CHANGE_QUORUM_SIZE               = 5;
  constexpr size_t    STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE = 1;
  constexpr ptrdiff_t MIN_TIME_IN_S_BEFORE_VOTING            = 0;
  constexpr size_t    CHECKPOINT_QUORUM_SIZE                 = 5;
  constexpr size_t    CHECKPOINT_MIN_VOTES                   = 1;
#else
  constexpr size_t    STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE = 7;
  constexpr size_t    STATE_CHANGE_QUORUM_SIZE               = 10;
  constexpr ptrdiff_t MIN_TIME_IN_S_BEFORE_VOTING            = UPTIME_PROOF_MAX_TIME_IN_SECONDS;
  constexpr size_t    CHECKPOINT_QUORUM_SIZE                 = 20;
  constexpr size_t    CHECKPOINT_MIN_VOTES                   = 13;
#endif

  static_assert(STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE <= STATE_CHANGE_QUORUM_SIZE, "The number of votes required to kick can't exceed the actual quorum size, otherwise we never kick.");
  static_assert(CHECKPOINT_MIN_VOTES <= CHECKPOINT_QUORUM_SIZE, "The number of votes required to kick can't exceed the actual quorum size, otherwise we never kick.");

  // NOTE: We can reorg up to last 2 checkpoints + the number of extra blocks before the next checkpoint is set
  constexpr uint64_t  REORG_SAFETY_BUFFER_BLOCKS_POST_HF12 = (CHECKPOINT_INTERVAL * CHECKPOINT_NUM_CHECKPOINTS_FOR_CHAIN_FINALITY) + (CHECKPOINT_INTERVAL - 1);
  constexpr uint64_t  REORG_SAFETY_BUFFER_BLOCKS_PRE_HF12  = 20;
  static_assert(REORG_SAFETY_BUFFER_BLOCKS_POST_HF12 < VOTE_LIFETIME, "Safety buffer should always be less than the vote lifetime");
  static_assert(REORG_SAFETY_BUFFER_BLOCKS_PRE_HF12  < VOTE_LIFETIME, "Safety buffer should always be less than the vote lifetime");

  constexpr uint64_t  IP_CHANGE_WINDOW_IN_SECONDS     = 24*60*60; // How far back an obligations quorum looks for multiple IPs (unless the following buffer is more recent)
  constexpr uint64_t  IP_CHANGE_BUFFER_IN_SECONDS     = 2*60*60; // After we bump a SN for an IP change we don't bump again for changes within this time period

  constexpr size_t   MAX_SWARM_SIZE                   = 10;
  // We never create a new swarm unless there are SWARM_BUFFER extra nodes
  // available in the queue.
  constexpr size_t   SWARM_BUFFER                     = 5;
  // if a swarm has strictly less nodes than this, it is considered unhealthy
  // and nearby swarms will mirror it's data. It will disappear, and is already considered gone.
  constexpr size_t   MIN_SWARM_SIZE                   = 5;
  constexpr size_t   IDEAL_SWARM_MARGIN               = 2;
  constexpr size_t   IDEAL_SWARM_SIZE                 = MIN_SWARM_SIZE + IDEAL_SWARM_MARGIN;
  constexpr size_t   EXCESS_BASE                      = MIN_SWARM_SIZE;
  constexpr size_t   NEW_SWARM_SIZE                   = IDEAL_SWARM_SIZE;
  // The lower swarm percentile that will be randomly filled with new service nodes
  constexpr size_t   FILL_SWARM_LOWER_PERCENTILE      = 25;
  // Redistribute snodes from decommissioned swarms to the smallest swarms
  constexpr size_t   DECOMMISSIONED_REDISTRIBUTION_LOWER_PERCENTILE = 0;
  // The upper swarm percentile that will be randomly selected during stealing
  constexpr size_t   STEALING_SWARM_UPPER_PERCENTILE  = 75;
  constexpr int      MAX_KEY_IMAGES_PER_CONTRIBUTOR   = 1;
  constexpr uint64_t KEY_IMAGE_AWAITING_UNLOCK_HEIGHT = 0;

  constexpr uint64_t STATE_CHANGE_TX_LIFETIME_IN_BLOCKS = VOTE_LIFETIME;
  constexpr size_t   QUORUM_LIFETIME                    = (6 * STATE_CHANGE_TX_LIFETIME_IN_BLOCKS);

  using swarm_id_t                         = uint64_t;
  constexpr swarm_id_t UNASSIGNED_SWARM_ID = UINT64_MAX;

  inline quorum_type max_quorum_type_for_hf(uint8_t hf_version)
  {
    quorum_type result = (hf_version <= cryptonote::network_version_11_infinite_staking) ? quorum_type::obligations
                                                                                         : quorum_type::checkpointing;
    assert(result != quorum_type::count);
    return result;
  }

  inline uint64_t staking_num_lock_blocks(cryptonote::network_type nettype)
  {
    switch (nettype)
    {
      case cryptonote::FAKECHAIN: return 30;
      case cryptonote::TESTNET:   return BLOCKS_EXPECTED_IN_DAYS(2);
      default:                    return BLOCKS_EXPECTED_IN_DAYS(30);
    }
  }

static_assert(STAKING_PORTIONS != UINT64_MAX, "UINT64_MAX is used as the invalid value for failing to calculate the min_node_contribution");
// return: UINT64_MAX if (num_contributions > the max number of contributions), otherwise the amount in loki atomic units
uint64_t get_min_node_contribution            (uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions);
uint64_t get_min_node_contribution_in_portions(uint8_t version, uint64_t staking_requirement, uint64_t total_reserved, size_t num_contributions);

uint64_t get_staking_requirement(cryptonote::network_type nettype, uint64_t height, uint8_t hf_version);

uint64_t portions_to_amount(uint64_t portions, uint64_t staking_requirement);

/// Check if portions are sufficiently large (provided the contributions
/// are made in the specified order) and don't exceed the required amount
bool check_service_node_portions(uint8_t version, const std::vector<uint64_t>& portions);

crypto::hash generate_request_stake_unlock_hash(uint32_t nonce);
uint64_t     get_locked_key_image_unlock_height(cryptonote::network_type nettype, uint64_t node_register_height, uint64_t curr_height);

// Returns lowest x such that (staking_requirement * x/STAKING_PORTIONS) >= amount
uint64_t get_portions_to_make_amount(uint64_t staking_requirement, uint64_t amount);

bool get_portions_from_percent_str(std::string cut_str, uint64_t& portions);
uint64_t uniform_distribution_portable(std::mt19937_64& mersenne_twister, uint64_t n);
}
