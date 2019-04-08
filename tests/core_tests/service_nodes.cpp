// Copyright (c) 2014-2018, The Monero Project
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

#include "chaingen.h"
#include "service_nodes.h"
#include "cryptonote_core/service_node_list.h"

using namespace std;

using namespace epee;
using namespace cryptonote;

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "sn_core_tests"

static const std::pair<const char*, const char*> service_node_keys[] = {
  { "1c09e237e7f451ee5df4ea6db970ba0b17597d788106df0c3b436a8ff8c94806",
    "06d471cbb6d67cae5002f18dd1c46ae6307b474b6fad4d720bd639513438f219" },
  { "8336af81ddffa292122064b8ffa26977b76efdcfa441604b6013f43dd8e90101",
    "fe979e53de1f82458f000d84bb8bfb6013153a7811a19a5b4d534794cb87c311" },
  { "b0169fced37d767a33a975ae3f8f7a12c3034e654024c62ce5de2a6e163b5b05",
    "beca119a8a5581d0209a155f974e5331f08df48dbb3e11399f87867aae1973cc" },
  { "ee451af460163d7940a79f48890509ebc3efb5aad014c21964707dea8c604d0f",
    "7897152e723644d325eb10e677197cfebd630c4e6be6908330ec1f47094e8b35" },
  { "d0704bb98564800a5554c400da220b0265b338c37b80a5cd8af4b010b57cb301",
    "50483a668bf25d989a25d87f233835df3357c7e72000762529984d8c111a2c52" },
  { "878b8c4560499fdb929ebd5d9bd124603bcc7f3495c9581edcbdf7360fa51706",
    "467ba9a442a9d2dfa8e4f9f07dd49e40554b299a9144f3fef392e8c5e1038cd7" },
  { "9dae6882b90ddc63b1832500fcccd7d027ea1081db9127f27316d16708b96c0a",
    "151c6ccbfd05465d2752d6794dc2892381a284750597b8b321b712036cda3c4a" },
  { "91bab98d4c9bfbe39c84effb68cb7a68020f16583f0eca0e60c1cae1cf13920e",
    "b16768d1a95d653310941c75e9ab9dcb6c79019195def04fd2279f67e32a4b7b" },
  { "a0a5daa0ddd972551147c48a223800c27a38eb156ff97d9c0e7595089bc04203",
    "090788a7faeab266a3fe1d755df1e58a989e676ea5d94a2c748b5b94ad30ee30" },
  { "992f5da1cd19727a03becaacd21251485ddf4ffaa827b44d6c5ff9a2ec1d5402",
    "f20624bc19ec1486ad49c0994faf4523b471fa53bf0bf856d142cdaf13b93165" },
  { "1a87c618ff444e324a0212e26e14af65670d566363b204300102afcaa085590c",
    "f3bd64b0ba024ad8068b2e96213e6b7d354d13f468a2775e2fd22a85f352aa73" },
  { "8b079185e6e1df1b61d18a7b71fdea149e6555271fae055c28e3fe7174abc107",
    "3b4d8725de3a16615358dad594114224a9fff21d5c6f4680528840b17e19e02b" },
  { "a01f14e058a0c414577e978f3e290cf662ae670fdee6699c57b14c9c4a452706",
    "ff099e244736503eb32b6a869674c9bb1f2a29590026f2680968cbae9c466a41" },
  { "3c9d7ecac68bb506c0eb4b1de21839f14afd59ae292b55ce7c104322eca4e308",
    "2a1d51e4f6f36ab79f1c642a9ab109eb34527fd1e4e9f130127662ca3c8a0fb9" },
  { "af7e04cf1f0816c66ee0b72fcad40e577bbffd10a391245c2b5b29c87a7a1304",
    "0a25fae0eeb6418a291d0e93410510f2cb66b1cfa897398c9196737fbe19f35d" },
  { "b861b35dc2479652c37d5afcc4231b6363172ee73e4bf88b52a1a33e52490f0f",
    "03cc9506ecf7acd5b13bbb5d3b59e4d4e3ff408f70622b9db131d0c120b841d0" },
  { "834b35e68c7fc5bcf56ed772bb5d72e08dcc38b987f16e52d79e9df2f9b7ec00",
    "5f40d0c72036a85163ea97a23ef67b8c52c06f5eee59bfb03db1ff07b761604c" },
  { "441e071ba14153d0a931f9eaa7ee4dbae4306d2921865f2f3a7cc625b4fbc60b",
    "836bbceec1b80f82e2edb43e3583cbc3a09925e16c5c66a08b94899de5fb59ce" },
  { "9c6e82db18c537c333b28b515cb25708ed42e22f3c9948511890389ea79e3d08",
    "2d848c307f1e734af4dd1f09cc132038d6e0a5fc8223e4548094a3e354f43aec" },
  { "3f4472d29b95c742ad092ca603f2b888f7a699b000d9fdb7ca003ee86218cf02",
    "356cf984c6a9efd2ca965bfdbb2d94d2906113e1a7acabf5dbda6a4911bd43fc" },
  { "78b784f007f7d1f80c6b6da2627b4f47c37f88ac96c7a0006e02727cd2b7770b",
    "b1f24cf86553b1fc72ed5a9a09286200cdf62dabc52683ba61ee95b6d9b64679" },
  { "c8489fe5119e2ea00116a33ac38ec0636312dadeffe3a85967ae5243afd3a005",
    "9d6bbe3c47d0994bd3383e88e84f1ab4daac4fd28843490ce5d101f1b52d4875" },
  { "7b9a1077d3ba10833b596d4c254ae2dc26c8b801807a3c61f78e8055aba19a04",
    "b88af38e69c4fd44d95c3f41b551d9c41edb834331be2e85a3693f7a3eaa2004" },
  { "8376b213f06785788bde97027caab42384f2249d272c4eefc15ef9cc02efa302",
    "5e2033fe1a5d4bf21952934b2a4b005d57075861baafc26d98c7e090d8ecbcac" },
  { "fad6778760c4f357559131f77ea3f2fb7e92cfb660bad72c0a37b29f8e37030c",
    "30660777ff79db10e735be0c79732d0cbd15d5812a59e3467fb6b4a7bc71a601" }
};

static const auto SN_KEYS_COUNT = sizeof(service_node_keys) / sizeof(service_node_keys[0]);

static_assert(SN_KEYS_COUNT == 25, "Need to adjust the key count (for readability)");

cryptonote::keypair get_static_keys(size_t idx) {

  if (idx >= SN_KEYS_COUNT) { MERROR("out of bounds"); throw std::exception(); }

  cryptonote::keypair keypair;

  epee::string_tools::hex_to_pod(service_node_keys[idx].first, keypair.sec);
  epee::string_tools::hex_to_pod(service_node_keys[idx].second, keypair.pub);

  return keypair;
}


//-----------------------------------------------------------------------------------------------------
//---------------------------------- Generate Service Nodes -------------------------------------------
//-----------------------------------------------------------------------------------------------------
gen_service_nodes::gen_service_nodes()
{
  /// NOTE: we don't generate random keys here, because the verification will call its own constructor
  m_alice_service_node_keys = get_static_keys(0);

  REGISTER_CALLBACK("check_registered", gen_service_nodes::check_registered);
  REGISTER_CALLBACK("check_expired", gen_service_nodes::check_expired);
}
//-----------------------------------------------------------------------------------------------------
bool gen_service_nodes::generate(std::vector<test_event_entry> &events) const
{

  const get_test_options<gen_service_nodes> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);
  gen.create_genesis_block();

  const auto miner = gen.first_miner();
  const auto alice = gen.create_account();

  gen.rewind_until_version(network_version_9_service_nodes);
  gen.rewind_blocks_n(10);

  gen.rewind_blocks();

  const auto tx0 = gen.create_tx(miner, alice, MK_COINS(101));
  gen.create_block({tx0});

  gen.rewind_blocks();

  const auto reg_tx = gen.create_registration_tx(alice, m_alice_service_node_keys);

  gen.create_block({reg_tx});

  DO_CALLBACK(events, "check_registered");

  for (auto i = 0u; i < service_nodes::staking_num_lock_blocks(cryptonote::FAKECHAIN); ++i) {
    gen.create_block();
  }

  DO_CALLBACK(events, "check_expired");

  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_service_nodes::check_registered(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_service_nodes::check_registered");

  cryptonote::account_base alice = boost::get<cryptonote::account_base>(events[1]);

  std::vector<block> blocks;
  size_t count = 15 + (2 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW);
  bool r = c.get_blocks((uint64_t)0, count, blocks);
  CHECK_TEST_CONDITION(r);
  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);

  // Expect the change to have unlock time of 0, and we get that back immediately ~0.8 loki
  // 101 (balance) - 100 (stake) - 0.2 (test fee) = 0.8 loki
  const uint64_t unlocked_balance    = get_unlocked_balance(alice, blocks, mtx);
  const uint64_t staking_requirement = MK_COINS(100);

  CHECK_EQ(MK_COINS(101) - TESTS_DEFAULT_FEE - staking_requirement, unlocked_balance);

  /// check that alice is registered
  const auto info_v = c.get_service_node_list_state({m_alice_service_node_keys.pub});
  CHECK_EQ(info_v.empty(), false);

  return true;
}
//-----------------------------------------------------------------------------------------------------
bool gen_service_nodes::check_expired(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("gen_service_nodes::check_expired");

  cryptonote::account_base alice = boost::get<cryptonote::account_base>(events[1]);

  const auto stake_lock_time = service_nodes::staking_num_lock_blocks(cryptonote::FAKECHAIN);

  std::vector<block> blocks;
  size_t count = 15 + (2 * CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW) + stake_lock_time;
  bool r = c.get_blocks((uint64_t)0, count, blocks);
  CHECK_TEST_CONDITION(r);
  std::vector<cryptonote::block> chain;
  map_hash2tx_t mtx;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);

  /// check that alice's registration expired
  const auto info_v = c.get_service_node_list_state({m_alice_service_node_keys.pub});
  CHECK_EQ(info_v.empty(), true);

  /// check that alice received some service node rewards (TODO: check the balance precisely)
  CHECK_TEST_CONDITION(get_balance(alice, blocks, mtx) > MK_COINS(101) - TESTS_DEFAULT_FEE);

  return true;

}
//-----------------------------------------------------------------------------------------------------
//------------------------------ Test Blocks Prefer Deregisters ---------------------------------------
//-----------------------------------------------------------------------------------------------------
test_prefer_deregisters::test_prefer_deregisters() {
  REGISTER_CALLBACK("check_prefer_deregisters", test_prefer_deregisters::check_prefer_deregisters);
}
//-----------------------------------------------------------------------------------------------------
bool test_prefer_deregisters::generate(std::vector<test_event_entry> &events)
{
  const get_test_options<test_prefer_deregisters> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);

  gen.create_genesis_block();

  const auto miner = gen.first_miner();
  const auto alice = gen.create_account();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// give miner some outputs to spend and unlock them
  gen.rewind_blocks_n(60);
  gen.rewind_blocks();

  /// register 12 random service nodes
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0; i < 12; ++i) {
    const auto tx = gen.create_registration_tx();
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);

  /// generate transactions to fill up txpool entirely
  for (auto i = 0u; i < 45; ++i) {
    gen.create_tx(miner, alice, MK_COINS(1), TESTS_DEFAULT_FEE * 100);
  }

  /// generate two deregisters
  const auto pk1 = gen.get_test_pk(0);
  const auto pk2 = gen.get_test_pk(1);

  gen.build_deregister(pk1).build();
  gen.build_deregister(pk2).build();

  DO_CALLBACK(events, "check_prefer_deregisters");

  return true;
}
//-----------------------------------------------------------------------------------------------------
bool test_prefer_deregisters::check_prefer_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{

  DEFINE_TESTS_ERROR_CONTEXT("test_prefer_deregisters::check_prefer_deregisters");

  const auto tx_count = c.get_pool_transactions_count();

  cryptonote::block full_blk;
  {
    difficulty_type diffic;
    uint64_t height;
    uint64_t expected_reward;
    cryptonote::blobdata extra_nonce;
    const auto miner = boost::get<cryptonote::account_base>(events[1]);
    c.get_block_template(full_blk, miner.get_keys().m_account_address, diffic, height, expected_reward, extra_nonce);
  }

  map_hash2tx_t mtx;
  {
    std::vector<cryptonote::block> chain;
    CHECK_TEST_CONDITION(find_block_chain(events, chain, mtx, get_block_hash(boost::get<cryptonote::block>(events[0]))));
  }

  const auto deregister_count =
    std::count_if(full_blk.tx_hashes.begin(), full_blk.tx_hashes.end(), [&mtx](const crypto::hash& tx_hash) {
      return mtx[tx_hash]->get_type() == cryptonote::transaction::type_deregister;
    });

  /// test that there are more transactions in tx pool
  CHECK_TEST_CONDITION(tx_count > full_blk.tx_hashes.size());
  /// test that all 2 deregister tx are in the block
  CHECK_EQ(deregister_count, 2);

  return true;

}
//-----------------------------------------------------------------------------------------------------
//------------------------------ Test Zero-Fee Deregisters Fail ---------------------------------------
//-----------------------------------------------------------------------------------------------------
test_zero_fee_deregister::test_zero_fee_deregister() {
  // Test that deregister that are non-zero fee don't get accepted onto the blockchain or anywhere.
  REGISTER_CALLBACK_METHOD(test_zero_fee_deregister, mark_invalid_tx);
}
//-----------------------------------------------------------------------------------------------------
bool test_zero_fee_deregister::generate(std::vector<test_event_entry> &events)
{

  const get_test_options<test_zero_fee_deregister> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);

  gen.create_genesis_block();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// give miner some outputs to spend and unlock them
  gen.rewind_blocks_n(20);
  gen.rewind_blocks();

  /// register 11 random service nodes
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0; i < 11; ++i) {
    const auto tx = gen.create_registration_tx();
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);
  /// create a deregister with a fee, should fail
  DO_CALLBACK(events, "mark_invalid_tx");
  const auto pk = gen.get_test_pk(0);
  gen.build_deregister(pk).with_fee(MK_COINS(1)).build();

  return true;
}
//-----------------------------------------------------------------------------------------------------
//-------------------------------- Test Deregister Safety Buffer --------------------------------------
//-----------------------------------------------------------------------------------------------------

// Test if a person registers onto the network and they get included in the nodes to test (i.e. heights 0, 5, 10). If
// they get dereigstered in the nodes to test, height 5, and rejoin the network before height 10 (and are in the nodes
// to test), they don't get deregistered.
test_deregister_safety_buffer::test_deregister_safety_buffer() {
  REGISTER_CALLBACK_METHOD(test_deregister_safety_buffer, mark_invalid_tx);
}
//-----------------------------------------------------------------------------------------------------
bool test_deregister_safety_buffer::generate(std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_deregister_safety_buffer::generate");

  const get_test_options<test_deregister_safety_buffer> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);

  gen.create_genesis_block();

  const auto miner = gen.first_miner();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// give miner some outputs to spend and unlock them
  gen.rewind_blocks_n(40);
  gen.rewind_blocks();

  /// save generated keys here
  std::vector<cryptonote::keypair> used_sn_keys;

  /// register 21 random service nodes
  std::vector<cryptonote::transaction> reg_txs;

  constexpr auto SERVICE_NODES_NEEDED = service_nodes::QUORUM_SIZE * 2 + 1;
  static_assert(SN_KEYS_COUNT >= SERVICE_NODES_NEEDED, "not enough pre-computed service node keys");

  for (auto i = 0u; i < SERVICE_NODES_NEEDED; ++i)
  {
    const auto sn_keys = get_static_keys(i);
    used_sn_keys.push_back(sn_keys);

    const auto tx = gen.create_registration_tx(miner, sn_keys);
    reg_txs.push_back(tx);
  }

  gen.create_block({reg_txs});

  const auto heightA = gen.height();

  /// Note: would've used sets, but need `less` for public keys etc.
  std::vector<crypto::public_key> sn_set_A;

  for (const auto& sn : gen.get_quorum_idxs(heightA).to_test) {
    sn_set_A.push_back(sn.sn_pk);
  }

  /// create 5 blocks and find public key to be tested twice
  for (auto i = 0; i < 5; ++i) {
    gen.create_block();
  }

  const auto heightB = gen.height();

  std::vector<crypto::public_key> sn_set_B;

  for (const auto& sn : gen.get_quorum_idxs(heightB).to_test) {
    sn_set_B.push_back(sn.sn_pk);
  }

  /// Guaranteed to contain at least one element
  std::vector<crypto::public_key> tested_twice;

  for (const auto& sn : sn_set_A) {
    if (std::find(sn_set_B.begin(), sn_set_B.end(), sn) != sn_set_B.end()) {
      tested_twice.push_back(sn);
    }
  }

  /// Pick a node to deregister
  const auto pk = tested_twice.at(0);

  /// Deregister for heightA
  {
    const auto dereg_tx = gen.build_deregister(pk).with_height(heightA).build();
    gen.create_block({dereg_tx});
  }

  /// Register the node again
  {
    // find secret key for pk:
    auto it = std::find_if(used_sn_keys.begin(), used_sn_keys.end(), [&pk](const cryptonote::keypair& keys) {
      return keys.pub == pk;
    });

    if (it == used_sn_keys.end()) { MERROR("SN pub key not found in generated keys"); return false; };

    const auto tx = gen.create_registration_tx(miner, *it);
    gen.create_block({tx});
  }

  /// Try to deregister the node again for heightB (should fail)
  {
    DO_CALLBACK(events, "mark_invalid_tx");
    const auto dereg_tx = gen.build_deregister(pk).with_height(heightB).build();
  }

  return true;

}

//-----------------------------------------------------------------------------------------------------
//---------------------------------- Test Deregisters on Split ----------------------------------------
//-----------------------------------------------------------------------------------------------------
// Test a chain that is equal up to a certain point, splits, and 1 of the chains forms a block that has a deregister
// for Service Node A. Chain 2 receives a deregister for Service Node A with a different permutation of votes than
// the one known in Chain 1 and is sitting in the mempool. On reorg, Chain 1 should become the canonical chain and
// those sitting on Chain 2 should not have problems switching over.
test_deregisters_on_split::test_deregisters_on_split()
{
  REGISTER_CALLBACK_METHOD(test_deregisters_on_split, mark_invalid_tx);
  REGISTER_CALLBACK("test_on_split", test_deregisters_on_split::test_on_split);
}
//-----------------------------------------------------------------------------------------------------
bool test_deregisters_on_split::generate(std::vector<test_event_entry> &events)
{
  const get_test_options<test_deregisters_on_split> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);
  gen.create_genesis_block();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// generate some outputs and unlock them
  gen.rewind_blocks_n(20);
  gen.rewind_blocks();
 
  /// register 12 random service nodes
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0; i < 12; ++i) {
    const auto sn = get_static_keys(i);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);

  /// chain split
  auto fork = gen;

  /// public key of the node to deregister (valid at the height of the pivot block)
  const auto pk = gen.get_test_pk(0);
  const auto split_height = gen.height();

  /// create deregistration A
  auto quorumA = gen.get_quorum_idxs(split_height).voters;
  quorumA.erase(quorumA.begin()); /// remove first voter
  const auto dereg_A = gen.build_deregister(pk).with_voters(quorumA).with_height(split_height).build();

  /// create deregistration on alt chain (B)
  auto quorumB = fork.get_quorum_idxs(split_height).voters;
  quorumB.erase(quorumB.begin() + 1); /// remove second voter
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, true);
  const auto dereg_B = fork.build_deregister(pk).with_voters(quorumB).with_height(split_height).build(); /// events[68]
  SET_EVENT_VISITOR_SETT(events, event_visitor_settings::set_txs_keeped_by_block, false);

  /// continue main chain with deregister A
  gen.create_block({dereg_A});

  /// continue alt chain with deregister B
  fork.create_block({ dereg_B });

  /// one more block on alt chain to switch
  fork.create_block();

  DO_CALLBACK(events, "test_on_split");

  return true;
}
//-----------------------------------------------------------------------------------------------------
/// Check that the deregister transaction is the one from the alternative branch
bool test_deregisters_on_split::test_on_split(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_deregisters_on_split::test_on_split");

  /// obtain the expected deregister from events
  const size_t dereg_idx = 68;
  auto dereg_tx = boost::get<cryptonote::transaction>(events.at(dereg_idx));
  CHECK_AND_ASSERT_MES(dereg_tx.get_type() == cryptonote::transaction::type_deregister, false, "event is not a deregister transaction");

  const auto expected_tx_hash = get_transaction_hash(dereg_tx);

  /// find a deregister transaction in the blockchain
  std::vector<block> blocks;
  bool r = c.get_blocks(0, 1000, blocks);
  CHECK_TEST_CONDITION(r);

  map_hash2tx_t mtx;
  std::vector<cryptonote::block> chain;
  r = find_block_chain(events, chain, mtx, get_block_hash(blocks.back()));
  CHECK_TEST_CONDITION(r);

  /// get the second last block; it contains the deregister
  const auto blk = blocks[blocks.size() - 2];

  /// find the deregister tx:
  const auto found_tx_hash = std::find_if(blk.tx_hashes.begin(), blk.tx_hashes.end(), [&mtx](const crypto::hash& hash) {
    return mtx.at(hash)->is_deregister;
  });

  CHECK_TEST_CONDITION(found_tx_hash != blk.tx_hashes.end());

  /// check that it is the expected one
  CHECK_EQ(*found_tx_hash, expected_tx_hash);

  return true;
}
//-----------------------------------------------------------------------------------------------------
//------------------------------ Should not Add Old Deregistration -------------------------------------
//-----------------------------------------------------------------------------------------------------

// Daemon A has a deregistration TX (X) in the pool. Daemon B creates a block before receiving X.
// Daemon A accepts the block without X. Now X is too old and should not be added in future blocks.
deregister_too_old::deregister_too_old()
{
  REGISTER_CALLBACK_METHOD(deregister_too_old, mark_invalid_block);
}
//-----------------------------------------------------------------------------------------------------
bool deregister_too_old::generate(std::vector<test_event_entry>& events)
{
  const get_test_options<deregister_too_old> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);
  gen.create_genesis_block();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// generate some outputs and unlock them
  gen.rewind_blocks_n(20);
  gen.rewind_blocks();
 
  /// register 11 service nodes (10 voters and 1 to test)
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0; i < 11; ++i) {
    const auto sn = get_static_keys(i);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);

  // create a deregister for this height (without committing)
  const auto pk = gen.get_test_pk(0);
  const auto dereg_tx = gen.build_deregister(pk, false).build();

  /// create enough blocks to make deregistrations invalid (60 blocks)
  gen.rewind_blocks_n(service_nodes::deregister_vote::DEREGISTER_LIFETIME_BY_HEIGHT);

  /// In the real world, this transaction should not make it into a block, but in this case we do try to add it (as in
  /// tests we must add specify transactions manually), which should exercise the same validation code and reject the
  /// block
  DO_CALLBACK(events, "mark_invalid_block");
  gen.create_block({dereg_tx});

  return true;
}
//-----------------------------------------------------------------------------------------------------
//---------------------------- Test Rollback System for Service Nodes ---------------------------------
//-----------------------------------------------------------------------------------------------------
sn_test_rollback::sn_test_rollback()
{
  REGISTER_CALLBACK("test_registrations", sn_test_rollback::test_registrations);
}
//-----------------------------------------------------------------------------------------------------
bool sn_test_rollback::generate(std::vector<test_event_entry>& events)
{
  const get_test_options<sn_test_rollback> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);
  gen.create_genesis_block();

  gen.rewind_until_version(network_version_9_service_nodes);

  /// generate some outputs and unlock them
  gen.rewind_blocks_n(20);
  gen.rewind_blocks();

  constexpr auto INIT_SN_COUNT = 11;

  /// register some service nodes
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0; i < INIT_SN_COUNT; ++i) {
    const auto sn = get_static_keys(i);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);

  /// create a few blocks with active service nodes
  gen.rewind_blocks_n(5);

  /// chain split here
  auto fork = gen;

  // deregister some node (A) on main
  {
    const auto pk = gen.get_test_pk(0);
    const auto dereg_tx = gen.build_deregister(pk).build();
    gen.create_block({dereg_tx});
  }

  /// create a new service node (B) in the next block
  {
    const auto sn = get_static_keys(INIT_SN_COUNT);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    gen.create_block({tx});
  }

  /// create blocks on the alt chain and trigger chain switch
  fork.rewind_blocks_n(3);

  // create a few more blocks to test winner selection
  fork.rewind_blocks_n(15);

  DO_CALLBACK(events, "test_registrations");

  return true;
}

using sn_info_t = service_nodes::service_node_pubkey_info;

static bool contains(const std::vector<sn_info_t>& infos, const crypto::public_key& key)
{
  const auto it =
    std::find_if(infos.begin(), infos.end(), [&key](const sn_info_t& info) { return info.pubkey == key; });
  return it != infos.end();
}

bool sn_test_rollback::test_registrations(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{

  DEFINE_TESTS_ERROR_CONTEXT("sn_test_rollback::test_registrations");

  const auto sn_list = c.get_service_node_list_state({});

  /// Test that node A is still registered
  {
    /// obtain public key of node A
    constexpr size_t dereg_evnt_idx = 70;

    const auto event_a = events.at(dereg_evnt_idx);

    CHECK_TEST_CONDITION(event_a.type() == typeid(cryptonote::transaction));
    const auto dereg_tx = boost::get<cryptonote::transaction>(event_a);
    CHECK_TEST_CONDITION(dereg_tx.get_type() == transaction::type_deregister);

    tx_extra_service_node_deregister deregistration;
    get_service_node_deregister_from_tx_extra(dereg_tx.extra, deregistration);

    const auto quorum_state = c.get_quorum_state(deregistration.block_height);
    CHECK_TEST_CONDITION(quorum_state);
    const auto pk_a = quorum_state->nodes_to_test.at(deregistration.service_node_index);

    /// Check present
    const bool found_a = contains(sn_list, pk_a);
    CHECK_AND_ASSERT_MES(found_a, false, "Node deregistered in alt chain is not found in the main chain after reorg.");
  }

  /// Test that node B is not registered
  {
    /// obtain public key of node B
    constexpr size_t reg_evnt_idx = 72;
    const auto event_b = events.at(reg_evnt_idx);
    CHECK_TEST_CONDITION(event_b.type() == typeid(cryptonote::transaction));
    const auto reg_tx = boost::get<cryptonote::transaction>(event_b);

    crypto::public_key pk_b;
    if (!cryptonote::get_service_node_pubkey_from_tx_extra(reg_tx.extra, pk_b)) {
      MERROR("Could not get service node key from tx extra");
      return false;
    }

    /// Check not present
    const bool found_b = contains(sn_list, pk_b);
    CHECK_AND_ASSERT_MES(!found_b, false, "Node registered in alt chain is present in the main chain after reorg.");
  }

  return true;

}

//-----------------------------------------------------------------------------------------------------
//------------------------------------- Test Swarm Basics ---------------------------------------------
//-----------------------------------------------------------------------------------------------------

test_swarms_basic::test_swarms_basic() {
  REGISTER_CALLBACK("test_initial_swarms", test_swarms_basic::test_initial_swarms);
  REGISTER_CALLBACK("test_with_one_more_sn", test_swarms_basic::test_with_one_more_sn);
  REGISTER_CALLBACK("test_with_more_sn", test_swarms_basic::test_with_more_sn);
  REGISTER_CALLBACK("test_after_first_deregisters", test_swarms_basic::test_after_first_deregisters);
  REGISTER_CALLBACK("test_after_final_deregisters", test_swarms_basic::test_after_final_deregisters);
}

bool test_swarms_basic::generate(std::vector<test_event_entry>& events)
{
  const get_test_options<test_swarms_basic> test_options = {};
  linear_chain_generator gen(events, test_options.hard_forks);

  gen.rewind_until_version(network_version_9_service_nodes);

  /// Create some service nodes before hf version 10
  constexpr size_t INIT_SN_COUNT = 13;

  gen.rewind_blocks_n(100);
  gen.rewind_blocks();

  /// register some service nodes
  std::vector<cryptonote::transaction> reg_txs;
  for (auto i = 0u; i < INIT_SN_COUNT; ++i) {
    const auto sn = get_static_keys(i);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    reg_txs.push_back(tx);
  }

  gen.create_block(reg_txs);

  /// create a few blocks with active service nodes
  gen.rewind_blocks_n(5);

  if (gen.get_hf_version() != network_version_9_service_nodes) {
    std::cerr << "wrong hf version\n";
    return false;
  }

  gen.rewind_until_version(network_version_10_bulletproofs);

  /// test that we now have swarms
  DO_CALLBACK(events, "test_initial_swarms");

  /// rewind some blocks and register 1 more service node
  {
    const auto sn = get_static_keys(INIT_SN_COUNT);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    gen.create_block({tx});
  }

  /// test that another swarm has been created
  DO_CALLBACK(events, "test_with_one_more_sn");


  for (auto i = INIT_SN_COUNT + 1; i < SN_KEYS_COUNT; ++i) {
    const auto sn = get_static_keys(i);
    const auto tx = gen.create_registration_tx(gen.first_miner(), sn);
    gen.create_block({tx});
  }

  /// test that another swarm has been created
  DO_CALLBACK(events, "test_with_more_sn");

  /// deregister enough snode to bring all 3 swarm to the min size
  std::vector<cryptonote::transaction> dereg_txs;

  const size_t excess = SN_KEYS_COUNT - 3 * service_nodes::EXCESS_BASE;
  for (size_t i = 0; i < excess; ++i) {
    const auto pk = gen.get_test_pk(i);
    const auto tx = gen.build_deregister(pk).build();
    dereg_txs.push_back(tx);
  }

  gen.create_block(dereg_txs);

  DO_CALLBACK(events, "test_after_first_deregisters");

  /// deregister 1 snode, which should trigger a decommission
  dereg_txs.clear();
  {
    const auto pk = gen.get_test_pk(0);
    const auto tx = gen.build_deregister(pk).build();
    dereg_txs.push_back(tx);
  }

  gen.create_block(dereg_txs);

  DO_CALLBACK(events, "test_after_final_deregisters");

  /// test (implicitly) that deregistered nodes do not receive rewards
  gen.rewind_blocks_n(5);

  return true;
}

bool test_swarms_basic::test_initial_swarms(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_swarms_basic::test_initial_swarms");

  /// Check that there is one active swarm and the swarm queue is not empty
  const auto sn_list = c.get_service_node_list_state({});

  std::map<service_nodes::swarm_id_t, std::vector<crypto::public_key>> swarms;

  for (const auto& entry : sn_list) {
    const auto id = entry.info.swarm_id;
    swarms[id].push_back(entry.pubkey);
  }

  CHECK_EQ(swarms.size(), 1);
  CHECK_EQ(swarms.begin()->second.size(), 13);

  return true;
}

bool test_swarms_basic::test_with_one_more_sn(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_swarms_basic::test_with_one_more_sn");

  const auto sn_list = c.get_service_node_list_state({});

  std::map<service_nodes::swarm_id_t, std::vector<crypto::public_key>> swarms;

  for (const auto& entry : sn_list) {
    const auto id = entry.info.swarm_id;
    swarms[id].push_back(entry.pubkey);
  }

  CHECK_EQ(swarms.size(), 2);

  return true;
}

bool test_swarms_basic::test_with_more_sn(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_swarms_basic::test_with_more_sn");

  const auto sn_list = c.get_service_node_list_state({});

  std::map<service_nodes::swarm_id_t, std::vector<crypto::public_key>> swarms;

  for (const auto& entry : sn_list) {
    const auto id = entry.info.swarm_id;
    swarms[id].push_back(entry.pubkey);
  }

  CHECK_EQ(swarms.size(), 3);

  return true;
}

bool test_swarms_basic::test_after_first_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_swarms_basic::test_after_first_deregisters");

  const auto sn_list = c.get_service_node_list_state({});

  std::map<service_nodes::swarm_id_t, std::vector<crypto::public_key>> swarms;

  for (const auto& entry : sn_list) {
    const auto id = entry.info.swarm_id;
    swarms[id].push_back(entry.pubkey);
  }

  CHECK_EQ(swarms.size(), 3);

  return true;
}

bool test_swarms_basic::test_after_final_deregisters(cryptonote::core& c, size_t ev_index, const std::vector<test_event_entry> &events)
{
  DEFINE_TESTS_ERROR_CONTEXT("test_swarms_basic::test_after_first_deregisters");

  const auto sn_list = c.get_service_node_list_state({});

  std::map<service_nodes::swarm_id_t, std::vector<crypto::public_key>> swarms;

  for (const auto& entry : sn_list) {
    const auto id = entry.info.swarm_id;
    swarms[id].push_back(entry.pubkey);
  }


  CHECK_EQ(swarms.size(), 2);

  return true;
}
