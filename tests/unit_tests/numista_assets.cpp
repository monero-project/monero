// Copyright (c) 2026, The Numista Coins Project
// Portions Copyright (c) 2014-2026, The Monero Project
// See LICENSE for licensing and attribution details.

#include "gtest/gtest.h"

#include "crypto/crypto.h"
#include "crypto/generators.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/numista_assets.h"
#include "hardforks/hardforks.h"

namespace
{
  struct test_keypair
  {
    crypto::public_key public_key;
    crypto::secret_key secret_key;
  };

  test_keypair make_keypair()
  {
    test_keypair keys;
    crypto::generate_keys(keys.public_key, keys.secret_key);
    return keys;
  }

  void sign_with(const test_keypair& keys, cryptonote::tx_extra_numista_asset& operation)
  {
    ASSERT_TRUE(cryptonote::sign_numista_asset_operation(operation, keys.public_key, keys.secret_key));
  }
}

TEST(numista_assets, issuer_registers_users_creates_asset_and_mints_with_supply_cap)
{
  const test_keypair issuer = make_keypair();
  const test_keypair alice = make_keypair();
  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  const crypto::hash asset_id = crypto::rand<crypto::hash>();

  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice_1";
  operation.asset_key = alice.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::create_asset;
  operation.asset_id = asset_id;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.max_supply = 100;
  operation.metadata_hash = crypto::rand<crypto::hash>();
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::mint;
  operation.asset_id = asset_id;
  operation.amount = 75;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.recipient_username = "alice_1";
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));
  EXPECT_EQ(75, ledger.get_balance(asset_id, "alice_1"));

  operation = {};
  operation.action = cryptonote::numista_asset_action::mint;
  operation.asset_id = asset_id;
  operation.amount = 26;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.recipient_username = "alice_1";
  sign_with(issuer, operation);
  EXPECT_FALSE(ledger.apply(operation));
  EXPECT_EQ(75, ledger.get_balance(asset_id, "alice_1"));
}

TEST(numista_assets, owner_signature_transfers_fungible_units_between_registered_usernames)
{
  const test_keypair issuer = make_keypair();
  const test_keypair alice = make_keypair();
  const test_keypair bob = make_keypair();
  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  const crypto::hash asset_id = crypto::rand<crypto::hash>();

  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice";
  operation.asset_key = alice.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));
  operation = {};
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "bob";
  operation.asset_key = bob.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::create_asset;
  operation.asset_id = asset_id;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.max_supply = 100;
  operation.metadata_hash = crypto::rand<crypto::hash>();
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));
  operation = {};
  operation.action = cryptonote::numista_asset_action::mint;
  operation.asset_id = asset_id;
  operation.amount = 60;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.recipient_username = "alice";
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::transfer;
  operation.asset_id = asset_id;
  operation.amount = 17;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.username = "alice";
  operation.recipient_username = "bob";
  sign_with(alice, operation);
  ASSERT_TRUE(ledger.apply(operation));
  EXPECT_EQ(43, ledger.get_balance(asset_id, "alice"));
  EXPECT_EQ(17, ledger.get_balance(asset_id, "bob"));

  operation.amount = 1;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  sign_with(bob, operation);
  EXPECT_FALSE(ledger.apply(operation));
  EXPECT_EQ(43, ledger.get_balance(asset_id, "alice"));
  EXPECT_EQ(17, ledger.get_balance(asset_id, "bob"));
}

TEST(numista_assets, collectible_is_indivisible_and_cannot_be_minted_twice)
{
  const test_keypair issuer = make_keypair();
  const test_keypair alice = make_keypair();
  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  const crypto::hash asset_id = crypto::rand<crypto::hash>();

  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice";
  operation.asset_key = alice.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::create_asset;
  operation.asset_id = asset_id;
  operation.asset_type = cryptonote::numista_asset_type::collectible;
  operation.max_supply = 1;
  operation.metadata_hash = crypto::rand<crypto::hash>();
  sign_with(issuer, operation);
  ASSERT_TRUE(ledger.apply(operation));

  operation = {};
  operation.action = cryptonote::numista_asset_action::mint;
  operation.asset_id = asset_id;
  operation.amount = 2;
  operation.recipient_username = "alice";
  sign_with(issuer, operation);
  EXPECT_FALSE(ledger.apply(operation));
  EXPECT_EQ(0, ledger.get_balance(asset_id, "alice"));
}

TEST(numista_assets, operation_roundtrips_in_transaction_extra_and_signatures_bind_fields)
{
  const test_keypair issuer = make_keypair();
  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice";
  operation.asset_key = make_keypair().public_key;
  sign_with(issuer, operation);

  std::vector<uint8_t> extra;
  ASSERT_TRUE(cryptonote::add_numista_asset_operation(extra, operation));
  std::vector<uint8_t> sorted_extra;
  ASSERT_TRUE(cryptonote::sort_tx_extra(extra, sorted_extra));
  std::vector<cryptonote::tx_extra_field> fields;
  ASSERT_TRUE(cryptonote::parse_tx_extra(sorted_extra, fields));
  ASSERT_EQ(1, fields.size());
  const auto& parsed = boost::get<cryptonote::tx_extra_numista_asset>(fields.front());
  EXPECT_EQ(operation.username, parsed.username);
  EXPECT_EQ(operation.asset_key, parsed.asset_key);
  EXPECT_EQ(operation.signature, parsed.signature);

  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  EXPECT_TRUE(ledger.apply(parsed));
  cryptonote::tx_extra_numista_asset changed = parsed;
  changed.username = "mallory";
  EXPECT_FALSE(ledger.apply(changed));
}

TEST(numista_assets, transaction_extra_rejects_multiple_asset_operations)
{
  const test_keypair issuer = make_keypair();
  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice";
  operation.asset_key = make_keypair().public_key;
  sign_with(issuer, operation);

  std::vector<uint8_t> extra;
  ASSERT_TRUE(cryptonote::add_numista_asset_operation(extra, operation));
  ASSERT_TRUE(cryptonote::add_numista_asset_operation(extra, operation));
  std::string error;
  EXPECT_FALSE(ledger.apply_tx_extra(extra, &error));
  EXPECT_EQ("Multiple Numista asset operations in one transaction", error);
  EXPECT_FALSE(ledger.get_username_key("alice", operation.asset_key));
}

TEST(numista_assets, usernames_are_ascii_only)
{
  const test_keypair issuer = make_keypair();
  cryptonote::numista_asset_ledger ledger(issuer.public_key);
  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = std::string("alice") + static_cast<char>(0xE9);
  operation.asset_key = make_keypair().public_key;
  sign_with(issuer, operation);
  EXPECT_FALSE(ledger.apply(operation));
}

TEST(numista_assets, numista_testnet_is_isolated_and_activates_asset_fork)
{
  const cryptonote::config_t& numista_config = cryptonote::get_config(cryptonote::NUMISTA_TESTNET);
  const cryptonote::config_t& monero_mainnet_config = cryptonote::get_config(cryptonote::MAINNET);
  const cryptonote::config_t& monero_testnet_config = cryptonote::get_config(cryptonote::TESTNET);
  const cryptonote::config_t& monero_stagenet_config = cryptonote::get_config(cryptonote::STAGENET);
  EXPECT_NE(numista_config.NETWORK_ID, monero_mainnet_config.NETWORK_ID);
  EXPECT_NE(numista_config.NETWORK_ID, monero_testnet_config.NETWORK_ID);
  EXPECT_NE(numista_config.NETWORK_ID, monero_stagenet_config.NETWORK_ID);
  EXPECT_NE(numista_config.GENESIS_NONCE, monero_testnet_config.GENESIS_NONCE);
  EXPECT_NE(numista_config.P2P_DEFAULT_PORT, monero_testnet_config.P2P_DEFAULT_PORT);
  EXPECT_NE(numista_config.P2P_DEFAULT_PORT, monero_mainnet_config.P2P_DEFAULT_PORT);
  EXPECT_NE(numista_config.RPC_DEFAULT_PORT, monero_testnet_config.RPC_DEFAULT_PORT);
  EXPECT_NE(numista_config.CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX,
    monero_testnet_config.CRYPTONOTE_PUBLIC_ADDRESS_BASE58_PREFIX);
  ASSERT_EQ(2, num_numista_testnet_hard_forks);
  EXPECT_EQ(1, numista_testnet_hard_forks[0].version);
  EXPECT_EQ(17, numista_testnet_hard_forks[1].version);
  EXPECT_EQ(1, numista_testnet_hard_forks[1].height);
}

TEST(numista_assets, copying_ledger_provides_a_reversible_candidate_state)
{
  const test_keypair issuer = make_keypair();
  const test_keypair alice = make_keypair();
  const test_keypair bob = make_keypair();
  cryptonote::numista_asset_ledger chain_state(issuer.public_key);
  const crypto::hash asset_id = crypto::rand<crypto::hash>();

  cryptonote::tx_extra_numista_asset operation;
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "alice";
  operation.asset_key = alice.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(chain_state.apply(operation));
  operation = {};
  operation.action = cryptonote::numista_asset_action::register_username;
  operation.username = "bob";
  operation.asset_key = bob.public_key;
  sign_with(issuer, operation);
  ASSERT_TRUE(chain_state.apply(operation));
  operation = {};
  operation.action = cryptonote::numista_asset_action::create_asset;
  operation.asset_id = asset_id;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.max_supply = 100;
  operation.metadata_hash = crypto::rand<crypto::hash>();
  sign_with(issuer, operation);
  ASSERT_TRUE(chain_state.apply(operation));
  operation = {};
  operation.action = cryptonote::numista_asset_action::mint;
  operation.asset_id = asset_id;
  operation.amount = 60;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.recipient_username = "alice";
  sign_with(issuer, operation);
  ASSERT_TRUE(chain_state.apply(operation));

  cryptonote::numista_asset_ledger candidate_state = chain_state;
  operation = {};
  operation.action = cryptonote::numista_asset_action::transfer;
  operation.asset_id = asset_id;
  operation.amount = 20;
  operation.asset_type = cryptonote::numista_asset_type::fungible;
  operation.username = "alice";
  operation.recipient_username = "bob";
  sign_with(alice, operation);
  ASSERT_TRUE(candidate_state.apply(operation));
  std::vector<uint8_t> transfer_extra;
  ASSERT_TRUE(cryptonote::add_numista_asset_operation(transfer_extra, operation));

  EXPECT_EQ(60, chain_state.get_balance(asset_id, "alice"));
  EXPECT_EQ(0, chain_state.get_balance(asset_id, "bob"));
  EXPECT_EQ(40, candidate_state.get_balance(asset_id, "alice"));
  EXPECT_EQ(20, candidate_state.get_balance(asset_id, "bob"));

  std::string error;
  ASSERT_TRUE(candidate_state.revert_tx_extra(transfer_extra, &error)) << error;
  EXPECT_EQ(60, candidate_state.get_balance(asset_id, "alice"));
  EXPECT_EQ(0, candidate_state.get_balance(asset_id, "bob"));
}
