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

#include "include_base_utils.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "misc_language.h"
#include "string_tools.h"

using namespace cryptonote;

#include <boost/regex.hpp>
#include "common/util.h"
#include "common/command_line.h"
#include "trezor_tests.h"
#include "device/device_cold.hpp"
#include "device_trezor/device_trezor.hpp"


namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_filter                      = { "filter", "Regular expression filter for which tests to run" };
  const command_line::arg_descriptor<bool>        arg_generate_and_play_test_data = {"generate_and_play_test_data", ""};
  const command_line::arg_descriptor<std::string> arg_trezor_path                 = {"trezor_path", "Path to the trezor device to use, has to support debug link", ""};
  const command_line::arg_descriptor<bool>        arg_heavy_tests                 = {"heavy_tests", "Runs expensive tests (volume tests with real device)", false};
  const command_line::arg_descriptor<std::string> arg_chain_path                  = {"chain_path", "Path to the serialized blockchain, speeds up testing", ""};
  const command_line::arg_descriptor<bool>        arg_fix_chain                   = {"fix_chain", "If chain_patch is given and file cannot be used, it is ignored and overwriten", false};
}


#define TREZOR_ACCOUNT_ORDERING &m_miner_account, &m_alice_account, &m_bob_account, &m_eve_account
#define TREZOR_COMMON_TEST_CASE(genclass, CORE, BASE)                                                           \
  rollback_chain(CORE, BASE.head_block());                                                                      \
  {                                                                                                             \
    genclass ctest;                                                                                             \
    BASE.fork(ctest);                                                                                           \
    GENERATE_AND_PLAY_INSTANCE(genclass, ctest, *(CORE));                                                       \
  }

#define TREZOR_SETUP_CHAIN(NAME) do {                                                                           \
  ++tests_count;                                                                                                \
  try {                                                                                                         \
    setup_chain(&core, trezor_base, chain_path, fix_chain);                                                     \
  } catch (const std::exception& ex) {                                                                          \
    failed_tests.emplace_back("gen_trezor_base " #NAME);                                                        \
  }                                                                                                             \
} while(0)

static void rollback_chain(cryptonote::core * core, const cryptonote::block & head);
static void setup_chain(cryptonote::core ** core, gen_trezor_base & trezor_base, std::string chain_path, bool fix_chain);

int main(int argc, char* argv[])
{
  TRY_ENTRY();
    tools::on_startup();
    epee::string_tools::set_module_name_and_folder(argv[0]);

    //set up logging options
    mlog_configure(mlog_get_default_log_path("trezor_tests.log"), true);
    mlog_set_log_level(2);

    po::options_description desc_options("Allowed options");
    command_line::add_arg(desc_options, command_line::arg_help);
    command_line::add_arg(desc_options, arg_filter);
    command_line::add_arg(desc_options, arg_trezor_path);
    command_line::add_arg(desc_options, arg_heavy_tests);
    command_line::add_arg(desc_options, arg_chain_path);
    command_line::add_arg(desc_options, arg_fix_chain);

    po::variables_map vm;
    bool r = command_line::handle_error_helper(desc_options, [&]()
    {
      po::store(po::parse_command_line(argc, argv, desc_options), vm);
      po::notify(vm);
      return true;
    });
    if (!r)
      return 1;

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << desc_options << std::endl;
      return 0;
    }

    const std::string filter = tools::glob_to_regex(command_line::get_arg(vm, arg_filter));
    boost::smatch match;

    size_t tests_count = 0;
    std::vector<std::string> failed_tests;
    std::string trezor_path = command_line::get_arg(vm, arg_trezor_path);
    std::string chain_path = command_line::get_arg(vm, arg_chain_path);
    const bool heavy_tests = command_line::get_arg(vm, arg_heavy_tests);
    const bool fix_chain = command_line::get_arg(vm, arg_fix_chain);

    hw::trezor::register_all();

    // Bootstrapping common chain & accounts
    cryptonote::core * core = nullptr;

    gen_trezor_base trezor_base;
    trezor_base.setup_args(trezor_path, heavy_tests);
    trezor_base.rct_config({rct::RangeProofPaddedBulletproof, 1});  // HF9 tests

    TREZOR_SETUP_CHAIN("HF9");

    // Individual test cases using shared pre-generated blockchain.
    TREZOR_COMMON_TEST_CASE(gen_trezor_ki_sync, core, trezor_base);

    // Transaction tests
    TREZOR_COMMON_TEST_CASE(gen_trezor_1utxo, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_1utxo_paymentid_short, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_1utxo_paymentid_short_integrated, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_1utxo_paymentid_long, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo_acc1, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo_to_sub, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo_to_2sub, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo_to_1norm_2sub, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_2utxo_sub_acc_to_1norm_2sub, core, trezor_base);
    TREZOR_COMMON_TEST_CASE(gen_trezor_4utxo_to_7outs, core, trezor_base);

    if (trezor_base.heavy_tests())
    {
      TREZOR_COMMON_TEST_CASE(gen_trezor_many_utxo, core, trezor_base);
    }

    core->deinit();
    el::Level level = (failed_tests.empty() ? el::Level::Info : el::Level::Error);
    MLOG(level, "\nREPORT:");
    MLOG(level, "  Test run: " << tests_count);
    MLOG(level, "  Failures: " << failed_tests.size());
    if (!failed_tests.empty())
    {
      MLOG(level, "FAILED TESTS:");
      BOOST_FOREACH(auto test_name, failed_tests)
      {
        MLOG(level, "  " << test_name);
      }
    }

    return failed_tests.empty() ? 0 : 1;

  CATCH_ENTRY_L0("main", 1);
}

static void rollback_chain(cryptonote::core * core, const cryptonote::block & head)
{
  CHECK_AND_ASSERT_THROW_MES(core, "Core is null");

  block popped_block;
  std::vector<transaction> popped_txs;

  crypto::hash head_hash = get_block_hash(head), cur_hash{};
  uint64_t height = get_block_height(head), cur_height=0;

  do {
    core->get_blockchain_top(cur_height, cur_hash);

    if (cur_height <= height && head_hash == cur_hash)
      return;

    CHECK_AND_ASSERT_THROW_MES(cur_height > height, "Height differs");
    core->get_blockchain_storage().get_db().pop_block(popped_block, popped_txs);
  } while(true);
}

static bool unserialize_chain_from_file(std::vector<test_event_entry>& events, gen_trezor_base &test_base, const std::string& file_path)
{
  TRY_ENTRY();
    std::ifstream data_file;
    data_file.open( file_path, std::ios_base::binary | std::ios_base::in);
    if(data_file.fail())
      return false;
    try
    {
      boost::archive::portable_binary_iarchive a(data_file);
      test_base.clear();

      a >> events;
      a >> test_base;
      return true;
    }
    catch(...)
    {
      MWARNING("Chain deserialization failed");
      return false;
    }
  CATCH_ENTRY_L0("unserialize_chain_from_file", false);
}

static bool serialize_chain_to_file(std::vector<test_event_entry>& events, gen_trezor_base &test_base, const std::string& file_path)
{
  TRY_ENTRY();
    std::ofstream data_file;
    data_file.open( file_path, std::ios_base::binary | std::ios_base::out | std::ios::trunc);
    if(data_file.fail())
      return false;
    try
    {

      boost::archive::portable_binary_oarchive a(data_file);
      a << events;
      a << test_base;
      return !data_file.fail();
    }
    catch(...)
    {
      MWARNING("Chain deserialization failed");
      return false;
    }
    return false;
  CATCH_ENTRY_L0("serialize_chain_to_file", false);
}

static void setup_chain(cryptonote::core ** core, gen_trezor_base & trezor_base, std::string chain_path, bool fix_chain)
{
  std::vector<test_event_entry> events;
  const bool do_serialize = !chain_path.empty();
  const bool chain_file_exists = do_serialize && boost::filesystem::exists(chain_path);
  bool loaded = false;
  bool generated = false;

  if (chain_file_exists)
  {
    if (!unserialize_chain_from_file(events, trezor_base, chain_path))
    {
      MERROR("Failed to deserialize data from file: " << chain_path);
      if (!fix_chain)
      {
        throw std::runtime_error("Chain load error");
      }
    } else
    {
      trezor_base.load(events);
      generated = true;
      loaded = true;
    }
  }

  if (!generated)
  {
    try
    {
      generated = trezor_base.generate(events);

      if (generated && !loaded && do_serialize)
      {
        trezor_base.update_trackers(events);
        if (!serialize_chain_to_file(events, trezor_base, chain_path))
        {
          MERROR("Failed to serialize data to file: " << chain_path);
        }
      }
    }
    CATCH_REPLAY(gen_trezor_base);
  }

  trezor_base.fix_hf(events);
  if (generated && do_replay_events_get_core<gen_trezor_base>(events, core))
  {
    MGINFO_GREEN("#TEST-chain-init# Succeeded ");
  }
  else
  {
    MERROR("#TEST-chain-init# Failed ");
    throw std::runtime_error("Chain init error");
  }
}

static void add_hforks(std::vector<test_event_entry>& events, const v_hardforks_t& hard_forks)
{
  event_replay_settings repl_set;
  repl_set.hard_forks = boost::make_optional(hard_forks);
  events.push_back(repl_set);
}

static void add_top_hfork(std::vector<test_event_entry>& events, const v_hardforks_t& hard_forks)
{
  event_replay_settings repl_set;
  v_hardforks_t top_fork;
  top_fork.push_back(hard_forks.back());
  repl_set.hard_forks = boost::make_optional(top_fork);
  events.push_back(repl_set);
}

static crypto::public_key get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td)
{
  std::vector<tx_extra_field> tx_extra_fields;
  parse_tx_extra(td.m_tx.extra, tx_extra_fields);

  tx_extra_pub_key pub_key_field;
  THROW_WALLET_EXCEPTION_IF(!find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 0), tools::error::wallet_internal_error,
                            "Public key wasn't found in the transaction extra");
  const crypto::public_key tx_pub_key = pub_key_field.pub_key;
  bool two_found = find_tx_extra_field_by_type(tx_extra_fields, pub_key_field, 1);
  if (!two_found) {
    return tx_pub_key;
  } else {
    throw std::runtime_error("Unsupported tx pub resolution");
  }
}

static void setup_shim(hw::wallet_shim * shim)
{
  shim->get_tx_pub_key_from_received_outs = &get_tx_pub_key_from_received_outs;
}

bool get_short_payment_id(crypto::hash8 &payment_id8, const tools::wallet2::pending_tx &ptx, hw::device &hwdev)
{
  std::vector<tx_extra_field> tx_extra_fields;
  parse_tx_extra(ptx.tx.extra, tx_extra_fields); // ok if partially parsed
  cryptonote::tx_extra_nonce extra_nonce;
  if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
  {
    if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
    {
      if (ptx.dests.empty())
      {
        MWARNING("Encrypted payment id found, but no destinations public key, cannot decrypt");
        return false;
      }
      return hwdev.decrypt_payment_id(payment_id8, ptx.dests[0].addr.m_view_public_key, ptx.tx_key);
    }
  }
  return false;
}

static tools::wallet2::tx_construction_data get_construction_data_with_decrypted_short_payment_id(const tools::wallet2::pending_tx &ptx, hw::device &hwdev)
{
  tools::wallet2::tx_construction_data construction_data = ptx.construction_data;
  crypto::hash8 payment_id = crypto::null_hash8;
  if (get_short_payment_id(payment_id, ptx, hwdev))
  {
    // Remove encrypted
    remove_field_from_tx_extra(construction_data.extra, typeid(cryptonote::tx_extra_nonce));
    // Add decrypted
    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
    THROW_WALLET_EXCEPTION_IF(!add_extra_nonce_to_tx_extra(construction_data.extra, extra_nonce),
                              tools::error::wallet_internal_error, "Failed to add decrypted payment id to tx extra");
    MDEBUG("Decrypted payment ID: " << payment_id);
  }
  return construction_data;
}

static std::string get_payment_id(const std::vector<uint8_t> &tx_extra)
{
  std::vector<cryptonote::tx_extra_field> tx_extra_fields;
  cryptonote::parse_tx_extra(tx_extra, tx_extra_fields); // ok if partially parsed
  cryptonote::tx_extra_nonce extra_nonce;

  ::crypto::hash payment_id{};
  if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
  {
    ::crypto::hash8 payment_id8{};
    if(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
    {
      return std::string(payment_id8.data, 8);
    }
    else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
    {
      return std::string(payment_id.data, 32);
    }
  }
  return std::string();
}

static crypto::hash8 to_short_payment_id(const std::string & payment_id)
{
  crypto::hash8 payment_id_short;
  CHECK_AND_ASSERT_THROW_MES(payment_id.size() == 8, "Invalid argument");
  memcpy(payment_id_short.data, payment_id.data(), 8);
  return payment_id_short;
}

static crypto::hash to_long_payment_id(const std::string & payment_id)
{
  crypto::hash payment_id_long;
  CHECK_AND_ASSERT_THROW_MES(payment_id.size() == 32, "Invalid argument");
  memcpy(payment_id_long.data, payment_id.data(), 32);
  return payment_id_long;
}

static std::vector<uint8_t> build_payment_id_extra(const std::string & payment_id)
{
  std::vector<uint8_t> res;

  if (payment_id.size() == 8) {
    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, to_short_payment_id(payment_id));
    THROW_WALLET_EXCEPTION_IF(!add_extra_nonce_to_tx_extra(res, extra_nonce),
                              tools::error::wallet_internal_error, "Failed to add decrypted payment id to tx extra");

  } else if (payment_id.size() == 32){
    std::string extra_nonce;
    set_payment_id_to_tx_extra_nonce(extra_nonce, to_long_payment_id(payment_id));
    THROW_WALLET_EXCEPTION_IF(!add_extra_nonce_to_tx_extra(res, extra_nonce),
                              tools::error::wallet_internal_error, "Failed to add decrypted payment id to tx extra");
  }

  return res;
}

static cryptonote::address_parse_info init_addr_parse_info(cryptonote::account_public_address &addr, bool is_sub=false, boost::optional<crypto::hash8> payment_id = boost::none)
{
  cryptonote::address_parse_info res;
  res.address = addr;
  res.is_subaddress = is_sub;
  if (payment_id){
    res.has_payment_id = true;
    res.payment_id = payment_id.get();
  } else {
    res.has_payment_id = false;
  }
  return res;
}

static void expand_tsx(cryptonote::transaction &tx)
{
  auto & rv = tx.rct_signatures;
  if (rv.type == rct::RCTTypeFull)
  {
    rv.p.MGs.resize(1);
    rv.p.MGs[0].II.resize(tx.vin.size());
    for (size_t n = 0; n < tx.vin.size(); ++n)
      rv.p.MGs[0].II[n] = rct::ki2rct(boost::get<txin_to_key>(tx.vin[n]).k_image);
  }
  else if (rv.type == rct::RCTTypeSimple || rv.type == rct::RCTTypeBulletproof || rv.type == rct::RCTTypeBulletproof2)
  {
    CHECK_AND_ASSERT_THROW_MES(rv.p.MGs.size() == tx.vin.size(), "Bad MGs size");
    for (size_t n = 0; n < tx.vin.size(); ++n)
    {
      rv.p.MGs[n].II.resize(1);
      rv.p.MGs[n].II[0] = rct::ki2rct(boost::get<txin_to_key>(tx.vin[n]).k_image);
    }
  }
}

static std::vector<tools::wallet2*> vct_wallets(tools::wallet2* w1=nullptr, tools::wallet2* w2=nullptr, tools::wallet2* w3=nullptr, tools::wallet2* w4=nullptr, tools::wallet2* w5=nullptr)
{
  std::vector<tools::wallet2*> res;
  if (w1)
    res.push_back(w1);
  if (w2)
    res.push_back(w2);
  if (w3)
    res.push_back(w3);
  if (w4)
    res.push_back(w4);
  if (w5)
    res.push_back(w5);
  return res;
}

// gen_trezor_base
const uint64_t gen_trezor_base::m_ts_start = 1338224400;
const uint64_t gen_trezor_base::m_wallet_ts = m_ts_start - 60*60*24*4;
const std::string gen_trezor_base::m_device_name = "Trezor:udp";
const std::string gen_trezor_base::m_master_seed_str = "14821d0bc5659b24cafbc889dc4fc60785ee08b65d71c525f81eeaba4f3a570f";
const std::string gen_trezor_base::m_device_seed = "permit universe parent weapon amused modify essay borrow tobacco budget walnut lunch consider gallery ride amazing frog forget treat market chapter velvet useless topple";
const std::string gen_trezor_base::m_alice_spend_private = m_master_seed_str;
const std::string gen_trezor_base::m_alice_view_private = "a6ccd4ac344a295d1387f8d18c81bdd394f1845de84188e204514ef9370fd403";

gen_trezor_base::gen_trezor_base(){
  m_rct_config = {rct::RangeProofPaddedBulletproof, 1};
}

gen_trezor_base::gen_trezor_base(const gen_trezor_base &other):
    m_generator(other.m_generator), m_bt(other.m_bt), m_miner_account(other.m_miner_account),
    m_bob_account(other.m_bob_account), m_alice_account(other.m_alice_account), m_eve_account(other.m_eve_account),
    m_hard_forks(other.m_hard_forks), m_trezor(other.m_trezor), m_rct_config(other.m_rct_config)
{

}

void gen_trezor_base::setup_args(const std::string & trezor_path, bool heavy_tests)
{
  m_trezor_path = trezor_path.empty() ? m_device_name : std::string("Trezor:") + trezor_path;
  m_heavy_tests = heavy_tests;
}

void gen_trezor_base::setup_trezor()
{
  hw::device &hwdev = hw::get_device(m_trezor_path);
  m_trezor = dynamic_cast<hw::trezor::device_trezor *>(&hwdev);
  CHECK_AND_ASSERT_THROW_MES(m_trezor, "Dynamic cast failed");

  m_trezor->set_debug(true);  // debugging commands on Trezor (auto-confirm transactions)

  CHECK_AND_ASSERT_THROW_MES(m_trezor->set_name(m_trezor_path), "Could not set device name " << m_trezor_path);
  m_trezor->set_network_type(MAINNET);
  m_trezor->set_derivation_path("");  // empty derivation path

  CHECK_AND_ASSERT_THROW_MES(m_trezor->init(), "Could not initialize the device " << m_trezor_path);
  CHECK_AND_ASSERT_THROW_MES(m_trezor->connect(), "Could not connect to the device " << m_trezor_path);
  m_trezor->wipe_device();
  m_trezor->load_device(m_device_seed);
  m_trezor->release();
  m_trezor->disconnect();
}

void gen_trezor_base::fork(gen_trezor_base & other)
{
  other.m_generator = m_generator;
  other.m_bt = m_bt;
  other.m_events = m_events;
  other.m_head = m_head;
  other.m_hard_forks = m_hard_forks;
  other.m_trezor_path = m_trezor_path;
  other.m_heavy_tests = m_heavy_tests;
  other.m_rct_config = m_rct_config;

  other.m_miner_account = m_miner_account;
  other.m_bob_account = m_bob_account;
  other.m_alice_account = m_alice_account;
  other.m_eve_account = m_eve_account;
  other.m_trezor = m_trezor;
}

void gen_trezor_base::clear()
{
  m_generator = test_generator();
  m_bt = block_tracker();
  m_events.clear();
  m_hard_forks.clear();
  m_trezor = nullptr;
}

void gen_trezor_base::add_shared_events(std::vector<test_event_entry>& events)
{
  events.reserve(m_events.size());
  for(const test_event_entry & c : m_events){
    events.push_back(c);
  }
}

void gen_trezor_base::init_fields()
{
  m_miner_account.generate();
  DEFAULT_HARDFORKS(m_hard_forks);

  crypto::secret_key master_seed{};
  CHECK_AND_ASSERT_THROW_MES(epee::string_tools::hex_to_pod(m_master_seed_str, master_seed), "Hexdecode fails");

  m_alice_account.generate(master_seed, true);
  m_alice_account.set_createtime(m_wallet_ts);
}

bool gen_trezor_base::generate(std::vector<test_event_entry>& events)
{
  init_fields();
  setup_trezor();
  m_alice_account.create_from_device(*m_trezor);
  m_alice_account.set_createtime(m_wallet_ts);

  // Events, custom genesis so it matches wallet genesis
  auto & generator = m_generator;  // macro shortcut

  cryptonote::block blk_gen;
  std::vector<size_t> block_weights;
  generate_genesis_block(blk_gen, get_config(MAINNET).GENESIS_TX, get_config(MAINNET).GENESIS_NONCE);
  events.push_back(blk_gen);
  generator.add_block(blk_gen, 0, block_weights, 0);

  // First event has to be the genesis block
  m_bob_account.generate();
  m_eve_account.generate();
  m_bob_account.set_createtime(m_wallet_ts);
  m_eve_account.set_createtime(m_wallet_ts);
  cryptonote::account_base * accounts[] = {TREZOR_ACCOUNT_ORDERING};
  for(cryptonote::account_base * ac : accounts){
    events.push_back(*ac);
  }

  // Another block with predefined timestamp.
  // Carefully set reward and already generated coins so it passes miner_tx check.
  cryptonote::block blk_0;
  {
    std::list<cryptonote::transaction> tx_list;
    const crypto::hash prev_id = get_block_hash(blk_gen);
    const uint64_t already_generated_coins = generator.get_already_generated_coins(prev_id);
    block_weights.clear();
    generator.get_last_n_block_weights(block_weights, prev_id, CRYPTONOTE_REWARD_BLOCKS_WINDOW);
    generator.construct_block(blk_0, 1, prev_id, m_miner_account, m_ts_start, already_generated_coins, block_weights, tx_list);
  }

  events.push_back(blk_0);
  MDEBUG("Gen+1 block has time: " << blk_0.timestamp << " blid: " << get_block_hash(blk_0));

  // Generate some spendable funds on the Miner account
  REWIND_BLOCKS_N(events, blk_3, blk_0, m_miner_account, 40);

  // Rewind so the miners funds are unlocked for initial transactions.
  REWIND_BLOCKS(events, blk_3r, blk_3, m_miner_account);

  // Non-rct transactions Miner -> Bob
  MAKE_TX_LIST_START(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(10), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(7), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(7), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(14), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(20), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(2), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(2), blk_3);
  MAKE_TX_LIST(events, txs_blk_4, m_miner_account, m_alice_account, MK_COINS(5), blk_3);
  MAKE_NEXT_BLOCK_TX_LIST(events, blk_4, blk_3r, m_miner_account, txs_blk_4);
  REWIND_BLOCKS(events, blk_4r, blk_4, m_miner_account);  // rewind to unlock

  // Hard fork to bulletproofs version, v9.
  const uint8_t CUR_HF = 9;
  auto hardfork_height = num_blocks(events);  // next block is v9
  ADD_HARDFORK(m_hard_forks, CUR_HF, hardfork_height);
  add_hforks(events, m_hard_forks);
  MDEBUG("Hardfork height: " << hardfork_height << " at block: " << get_block_hash(blk_4r));

  // RCT transactions, wallets have to be used, wallet init
  m_wl_alice.reset(new tools::wallet2(MAINNET, 1, true));
  m_wl_bob.reset(new tools::wallet2(MAINNET, 1, true));
  wallet_accessor_test::set_account(m_wl_alice.get(), m_alice_account);
  wallet_accessor_test::set_account(m_wl_bob.get(), m_bob_account);

  auto addr_alice_sub_0_1 = m_wl_alice->get_subaddress({0, 1});
  auto addr_alice_sub_0_2 = m_wl_alice->get_subaddress({0, 2});
  auto addr_alice_sub_0_3 = m_wl_alice->get_subaddress({0, 3});
  auto addr_alice_sub_0_4 = m_wl_alice->get_subaddress({0, 4});
  auto addr_alice_sub_0_5 = m_wl_alice->get_subaddress({0, 5});
  auto addr_alice_sub_1_0 = m_wl_alice->get_subaddress({1, 0});
  auto addr_alice_sub_1_1 = m_wl_alice->get_subaddress({1, 1});
  auto addr_alice_sub_1_2 = m_wl_alice->get_subaddress({1, 2});

  // Miner -> Bob, RCT funds
  MAKE_TX_LIST_START_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(5), 10, blk_4);

  const size_t target_rct = m_heavy_tests ? 105 : 15;
  for(size_t i = 0; i < target_rct; ++i)
  {
    MAKE_TX_MIX_LIST_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(1) >> 2, 10, blk_4);
  }

  // Sub-address destinations
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_0_1, true, MK_COINS(1) >> 1), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_0_2, true, MK_COINS(1) >> 1), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_0_3, true, MK_COINS(1) >> 1), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_0_4, true, MK_COINS(1) >> 1), 10, blk_4);

  // Sub-address destinations + multi out to force use of additional keys
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{addr_alice_sub_0_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_2, true, MK_COINS(1) >> 1}}), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{addr_alice_sub_0_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_2, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_3, true, MK_COINS(1) >> 1}}), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{m_miner_account, false, MK_COINS(1) >> 1}, {addr_alice_sub_0_2, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_3, true, MK_COINS(1) >> 1}}), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{m_miner_account, false, MK_COINS(1) >> 1}, {addr_alice_sub_0_2, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_3, true, MK_COINS(1) >> 1}}), 10, blk_4);

  // Transfer to other accounts
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_1_0, true, MK_COINS(1) >> 1), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts(addr_alice_sub_1_1, true, MK_COINS(1) >> 1), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{addr_alice_sub_1_0, true, MK_COINS(1) >> 1}, {addr_alice_sub_1_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_3, true, MK_COINS(1) >> 1}}), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{addr_alice_sub_1_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_1_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_2, true, MK_COINS(1) >> 1}}), 10, blk_4);
  MAKE_TX_MIX_DEST_LIST_RCT(events, txs_blk_5, m_miner_account, build_dsts({{addr_alice_sub_1_2, true, MK_COINS(1) >> 1}, {addr_alice_sub_1_1, true, MK_COINS(1) >> 1}, {addr_alice_sub_0_5, true, MK_COINS(1) >> 1}}), 10, blk_4);

  // Simple RCT transactions
  MAKE_TX_MIX_LIST_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(7), 10, blk_4);
  MAKE_TX_MIX_LIST_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(1), 10, blk_4);
  MAKE_TX_MIX_LIST_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(3), 10, blk_4);
  MAKE_TX_MIX_LIST_RCT(events, txs_blk_5, m_miner_account, m_alice_account, MK_COINS(4), 10, blk_4);
  MAKE_NEXT_BLOCK_TX_LIST_HF(events, blk_5, blk_4r, m_miner_account, txs_blk_5, CUR_HF);

  // Simple transaction check
  bool resx = rct::verRctSemanticsSimple(txs_blk_5.begin()->rct_signatures);
  bool resy = rct::verRctNonSemanticsSimple(txs_blk_5.begin()->rct_signatures);
  CHECK_AND_ASSERT_THROW_MES(resx, "Tsx5[0] semantics failed");
  CHECK_AND_ASSERT_THROW_MES(resy, "Tsx5[0] non-semantics failed");

  REWIND_BLOCKS_HF(events, blk_5r, blk_5, m_miner_account, CUR_HF);  // rewind to unlock

  // RCT transactions, wallets have to be used
  wallet_tools::process_transactions(m_wl_alice.get(), events, blk_5r, m_bt);
  wallet_tools::process_transactions(m_wl_bob.get(), events, blk_5r, m_bt);

  // Send Alice -> Bob, manually constructed. Simple TX test, precondition.
  cryptonote::transaction tx_1;
  std::vector<size_t> selected_transfers;
  std::vector<tx_source_entry> sources;
  bool res = wallet_tools::fill_tx_sources(m_wl_alice.get(), sources, TREZOR_TEST_MIXIN, boost::none, MK_COINS(2), m_bt, selected_transfers, num_blocks(events) - 1, 0, 1);
  CHECK_AND_ASSERT_THROW_MES(res, "TX Fill sources failed");

  construct_tx_to_key(tx_1, m_wl_alice.get(), m_bob_account, MK_COINS(1), sources, TREZOR_TEST_FEE, true, rct::RangeProofPaddedBulletproof, 1);
  events.push_back(tx_1);
  MAKE_NEXT_BLOCK_TX1_HF(events, blk_6, blk_5r, m_miner_account, tx_1, CUR_HF);
  MDEBUG("Post 1st tsx: " << (num_blocks(events) - 1) << " at block: " << get_block_hash(blk_6));

  // Simple transaction check
  resx = rct::verRctSemanticsSimple(tx_1.rct_signatures);
  resy = rct::verRctNonSemanticsSimple(tx_1.rct_signatures);
  CHECK_AND_ASSERT_THROW_MES(resx, "tx_1 semantics failed");
  CHECK_AND_ASSERT_THROW_MES(resy, "tx_1 non-semantics failed");

  REWIND_BLOCKS_HF(events, blk_6r, blk_6, m_miner_account, CUR_HF);
  m_head = blk_6r;
  m_events = events;
  return true;
}

void gen_trezor_base::load(std::vector<test_event_entry>& events)
{
  init_fields();
  m_events = events;

  unsigned acc_idx = 0;
  cryptonote::account_base * accounts[] = {TREZOR_ACCOUNT_ORDERING};
  unsigned accounts_num = (sizeof(accounts) / sizeof(accounts[0]));

  for(auto & ev : events)
  {
    if (typeid(cryptonote::block) == ev.type())
    {
      m_head = boost::get<cryptonote::block>(ev);
    }
    else if (typeid(cryptonote::account_base) == ev.type())  // accounts
    {
      const auto & acc = boost::get<cryptonote::account_base>(ev);
      if (acc_idx < accounts_num)
      {
        *accounts[acc_idx++] = acc;
      }
    }
    else if (typeid(event_replay_settings) == ev.type())  // hard forks
    {
      const auto & rep_settings = boost::get<event_replay_settings>(ev);
      if (rep_settings.hard_forks)
      {
        const auto & hf = rep_settings.hard_forks.get();
        std::copy(hf.begin(), hf.end(), std::back_inserter(m_hard_forks));
      }
    }
  }

  // Setup wallets, synchronize blocks
  m_bob_account.set_createtime(m_wallet_ts);
  m_eve_account.set_createtime(m_wallet_ts);

  setup_trezor();
  m_alice_account.create_from_device(*m_trezor);
  m_alice_account.set_createtime(m_wallet_ts);

  m_wl_alice.reset(new tools::wallet2(MAINNET, 1, true));
  m_wl_bob.reset(new tools::wallet2(MAINNET, 1, true));
  m_wl_eve.reset(new tools::wallet2(MAINNET, 1, true));
  wallet_accessor_test::set_account(m_wl_alice.get(), m_alice_account);
  wallet_accessor_test::set_account(m_wl_bob.get(), m_bob_account);
  wallet_accessor_test::set_account(m_wl_eve.get(), m_eve_account);

  wallet_tools::process_transactions(m_wl_alice.get(), events, m_head, m_bt);
  wallet_tools::process_transactions(m_wl_bob.get(), events, m_head, m_bt);
}

void gen_trezor_base::fix_hf(std::vector<test_event_entry>& events)
{
  // If current test requires higher hard-fork, move it up
  const auto current_hf = m_hard_forks.back().first;
  if (m_rct_config.bp_version == 2 && current_hf < 10){
    auto hardfork_height = num_blocks(events);
    ADD_HARDFORK(m_hard_forks, 10, hardfork_height);
    add_top_hfork(events, m_hard_forks);
    MDEBUG("Hardfork height: " << hardfork_height);
  }
}

void gen_trezor_base::update_trackers(std::vector<test_event_entry>& events)
{
  wallet_tools::process_transactions(nullptr, events, m_head, m_bt);
}

void gen_trezor_base::test_setup(std::vector<test_event_entry>& events)
{
  add_shared_events(events);

  setup_trezor();
  m_alice_account.create_from_device(*m_trezor);
  m_alice_account.set_createtime(m_wallet_ts);

  m_wl_alice.reset(new tools::wallet2(MAINNET, 1, true));
  m_wl_bob.reset(new tools::wallet2(MAINNET, 1, true));
  m_wl_eve.reset(new tools::wallet2(MAINNET, 1, true));
  wallet_accessor_test::set_account(m_wl_alice.get(), m_alice_account);
  wallet_accessor_test::set_account(m_wl_bob.get(), m_bob_account);
  wallet_accessor_test::set_account(m_wl_eve.get(), m_eve_account);
  wallet_tools::process_transactions(m_wl_alice.get(), events, m_head, m_bt);
  wallet_tools::process_transactions(m_wl_bob.get(), events, m_head, m_bt);
  wallet_tools::process_transactions(m_wl_eve.get(), events, m_head, m_bt);
}

void gen_trezor_base::test_trezor_tx(std::vector<test_event_entry>& events, std::vector<tools::wallet2::pending_tx>& ptxs, std::vector<cryptonote::address_parse_info>& dsts_info, test_generator &generator, std::vector<tools::wallet2*> wallets, bool is_sweep)
{
  // Construct pending transaction for signature in the Trezor.
  const uint64_t height_pre = num_blocks(events) - 1;
  cryptonote::block head_block = get_head_block(events);
  const crypto::hash head_hash = get_block_hash(head_block);

  // If current test requires higher hard-fork, move it up
  const auto current_hf = m_hard_forks.back().first;
  const uint8_t tx_hf = m_rct_config.bp_version == 2 ? 10 : 9;
  if (tx_hf > current_hf){
    throw std::runtime_error("Too late for HF change");
  }

  tools::wallet2::unsigned_tx_set txs;
  std::list<cryptonote::transaction> tx_list;

  for(auto &ptx : ptxs) {
    txs.txes.push_back(get_construction_data_with_decrypted_short_payment_id(ptx, *m_trezor));
  }
  txs.transfers = std::make_pair(0, wallet_accessor_test::get_transfers(m_wl_alice.get()));

  auto dev_cold = dynamic_cast<::hw::device_cold*>(m_trezor);
  CHECK_AND_ASSERT_THROW_MES(dev_cold, "Device does not implement cold signing interface");

  tools::wallet2::signed_tx_set exported_txs;
  hw::tx_aux_data aux_data;
  hw::wallet_shim wallet_shim;
  setup_shim(&wallet_shim);
  aux_data.tx_recipients = dsts_info;
  dev_cold->tx_sign(&wallet_shim, txs, exported_txs, aux_data);

  MDEBUG("Signed tx data from hw: " << exported_txs.ptx.size() << " transactions");
  CHECK_AND_ASSERT_THROW_MES(exported_txs.ptx.size() == ptxs.size(), "Invalid transaction sizes");

  for (size_t i = 0; i < exported_txs.ptx.size(); ++i){
    auto &c_ptx = exported_txs.ptx[i];
    c_ptx.tx.rct_signatures.mixRing = ptxs[i].tx.rct_signatures.mixRing;
    expand_tsx(c_ptx.tx);

    // Simple TX tests, more complex are performed in the core.
    MTRACE(cryptonote::obj_to_json_str(c_ptx.tx));
    bool resx = rct::verRctSemanticsSimple(c_ptx.tx.rct_signatures);
    bool resy = rct::verRctNonSemanticsSimple(c_ptx.tx.rct_signatures);
    CHECK_AND_ASSERT_THROW_MES(resx, "Trezor tx_1 semantics failed");
    CHECK_AND_ASSERT_THROW_MES(resy, "Trezor tx_1 Nonsemantics failed");

    events.push_back(c_ptx.tx);
    tx_list.push_back(c_ptx.tx);
    MDEBUG("Transaction: " << dump_data(c_ptx.tx));
  }

  MAKE_NEXT_BLOCK_TX_LIST_HF(events, blk_7, m_head, m_miner_account, tx_list, tx_hf);
  MDEBUG("Trezor tsx: " << (num_blocks(events) - 1) << " at block: " << get_block_hash(blk_7));

  // TX receive test
  uint64_t sum_in = 0;
  uint64_t sum_out = 0;

  for(size_t txid = 0; txid < exported_txs.ptx.size(); ++txid) {
    auto &c_ptx = exported_txs.ptx[txid];
    auto &c_tx = c_ptx.tx;
    const crypto::hash txhash = cryptonote::get_transaction_hash(c_tx);
    const size_t num_outs = c_tx.vout.size();
    size_t num_received = 0;
    uint64_t cur_sum_in = 0;
    uint64_t cur_sum_out = 0;
    uint64_t cur_sum_out_recv = 0;
    std::unordered_set<size_t> recv_out_idx;
    std::string exp_payment_id = get_payment_id(c_ptx.construction_data.extra);
    std::string enc_payment_id = get_payment_id(c_tx.extra);
    size_t num_payment_id_checks_done = 0;

    CHECK_AND_ASSERT_THROW_MES(exp_payment_id.empty() || exp_payment_id.size() == 8 || exp_payment_id.size() == 32, "Required payment ID invalid");
    CHECK_AND_ASSERT_THROW_MES((exp_payment_id.size() == 32) == (enc_payment_id.size() == 32), "Required and built payment ID size mismatch");
    CHECK_AND_ASSERT_THROW_MES(exp_payment_id.size() <= enc_payment_id.size(), "Required and built payment ID size mismatch");

    for(auto &src : c_ptx.construction_data.sources){
      cur_sum_in += src.amount;
    }

    for(auto &dst : c_ptx.construction_data.splitted_dsts){
      cur_sum_out += dst.amount;
    }

    CHECK_AND_ASSERT_THROW_MES(c_tx.rct_signatures.txnFee + cur_sum_out == cur_sum_in, "Tx Input Output amount mismatch");

    for (size_t widx = 0; widx < wallets.size(); ++widx) {
      const bool sender = widx == 0;
      tools::wallet2 *wl = wallets[widx];

      wallet_tools::process_transactions(wl, events, blk_7, m_bt, boost::make_optional(head_hash));

      tools::wallet2::transfer_container m_trans;
      tools::wallet2::transfer_container m_trans_txid;
      wl->get_transfers(m_trans);

      std::copy_if(m_trans.begin(), m_trans.end(), std::back_inserter(m_trans_txid), [&txhash](const tools::wallet2::transfer_details& item) {
        return item.m_txid == txhash;
      });

      // Testing if the transaction output has been received
      num_received += m_trans_txid.size();
      for (auto & ctran : m_trans_txid){
        cur_sum_out_recv += ctran.amount();
        recv_out_idx.insert(ctran.m_internal_output_index);
        CHECK_AND_ASSERT_THROW_MES(!ctran.m_spent, "Txout is spent");
        CHECK_AND_ASSERT_THROW_MES(!sender || ctran.m_key_image_known, "Key Image unknown for recipient");  // sender is Trezor, does not need to have KI
      }

      // Sender output payment (contains change and stuff)
      if (sender) {
        std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> confirmed_transfers;  // txid -> tdetail
        std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> confirmed_transfers_txid;  // txid -> tdetail
        wl->get_payments_out(confirmed_transfers, height_pre);

        std::copy_if(confirmed_transfers.begin(), confirmed_transfers.end(), std::back_inserter(confirmed_transfers_txid), [&txhash](const std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>& item) {
          return item.first == txhash;
        });

        CHECK_AND_ASSERT_THROW_MES(confirmed_transfers_txid.size() == 1, "Sender does not have outgoing transfer for the transaction");
      }

      // Received payment from the block
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments; // payment id -> [payment details] multimap
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments_txid; // payment id -> [payment details] multimap
      wl->get_payments(payments, height_pre);

      std::copy_if(payments.begin(), payments.end(), std::back_inserter(payments_txid), [&txhash](const std::pair<crypto::hash, tools::wallet2::payment_details>& item) {
        return item.second.m_tx_hash == txhash;
      });

      for(auto &paydet : payments_txid){
        CHECK_AND_ASSERT_THROW_MES(exp_payment_id.empty() || (memcmp(exp_payment_id.data(), paydet.first.data, exp_payment_id.size()) == 0), "Payment ID mismatch");
        num_payment_id_checks_done += 1;
      }
    }

    CHECK_AND_ASSERT_THROW_MES(c_tx.rct_signatures.txnFee + cur_sum_out_recv == cur_sum_in, "Tx Input Output amount mismatch");
    CHECK_AND_ASSERT_THROW_MES(exp_payment_id.empty() || num_payment_id_checks_done > 0, "No Payment ID checks");

    if(!is_sweep){
      CHECK_AND_ASSERT_THROW_MES(num_received == num_outs, "Number of received outputs do not match number of outgoing");
      CHECK_AND_ASSERT_THROW_MES(recv_out_idx.size() == num_outs, "Num of outs received do not match");
    } else {
      CHECK_AND_ASSERT_THROW_MES(num_received + 1 >= num_outs, "Number of received outputs do not match number of outgoing");
      CHECK_AND_ASSERT_THROW_MES(recv_out_idx.size() + 1 >= num_outs, "Num of outs received do not match");  // can have dummy out
    }

    sum_in += cur_sum_in;
    sum_out += cur_sum_out + c_tx.rct_signatures.txnFee;
  }

  CHECK_AND_ASSERT_THROW_MES(sum_in == sum_out, "Tx amount mismatch");
}

#define TREZOR_TEST_PREFIX()                              \
  test_generator generator(m_generator);                  \
  test_setup(events);                                     \
  tsx_builder t_builder_o(this);                          \
  tsx_builder * t_builder = &t_builder_o

#define TREZOR_TEST_SUFFIX()                              \
  auto _dsts = t_builder->build();                        \
  auto _dsts_info = t_builder->dest_info();               \
  test_trezor_tx(events, _dsts, _dsts_info, generator, vct_wallets(m_wl_alice.get(), m_wl_bob.get(), m_wl_eve.get())); \
  return true

#define TREZOR_SKIP_IF_VERSION_LEQ(x) if (m_trezor->get_version() <= x) { MDEBUG("Test skipped"); return true; }
#define TREZOR_TEST_PAYMENT_ID "\xde\xad\xc0\xde\xde\xad\xc0\xde"
#define TREZOR_TEST_PAYMENT_ID_LONG "\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde\xde\xad\xc0\xde"

tsx_builder * tsx_builder::sources(std::vector<cryptonote::tx_source_entry> & sources, std::vector<size_t> & selected_transfers)
{
    m_sources = sources;
    m_selected_transfers = selected_transfers;
    return this;
}

tsx_builder * tsx_builder::compute_sources(boost::optional<size_t> num_utxo, boost::optional<uint64_t> min_amount, ssize_t offset, int step, boost::optional<fnc_accept_tx_source_t> fnc_accept)
{
  CHECK_AND_ASSERT_THROW_MES(m_tester, "m_tester wallet empty");
  CHECK_AND_ASSERT_THROW_MES(m_from, "m_from wallet empty");

  // typedef std::function<bool(const tx_source_info_crate_t &info, bool &abort)> fnc_accept_tx_source_t;
  boost::optional<fnc_accept_tx_source_t> fnc_accept_to_use = boost::none;

  auto c_account = m_account;
  fnc_accept_tx_source_t fnc_acc = [c_account, &fnc_accept] (const tx_source_info_crate_t &info, bool &abort) -> bool {
    if (info.td->m_subaddr_index.major != c_account){
      return false;
    }
    if (fnc_accept){
      return (fnc_accept.get())(info, abort);
    }
    return true;
  };

  fnc_accept_to_use = fnc_acc;
  bool res = wallet_tools::fill_tx_sources(m_from, m_sources, m_mixin, num_utxo, min_amount, m_tester->m_bt, m_selected_transfers, m_cur_height, offset, step, fnc_accept_to_use);
  CHECK_AND_ASSERT_THROW_MES(res, "Tx source fill error");
  return this;
}

tsx_builder * tsx_builder::compute_sources_to_sub(boost::optional<size_t> num_utxo, boost::optional<uint64_t> min_amount, ssize_t offset, int step, boost::optional<fnc_accept_tx_source_t> fnc_accept)
{
  fnc_accept_tx_source_t fnc = [&fnc_accept] (const tx_source_info_crate_t &info, bool &abort) -> bool {
    if (info.td->m_subaddr_index.minor == 0){
      return false;
    }
    if (fnc_accept){
      return (fnc_accept.get())(info, abort);
    }
    return true;
  };

  return compute_sources(num_utxo, min_amount, offset, step, fnc);
}

tsx_builder * tsx_builder::compute_sources_to_sub_acc(boost::optional<size_t> num_utxo, boost::optional<uint64_t> min_amount, ssize_t offset, int step, boost::optional<fnc_accept_tx_source_t> fnc_accept)
{
  fnc_accept_tx_source_t fnc = [&fnc_accept] (const tx_source_info_crate_t &info, bool &abort) -> bool {
    if (info.td->m_subaddr_index.minor == 0 || info.src->real_out_additional_tx_keys.size() == 0){
      return false;
    }
    if (fnc_accept){
      return (fnc_accept.get())(info, abort);
    }
    return true;
  };

  return compute_sources(num_utxo, min_amount, offset, step, fnc);
}

tsx_builder * tsx_builder::destinations(std::vector<cryptonote::tx_destination_entry> &dsts)
{
  m_destinations_orig = dsts;
  return this;
}

tsx_builder * tsx_builder::add_destination(const cryptonote::tx_destination_entry &dst)
{
  m_destinations_orig.push_back(dst);
  return this;
}

tsx_builder * tsx_builder::add_destination(const var_addr_t addr, bool is_subaddr, uint64_t amount)
{
  m_destinations_orig.push_back(build_dst(addr, is_subaddr, amount));
  return this;
}

tsx_builder * tsx_builder::add_destination(const tools::wallet2 * wallet, bool is_subaddr, uint64_t amount)
{
  m_destinations_orig.push_back(build_dst(get_address(wallet), is_subaddr, amount));
  return this;
}

tsx_builder * tsx_builder::set_integrated(size_t idx)
{
  m_integrated.insert(idx);
  return this;
}

tsx_builder * tsx_builder::clear_current()
{
  m_account = 0;
  m_selected_transfers.clear();
  m_sources.clear();
  m_destinations.clear();
  m_destinations_orig.clear();
  m_dsts_info.clear();
  m_integrated.clear();
  m_payment_id.clear();
  return this;
}

tsx_builder * tsx_builder::build_tx()
{
  CHECK_AND_ASSERT_THROW_MES(m_tester, "m_tester wallet empty");
  CHECK_AND_ASSERT_THROW_MES(m_from, "m_from wallet empty");

  // Amount sanity check input >= fee + outputs
  const uint64_t out_amount = sum_amount(m_destinations_orig);
  const uint64_t in_amount = sum_amount(m_sources);
  CHECK_AND_ASSERT_THROW_MES(in_amount >= out_amount + m_fee, "Not enough input credits for outputs and fees");

  // Create new pending transaction, init with sources and destinations
  m_ptxs.emplace_back();
  auto & ptx = m_ptxs.back();

  std::vector<uint8_t> extra = build_payment_id_extra(m_payment_id);
  fill_tx_destinations(m_from->get_subaddress({m_account, 0}), m_destinations_orig, m_fee, m_sources, m_destinations, true);
  construct_pending_tx(ptx, extra);

  ptx.construction_data.subaddr_account = m_account;

  // Build destinations parse info
  for(size_t i = 0; i < m_destinations_orig.size(); ++i){
    auto & cdest = m_destinations_orig[i];
    cryptonote::address_parse_info info = init_addr_parse_info(cdest.addr, cdest.is_subaddress);
    if (m_integrated.find(i) != m_integrated.end()){
      CHECK_AND_ASSERT_THROW_MES(m_payment_id.size() == 8, "Integrated set but payment_id.size() != 8");
      info.has_payment_id = true;
      info.payment_id = to_short_payment_id(m_payment_id);
    }

    m_dsts_info.push_back(info);
  }

  return this;
}

tsx_builder * tsx_builder::construct_pending_tx(tools::wallet2::pending_tx &ptx, boost::optional<std::vector<uint8_t>> extra)
{
  CHECK_AND_ASSERT_THROW_MES(m_from, "Wallet not provided");

  cryptonote::transaction tx;
  subaddresses_t & subaddresses = wallet_accessor_test::get_subaddresses(m_from);
  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  std::vector<tx_destination_entry> destinations_copy = m_destinations;

  auto change_addr = m_from->get_account().get_keys().m_account_address;
  bool r = construct_tx_and_get_tx_key(m_from->get_account().get_keys(), subaddresses, m_sources, destinations_copy,
                                       change_addr, extra ? extra.get() : std::vector<uint8_t>(), tx, 0, tx_key,
                                       additional_tx_keys, true, m_rct_config, nullptr);

  CHECK_AND_ASSERT_THROW_MES(r, "Transaction construction failed");

  ptx.key_images = "";
  ptx.fee = TESTS_DEFAULT_FEE;
  ptx.dust = 0;
  ptx.dust_added_to_fee = false;
  ptx.tx = tx;
  ptx.change_dts = m_destinations.back();
  ptx.selected_transfers = m_selected_transfers;
  ptx.tx_key = tx_key;
  ptx.additional_tx_keys = additional_tx_keys;
  ptx.dests = m_destinations;
  ptx.multisig_sigs.clear();
  ptx.construction_data.sources = m_sources;
  ptx.construction_data.change_dts = m_destinations.back();
  ptx.construction_data.splitted_dsts = m_destinations;
  ptx.construction_data.selected_transfers = ptx.selected_transfers;
  ptx.construction_data.extra = tx.extra;
  ptx.construction_data.unlock_time = 0;
  ptx.construction_data.use_rct = true;
  ptx.construction_data.use_bulletproofs = true;
  ptx.construction_data.dests = m_destinations_orig;

  ptx.construction_data.subaddr_account = 0;
  ptx.construction_data.subaddr_indices.clear();
  for(uint32_t i = 0; i < 20; ++i)
    ptx.construction_data.subaddr_indices.insert(i);

  return this;
}

std::vector<tools::wallet2::pending_tx> tsx_builder::build()
{
  return m_ptxs;
}

bool gen_trezor_ki_sync::generate(std::vector<test_event_entry>& events)
{
  test_generator generator(m_generator);
  test_setup(events);

  auto dev_cold = dynamic_cast<::hw::device_cold*>(m_trezor);
  CHECK_AND_ASSERT_THROW_MES(dev_cold, "Device does not implement cold signing interface");

  std::vector<std::pair<crypto::key_image, crypto::signature>> ski;
  tools::wallet2::transfer_container transfers;
  hw::wallet_shim wallet_shim;
  setup_shim(&wallet_shim);
  m_wl_alice->get_transfers(transfers);

  dev_cold->ki_sync(&wallet_shim, transfers, ski);
  CHECK_AND_ASSERT_THROW_MES(ski.size() == transfers.size(), "Size mismatch");
  for(size_t i = 0; i < transfers.size(); ++i)
  {
    auto & td = transfers[i];
    auto & kip = ski[i];
    CHECK_AND_ASSERT_THROW_MES(!td.m_key_image_known || td.m_key_image == kip.first, "Key Image invalid: " << i);
  }

  uint64_t spent = 0, unspent = 0;
  m_wl_alice->import_key_images(ski, 0, spent, unspent, false);
  return true;
}

bool gen_trezor_1utxo::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
           ->mixin(TREZOR_TEST_MIXIN)
           ->fee(TREZOR_TEST_FEE)
           ->from(m_wl_alice.get(), 0)
           ->compute_sources(boost::none, MK_COINS(1), -1, -1)
           ->add_destination(m_eve_account, false, 1000)
           ->rct_config(m_rct_config)
           ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_1utxo_paymentid_short::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  TREZOR_SKIP_IF_VERSION_LEQ(hw::trezor::pack_version(2, 0, 9));
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(boost::none, MK_COINS(1), -1, -1)
      ->add_destination(m_eve_account, false, 1000)
      ->payment_id(TREZOR_TEST_PAYMENT_ID)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_1utxo_paymentid_short_integrated::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  TREZOR_SKIP_IF_VERSION_LEQ(hw::trezor::pack_version(2, 0, 9));
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(boost::none, MK_COINS(1), -1, -1)
      ->add_destination(m_eve_account, false, 1000)
      ->payment_id(TREZOR_TEST_PAYMENT_ID)
      ->set_integrated(0)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_1utxo_paymentid_long::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(boost::none, MK_COINS(1), -1, -1)
      ->add_destination(m_eve_account, false, 1000)
      ->payment_id(TREZOR_TEST_PAYMENT_ID_LONG)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_eve_account, false, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo_acc1::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 1)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({0, 1}), true, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo_to_sub::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({0, 1}), true, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo_to_2sub::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({0, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({1, 3}), true, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo_to_1norm_2sub::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({1, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({2, 1}), true, 1000)
      ->add_destination(m_wl_eve.get(), false, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_2utxo_sub_acc_to_1norm_2sub::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources_to_sub_acc(2, MK_COINS(1) >> 2, -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({1, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({2, 1}), true, 1000)
      ->add_destination(m_wl_eve.get(), false, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_4utxo_to_7outs::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(4, MK_COINS(1), -1, -1)
      ->add_destination(m_wl_eve->get_subaddress({1, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({2, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({0, 1}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({0, 2}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({0, 3}), true, 1000)
      ->add_destination(m_wl_eve->get_subaddress({0, 4}), true, 1000)
      ->add_destination(m_wl_eve.get(), false, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

bool gen_trezor_many_utxo::generate(std::vector<test_event_entry>& events)
{
  TREZOR_TEST_PREFIX();
  t_builder->cur_height(num_blocks(events) - 1)
      ->mixin(TREZOR_TEST_MIXIN)
      ->fee(TREZOR_TEST_FEE)
      ->from(m_wl_alice.get(), 0)
      ->compute_sources(110, MK_COINS(1), -1, -1)
      ->add_destination(m_eve_account, false, 1000)
      ->rct_config(m_rct_config)
      ->build_tx();

  TREZOR_TEST_SUFFIX();
}

