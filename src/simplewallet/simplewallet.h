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
#include "common/password.h"
#include "crypto/crypto.h"  // for definition of crypto::secret_key

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.simplewallet"

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
  private:
    bool handle_command_line(const boost::program_options::variables_map& vm);

    bool run_console_handler();

    void wallet_idle_thread();

    //! \return Prompts user for password and verifies against local file. Logs on error and returns `none`
    boost::optional<tools::password_container> get_and_verify_password() const;

    bool new_wallet(const boost::program_options::variables_map& vm, const crypto::secret_key& recovery_key,
        bool recover, bool two_random, const std::string &old_language);
    bool new_wallet(const boost::program_options::variables_map& vm, const cryptonote::account_public_address& address,
        const boost::optional<crypto::secret_key>& spendkey, const crypto::secret_key& viewkey);
    bool open_wallet(const boost::program_options::variables_map& vm);
    bool close_wallet();

    bool viewkey(const std::vector<std::string> &args = std::vector<std::string>());
    bool spendkey(const std::vector<std::string> &args = std::vector<std::string>());
    bool seed(const std::vector<std::string> &args = std::vector<std::string>());

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
    bool set_default_ring_size(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_auto_refresh(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_refresh_type(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_missing_payment_id(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_ask_password(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_unit(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_min_output_count(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_min_output_value(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_merge_destinations(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_confirm_backlog(const std::vector<std::string> &args = std::vector<std::string>());
    bool set_refresh_from_block_height(const std::vector<std::string> &args = std::vector<std::string>());
    bool help(const std::vector<std::string> &args = std::vector<std::string>());
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool save_bc(const std::vector<std::string>& args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance_unlocked();
    bool show_balance(const std::vector<std::string> &args = std::vector<std::string>());
    bool show_incoming_transfers(const std::vector<std::string> &args);
    bool show_payments(const std::vector<std::string> &args);
    bool show_blockchain_height(const std::vector<std::string> &args);
    bool transfer_main(int transfer_type, const std::vector<std::string> &args);
    bool transfer(const std::vector<std::string> &args);
    bool transfer_new(const std::vector<std::string> &args);
    bool locked_transfer(const std::vector<std::string> &args);
    bool sweep_main(uint64_t below, const std::vector<std::string> &args);
    bool sweep_all(const std::vector<std::string> &args);
    bool sweep_below(const std::vector<std::string> &args);
    bool sweep_unmixable(const std::vector<std::string> &args);
    bool donate(const std::vector<std::string> &args);
    bool sign_transfer(const std::vector<std::string> &args);
    bool submit_transfer(const std::vector<std::string> &args);
    std::vector<std::vector<cryptonote::tx_destination_entry>> split_amounts(
        std::vector<cryptonote::tx_destination_entry> dsts, size_t num_splits
    );
    bool print_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool print_integrated_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool address_book(const std::vector<std::string> &args = std::vector<std::string>());
    bool save(const std::vector<std::string> &args);
    bool save_watch_only(const std::vector<std::string> &args);
    bool set_variable(const std::vector<std::string> &args);
    bool rescan_spent(const std::vector<std::string> &args);
    bool set_log(const std::vector<std::string> &args);
    bool get_tx_key(const std::vector<std::string> &args);
    bool check_tx_key(const std::vector<std::string> &args);
    bool check_tx_key_helper(const crypto::hash &txid, const cryptonote::account_public_address &address, const crypto::key_derivation &derivation);
    bool get_tx_proof(const std::vector<std::string> &args);
    bool check_tx_proof(const std::vector<std::string> &args);
    bool show_transfers(const std::vector<std::string> &args);
    bool unspent_outputs(const std::vector<std::string> &args);
    bool rescan_blockchain(const std::vector<std::string> &args);
    bool refresh_main(uint64_t start_height, bool reset = false);
    bool set_tx_note(const std::vector<std::string> &args);
    bool get_tx_note(const std::vector<std::string> &args);
    bool status(const std::vector<std::string> &args);
    bool set_default_priority(const std::vector<std::string> &args);
    bool sign(const std::vector<std::string> &args);
    bool verify(const std::vector<std::string> &args);
    bool export_key_images(const std::vector<std::string> &args);
    bool import_key_images(const std::vector<std::string> &args);
    bool export_outputs(const std::vector<std::string> &args);
    bool import_outputs(const std::vector<std::string> &args);
    bool show_transfer(const std::vector<std::string> &args);
    bool change_password(const std::vector<std::string>& args);
    bool payment_id(const std::vector<std::string> &args);
    bool print_fee_info(const std::vector<std::string> &args);

    uint64_t get_daemon_blockchain_height(std::string& err);
    bool try_connect_to_daemon(bool silent = false, uint32_t* version = nullptr);
    bool ask_wallet_create_if_needed();
    bool accept_loaded_tx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message = std::string());
    bool accept_loaded_tx(const tools::wallet2::unsigned_tx_set &txs);
    bool accept_loaded_tx(const tools::wallet2::signed_tx_set &txs);
    bool print_ring_members(const std::vector<tools::wallet2::pending_tx>& ptx_vector, std::ostream& ostr);
    std::string get_prompt() const;

    /*!
     * \brief Prints the seed with a nice message
     * \param seed seed to print
     */
    void print_seed(std::string seed);

    /*!
     * \brief Gets the word seed language from the user.
     * 
     * User is asked to choose from a list of supported languages.
     * 
     * \return The chosen language.
     */
    std::string get_mnemonic_language();

    //----------------- i_wallet2_callback ---------------------
    virtual void on_new_block(uint64_t height, const cryptonote::block& block);
    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount);
    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount);
    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx);
    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx);
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
        const auto node_update_threshold = std::chrono::seconds(DIFFICULTY_TARGET / 2); // use min of V1/V2
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
    std::string m_generate_from_view_key;
    std::string m_generate_from_keys;
    std::string m_generate_from_multisig_keys;
    std::string m_generate_from_json;
    std::string m_mnemonic_language;
    std::string m_import_path;

    std::string m_electrum_seed;  // electrum-style seed parameter

    crypto::secret_key m_recovery_key;  // recovery key (used as random for wallet gen)
    bool m_restore_deterministic_wallet;  // recover flag
    bool m_non_deterministic;  // old 2-random generation
    bool m_trusted_daemon;
    bool m_allow_mismatched_daemon_version;
    bool m_restoring;           // are we restoring, by whatever method?
    uint64_t m_restore_height;  // optional

    epee::console_handlers_binder m_cmd_binder;

    std::unique_ptr<tools::wallet2> m_wallet;
    epee::net_utils::http::http_simple_client m_http_client;
    refresh_progress_reporter_t m_refresh_progress_reporter;

    std::atomic<bool> m_idle_run;
    boost::thread m_idle_thread;
    boost::mutex m_idle_mutex;
    boost::condition_variable m_idle_cond;

    std::atomic<bool> m_auto_refresh_enabled;
    bool m_auto_refresh_refreshing;
    std::atomic<bool> m_in_manual_refresh;
  };
}
