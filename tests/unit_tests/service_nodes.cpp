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
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "gtest/gtest.h"
#include "cryptonote_core/service_node_list.h"
#include "cryptonote_core/service_node_voting.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/verification_context.h"
#include "cryptonote_config.h"

TEST(service_nodes, staking_requirement)
{
  // TODO(loki): The current reference values here for the staking requirement
  // at certain heights has been derived from excel, so we have to use an
  // epsilon for dust amounts as amounts are off by a bit. When we switch to
  // integer math we can remove the need for this. Doyle - 2018-08-28

  // NOTE: Thanks for the values @Sonofotis
  const uint64_t atomic_epsilon = config::DEFAULT_DUST_THRESHOLD;

  // LHS of Equation
  // Try underflow
  {
    uint64_t height = 100;
    uint64_t mainnet_requirement   = service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_8);
    uint64_t stagenet_requirement  = service_nodes::get_staking_requirement(cryptonote::STAGENET, height, cryptonote::network_version_8);
    ASSERT_EQ(stagenet_requirement, (45000 * COIN));
    ASSERT_EQ(mainnet_requirement,  (45000 * COIN));
  }

  // Starting height for stagenet
  {
    uint64_t height = 96210;
    uint64_t stagenet_requirement  = service_nodes::get_staking_requirement(cryptonote::STAGENET, height, cryptonote::network_version_8);
    ASSERT_EQ(stagenet_requirement, (45000 * COIN));
  }

  // Starting height for mainnet
  {
    // NOTE: The maximum staking requirement is 50,000, in atomic units is 50,000,000,000,000 < int64 range (2^63-1)
    // so casting is safe.
    uint64_t height = 101250;
    int64_t mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_9_service_nodes);
    int64_t stagenet_requirement = (int64_t)service_nodes::get_staking_requirement(cryptonote::STAGENET, height, cryptonote::network_version_9_service_nodes);

    ASSERT_EQ(mainnet_requirement,  (45000 * COIN));

    int64_t stagenet_expected = (int64_t)((44069 * COIN) + 151880000);
    int64_t stagenet_delta    = std::abs(stagenet_requirement - stagenet_expected);
    ASSERT_LT(stagenet_delta, atomic_epsilon);
  }

  // Check the requirements are decreasing
  {
    uint64_t height = 209250;
    int64_t mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_10_bulletproofs);

    int64_t  mainnet_expected = (int64_t)((29643 * COIN) + 670390000);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);
  }

  // Sliftly after the boundary when the scheme switches over to a smooth emissions curve to 15k
  {
    uint64_t height = 235987;
    int64_t  mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_11_infinite_staking);

    int64_t  mainnet_expected = (int64_t)((27164 * COIN) + 648610000);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);
  }

  // Check requirements are decreasing after switching over to new requirements curve
  {
    uint64_t height = 706050;
    int64_t  mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_11_infinite_staking);

    int64_t  mainnet_expected = (int64_t)((15984 * COIN) + 588930000);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);
  }

  // Check approaching 15k requirement
  {
    uint64_t height = 3643650;
    int64_t  mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height, cryptonote::network_version_11_infinite_staking);

    int64_t  mainnet_expected = (int64_t)((15000 * COIN) + 150000);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);
  }
}

TEST(service_nodes, vote_validation)
{
  // Generate a quorum and the voter
  cryptonote::keypair service_node_voter = cryptonote::keypair::generate(hw::get_device("default"));
  int voter_index = 0;

  service_nodes::testing_quorum state = {};
  {
    state.validators.resize(10);
    state.workers.resize(state.validators.size());

    for (size_t i = 0; i < state.validators.size(); ++i)
    {
      state.validators[i] = (i == voter_index) ? service_node_voter.pub : cryptonote::keypair::generate(hw::get_device("default")).pub;
      state.workers[i] = cryptonote::keypair::generate(hw::get_device("default")).pub;
    }
  }

  // Valid vote
  uint64_t block_height = 70;
  service_nodes::quorum_vote_t valid_vote = service_nodes::make_state_change_vote(block_height, voter_index, 1 /*worker_index*/, service_nodes::new_state::decommission, service_node_voter.pub, service_node_voter.sec);
  {
    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::verify_vote(valid_vote, block_height, vvc, state);
    if (!result)
      printf("%s\n", cryptonote::print_vote_verification_context(vvc, &valid_vote));

    ASSERT_TRUE(result);
  }

  // Voters validator index out of bounds
  {
    auto vote           = valid_vote;
    vote.index_in_group = state.validators.size() + 10;
    vote.signature      = service_nodes::make_signature_from_vote(vote, service_node_voter.pub, service_node_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result                               = service_nodes::verify_vote(vote, block_height, vvc, state);
    ASSERT_FALSE(result);
  }

  // Voters worker index out of bounds
  {
    auto vote                      = valid_vote;
    vote.state_change.worker_index = state.workers.size() + 10;
    vote.signature = service_nodes::make_signature_from_vote(vote, service_node_voter.pub, service_node_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::verify_vote(vote, block_height, vvc, state);
    ASSERT_FALSE(result);
  }

  // Signature not valid
  {
    auto vote                       = valid_vote;
    cryptonote::keypair other_voter = cryptonote::keypair::generate(hw::get_device("default"));
    vote.signature                  = {};

    cryptonote::vote_verification_context vvc = {};
    bool result                               = service_nodes::verify_vote(vote, block_height, vvc, state);
    ASSERT_FALSE(result);
  }

  // Vote too old
  {
    cryptonote::vote_verification_context vvc = {};
    bool result                               = service_nodes::verify_vote(valid_vote, 1 /*latest_height*/, vvc, state);
    ASSERT_FALSE(result);
  }

  // Vote too far in the future
  {
    cryptonote::vote_verification_context vvc = {};
    bool result                               = service_nodes::verify_vote(valid_vote, block_height + 10000, vvc, state);
    ASSERT_FALSE(result);
  }
}

TEST(service_nodes, tx_extra_state_change_validation)
{
  // Generate a quorum and the voter
  const size_t num_voters = 10;
  cryptonote::keypair voters[num_voters] = {};

  service_nodes::testing_quorum state = {};
  {
    state.validators.resize(num_voters);
    state.workers.resize(num_voters);

    for (size_t i = 0; i < state.validators.size(); ++i)
    {
      voters[i]           = cryptonote::keypair::generate(hw::get_device("default"));
      state.validators[i] = voters[i].pub;
      state.workers[i]    = cryptonote::keypair::generate(hw::get_device("default")).pub;
    }
  }

  // Valid state_change
  cryptonote::tx_extra_service_node_state_change valid_state_change = {};
  uint8_t hf_version = cryptonote::network_version_11_infinite_staking;
  const uint64_t HEIGHT = 100;
  {
    valid_state_change.block_height       = HEIGHT - 1;
    valid_state_change.service_node_index = 1;
    valid_state_change.votes.reserve(num_voters);
    for (size_t i = 0; i < num_voters; ++i)
    {
      cryptonote::keypair const *voter                        = voters + i;
      cryptonote::tx_extra_service_node_state_change::vote vote = {};
      vote.validator_index                                    = i;
      vote.signature = service_nodes::make_signature_from_tx_state_change(valid_state_change, voter->pub, voter->sec);
      valid_state_change.votes.push_back(vote);
    }

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(valid_state_change, HEIGHT, tvc, state, hf_version);
    if (!result)
      printf("%s\n", cryptonote::print_tx_verification_context(tvc));
    ASSERT_TRUE(result);
  }

  // State Change has insufficient votes
  {
    auto state_change = valid_state_change;
    while (state_change.votes.size() >= service_nodes::STATE_CHANGE_MIN_VOTES_TO_CHANGE_STATE)
      state_change.votes.pop_back();

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(state_change, HEIGHT, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change has duplicated voter
  {
    auto state_change     = valid_state_change;
    state_change.votes[0] = state_change.votes[1];

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(state_change, HEIGHT, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change has one voter with invalid signature
  {
    auto state_change               = valid_state_change;
    state_change.votes[0].signature = state_change.votes[1].signature;

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(state_change, HEIGHT, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change has one voter with index out of bounds
  {
    auto state_change                     = valid_state_change;
    state_change.votes[0].validator_index = state.validators.size() + 10;

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(state_change, HEIGHT, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change service node index is out of bounds
  {
    auto state_change               = valid_state_change;
    state_change.service_node_index = state.workers.size() + 10;

    cryptonote::tx_verification_context tvc = {};
    bool result = service_nodes::verify_tx_state_change(state_change, HEIGHT, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change too old
  {
    auto state_change                         = valid_state_change;
    cryptonote::tx_verification_context tvc = {};
    bool result                               = service_nodes::verify_tx_state_change(state_change, 0, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }

  // State Change too new
  {
    auto state_change                         = valid_state_change;
    cryptonote::tx_verification_context tvc = {};
    bool result                               = service_nodes::verify_tx_state_change(state_change, HEIGHT + 1000, tvc, state, hf_version);
    ASSERT_FALSE(result);
  }
}

TEST(service_nodes, min_portions)
{

  uint8_t hf_version = cryptonote::network_version_9_service_nodes;
  // Test new contributors can *NOT* stake to a registration with under 25% of the total stake if there is more than 25% available.
  {
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {0, STAKING_PORTIONS}));
  }

  {
    const auto small = MIN_PORTIONS - 1;
    const auto rest = STAKING_PORTIONS - small;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {small, rest}));
  }

  {
    /// TODO: fix this test
    const auto small = MIN_PORTIONS - 1;
    const auto rest = STAKING_PORTIONS - small - STAKING_PORTIONS / 2;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {STAKING_PORTIONS / 2, small, rest}));
  }

  {
    const auto small = MIN_PORTIONS - 1;
    const auto rest = STAKING_PORTIONS - small - 2 * MIN_PORTIONS;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {MIN_PORTIONS, MIN_PORTIONS, small, rest}));
  }

  // Test new contributors *CAN* stake as the last person with under 25% if there is less than 25% available.

  // Two contributers
  {
    const auto large = 4 * (STAKING_PORTIONS / 5);
    const auto rest = STAKING_PORTIONS - large;
    bool result = service_nodes::check_service_node_portions(hf_version, {large, rest});
    ASSERT_TRUE(result);
  }

  // Three contributers
  {
    const auto half = STAKING_PORTIONS / 2 - 1;
    const auto rest = STAKING_PORTIONS - 2 * half;
    bool result = service_nodes::check_service_node_portions(hf_version, {half, half, rest});
    ASSERT_TRUE(result);
  }

  // Four contributers
  {
    const auto third = STAKING_PORTIONS / 3 - 1;
    const auto rest = STAKING_PORTIONS - 3 * third;
    bool result = service_nodes::check_service_node_portions(hf_version, {third, third, third, rest});
    ASSERT_TRUE(result);
  }

  // ===== After hard fork v11 =====
  hf_version = cryptonote::network_version_11_infinite_staking;

  {
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {0, STAKING_PORTIONS}));
  }

  {
    const auto small = MIN_PORTIONS - 1;
    const auto rest = STAKING_PORTIONS - small;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {small, rest}));
  }

  {
    const auto small = STAKING_PORTIONS / 8;
    const auto rest = STAKING_PORTIONS - small - STAKING_PORTIONS / 2;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {STAKING_PORTIONS / 2, small, rest}));
  }

  {
    const auto small = MIN_PORTIONS - 1;
    const auto rest = STAKING_PORTIONS - small - 2 * MIN_PORTIONS;
    ASSERT_FALSE(service_nodes::check_service_node_portions(hf_version, {MIN_PORTIONS, MIN_PORTIONS, small, rest}));
  }

  // Test new contributors *CAN* stake as the last person with under 25% if there is less than 25% available.

  // Two contributers
  {
    const auto large = 4 * (STAKING_PORTIONS / 5);
    const auto rest = STAKING_PORTIONS - large;
    bool result = service_nodes::check_service_node_portions(hf_version, {large, rest});
    ASSERT_TRUE(result);
  }

  // Three contributers
  {
    const auto half = STAKING_PORTIONS / 2 - 1;
    const auto rest = STAKING_PORTIONS - 2 * half;
    bool result = service_nodes::check_service_node_portions(hf_version, {half, half, rest});
    ASSERT_TRUE(result);
  }

  // Four contributers
  {
    const auto third = STAKING_PORTIONS / 3 - 1;
    const auto rest = STAKING_PORTIONS - 3 * third;
    bool result = service_nodes::check_service_node_portions(hf_version, {third, third, third, rest});
    ASSERT_TRUE(result);
  }

  // New test for hf_v11: allow less than 25% stake in the presence of large existing contributions
  {
    const auto large = STAKING_PORTIONS / 2;
    const auto small_1 = STAKING_PORTIONS / 6;
    const auto small_2 = STAKING_PORTIONS / 6;
    const auto rest = STAKING_PORTIONS - large - small_1 - small_2;
    bool result = service_nodes::check_service_node_portions(hf_version, {large, small_1, small_2, rest});
    ASSERT_TRUE(result);
  }

}


// Test minimum stake contributions (should test pre and post this change)
TEST(service_nodes, min_stake_amount)
{
  /// pre v11
  uint64_t height            = 101250;
  uint8_t hf_version         = cryptonote::network_version_9_service_nodes;
  uint64_t stake_requirement = service_nodes::get_staking_requirement(cryptonote::MAINNET, height, hf_version);
  {
    const uint64_t reserved = stake_requirement / 2;
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1);
    ASSERT_EQ(min_stake, stake_requirement / 4);
  }

  {
    const uint64_t reserved = 5 * stake_requirement / 6;
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1);
    ASSERT_EQ(min_stake, stake_requirement / 6);
  }

  /// post v11
  hf_version = cryptonote::network_version_11_infinite_staking;
  stake_requirement = service_nodes::get_staking_requirement(cryptonote::MAINNET, height, hf_version);
  {
    // 50% reserved, with 1 contribution, max of 4- the minimum stake should be (50% / 3)
    const uint64_t reserved  = stake_requirement / 2;
    const uint64_t remaining = stake_requirement - reserved;
    uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1 /*num_contributions_locked*/);
    ASSERT_EQ(min_stake, remaining / 3);

    // As above, but with 2 contributions locked up, minimum stake should be (50% / 2)
    min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 2 /*num_contributions_locked*/);
    ASSERT_EQ(min_stake, remaining / 2);
  }

  {
    /// Cannot contribute less than 25% under normal circumstances
    const uint64_t reserved = stake_requirement / 4;
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1);
    ASSERT_FALSE(min_stake <= stake_requirement / 6);
  }

  {
    // Cannot contribute less than 25% as first contributor
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, 0, 0/*num_contributions_locked*/);
    ASSERT_TRUE(min_stake >= stake_requirement / 4);
  }

}

// Test service node receive rewards proportionate to the amount they contributed.
TEST(service_nodes, service_node_rewards_proportional_to_portions)
{
  {
    const auto reward_a = cryptonote::get_portion_of_reward(MIN_PORTIONS, COIN);
    const auto reward_b = cryptonote::get_portion_of_reward(3 * MIN_PORTIONS, COIN);
    ASSERT_TRUE(3 * reward_a == reward_b);
  }

  {
    const auto reward_a = cryptonote::get_portion_of_reward(STAKING_PORTIONS/2, COIN);
    const auto reward_b = cryptonote::get_portion_of_reward(STAKING_PORTIONS, COIN);
    ASSERT_TRUE(2 * reward_a == reward_b);
  }

}

TEST(service_nodes, service_node_get_locked_key_image_unlock_height)
{
  uint64_t lock_duration = service_nodes::staking_num_lock_blocks(cryptonote::MAINNET) / 2;

  {
    uint64_t curr_height   = 100;
    uint64_t expected      = curr_height + lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, curr_height);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t curr_height   = lock_duration - 1;
    uint64_t expected      = curr_height + lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, curr_height);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t curr_height   = lock_duration + 100;
    uint64_t expected      = curr_height + lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, curr_height);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t expected      = lock_duration + lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, lock_duration, lock_duration);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t register_height = lock_duration + 1;
    uint64_t curr_height     = register_height + 2;
    uint64_t expected        = curr_height + lock_duration;
    uint64_t unlock_height   = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, register_height, curr_height);
    ASSERT_EQ(unlock_height, expected);
  }
}
