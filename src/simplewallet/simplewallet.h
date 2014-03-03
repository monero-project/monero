// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>

#include <boost/program_options/variables_map.hpp>

#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "wallet/wallet2.h"
#include "console_handler.h"
#include "password_container.h"


namespace cryptonote
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class simple_wallet
  {
  public:
    typedef std::vector<std::string> command_type;

    simple_wallet();
    bool init(const boost::program_options::variables_map& vm);
    bool deinit();
    bool run();

    //wallet *create_wallet();
    bool process_command(const std::vector<std::string> &args);
    std::string get_commands_str();
  private:
    bool handle_command_line(const boost::program_options::variables_map& vm);

    bool run_console_handler();

    bool new_wallet(const std::string &wallet_file, const std::string& password);
    bool open_wallet(const std::string &wallet_file, const std::string& password);
    bool close_wallet();

    bool help(const std::vector<std::string> &args);
    bool start_mining(const std::vector<std::string> &args);
    bool stop_mining(const std::vector<std::string> &args);
    bool refresh(const std::vector<std::string> &args);
    bool show_balance(const std::vector<std::string> &args);
    bool show_incoming_transfers(const std::vector<std::string> &args);
    bool show_blockchain_height(const std::vector<std::string> &args);
    bool transfer(const std::vector<std::string> &args);
    bool print_address(const std::vector<std::string> &args);
    bool save(const std::vector<std::string> &args);
    bool set_log(const std::vector<std::string> &args);

    uint64_t get_daemon_blockchain_height(bool& ok);
    void try_connect_to_daemon();

    std::string m_wallet_file;
    std::string m_generate_new;
    std::string m_import_path;

    std::string m_daemon_address;
    std::string m_daemon_host;
    int m_daemon_port;
    bool m_tried_to_connect;

    epee::console_handlers_binder m_cmd_binder;

    std::auto_ptr<tools::wallet2> m_wallet;
    net_utils::http::http_simple_client m_http_client;
  };
}
