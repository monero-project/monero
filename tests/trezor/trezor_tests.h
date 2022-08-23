// Copyright (c) 2014-2022, The Monero Project
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

#pragma once

#include <device_trezor/device_trezor.hpp>
#include <wallet/api/wallet2_api.h>
#include "daemon.h"
#include "../core_tests/chaingen.h"
#include "../core_tests/wallet_tools.h"

#define TREZOR_TEST_FEE 90000000000
#define TREZOR_TEST_CLSAG_MIXIN 11
#define TREZOR_TEST_HF15_MIXIN 16
#define TREZOR_TEST_MIXIN TREZOR_TEST_CLSAG_MIXIN

/************************************************************************/
/*                                                                      */
/************************************************************************/
class tsx_builder;
class gen_trezor_base : public test_chain_unit_base
{
public:
  friend class tsx_builder;

  gen_trezor_base();
  gen_trezor_base(const gen_trezor_base &other);
  virtual ~gen_trezor_base() {};

  virtual void setup_args(const std::string & trezor_path, bool heavy_tests=false);
  virtual bool generate(std::vector<test_event_entry>& events);
  virtual void load(std::vector<test_event_entry>& events);       // load events, init test obj
  virtual void fix_hf(std::vector<test_event_entry>& events);
  virtual void update_trackers(std::vector<test_event_entry>& events);

  virtual void fork(gen_trezor_base & other);                             // fork generated chain to another test
  virtual void clear();                                                   // clears m_events, bt, generator, hforks
  virtual void add_shared_events(std::vector<test_event_entry>& events);  // m_events -> events
  virtual void test_setup(std::vector<test_event_entry>& events);         // init setup env, wallets

  virtual void add_transactions_to_events(
      std::vector<test_event_entry>& events,
      test_generator &generator,
      const std::vector<cryptonote::transaction> &txs);

  virtual void test_trezor_tx(
      std::vector<test_event_entry>& events,
      std::vector<tools::wallet2::pending_tx>& ptxs,
      std::vector<cryptonote::address_parse_info>& dsts_info,
      test_generator &generator,
      std::vector<tools::wallet2*> wallets,
      bool is_sweep=false);

  virtual void test_get_tx(
      std::vector<test_event_entry>& events,
      std::vector<tools::wallet2*> wallets,
      const std::vector<tools::wallet2::pending_tx> &ptxs,
      const std::vector<std::string> &aux_tx_info);

  virtual void mine_and_test(std::vector<test_event_entry>& events);

  virtual void rewind_blocks(std::vector<test_event_entry>& events, size_t rewind_n, uint8_t hf);

  virtual void set_hard_fork(uint8_t hf);

  crypto::hash head_hash() const { return get_block_hash(m_head); }
  cryptonote::block head_block() const { return m_head; }
  bool heavy_tests() const { return m_heavy_tests; }
  void rct_config(rct::RCTConfig rct_config) { m_rct_config = rct_config; }
  uint8_t cur_hf() const { return m_hard_forks.size() > 0 ? m_hard_forks.back().first : 0; }
  size_t num_mixin() const { return  m_top_hard_fork >= HF_VERSION_BULLETPROOF_PLUS ? TREZOR_TEST_HF15_MIXIN : TREZOR_TEST_CLSAG_MIXIN; }
  cryptonote::network_type nettype() const { return m_network_type; }
  std::shared_ptr<mock_daemon> daemon() const { return m_daemon; }
  void daemon(std::shared_ptr<mock_daemon> daemon){ m_daemon = std::move(daemon); }

  // Static configuration
  static const uint64_t m_ts_start;
  static const uint64_t m_wallet_ts;
  static const std::string  m_device_name;
  static const std::string  m_master_seed_str;
  static const std::string  m_device_seed;
  static const std::string  m_alice_spend_private;
  static const std::string  m_alice_view_private;

protected:
  virtual void setup_trezor();
  virtual void init_fields();
  virtual void update_client_settings();
  virtual bool verify_tx_key(const ::crypto::secret_key & tx_priv, const ::crypto::public_key & tx_pub, const subaddresses_t & subs);

  test_generator m_generator;
  block_tracker m_bt;
  cryptonote::network_type m_network_type;
  std::shared_ptr<mock_daemon> m_daemon;

  uint8_t m_top_hard_fork;
  v_hardforks_t m_hard_forks;
  cryptonote::block m_head;
  std::vector<test_event_entry> m_events;

  std::string m_trezor_path;
  bool m_heavy_tests;
  bool m_test_get_tx_key;
  rct::RCTConfig m_rct_config;
  bool m_live_refresh_enabled;

  cryptonote::account_base m_miner_account;
  cryptonote::account_base m_bob_account;
  cryptonote::account_base m_alice_account;
  cryptonote::account_base m_eve_account;
  hw::trezor::device_trezor * m_trezor;
  std::unique_ptr<tools::wallet2> m_wl_alice;
  std::unique_ptr<tools::wallet2> m_wl_bob;
  std::unique_ptr<tools::wallet2> m_wl_eve;

  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive & ar, const unsigned int /*version*/)
  {
    ar & m_generator;
    ar & m_network_type;
  }
};

class tsx_builder {
public:
  tsx_builder(): m_tester(nullptr), m_from(nullptr), m_account(0), m_mixin(TREZOR_TEST_MIXIN), m_fee(TREZOR_TEST_FEE),
  m_rct_config({rct::RangeProofPaddedBulletproof, 1 }){}

  tsx_builder(gen_trezor_base * tester): m_tester(tester), m_from(nullptr), m_account(0),
  m_mixin(TREZOR_TEST_MIXIN), m_fee(TREZOR_TEST_FEE),
  m_rct_config({rct::RangeProofPaddedBulletproof, 1 }){}

  tsx_builder * cur_height(uint64_t cur_height) { m_cur_height = cur_height; return this; }
  tsx_builder * mixin(size_t mixin=TREZOR_TEST_MIXIN) { m_mixin = mixin; return this; }
  tsx_builder * fee(uint64_t fee=TREZOR_TEST_FEE) { m_fee = fee; return this; }
  tsx_builder * payment_id(const std::string & payment_id) { m_payment_id = payment_id; return this; }
  tsx_builder * from(tools::wallet2 *from, uint32_t account=0) { m_from = from; m_account=account; return this; }
  tsx_builder * sources(std::vector<cryptonote::tx_source_entry> & sources, std::vector<size_t> & selected_transfers);
  tsx_builder * compute_sources(boost::optional<size_t> num_utxo=boost::none, boost::optional<uint64_t> min_amount=boost::none, ssize_t offset=-1, int step=1, boost::optional<fnc_accept_tx_source_t> fnc_accept=boost::none);
  tsx_builder * compute_sources_to_sub(boost::optional<size_t> num_utxo=boost::none, boost::optional<uint64_t> min_amount=boost::none, ssize_t offset=-1, int step=1, boost::optional<fnc_accept_tx_source_t> fnc_accept=boost::none);
  tsx_builder * compute_sources_to_sub_acc(boost::optional<size_t> num_utxo=boost::none, boost::optional<uint64_t> min_amount=boost::none, ssize_t offset=-1, int step=1, boost::optional<fnc_accept_tx_source_t> fnc_accept=boost::none);

  tsx_builder * destinations(std::vector<cryptonote::tx_destination_entry> &dsts);
  tsx_builder * add_destination(const cryptonote::tx_destination_entry &dst);
  tsx_builder * add_destination(const tools::wallet2 * wallet, bool is_subaddr=false, uint64_t amount=1000);
  tsx_builder * add_destination(const var_addr_t addr, bool is_subaddr=false, uint64_t amount=1000);
  tsx_builder * set_integrated(size_t idx);
  tsx_builder * rct_config(const rct::RCTConfig & rct_config) {m_rct_config = rct_config; return this; };

  tsx_builder * build_tx();
  tsx_builder * construct_pending_tx(tools::wallet2::pending_tx &ptx, boost::optional<std::vector<uint8_t>> extra = boost::none);
  tsx_builder * clear_current();
  std::vector<tools::wallet2::pending_tx> build();
  std::vector<cryptonote::address_parse_info> dest_info(){ return m_dsts_info; }

protected:
  gen_trezor_base * m_tester;
  uint64_t m_cur_height;
  std::vector<tools::wallet2::pending_tx> m_ptxs;         // all transactions

  // current transaction
  size_t m_mixin;
  uint64_t m_fee;
  tools::wallet2 * m_from;
  uint32_t m_account;
  cryptonote::transaction m_tx;
  std::vector<size_t> m_selected_transfers;
  std::vector<cryptonote::tx_source_entry> m_sources;
  std::vector<cryptonote::tx_destination_entry> m_destinations;
  std::vector<cryptonote::tx_destination_entry> m_destinations_orig;
  std::vector<cryptonote::address_parse_info> m_dsts_info;
  std::unordered_set<size_t> m_integrated;
  std::string m_payment_id;
  rct::RCTConfig m_rct_config;
};

// Trezor device ship to track actual method calls.
class device_trezor_test : public hw::trezor::device_trezor {
public:
  size_t m_tx_sign_ctr;
  size_t m_compute_key_image_ctr;

  device_trezor_test();

  void clear_test_counters();
  void setup_for_tests(const std::string & trezor_path, const std::string & seed, cryptonote::network_type network_type);

  bool compute_key_image(const ::cryptonote::account_keys &ack, const ::crypto::public_key &out_key,
                         const ::crypto::key_derivation &recv_derivation, size_t real_output_index,
                         const ::cryptonote::subaddress_index &received_index, ::cryptonote::keypair &in_ephemeral,
                         ::crypto::key_image &ki) override;

protected:
  void tx_sign(hw::wallet_shim *wallet, const ::tools::wallet2::unsigned_tx_set &unsigned_tx, size_t idx,
               hw::tx_aux_data &aux_data, std::shared_ptr<hw::trezor::protocol::tx::Signer> &signer) override;
};

// Tests
class gen_trezor_ki_sync : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_ki_sync_with_refresh : public gen_trezor_ki_sync
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_ki_sync_without_refresh : public gen_trezor_ki_sync
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_live_refresh : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_1utxo : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_1utxo_paymentid_short : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_1utxo_paymentid_short_integrated : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_acc1 : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_to_sub : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_to_2sub : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_to_1norm_2sub : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_2utxo_sub_acc_to_1norm_2sub : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_to_7outs : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_4utxo_to_15outs : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_many_utxo : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

class gen_trezor_many_utxo_many_txo : public gen_trezor_base
{
public:
  bool generate(std::vector<test_event_entry>& events) override;
};

// Wallet::API tests
class wallet_api_tests : public gen_trezor_base
{
public:
  virtual ~wallet_api_tests();
  void init();
  bool generate(std::vector<test_event_entry>& events) override;

protected:
  boost::filesystem::path m_wallet_dir;
};