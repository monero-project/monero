// Copyright (c) 2014, The Monero Project
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

#include <boost/program_options/variables_map.hpp>

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "wallet/wallet2.h"
#include "console_handler.h"
#include "password_container.h"
#include "crypto/crypto.h"  // for definition of crypto::secret_key


namespace cryptonote
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class simple_wallet : public tools::i_wallet2_callback
  {
  public:
    typedef std::vector<std::string> command_type;

    simple_wallet();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    bool run();
    void stop();

    //wallet *create_wallet();
    bool process_command(const std::vector<std::string> &args);
    std::string get_commands_str();
  private:
    void handle_command_line(const boost::program_options::variables_map& vm);

    bool run_console_handler();

    bool new_wallet(const std::string &wallet_file, const std::string& password, const crypto::secret_key& recovery_key = crypto::secret_key(), bool recover = false, bool two_random = false);
    bool open_wallet(const std::string &wallet_file, const std::string& password);
    bool close_wallet();

    bool seed(const std::vector<std::string> &args = std::vector<std::string>());
    bool help(const std::vector<std::string> &args = std::vector<std::string>());
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool save_bc(const std::vector<std::string>& args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance(const std::vector<std::string> &args = std::vector<std::string>());
    bool show_incoming_transfers(const std::vector<std::string> &args);
    bool show_payments(const std::vector<std::string> &args);
    bool show_blockchain_height(const std::vector<std::string> &args);
    bool transfer(const std::vector<std::string> &args);
    std::vector<std::vector<cryptonote::tx_destination_entry>> split_amounts(
        std::vector<cryptonote::tx_destination_entry> dsts, size_t num_splits
    );
    bool print_address(const std::vector<std::string> &args = std::vector<std::string>());
    bool save(const std::vector<std::string> &args);
    bool set_log(const std::vector<std::string> &args);

    uint64_t get_daemon_blockchain_height(std::string& err);
    bool try_connect_to_daemon();
    bool ask_wallet_create_if_needed();

    //----------------- i_wallet2_callback ---------------------
    virtual void on_new_block(uint64_t height, const cryptonote::block& block);
    virtual void on_money_received(uint64_t height, const cryptonote::transaction& tx, size_t out_index);
    virtual void on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx);
    virtual void on_skip_transaction(uint64_t height, const cryptonote::transaction& tx);
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
        if (std::chrono::seconds(DIFFICULTY_TARGET / 2) < current_time - m_blockchain_height_update_time || m_blockchain_height <= height)
        {
          update_blockchain_height();
          m_blockchain_height = (std::max)(m_blockchain_height, height);
        }

        if (std::chrono::milliseconds(1) < current_time - m_print_time || force)
        {
          std::cout << "Height " << height << " of " << m_blockchain_height << '\r';
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
    std::string m_import_path;

    std::string m_electrum_seed;  // electrum-style seed parameter

    crypto::secret_key m_recovery_key;  // recovery key (used as random for wallet gen)
    bool m_restore_deterministic_wallet;  // recover flag
    bool m_non_deterministic;  // old 2-random generation

    std::string m_daemon_address;
    std::string m_daemon_host;
    int m_daemon_port;

    epee::console_handlers_binder m_cmd_binder;

    std::unique_ptr<tools::wallet2> m_wallet;
    epee::net_utils::http::http_simple_client m_http_client;
    refresh_progress_reporter_t m_refresh_progress_reporter;
  };
}
