// Copyright (c) 2014-2017, The Monero Project
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

#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <atomic>

#include "include_base_utils.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "net/http_client.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "crypto/chacha8.h"
#include "crypto/hash.h"
#include "ringct/rctTypes.h"
#include "ringct/rctOps.h"

#include "wallet_errors.h"
#include "common/password.h"
#include "node_rpc_proxy.h"

#include <iostream>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.wallet2"

class Serialization_portability_wallet_Test;

namespace tools
{
  class i_wallet2_callback
  {
  public:
    virtual void on_new_block(uint64_t height, const cryptonote::block& block) {}
    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount) {}
    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount) {}
    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx) {}
    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx) {}
    virtual ~i_wallet2_callback() {}
  };

  struct tx_dust_policy
  {
    uint64_t dust_threshold;
    bool add_to_fee;
    cryptonote::account_public_address addr_for_dust;

    tx_dust_policy(uint64_t a_dust_threshold = 0, bool an_add_to_fee = true, cryptonote::account_public_address an_addr_for_dust = cryptonote::account_public_address())
      : dust_threshold(a_dust_threshold)
      , add_to_fee(an_add_to_fee)
      , addr_for_dust(an_addr_for_dust)
    {
    }
  };

  class wallet2
  {
    friend class ::Serialization_portability_wallet_Test;
  public:
    static constexpr const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

    enum RefreshType {
      RefreshFull,
      RefreshOptimizeCoinbase,
      RefreshNoCoinbase,
      RefreshDefault = RefreshOptimizeCoinbase,
    };

  private:
    wallet2(const wallet2&) : m_run(true), m_callback(0), m_testnet(false), m_always_confirm_transfers(true), m_print_ring_members(false), m_store_tx_info(true), m_default_mixin(0), m_default_priority(0), m_refresh_type(RefreshOptimizeCoinbase), m_auto_refresh(true), m_refresh_from_block_height(0), m_confirm_missing_payment_id(true), m_ask_password(true), m_min_output_count(0), m_min_output_value(0), m_merge_destinations(false), m_confirm_backlog(true), m_is_initialized(false),m_node_rpc_proxy(m_http_client, m_daemon_rpc_mutex) {}

  public:
    static const char* tr(const char* str);

    static bool has_testnet_option(const boost::program_options::variables_map& vm);
    static void init_options(boost::program_options::options_description& desc_params);

    //! \return Password retrieved from prompt. Logs error on failure.
    static boost::optional<password_container> password_prompt(const bool new_password);

    //! Uses stdin and stdout. Returns a wallet2 if no errors.
    static std::unique_ptr<wallet2> make_from_json(const boost::program_options::variables_map& vm, const std::string& json_file);

    //! Uses stdin and stdout. Returns a wallet2 and password for `wallet_file` if no errors.
    static std::pair<std::unique_ptr<wallet2>, password_container>
      make_from_file(const boost::program_options::variables_map& vm, const std::string& wallet_file);

    //! Uses stdin and stdout. Returns a wallet2 and password for wallet with no file if no errors.
    static std::pair<std::unique_ptr<wallet2>, password_container> make_new(const boost::program_options::variables_map& vm);

    //! Just parses variables.
    static std::unique_ptr<wallet2> make_dummy(const boost::program_options::variables_map& vm);

    static bool verify_password(const std::string& keys_file_name, const std::string& password, bool watch_only);

    wallet2(bool testnet = false, bool restricted = false) : m_run(true), m_callback(0), m_testnet(testnet), m_always_confirm_transfers(true), m_print_ring_members(false), m_store_tx_info(true), m_default_mixin(0), m_default_priority(0), m_refresh_type(RefreshOptimizeCoinbase), m_auto_refresh(true), m_refresh_from_block_height(0), m_confirm_missing_payment_id(true), m_ask_password(true), m_min_output_count(0), m_min_output_value(0), m_merge_destinations(false), m_confirm_backlog(true), m_is_initialized(false), m_restricted(restricted), is_old_file_format(false), m_node_rpc_proxy(m_http_client, m_daemon_rpc_mutex) {}

    struct transfer_details
    {
      uint64_t m_block_height;
      cryptonote::transaction_prefix m_tx;
      crypto::hash m_txid;
      size_t m_internal_output_index;
      uint64_t m_global_output_index;
      bool m_spent;
      uint64_t m_spent_height;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(
      rct::key m_mask;
      uint64_t m_amount;
      bool m_rct;
      bool m_key_image_known;
      size_t m_pk_index;

      bool is_rct() const { return m_rct; }
      uint64_t amount() const { return m_amount; }
      const crypto::public_key &get_public_key() const { return boost::get<const cryptonote::txout_to_key>(m_tx.vout[m_internal_output_index].target).key; }

      BEGIN_SERIALIZE_OBJECT()
        FIELD(m_block_height)
        FIELD(m_tx)
        FIELD(m_txid)
        FIELD(m_internal_output_index)
        FIELD(m_global_output_index)
        FIELD(m_spent)
        FIELD(m_spent_height)
        FIELD(m_key_image)
        FIELD(m_mask)
        FIELD(m_amount)
        FIELD(m_rct)
        FIELD(m_key_image_known)
        FIELD(m_pk_index)
      END_SERIALIZE()
    };

    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
      uint64_t m_timestamp;
    };

    struct unconfirmed_transfer_details
    {
      cryptonote::transaction_prefix m_tx;
      uint64_t m_amount_in;
      uint64_t m_amount_out;
      uint64_t m_change;
      time_t m_sent_time;
      std::vector<cryptonote::tx_destination_entry> m_dests;
      crypto::hash m_payment_id;
      enum { pending, pending_not_in_pool, failed } m_state;
      uint64_t m_timestamp;
    };

    struct confirmed_transfer_details
    {
      uint64_t m_amount_in;
      uint64_t m_amount_out;
      uint64_t m_change;
      uint64_t m_block_height;
      std::vector<cryptonote::tx_destination_entry> m_dests;
      crypto::hash m_payment_id;
      uint64_t m_timestamp;
      uint64_t m_unlock_time;

      confirmed_transfer_details(): m_amount_in(0), m_amount_out(0), m_change((uint64_t)-1), m_block_height(0), m_payment_id(cryptonote::null_hash), m_timestamp(0), m_unlock_time(0) {}
      confirmed_transfer_details(const unconfirmed_transfer_details &utd, uint64_t height):
        m_amount_in(utd.m_amount_in), m_amount_out(utd.m_amount_out), m_change(utd.m_change), m_block_height(height), m_dests(utd.m_dests), m_payment_id(utd.m_payment_id), m_timestamp(utd.m_timestamp), m_unlock_time(utd.m_tx.unlock_time) {}
    };

    struct tx_construction_data
    {
      std::vector<cryptonote::tx_source_entry> sources;
      cryptonote::tx_destination_entry change_dts;
      std::vector<cryptonote::tx_destination_entry> splitted_dsts; // split, includes change
      std::list<size_t> selected_transfers;
      std::vector<uint8_t> extra;
      uint64_t unlock_time;
      bool use_rct;
      std::vector<cryptonote::tx_destination_entry> dests; // original setup, does not include change
    };

    typedef std::vector<transfer_details> transfer_container;
    typedef std::unordered_multimap<crypto::hash, payment_details> payment_container;

    // The convention for destinations is:
    // dests does not include change
    // splitted_dsts (in construction_data) does
    struct pending_tx
    {
      cryptonote::transaction tx;
      uint64_t dust, fee;
      bool dust_added_to_fee;
      cryptonote::tx_destination_entry change_dts;
      std::list<size_t> selected_transfers;
      std::string key_images;
      crypto::secret_key tx_key;
      std::vector<cryptonote::tx_destination_entry> dests;

      tx_construction_data construction_data;
    };

    // The term "Unsigned tx" is not really a tx since it's not signed yet.
    // It doesnt have tx hash, key and the integrated address is not separated into addr + payment id.
    struct unsigned_tx_set
    {
      std::vector<tx_construction_data> txes;
      wallet2::transfer_container transfers;
    };

    struct signed_tx_set
    {
      std::vector<pending_tx> ptx;
      std::vector<crypto::key_image> key_images;
    };

    struct keys_file_data
    {
      crypto::chacha8_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    struct cache_file_data
    {
      crypto::chacha8_iv iv;
      std::string cache_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(cache_data)
      END_SERIALIZE()
    };
    
    // GUI Address book
    struct address_book_row
    {
      cryptonote::account_public_address m_address;
      crypto::hash m_payment_id;
      std::string m_description;   
    };

    typedef std::tuple<uint64_t, crypto::public_key, rct::key> get_outs_entry;

    /*!
     * \brief Generates a wallet or restores one.
     * \param  wallet_        Name of wallet file
     * \param  password       Password of wallet file
     * \param  recovery_param If it is a restore, the recovery key
     * \param  recover        Whether it is a restore
     * \param  two_random     Whether it is a non-deterministic wallet
     * \return                The secret key of the generated wallet
     */
    crypto::secret_key generate(const std::string& wallet, const std::string& password,
      const crypto::secret_key& recovery_param = crypto::secret_key(), bool recover = false,
      bool two_random = false);
    /*!
     * \brief Creates a wallet from a public address and a spend/view secret key pair.
     * \param  wallet_        Name of wallet file
     * \param  password       Password of wallet file
     * \param  viewkey        view secret key
     * \param  spendkey       spend secret key
     */
    void generate(const std::string& wallet, const std::string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& spendkey, const crypto::secret_key& viewkey);
    /*!
     * \brief Creates a watch only wallet from a public address and a view secret key.
     * \param  wallet_        Name of wallet file
     * \param  password       Password of wallet file
     * \param  viewkey        view secret key
     */
    void generate(const std::string& wallet, const std::string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& viewkey = crypto::secret_key());
    /*!
     * \brief Rewrites to the wallet file for wallet upgrade (doesn't generate key, assumes it's already there)
     * \param wallet_name Name of wallet file (should exist)
     * \param password    Password for wallet file
     */
    void rewrite(const std::string& wallet_name, const std::string& password);
    void write_watch_only_wallet(const std::string& wallet_name, const std::string& password);
    void load(const std::string& wallet, const std::string& password);
    void store();
    /*!
     * \brief store_to - stores wallet to another file(s), deleting old ones
     * \param path     - path to the wallet file (keys and address filenames will be generated based on this filename)
     * \param password - password to protect new wallet (TODO: probably better save the password in the wallet object?)
     */
    void store_to(const std::string &path, const std::string &password);

    std::string path() const;

    /*!
     * \brief verifies given password is correct for default wallet keys file
     */
    bool verify_password(const std::string& password) const;
    cryptonote::account_base& get_account(){return m_account;}
    const cryptonote::account_base& get_account()const{return m_account;}

    void set_refresh_from_block_height(uint64_t height) {m_refresh_from_block_height = height;}
    uint64_t get_refresh_from_block_height() const {return m_refresh_from_block_height;}

    // upper_transaction_size_limit as defined below is set to 
    // approximately 125% of the fixed minimum allowable penalty
    // free block size. TODO: fix this so that it actually takes
    // into account the current median block size rather than
    // the minimum block size.
    bool deinit();
    bool init(std::string daemon_address = "http://localhost:8080",
      boost::optional<epee::net_utils::http::login> daemon_login = boost::none, uint64_t upper_transaction_size_limit = 0);

    void stop() { m_run.store(false, std::memory_order_relaxed); }

    i_wallet2_callback* callback() const { return m_callback; }
    void callback(i_wallet2_callback* callback) { m_callback = callback; }

    /*!
     * \brief Checks if deterministic wallet
     */
    bool is_deterministic() const;
    bool get_seed(std::string& electrum_words) const;
    /*!
     * \brief Gets the seed language
     */
    const std::string &get_seed_language() const;
    /*!
     * \brief Sets the seed language
     */
    void set_seed_language(const std::string &language);
    /*!
     * \brief Tells if the wallet file is deprecated.
     */
    bool is_deprecated() const;
    void refresh();
    void refresh(uint64_t start_height, uint64_t & blocks_fetched);
    void refresh(uint64_t start_height, uint64_t & blocks_fetched, bool& received_money);
    bool refresh(uint64_t & blocks_fetched, bool& received_money, bool& ok);

    void set_refresh_type(RefreshType refresh_type) { m_refresh_type = refresh_type; }
    RefreshType get_refresh_type() const { return m_refresh_type; }

    bool testnet() const { return m_testnet; }
    bool restricted() const { return m_restricted; }
    bool watch_only() const { return m_watch_only; }

    uint64_t balance() const;
    uint64_t unlocked_balance() const;
    uint64_t unlocked_dust_balance(const tx_dust_policy &dust_policy) const;
    template<typename T>
    void transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outputs_count, const std::vector<size_t> &unused_transfers_indices, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, bool trusted_daemon);
    template<typename T>
    void transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outputs_count, const std::vector<size_t> &unused_transfers_indices, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx& ptx, bool trusted_daemon);
    void transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outputs_count, const std::vector<size_t> &unused_transfers_indices, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, bool trusted_daemon);
    void transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outputs_count, const std::vector<size_t> &unused_transfers_indices, uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx& ptx, bool trusted_daemon);
    template<typename T>
    void transfer_selected(const std::vector<cryptonote::tx_destination_entry>& dsts, const std::list<size_t> selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx);
    void transfer_selected_rct(std::vector<cryptonote::tx_destination_entry> dsts, const std::list<size_t> selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx &ptx);

    void commit_tx(pending_tx& ptx_vector);
    void commit_tx(std::vector<pending_tx>& ptx_vector);
    bool save_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename);
    // load unsigned tx from file and sign it. Takes confirmation callback as argument. Used by the cli wallet
    bool sign_tx(const std::string &unsigned_filename, const std::string &signed_filename, std::vector<wallet2::pending_tx> &ptx, std::function<bool(const unsigned_tx_set&)> accept_func = NULL);
    // sign unsigned tx. Takes unsigned_tx_set as argument. Used by GUI
    bool sign_tx(unsigned_tx_set &exported_txs, const std::string &signed_filename, std::vector<wallet2::pending_tx> &ptx);
    // load unsigned_tx_set from file. 
    bool load_unsigned_tx(const std::string &unsigned_filename, unsigned_tx_set &exported_txs);
    bool load_tx(const std::string &signed_filename, std::vector<tools::wallet2::pending_tx> &ptx, std::function<bool(const signed_tx_set&)> accept_func = NULL);
    std::vector<pending_tx> create_transactions(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon);
    std::vector<wallet2::pending_tx> create_transactions_2(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon);
    std::vector<wallet2::pending_tx> create_transactions_all(uint64_t below, const cryptonote::account_public_address &address, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon);
    std::vector<wallet2::pending_tx> create_transactions_from(const cryptonote::account_public_address &address, std::vector<size_t> unused_transfers_indices, std::vector<size_t> unused_dust_indices, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t> extra, bool trusted_daemon);
    std::vector<pending_tx> create_unmixable_sweep_transactions(bool trusted_daemon);
    bool check_connection(uint32_t *version = NULL, uint32_t timeout = 200000);
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    void get_payments(const crypto::hash& payment_id, std::list<wallet2::payment_details>& payments, uint64_t min_height = 0) const;
    void get_payments(std::list<std::pair<crypto::hash,wallet2::payment_details>>& payments, uint64_t min_height, uint64_t max_height = (uint64_t)-1) const;
    void get_payments_out(std::list<std::pair<crypto::hash,wallet2::confirmed_transfer_details>>& confirmed_payments,
      uint64_t min_height, uint64_t max_height = (uint64_t)-1) const;
    void get_unconfirmed_payments_out(std::list<std::pair<crypto::hash,wallet2::unconfirmed_transfer_details>>& unconfirmed_payments) const;
    void get_unconfirmed_payments(std::list<std::pair<crypto::hash,wallet2::payment_details>>& unconfirmed_payments) const;

    uint64_t get_blockchain_current_height() const { return m_local_bc_height; }
    void rescan_spent();
    void rescan_blockchain(bool refresh = true);
    bool is_transfer_unlocked(const transfer_details& td) const;
    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      uint64_t dummy_refresh_height = 0; // moved to keys file
      if(ver < 5)
        return;
      a & m_blockchain;
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images;
      if(ver < 6)
        return;
      a & m_unconfirmed_txs;
      if(ver < 7)
        return;
      a & m_payments;
      if(ver < 8)
        return;
      a & m_tx_keys;
      if(ver < 9)
        return;
      a & m_confirmed_txs;
      if(ver < 11)
        return;
      a & dummy_refresh_height;
      if(ver < 12)
        return;
      a & m_tx_notes;
      if(ver < 13)
        return;
      if (ver < 17)
      {
        // we're loading an old version, where m_unconfirmed_payments was a std::map
        std::unordered_map<crypto::hash, payment_details> m;
        a & m;
        for (std::unordered_map<crypto::hash, payment_details>::const_iterator i = m.begin(); i != m.end(); ++i)
          m_unconfirmed_payments.insert(*i);
      }
      if(ver < 14)
        return;
      if(ver < 15)
      {
        // we're loading an older wallet without a pubkey map, rebuild it
        for (size_t i = 0; i < m_transfers.size(); ++i)
        {
          const transfer_details &td = m_transfers[i];
          const cryptonote::tx_out &out = td.m_tx.vout[td.m_internal_output_index];
          const cryptonote::txout_to_key &o = boost::get<const cryptonote::txout_to_key>(out.target);
          m_pub_keys.emplace(o.key, i);
        }
        return;
      }
      a & m_pub_keys;
      if(ver < 16)
        return;
      a & m_address_book;
      if(ver < 17)
        return;
      a & m_unconfirmed_payments;
      if(ver < 18)
        return;
      a & m_scanned_pool_txs[0];
      a & m_scanned_pool_txs[1];
    }

    /*!
     * \brief  Check if wallet keys and bin files exist
     * \param  file_path           Wallet file path
     * \param  keys_file_exists    Whether keys file exists
     * \param  wallet_file_exists  Whether bin file exists
     */
    static void wallet_exists(const std::string& file_path, bool& keys_file_exists, bool& wallet_file_exists);
    /*!
     * \brief  Check if wallet file path is valid format
     * \param  file_path      Wallet file path
     * \return                Whether path is valid format
     */
    static bool wallet_valid_path_format(const std::string& file_path);
    static bool parse_long_payment_id(const std::string& payment_id_str, crypto::hash& payment_id);
    static bool parse_short_payment_id(const std::string& payment_id_str, crypto::hash8& payment_id);
    static bool parse_payment_id(const std::string& payment_id_str, crypto::hash& payment_id);

    bool always_confirm_transfers() const { return m_always_confirm_transfers; }
    void always_confirm_transfers(bool always) { m_always_confirm_transfers = always; }
    bool print_ring_members() const { return m_print_ring_members; }
    void print_ring_members(bool value) { m_print_ring_members = value; }
    bool store_tx_info() const { return m_store_tx_info; }
    void store_tx_info(bool store) { m_store_tx_info = store; }
    uint32_t default_mixin() const { return m_default_mixin; }
    void default_mixin(uint32_t m) { m_default_mixin = m; }
    uint32_t get_default_priority() const { return m_default_priority; }
    void set_default_priority(uint32_t p) { m_default_priority = p; }
    bool auto_refresh() const { return m_auto_refresh; }
    void auto_refresh(bool r) { m_auto_refresh = r; }
    bool confirm_missing_payment_id() const { return m_confirm_missing_payment_id; }
    void confirm_missing_payment_id(bool always) { m_confirm_missing_payment_id = always; }
    bool ask_password() const { return m_ask_password; }
    void ask_password(bool always) { m_ask_password = always; }
    void set_default_decimal_point(unsigned int decimal_point);
    unsigned int get_default_decimal_point() const;
    void set_min_output_count(uint32_t count) { m_min_output_count = count; }
    uint32_t get_min_output_count() const { return m_min_output_count; }
    void set_min_output_value(uint64_t value) { m_min_output_value = value; }
    uint64_t get_min_output_value() const { return m_min_output_value; }
    void merge_destinations(bool merge) { m_merge_destinations = merge; }
    bool merge_destinations() const { return m_merge_destinations; }
    bool confirm_backlog() const { return m_confirm_backlog; }
    void confirm_backlog(bool always) { m_confirm_backlog = always; }

    bool get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key) const;

   /*!
    * \brief GUI Address book get/store
    */
    std::vector<address_book_row> get_address_book() const { return m_address_book; }
    bool add_address_book_row(const cryptonote::account_public_address &address, const crypto::hash &payment_id, const std::string &description);
    bool delete_address_book_row(std::size_t row_id);
        
    uint64_t get_num_rct_outputs();
    const transfer_details &get_transfer_details(size_t idx) const;

    void get_hard_fork_info(uint8_t version, uint64_t &earliest_height);
    bool use_fork_rules(uint8_t version, int64_t early_blocks = 0);
    int get_fee_algorithm();

    std::string get_wallet_file() const;
    std::string get_keys_file() const;
    std::string get_daemon_address() const;
    const boost::optional<epee::net_utils::http::login>& get_daemon_login() const { return m_daemon_login; }
    uint64_t get_daemon_blockchain_height(std::string& err);
    uint64_t get_daemon_blockchain_target_height(std::string& err);
   /*!
    * \brief Calculates the approximate blockchain height from current date/time.
    */
    uint64_t get_approximate_blockchain_height() const;
    std::vector<size_t> select_available_outputs_from_histogram(uint64_t count, bool atleast, bool unlocked, bool allow_rct, bool trusted_daemon);
    std::vector<size_t> select_available_outputs(const std::function<bool(const transfer_details &td)> &f);
    std::vector<size_t> select_available_unmixable_outputs(bool trusted_daemon);
    std::vector<size_t> select_available_mixable_outputs(bool trusted_daemon);

    size_t pop_best_value_from(const transfer_container &transfers, std::vector<size_t> &unused_dust_indices, const std::list<size_t>& selected_transfers, bool smallest = false) const;
    size_t pop_best_value(std::vector<size_t> &unused_dust_indices, const std::list<size_t>& selected_transfers, bool smallest = false) const;

    void set_tx_note(const crypto::hash &txid, const std::string &note);
    std::string get_tx_note(const crypto::hash &txid) const;

    std::string sign(const std::string &data) const;
    bool verify(const std::string &data, const cryptonote::account_public_address &address, const std::string &signature) const;

    std::vector<tools::wallet2::transfer_details> export_outputs() const;
    size_t import_outputs(const std::vector<tools::wallet2::transfer_details> &outputs);

    bool export_key_images(const std::string filename);
    std::vector<std::pair<crypto::key_image, crypto::signature>> export_key_images() const;
    uint64_t import_key_images(const std::vector<std::pair<crypto::key_image, crypto::signature>> &signed_key_images, uint64_t &spent, uint64_t &unspent);
    uint64_t import_key_images(const std::string &filename, uint64_t &spent, uint64_t &unspent);

    void update_pool_state(bool refreshed = false);

    std::string encrypt(const std::string &plaintext, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string encrypt_with_view_secret_key(const std::string &plaintext, bool authenticated = true) const;
    std::string decrypt(const std::string &ciphertext, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string decrypt_with_view_secret_key(const std::string &ciphertext, bool authenticated = true) const;

    std::string make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error);
    bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error);

    uint64_t get_blockchain_height_by_date(uint16_t year, uint8_t month, uint8_t day);    // 1<=month<=12, 1<=day<=31

    bool is_synced() const;

    std::vector<std::pair<uint64_t, uint64_t>> estimate_backlog(uint64_t min_blob_size, uint64_t max_blob_size, const std::vector<uint64_t> &fees);

    uint64_t get_fee_multiplier(uint32_t priority, int fee_algorithm = -1);
    uint64_t get_per_kb_fee();

  private:
    /*!
     * \brief  Stores wallet information to wallet file.
     * \param  keys_file_name Name of wallet file
     * \param  password       Password of wallet file
     * \param  watch_only     true to save only view key, false to save both spend and view keys
     * \return                Whether it was successful.
     */
    bool store_keys(const std::string& keys_file_name, const std::string& password, bool watch_only = false);
    /*!
     * \brief Load wallet information from wallet file.
     * \param keys_file_name Name of wallet file
     * \param password       Password of wallet file
     */
    bool load_keys(const std::string& keys_file_name, const std::string& password);
    void process_new_transaction(const crypto::hash &txid, const cryptonote::transaction& tx, const std::vector<uint64_t> &o_indices, uint64_t height, uint64_t ts, bool miner_tx, bool pool);
    void process_new_blockchain_entry(const cryptonote::block& b, const cryptonote::block_complete_entry& bche, const crypto::hash& bl_id, uint64_t height, const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices &o_indices);
    void detach_blockchain(uint64_t height);
    void get_short_chain_history(std::list<crypto::hash>& ids) const;
    bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height) const;
    bool clear();
    void pull_blocks(uint64_t start_height, uint64_t& blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::list<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices);
    void pull_hashes(uint64_t start_height, uint64_t& blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::list<crypto::hash> &hashes);
    void fast_refresh(uint64_t stop_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history);
    void pull_next_blocks(uint64_t start_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history, const std::list<cryptonote::block_complete_entry> &prev_blocks, std::list<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices, bool &error);
    void process_blocks(uint64_t start_height, const std::list<cryptonote::block_complete_entry> &blocks, const std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices, uint64_t& blocks_added);
    uint64_t select_transfers(uint64_t needed_money, std::vector<size_t> unused_transfers_indices, std::list<size_t>& selected_transfers, bool trusted_daemon);
    bool prepare_file_names(const std::string& file_path);
    void process_unconfirmed(const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t height);
    void process_outgoing(const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t height, uint64_t ts, uint64_t spent, uint64_t received);
    void add_unconfirmed_tx(const cryptonote::transaction& tx, uint64_t amount_in, const std::vector<cryptonote::tx_destination_entry> &dests, const crypto::hash &payment_id, uint64_t change_amount);
    void generate_genesis(cryptonote::block& b);
    void check_genesis(const crypto::hash& genesis_hash) const; //throws
    bool generate_chacha8_key_from_secret_keys(crypto::chacha8_key &key) const;
    crypto::hash get_payment_id(const pending_tx &ptx) const;
    crypto::hash8 get_short_payment_id(const pending_tx &ptx) const;
    void check_acc_out_precomp(const crypto::public_key &spend_public_key, const cryptonote::tx_out &o, const crypto::key_derivation &derivation, size_t i, bool &received, uint64_t &money_transfered, bool &error) const;
    void parse_block_round(const cryptonote::blobdata &blob, cryptonote::block &bl, crypto::hash &bl_id, bool &error) const;
    uint64_t get_upper_transaction_size_limit();
    std::vector<uint64_t> get_unspent_amounts_vector();
    uint64_t get_dynamic_per_kb_fee_estimate();
    float get_output_relatedness(const transfer_details &td0, const transfer_details &td1) const;
    std::vector<size_t> pick_preferred_rct_inputs(uint64_t needed_money) const;
    void set_spent(size_t idx, uint64_t height);
    void set_unspent(size_t idx);
    void get_outs(std::vector<std::vector<get_outs_entry>> &outs, const std::list<size_t> &selected_transfers, size_t fake_outputs_count);
    bool wallet_generate_key_image_helper(const cryptonote::account_keys& ack, const crypto::public_key& tx_public_key, size_t real_output_index, cryptonote::keypair& in_ephemeral, crypto::key_image& ki);
    crypto::public_key get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td) const;
    bool should_pick_a_second_output(bool use_rct, size_t n_transfers, const std::vector<size_t> &unused_transfers_indices, const std::vector<size_t> &unused_dust_indices) const;
    std::vector<size_t> get_only_rct(const std::vector<size_t> &unused_dust_indices, const std::vector<size_t> &unused_transfers_indices) const;

    cryptonote::account_base m_account;
    boost::optional<epee::net_utils::http::login> m_daemon_login;
    std::string m_daemon_address;
    std::string m_wallet_file;
    std::string m_keys_file;
    epee::net_utils::http::http_simple_client m_http_client;
    std::vector<crypto::hash> m_blockchain;
    std::atomic<uint64_t> m_local_bc_height; //temporary workaround
    std::unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;
    std::unordered_map<crypto::hash, confirmed_transfer_details> m_confirmed_txs;
    std::unordered_multimap<crypto::hash, payment_details> m_unconfirmed_payments;
    std::unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;

    transfer_container m_transfers;
    payment_container m_payments;
    std::unordered_map<crypto::key_image, size_t> m_key_images;
    std::unordered_map<crypto::public_key, size_t> m_pub_keys;
    cryptonote::account_public_address m_account_public_address;
    std::unordered_map<crypto::hash, std::string> m_tx_notes;
    std::vector<tools::wallet2::address_book_row> m_address_book;
    uint64_t m_upper_transaction_size_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value

    std::atomic<bool> m_run;

    boost::mutex m_daemon_rpc_mutex;

    i_wallet2_callback* m_callback;
    bool m_testnet;
    bool m_restricted;
    std::string seed_language; /*!< Language of the mnemonics (seed). */
    bool is_old_file_format; /*!< Whether the wallet file is of an old file format */
    bool m_watch_only; /*!< no spend key */
    bool m_always_confirm_transfers;
    bool m_print_ring_members;
    bool m_store_tx_info; /*!< request txkey to be returned in RPC, and store in the wallet cache file */
    uint32_t m_default_mixin;
    uint32_t m_default_priority;
    RefreshType m_refresh_type;
    bool m_auto_refresh;
    uint64_t m_refresh_from_block_height;
    bool m_confirm_missing_payment_id;
    bool m_ask_password;
    uint32_t m_min_output_count;
    uint64_t m_min_output_value;
    bool m_merge_destinations;
    bool m_confirm_backlog;
    bool m_is_initialized;
    NodeRPCProxy m_node_rpc_proxy;
    std::unordered_set<crypto::hash> m_scanned_pool_txs[2];
  };
}
BOOST_CLASS_VERSION(tools::wallet2, 18)
BOOST_CLASS_VERSION(tools::wallet2::transfer_details, 7)
BOOST_CLASS_VERSION(tools::wallet2::payment_details, 1)
BOOST_CLASS_VERSION(tools::wallet2::unconfirmed_transfer_details, 6)
BOOST_CLASS_VERSION(tools::wallet2::confirmed_transfer_details, 4)
BOOST_CLASS_VERSION(tools::wallet2::address_book_row, 16)    
BOOST_CLASS_VERSION(tools::wallet2::unsigned_tx_set, 0)
BOOST_CLASS_VERSION(tools::wallet2::signed_tx_set, 0)
BOOST_CLASS_VERSION(tools::wallet2::tx_construction_data, 0)
BOOST_CLASS_VERSION(tools::wallet2::pending_tx, 0)

namespace boost
{
  namespace serialization
  {
    template <class Archive>
    inline typename std::enable_if<!Archive::is_loading::value, void>::type initialize_transfer_details(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
    }
    template <class Archive>
    inline typename std::enable_if<Archive::is_loading::value, void>::type initialize_transfer_details(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
        if (ver < 1)
        {
          x.m_mask = rct::identity();
          x.m_amount = x.m_tx.vout[x.m_internal_output_index].amount;
        }
        if (ver < 2)
        {
          x.m_spent_height = 0;
        }
        if (ver < 4)
        {
          x.m_rct = x.m_tx.vout[x.m_internal_output_index].amount == 0;
        }
        if (ver < 6)
        {
          x.m_key_image_known = true;
        }
        if (ver < 7)
        {
          x.m_pk_index = 0;
        }
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_global_output_index;
      a & x.m_internal_output_index;
      if (ver < 3)
      {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
        x.m_txid = cryptonote::get_transaction_hash(tx);
      }
      else
      {
        a & x.m_tx;
      }
      a & x.m_spent;
      a & x.m_key_image;
      if (ver < 1)
      {
        // ensure mask and amount are set
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_mask;
      a & x.m_amount;
      if (ver < 2)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_spent_height;
      if (ver < 3)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_txid;
      if (ver < 4)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_rct;
      if (ver < 5)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      if (ver < 6)
      {
        // v5 did not properly initialize
        uint8_t u;
        a & u;
        x.m_key_image_known = true;
        return;
      }
      a & x.m_key_image_known;
      if (ver < 7)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_pk_index;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unconfirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_change;
      a & x.m_sent_time;
      if (ver < 5)
      {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
      }
      else
      {
        a & x.m_tx;
      }
      if (ver < 1)
        return;
      a & x.m_dests;
      a & x.m_payment_id;
      if (ver < 2)
        return;
      a & x.m_state;
      if (ver < 3)
        return;
      a & x.m_timestamp;
      if (ver < 4)
        return;
      a & x.m_amount_in;
      a & x.m_amount_out;
      if (ver < 6)
      {
        // v<6 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs.
        // We convert it to include change from v6
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)-1)
          x.m_amount_out += x.m_change;
      }
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::confirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_amount_in;
      a & x.m_amount_out;
      a & x.m_change;
      a & x.m_block_height;
      if (ver < 1)
        return;
      a & x.m_dests;
      a & x.m_payment_id;
      if (ver < 2)
        return;
      a & x.m_timestamp;
      if (ver < 3)
      {
        // v<3 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs. Whether it got added
        // or not depends on whether it came from a unconfirmed_transfer_details
        // (not included) or not (included). We can't reliably tell here, so we
        // check whether either yields a "negative" fee, or use the other if so.
        // We convert it to include change from v3
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)-1)
        {
          if (x.m_amount_in > (x.m_amount_out + x.m_change))
            x.m_amount_out += x.m_change;
        }
      }
      if (ver < 4)
      {
        if (!typename Archive::is_saving())
          x.m_unlock_time = 0;
        return;
      }
      a & x.m_unlock_time;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_tx_hash;
      a & x.m_amount;
      a & x.m_block_height;
      a & x.m_unlock_time;
      if (ver < 1)
        return;
      a & x.m_timestamp;
    }

    template <class Archive>
    inline void serialize(Archive& a, cryptonote::tx_destination_entry& x, const boost::serialization::version_type ver)
    {
      a & x.amount;
      a & x.addr;
    }
    
    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::address_book_row& x, const boost::serialization::version_type ver)
    {
      a & x.m_address;
      a & x.m_payment_id;
      a & x.m_description;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unsigned_tx_set &x, const boost::serialization::version_type ver)
    {
      a & x.txes;
      a & x.transfers;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::signed_tx_set &x, const boost::serialization::version_type ver)
    {
      a & x.ptx;
      a & x.key_images;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::tx_construction_data &x, const boost::serialization::version_type ver)
    {
      a & x.sources;
      a & x.change_dts;
      a & x.splitted_dsts;
      a & x.selected_transfers;
      a & x.extra;
      a & x.unlock_time;
      a & x.use_rct;
      a & x.dests;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::pending_tx &x, const boost::serialization::version_type ver)
    {
      a & x.tx;
      a & x.dust;
      a & x.fee;
      a & x.dust_added_to_fee;
      a & x.change_dts;
      a & x.selected_transfers;
      a & x.key_images;
      a & x.tx_key;
      a & x.dests;
      a & x.construction_data;
    }
  }
}

namespace tools
{

  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, std::vector<cryptonote::tx_destination_entry> &dust_dsts)
    {
      splitted_dsts.clear();
      dust_dsts.clear();

      for(auto& de: dsts)
      {
        cryptonote::decompose_amount_into_digits(de.amount, 0,
          [&](uint64_t chunk) { splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, de.addr)); },
          [&](uint64_t a_dust) { splitted_dsts.push_back(cryptonote::tx_destination_entry(a_dust, de.addr)); } );
      }

      cryptonote::decompose_amount_into_digits(change_dst.amount, 0,
        [&](uint64_t chunk) {
          if (chunk <= dust_threshold)
            dust_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_dst.addr));
          else
            splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_dst.addr));
        },
        [&](uint64_t a_dust) { dust_dsts.push_back(cryptonote::tx_destination_entry(a_dust, change_dst.addr)); } );
    }
    //----------------------------------------------------------------------------------------------------
    inline void null_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, std::vector<cryptonote::tx_destination_entry> &dust_dsts)
    {
      splitted_dsts = dsts;

      dust_dsts.clear();
      uint64_t change = change_dst.amount;

      if (0 != change)
      {
        splitted_dsts.push_back(cryptonote::tx_destination_entry(change, change_dst.addr));
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void print_source_entry(const cryptonote::tx_source_entry& src)
    {
      std::string indexes;
      std::for_each(src.outputs.begin(), src.outputs.end(), [&](const cryptonote::tx_source_entry::output_entry& s_e) { indexes += boost::to_string(s_e.first) + " "; });
      LOG_PRINT_L0("amount=" << cryptonote::print_money(src.amount) << ", real_output=" <<src.real_output << ", real_output_in_tx_index=" << src.real_output_in_tx_index << ", indexes: " << indexes);
    }
    //----------------------------------------------------------------------------------------------------
  }
  //----------------------------------------------------------------------------------------------------
  template<typename T>
  void wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, const size_t fake_outs_count, const std::vector<size_t> &unused_transfers_indices,
    uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, bool trusted_daemon)
  {
    pending_tx ptx;
    cryptonote::transaction tx;
    transfer(dsts, fake_outs_count, unused_transfers_indices, unlock_time, fee, extra, destination_split_strategy, dust_policy, tx, ptx, trusted_daemon);
  }

  template<typename T>
  void wallet2::transfer(const std::vector<cryptonote::tx_destination_entry>& dsts, size_t fake_outputs_count, const std::vector<size_t> &unused_transfers_indices,
    uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx, bool trusted_daemon)
  {
    using namespace cryptonote;
    // throw if attempting a transaction with no destinations
    THROW_WALLET_EXCEPTION_IF(dsts.empty(), error::zero_destination);

    uint64_t upper_transaction_size_limit = get_upper_transaction_size_limit();
    uint64_t needed_money = fee;

    // calculate total amount being sent to all destinations
    // throw if total amount overflows uint64_t
    for(auto& dt: dsts)
    {
      THROW_WALLET_EXCEPTION_IF(0 == dt.amount, error::zero_destination);
      needed_money += dt.amount;
      THROW_WALLET_EXCEPTION_IF(needed_money < dt.amount, error::tx_sum_overflow, dsts, fee, m_testnet);
    }

    // randomly select inputs for transaction
    // throw if requested send amount is greater than amount available to send
    std::list<size_t> selected_transfers;
    uint64_t found_money = select_transfers(needed_money, unused_transfers_indices, selected_transfers, trusted_daemon);
    THROW_WALLET_EXCEPTION_IF(found_money < needed_money, error::not_enough_money, found_money, needed_money - fee, fee);

    typedef COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::out_entry out_entry;
    typedef cryptonote::tx_source_entry::output_entry tx_output_entry;

    COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::response daemon_resp = AUTO_VAL_INIT(daemon_resp);
    if(fake_outputs_count)
    {
      COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::request req = AUTO_VAL_INIT(req);
      req.outs_count = fake_outputs_count + 1;// add one to make possible (if need) to skip real output key
      for(size_t idx: selected_transfers)
      {
        const transfer_container::const_iterator it = m_transfers.begin() + idx;
        THROW_WALLET_EXCEPTION_IF(it->m_tx.vout.size() <= it->m_internal_output_index, error::wallet_internal_error,
          "m_internal_output_index = " + std::to_string(it->m_internal_output_index) +
          " is greater or equal to outputs count = " + std::to_string(it->m_tx.vout.size()));
        req.amounts.push_back(it->amount());
      }

      m_daemon_rpc_mutex.lock();
      bool r = epee::net_utils::invoke_http_bin("/getrandom_outs.bin", req, daemon_resp, m_http_client, rpc_timeout);
      m_daemon_rpc_mutex.unlock();
      THROW_WALLET_EXCEPTION_IF(!r, error::no_connection_to_daemon, "getrandom_outs.bin");
      THROW_WALLET_EXCEPTION_IF(daemon_resp.status == CORE_RPC_STATUS_BUSY, error::daemon_busy, "getrandom_outs.bin");
      THROW_WALLET_EXCEPTION_IF(daemon_resp.status != CORE_RPC_STATUS_OK, error::get_random_outs_error, daemon_resp.status);
      THROW_WALLET_EXCEPTION_IF(daemon_resp.outs.size() != selected_transfers.size(), error::wallet_internal_error,
        "daemon returned wrong response for getrandom_outs.bin, wrong amounts count = " +
        std::to_string(daemon_resp.outs.size()) + ", expected " +  std::to_string(selected_transfers.size()));

      std::unordered_map<uint64_t, uint64_t> scanty_outs;
      for(COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& amount_outs: daemon_resp.outs)
      {
        if (amount_outs.outs.size() < fake_outputs_count)
        {
          scanty_outs[amount_outs.amount] = amount_outs.outs.size();
        }
      }
      THROW_WALLET_EXCEPTION_IF(!scanty_outs.empty(), error::not_enough_outs_to_mix, scanty_outs, fake_outputs_count);
    }

    //prepare inputs
    size_t i = 0;
    std::vector<cryptonote::tx_source_entry> sources;
    for(size_t idx: selected_transfers)
    {
      sources.resize(sources.size()+1);
      cryptonote::tx_source_entry& src = sources.back();
      const transfer_details& td = m_transfers[idx];
      src.amount = td.amount();
      src.rct = false;
      //paste mixin transaction
      if(daemon_resp.outs.size())
      {
        daemon_resp.outs[i].outs.sort([](const out_entry& a, const out_entry& b){return a.global_amount_index < b.global_amount_index;});
        for(out_entry& daemon_oe: daemon_resp.outs[i].outs)
        {
          if(td.m_global_output_index == daemon_oe.global_amount_index)
            continue;
          tx_output_entry oe;
          oe.first = daemon_oe.global_amount_index;
          oe.second.dest = rct::pk2rct(daemon_oe.out_key);
          oe.second.mask = rct::identity();
          src.outputs.push_back(oe);
          if(src.outputs.size() >= fake_outputs_count)
            break;
        }
      }

      //paste real transaction to the random index
      auto it_to_insert = std::find_if(src.outputs.begin(), src.outputs.end(), [&](const tx_output_entry& a)
      {
        return a.first >= td.m_global_output_index;
      });
      //size_t real_index = src.outputs.size() ? (rand() % src.outputs.size() ):0;
      tx_output_entry real_oe;
      real_oe.first = td.m_global_output_index;
      real_oe.second.dest = rct::pk2rct(boost::get<txout_to_key>(td.m_tx.vout[td.m_internal_output_index].target).key);
      real_oe.second.mask = rct::identity();
      auto interted_it = src.outputs.insert(it_to_insert, real_oe);
      src.real_out_tx_key = get_tx_pub_key_from_extra(td.m_tx);
      src.real_output = interted_it - src.outputs.begin();
      src.real_output_in_tx_index = td.m_internal_output_index;
      detail::print_source_entry(src);
      ++i;
    }

    cryptonote::tx_destination_entry change_dts = AUTO_VAL_INIT(change_dts);
    if (needed_money < found_money)
    {
      change_dts.addr = m_account.get_keys().m_account_address;
      change_dts.amount = found_money - needed_money;
    }

    std::vector<cryptonote::tx_destination_entry> splitted_dsts, dust_dsts;
    uint64_t dust = 0;
    destination_split_strategy(dsts, change_dts, dust_policy.dust_threshold, splitted_dsts, dust_dsts);
    for(auto& d: dust_dsts) {
      THROW_WALLET_EXCEPTION_IF(dust_policy.dust_threshold < d.amount, error::wallet_internal_error, "invalid dust value: dust = " +
        std::to_string(d.amount) + ", dust_threshold = " + std::to_string(dust_policy.dust_threshold));
    }
    for(auto& d: dust_dsts) {
      if (!dust_policy.add_to_fee)
        splitted_dsts.push_back(cryptonote::tx_destination_entry(d.amount, dust_policy.addr_for_dust));
      dust += d.amount;
    }

    crypto::secret_key tx_key;
    bool r = cryptonote::construct_tx_and_get_tx_key(m_account.get_keys(), sources, splitted_dsts, extra, tx, unlock_time, tx_key);
    THROW_WALLET_EXCEPTION_IF(!r, error::tx_not_constructed, sources, splitted_dsts, unlock_time, m_testnet);
    THROW_WALLET_EXCEPTION_IF(upper_transaction_size_limit <= get_object_blobsize(tx), error::tx_too_big, tx, upper_transaction_size_limit);

    std::string key_images;
    bool all_are_txin_to_key = std::all_of(tx.vin.begin(), tx.vin.end(), [&](const txin_v& s_e) -> bool
    {
      CHECKED_GET_SPECIFIC_VARIANT(s_e, const txin_to_key, in, false);
      key_images += boost::to_string(in.k_image) + " ";
      return true;
    });
    THROW_WALLET_EXCEPTION_IF(!all_are_txin_to_key, error::unexpected_txin_type, tx);
    
    bool dust_sent_elsewhere = (dust_policy.addr_for_dust.m_view_public_key != change_dts.addr.m_view_public_key
                                || dust_policy.addr_for_dust.m_spend_public_key != change_dts.addr.m_spend_public_key);      

    if (dust_policy.add_to_fee || dust_sent_elsewhere) change_dts.amount -= dust;

    ptx.key_images = key_images;
    ptx.fee = (dust_policy.add_to_fee ? fee+dust : fee);
    ptx.dust = ((dust_policy.add_to_fee || dust_sent_elsewhere) ? dust : 0);
    ptx.dust_added_to_fee = dust_policy.add_to_fee;
    ptx.tx = tx;
    ptx.change_dts = change_dts;
    ptx.selected_transfers = selected_transfers;
    ptx.tx_key = tx_key;
    ptx.dests = dsts;
    ptx.construction_data.sources = sources;
    ptx.construction_data.change_dts = change_dts;
    ptx.construction_data.splitted_dsts = splitted_dsts;
    ptx.construction_data.selected_transfers = selected_transfers;
    ptx.construction_data.extra = tx.extra;
    ptx.construction_data.unlock_time = unlock_time;
    ptx.construction_data.use_rct = false;
    ptx.construction_data.dests = dsts;
  }


}
