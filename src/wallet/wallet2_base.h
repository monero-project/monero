// Copyright (c) 2014-2020, The Monero Project
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

namespace boost
{
    namespace program_options
    {
        class options_description;
        class variables_map;
    }
}

namespace cryptonote
{
    struct address_parse_info;
    class account_base;
    class transaction;
    struct public_node;
}

namespace mms
{
    class message_store;
    struct multisig_wallet_state;
}

#include "cryptonote_basic/blobdatatype.h"
#include "device/device.hpp"
#include "net/http.h"
#include "wallet_light_rpc.h"

#include <memory>

class Serialization_portability_wallet_Test;
class wallet_accessor_test;
struct tx_dust_policy;

namespace tools
{ 
  class password_container;   
  class i_wallet2_callback;
  class ringdb;
  class Notify;

  class wallet_keys_unlocker;
  class wallet2_base
  {
    friend class ::Serialization_portability_wallet_Test;
    friend class ::wallet_accessor_test;
    friend class wallet_keys_unlocker;
    friend class wallet_device_callback;
  public:
    static constexpr const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

    enum RefreshType {
      RefreshFull,
      RefreshOptimizeCoinbase,
      RefreshNoCoinbase,
      RefreshDefault = RefreshOptimizeCoinbase,
    };

    enum AskPasswordType {
      AskPasswordNever = 0,
      AskPasswordOnAction = 1,
      AskPasswordToDecrypt = 2,
    };

    enum BackgroundMiningSetupType {
      BackgroundMiningMaybe = 0,
      BackgroundMiningYes = 1,
      BackgroundMiningNo = 2,
    };

    enum ExportFormat {
      Binary = 0,
      Ascii,
    };

    static const char* tr(const char* str);

    static bool has_testnet_option(const boost::program_options::variables_map& vm);
    static bool has_stagenet_option(const boost::program_options::variables_map& vm);
    static std::string device_name_option(const boost::program_options::variables_map& vm);
    static std::string device_derivation_path_option(const boost::program_options::variables_map &vm);
    static void init_options(boost::program_options::options_description& desc_params);

    //! Uses stdin and stdout. Returns a wallet2_base if no errors.
    static std::pair<std::unique_ptr<wallet2_base>, password_container> make_from_json(const boost::program_options::variables_map& vm, bool unattended, const std::string& json_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Uses stdin and stdout. Returns a wallet2_base and password for `wallet_file` if no errors.
    static std::pair<std::unique_ptr<wallet2_base>, password_container>
      make_from_file(const boost::program_options::variables_map& vm, bool unattended, const std::string& wallet_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Uses stdin and stdout. Returns a wallet2_base and password for wallet with no file if no errors.
    static std::pair<std::unique_ptr<wallet2_base>, password_container> make_new(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Just parses variables.
    static std::unique_ptr<wallet2_base> make_dummy(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    static bool verify_password(const std::string& keys_file_name, const epee::wipeable_string& password, bool no_spend_key, hw::device &hwdev, uint64_t kdf_rounds);
    static bool query_device(hw::device::device_type& device_type, const std::string& keys_file_name, const epee::wipeable_string& password, uint64_t kdf_rounds = 1);

    static std::unique_ptr<wallet2_base> create(cryptonote::network_type nettype = cryptonote::MAINNET, uint64_t kdf_rounds = 1, bool unattended = false, std::unique_ptr<epee::net_utils::http::http_client_factory> http_client_factory = std::unique_ptr<epee::net_utils::http::http_client_factory>(new net::http::client_factory()));
    virtual ~wallet2_base();

    //struct multisig_info;
    //struct tx_scan_info_t;

    //struct transfer_details;
    //typedef std::vector<uint64_t> amounts_container;
    //struct payment_details;
    //struct address_tx;
    //struct pool_payment_details;

    //struct unconfirmed_transfer_details;
    //struct confirmed_transfer_details;
    //struct tx_construction_data;
    //typedef std::vector<transfer_details> transfer_container;
    //typedef serializable_unordered_multimap<crypto::hash, payment_details> payment_container;

    //struct multisig_sig;
    // The convention for destinations is:
    // dests does not include change
    // splitted_dsts (in construction_data) does
    //struct pending_tx;
    // The term "Unsigned tx" is not really a tx since it's not signed yet.
    // It doesnt have tx hash, key and the integrated address is not separated into addr + payment id.
    //struct unsigned_tx_set;
    //struct signed_tx_set;
    //struct multisig_tx_set;
    //struct keys_file_data;
    //struct cache_file_data;
    // GUI Address book
    //struct address_book_row;
    //struct reserve_proof_entry;
    //typedef std::tuple<uint64_t, crypto::public_key, rct::key> get_outs_entry;

    //struct parsed_block;
    //struct is_out_data;
    //struct tx_cache_data;

    /*!
     * \brief  Generates a wallet or restores one.
     * \param  wallet_              Name of wallet file
     * \param  password             Password of wallet file
     * \param  multisig_data        The multisig restore info and keys
     * \param  create_address_file  Whether to create an address file
     */
    virtual void generate(const std::string& wallet_, const epee::wipeable_string& password,
      const epee::wipeable_string& multisig_data, bool create_address_file = false) = 0;

    /*!
     * \brief Generates a wallet or restores one.
     * \param  wallet_              Name of wallet file
     * \param  password             Password of wallet file
     * \param  recovery_param       If it is a restore, the recovery key
     * \param  recover              Whether it is a restore
     * \param  two_random           Whether it is a non-deterministic wallet
     * \param  create_address_file  Whether to create an address file
     * \return                      The secret key of the generated wallet
     */
    virtual crypto::secret_key generate(const std::string& wallet, const epee::wipeable_string& password,
      const crypto::secret_key& recovery_param = crypto::secret_key(), bool recover = false,
      bool two_random = false, bool create_address_file = false) = 0;
    /*!
     * \brief Creates a wallet from a public address and a spend/view secret key pair.
     * \param  wallet_                 Name of wallet file
     * \param  password                Password of wallet file
     * \param  account_public_address  The account's public address
     * \param  spendkey                spend secret key
     * \param  viewkey                 view secret key
     * \param  create_address_file     Whether to create an address file
     */
    virtual void generate(const std::string& wallet, const epee::wipeable_string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& spendkey, const crypto::secret_key& viewkey, bool create_address_file = false) = 0;
    /*!
     * \brief Creates a watch only wallet from a public address and a view secret key.
     * \param  wallet_                 Name of wallet file
     * \param  password                Password of wallet file
     * \param  account_public_address  The account's public address
     * \param  viewkey                 view secret key
     * \param  create_address_file     Whether to create an address file
     */
    virtual void generate(const std::string& wallet, const epee::wipeable_string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& viewkey = crypto::secret_key(), bool create_address_file = false) = 0;
    /*!
     * \brief Restore a wallet hold by an HW.
     * \param  wallet_        Name of wallet file
     * \param  password       Password of wallet file
     * \param  device_name    name of HW to use
     * \param  create_address_file     Whether to create an address file
     */
    virtual void restore(const std::string& wallet_, const epee::wipeable_string& password, const std::string &device_name, bool create_address_file = false) = 0;

    /*!
     * \brief Creates a multisig wallet
     * \return empty if done, non empty if we need to send another string
     * to other participants
     */
    virtual std::string make_multisig(const epee::wipeable_string &password,
      const std::vector<std::string> &info,
      uint32_t threshold) = 0;
    /*!
     * \brief Creates a multisig wallet
     * \return empty if done, non empty if we need to send another string
     * to other participants
     */
    virtual std::string make_multisig(const epee::wipeable_string &password,
      const std::vector<crypto::secret_key> &view_keys,
      const std::vector<crypto::public_key> &spend_keys,
      uint32_t threshold) = 0;
    virtual std::string exchange_multisig_keys(const epee::wipeable_string &password,
      const std::vector<std::string> &info) = 0;
    /*!
     * \brief Any but first round of keys exchange
     */
    virtual std::string exchange_multisig_keys(const epee::wipeable_string &password,
      std::unordered_set<crypto::public_key> pkeys,
      std::vector<crypto::public_key> signers) = 0;
    /*!
     * \brief Finalizes creation of a multisig wallet
     */
    virtual bool finalize_multisig(const epee::wipeable_string &password, const std::vector<std::string> &info) = 0;
    /*!
     * \brief Finalizes creation of a multisig wallet
     */
    virtual bool finalize_multisig(const epee::wipeable_string &password, const std::unordered_set<crypto::public_key> &pkeys, std::vector<crypto::public_key> signers) = 0;
    /*!
     * Get a packaged multisig information string
     */
    virtual std::string get_multisig_info() const = 0;
    /*!
     * Verifies and extracts keys from a packaged multisig information string
     */
    static bool verify_multisig_info(const std::string &data, crypto::secret_key &skey, crypto::public_key &pkey);
    /*!
     * Verifies and extracts keys from a packaged multisig information string
     */
    static bool verify_extra_multisig_info(const std::string &data, std::unordered_set<crypto::public_key> &pkeys, crypto::public_key &signer);
    /*!
     * Export multisig info
     * This will generate and remember new k values
     */
    virtual cryptonote::blobdata export_multisig() = 0;
    /*!
     * Import a set of multisig info from multisig partners
     * \return the number of inputs which were imported
     */
    virtual size_t import_multisig(std::vector<cryptonote::blobdata> info) = 0;
    /*!
     * \brief Rewrites to the wallet file for wallet upgrade (doesn't generate key, assumes it's already there)
     * \param wallet_name Name of wallet file (should exist)
     * \param password    Password for wallet file
     */
    virtual void rewrite(const std::string& wallet_name, const epee::wipeable_string& password) = 0;
    virtual void write_watch_only_wallet(const std::string& wallet_name, const epee::wipeable_string& password, std::string &new_keys_filename) = 0;
    virtual void load(const std::string& wallet, const epee::wipeable_string& password, const std::string& keys_buf = "", const std::string& cache_buf = "") = 0;
    virtual void store() = 0;
    /*!
     * \brief store_to  Stores wallet to another file(s), deleting old ones
     * \param path      Path to the wallet file (keys and address filenames will be generated based on this filename)
     * \param password  Password to protect new wallet (TODO: probably better save the password in the wallet object?)
     */
    virtual void store_to(const std::string &path, const epee::wipeable_string &password) = 0;
    /*!
     * \brief get_keys_file_data  Get wallet keys data which can be stored to a wallet file.
     * \param password            Password of the encrypted wallet buffer (TODO: probably better save the password in the wallet object?)
     * \param watch_only          true to include only view key, false to include both spend and view keys
     * \return                    Encrypted wallet keys data which can be stored to a wallet file
     */
    //virtual boost::optional<wallet2_base::keys_file_data> get_keys_file_data(const epee::wipeable_string& password, bool watch_only) = 0;
    /*!
     * \brief get_cache_file_data   Get wallet cache data which can be stored to a wallet file.
     * \param password              Password to protect the wallet cache data (TODO: probably better save the password in the wallet object?)
     * \return                      Encrypted wallet cache data which can be stored to a wallet file
     */
    //virtual boost::optional<wallet2_base::cache_file_data> get_cache_file_data(const epee::wipeable_string& password) = 0;

    virtual std::string path() const = 0;

    /*!
     * \brief verifies given password is correct for default wallet keys file
     */
    virtual bool verify_password(const epee::wipeable_string& password) = 0;
    virtual cryptonote::account_base& get_account() = 0;
    virtual const cryptonote::account_base& get_account()const = 0;

    virtual void encrypt_keys(const crypto::chacha_key &key) = 0;
    virtual void encrypt_keys(const epee::wipeable_string &password) = 0;
    virtual void decrypt_keys(const crypto::chacha_key &key) = 0;
    virtual void decrypt_keys(const epee::wipeable_string &password) = 0;

    virtual void set_refresh_from_block_height(uint64_t height) = 0;
    virtual uint64_t get_refresh_from_block_height() const = 0;

    virtual void explicit_refresh_from_block_height(bool expl) = 0;
    virtual bool explicit_refresh_from_block_height() const = 0;

    virtual void max_reorg_depth(uint64_t depth) = 0;
    virtual uint64_t max_reorg_depth() const = 0;

    virtual bool deinit() = 0;
    virtual bool init(std::string daemon_address = "http://localhost:8080",
      boost::optional<epee::net_utils::http::login> daemon_login = boost::none,
      const std::string &proxy = "",
      uint64_t upper_transaction_weight_limit = 0,
      bool trusted_daemon = true,
      epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect) = 0;
    virtual bool set_daemon(std::string daemon_address = "http://localhost:8080",
      boost::optional<epee::net_utils::http::login> daemon_login = boost::none, bool trusted_daemon = true,
      epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect) = 0;
    virtual bool set_proxy(const std::string &address) = 0;

    virtual void stop() = 0;

    virtual i_wallet2_callback* callback() const = 0;
    virtual void callback(i_wallet2_callback* callback) = 0;

    virtual bool is_trusted_daemon() const = 0;
    virtual void set_trusted_daemon(bool trusted) = 0;

    /*!
     * \brief Checks if deterministic wallet
     */
    virtual bool is_deterministic() const = 0;
    virtual bool get_seed(epee::wipeable_string& electrum_words, const epee::wipeable_string &passphrase = epee::wipeable_string()) const = 0;

    /*!
    * \brief Checks if light wallet. A light wallet sends view key to a server where the blockchain is scanned.
    */
    virtual bool light_wallet() const = 0;
    virtual void set_light_wallet(bool light_wallet) = 0;
    virtual uint64_t get_light_wallet_scanned_block_height() const = 0;
    virtual uint64_t get_light_wallet_blockchain_height() const = 0;

    /*!
     * \brief Gets the seed language
     */
    virtual const std::string &get_seed_language() const = 0;
    /*!
     * \brief Sets the seed language
     */
    virtual void set_seed_language(const std::string &language) = 0;

    // Subaddress scheme
    virtual cryptonote::account_public_address get_subaddress(const cryptonote::subaddress_index& index) const = 0;
    virtual cryptonote::account_public_address get_address() const = 0;
    virtual boost::optional<cryptonote::subaddress_index> get_subaddress_index(const cryptonote::account_public_address& address) const = 0;
    virtual crypto::public_key get_subaddress_spend_public_key(const cryptonote::subaddress_index& index) const = 0;
    virtual std::vector<crypto::public_key> get_subaddress_spend_public_keys(uint32_t account, uint32_t begin, uint32_t end) const = 0;
    virtual std::string get_subaddress_as_str(const cryptonote::subaddress_index& index) const = 0;
    virtual std::string get_address_as_str() const = 0;
    virtual std::string get_integrated_address_as_str(const crypto::hash8& payment_id) const = 0;
    virtual void add_subaddress_account(const std::string& label) = 0;
    virtual size_t get_num_subaddress_accounts() const = 0;
    virtual size_t get_num_subaddresses(uint32_t index_major) const = 0;
    virtual void add_subaddress(uint32_t index_major, const std::string& label) = 0; // throws when index is out of bound
    virtual void expand_subaddresses(const cryptonote::subaddress_index& index) = 0;
    virtual void create_one_off_subaddress(const cryptonote::subaddress_index& index) = 0;
    virtual std::string get_subaddress_label(const cryptonote::subaddress_index& index) const = 0;
    virtual void set_subaddress_label(const cryptonote::subaddress_index &index, const std::string &label) = 0;
    virtual void set_subaddress_lookahead(size_t major, size_t minor) = 0;
    virtual std::pair<size_t, size_t> get_subaddress_lookahead() const = 0;
    /*!
     * \brief Tells if the wallet file is deprecated.
     */
    virtual bool is_deprecated() const = 0;
    virtual void refresh(bool trusted_daemon) = 0;
    virtual void refresh(bool trusted_daemon, uint64_t start_height, uint64_t & blocks_fetched) = 0;
    virtual void refresh(bool trusted_daemon, uint64_t start_height, uint64_t & blocks_fetched, bool& received_money, bool check_pool = true) = 0;
    virtual bool refresh(bool trusted_daemon, uint64_t & blocks_fetched, bool& received_money, bool& ok) = 0;

    virtual void set_refresh_type(RefreshType refresh_type) = 0;
    virtual RefreshType get_refresh_type() const = 0;

    virtual cryptonote::network_type nettype() const = 0;
    virtual bool watch_only() const = 0;
    virtual bool multisig(bool *ready = NULL, uint32_t *threshold = NULL, uint32_t *total = NULL) const = 0;
    virtual bool has_multisig_partial_key_images() const = 0;
    virtual bool has_unknown_key_images() const = 0;
    virtual bool get_multisig_seed(epee::wipeable_string& seed, const epee::wipeable_string &passphrase = std::string(), bool raw = true) const = 0;
    virtual bool key_on_device() const = 0;
    virtual hw::device::device_type get_device_type() const = 0;
    virtual bool reconnect_device() = 0;

    // locked & unlocked balance of given or current subaddress account
    virtual uint64_t balance(uint32_t subaddr_index_major, bool strict) const = 0;
    virtual uint64_t unlocked_balance(uint32_t subaddr_index_major, bool strict, uint64_t *blocks_to_unlock = NULL, uint64_t *time_to_unlock = NULL) = 0;
    // locked & unlocked balance per subaddress of given or current subaddress account
    virtual std::map<uint32_t, uint64_t> balance_per_subaddress(uint32_t subaddr_index_major, bool strict) const = 0;
    virtual std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> unlocked_balance_per_subaddress(uint32_t subaddr_index_major, bool strict) = 0;
    // all locked & unlocked balances of all subaddress accounts
    virtual uint64_t balance_all(bool strict) const = 0;
    virtual uint64_t unlocked_balance_all(bool strict, uint64_t *blocks_to_unlock = NULL, uint64_t *time_to_unlock = NULL) = 0;
    
    /*template<typename T>
    void transfer_selected(const std::vector<cryptonote::tx_destination_entry>& dsts, const std::vector<size_t>& selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2_base::get_outs_entry>> &outs,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx) = 0;
    virtual void transfer_selected_rct(std::vector<cryptonote::tx_destination_entry> dsts, const std::vector<size_t>& selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2_base::get_outs_entry>> &outs,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx &ptx, const rct::RCTConfig &rct_config) = 0;
    */
    
    // All these commented out methods will be able to be reenabled after extracting the types from wallet2 class.
    
    //virtual void commit_tx(pending_tx& ptx_vector) = 0;
    //virtual void commit_tx(std::vector<pending_tx>& ptx_vector) = 0;
    //virtual bool save_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename) const = 0;
    //virtual std::string dump_tx_to_str(const std::vector<pending_tx> &ptx_vector) const = 0;
    //virtual std::string save_multisig_tx(multisig_tx_set txs) = 0;
    //virtual bool save_multisig_tx(const multisig_tx_set &txs, const std::string &filename) = 0;
    //virtual std::string save_multisig_tx(const std::vector<pending_tx>& ptx_vector) = 0;
    //virtual bool save_multisig_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename) = 0;
    //virtual multisig_tx_set make_multisig_tx_set(const std::vector<pending_tx>& ptx_vector) const = 0;
    // load unsigned tx from file and sign it. Takes confirmation callback as argument. Used by the cli wallet
    //virtual bool sign_tx(const std::string &unsigned_filename, const std::string &signed_filename, std::vector<wallet2_base::pending_tx> &ptx, std::function<bool(const unsigned_tx_set&)> accept_func = NULL, bool export_raw = false) = 0;
    // sign unsigned tx. Takes unsigned_tx_set as argument. Used by GUI
    //virtual bool sign_tx(unsigned_tx_set &exported_txs, const std::string &signed_filename, std::vector<wallet2_base::pending_tx> &ptx, bool export_raw = false) = 0;
    //virtual bool sign_tx(unsigned_tx_set &exported_txs, std::vector<wallet2_base::pending_tx> &ptx, signed_tx_set &signed_txs) = 0;
    //virtual std::string sign_tx_dump_to_str(unsigned_tx_set &exported_txs, std::vector<wallet2_base::pending_tx> &ptx, signed_tx_set &signed_txes) = 0;
    // load unsigned_tx_set from file. 
    //virtual bool load_unsigned_tx(const std::string &unsigned_filename, unsigned_tx_set &exported_txs) const = 0;
    //virtual bool parse_unsigned_tx_from_str(const std::string &unsigned_tx_st, unsigned_tx_set &exported_txs) const = 0;
    //virtual bool load_tx(const std::string &signed_filename, std::vector<tools::wallet2_base::pending_tx> &ptx, std::function<bool(const signed_tx_set&)> accept_func = NULL) = 0;
    //virtual bool parse_tx_from_str(const std::string &signed_tx_st, std::vector<tools::wallet2_base::pending_tx> &ptx, std::function<bool(const signed_tx_set &)> accept_func) = 0;
    //virtual std::vector<wallet2_base::pending_tx> create_transactions_2(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices) = 0;     // pass subaddr_indices by value on purpose
    //virtual std::vector<wallet2_base::pending_tx> create_transactions_all(uint64_t below, const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices) = 0;
    //virtual std::vector<wallet2_base::pending_tx> create_transactions_single(const crypto::key_image &ki, const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra) = 0;
    //virtual std::vector<wallet2_base::pending_tx> create_transactions_from(const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, std::vector<size_t> unused_transfers_indices, std::vector<size_t> unused_dust_indices, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra) = 0;
    //virtual bool sanity_check(const std::vector<wallet2_base::pending_tx> &ptx_vector, std::vector<cryptonote::tx_destination_entry> dsts) const = 0;
    //virtual void cold_tx_aux_import(const std::vector<pending_tx>& ptx, const std::vector<std::string>& tx_device_aux) = 0;
    //virtual void cold_sign_tx(const std::vector<pending_tx>& ptx_vector, signed_tx_set &exported_txs, std::vector<cryptonote::address_parse_info> &dsts_info, std::vector<std::string> & tx_device_aux) = 0;
    virtual uint64_t cold_key_image_sync(uint64_t &spent, uint64_t &unspent) = 0;
    virtual void device_show_address(uint32_t account_index, uint32_t address_index, const boost::optional<crypto::hash8> &payment_id) = 0;
    //virtual bool parse_multisig_tx_from_str(std::string multisig_tx_st, multisig_tx_set &exported_txs) const = 0;
    //virtual bool load_multisig_tx(cryptonote::blobdata blob, multisig_tx_set &exported_txs, std::function<bool(const multisig_tx_set&)> accept_func = NULL) = 0;
    //virtual bool load_multisig_tx_from_file(const std::string &filename, multisig_tx_set &exported_txs, std::function<bool(const multisig_tx_set&)> accept_func = NULL) = 0;
    //virtual bool sign_multisig_tx_from_file(const std::string &filename, std::vector<crypto::hash> &txids, std::function<bool(const multisig_tx_set&)> accept_func) = 0;
    //virtual bool sign_multisig_tx(multisig_tx_set &exported_txs, std::vector<crypto::hash> &txids) = 0;
    //virtual bool sign_multisig_tx_to_file(multisig_tx_set &exported_txs, const std::string &filename, std::vector<crypto::hash> &txids) = 0;
    //virtual std::vector<pending_tx> create_unmixable_sweep_transactions() = 0;
    virtual void discard_unmixable_outputs() = 0;
    virtual bool check_connection(uint32_t *version = NULL, bool *ssl = NULL, uint32_t timeout = 200000) = 0;
    //virtual void get_transfers(wallet2_base::transfer_container& incoming_transfers) const = 0;
    //virtual void get_payments(const crypto::hash& payment_id, std::list<wallet2_base::payment_details>& payments, uint64_t min_height = 0, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const = 0;
    //virtual void get_payments(std::list<std::pair<crypto::hash,wallet2_base::payment_details>>& payments, uint64_t min_height, uint64_t max_height = (uint64_t)-1, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const = 0;
    //virtual void get_payments_out(std::list<std::pair<crypto::hash,wallet2_base::confirmed_transfer_details>>& confirmed_payments,
    //  uint64_t min_height, uint64_t max_height = (uint64_t)-1, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const = 0;
    //virtual void get_unconfirmed_payments_out(std::list<std::pair<crypto::hash,wallet2_base::unconfirmed_transfer_details>>& unconfirmed_payments, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const = 0;
    //virtual void get_unconfirmed_payments(std::list<std::pair<crypto::hash,wallet2_base::pool_payment_details>>& unconfirmed_payments, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const = 0;

    virtual uint64_t get_blockchain_current_height() const = 0;
    virtual void rescan_spent() = 0;
    virtual void rescan_blockchain(bool hard, bool refresh = true, bool keep_key_images = false) = 0;
    //virtual bool is_transfer_unlocked(const transfer_details& td) = 0;
    virtual bool is_transfer_unlocked(uint64_t unlock_time, uint64_t block_height) = 0;

    virtual uint64_t get_last_block_reward() const = 0;
    virtual uint64_t get_device_last_key_image_sync() const = 0;

    virtual std::vector<cryptonote::public_node> get_public_nodes(bool white_only = true) = 0;

    //template <class t_archive>
    //void serialize(t_archive &a, const unsigned int ver) = 0;

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

    virtual bool always_confirm_transfers() const = 0;
    virtual void always_confirm_transfers(bool always) = 0;
    virtual bool print_ring_members() const = 0;
    virtual void print_ring_members(bool value) = 0;
    virtual bool store_tx_info() const = 0;
    virtual void store_tx_info(bool store) = 0;
    virtual uint32_t default_mixin() const = 0;
    virtual void default_mixin(uint32_t m) = 0;
    virtual uint32_t get_default_priority() const = 0;
    virtual void set_default_priority(uint32_t p) = 0;
    virtual bool auto_refresh() const = 0;
    virtual void auto_refresh(bool r) = 0;
    virtual AskPasswordType ask_password() const = 0;
    virtual void ask_password(AskPasswordType ask) = 0;
    virtual void set_min_output_count(uint32_t count) = 0;
    virtual uint32_t get_min_output_count() const = 0;
    virtual void set_min_output_value(uint64_t value) = 0;
    virtual uint64_t get_min_output_value() const = 0;
    virtual void merge_destinations(bool merge) = 0;
    virtual bool merge_destinations() const = 0;
    virtual bool confirm_backlog() const = 0;
    virtual void confirm_backlog(bool always) = 0;
    virtual void set_confirm_backlog_threshold(uint32_t threshold) = 0;
    virtual uint32_t get_confirm_backlog_threshold() const = 0;
    virtual bool confirm_export_overwrite() const = 0;
    virtual void confirm_export_overwrite(bool always) = 0;
    virtual bool auto_low_priority() const = 0;
    virtual void auto_low_priority(bool value) = 0;
    virtual bool segregate_pre_fork_outputs() const = 0;
    virtual void segregate_pre_fork_outputs(bool value) = 0;
    virtual bool key_reuse_mitigation2() const = 0;
    virtual void key_reuse_mitigation2(bool value) = 0;
    virtual uint64_t segregation_height() const = 0;
    virtual void segregation_height(uint64_t height) = 0;
    virtual bool ignore_fractional_outputs() const = 0;
    virtual void ignore_fractional_outputs(bool value) = 0;
    virtual bool confirm_non_default_ring_size() const = 0;
    virtual void confirm_non_default_ring_size(bool always) = 0;
    virtual uint64_t ignore_outputs_above() const = 0;
    virtual void ignore_outputs_above(uint64_t value) = 0;
    virtual uint64_t ignore_outputs_below() const = 0;
    virtual void ignore_outputs_below(uint64_t value) = 0;
    virtual bool track_uses() const = 0;
    virtual void track_uses(bool value) = 0;
    virtual BackgroundMiningSetupType setup_background_mining() const = 0;
    virtual void setup_background_mining(BackgroundMiningSetupType value) = 0;
    virtual uint32_t inactivity_lock_timeout() const = 0;
    virtual void inactivity_lock_timeout(uint32_t seconds) = 0;
    virtual const std::string & device_name() const = 0;
    virtual void device_name(const std::string & device_name) = 0;
    virtual const std::string & device_derivation_path() const = 0;
    virtual void device_derivation_path(const std::string &device_derivation_path) = 0;
    virtual const ExportFormat & export_format() const = 0;
    virtual inline void set_export_format(const ExportFormat& export_format) = 0;
    virtual bool load_deprecated_formats() const = 0;
    virtual void load_deprecated_formats(bool load) = 0;
    virtual bool persistent_rpc_client_id() const = 0;
    virtual void persistent_rpc_client_id(bool persistent) = 0;
    virtual void auto_mine_for_rpc_payment_threshold(float threshold) = 0;
    virtual float auto_mine_for_rpc_payment_threshold() const = 0;
    virtual crypto::secret_key get_rpc_client_secret_key() const = 0;
    virtual void set_rpc_client_secret_key(const crypto::secret_key &key) = 0;
    virtual uint64_t credits_target() const = 0;
    virtual void credits_target(uint64_t threshold) = 0;

    virtual bool get_tx_key_cached(const crypto::hash &txid, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys) const = 0;
    virtual void set_tx_key(const crypto::hash &txid, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const boost::optional<cryptonote::account_public_address> &single_destination_subaddress) = 0;
    virtual bool get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys) = 0;
    virtual void check_tx_key(const crypto::hash &txid, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const cryptonote::account_public_address &address, uint64_t &received, bool &in_pool, uint64_t &confirmations) = 0;
    virtual void check_tx_key_helper(const crypto::hash &txid, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, const cryptonote::account_public_address &address, uint64_t &received, bool &in_pool, uint64_t &confirmations) = 0;
    virtual void check_tx_key_helper(const cryptonote::transaction &tx, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, const cryptonote::account_public_address &address, uint64_t &received) const = 0;
    virtual std::string get_tx_proof(const crypto::hash &txid, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message) = 0;
    virtual std::string get_tx_proof(const cryptonote::transaction &tx, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message) const = 0;
    virtual bool check_tx_proof(const crypto::hash &txid, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message, const std::string &sig_str, uint64_t &received, bool &in_pool, uint64_t &confirmations) = 0;
    virtual bool check_tx_proof(const cryptonote::transaction &tx, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message, const std::string &sig_str, uint64_t &received) const = 0;

    virtual std::string get_spend_proof(const crypto::hash &txid, const std::string &message) = 0;
    virtual bool check_spend_proof(const crypto::hash &txid, const std::string &message, const std::string &sig_str) = 0;

    virtual void scan_tx(const std::vector<crypto::hash> &txids) = 0;

    /*!
     * \brief  Generates a proof that proves the reserve of unspent funds
     * \param  account_minreserve       When specified, collect outputs only belonging to the given account and prove the smallest reserve above the given amount
     *                                  When unspecified, proves for all unspent outputs across all accounts
     * \param  message                  Arbitrary challenge message to be signed together
     * \return                          Signature string
     */
    virtual std::string get_reserve_proof(const boost::optional<std::pair<uint32_t, uint64_t>> &account_minreserve, const std::string &message) = 0;
    /*!
     * \brief  Verifies a proof of reserve
     * \param  address                  The signer's address
     * \param  message                  Challenge message used for signing
     * \param  sig_str                  Signature string
     * \param  total                    [OUT] the sum of funds included in the signature
     * \param  spent                    [OUT] the sum of spent funds included in the signature
     * \return                          true if the signature verifies correctly
     */
    virtual bool check_reserve_proof(const cryptonote::account_public_address &address, const std::string &message, const std::string &sig_str, uint64_t &total, uint64_t &spent) = 0;

   /*!
    * \brief GUI Address book get/store
    */
    //virtual std::vector<address_book_row> get_address_book() const = 0;
    virtual bool add_address_book_row(const cryptonote::account_public_address &address, const crypto::hash8 *payment_id, const std::string &description, bool is_subaddress) = 0;
    virtual bool set_address_book_row(size_t row_id, const cryptonote::account_public_address &address, const crypto::hash8 *payment_id, const std::string &description, bool is_subaddress) = 0;
    virtual bool delete_address_book_row(std::size_t row_id) = 0;
        
    virtual uint64_t get_num_rct_outputs() = 0;
    virtual size_t get_num_transfer_details() const = 0;
    //virtual const transfer_details &get_transfer_details(size_t idx) const = 0;

    virtual uint8_t get_current_hard_fork() = 0;
    virtual void get_hard_fork_info(uint8_t version, uint64_t &earliest_height) = 0;
    virtual bool use_fork_rules(uint8_t version, int64_t early_blocks = 0) = 0;
    virtual int get_fee_algorithm() = 0;

    virtual std::string get_wallet_file() const = 0;
    virtual std::string get_keys_file() const = 0;
    virtual std::string get_daemon_address() const = 0;
    virtual const boost::optional<epee::net_utils::http::login>& get_daemon_login() const = 0;
    virtual uint64_t get_daemon_blockchain_height(std::string& err) = 0;
    virtual uint64_t get_daemon_blockchain_target_height(std::string& err) = 0;
    virtual uint64_t get_daemon_adjusted_time() = 0;

   /*!
    * \brief Calculates the approximate blockchain height from current date/time.
    */
    virtual uint64_t get_approximate_blockchain_height() const = 0;
    virtual uint64_t estimate_blockchain_height() = 0;
    virtual std::vector<size_t> select_available_outputs_from_histogram(uint64_t count, bool atleast, bool unlocked, bool allow_rct) = 0;
    //virtual std::vector<size_t> select_available_outputs(const std::function<bool(const transfer_details &td)> &f) = 0;
    virtual std::vector<size_t> select_available_unmixable_outputs() = 0;
    virtual std::vector<size_t> select_available_mixable_outputs() = 0;

    //virtual size_t pop_best_value_from(const transfer_container &transfers, std::vector<size_t> &unused_dust_indices, const std::vector<size_t>& selected_transfers, bool smallest = false) const = 0;
    virtual size_t pop_best_value(std::vector<size_t> &unused_dust_indices, const std::vector<size_t>& selected_transfers, bool smallest = false) const = 0;

    virtual void set_tx_note(const crypto::hash &txid, const std::string &note) = 0;
    virtual std::string get_tx_note(const crypto::hash &txid) const = 0;

    virtual void set_tx_device_aux(const crypto::hash &txid, const std::string &aux) = 0;
    virtual std::string get_tx_device_aux(const crypto::hash &txid) const = 0;

    virtual void set_description(const std::string &description) = 0;
    virtual std::string get_description() const = 0;

    /*!
     * \brief  Get the list of registered account tags. 
     * \return first.Key=(tag's name), first.Value=(tag's label), second[i]=(i-th account's tag)
     */
    virtual const std::pair<serializable_map<std::string, std::string>, std::vector<std::string>>& get_account_tags() = 0;
    /*!
     * \brief  Set a tag to the given accounts.
     * \param  account_indices  Indices of accounts.
     * \param  tag              Tag's name. If empty, the accounts become untagged.
     */
    virtual void set_account_tag(const std::set<uint32_t> &account_indices, const std::string& tag) = 0;
    /*!
     * \brief  Set the label of the given tag.
     * \param  tag            Tag's name (which must be non-empty).
     * \param  description    Tag's description.
     */
    virtual void set_account_tag_description(const std::string& tag, const std::string& description) = 0;

    enum message_signature_type_t { sign_with_spend_key, sign_with_view_key };
    virtual std::string sign(const std::string &data, message_signature_type_t signature_type, const cryptonote::subaddress_index & index) const = 0;
    struct message_signature_result_t { bool valid; unsigned version; bool old; message_signature_type_t type; };
    virtual message_signature_result_t verify(const std::string &data, const cryptonote::account_public_address &address, const std::string &signature) const = 0;

    /*!
     * \brief sign_multisig_participant signs given message with the multisig public signer key
     * \param data                      message to sign
     * \throws                          if wallet is not multisig
     * \return                          signature
     */
    virtual std::string sign_multisig_participant(const std::string& data) const = 0;
    /*!
     * \brief verify_with_public_key verifies message was signed with given public key
     * \param data                   message
     * \param public_key             public key to check signature
     * \param signature              signature of the message
     * \return                       true if the signature is correct
     */
    virtual bool verify_with_public_key(const std::string &data, const crypto::public_key &public_key, const std::string &signature) const = 0;

    // Import/Export wallet data
    //virtual std::pair<uint64_t, std::vector<tools::wallet2_base::transfer_details>> export_outputs(bool all = false) const = 0;
    virtual std::string export_outputs_to_str(bool all = false) const = 0;
    //virtual size_t import_outputs(const std::pair<uint64_t, std::vector<tools::wallet2_base::transfer_details>> &outputs) = 0;
    virtual size_t import_outputs_from_str(const std::string &outputs_st) = 0;
    //virtual payment_container export_payments() const = 0;
    //virtual void import_payments(const payment_container &payments) = 0;
    //virtual void import_payments_out(const std::list<std::pair<crypto::hash,wallet2_base::confirmed_transfer_details>> &confirmed_payments) = 0;
    virtual std::tuple<size_t, crypto::hash, std::vector<crypto::hash>> export_blockchain() const = 0;
    virtual void import_blockchain(const std::tuple<size_t, crypto::hash, std::vector<crypto::hash>> &bc) = 0;
    virtual bool export_key_images(const std::string &filename, bool all = false) const = 0;
    virtual std::pair<uint64_t, std::vector<std::pair<crypto::key_image, crypto::signature>>> export_key_images(bool all = false) const = 0;
    virtual uint64_t import_key_images(const std::vector<std::pair<crypto::key_image, crypto::signature>> &signed_key_images, size_t offset, uint64_t &spent, uint64_t &unspent, bool check_spent = true) = 0;
    virtual uint64_t import_key_images(const std::string &filename, uint64_t &spent, uint64_t &unspent) = 0;
    virtual bool import_key_images(std::vector<crypto::key_image> key_images, size_t offset=0, boost::optional<std::unordered_set<size_t>> selected_transfers=boost::none) = 0;
    //virtual bool import_key_images(signed_tx_set & signed_tx, size_t offset=0, bool only_selected_transfers=false) = 0;
    //virtual crypto::public_key get_tx_pub_key_from_received_outs(const tools::wallet2_base::transfer_details &td) const = 0;

    virtual void update_pool_state(std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &process_txs, bool refreshed = false) = 0;
    virtual void process_pool_state(const std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &txs) = 0;
    virtual void remove_obsolete_pool_txs(const std::vector<crypto::hash> &tx_hashes) = 0;

    virtual std::string encrypt(const char *plaintext, size_t len, const crypto::secret_key &skey, bool authenticated = true) const = 0;
    virtual std::string encrypt(const epee::span<char> &span, const crypto::secret_key &skey, bool authenticated = true) const = 0;
    virtual std::string encrypt(const std::string &plaintext, const crypto::secret_key &skey, bool authenticated = true) const = 0;
    virtual std::string encrypt(const epee::wipeable_string &plaintext, const crypto::secret_key &skey, bool authenticated = true) const = 0;
    virtual std::string encrypt_with_view_secret_key(const std::string &plaintext, bool authenticated = true) const = 0;
    //template<typename T=std::string> T decrypt(const std::string &ciphertext, const crypto::secret_key &skey, bool authenticated = true) const = 0;
    virtual std::string decrypt_with_view_secret_key(const std::string &ciphertext, bool authenticated = true) const = 0;

    virtual std::string make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error) const = 0;
    virtual bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error) = 0;

    virtual uint64_t get_blockchain_height_by_date(uint16_t year, uint8_t month, uint8_t day) = 0;    // 1<=month<=12, 1<=day<=31

    virtual bool is_synced() = 0;

    virtual std::vector<std::pair<uint64_t, uint64_t>> estimate_backlog(const std::vector<std::pair<double, double>> &fee_levels) = 0;
    virtual std::vector<std::pair<uint64_t, uint64_t>> estimate_backlog(uint64_t min_tx_weight, uint64_t max_tx_weight, const std::vector<uint64_t> &fees) = 0;

    virtual uint64_t estimate_fee(bool use_per_byte_fee, bool use_rct, int n_inputs, int mixin, int n_outputs, size_t extra_size, bool bulletproof, bool clsag, uint64_t base_fee, uint64_t fee_multiplier, uint64_t fee_quantization_mask) const = 0;
    virtual uint64_t get_fee_multiplier(uint32_t priority, int fee_algorithm = -1) = 0;
    virtual uint64_t get_base_fee() = 0;
    virtual uint64_t get_fee_quantization_mask() = 0;
    virtual uint64_t get_min_ring_size() = 0;
    virtual uint64_t get_max_ring_size() = 0;
    virtual uint64_t adjust_mixin(uint64_t mixin) = 0;

    virtual uint32_t adjust_priority(uint32_t priority) = 0;

    virtual bool is_unattended() const = 0;

    virtual std::pair<size_t, uint64_t> estimate_tx_size_and_weight(bool use_rct, int n_inputs, int ring_size, int n_outputs, size_t extra_size) = 0;

    virtual bool get_rpc_payment_info(bool mining, bool &payment_required, uint64_t &credits, uint64_t &diff, uint64_t &credits_per_hash_found, cryptonote::blobdata &hashing_blob, uint64_t &height, uint64_t &seed_height, crypto::hash &seed_hash, crypto::hash &next_seed_hash, uint32_t &cookie) = 0;
    virtual bool daemon_requires_payment() = 0;
    virtual bool make_rpc_payment(uint32_t nonce, uint32_t cookie, uint64_t &credits, uint64_t &balance) = 0;
    virtual bool search_for_rpc_payment(uint64_t credits_target, uint32_t n_threads, const std::function<bool(uint64_t, uint64_t)> &startfunc, const std::function<bool(unsigned)> &contfunc, const std::function<bool(uint64_t)> &foundfunc = NULL, const std::function<void(const std::string&)> &errorfunc = NULL) = 0;
    //template<typename T> void handle_payment_changes(const T &res, std::true_type) = 0;
    //template<typename T> void handle_payment_changes(const T &res, std::false_type) {}

    // Light wallet specific functions
    // fetch unspent outs from lw node and store in m_transfers
    virtual void light_wallet_get_unspent_outs() = 0;
    // fetch txs and store in m_payments
    virtual void light_wallet_get_address_txs() = 0;
    // get_address_info
    virtual bool light_wallet_get_address_info(tools::COMMAND_RPC_GET_ADDRESS_INFO::response &response) = 0;
    // Login. new_address is true if address hasn't been used on lw node before.
    virtual bool light_wallet_login(bool &new_address) = 0;
    // Send an import request to lw node. returns info about import fee, address and payment_id
    virtual bool light_wallet_import_wallet_request(tools::COMMAND_RPC_IMPORT_WALLET_REQUEST::response &response) = 0;
    // get random outputs from light wallet server
    //virtual void light_wallet_get_outs(std::vector<std::vector<get_outs_entry>> &outs, const std::vector<size_t> &selected_transfers, size_t fake_outputs_count) = 0;
    // Parse rct string
    virtual bool light_wallet_parse_rct_str(const std::string& rct_string, const crypto::public_key& tx_pub_key, uint64_t internal_output_index, rct::key& decrypted_mask, rct::key& rct_commit, bool decrypt) const = 0;
    // check if key image is ours
    virtual bool light_wallet_key_image_is_ours(const crypto::key_image& key_image, const crypto::public_key& tx_public_key, uint64_t out_index) = 0;

    /*
     * "attributes" are a mechanism to store an arbitrary number of string values
     * on the level of the wallet as a whole, identified by keys. Their introduction,
     * technically the unordered map m_attributes stored as part of a wallet file,
     * led to a new wallet file version, but now new singular pieces of info may be added
     * without the need for a new version.
     *
     * The first and so far only value stored as such an attribute is the description.
     * It's stored under the standard key ATTRIBUTE_DESCRIPTION (see method set_description).
     *
     * The mechanism is open to all clients and allows them to use it for storing basically any
     * single string values in a wallet. To avoid the problem that different clients possibly
     * overwrite or misunderstand each other's attributes, a two-part key scheme is
     * proposed: <client name>.<value name>
     */
    const char* const ATTRIBUTE_DESCRIPTION = "wallet2_base.description";
    virtual void set_attribute(const std::string &key, const std::string &value) = 0;
    virtual bool get_attribute(const std::string &key, std::string &value) const = 0;

    virtual crypto::public_key get_multisig_signer_public_key(const crypto::secret_key &spend_skey) const = 0;
    virtual crypto::public_key get_multisig_signer_public_key() const = 0;
    virtual crypto::public_key get_multisig_signing_public_key(size_t idx) const = 0;
    virtual crypto::public_key get_multisig_signing_public_key(const crypto::secret_key &skey) const = 0;

    //template<class t_request, class t_response>
    //bool invoke_http_json(const boost::string_ref uri, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST") = 0;
    //template<class t_request, class t_response>
    //bool invoke_http_bin(const boost::string_ref uri, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST") = 0;
    //template<class t_request, class t_response>
    //bool invoke_http_json_rpc(const boost::string_ref uri, const std::string& method_name, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST", const std::string& req_id = "0") = 0;
    virtual bool set_ring_database(const std::string &filename) = 0;
    virtual const std::string get_ring_database() const = 0;
    virtual bool get_ring(const crypto::key_image &key_image, std::vector<uint64_t> &outs) = 0;
    virtual bool get_rings(const crypto::hash &txid, std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> &outs) = 0;
    virtual bool set_ring(const crypto::key_image &key_image, const std::vector<uint64_t> &outs, bool relative) = 0;
    virtual bool unset_ring(const std::vector<crypto::key_image> &key_images) = 0;
    virtual bool unset_ring(const crypto::hash &txid) = 0;
    virtual bool find_and_save_rings(bool force = true) = 0;

    virtual bool blackball_output(const std::pair<uint64_t, uint64_t> &output) = 0;
    virtual bool set_blackballed_outputs(const std::vector<std::pair<uint64_t, uint64_t>> &outputs, bool add = false) = 0;
    virtual bool unblackball_output(const std::pair<uint64_t, uint64_t> &output) = 0;
    virtual bool is_output_blackballed(const std::pair<uint64_t, uint64_t> &output) const = 0;

    virtual void freeze(size_t idx) = 0;
    virtual void thaw(size_t idx) = 0;
    virtual bool frozen(size_t idx) const = 0;
    virtual void freeze(const crypto::key_image &ki) = 0;
    virtual void thaw(const crypto::key_image &ki) = 0;
    virtual bool frozen(const crypto::key_image &ki) const = 0;
    //virtual bool frozen(const transfer_details &td) const = 0;

    virtual bool save_to_file(const std::string& path_to_file, const std::string& binary, bool is_printable = false) const = 0;
    static bool load_from_file(const std::string& path_to_file, std::string& target_str, size_t max_size = 1000000000);

    virtual uint64_t get_bytes_sent() const = 0;
    virtual uint64_t get_bytes_received() const = 0;

    // MMS -------------------------------------------------------------------------------------------------
    virtual mms::message_store& get_message_store() = 0;
    virtual const mms::message_store& get_message_store() const = 0;
    virtual mms::multisig_wallet_state get_multisig_wallet_state() const = 0;

    virtual bool lock_keys_file() = 0;
    virtual bool unlock_keys_file() = 0;
    virtual bool is_keys_file_locked() const = 0;

    virtual void change_password(const std::string &filename, const epee::wipeable_string &original_password, const epee::wipeable_string &new_password) = 0;

    virtual void set_tx_notify(const std::shared_ptr<tools::Notify> &notify) = 0;

    virtual bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height) = 0;
    //virtual void hash_m_transfer(const transfer_details & transfer, crypto::hash &hash) const = 0;
    virtual uint64_t hash_m_transfers(boost::optional<uint64_t> transfer_height, crypto::hash &hash) const = 0;
    virtual void finish_rescan_bc_keep_key_images(uint64_t transfer_height, const crypto::hash &hash) = 0;
    virtual void enable_dns(bool enable) = 0;
    virtual void set_offline(bool offline = true) = 0;
    virtual bool is_offline() const = 0;

    virtual uint64_t credits() const = 0;
    virtual void credit_report(uint64_t &expected_spent, uint64_t &discrepancy) const = 0;

    static std::string get_default_daemon_address();

  private:
  };
}

