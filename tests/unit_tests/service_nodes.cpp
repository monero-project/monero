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
#include "cryptonote_core/service_node_deregister.h"
#include "cryptonote_basic/cryptonote_basic.h"
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
    uint64_t mainnet_requirement   = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    uint64_t stagenet_requirement  = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);
    ASSERT_EQ(stagenet_requirement, (45000 * COIN));
    ASSERT_EQ(mainnet_requirement,  (45000 * COIN));
  }

  // Starting height for stagenet
  {
    uint64_t height = 96210;
    uint64_t stagenet_requirement  = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);
    ASSERT_EQ(stagenet_requirement, (45000 * COIN));
  }

  // Starting height for mainnet
  {
    // NOTE: The maximum staking requirement is 50,000, in atomic units is 50,000,000,000,000 < int64 range (2^63-1)
    // so casting is safe.
    uint64_t height = 101250;
    int64_t mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    int64_t stagenet_requirement = (int64_t)service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    ASSERT_EQ(mainnet_requirement,  (45000 * COIN));

    int64_t stagenet_expected = (int64_t)((44069 * COIN) + 151880000);
    int64_t stagenet_delta    = std::abs(stagenet_requirement - stagenet_expected);
    ASSERT_LT(stagenet_delta, atomic_epsilon);
  }

  // Check the requirements are decreasing
  {
    uint64_t height = 250000;
    int64_t mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    int64_t stagenet_requirement = (int64_t)service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    int64_t  mainnet_expected = (int64_t)((25796 * COIN) + 364642307);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);

    int64_t stagenet_expected = (int64_t)((25376 * COIN) + 249888366);
    int64_t stagenet_delta    = std::abs(stagenet_requirement - stagenet_expected);
    ASSERT_LT(stagenet_delta, atomic_epsilon);
  }

  // Bottom of the curve, generally this should be the lowest the staking requirement will be
  {
    uint64_t height = 1036800;
    int64_t  mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    int64_t  stagenet_requirement = (int64_t)service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    int64_t  mainnet_expected = (int64_t)((10234 * COIN) + 967482165);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);

    int64_t  stagenet_expected = (int64_t)((10228 * COIN) + 718366740);
    int64_t  stagenet_delta    = std::abs(stagenet_requirement - stagenet_expected);
    ASSERT_LT(stagenet_delta, atomic_epsilon);
  }

  // RHS of Rewards Formula, 1st part
  // Where the two equations should meet and staking formula equalizes
  {
    uint64_t height = 1166400;
    uint64_t mainnet_requirement  = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    uint64_t stagenet_requirement = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);
    ASSERT_EQ(mainnet_requirement,  (10250 * COIN));
    ASSERT_EQ(stagenet_requirement, (10250 * COIN));
  }

  // Checking the requirements still equal
  {
    uint64_t height = 1296000;
    uint64_t mainnet_requirement  = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    uint64_t stagenet_requirement = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    ASSERT_EQ(mainnet_requirement,  (10500 * COIN));
    ASSERT_EQ(stagenet_requirement, (10500 * COIN));
  }

  // Checking we are approaching 15000
  {
    uint64_t height = 3000000;
    int64_t  mainnet_requirement  = (int64_t)service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    int64_t  stagenet_requirement = (int64_t)service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    int64_t  mainnet_expected = (int64_t)((13787 * COIN) + 37037037);
    int64_t  mainnet_delta    = std::abs(mainnet_requirement - mainnet_expected);
    ASSERT_LT(mainnet_delta, atomic_epsilon);

    int64_t  stagenet_expected = (int64_t)((13787 * COIN) + 37037037);
    int64_t  stagenet_delta    = std::abs(stagenet_requirement - stagenet_expected);
    ASSERT_LT(stagenet_delta, atomic_epsilon);
  }

  // RHS of Rewards Formula, 2nd part
  // Last part of formula maxes out at 15000 if height > 3628800
  {
    uint64_t height = 3628800;
    uint64_t mainnet_requirement  = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    uint64_t stagenet_requirement = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    ASSERT_EQ(mainnet_requirement,  (15000 * COIN));
    ASSERT_EQ(stagenet_requirement, (15000 * COIN));
  }

  // Check we stay capped at 15000
  {
    uint64_t height = 4082400;
    uint64_t mainnet_requirement  = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);
    uint64_t stagenet_requirement = service_nodes::get_staking_requirement(cryptonote::STAGENET, height);

    ASSERT_EQ(mainnet_requirement,  (15000 * COIN));
    ASSERT_EQ(stagenet_requirement, (15000 * COIN));
  }
}

TEST(service_nodes, vote_validation)
{
  // Generate a quorum and the voter
  cryptonote::keypair service_node_voter = cryptonote::keypair::generate(hw::get_device("default"));
  int voter_index = 0;

  service_nodes::quorum_state state = {};
  {
    state.quorum_nodes.resize(10);
    state.nodes_to_test.resize(state.quorum_nodes.size());

    for (size_t i = 0; i < state.quorum_nodes.size(); ++i)
    {
      state.quorum_nodes[i] = (i == voter_index) ? service_node_voter.pub : cryptonote::keypair::generate(hw::get_device("default")).pub;
      state.nodes_to_test[i] = cryptonote::keypair::generate(hw::get_device("default")).pub;
    }
  }

  // Valid vote
  service_nodes::deregister_vote valid_vote = {};
  {
    valid_vote.block_height         = 10;
    valid_vote.service_node_index   = 1;
    valid_vote.voters_quorum_index  = voter_index;
    valid_vote.signature            = service_nodes::deregister_vote::sign_vote(valid_vote.block_height, valid_vote.service_node_index, service_node_voter.pub, service_node_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_vote(cryptonote::MAINNET, valid_vote, vvc, state);
    if (!result)
      printf("%s\n", cryptonote::print_vote_verification_context(vvc, &valid_vote));

    ASSERT_TRUE(result);
  }

  // Voters quorum index out of bounds
  {
    auto vote                = valid_vote;
    vote.voters_quorum_index = state.quorum_nodes.size() + 10;
    vote.signature           = service_nodes::deregister_vote::sign_vote(vote.block_height, vote.service_node_index, service_node_voter.pub, service_node_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_vote(cryptonote::MAINNET, vote, vvc, state);
    ASSERT_FALSE(result);
  }

  // Voters service node index out of bounds
  {
    auto vote               = valid_vote;
    vote.service_node_index = state.nodes_to_test.size() + 10;
    vote.signature          = service_nodes::deregister_vote::sign_vote(vote.block_height, vote.service_node_index, service_node_voter.pub, service_node_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_vote(cryptonote::MAINNET, vote, vvc, state);
    ASSERT_FALSE(result);
  }

  // Signature not valid
  {
    auto vote                       = valid_vote;
    cryptonote::keypair other_voter = cryptonote::keypair::generate(hw::get_device("default"));
    vote.signature                  = service_nodes::deregister_vote::sign_vote(vote.block_height, vote.service_node_index, other_voter.pub, other_voter.sec);

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_vote(cryptonote::MAINNET, vote, vvc, state);
    ASSERT_FALSE(result);
  }
}

TEST(service_nodes, tx_extra_deregister_validation)
{
  // Generate a quorum and the voter
  const size_t num_voters = 10;
  cryptonote::keypair voters[num_voters] = {};

  service_nodes::quorum_state state = {};
  {
    state.quorum_nodes.resize(num_voters);
    state.nodes_to_test.resize(num_voters);

    for (size_t i = 0; i < state.quorum_nodes.size(); ++i)
    {
      voters[i]              = cryptonote::keypair::generate(hw::get_device("default"));
      state.quorum_nodes[i]  = voters[i].pub;
      state.nodes_to_test[i] = cryptonote::keypair::generate(hw::get_device("default")).pub;
    }
  }

  // Valid deregister
  cryptonote::tx_extra_service_node_deregister valid_deregister = {};
  {
    valid_deregister.block_height       = 10;
    valid_deregister.service_node_index = 1;
    valid_deregister.votes.reserve(num_voters);
    for (size_t i = 0; i < num_voters; ++i)
    {
      cryptonote::keypair const *voter                        = voters + i;
      cryptonote::tx_extra_service_node_deregister::vote vote = {};

      vote.voters_quorum_index = i;
      vote.signature           = service_nodes::deregister_vote::sign_vote(valid_deregister.block_height, valid_deregister.service_node_index, voter->pub, voter->sec);
      valid_deregister.votes.push_back(vote);
    }

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, valid_deregister, vvc, state);
    if (!result)
      printf("%s\n", cryptonote::print_vote_verification_context(vvc));
    ASSERT_TRUE(result);
  }

  // Deregister has insufficient votes
  {
    auto deregister = valid_deregister;
    while (deregister.votes.size() >= service_nodes::MIN_VOTES_TO_KICK_SERVICE_NODE)
      deregister.votes.pop_back();

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, deregister, vvc, state);
    ASSERT_FALSE(result);
  }

  // Deregister has duplicated voter
  {
    auto deregister     = valid_deregister;
    deregister.votes[0] = deregister.votes[1];

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, deregister, vvc, state);
    ASSERT_FALSE(result);
  }

  // Deregister has one voter with invalid signature
  {
    auto deregister               = valid_deregister;
    deregister.votes[0].signature = deregister.votes[1].signature;

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, deregister, vvc, state);
    ASSERT_FALSE(result);
  }

  // Deregister has one voter with index out of bounds
  {
    auto deregister                         = valid_deregister;
    deregister.votes[0].voters_quorum_index = state.quorum_nodes.size() + 10;

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, deregister, vvc, state);
    ASSERT_FALSE(result);
  }

  // Deregister service node index is out of bounds
  {
    auto deregister               = valid_deregister;
    deregister.service_node_index = state.nodes_to_test.size() + 10;

    cryptonote::vote_verification_context vvc = {};
    bool result = service_nodes::deregister_vote::verify_deregister(cryptonote::MAINNET, deregister, vvc, state);
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
  hf_version = cryptonote::network_version_11_swarms;

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
  uint64_t height = 101250;
  const uint64_t stake_requirement = service_nodes::get_staking_requirement(cryptonote::MAINNET, height);

  /// pre v11
  uint8_t hf_version = cryptonote::network_version_9_service_nodes;

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
  hf_version = cryptonote::network_version_11_swarms;
  {
    /// Should be able to contribute roughly one sixth of the total requirement, but not less
    const uint64_t reserved = stake_requirement / 2;
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1);
    ASSERT_EQ(min_stake, stake_requirement / 6);
  }

  {
    /// Cannot contribute less than 25% under normal circumstances
    const uint64_t reserved = stake_requirement / 4;
    const uint64_t min_stake = service_nodes::get_min_node_contribution(hf_version, stake_requirement, reserved, 1);
    ASSERT_FALSE(min_stake <= stake_requirement / 6);
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
  uint64_t lock_duration = service_nodes::staking_initial_num_lock_blocks(cryptonote::MAINNET);

  {
    uint64_t expected      = lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, 100);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t expected      = lock_duration;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, lock_duration - 1);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t expected      = lock_duration * 2;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, 0, lock_duration + 100);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t expected      = lock_duration * 2;
    uint64_t unlock_height = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, lock_duration, lock_duration);
    ASSERT_EQ(unlock_height, expected);
  }

  {
    uint64_t register_height = lock_duration + 1;
    uint64_t curr_height     = register_height + 2;
    uint64_t expected        = register_height + lock_duration;
    uint64_t unlock_height   = service_nodes::get_locked_key_image_unlock_height(cryptonote::MAINNET, register_height, curr_height);
    ASSERT_EQ(unlock_height, expected);
  }
}
