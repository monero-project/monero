// Copyright (c) 2014-2015, The Monero Project
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
 * \file simplewallet.cpp
 * 
 * \brief Source file that defines simple_wallet class.
 */

#include <thread>
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "include_base_utils.h"
#include "common/i18n.h"
#include "common/command_line.h"
#include "common/util.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "simplewallet.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "wallet/wallet_rpc_server.h"
#include "version.h"
#include "crypto/crypto.h"  // for crypto::secret_key definition
#include "mnemonics/electrum-words.h"
#include <stdexcept>

#if defined(WIN32)
#include <crtdbg.h>
#endif

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;
namespace bf = boost::filesystem;
typedef cryptonote::simple_wallet sw;

#define EXTENDED_LOGS_FILE "wallet_details.log"

unsigned int epee::g_test_dbg_lock_sleep = 0;

#define DEFAULT_MIX 4

namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", sw::tr("Use wallet <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", sw::tr("Generate new wallet and save it to <arg> or <address>.wallet by default"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_view_key = {"generate-from-view-key", sw::tr("Generate wallet from (address:viewkey:filename) and save it to <filename>"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", sw::tr("Use daemon instance at <host>:<port>"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", sw::tr("Use daemon instance at host <arg> instead of localhost"), ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", sw::tr("Wallet password"), "", true};
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", sw::tr("Specify electrum seed for wallet recovery/creation"), ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", sw::tr("Recover wallet using electrum-style mnemonic"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("creates non-deterministic view and spend keys"), false};
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", sw::tr("Use daemon instance at port <arg> instead of 8081"), 0};
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<std::string> arg_log_file = {"log-file", sw::tr("Specify log file"), ""};
  const command_line::arg_descriptor<bool> arg_testnet = {"testnet", sw::tr("Used to deploy test nets. The daemon must be launched with --testnet flag"), false};
  const command_line::arg_descriptor<bool> arg_restricted = {"restricted-rpc", sw::tr("Restricts RPC to view only commands"), false};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", sw::tr("Enable commands which rely on a trusted daemon"), false};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = sw::tr("daemon is busy. Please try later");
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = sw::tr("possible lost connection to daemon");
    }
    return err;
  }

  class message_writer
  {
  public:
    message_writer(epee::log_space::console_colors color = epee::log_space::console_color_default, bool bright = false,
      std::string&& prefix = std::string(), int log_level = LOG_LEVEL_2)
      : m_flush(true)
      , m_color(color)
      , m_bright(bright)
      , m_log_level(log_level)
    {
      m_oss << prefix;
    }

    message_writer(message_writer&& rhs)
      : m_flush(std::move(rhs.m_flush))
#if defined(_MSC_VER)
      , m_oss(std::move(rhs.m_oss))
#else
      // GCC bug: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=54316
      , m_oss(rhs.m_oss.str(), ios_base::out | ios_base::ate)
#endif
      , m_color(std::move(rhs.m_color))
      , m_log_level(std::move(rhs.m_log_level))
    {
      rhs.m_flush = false;
    }

    template<typename T>
    std::ostream& operator<<(const T& val)
    {
      m_oss << val;
      return m_oss;
    }

    ~message_writer()
    {
      if (m_flush)
      {
        m_flush = false;

        LOG_PRINT(m_oss.str(), m_log_level)

        if (epee::log_space::console_color_default == m_color)
        {
          std::cout << m_oss.str();
        }
        else
        {
          epee::log_space::set_console_color(m_color, m_bright);
          std::cout << m_oss.str();
          epee::log_space::reset_console_color();
        }
        std::cout << std::endl;
      }
    }

  private:
    message_writer(message_writer& rhs);
    message_writer& operator=(message_writer& rhs);
    message_writer& operator=(message_writer&& rhs);

  private:
    bool m_flush;
    std::stringstream m_oss;
    epee::log_space::console_colors m_color;
    bool m_bright;
    int m_log_level;
  };

  message_writer success_msg_writer(bool color = false)
  {
    return message_writer(color ? epee::log_space::console_color_green : epee::log_space::console_color_default, false, std::string(), LOG_LEVEL_2);
  }

  message_writer fail_msg_writer()
  {
    return message_writer(epee::log_space::console_color_red, true, sw::tr("Error: "), LOG_LEVEL_0);
  }

  bool is_it_true(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "true")
      return true;
    if (s == "1")
      return true;
    if (s == "y" || s == "yes")
      return true;
    if (s == sw::tr("yes"))
      return true;
    return false;
  }
}


std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << tr("Commands: ") << ENDL;
  std::string usage = m_cmd_binder.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

bool simple_wallet::viewkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // don't log
  std::cout << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << std::endl;

  return true;
}

bool simple_wallet::spendkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // don't log
  std::cout << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key) << std::endl;

  return true;
}

bool simple_wallet::seed(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success =  false;
  std::string electrum_words;

  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("This wallet is watch-only and cannot have a seed.");
    return true;
  }
  if (m_wallet->is_deterministic())
  {
    if (m_wallet->get_seed_language().empty())
    {
      std::string mnemonic_language = get_mnemonic_language();
      m_wallet->set_seed_language(mnemonic_language);
    }

    success = m_wallet->get_seed(electrum_words);
  }

  if (success) 
  {
    print_seed(electrum_words);
  }
  else
  {
    fail_msg_writer() << tr("The wallet is non-deterministic. Cannot display seed.");
  }
  return true;
}

bool simple_wallet::seed_set_language(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("This wallet is watch-only and doesn't have a seed.");
    return true;
  }
  if (!m_wallet->is_deterministic())
  {
    fail_msg_writer() << tr("This wallet is non-deterministic and doesn't have a seed.");
    return true;
  }
  tools::password_container pwd_container;
  success = pwd_container.read_password();
  if (!success)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  /* verify password before using so user doesn't accidentally set a new password for rewritten wallet */
  success = m_wallet->verify_password(pwd_container.password());
  if (!success)
  {
    fail_msg_writer() << tr("invalid password");
    return true;
  }

  std::string mnemonic_language = get_mnemonic_language();
  m_wallet->set_seed_language(mnemonic_language);
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("This wallet is watch-only and cannot transfer.");
    return true;
  }
  tools::password_container pwd_container;
  success = pwd_container.read_password();
  if (!success)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  /* verify password before using so user doesn't accidentally set a new password for rewritten wallet */
  success = m_wallet->verify_password(pwd_container.password());
  if (!success)
  {
    fail_msg_writer() << tr("invalid password");
    return true;
  }

  m_wallet->always_confirm_transfers(is_it_true(args[1]));
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_store_tx_keys(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("This wallet is watch-only and cannot transfer.");
    return true;
  }
  tools::password_container pwd_container;
  success = pwd_container.read_password();
  if (!success)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  /* verify password before using so user doesn't accidentally set a new password for rewritten wallet */
  success = m_wallet->verify_password(pwd_container.password());
  if (!success)
  {
    fail_msg_writer() << tr("invalid password");
    return true;
  }

  m_wallet->store_tx_keys(is_it_true(args[1]));
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_default_mixin(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("This wallet is watch-only and cannot transfer.");
    return true;
  }
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("Error: mixin must be an integer greater or equal to 2");
      return true;
    }
    uint32_t mixin = boost::lexical_cast<uint32_t>(args[1]);
    if (mixin < 2 && mixin != 0)
    {
      fail_msg_writer() << tr("Error: mixin must be an integer greater or equal to 2");
      return true;
    }
    if (mixin == 0)
      mixin = DEFAULT_MIX;

    tools::password_container pwd_container;
    success = pwd_container.read_password();
    if (!success)
    {
      fail_msg_writer() << tr("failed to read wallet password");
      return true;
    }

    /* verify password before using so user doesn't accidentally set a new password for rewritten wallet */
    success = m_wallet->verify_password(pwd_container.password());
    if (!success)
    {
      fail_msg_writer() << tr("invalid password");
      return true;
    }

    m_wallet->default_mixin(mixin);
    m_wallet->rewrite(m_wallet_file, pwd_container.password());
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("Error: mixin must be an integer greater or equal to 2");
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("Error changing default mixin");
    return true;
  }
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_daemon_port(0)
  , m_refresh_progress_reporter(*this)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), tr("start_mining [<number_of_threads>] - Start mining in daemon"));
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), tr("Stop mining in daemon"));
  m_cmd_binder.set_handler("save_bc", boost::bind(&simple_wallet::save_bc, this, _1), tr("Save current blockchain data"));
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), tr("Resynchronize transactions and balance"));
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), tr("Show current wallet balance"));
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), tr("incoming_transfers [available|unavailable] - Show incoming transfers - all of them or filter them by availability"));
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), tr("payments <payment_id_1> [<payment_id_2> ... <payment_id_N>] - Show payments <payment_id_1>, ... <payment_id_N>"));
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), tr("Show blockchain height"));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), tr("transfer [<mixin_count>] <addr_1> <amount_1> [<addr_2> <amount_2> ... <addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available)"));
  m_cmd_binder.set_handler("transfer_new", boost::bind(&simple_wallet::transfer_new, this, _1), tr("Same as transfer, but using a new transaction building algorithm"));
  m_cmd_binder.set_handler("sweep_dust", boost::bind(&simple_wallet::sweep_dust, this, _1), tr("Send all dust outputs to the same address with mixin 0"));
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), tr("set_log <level> - Change current log detalization level, <level> is a number 0-4"));
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), tr("Show current wallet public address"));
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::print_integrated_address, this, _1), tr("Convert a payment ID to an integrated address for the current wallet public address (no arguments use a random payment ID), or display standard addres and payment ID corresponding to an integrated addres"));
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), tr("Save wallet synchronized data"));
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), tr("Save watch only keys file"));
  m_cmd_binder.set_handler("viewkey", boost::bind(&simple_wallet::viewkey, this, _1), tr("Get viewkey"));
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this, _1), tr("Get spendkey"));
  m_cmd_binder.set_handler("seed", boost::bind(&simple_wallet::seed, this, _1), tr("Get deterministic seed"));
  m_cmd_binder.set_handler("set", boost::bind(&simple_wallet::set_variable, this, _1), tr("available options: seed language - Set wallet seed langage; always-confirm-transfers <1|0> - whether to confirm unsplit txes; store-tx-keys <1|0> - whether to store per-tx private keys for future reference; default_mixin <n> - set default mixin (default default is 4"));
  m_cmd_binder.set_handler("rescan_spent", boost::bind(&simple_wallet::rescan_spent, this, _1), tr("Rescan blockchain for spent outputs"));
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), tr("Get transaction key (r) for a given tx"));
  m_cmd_binder.set_handler("check_tx_key", boost::bind(&simple_wallet::check_tx_key, this, _1), tr("Check amount going to a given address in a partcular tx"));
  m_cmd_binder.set_handler("show_transfers", boost::bind(&simple_wallet::show_transfers, this, _1), tr("show_transfers [in|out] [<min_height> [<max_height>]] - show incoming/outgoing transfers within an optional height range"));
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), tr("Show this help"));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    fail_msg_writer() << tr("set: needs an argument. available options: seed, always-confirm-transfers, default-mixin");
    return true;
  }
  else
  {
    if (args[0] == "seed")
    {
      if (args.size() == 1)
      {
        fail_msg_writer() << tr("set seed: needs an argument. available options: language");
        return true;
      }
      else if (args[1] == "language")
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        seed_set_language(local_args);
        return true;
      }
    }
    else if (args[0] == "always-confirm-transfers")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set always-confirm-transfers: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_always_confirm_transfers(local_args);
        return true;
      }
    }
    else if (args[0] == "store-tx-keys")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set store-tx-keys: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_store_tx_keys(local_args);
        return true;
      }
    }
    else if (args[0] == "default-mixin")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set default-mixin: needs an argument (integer greater of equal to 2)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_default_mixin(local_args);
        return true;
      }
    }
  }
  fail_msg_writer() << tr("set: unrecognized argument(s)");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1)
  {
    fail_msg_writer() << tr("use: set_log <log_level_number_0-4>");
    return true;
  }
  uint16_t l = 0;
  if(!epee::string_tools::get_xtype_from_string(l, args[0]))
  {
    fail_msg_writer() << tr("wrong number format, use: set_log <log_level_number_0-4>");
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    fail_msg_writer() << tr("wrong number range, use: set_log <log_level_number_0-4>");
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::ask_wallet_create_if_needed()
{
  std::string wallet_path;

  bool valid_path = false;
  do {
    wallet_path = command_line::input_line(
        tr("Specify wallet file name (e.g., wallet.bin). If the wallet doesn't exist, it will be created.\n"
        "Wallet file name: ")
    );
    valid_path = tools::wallet2::wallet_valid_path_format(wallet_path);
    if (!valid_path)
    {
      fail_msg_writer() << tr("wallet file path not valid: ") << wallet_path;
    }
  }
  while (!valid_path);

  bool keys_file_exists;
  bool wallet_file_exists;
  tools::wallet2::wallet_exists(wallet_path, keys_file_exists, wallet_file_exists);
  LOG_PRINT_L3("wallet_path: " << wallet_path << "");
  LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
    << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);

  LOG_PRINT_L1("Loading wallet...");

  // add logic to error out if new wallet requested but named wallet file exists
  if (keys_file_exists || wallet_file_exists)
  {
    if (!m_generate_new.empty() || m_restore_deterministic_wallet || !m_generate_from_view_key.empty())
    {
      fail_msg_writer() << tr("Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.");
      return false;
    }
  }

  bool r;
  if(keys_file_exists)
  {
    m_wallet_file = wallet_path;
    r = true;
  }else
  {
    if(!wallet_file_exists)
    {
      std::cout << tr("The wallet doesn't exist, generating new one") << std::endl;
      m_generate_new = wallet_path;
      r = true;
    }else
    {
      fail_msg_writer() << tr("Keys file wasn't found: failed to open wallet: ") << "\"" << wallet_path << "\".";
      r = false;
    }
  }

  return r;
}

/*!
 * \brief Prints the seed with a nice message
 * \param seed seed to print
 */
void simple_wallet::print_seed(std::string seed)
{
  success_msg_writer(true) << "\n" << tr("PLEASE NOTE: the following 25 words can be used to recover access to your wallet. "
    "Please write them down and store them somewhere safe and secure. Please do not store them in "
    "your email or on file storage services outside of your immediate control.\n");
  boost::replace_nth(seed, " ", 15, "\n");
  boost::replace_nth(seed, " ", 7, "\n");
  // don't log
  std::cout << seed << std::endl;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  handle_command_line(vm);

  if (!m_daemon_address.empty() && !m_daemon_host.empty() && 0 != m_daemon_port)
  {
    fail_msg_writer() << tr("you can't specify daemon host or port several times");
    return false;
  }

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_view_key.empty()) > 1)
  {
    fail_msg_writer() << tr("Specifying more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\" and --generate-from-keys doesn't make sense!");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_view_key.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  bool testnet = command_line::get_arg(vm, arg_testnet);

  if (m_daemon_host.empty())
    m_daemon_host = "localhost";

  if (!m_daemon_port)
  {
    m_daemon_port = testnet ? config::testnet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
  }

  if (m_daemon_address.empty())
    m_daemon_address = std::string("http://") + m_daemon_host + ":" + std::to_string(m_daemon_port);

  tools::password_container pwd_container;
  if (command_line::has_arg(vm, arg_password))
  {
    pwd_container.password(command_line::get_arg(vm, arg_password));
  }
  else
  {
    bool r = pwd_container.read_password();
    if (!r)
    {
      fail_msg_writer() << tr("failed to read wallet password");
      return false;
    }
  }

  if (!m_generate_new.empty() || m_restore_deterministic_wallet || !m_generate_from_view_key.empty())
  {
    if (m_wallet_file.empty()) m_wallet_file = m_generate_new;  // alias for simplicity later

    std::string old_language;
    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << tr("Cannot specify both --restore-deterministic-wallet and --non-deterministic");
        return false;
      }

      if (m_electrum_seed.empty())
      {
        m_electrum_seed = command_line::input_line("Specify electrum seed: ");
        if (m_electrum_seed.empty())
        {
          fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"words list here\"");
          return false;
        }
      }

      if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
      {
        fail_msg_writer() << tr("electrum-style word list failed verification");
        return false;
      }
    }
    if (!m_generate_from_view_key.empty())
    {
      // split address:viewkey:filename triple
      std::vector<std::string> parts;
      boost::split(parts,m_generate_from_view_key, boost::is_any_of(":"));
      if (parts.size() < 3)
      {
        fail_msg_writer() << tr("--generate-from-view-key needs a address:viewkey:filename triple");
        return false;
      }

      // parse address
      cryptonote::account_public_address address;
      bool has_payment_id;
      crypto::hash8 new_payment_id;
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, testnet, parts[0]))
      {
          fail_msg_writer() << tr("Failed to parse address");
          return false;
      }

      // parse view secret key
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(parts[1], viewkey_data))
      {
        fail_msg_writer() << tr("Failed to parse view key secret key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

      // parse filename
      m_wallet_file = parts[2];
      for (size_t n = 3; n < parts.size(); ++n)
        m_wallet_file += std::string(":") + parts[n];

      bool r = new_wallet(m_wallet_file, pwd_container.password(), address, viewkey, testnet);
      CHECK_AND_ASSERT_MES(r, false, "account creation failed");
    }
    else
    {
      bool r = new_wallet(m_wallet_file, pwd_container.password(), m_recovery_key, m_restore_deterministic_wallet,
        m_non_deterministic, testnet, old_language);
      CHECK_AND_ASSERT_MES(r, false, "account creation failed");
    }
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password(), testnet);
    CHECK_AND_ASSERT_MES(r, false, "could not open account");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet.get())
    return true;

  return close_wallet();
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file                   = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new                  = command_line::get_arg(vm, arg_generate_new_wallet);
  m_generate_from_view_key        = command_line::get_arg(vm, arg_generate_from_view_key);
  m_daemon_address                = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host                   = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port                   = command_line::get_arg(vm, arg_daemon_port);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  m_trusted_daemon                = command_line::get_arg(vm, arg_trusted_daemon);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon()
{
  if (!m_wallet->check_connection())
  {
    fail_msg_writer() << tr("wallet failed to connect to daemon: ") << m_daemon_address << ". " <<
      tr("Daemon either is not started or passed wrong port. "
      "Please, make sure that daemon is running or restart the wallet with correct daemon address.");
    return false;
  }
  return true;
}

/*!
 * \brief Gets the word seed language from the user.
 * 
 * User is asked to choose from a list of supported languages.
 * 
 * \return The chosen language.
 */
std::string simple_wallet::get_mnemonic_language()
{
  std::vector<std::string> language_list;
  std::string language_choice;
  int language_number = -1;
  crypto::ElectrumWords::get_language_list(language_list);
  std::cout << tr("List of available languages for your wallet's seed:") << std::endl;
  int ii;
  std::vector<std::string>::iterator it;
  for (it = language_list.begin(), ii = 0; it != language_list.end(); it++, ii++)
  {
    std::cout << ii << " : " << *it << std::endl;
  }
  while (language_number < 0)
  {
    language_choice = command_line::input_line(tr("Enter the number corresponding to the language of your choice: "));
    try
    {
      language_number = std::stoi(language_choice);
      if (!((language_number >= 0) && (static_cast<unsigned int>(language_number) < language_list.size())))
      {
        language_number = -1;
        fail_msg_writer() << tr("Invalid language choice passed. Please try again.\n");
      }
    }
    catch (std::exception &e)
    {
      fail_msg_writer() << tr("Invalid language choice passed. Please try again.\n");
    }
  }
  return language_list[language_number];
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const std::string &wallet_file, const std::string& password, const crypto::secret_key& recovery_key,
  bool recover, bool two_random, bool testnet, const std::string &old_language)
{
  bool was_deprecated_wallet = m_restore_deterministic_wallet && ((old_language == crypto::ElectrumWords::old_language_name) ||
    crypto::ElectrumWords::get_is_old_style_seed(m_electrum_seed));

  std::string mnemonic_language = old_language;
  // Ask for seed language if:
  // it's a deterministic wallet AND
  // (it is not a wallet restore OR if it was a deprecated wallet
  // that was earlier used before this restore)
  if ((!two_random) && (!m_restore_deterministic_wallet || was_deprecated_wallet))
  {
    if (was_deprecated_wallet)
    {
      // The user had used an older version of the wallet with old style mnemonics.
      message_writer(epee::log_space::console_color_green, false) << "\n" << tr("You had been using "
        "a deprecated version of the wallet. Please use the new seed that we provide.\n");
    }
    mnemonic_language = get_mnemonic_language();
  }

  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);
  m_wallet->set_seed_language(mnemonic_language);

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(wallet_file, password, recovery_key, recover, two_random);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    std::cout << tr("view key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  // convert rng value to electrum-style word list
  std::string electrum_words;

  crypto::ElectrumWords::bytes_to_words(recovery_val, electrum_words, mnemonic_language);

  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Your wallet has been generated.\n"
    "To start synchronizing with the daemon use \"refresh\" command.\n"
    "Use \"help\" command to see the list of available commands.\n"
    "Always use \"exit\" command when closing simplewallet to save\n"
    "current session's state. Otherwise, you will possibly need to synchronize \n"
    "your wallet again. Your wallet key is NOT under risk anyway.\n")
  ;

  if (!two_random)
  {
    print_seed(electrum_words);
  }
  success_msg_writer() << "**********************************************************************";

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const std::string &wallet_file, const std::string& password, const cryptonote::account_public_address& address,
  const crypto::secret_key& viewkey, bool testnet)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);

  try
  {
    m_wallet->generate(wallet_file, password, address, viewkey);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new watch-only wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    std::cout << tr("view key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password, bool testnet)
{
  if (!tools::wallet2::wallet_valid_path_format(wallet_file))
  {
    fail_msg_writer() << tr("wallet file path not valid: ") << wallet_file;
    return false;
  }

  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);

  try
  {
    m_wallet->load(m_wallet_file, password);
    message_writer(epee::log_space::console_color_white, true) <<
      (m_wallet->watch_only() ? tr("Opened watch-only wallet") : tr("Opened wallet")) << ": "
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    // If the wallet file is deprecated, we should ask for mnemonic language again and store
    // everything in the new format.
    // NOTE: this is_deprecated() refers to the wallet file format before becoming JSON. It does not refer to the "old english" seed words form of "deprecated" used elsewhere.
    if (m_wallet->is_deprecated())
    {
      if (m_wallet->is_deterministic())
      {
        message_writer(epee::log_space::console_color_green, false) << "\n" << tr("You had been using "
          "a deprecated version of the wallet. Please proceed to upgrade your wallet.\n");
        std::string mnemonic_language = get_mnemonic_language();
        m_wallet->set_seed_language(mnemonic_language);
        m_wallet->rewrite(m_wallet_file, password);

        // Display the seed
        std::string seed;
        m_wallet->get_seed(seed);
        print_seed(seed);
      }
      else
      {
        message_writer(epee::log_space::console_color_green, false) << "\n" << tr("You had been using "
          "a deprecated version of the wallet. Your wallet file format is being upgraded now.\n");
          m_wallet->rewrite(m_wallet_file, password);
      }
    }
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to load wallet: ") << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  refresh(std::vector<std::string>());
  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Use \"help\" command to see the list of available commands.\n") <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << tr("failed to deinit wallet");
    return false;
  }

  try
  {
    m_wallet->store();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  try
  {
    m_wallet->store();
    success_msg_writer() << tr("Wallet data saved");
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_watch_only(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  tools::password_container pwd_container;

  success = pwd_container.read_password(tr("Password for the new watch-only wallet"));
  if (!success)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }
  std::string password = pwd_container.password();
  success = pwd_container.read_password(tr("Enter new password again"));
  if (!success)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }
  if (password != pwd_container.password())
  {
    fail_msg_writer() << tr("passwords do not match");
    return true;
  }

  m_wallet->write_watch_only_wallet(m_wallet_file, pwd_container.password());
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!m_trusted_daemon)
  {
    fail_msg_writer() << tr("This command assume a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());

  bool ok = true;
  size_t max_mining_threads_count = (std::max)(std::thread::hardware_concurrency(), static_cast<unsigned>(2));
  if (0 == args.size())
  {
    req.threads_count = 1;
  }
  else if (1 == args.size())
  {
    uint16_t num = 1;
    ok = string_tools::get_xtype_from_string(num, args[0]);
    ok = ok && (1 <= num && num <= max_mining_threads_count);
    req.threads_count = num;
  }
  else
  {
    ok = false;
  }

  if (!ok)
  {
    fail_msg_writer() << tr("invalid arguments. Please use start_mining [<number_of_threads>], "
      "<number_of_threads> should be from 1 to ") << max_mining_threads_count;
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Mining started in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been started: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/stop_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Mining stopped in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been stopped: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_bc(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_SAVE_BC::request req;
  COMMAND_RPC_SAVE_BC::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/save_bc", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Blockchain saved");
  else
    fail_msg_writer() << tr("Blockchain can't be saved: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_new_block(uint64_t height, const cryptonote::block& block)
{
  m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_received(uint64_t height, const cryptonote::transaction& tx, size_t out_index)
{
  message_writer(epee::log_space::console_color_green, false) <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(tx) << ", " <<
    tr("received ") << print_money(tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx)
{
  message_writer(epee::log_space::console_color_magenta, false) <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(spend_tx) << ", " <<
    tr("spent ") << print_money(in_tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const cryptonote::transaction& tx)
{
  message_writer(epee::log_space::console_color_red, true) <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(tx) << ", " <<
    tr("unsupported transaction format");
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  message_writer() << tr("Starting refresh...");

  uint64_t fetched_blocks = 0;
  uint64_t start_height = 0;
  if(!args.empty()){
    try
    {
        start_height = boost::lexical_cast<uint64_t>( args[0] );
    }
    catch(const boost::bad_lexical_cast &)
    {
        start_height = 0;
    }
  }
  
  bool ok = false;
  std::ostringstream ss;
  try
  {
    m_wallet->refresh(start_height, fetched_blocks);
    ok = true;
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << tr("Refresh done, blocks received: ") << fetched_blocks;
    show_balance();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << tr("daemon is busy. Please try later");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << tr("no connection to daemon. Please, make sure daemon is running");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    ss << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << tr("Error refreshing: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    ss << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    ss << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    ss << tr("unknown error");
  }

  if (!ok)
  {
    fail_msg_writer() << tr("refresh failed: ") << ss.str() << ". " << tr("Blocks received: ") << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << tr("balance: ") << print_money(m_wallet->balance()) << ", "
    << tr("unlocked balance: ") << print_money(m_wallet->unlocked_balance()) << ", "
    << tr("including unlocked dust: ") << print_money(m_wallet->unlocked_dust_balance(tools::tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD)));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  bool filter = false;
  bool available = false;
  if (!args.empty())
  {
    if (args[0] == "available")
    {
      filter = true;
      available = true;
    }
    else if (args[0] == "unavailable")
    {
      filter = true;
      available = false;
    }
  }

  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != td.m_spent)
    {
      if (!transfers_found)
      {
        message_writer() << boost::format("%21s%8s%16s%68s") % tr("amount") % tr("spent") % tr("global index") % tr("tx id");
        transfers_found = true;
      }
      message_writer(td.m_spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        boost::format("%21s%8s%16u%68s") %
        print_money(td.amount()) %
        (td.m_spent ? "T" : "F") %
        td.m_global_output_index %
        get_transaction_hash (td.m_tx);
    }
  }

  if (!transfers_found)
  {
    if (!filter)
    {
      success_msg_writer() << tr("No incoming transfers");
    }
    else if (available)
    {
      success_msg_writer() << tr("No incoming available transfers");
    }
    else
    {
      success_msg_writer() << tr("No incoming unavailable transfers");
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    fail_msg_writer() << tr("expected at least one payment_id");
    return true;
  }

  message_writer() << boost::format("%68s%68s%12s%21s%16s") %
    tr("payment") % tr("transaction") % tr("height") % tr("amount") % tr("unlock time");

  bool payments_found = false;
  for(std::string arg : args)
  {
    crypto::hash payment_id;
    if(tools::wallet2::parse_payment_id(arg, payment_id))
    {
      std::list<tools::wallet2::payment_details> payments;
      m_wallet->get_payments(payment_id, payments);
      if(payments.empty())
      {
        success_msg_writer() << tr("No payments with id ") << payment_id;
        continue;
      }

      for (const tools::wallet2::payment_details& pd : payments)
      {
        if(!payments_found)
        {
          payments_found = true;
        }
        success_msg_writer(true) <<
          boost::format("%68s%68s%12s%21s%16s") %
          payment_id %
          pd.m_tx_hash %
          pd.m_block_height %
          print_money(pd.m_amount) %
          pd.m_unlock_time;
      }
    }
    else
    {
      fail_msg_writer() << tr("payment id has invalid format, expected 64-character string: ") << arg;
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/getheight", req, res, m_http_client);
  err = interpret_rpc_response(r, res.status);
  return res.height;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_blockchain_height(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
    success_msg_writer() << bc_height;
  else
    fail_msg_writer() << tr("failed to get blockchain height: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::rescan_spent(const std::vector<std::string> &args)
{
  if (!m_trusted_daemon)
  {
    fail_msg_writer() << tr("This command assume a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  try
  {
    m_wallet->rescan_spent();
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try later");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please, make sure daemon is running.");
  }
  catch (const tools::error::is_key_image_spent_error&)
  {
    fail_msg_writer() << tr("failed to get spent status");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_main(bool new_algorithm, const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  size_t fake_outs_count;
  if(local_args.size() > 0) {
    if(!epee::string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
    {
      fake_outs_count = m_wallet->default_mixin();
      if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIX;
    }
    else
    {
      local_args.erase(local_args.begin());
    }
  }

  if(local_args.size() < 2)
  {
     fail_msg_writer() << tr("wrong number of arguments");
     return true;
  }

  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (1 == local_args.size() % 2)
  {
    std::string payment_id_str = local_args.back();
    local_args.pop_back();

    crypto::hash payment_id;
    bool r = tools::wallet2::parse_long_payment_id(payment_id_str, payment_id);
    if(r)
    {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    }
    else
    {
      crypto::hash8 payment_id8;
      r = tools::wallet2::parse_short_payment_id(payment_id_str, payment_id8);
      if(r)
      {
        std::string extra_nonce;
        set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
        r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      }
    }

    if(!r)
    {
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character string: ") << payment_id_str;
      return true;
    }
    payment_id_seen = true;
  }

  vector<cryptonote::tx_destination_entry> dsts;
  for (size_t i = 0; i < local_args.size(); i += 2)
  {
    cryptonote::tx_destination_entry de;
    bool has_payment_id;
    crypto::hash8 new_payment_id;
    if(!get_account_integrated_address_from_str(de.addr, has_payment_id, new_payment_id, m_wallet->testnet(), local_args[i]))
    {
      // if treating as an address fails, try as url
      bool dnssec_ok = false;
      std::string url = local_args[i];

      // attempt to get address from dns query
      auto addresses_from_dns = tools::wallet2::addresses_from_url(url, dnssec_ok);

      // for now, move on only if one address found
      if (addresses_from_dns.size() == 1)
      {
        if (get_account_integrated_address_from_str(de.addr, has_payment_id, new_payment_id, m_wallet->testnet(), addresses_from_dns[0]))
        {
          // if it was an address, prompt user for confirmation.
          // inform user of DNSSEC validation status as well.

          std::string dnssec_str;
          if (dnssec_ok)
          {
            dnssec_str = tr("DNSSEC validation passed");
          }
          else
          {
            dnssec_str = tr("WARNING: DNSSEC validation was unsuccessful, this address may not be correct!");
          }
          std::stringstream prompt;
          prompt << tr("For URL: ") << url
                 << ", " << dnssec_str << std::endl
                 << tr(" Monero Address = ") << addresses_from_dns[0]
                 << std::endl
                 << tr("Is this OK? (Y/n) ")
          ;

          // prompt the user for confirmation given the dns query and dnssec status
          std::string confirm_dns_ok = command_line::input_line(prompt.str());
          if (confirm_dns_ok != "Y" && confirm_dns_ok != "y" && confirm_dns_ok != "Yes" && confirm_dns_ok != "yes"
            && confirm_dns_ok != tr("yes") && confirm_dns_ok != tr("no"))
          {
            fail_msg_writer() << tr("You have cancelled the transfer request");
            return true;
          }
        }
        else
        {
          fail_msg_writer() << tr("Failed to get a Monero address from: ") << local_args[i];
          return true;
        }
      }
      else if (addresses_from_dns.size() > 1)
      {
        fail_msg_writer() << tr("Not yet supported: Multiple Monero addresses found for given URL: ") << url;
      }
      else
      {
        fail_msg_writer() << tr("Wrong address: ") << local_args[i];
        return true;
      }
    }

    if (has_payment_id)
    {
      if (payment_id_seen)
      {
        fail_msg_writer() << tr("A single transaction cannot use more than one payment id: ") << local_args[i];
        return true;
      }

      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, new_payment_id);
      bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      if(!r)
      {
        fail_msg_writer() << tr("Failed to set up payment id, though it was decoded correctly");
        return true;
      }
    }

    bool ok = cryptonote::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      fail_msg_writer() << tr("amount is wrong: ") << local_args[i] << ' ' << local_args[i + 1] <<
        ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }

    dsts.push_back(de);
  }

  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    if (new_algorithm)
      ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra);
    else
      ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra);

    // if more than one tx necessary, prompt user to confirm
    if (m_wallet->always_confirm_transfers() || ptx_vector.size() > 1)
    {
        uint64_t total_fee = 0;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          total_fee += ptx_vector[n].fee;
        }

        std::string prompt_str = (boost::format(tr("Your transaction needs to be split into %llu transactions.  "
          "This will result in a transaction fee being applied to each transaction, for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
          ((unsigned long long)ptx_vector.size()) % print_money(total_fee)).str();
        std::string accepted = command_line::input_line(prompt_str);
        if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
        {
          fail_msg_writer() << tr("Transaction cancelled.");

          // would like to return false, because no tx made, but everything else returns true
          // and I don't know what returning false might adversely affect.  *sigh*
          return true; 
        }
    }

    // actually commit the transactions
    while (!ptx_vector.empty())
    {
      auto & ptx = ptx_vector.back();
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Money successfully sent, transaction ") << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try later");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please, make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    fail_msg_writer() << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee());
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (const cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.amount) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.outs.size();
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << tr("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    fail_msg_writer() << tr("Failed to find a suitable way to split transactions");
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << tr("unknown transfer error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  return transfer_main(false, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_new(const std::vector<std::string> &args_)
{
  return transfer_main(true, args_);
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_dust(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }

  try
  {
    uint64_t total_dust = m_wallet->unlocked_dust_balance(tools::tx_dust_policy(::config::DEFAULT_DUST_THRESHOLD));

    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_dust_sweep_transactions();

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;
    }

    std::string prompt_str = tr("Sweeping ") + print_money(total_dust);
    if (ptx_vector.size() > 1) {
      prompt_str = (boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_dust) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee)).str();
    }
    else {
      prompt_str = (boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_dust) %
        print_money(total_fee)).str();
    }
    std::string accepted = command_line::input_line(prompt_str);
    if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
    {
      fail_msg_writer() << tr("Transaction cancelled.");

      // would like to return false, because no tx made, but everything else returns true
      // and I don't know what returning false might adversely affect.  *sigh*
      return true;
    }

    // actually commit the transactions
    while (!ptx_vector.empty())
    {
      auto & ptx = ptx_vector.back();
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Money successfully sent, transaction: ") << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try later");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please, make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    fail_msg_writer() << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee()) %
      print_money(e.tx_amount()) %
      print_money(e.fee());
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (const cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.amount) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.outs.size();
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << tr("one of destinations is zero");
  }
  catch (const tools::error::tx_too_big& e)
  {
    fail_msg_writer() << tr("Failed to find a suitable way to split transactions");
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << tr("unknown transfer error: ") << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << tr("internal error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 1) {
    fail_msg_writer() << tr("Usage: get_tx_key <txid>");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data))
  {
    fail_msg_writer() << tr("Failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  crypto::secret_key tx_key;
  bool r = m_wallet->get_tx_key(txid, tx_key);
  if (r)
  {
    success_msg_writer() << tr("tx key: ") << tx_key;
    return true;
  }
  else
  {
    fail_msg_writer() << tr("No tx key found for this txid");
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 3) {
    fail_msg_writer() << tr("Usage: check_tx_key <txid> <txkey> <address>");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[0], txid_data))
  {
    fail_msg_writer() << tr("Failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  cryptonote::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[1], tx_key_data))
  {
    fail_msg_writer() << tr("Failed to parse tx key");
    return true;
  }
  crypto::secret_key tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!get_account_integrated_address_from_str(address, has_payment_id, payment_id, m_wallet->testnet(), local_args[2]))
  {
    fail_msg_writer() << tr("Failed to parse address");
    return true;
  }

  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json_remote_command2(m_daemon_address + "/gettransactions", req, res, m_http_client) ||
      res.txs_as_hex.empty())
  {
    fail_msg_writer() << tr("Failed to get transaction from daemon");
    return true;
  }
  cryptonote::blobdata tx_data;
  if (!string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data))
  {
    fail_msg_writer() << tr("Failed to parse transaction from daemon");
    return true;
  }
  crypto::hash tx_hash, tx_prefix_hash;
  cryptonote::transaction tx;
  if (!cryptonote::parse_and_validate_tx_from_blob(tx_data, tx, tx_hash, tx_prefix_hash))
  {
    fail_msg_writer() << tr("Failed to validate transaction from daemon");
    return true;
  }
  if (tx_hash != txid)
  {
    fail_msg_writer() << tr("Failed to get the right transaction from daemon");
    return true;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
  {
    fail_msg_writer() << tr("Failed to generate key derivation from supplied parameters");
    return true;
  }

  uint64_t received = 0;
  try {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      if (typeid(txout_to_key) != tx.vout[n].target.type())
        continue;
      const txout_to_key tx_out_to_key = boost::get<txout_to_key>(tx.vout[n].target);
      crypto::public_key pubkey;
      derive_public_key(derivation, n, address.m_spend_public_key, pubkey);
      if (pubkey == tx_out_to_key.key)
        received += tx.vout[n].amount;
    }
  }
  catch(...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << tr("unknown error");
    return true;
  }

  if (received > 0)
  {
    success_msg_writer() << get_account_address_as_str(m_wallet->testnet(), address) << " received " << print_money(received) << " in txid " << txid;
  }
  else
  {
    fail_msg_writer() << get_account_address_as_str(m_wallet->testnet(), address) << " received nothing in txid " << txid;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfers(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;
  bool in = true;
  bool out = true;
  bool pending = true;
  uint64_t min_height = 0;
  uint64_t max_height = (uint64_t)-1;

  if(local_args.size() > 3) {
    fail_msg_writer() << tr("Usage: show_transfers [in|out|all|pending] [<min_height> [<max_height>]]");
    return true;
  }

  // optional in/out selector
  if (local_args.size() > 0) {
    if (local_args[0] == "in" || local_args[0] == "incoming") {
      out = pending = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "out" || local_args[0] == "outgoing") {
      in = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pending") {
      in = out = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "all" || local_args[0] == "both") {
      local_args.erase(local_args.begin());
    }
  }

  // min height
  if (local_args.size() > 0) {
    try {
      min_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (boost::bad_lexical_cast &) {
      fail_msg_writer() << "Bad min_height parameter: " << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
  }

  // max height
  if (local_args.size() > 0) {
    try {
      max_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (boost::bad_lexical_cast &) {
      fail_msg_writer() << "Bad max_height parameter: " << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
  }

  std::multimap<uint64_t, std::pair<bool,std::string>> output;

  if (in) {
    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
    m_wallet->get_payments(payments, min_height, max_height);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::payment_details &pd = i->second;
      std::string payment_id = string_tools::pod_to_hex(i->first);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(true, (boost::format("%20.20s %s %s %s") % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % "-").str())));
    }
  }

  if (out) {
    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments;
    m_wallet->get_payments_out(payments, min_height, max_height);
    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::confirmed_transfer_details &pd = i->second;
      uint64_t fee = pd.m_amount_in - pd.m_amount_out;
      uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
      std::string dests;
      for (const auto &d: pd.m_dests) {
        if (!dests.empty())
          dests += ", ";
        dests +=  get_account_address_as_str(m_wallet->testnet(), d.addr) + ": " + print_money(d.amount);
      }
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(false, (boost::format("%20.20s %s %s %14.14s %s") % print_money(pd.m_amount_in - change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % dests).str())));
    }
  }

  // print in and out sorted by height
  for (std::map<uint64_t, std::pair<bool, std::string>>::const_iterator i = output.begin(); i != output.end(); ++i) {
    message_writer(i->second.first ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
      boost::format("%8.8llu %6.6s %s") %
      ((unsigned long long)i->first) % (i->second.first ? tr("in") : tr("out")) % i->second.second;
  }

  // print unconfirmed last
  if (pending) {
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
      uint64_t amount = 0;
      cryptonote::get_inputs_money_amount(pd.m_tx, amount);
      uint64_t fee = amount - get_outs_money_amount(pd.m_tx);
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      message_writer() << (boost::format("%8.8s %6.6s %20.20s %s %s %14.14s") % tr("pending") % tr("out") % print_money(amount - pd.m_change) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee)).str();
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  std::string addr_start = m_wallet->get_account().get_public_address_str(m_wallet->testnet()).substr(0, 6);
  return m_cmd_binder.run_handling(std::string("[") + tr("wallet") + " " + addr_start + "]: ", "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();
  m_wallet->stop();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_integrated_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  crypto::hash8 payment_id;
  if (args.size() > 1)
  {
    fail_msg_writer() << tr("integrated_address only takes one or zero arguments");
    return true;
  }
  if (args.size() == 0)
  {
    payment_id = crypto::rand<crypto::hash8>();
    success_msg_writer() << tr("Random payment ID: ") << payment_id;
    success_msg_writer() << tr("Matching integrated address: ") << m_wallet->get_account().get_public_integrated_address_str(payment_id, m_wallet->testnet());
    return true;
  }
  if(tools::wallet2::parse_short_payment_id(args.back(), payment_id))
  {
    success_msg_writer() << m_wallet->get_account().get_public_integrated_address_str(payment_id, m_wallet->testnet());
    return true;
  }
  else {
    bool has_payment_id;
    crypto::hash8 payment_id;
    account_public_address addr;
    if(get_account_integrated_address_from_str(addr, has_payment_id, payment_id, m_wallet->testnet(), args.back()))
    {
      if (has_payment_id)
      {
        success_msg_writer() << boost::format(tr("Integrated address: account %s, payment id %s")) %
          get_account_address_as_str(m_wallet->testnet(),addr) % epee::string_tools::pod_to_hex(payment_id);
      }
      else
      {
        success_msg_writer() << tr("Standard address: account: ") << get_account_address_as_str(m_wallet->testnet(),addr);
      }
      return true;
    }
  }
  fail_msg_writer() << tr("Failed to parse payment id or address");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  //TRY_ENTRY();

  string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general(sw::tr("General options"));
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params(sw::tr("Wallet options"));
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);

  bf::path default_log {log_space::log_singletone::get_default_log_folder()};
  std::string log_file_name = log_space::log_singletone::get_default_log_file();
  if (log_file_name.empty())
  {
    // Sanity check: File path should also be empty if file name is. If not,
    // this would be a problem in epee's discovery of current process's file
    // path.
    if (! default_log.empty())
    {
      fail_msg_writer() << sw::tr("Unexpected empty log file name in presence of non-empty file path");
      return false;
    }
    // epee didn't find path to executable from argv[0], so use this default file name.
    log_file_name = "simplewallet.log";
    // The full path will use cwd because epee also returned an empty default log folder.
  }
  default_log /= log_file_name;

  command_line::add_arg(desc_params, arg_log_file, default_log.string());

  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_electrum_seed );
  command_line::add_arg(desc_params, arg_testnet);
  command_line::add_arg(desc_params, arg_restricted);
  command_line::add_arg(desc_params, arg_trusted_daemon);
  tools::wallet_rpc_server::init_options(desc_params);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  i18n_set_language("translations", "monero");

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  cryptonote::simple_wallet w;
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      success_msg_writer() << CRYPTONOTE_NAME << " " << sw::tr("wallet") << " v" << MONERO_VERSION_FULL;
      success_msg_writer() << sw::tr("Usage:") << " simplewallet [--wallet-file=<file>|--generate-new-wallet=<file>] [--daemon-address=<host>:<port>] [<COMMAND>]";
      success_msg_writer() << desc_all << '\n' << w.get_commands_str();
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      success_msg_writer() << CRYPTONOTE_NAME << " " << sw::tr("wallet") << " v" << MONERO_VERSION_FULL;
      return false;
    }

    auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 0;

  // log_file_path
  //   default: < argv[0] directory >/simplewallet.log
  //     so if ran as "simplewallet" (no path), log file will be in cwd
  //
  //   if log-file argument given:
  //     absolute path
  //     relative path: relative to cwd

  // Set log file
  bf::path log_file_path {bf::absolute(command_line::get_arg(vm, arg_log_file))};

  // Set up logging options
  int log_level = LOG_LEVEL_2;
  log_space::get_set_log_detalisation_level(true, log_level);
  //log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_file_path.filename().string().c_str(),
    log_file_path.parent_path().string().c_str(),
    LOG_LEVEL_4
    );

  message_writer(epee::log_space::console_color_white, true) << CRYPTONOTE_NAME << " " << sw::tr("wallet") << " v" << MONERO_VERSION_FULL;

  if(command_line::has_arg(vm, arg_log_level))
    log_level = command_line::get_arg(vm, arg_log_level);
  LOG_PRINT_L0("Setting log level = " << log_level);
  LOG_PRINT_L0(sw::tr("default_log: ") << default_log.string());
  message_writer(epee::log_space::console_color_white, true) << boost::format(sw::tr("Logging at log level %d to %s")) %
    log_level % log_file_path.string();
  log_space::get_set_log_detalisation_level(true, log_level);

  if(command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
    //runs wallet with rpc interface
    if(!command_line::has_arg(vm, arg_wallet_file) )
    {
      LOG_ERROR(sw::tr("Wallet file not set."));
      return 1;
    }
    if(!command_line::has_arg(vm, arg_daemon_address) )
    {
      LOG_ERROR(sw::tr("Daemon address not set."));
      return 1;
    }
    if(!command_line::has_arg(vm, arg_password) )
    {
      LOG_ERROR(sw::tr("Wallet password not set."));
      return 1;
    }

    bool testnet = command_line::get_arg(vm, arg_testnet);
    bool restricted = command_line::get_arg(vm, arg_restricted);
    std::string wallet_file     = command_line::get_arg(vm, arg_wallet_file);
    std::string wallet_password = command_line::get_arg(vm, arg_password);
    std::string daemon_address  = command_line::get_arg(vm, arg_daemon_address);
    std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
    int daemon_port = command_line::get_arg(vm, arg_daemon_port);
    if (daemon_host.empty())
      daemon_host = "localhost";
    if (!daemon_port)
      daemon_port = testnet ? config::testnet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    tools::wallet2 wal(testnet,restricted);
    try
    {
      LOG_PRINT_L0(sw::tr("Loading wallet..."));
      wal.load(wallet_file, wallet_password);
      wal.init(daemon_address);
      wal.refresh();
      LOG_PRINT_GREEN(sw::tr("Loaded ok"), LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(sw::tr("Wallet initialization failed: ") << e.what());
      return 1;
    }
    tools::wallet_rpc_server wrpc(wal);
    bool r = wrpc.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, sw::tr("Failed to initialize wallet rpc server"));

    tools::signal_handler::install([&wrpc, &wal] {
      wrpc.send_stop_signal();
      wal.store();
    });
    LOG_PRINT_L0(sw::tr("Starting wallet rpc server"));
    wrpc.run();
    LOG_PRINT_L0(sw::tr("Stopped wallet rpc server"));
    try
    {
      LOG_PRINT_L0(sw::tr("Storing wallet..."));
      wal.store();
      LOG_PRINT_GREEN(sw::tr("Stored ok"), LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(sw::tr("Failed to store wallet: ") << e.what());
      return 1;
    }
  }else
  {
    //runs wallet with console interface
    r = w.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, sw::tr("Failed to initialize wallet"));

    std::vector<std::string> command = command_line::get_arg(vm, arg_command);
    if (!command.empty())
    {
      w.process_command(command);
      w.stop();
      w.deinit();
    }
    else
    {
      tools::signal_handler::install([&w] {
        w.stop();
      });
      w.run();

      w.deinit();
    }
  }
  return 0;
  //CATCH_ENTRY_L0("main", 1);
}
