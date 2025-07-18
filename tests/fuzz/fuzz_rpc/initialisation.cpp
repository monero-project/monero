#include "initialisation.h"

#include "crypto/crypto.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/tx_extra.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "p2p/net_node.h"
#include "p2p/net_node.inl"
#include "ringct/rctTypes.h"
#include "serialization/binary_utils.h"
#include "serialization/variant.h"

#include <boost/program_options/variables_map.hpp>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

// For storing valid transaction hashes
std::vector<crypto::hash> cached_tx_hashes;

// Configure a dummy protocol to avoid external requests
bool DummyProtocol::is_synchronized() const {
  return true;
}

bool DummyProtocol::relay_transactions(cryptonote::NOTIFY_NEW_TRANSACTIONS::request&, const boost::uuids::uuid&, epee::net_utils::zone, cryptonote::relay_method) {
  return true;
}

bool DummyProtocol::relay_block(cryptonote::NOTIFY_NEW_FLUFFY_BLOCK::request&, cryptonote::cryptonote_connection_context&) {
  return true;
}

// Function to create and initialise a dummy rpc core object
std::unique_ptr<CoreEnv> initialise_rpc_core() {
  auto env = std::make_unique<CoreEnv>();

  // Create fresh pointer for dummy protocol and core
  env->protocol = std::make_unique<DummyProtocol>();
  env->core = std::make_unique<cryptonote::core>(env->protocol.get());

  // Configure the rpc core object with default settings
  boost::program_options::options_description desc;
  cryptonote::core::init_options(desc);
  boost::program_options::variables_map vm;

  // Add command line argument to configure the rpc core object to use regression testing mode (FAKECHAIN)
  // Enabling FAKECHAIN mode allows skipping validation logic of the authors signature and transactions ID
  // on valid blocks and transactions while keeping other logic
  std::vector<const char*> argv = {"fuzz", "--regtest"};
  boost::program_options::store(boost::program_options::parse_command_line(argv.size(), argv.data(), desc), vm);
  boost::program_options::notify(vm);

  // Initialise the dummy core with all the above settings
  env->core->init(vm);

  return env;
}

// Build the core_rpc_server handler object with the configured dummy core object
std::unique_ptr<RpcServerBundle> initialise_rpc_server(cryptonote::core& dummy_core, FuzzedDataProvider& provider, bool need_payment) {
  // Create rpc server bundle object
  auto bundle = std::make_unique<RpcServerBundle>();

  // Allocate new protocol and node server then create RPC server object
  bundle->proto_handler = std::make_unique<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>(dummy_core, nullptr, true);
  bundle->proto_handler->set_no_sync(false);
  bundle->dummy_p2p = std::make_unique<nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core>>>(*bundle->proto_handler);
  bundle->rpc = std::make_unique<cryptonote::core_rpc_server>(dummy_core, *bundle->dummy_p2p);

  // Set up dummy variable map for rpc initialisation with payment
  if (need_payment) {
    boost::program_options::variables_map vm;
    boost::program_options::options_description desc{"fuzz"};
    command_line::add_arg(desc, cryptonote::arg_data_dir);
    command_line::add_arg(desc, cryptonote::arg_testnet_on);
    command_line::add_arg(desc, cryptonote::arg_stagenet_on);
    cryptonote::core_rpc_server::init_options(desc);

    // Generate random address and use it if it is a valid address with valid format
    std::string address_arg;
    if (provider.remaining_bytes() > 95) {
      std::string random_str = provider.ConsumeBytesAsString(95);
      if (!random_str.empty() && std::all_of(random_str.begin(), random_str.end(), [](char c) {
            return isalnum(c);
          })) {
        address_arg = "--rpc-payment-address=" + random_str;
      }
    }

    // Fall back to default hardcoded address if generated address is invalid
    if (address_arg.empty()) {
      address_arg = "--rpc-payment-address=44AFFq5kSiGBoZKfRLKFY7bUuS5JxqLkZ3Zf1diYv5ZdfTP7hS5gZtSGdgjNXmYGFzRiV3yTgF8Yf4zrhGcq14D3z8PUnHT";
    }

    // Provide needed payment related configuration for init of core_rpc_server
    std::vector<const char*> argv = {
      "fuzz",
      address_arg.c_str()
    };
    boost::program_options::store(boost::program_options::parse_command_line(argv.size(), argv.data(), desc), vm);
    boost::program_options::notify(vm);
    bool success = bundle->rpc->init(vm, true, "18089", true, "");
    if (!success) {
      // Revert back to a fresh core_rpc_server if payment module init is failed
      bundle->rpc = std::make_unique<cryptonote::core_rpc_server>(dummy_core, *bundle->dummy_p2p);
    }
  }

  return bundle;
}

bool generate_random_blocks(cryptonote::core& core, FuzzedDataProvider& provider) {
  static std::vector<cryptonote::block> cached_blocks;
  static std::vector<cryptonote::blobdata> cached_txs;

  bool use_cached_blocks = provider.ConsumeBool();
  bool use_cached_txs = provider.ConsumeBool();

  if (!use_cached_blocks || cached_blocks.empty()) {
    // Create new cached blocks
    cached_blocks.clear();

    // Prepare the genesis block (initial base block) in the block chains
    cryptonote::block genesis_block;
    cryptonote::generate_genesis_block(genesis_block, config::GENESIS_TX, config::GENESIS_NONCE);
    cached_blocks.push_back(genesis_block);

    // Setup initial values
    const size_t median_weight = 300000;
    const size_t block_weight = 100000;
    const uint8_t version = core.get_blockchain_storage().get_current_hard_fork_version();

    // Accumulate coins for genesis block
    uint64_t coins = 0;
    for (const auto& o : genesis_block.miner_tx.vout) {
      coins += o.amount;
    }

    // Create random number of miners
    size_t miner_count = provider.ConsumeIntegralInRange<size_t>(1, 4);
    std::vector<cryptonote::account_base> miners(miner_count);
    for (auto& miner : miners) {
      miner.generate();
    }

    // Create random number of blocks
    size_t total_blocks = provider.ConsumeIntegralInRange<size_t>(2, 15);
    for (size_t i = 0; i < total_blocks; ++i) {
      // Stop generate blocks if no more bytes left
      if (provider.remaining_bytes() <= 0) {
        break;
      }

      // Randomly link a miner identity to this block
      cryptonote::block blk;
      size_t selected_miner_index = provider.ConsumeIntegralInRange<size_t>(0, miners.size() - 1);
      cryptonote::account_base& miner = miners[selected_miner_index];

      // Initialise block property with random value
      uint64_t height = cached_blocks.size();
      crypto::hash prev_id = cached_blocks.back().hash;
      blk.major_version = version;
      blk.minor_version = version;
      blk.timestamp = core.get_blockchain_storage().get_adjusted_time(height);
      blk.prev_id = prev_id;
      blk.nonce = provider.ConsumeIntegral<uint32_t>();

      // Calculate and accumulate reward
      uint64_t base_reward = 0, fee = 0;
      if (!cryptonote::get_block_reward(median_weight, block_weight, coins, base_reward, version)) {
        break;
      }

      // Link empty miner transaction to block
      cryptonote::transaction miner_tx;
      cryptonote::construct_miner_tx(
        height, median_weight, coins, block_weight,
        fee, miner.get_keys().m_account_address,
        miner_tx, cryptonote::blobdata(),
        1, blk.major_version
      );
      for (const auto& o : miner_tx.vout) {
        coins += o.amount;
      }
      miner_tx.version = 2;
      blk.miner_tx = miner_tx;

      // Add the block to the cache
      cached_blocks.push_back(blk);
    }
  }

  if (cached_blocks.empty()) {
    // No random block generated, exit early to next iteration
    return false;
  }

  if (!use_cached_txs || cached_txs.empty()) {
    // Create new cache transactions
    cached_txs.clear();
    cached_tx_hashes.clear();

    // Random number of transactions to generate
    const size_t tx_count = provider.ConsumeIntegralInRange<size_t>(1, 4);
    for (size_t i = 0; i < tx_count; ++i) {
      // Stop generate transactions if no more bytes left
      if (provider.remaining_bytes() <= 0) {
        break;
      }

      // Initialise variables
      cryptonote::transaction tx;
      std::vector<cryptonote::tx_source_entry> sources;
      std::vector<cryptonote::tx_destination_entry> destinations;
      std::vector<uint8_t> extra;

      // Generate sender account
      cryptonote::account_base sender;
      sender.generate();

      // Generate receiver account
      cryptonote::account_base receiver;
      receiver.generate();

      // Generate random transfer amount
      uint64_t transfer_amount = provider.ConsumeIntegralInRange<uint64_t>(10000, 1000000000);
      uint64_t input_amount = transfer_amount + 10000;

      // Prepare source entry
      cryptonote::tx_source_entry src{};
      src.amount = input_amount;
      src.rct = false;
      src.real_output = 0;
      src.real_output_in_tx_index = 0;

      // Derive valid public key
      hw::device &hwdev = sender.get_keys().get_device();
      cryptonote::keypair tx_key = cryptonote::keypair::generate(hwdev);
      crypto::key_derivation derivation;
      if (!crypto::generate_key_derivation(sender.get_keys().m_account_address.m_view_public_key, tx_key.sec, derivation)) {
        continue;
      }
      crypto::public_key out_pub_key;
      if (!crypto::derive_public_key(derivation, 0, sender.get_keys().m_account_address.m_spend_public_key, out_pub_key)) {
        continue;
      }
      src.real_out_tx_key = tx_key.pub;

      // Configure output entry for source_entry
      for (size_t j = 0; j < 10; ++j) {
        cryptonote::tx_source_entry::output_entry oe;
        oe.first = j;
        crypto::public_key pk;

        if (j == src.real_output) {
          pk = out_pub_key;
        } else {
          cryptonote::keypair fake = cryptonote::keypair::generate(hwdev);
          pk = fake.pub;
        }

        crypto::view_tag vtag{};
        vtag.data = static_cast<uint8_t>(provider.ConsumeIntegral<uint8_t>());

        cryptonote::txout_to_tagged_key tagged{};
        tagged.key = pk;
        tagged.view_tag = vtag;

        oe.second.dest = *reinterpret_cast<const rct::key*>(&tagged.key);
        oe.second.mask = rct::identity();
        src.outputs.push_back(oe);
      }
      sources.push_back(src);

      // Prepare destination entry
      cryptonote::tx_destination_entry dst;
      dst.amount = transfer_amount;
      dst.addr = receiver.get_keys().m_account_address;
      dst.is_subaddress = false;
      destinations.push_back(dst);

      // Construct the transaction
      crypto::secret_key tx_secret_key;
      std::vector<crypto::secret_key> additional_tx_keys;
      std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
      subaddresses[sender.get_keys().m_account_address.m_spend_public_key] = {0, 0};
      std::vector<cryptonote::tx_destination_entry> destinations_copy = destinations;

      // Construct transaction objects
      bool success = cryptonote::construct_tx_and_get_tx_key(
        sender.get_keys(),
        subaddresses,
        sources,
        destinations_copy,
        boost::none,
        extra,
        tx,
        tx_secret_key,
        additional_tx_keys,
        true,
        {rct::RangeProofPaddedBulletproof, 0},
        true
      );
      if (!success) {
        continue;
      }

      // Force version and unlock_time
      tx.version = 2;
      tx.unlock_time = 0;

      // Serialise the transaction
      cryptonote::blobdata tx_blob;
      if (!cryptonote::t_serializable_object_to_blob(tx, tx_blob)) {
        continue;
      }

      // Add the transaction to cache
      cached_txs.push_back(tx_blob);
    }
  }

  core.get_blockchain_storage().get_db().batch_start();

  bool added_block = false;
  for (const auto& blk : cached_blocks) {
    cryptonote::block_verification_context bvc{};
    if (core.get_blockchain_storage().add_new_block(blk, bvc)) {
      added_block = true;
    }
  }

  bool added_txs = false;
  for (const auto& tx_blob : cached_txs) {
    cryptonote::tx_verification_context tvc;
    bool accepted = core.handle_incoming_tx(tx_blob, tvc, cryptonote::relay_method::block, true);
    if (accepted || tvc.m_added_to_pool) {
      added_txs = true;

      // Store legit hashes
      cryptonote::transaction tx;
      if (cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx)) {
        cached_tx_hashes.push_back(cryptonote::get_transaction_hash(tx));
      }
    }
  }

  return added_block;
}
