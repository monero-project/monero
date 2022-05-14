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

/*!
 * \file simplewallet.h
 * 
 * \brief Header file that declares simple_wallet class.
 */
#pragma once

#include <memory>

#include <boost/optional/optional.hpp>
#include <boost/program_options/variables_map.hpp>

#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "wallet/wallet2.h"
#include "console_handler.h"
#include "math_helper.h"
#include "wipeable_string.h"
#include "common/i18n.h"
#include "common/password.h"
#include "crypto/crypto.h"  // for definition of crypto::secret_key

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.simplewallet"
// Hardcode Monero's donation address (see #1447)
constexpr const char MONERO_DONATION_ADDR[] = "888tNkZrPN6JsEgekjMnABU4TBzc2Dt29EPAvkRxbANsAnjyPbb3iQ1YBRk1UXcdRsiKc9dhwMVgN5S9cQUiyoogDavup3H";

/*!
 * \namespace cryptonote
 * \brief Holds cryptonote related classes and helpers.
 */
namespace cryptonote
{
  /*!
   * \brief Manages wallet operations. This is the most abstracted wallet class.
   */
  class simple_wallet : public tools::i_wallet2_callback
  {
  public:
    static const char *tr(const char *str) { return i18n_translate(str, "cryptonote::simple_wallet"); }

  public:
    typedef std::vector<std::string> command_type;

    simple_wallet();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    bool run();
    void stop();
    void interrupt();

    //wallet *create_wallet();
    bool process_command(const std::vector<std::string> &args);
    std::string get_commands_str();
    std::string get_command_usage(const std::vector<std::string> &args);
  private:

    enum ResetType { ResetNone, ResetSoft, ResetHard, ResetSoftKeepKI };

    bool handle_command_line(const boost::program_options::variables_map& vm);

    bool run_console_handler();

    void wallet_idle_thread();

    //! \return Prompts user for password and verifies against local file. Logs on error and returns `none`
    boost::optional<tools::password_container> get_and_verify_password() const;

    boost::optional<epee::wipeable_string> new_wallet(const boost::program_options::variables_map& vm, const crypto::secret_key& recovery_key,
        bool recover, bool two_random, const std::string &old_language);
    boost::optional<epee::wipeable_string> new_wallet(const boost::program_options::variables_map& vm, const cryptonote::account_public_address& address,
        const boost::optional<crypto::secret_key>& spendkey, const crypto::secret_key& viewkey);
    boost::optional<epee::wipeable_string> new_wallet(const boost::program_options::variables_map& vm,
        const epee::wipeable_string &multisig_keys, const epee::wipeable_string &seed_pass, const std::string &old_language);
    boost::optional<epee::wipeable_string> new_wallet(const boost::program_options::variables_map& vm);
    boost::optional<epee::wipeable_string> open_wallet(const boost::program_options::variables_map& vm);
    bool close_wallet();

    bool viewkey(const std::vector<std::string> &args = std::vector<std::string>());
    bool spendkey(const std::vector<std::string> &args = std::vector<std::string>());
    bool seed(const std::vector<std::string> &args = std::vector<std::string>());
    bool encrypted_seed(const std::vector<std::string> &args = std::vector<std::string>());
    bool restore_height(const std::vector<std::string> &args = std::vector<std::string>());

    /*!
     * \brief Sets seed language.
     *
     * interactive
     *   - prompts for password so wallet can be rewritten
     *   - calls get_mnemonic_language() which prompts for language
     *
     * \return success status
     */
    bool seed_set_language(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_always_confirm_transfers(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_print_ring_members(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_store_tx_info(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_auto_refresh(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_refresh_type(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_missing_payment_id(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_ask_password(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_unit(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_max_reorg_depth(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_min_output_count(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_min_output_value(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_merge_destinations(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_backlog(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_backlog_threshold(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_export_overwrite(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_refresh_from_block_height(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_auto_low_priority(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_segregate_pre_fork_outputs(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_key_reuse_mitigation2(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_subaddress_lookahead(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_segregation_height(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_ignore_fractional_outputs(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_ignore_outputs_above(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_ignore_outputs_below(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_track_uses(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_show_wallet_name_when_locked(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_inactivity_lock_timeout(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_setup_background_mining(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_device_name(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_export_format(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_load_deprecated_formats(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_enable_multisig(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_persistent_rpc_client_id(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_auto_mine_for_rpc_payment_threshold(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_credits_target(const std::vector<std::string> &args = std::vector<std::string>());
    bool help(const std::vector<std::string> &args = std::vector<std::string>());
    bool apropos(const std::vector<std::string> &args);
    bool scan_tx(const std::vector<std::string> &args);
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool set_daemon(const std::vector<std::string> &args);
    bool save_bc(const std::vector<std::string> &args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance_unlocked(bool detailed = false);
    bool show_balance(const std::vector<std::string> &args = std::vector<std::string>());
    bool show_incoming_transfers(const std::vector<std::string> &args);
    bool show_payments(const std::vector<std::string> &args);
    bool show_blockchain_height(const std::vector<std::string> &args);
    bool transfer_main(int transfer_type, const std::vector<std::string> &args, bool called_by_mms);
    bool transfer(const std::vector<std::string> &args);
    bool locked_transfer(const std::vector<std::string> &args);
    bool locked_sweep_all(const std::vector<std::string> &args);
    bool sweep_main(uint32_t account, uint64_t below, bool locked, const std::vector<std::string> &args);
    bool sweep_all(const std::vector<std::string> &args);
    bool sweep_account(const std::vector<std::string> &args);
    bool sweep_below(const std::vector<std::string> &args);
    bool sweep_single(const std::vector<std::string> &args);
    bool sweep_unmixable(const std::vector<std::string> &args);
    bool donate(const std::vector<std::string> &args);
    bool sign_transfer(const std::vector<std::string> &args);
    bool submit_transfer(const std::vector<std::string> &args);
    std::vector<std::vector<cryptonote::tx_destination_entry>> split_amounts(
        std::vector<cryptonote::tx_destination_entry> dsts, size_t num_splits
    );
    bool account(const std::vector<std::string> &args = std::vector<std::string>());
    void print_accounts();
    void print_accounts(const std::string& tag);
    bool print_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool print_integrated_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool address_book(const std::vector<std::string> &args = std::vector<std::string>());
    bool save(const std::vector<std::string> &args);
    bool save_watch_only(const std::vector<std::string> &args);
    bool set_variable(const std::vector<std::string> &args);
    bool rescan_spent(const std::vector<std::string> &args);
    bool set_log(const std::vector<std::string> &args);
    bool get_tx_key(const std::vector<std::string> &args);
    bool set_tx_key(const std::vector<std::string> &args);
    bool check_tx_key(const std::vector<std::string> &args);
    bool get_tx_proof(const std::vector<std::string> &args);
    bool check_tx_proof(const std::vector<std::string> &args);
    bool get_spend_proof(const std::vector<std::string> &args);
    bool check_spend_proof(const std::vector<std::string> &args);
    bool get_reserve_proof(const std::vector<std::string> &args);
    bool check_reserve_proof(const std::vector<std::string> &args);
    bool show_transfers(const std::vector<std::string> &args);
    bool export_transfers(const std::vector<std::string> &args);
    bool unspent_outputs(const std::vector<std::string> &args);
    bool rescan_blockchain(const std::vector<std::string> &args);
    bool refresh_main(uint64_t start_height, ResetType reset, bool is_init = false);
    bool set_tx_note(const std::vector<std::string> &args);
    bool get_tx_note(const std::vector<std::string> &args);
    bool set_description(const std::vector<std::string> &args);
    bool get_description(const std::vector<std::string> &args);
    bool status(const std::vector<std::string> &args);
    bool wallet_info(const std::vector<std::string> &args);
    bool set_default_priority(const std::vector<std::string> &args);
    bool sign(const std::vector<std::string> &args);
    bool verify(const std::vector<std::string> &args);
    bool export_key_images(const std::vector<std::string> &args);
    bool import_key_images(const std::vector<std::string> &args);
    bool hw_key_images_sync(const std::vector<std::string> &args);
    bool hw_reconnect(const std::vector<std::string> &args);
    bool export_outputs(const std::vector<std::string> &args);
    bool import_outputs(const std::vector<std::string> &args);
    bool show_transfer(const std::vector<std::string> &args);
    bool change_password(const std::vector<std::string>& args);
    bool payment_id(const std::vector<std::string> &args);
    bool print_fee_info(const std::vector<std::string> &args);
    bool prepare_multisig(const std::vector<std::string>& args);
    bool prepare_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool make_multisig(const std::vector<std::string>& args);
    bool make_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool exchange_multisig_keys(const std::vector<std::string> &args);
    bool exchange_multisig_keys_main(const std::vector<std::string> &args, const bool force_update_use_with_caution, const bool called_by_mms);
    bool export_multisig(const std::vector<std::string>& args);
    bool export_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool import_multisig(const std::vector<std::string>& args);
    bool import_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool accept_loaded_tx(const tools::wallet2::multisig_tx_set &txs);
    bool sign_multisig(const std::vector<std::string>& args);
    bool sign_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool submit_multisig(const std::vector<std::string>& args);
    bool submit_multisig_main(const std::vector<std::string>& args, bool called_by_mms);
    bool export_raw_multisig(const std::vector<std::string>& args);
    bool mms(const std::vector<std::string>& args);
    bool print_ring(const std::vector<std::string>& args);
    bool set_ring(const std::vector<std::string>& args);
    bool unset_ring(const std::vector<std::string>& args);
    bool save_known_rings(const std::vector<std::string>& args);
    bool blackball(const std::vector<std::string>& args);
    bool unblackball(const std::vector<std::string>& args);
    bool blackballed(const std::vector<std::string>& args);
    bool freeze(const std::vector<std::string>& args);
    bool thaw(const std::vector<std::string>& args);
    bool frozen(const std::vector<std::string>& args);
    bool lock(const std::vector<std::string>& args);
    bool rpc_payment_info(const std::vector<std::string> &args);
    bool start_mining_for_rpc(const std::vector<std::string> &args);
    bool stop_mining_for_rpc(const std::vector<std::string> &args);
    bool show_qr_code(const std::vector<std::string> &args);
    bool net_stats(const std::vector<std::string>& args);
    bool public_nodes(const std::vector<std::string>& args);
    bool welcome(const std::vector<std::string>& args);
    bool version(const std::vector<std::string>& args);
    bool on_unknown_command(const std::vector<std::string>& args);

    bool cold_sign_tx(const std::vector<tools::wallet2::pending_tx>& ptx_vector, tools::wallet2::signed_tx_set &exported_txs, std::vector<cryptonote::address_parse_info> &dsts_info, std::function<bool(const tools::wallet2::signed_tx_set &)> accept_func);
    uint64_t get_daemon_blockchain_height(std::string& err);
    bool try_connect_to_daemon(bool silent = false, uint32_t* version = nullptr);
    bool ask_wallet_create_if_needed();
    bool accept_loaded_tx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message = std::string());
    bool accept_loaded_tx(const tools::wallet2::unsigned_tx_set &txs);
    bool accept_loaded_tx(const tools::wallet2::signed_tx_set &txs);
    bool process_ring_members(const std::vector<tools::wallet2::pending_tx>& ptx_vector, std::ostream& ostr, bool verbose);
    std::string get_prompt() const;
    bool print_seed(bool encrypted);
    void key_images_sync_intern();
    void on_refresh_finished(uint64_t start_height, uint64_t fetched_blocks, bool is_init, bool received_money);
    std::pair<std::string, std::string> show_outputs_line(const std::vector<uint64_t> &heights, uint64_t blockchain_height, uint64_t highlight_idx = std::numeric_limits<uint64_t>::max()) const;
    bool freeze_thaw(const std::vector<std::string>& args, bool freeze);
    bool prompt_if_old(const std::vector<tools::wallet2::pending_tx> &ptx_vector);
    bool on_command(bool (simple_wallet::*cmd)(const std::vector<std::string>&), const std::vector<std::string> &args);
    bool on_empty_command();
    bool on_cancelled_command();
    void check_for_inactivity_lock(bool user);

    struct transfer_view
    {
      std::string type;
      boost::variant<uint64_t, std::string> block;
      uint64_t timestamp;
      std::string direction;
      bool confirmed;
      uint64_t amount;
      crypto::hash hash;
      std::string payment_id;
      uint64_t fee;
      std::vector<std::pair<std::string, uint64_t>> outputs;
      std::set<uint32_t> index;
      std::string note;
      std::string unlocked;
    };
    bool get_transfers(std::vector<std::string>& args_, std::vector<transfer_view>& transfers);

    /*!
     * \brief Prints the seed with a nice message
     * \param seed seed to print
     */
    void print_seed(const epee::wipeable_string &seed);

    /*!
     * \brief Gets the word seed language from the user.
     * 
     * User is asked to choose from a list of supported languages.
     * 
     * \return The chosen language.
     */
    std::string get_mnemonic_language();

    /*!
     * \brief When --do-not-relay option is specified, save the raw tx hex blob to a file instead of calling m_wallet->commit_tx(ptx).
     * \param ptx_vector Pending tx(es) created by transfer/sweep_all
     */
    void commit_or_save(std::vector<tools::wallet2::pending_tx>& ptx_vector, bool do_not_relay);

    /*!
     * \brief checks whether background mining is enabled, and asks to configure it if not
     */
    void check_background_mining(const epee::wipeable_string &password);
    void start_background_mining();
    void stop_background_mining();

    // idle thread workers
    bool check_inactivity();
    bool check_refresh();
    bool check_mms();
    bool check_rpc_payment();

    void handle_transfer_exception(const std::exception_ptr &e, bool trusted_daemon);

    bool check_daemon_rpc_prices(const std::string &daemon_url, uint32_t &actual_cph, uint32_t &claimed_cph);

    //----------------- i_wallet2_callback ---------------------
    virtual void on_new_block(uint64_t height, const cryptonote::block& block);
    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, uint64_t burnt, const cryptonote::subaddress_index& subaddr_index, bool is_change, uint64_t unlock_time);
    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index);
    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index);
    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx);
    virtual boost::optional<epee::wipeable_string> on_get_password(const char *reason);
    virtual void on_device_button_request(uint64_t code);
    virtual boost::optional<epee::wipeable_string> on_device_pin_request();
    virtual boost::optional<epee::wipeable_string> on_device_passphrase_request(bool & on_device);
    //----------------------------------------------------------

    friend class refresh_progress_reporter_t;

    class refresh_progress_reporter_t
    {
    public:
      refresh_progress_reporter_t(cryptonote::simple_wallet& simple_wallet)
        : m_simple_wallet(simple_wallet)
        , m_blockchain_height(0)
        , m_blockchain_height_update_time()
        , m_print_time()
      {
      }

      void update(uint64_t height, bool force = false)
      {
        auto current_time = std::chrono::system_clock::now();
        const auto node_update_threshold = std::chrono::seconds(DIFFICULTY_TARGET_V1 / 2); // use min of V1/V2
        if (node_update_threshold < current_time - m_blockchain_height_update_time || m_blockchain_height <= height)
        {
          update_blockchain_height();
          m_blockchain_height = (std::max)(m_blockchain_height, height);
        }

        if (std::chrono::milliseconds(20) < current_time - m_print_time || force)
        {
          std::cout << QT_TRANSLATE_NOOP("cryptonote::simple_wallet", "Height ") << height << " / " << m_blockchain_height << '\r' << std::flush;
          m_print_time = current_time;
        }
      }

    private:
      void update_blockchain_height()
      {
        std::string err;
        uint64_t blockchain_height = m_simple_wallet.get_daemon_blockchain_height(err);
        if (err.empty())
        {
          m_blockchain_height = blockchain_height;
          m_blockchain_height_update_time = std::chrono::system_clock::now();
        }
        else
        {
          LOG_ERROR("Failed to get current blockchain height: " << err);
        }
      }

    private:
      cryptonote::simple_wallet& m_simple_wallet;
      uint64_t m_blockchain_height;
      std::chrono::system_clock::time_point m_blockchain_height_update_time;
      std::chrono::system_clock::time_point m_print_time;
    };

  private:
    std::string m_wallet_file;
    std::string m_generate_new;
    std::string m_generate_from_device;
    std::string m_generate_from_view_key;
    std::string m_generate_from_spend_key;
    std::string m_generate_from_keys;
    std::string m_generate_from_multisig_keys;
    std::string m_generate_from_json;
    std::string m_mnemonic_language;
    std::string m_import_path;
    std::string m_subaddress_lookahead;
    std::string m_restore_date;  // optional - converted to m_restore_height

    epee::wipeable_string m_electrum_seed;  // electrum-style seed parameter

    crypto::secret_key m_recovery_key;  // recovery key (used as random for wallet gen)
    bool m_restore_deterministic_wallet;  // recover flag
    bool m_restore_multisig_wallet;  // recover flag
    bool m_non_deterministic;  // old 2-random generation
    bool m_restoring;           // are we restoring, by whatever method?
    uint64_t m_restore_height;  // optional
    bool m_do_not_relay;
    bool m_use_english_language_names;

    epee::console_handlers_binder m_cmd_binder;

    std::unique_ptr<tools::wallet2> m_wallet;
    refresh_progress_reporter_t m_refresh_progress_reporter;

    std::atomic<bool> m_idle_run;
    boost::thread m_idle_thread;
    boost::mutex m_idle_mutex;
    boost::condition_variable m_idle_cond;

    std::atomic<bool> m_auto_refresh_enabled;
    bool m_auto_refresh_refreshing;
    std::atomic<bool> m_in_manual_refresh;
    uint32_t m_current_subaddress_account;

    std::atomic<time_t> m_last_activity_time;
    std::atomic<bool> m_locked;
    std::atomic<bool> m_in_command;

    template<uint64_t mini, uint64_t maxi> struct get_random_interval { public: uint64_t operator()() const { return crypto::rand_range(mini, maxi); } };

    epee::math_helper::once_a_time_seconds<1> m_inactivity_checker;
    epee::math_helper::once_a_time_seconds_range<get_random_interval<80 * 1000000, 100 * 1000000>> m_refresh_checker;
    epee::math_helper::once_a_time_seconds_range<get_random_interval<90 * 1000000, 110 * 1000000>> m_mms_checker;
    epee::math_helper::once_a_time_seconds_range<get_random_interval<90 * 1000000, 115 * 1000000>> m_rpc_payment_checker;
    
    std::atomic<bool> m_need_payment;
    boost::posix_time::ptime m_last_rpc_payment_mining_time;
    bool m_rpc_payment_mining_requested;
    uint32_t m_rpc_payment_threads = 0;
    bool m_daemon_rpc_payment_message_displayed;
    float m_rpc_payment_hash_rate;
    std::atomic<bool> m_suspend_rpc_payment_mining;

    std::unordered_map<std::string, uint32_t> m_claimed_cph;

    // MMS
    mms::message_store& get_message_store() const { return m_wallet->get_message_store(); };
    mms::multisig_wallet_state get_multisig_wallet_state() const { return m_wallet->get_multisig_wallet_state(); };
    bool mms_active() const { return get_message_store().get_active(); };
    bool choose_mms_processing(const std::vector<mms::processing_data> &data_list, uint32_t &choice);
    void list_mms_messages(const std::vector<mms::message> &messages);
    void list_signers(const std::vector<mms::authorized_signer> &signers);
    void add_signer_config_messages();
    void show_message(const mms::message &m);
    void ask_send_all_ready_messages();
    void check_for_messages();
    bool user_confirms(const std::string &question);
    bool user_confirms_auto_config();
    bool get_message_from_arg(const std::string &arg, mms::message &m);
    bool get_number_from_arg(const std::string &arg, uint32_t &number, const uint32_t lower_bound, const uint32_t upper_bound); 

    void mms_init(const std::vector<std::string> &args);
    void mms_info(const std::vector<std::string> &args);
    void mms_signer(const std::vector<std::string> &args);
    void mms_list(const std::vector<std::string> &args);
    void mms_next(const std::vector<std::string> &args);
    void mms_sync(const std::vector<std::string> &args);
    void mms_transfer(const std::vector<std::string> &args);
    void mms_delete(const std::vector<std::string> &args);
    void mms_send(const std::vector<std::string> &args);
    void mms_receive(const std::vector<std::string> &args);
    void mms_export(const std::vector<std::string> &args);
    void mms_note(const std::vector<std::string> &args);
    void mms_show(const std::vector<std::string> &args);
    void mms_set(const std::vector<std::string> &args);
    void mms_help(const std::vector<std::string> &args);
    void mms_send_signer_config(const std::vector<std::string> &args);
    void mms_start_auto_config(const std::vector<std::string> &args);
    void mms_config_checksum(const std::vector<std::string> &args);
    void mms_stop_auto_config(const std::vector<std::string> &args);
    void mms_auto_config(const std::vector<std::string> &args);
  };
}
