// Copyright (c) 2014-2016, The Monero Project
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
#include <fstream>
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
#include "rapidjson/document.h"
#include "common/json_util.h"
#include "ringct/rctSigs.h"
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

#define DEFAULT_MIX 4

#define KEY_IMAGE_EXPORT_FILE_MAGIC "Monero key image export\001"

// workaround for a suspected bug in pthread/kernel on MacOS X
#ifdef __APPLE__
#define DEFAULT_MAX_CONCURRENCY 1
#else
#define DEFAULT_MAX_CONCURRENCY 0
#endif

#define LOCK_IDLE_SCOPE() \
  bool auto_refresh_enabled = m_auto_refresh_enabled.load(std::memory_order_relaxed); \
  m_auto_refresh_enabled.store(false, std::memory_order_relaxed); \
  /* stop any background refresh, and take over */ \
  m_wallet->stop(); \
  m_idle_mutex.lock(); \
  while (m_auto_refresh_refreshing) \
    m_idle_cond.notify_one(); \
  m_idle_mutex.unlock(); \
/*  if (auto_refresh_run)*/ \
    /*m_auto_refresh_thread.join();*/ \
  boost::unique_lock<boost::mutex> lock(m_idle_mutex); \
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ \
    m_auto_refresh_enabled.store(auto_refresh_enabled, std::memory_order_relaxed); \
  })

enum TransferType {
  TransferOriginal,
  TransferNew,
};

namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", sw::tr("Use wallet <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", sw::tr("Generate new wallet and save it to <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_view_key = {"generate-from-view-key", sw::tr("Generate incoming-only wallet from view key"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_keys = {"generate-from-keys", sw::tr("Generate wallet from private keys"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_json = {"generate-from-json", sw::tr("Generate wallet from JSON format file"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", sw::tr("Use daemon instance at <host>:<port>"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", sw::tr("Use daemon instance at host <arg> instead of localhost"), ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", sw::tr("Wallet password"), "", true};
  const command_line::arg_descriptor<std::string> arg_password_file = {"password-file", sw::tr("Wallet password file"), "", true};
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", sw::tr("Specify Electrum seed for wallet recovery/creation"), ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", sw::tr("Recover wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("Create non-deterministic view and spend keys"), false};
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", sw::tr("Use daemon instance at port <arg> instead of 18081"), 0};
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"log-level", "", LOG_LEVEL_0};
  const command_line::arg_descriptor<uint32_t> arg_max_concurrency = {"max-concurrency", "Max number of threads to use for a parallel job", DEFAULT_MAX_CONCURRENCY};
  const command_line::arg_descriptor<std::string> arg_log_file = {"log-file", sw::tr("Specify log file"), ""};
  const command_line::arg_descriptor<bool> arg_testnet = {"testnet", sw::tr("For testnet. Daemon must also be launched with --testnet flag"), false};
  const command_line::arg_descriptor<bool> arg_restricted = {"restricted-rpc", sw::tr("Restricts RPC to view-only commands"), false};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", sw::tr("Enable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_allow_mismatched_daemon_version = {"allow-mismatched-daemon-version", sw::tr("Allow communicating with a daemon that uses a different RPC version"), false};
  const command_line::arg_descriptor<uint64_t> arg_restore_height = {"restore-height", sw::tr("Restore from specific blockchain height"), 0};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = sw::tr("daemon is busy. Please try again later.");
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = sw::tr("possibly lost connection to daemon");
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

        LOG_PRINT(m_oss.str(), m_log_level);

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

  const struct
  {
    const char *name;
    tools::wallet2::RefreshType refresh_type;
  } refresh_type_names[] =
  {
    { "full", tools::wallet2::RefreshFull },
    { "optimize-coinbase", tools::wallet2::RefreshOptimizeCoinbase },
    { "optimized-coinbase", tools::wallet2::RefreshOptimizeCoinbase },
    { "no-coinbase", tools::wallet2::RefreshNoCoinbase },
    { "default", tools::wallet2::RefreshDefault },
  };

  bool parse_refresh_type(const std::string &s, tools::wallet2::RefreshType &refresh_type)
  {
    for (size_t n = 0; n < sizeof(refresh_type_names) / sizeof(refresh_type_names[0]); ++n)
    {
      if (s == refresh_type_names[n].name)
      {
        refresh_type = refresh_type_names[n].refresh_type;
        return true;
      }
    }
    fail_msg_writer() << tr("failed to parse refresh type");
    return false;
  }

  std::string get_refresh_type_name(tools::wallet2::RefreshType type)
  {
    for (size_t n = 0; n < sizeof(refresh_type_names) / sizeof(refresh_type_names[0]); ++n)
    {
      if (type == refresh_type_names[n].refresh_type)
        return refresh_type_names[n].name;
    }
    return "invalid";
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
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  if (m_wallet->is_deterministic())
  {
    if (m_wallet->get_seed_language().empty())
    {
      std::string mnemonic_language = get_mnemonic_language();
      if (mnemonic_language.empty())
        return true;
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
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
  }
  return true;
}

bool simple_wallet::seed_set_language(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  if (!m_wallet->is_deterministic())
  {
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
    return true;
  }
 
  tools::password_container pwd_container(m_wallet_file.empty());
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
  if (mnemonic_language.empty())
    return true;
  m_wallet->set_seed_language(mnemonic_language);
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  tools::password_container pwd_container(m_wallet_file.empty());
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

bool simple_wallet::set_store_tx_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
 
  tools::password_container pwd_container(m_wallet_file.empty());
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

  m_wallet->store_tx_info(is_it_true(args[1]));
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_default_mixin(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("mixin must be an integer >= 2");
      return true;
    }
    uint32_t mixin = boost::lexical_cast<uint32_t>(args[1]);
    if (mixin < 2 && mixin != 0)
    {
      fail_msg_writer() << tr("mixin must be an integer >= 2");
      return true;
    }
    if (mixin == 0)
      mixin = DEFAULT_MIX;
 
    tools::password_container pwd_container(m_wallet_file.empty());

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
    fail_msg_writer() << tr("mixin must be an integer >= 2");
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("could not change default mixin");
    return true;
  }
}

bool simple_wallet::set_default_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  int priority = 0;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("priority must be 0, 1, 2, or 3 ");
      return true;
    }
    if (args[1] == "0")
    {
      priority = 0;
    }
    else
    {
      priority = boost::lexical_cast<int>(args[1]);
      if (priority != 1 && priority != 2 && priority != 3)
      {
        fail_msg_writer() << tr("priority must be 0, 1, 2, or 3");
        return true;
      }
    }
 
    tools::password_container pwd_container(m_wallet_file.empty());
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

    m_wallet->set_default_priority(priority);
    m_wallet->rewrite(m_wallet_file, pwd_container.password());
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("priority must be 0, 1, 2 or 3");
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("could not change default priority");
    return true;
  }
}

bool simple_wallet::set_auto_refresh(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
 
  tools::password_container pwd_container(m_wallet_file.empty());

  bool success = pwd_container.read_password();
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

  bool auto_refresh = is_it_true(args[1]);
  m_wallet->auto_refresh(auto_refresh);
  m_idle_mutex.lock();
  m_auto_refresh_enabled.store(auto_refresh, std::memory_order_relaxed);
  m_idle_cond.notify_one();
  m_idle_mutex.unlock();

  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_refresh_type(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;

  tools::wallet2::RefreshType refresh_type;
  if (!parse_refresh_type(args[1], refresh_type))
  {
    return true;
  }
 
  tools::password_container pwd_container(m_wallet_file.empty());
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

  m_wallet->set_refresh_type(refresh_type);

  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::set_confirm_missing_payment_id(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  bool success = false;
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
  tools::password_container pwd_container(m_wallet_file.empty());
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

  m_wallet->confirm_missing_payment_id(is_it_true(args[1]));
  m_wallet->rewrite(m_wallet_file, pwd_container.password());
  return true;
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_allow_mismatched_daemon_version(false)
  , m_daemon_port(0)
  , m_refresh_progress_reporter(*this)
  , m_idle_run(true)
  , m_auto_refresh_enabled(false)
  , m_auto_refresh_refreshing(false)
  , m_in_manual_refresh(false)
{
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), tr("start_mining [<number_of_threads>] - Start mining in daemon"));
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), tr("Stop mining in daemon"));
  m_cmd_binder.set_handler("save_bc", boost::bind(&simple_wallet::save_bc, this, _1), tr("Save current blockchain data"));
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), tr("Synchronize transactions and balance"));
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), tr("Show current wallet balance"));
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), tr("incoming_transfers [available|unavailable] - Show incoming transfers, all or filtered by availability"));
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), tr("payments <PID_1> [<PID_2> ... <PID_N>] - Show payments for given payment ID[s]"));
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), tr("Show blockchain height"));
  m_cmd_binder.set_handler("transfer_original", boost::bind(&simple_wallet::transfer, this, _1), tr("transfer [<mixin_count>] <addr_1> <amount_1> [<addr_2> <amount_2> ... <addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of extra inputs to include for untraceability (from 2 to maximum available)"));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer_new, this, _1), tr("Same as transfer_original, but using a new transaction building algorithm"));
  m_cmd_binder.set_handler("locked_transfer", boost::bind(&simple_wallet::locked_transfer, this, _1), tr("locked_transfer [<mixin_count>] <addr> <amount> <lockblocks>(Number of blocks to lock the transaction for, max 1000000) [<payment_id>]"));
  m_cmd_binder.set_handler("sweep_unmixable", boost::bind(&simple_wallet::sweep_unmixable, this, _1), tr("Send all unmixable outputs to yourself with mixin 0"));
  m_cmd_binder.set_handler("sweep_all", boost::bind(&simple_wallet::sweep_all, this, _1), tr("sweep_all [mixin] address [payment_id] - Send all unlocked balance an address"));
  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this, _1), tr("Sign a transaction from a file"));
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this, _1), tr("Submit a signed transaction from a file"));
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), tr("set_log <level> - Change current log detail level, <0-4>"));
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), tr("Show current wallet public address"));
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::print_integrated_address, this, _1), tr("integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID"));
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), tr("Save wallet data"));
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), tr("Save a watch-only keys file"));
  m_cmd_binder.set_handler("viewkey", boost::bind(&simple_wallet::viewkey, this, _1), tr("Display private view key"));
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this, _1), tr("Display private spend key"));
  m_cmd_binder.set_handler("seed", boost::bind(&simple_wallet::seed, this, _1), tr("Display Electrum-style mnemonic seed"));
  m_cmd_binder.set_handler("set", boost::bind(&simple_wallet::set_variable, this, _1), tr("Available options: seed language - set wallet seed language; always-confirm-transfers <1|0> - whether to confirm unsplit txes; store-tx-info <1|0> - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin <n> - set default mixin (default is 4); auto-refresh <1|0> - whether to automatically sync new blocks from the daemon; refresh-type <full|optimize-coinbase|no-coinbase|default> - set wallet refresh behaviour; priority [1|2|3] - normal/elevated/priority fee; confirm-missing-payment-id <1|0>"));
  m_cmd_binder.set_handler("rescan_spent", boost::bind(&simple_wallet::rescan_spent, this, _1), tr("Rescan blockchain for spent outputs"));
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), tr("Get transaction key (r) for a given <txid>"));
  m_cmd_binder.set_handler("check_tx_key", boost::bind(&simple_wallet::check_tx_key, this, _1), tr("Check amount going to <address> in <txid>"));
  m_cmd_binder.set_handler("show_transfers", boost::bind(&simple_wallet::show_transfers, this, _1), tr("show_transfers [in|out] [<min_height> [<max_height>]] - Show incoming/outgoing transfers within an optional height range"));
  m_cmd_binder.set_handler("rescan_bc", boost::bind(&simple_wallet::rescan_blockchain, this, _1), tr("Rescan blockchain from scratch"));
  m_cmd_binder.set_handler("set_tx_note", boost::bind(&simple_wallet::set_tx_note, this, _1), tr("Set an arbitrary string note for a txid"));
  m_cmd_binder.set_handler("get_tx_note", boost::bind(&simple_wallet::get_tx_note, this, _1), tr("Get a string note for a txid"));
  m_cmd_binder.set_handler("status", boost::bind(&simple_wallet::status, this, _1), tr("Show wallet status information"));
  m_cmd_binder.set_handler("sign", boost::bind(&simple_wallet::sign, this, _1), tr("Sign the contents of a file"));
  m_cmd_binder.set_handler("verify", boost::bind(&simple_wallet::verify, this, _1), tr("Verify a signature on the contents of a file"));
  m_cmd_binder.set_handler("export_key_images", boost::bind(&simple_wallet::export_key_images, this, _1), tr("Export a signed set of key images"));
  m_cmd_binder.set_handler("import_key_images", boost::bind(&simple_wallet::import_key_images, this, _1), tr("Import signed key images list and verify their spent status"));
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), tr("Show this help"));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    success_msg_writer() << "seed = " << m_wallet->get_seed_language();
    success_msg_writer() << "always-confirm-transfers = " << m_wallet->always_confirm_transfers();
    success_msg_writer() << "store-tx-info = " << m_wallet->store_tx_info();
    success_msg_writer() << "default-mixin = " << m_wallet->default_mixin();
    success_msg_writer() << "auto-refresh = " << m_wallet->auto_refresh();
    success_msg_writer() << "refresh-type = " << get_refresh_type_name(m_wallet->get_refresh_type());
    success_msg_writer() << "priority = " << m_wallet->get_default_priority();
    success_msg_writer() << "confirm-missing-payment-id = " << m_wallet->confirm_missing_payment_id();
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
    else if (args[0] == "store-tx-info")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set store-tx-info: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_store_tx_info(local_args);
        return true;
      }
    }
    else if (args[0] == "default-mixin")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set default-mixin: needs an argument (integer >= 2)");
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
    else if (args[0] == "auto-refresh")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set auto-refresh: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_auto_refresh(local_args);
        return true;
      }
    }
    else if (args[0] == "refresh-type")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set refresh-type: needs an argument:") <<
          tr("full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_refresh_type(local_args);
        return true;
      }
    }
    else if (args[0] == "priority")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set priority: needs an argument: 0, 1, 2, or 3");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_default_priority(local_args);
        return true;
      }
    }
    else if (args[0] == "confirm-missing-payment-id")
    {
      if (args.size() <= 1)
      {
        fail_msg_writer() << tr("set confirm-missing-payment-id: needs an argument (0 or 1)");
        return true;
      }
      else
      {
        std::vector<std::string> local_args = args;
        local_args.erase(local_args.begin(), local_args.begin()+2);
        set_confirm_missing_payment_id(local_args);
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
    fail_msg_writer() << tr("usage: set_log <log_level_number_0-4>");
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
        tr("Specify wallet file name (e.g., MyWallet). If the wallet doesn't exist, it will be created.\n"
        "Wallet file name: ")
    );
    if (std::cin.eof())
    {
      return false;
    }
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
    if (!m_generate_new.empty() || m_restoring)
    {
      fail_msg_writer() << tr("attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.");
      return false;
    }
  }

  bool r;
  if(keys_file_exists)
  {
    m_wallet_file=wallet_path;
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
      fail_msg_writer() << tr("keys file not found: failed to open wallet: ") << "\"" << wallet_path << "\".";
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
bool simple_wallet::get_password(const boost::program_options::variables_map& vm, bool allow_entry, tools::password_container &pwd_container)
{
  // has_arg returns true even when the parameter is not passed ??
  const std::string gfj = command_line::get_arg(vm, arg_generate_from_json);
  if (!gfj.empty()) {
    // will be in the json file, if any
    return true;
  }

  if (has_arg(vm, arg_password) && has_arg(vm, arg_password_file))
  {
    fail_msg_writer() << tr("can't specify more than one of --password and --password-file");
    return false;
  }

  if (command_line::has_arg(vm, arg_password))
  {
    pwd_container.password(command_line::get_arg(vm, arg_password));
    return true;
  }

  if (command_line::has_arg(vm, arg_password_file))
  {
    std::string password;
    bool r = epee::file_io_utils::load_file_to_string(command_line::get_arg(vm, arg_password_file),
                                                      password);
    if (!r)
    {
      fail_msg_writer() << tr("the password file specified could not be read");
      return false;
    }

    // Remove line breaks the user might have inserted
    password.erase(std::remove(password.begin() - 1, password.end(), '\n'), password.end());
    password.erase(std::remove(password.end() - 1, password.end(), '\r'), password.end());
    pwd_container.password(password.c_str());
    return true;
  }

  if (allow_entry)
  {
      //vm is already part of the password container class.  just need to check vm for an already existing wallet
      //here need to pass in variable map.  This will indicate if the wallet already exists to the read password function
    bool r = pwd_container.read_password();
    if (!r)
    {
      fail_msg_writer() << tr("failed to read wallet password");
      return false;
    }
    return true;
  }

  fail_msg_writer() << tr("Wallet password not set.");
  return false;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::generate_from_json(const boost::program_options::variables_map& vm, std::string &wallet_file, std::string &password)
{
  bool testnet = command_line::get_arg(vm, arg_testnet);

  std::string buf;
  bool r = epee::file_io_utils::load_file_to_string(m_generate_from_json, buf);
  if (!r) {
    fail_msg_writer() << tr("Failed to load file ") << m_generate_from_json;
    return false;
  }

  rapidjson::Document json;
  if (json.Parse(buf.c_str()).HasParseError()) {
    fail_msg_writer() << tr("Failed to parse JSON");
    return false;
  }

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, version, unsigned, Uint, true, 0);
  const int current_version = 1;
  if (field_version > current_version) {
    fail_msg_writer() << boost::format(tr("Version %u too new, we can only grok up to %u")) % field_version % current_version;
    return false;
  }

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, filename, std::string, String, true, std::string());

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, scan_from_height, uint64_t, Uint64, false, 0);
  bool recover = field_scan_from_height_found;

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, password, std::string, String, false, std::string());
  password = field_password;

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, viewkey, std::string, String, false, std::string());
  crypto::secret_key viewkey;
  if (field_viewkey_found)
  {
    cryptonote::blobdata viewkey_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(field_viewkey, viewkey_data))
    {
      fail_msg_writer() << tr("failed to parse view key secret key");
      return false;
    }
    viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());
    crypto::public_key pkey;
    if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
      fail_msg_writer() << tr("failed to verify view key secret key");
      return false;
    }
  }

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, spendkey, std::string, String, false, std::string());
  crypto::secret_key spendkey;
  if (field_spendkey_found)
  {
    cryptonote::blobdata spendkey_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(field_spendkey, spendkey_data))
    {
      fail_msg_writer() << tr("failed to parse spend key secret key");
      return false;
    }
    spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
    crypto::public_key pkey;
    if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
      fail_msg_writer() << tr("failed to verify spend key secret key");
      return false;
    }
  }

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, seed, std::string, String, false, std::string());
  std::string old_language;
  if (field_seed_found)
  {
    if (!crypto::ElectrumWords::words_to_bytes(field_seed, m_recovery_key, old_language))
    {
      fail_msg_writer() << tr("Electrum-style word list failed verification");
      return false;
    }
    m_electrum_seed = field_seed;
    m_restore_deterministic_wallet = true;
  }

  GET_FIELD_FROM_JSON_RETURN_ON_ERROR(json, address, std::string, String, false, std::string());

  // compatibility checks
  if (!field_seed_found && !field_viewkey_found)
  {
    fail_msg_writer() << tr("At least one of Electrum-style word list and private view key must be specified");
    return false;
  }
  if (field_seed_found && (field_viewkey_found || field_spendkey_found))
  {
    fail_msg_writer() << tr("Both Electrum-style word list and private key(s) specified");
    return false;
  }

  // if an address was given, we check keys against it, and deduce the spend
  // public key if it was not given
  if (field_address_found)
  {
    cryptonote::account_public_address address;
    bool has_payment_id;
    crypto::hash8 new_payment_id;
    if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, testnet, field_address))
    {
      fail_msg_writer() << tr("invalid address");
      return false;
    }
    if (field_viewkey_found)
    {
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }
    }
    if (field_spendkey_found)
    {
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
        fail_msg_writer() << tr("failed to verify spend key secret key");
        return false;
      }
      if (address.m_spend_public_key != pkey) {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
    }
  }

  m_wallet_file=field_filename;

  bool was_deprecated_wallet = m_restore_deterministic_wallet && ((old_language == crypto::ElectrumWords::old_language_name) ||
    crypto::ElectrumWords::get_is_old_style_seed(m_electrum_seed));
  if (was_deprecated_wallet) {
    fail_msg_writer() << tr("Cannot create deprecated wallets from JSON");
    return false;
  }

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);
  m_wallet->set_refresh_from_block_height(field_scan_from_height);

  try
  {
    if (!field_seed.empty())
    {
      m_wallet->generate(m_wallet_file, password, m_recovery_key, recover, false);
    }
    else
    {
      cryptonote::account_public_address address;
      if (!crypto::secret_key_to_public_key(viewkey, address.m_view_public_key)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }

      if (field_spendkey.empty())
      {
        // if we have an addres but no spend key, we can deduce the spend public key
        // from the address
        if (field_address_found)
        {
          cryptonote::account_public_address address2;
          bool has_payment_id;
          crypto::hash8 new_payment_id;
          get_account_integrated_address_from_str(address2, has_payment_id, new_payment_id, testnet, field_address);
          address.m_spend_public_key = address2.m_spend_public_key;
        }
        m_wallet->generate(m_wallet_file, password, address, viewkey);
      }
      else
      {
        if (!crypto::secret_key_to_public_key(spendkey, address.m_spend_public_key)) {
          fail_msg_writer() << tr("failed to verify spend key secret key");
          return false;
        }
        m_wallet->generate(m_wallet_file, password, address, spendkey, viewkey);
      }
    }
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  wallet_file = m_wallet_file;

  return r;
}

static bool is_local_daemon(const std::string &address)
{
  // extract host
  epee::net_utils::http::url_content u_c;
  if (!epee::net_utils::parse_url(address, u_c))
  {
    LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
    return false;
  }
  if (u_c.host.empty())
  {
    LOG_PRINT_L1("Failed to determine whether daemon is local, assuming not");
    return false;
  }

  // resolve to IP
  boost::asio::io_service io_service;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(u_c.host, "");
  boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query);
  while (i != boost::asio::ip::tcp::resolver::iterator())
  {
    const boost::asio::ip::tcp::endpoint &ep = *i;
    if (ep.address().is_loopback())
      return true;
    ++i;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  if (!handle_command_line(vm))
    return false;

  if (!m_daemon_address.empty() && !m_daemon_host.empty() && 0 != m_daemon_port)
  {
    fail_msg_writer() << tr("can't specify daemon host or port more than once");
    return false;
  }

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_view_key.empty()) + (!m_generate_from_keys.empty()) + (!m_generate_from_json.empty()) > 1)
  {
    fail_msg_writer() << tr("can't specify more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\", --generate-from-view-key=\"wallet_name\", --generate-from-json=\"jsonfilename\" and --generate-from-keys=\"wallet_name\"");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_view_key.empty() && m_generate_from_keys.empty() && m_generate_from_json.empty())
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

  // set --trusted-daemon if local
  try
  {
    if (is_local_daemon(m_daemon_address))
    {
      LOG_PRINT_L1(tr("Daemon is local, assuming trusted"));
      m_trusted_daemon = true;
    }
  }
  catch (const std::exception &e) { }
  tools::password_container pwd_container(m_wallet_file.empty()); //m_wallet_file will be empty at this point for new wallets
  if (!cryptonote::simple_wallet::get_password(vm, true, pwd_container))
    return false;

  if (!m_generate_new.empty() || m_restoring)
  {
    if (m_wallet_file.empty()) m_wallet_file = m_generate_new;  // alias for simplicity later

    std::string old_language;
    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << tr("can't specify both --restore-deterministic-wallet and --non-deterministic");
        return false;
      }

      if (m_electrum_seed.empty())
      {
        m_electrum_seed = command_line::input_line("Specify Electrum seed: ");
        if (std::cin.eof())
          return false;
        if (m_electrum_seed.empty())
        {
          fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"words list here\"");
          return false;
        }
      }

      if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
      {
        fail_msg_writer() << tr("Electrum-style word list failed verification");
        return false;
      }
    }
    if (!m_restore_height && m_restoring)
    {
      std::string heightstr = command_line::input_line("Restore from specific blockchain height (optional, default 0): ");
      if (std::cin.eof())
        return false;
      if (heightstr.size())
      {
        try {
          m_restore_height = boost::lexical_cast<uint64_t>(heightstr);
        }
        catch (boost::bad_lexical_cast &) {
          fail_msg_writer() << tr("bad m_restore_height parameter:") << " " << heightstr;
          return false;
        }
      }
    }
    if (!m_generate_from_view_key.empty())
    {
      // parse address
      std::string address_string = command_line::input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::account_public_address address;
      bool has_payment_id;
      crypto::hash8 new_payment_id;
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, testnet, address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }

      // parse view secret key
      std::string viewkey_string = command_line::input_line("View key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

      m_wallet_file=m_generate_from_view_key;

      // check the view key matches the given address
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }

      bool r = new_wallet(m_wallet_file, pwd_container.password(), address, viewkey, testnet);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_keys.empty())
    {
      // parse address
      std::string address_string = command_line::input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::account_public_address address;
      bool has_payment_id;
      crypto::hash8 new_payment_id;
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, testnet, address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }

      // parse spend secret key
      std::string spendkey_string = command_line::input_line("Spend key: ");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata spendkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }
      crypto::secret_key spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());

      // parse view secret key
      std::string viewkey_string = command_line::input_line("View key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

      m_wallet_file=m_generate_from_keys;

      // check the spend and view keys match the given address
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
        fail_msg_writer() << tr("failed to verify spend key secret key");
        return false;
      }
      if (address.m_spend_public_key != pkey) {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }

      bool r = new_wallet(m_wallet_file, pwd_container.password(), address, spendkey, viewkey, testnet);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_json.empty())
    {
      std::string wallet_file, password; // we don't need to remember them
      if (!generate_from_json(vm, wallet_file, password))
        return false;
    }
    else
    {
      bool r = new_wallet(m_wallet_file, pwd_container.password(), m_recovery_key, m_restore_deterministic_wallet,
        m_non_deterministic, testnet, old_language);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password(), testnet);
    CHECK_AND_ASSERT_MES(r, false, tr("failed to open account"));
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
bool simple_wallet::handle_command_line(const boost::program_options::variables_map& vm)
{
  m_wallet_file                   = command_line::get_arg(vm, arg_wallet_file);
  m_generate_new                  = command_line::get_arg(vm, arg_generate_new_wallet);
  m_generate_from_view_key        = command_line::get_arg(vm, arg_generate_from_view_key);
  m_generate_from_keys            = command_line::get_arg(vm, arg_generate_from_keys);
  m_generate_from_json            = command_line::get_arg(vm, arg_generate_from_json);
  m_daemon_address                = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host                   = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port                   = command_line::get_arg(vm, arg_daemon_port);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  m_trusted_daemon                = command_line::get_arg(vm, arg_trusted_daemon);
  m_allow_mismatched_daemon_version = command_line::get_arg(vm, arg_allow_mismatched_daemon_version);
  m_restore_height                = command_line::get_arg(vm, arg_restore_height);
  m_restoring                     = !m_generate_from_view_key.empty() ||
                                    !m_generate_from_keys.empty() ||
                                    !m_generate_from_json.empty() ||
                                    m_restore_deterministic_wallet;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon(bool silent)
{
  bool same_version = false;
  if (!m_wallet->check_connection(&same_version))
  {
    if (!silent)
      fail_msg_writer() << tr("wallet failed to connect to daemon: ") << m_daemon_address << ". " <<
        tr("Daemon either is not started or wrong port was passed. "
        "Please make sure daemon is running or restart the wallet with the correct daemon address.");
    return false;
  }
  if (!m_allow_mismatched_daemon_version && !same_version)
  {
    if (!silent)
      fail_msg_writer() << tr("Daemon uses a different RPC version that the wallet: ") << m_daemon_address << ". " <<
        tr("Either update one of them, or use --allow-mismatched-daemon-version.");
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
    if (std::cin.eof())
      return std::string();
    try
    {
      language_number = std::stoi(language_choice);
      if (!((language_number >= 0) && (static_cast<unsigned int>(language_number) < language_list.size())))
      {
        language_number = -1;
        fail_msg_writer() << tr("invalid language choice passed. Please try again.\n");
      }
    }
    catch (std::exception &e)
    {
      fail_msg_writer() << tr("invalid language choice passed. Please try again.\n");
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
    if (mnemonic_language.empty())
      return false;
  }


  m_wallet_file=wallet_file;

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);
  m_wallet->set_seed_language(mnemonic_language);

  // for a totally new account, we don't care about older blocks.
  if (!m_restoring)
  {
    std::string err;
    m_wallet->set_refresh_from_block_height(get_daemon_blockchain_height(err));
  } else if (m_restore_height)
  {
    m_wallet->set_refresh_from_block_height(m_restore_height);
  }

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(wallet_file, password, recovery_key, recover, two_random);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    std::cout << tr("View key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
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
    tr("Your wallet has been generated!\n"
    "To start synchronizing with the daemon, use \"refresh\" command.\n"
    "Use \"help\" command to see the list of available commands.\n"
    "Always use \"exit\" command when closing monero-wallet-cli to save your\n"
    "current session's state. Otherwise, you might need to synchronize \n"
    "your wallet again (your wallet keys are NOT at risk in any case).\n")
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
  m_wallet_file=wallet_file;

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);
  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  try
  {
    m_wallet->generate(wallet_file, password, address, viewkey);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new watch-only wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
    std::cout << tr("View key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
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
bool simple_wallet::new_wallet(const std::string &wallet_file, const std::string& password, const cryptonote::account_public_address& address,
  const crypto::secret_key& spendkey, const crypto::secret_key& viewkey, bool testnet)
{
  m_wallet_file=wallet_file;

  m_wallet.reset(new tools::wallet2(testnet));
  m_wallet->callback(this);
  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  try
  {
    m_wallet->generate(wallet_file, password, address, spendkey, viewkey);
    message_writer(epee::log_space::console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
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

  m_wallet_file=wallet_file;
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
        if (mnemonic_language.empty())
          return false;
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
    // only suggest removing cache if the password was actually correct
    if (m_wallet->verify_password(password))
      fail_msg_writer() << boost::format(tr("You may want to remove the file \"%s\" and try again")) % wallet_file;
    return false;
  }

  m_wallet->init(m_daemon_address);

  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Use \"help\" command to see the list of available commands.\n") <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  if (m_idle_run.load(std::memory_order_relaxed))
  {
    m_idle_run.store(false, std::memory_order_relaxed);
    m_wallet->stop();
    {
      boost::unique_lock<boost::mutex> lock(m_idle_mutex);
      m_idle_cond.notify_one();
    }
    m_idle_thread.join();
  }

  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << tr("failed to deinitialize wallet");
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
    LOCK_IDLE_SCOPE();
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
  tools::password_container pwd_container(m_wallet_file.empty());

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
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());

  bool ok = true;
  size_t max_mining_threads_count = (std::max)(tools::get_max_concurrency(), static_cast<unsigned>(2));
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
    fail_msg_writer() << tr("blockchain can't be saved: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_new_block(uint64_t height, const cryptonote::block& block)
{
  if (!m_auto_refresh_refreshing)
    m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_received(uint64_t height, const cryptonote::transaction& tx, uint64_t amount)
{
  message_writer(epee::log_space::console_color_green, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(tx) << ", " <<
    tr("received ") << print_money(amount);
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx)
{
  message_writer(epee::log_space::console_color_magenta, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(spend_tx) << ", " <<
    tr("spent ") << print_money(amount);
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const cryptonote::transaction& tx)
{
  message_writer(epee::log_space::console_color_red, true) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << get_transaction_hash(tx) << ", " <<
    tr("unsupported transaction format");
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh_main(uint64_t start_height, bool reset)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

  if (reset)
    m_wallet->rescan_blockchain(false);

  message_writer() << tr("Starting refresh...");

  uint64_t fetched_blocks = 0;
  bool ok = false;
  std::ostringstream ss;
  try
  {
    m_in_manual_refresh.store(true, std::memory_order_relaxed);
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){m_in_manual_refresh.store(false, std::memory_order_relaxed);});
    m_wallet->refresh(start_height, fetched_blocks);
    ok = true;
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << tr("Refresh done, blocks received: ") << fetched_blocks;
    show_balance_unlocked();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    ss << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << tr("refresh error: ") << e.what();
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
    LOG_ERROR("unknown error");
    ss << tr("unknown error");
  }

  if (!ok)
  {
    fail_msg_writer() << tr("refresh failed: ") << ss.str() << ". " << tr("Blocks received: ") << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
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
  return refresh_main(start_height, false);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance_unlocked()
{
  success_msg_writer() << tr("Balance: ") << print_money(m_wallet->balance()) << ", "
    << tr("unlocked balance: ") << print_money(m_wallet->unlocked_balance());
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  LOCK_IDLE_SCOPE();
  show_balance_unlocked();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  LOCK_IDLE_SCOPE();

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
        message_writer() << boost::format("%21s%8s%12s%8s%16s%68s") % tr("amount") % tr("spent") % tr("unlocked") % tr("ringct") % tr("global index") % tr("tx id");
        transfers_found = true;
      }
      message_writer(td.m_spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        boost::format("%21s%8s%12s%8s%16u%68s") %
        print_money(td.amount()) %
        (td.m_spent ? tr("T") : tr("F")) %
        (m_wallet->is_transfer_unlocked(td) ? tr("unlocked") : tr("locked")) %
        (td.is_rct() ? tr("RingCT") : tr("-")) %
        td.m_global_output_index %
        td.m_txid;
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

  LOCK_IDLE_SCOPE();

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
      fail_msg_writer() << tr("payment ID has invalid format, expected 16 or 64 character hex string: ") << arg;
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
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  try
  {
    LOCK_IDLE_SCOPE();
    m_wallet->rescan_spent();
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::is_key_image_spent_error&)
  {
    fail_msg_writer() << tr("failed to get spent status");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << tr("unexpected error: ") << e.what();
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_address_from_str(const std::string &str, cryptonote::account_public_address &address, bool &has_payment_id, crypto::hash8 &payment_id)
{
    if(!get_account_integrated_address_from_str(address, has_payment_id, payment_id, m_wallet->testnet(), str))
    {
      // if treating as an address fails, try as url
      bool dnssec_ok = false;
      std::string url = str;

      // attempt to get address from dns query
      auto addresses_from_dns = tools::wallet2::addresses_from_url(url, dnssec_ok);

      // for now, move on only if one address found
      if (addresses_from_dns.size() == 1)
      {
        if (get_account_integrated_address_from_str(address, has_payment_id, payment_id, m_wallet->testnet(), addresses_from_dns[0]))
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
          if (std::cin.eof())
          {
            return false;
          }
          if (confirm_dns_ok != "Y" && confirm_dns_ok != "y" && confirm_dns_ok != "Yes" && confirm_dns_ok != "yes"
            && confirm_dns_ok != tr("yes") && confirm_dns_ok != tr("no"))
          {
            fail_msg_writer() << tr("you have cancelled the transfer request");
            return false;
          }
        }
        else
        {
          fail_msg_writer() << tr("failed to get a Monero address from: ") << url;
          return false;
        }
      }
      else if (addresses_from_dns.size() > 1)
      {
        fail_msg_writer() << tr("not yet supported: Multiple Monero addresses found for given URL: ") << url;
        return false;
      }
      else
      {
        fail_msg_writer() << tr("wrong address: ") << url;
        return false;
      }
    }

    return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_main(int transfer_type, const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

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
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
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
    if (!get_address_from_str(local_args[i], de.addr, has_payment_id, new_payment_id))
      return true;

    if (has_payment_id)
    {
      if (payment_id_seen)
      {
        fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[i];
        return true;
      }

      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, new_payment_id);
      bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      if(!r)
      {
        fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
        return true;
      }
      payment_id_seen = true;
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

  // prompt is there is no payment id and confirmation is required
  if (!payment_id_seen && m_wallet->confirm_missing_payment_id())
  {
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No)"));
     if (std::cin.eof())
       return true;
     if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
     {
       fail_msg_writer() << tr("transaction cancelled.");

       // would like to return false, because no tx made, but everything else returns true
       // and I don't know what returning false might adversely affect.  *sigh*
       return true; 
     }
  }

  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    switch (transfer_type)
    {
      case TransferNew:
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra, m_trusted_daemon);
      break;
      default:
        LOG_ERROR("Unknown transfer method, using original");
      case TransferOriginal:
        ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra, m_trusted_daemon);
        break;
    }

    // if more than one tx necessary, prompt user to confirm
    if (m_wallet->always_confirm_transfers() || ptx_vector.size() > 1)
    {
        uint64_t total_fee = 0;
        uint64_t dust_not_in_fee = 0;
        uint64_t dust_in_fee = 0;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          total_fee += ptx_vector[n].fee;

          if (ptx_vector[n].dust_added_to_fee)
            dust_in_fee += ptx_vector[n].dust;
          else
            dust_not_in_fee += ptx_vector[n].dust;
        }

        std::stringstream prompt;
        if (ptx_vector.size() > 1)
        {
          prompt << boost::format(tr("Your transaction needs to be split into %llu transactions.  "
            "This will result in a transaction fee being applied to each transaction, for a total fee of %s")) %
            ((unsigned long long)ptx_vector.size()) % print_money(total_fee);
        }
        else
        {
          prompt << boost::format(tr("The transaction fee is %s")) %
            print_money(total_fee);
        }
        if (dust_in_fee != 0) prompt << boost::format(tr(", of which %s is dust from change")) % print_money(dust_in_fee);
        if (dust_not_in_fee != 0)  prompt << tr(".") << ENDL << boost::format(tr("A total of %s from dust change will be sent to dust address")) 
                                                   % print_money(dust_not_in_fee);
        prompt << tr(".") << ENDL << tr("Is this okay?  (Y/Yes/N/No)");
        
        std::string accepted = command_line::input_line(prompt.str());
        if (std::cin.eof())
          return true;
        if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
        {
          fail_msg_writer() << tr("transaction cancelled.");

          // would like to return false, because no tx made, but everything else returns true
          // and I don't know what returning false might adversely affect.  *sigh*
          return true; 
        }
    }

    // actually commit the transactions
    if (m_wallet->watch_only())
    {
      bool r = m_wallet->save_tx(ptx_vector, "unsigned_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_monero_tx";
      }
    }
    else while (!ptx_vector.empty())
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
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
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
    fail_msg_writer() << tr("failed to find a suitable way to split transactions");
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
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  return transfer_main(TransferOriginal, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_new(const std::vector<std::string> &args_)
{
  return transfer_main(TransferNew, args_);
}
//----------------------------------------------------------------------------------------------------

bool simple_wallet::locked_transfer(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

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



  if(m_wallet->watch_only())
  {
    fail_msg_writer() << tr("this is a watch only wallet");
    return true;
  }

  int locked_blocks = 0;
  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  
  if (2 < local_args.size() && local_args.size() < 5)
  {
    if (local_args.size() == 4)
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
      payment_id_seen = true;
      if(!r)
      {
        fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
        return true;
      }

    }

    locked_blocks = std::stoi( local_args.back() );
    if (locked_blocks > 1000000) {
      fail_msg_writer() << tr("Locked blocks too high, max 1000000 (˜4 yrs)");
      return true;  
    }
    local_args.pop_back(); 
      
  }
  else
  {
    fail_msg_writer() << tr("Wrong number of arguments, use: locked_transfer [<mixin_count>] <addr> <amount> <lockblocks> [<payment_id>]");
    return true;
  }

  vector<cryptonote::tx_destination_entry> dsts;

  cryptonote::tx_destination_entry de;
  bool has_payment_id;
  crypto::hash8 new_payment_id;
  if (!get_address_from_str(local_args[0], de.addr, has_payment_id, new_payment_id))
    return true;

  bool ok = cryptonote::parse_amount(de.amount, local_args[1]);
  if(!ok || 0 == de.amount)
  {
    fail_msg_writer() << tr("amount is wrong: ") << local_args[0] << ' ' << local_args[1] <<
      ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
    return true;
  }

  dsts.push_back(de);


  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;

    std::string err;
    int bc_height = get_daemon_blockchain_height(err);
    int unlock_block = locked_blocks + bc_height;
    ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, unlock_block , 0 /* unused fee arg*/, extra, m_trusted_daemon);

    uint64_t total_fee = 0;
    uint64_t dust_not_in_fee = 0;
    uint64_t dust_in_fee = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;

      if (ptx_vector[n].dust_added_to_fee)
        dust_in_fee += ptx_vector[n].dust;
      else
        dust_not_in_fee += ptx_vector[n].dust;
    }

    std::stringstream prompt;
    if (ptx_vector.size() > 1)
    {
      prompt << boost::format(tr("Your transaction needs to be split into %llu transactions.  "
        "This will result in a transaction fee being applied to each transaction, for a total fee of %s")) %
        ((unsigned long long)ptx_vector.size()) % print_money(total_fee);
    }
    else
    {
      
      prompt << boost::format(tr("Transaction fee is %s")) %
        print_money(total_fee);
    }
    if (dust_in_fee != 0) prompt << boost::format(tr(", of which %s is dust from change")) % print_money(dust_in_fee);
    if (dust_not_in_fee != 0)  prompt << tr(".") << ENDL << boost::format(tr("A total of %s from dust change will be sent to dust address")) 
                                               % print_money(dust_not_in_fee);
    
    float days = (float)(locked_blocks) / 720.0;
    prompt << boost::format(tr(".\nThe unlock time is approximately %s days")) % days;

    prompt << tr(".") << ENDL << tr("Is this okay?  (Y/Yes/N/No)");
    
    std::string accepted = command_line::input_line(prompt.str());
    if (std::cin.eof())
      return true;
    if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
    {
      fail_msg_writer() << tr("transaction cancelled.");
      return true; 
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
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Not enough money to transfer.");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
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
    fail_msg_writer() << tr("failed to find a suitable way to split transactions");
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
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------

bool simple_wallet::sweep_unmixable(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();
  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_unmixable_sweep_transactions(m_trusted_daemon);

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No unmixable outputs found");
      return true;
    }

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = 0, total_unmixable = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;
      for (const auto &vin: ptx_vector[n].tx.vin)
      {
        if (vin.type() == typeid(txin_to_key))
          total_unmixable += boost::get<txin_to_key>(vin).amount;
      }
    }

    std::string prompt_str = tr("Sweeping ") + print_money(total_unmixable);
    if (ptx_vector.size() > 1) {
      prompt_str = (boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_unmixable) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee)).str();
    }
    else {
      prompt_str = (boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_unmixable) %
        print_money(total_fee)).str();
    }
    std::string accepted = command_line::input_line(prompt_str);
    if (std::cin.eof())
      return true;
    if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
    {
      fail_msg_writer() << tr("transaction cancelled.");

      // would like to return false, because no tx made, but everything else returns true
      // and I don't know what returning false might adversely affect.  *sigh*
      return true;
    }

    // actually commit the transactions
    if (m_wallet->watch_only())
    {
      bool r = m_wallet->save_tx(ptx_vector, "unsigned_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_monero_tx";
      }
    }
    else while (!ptx_vector.empty())
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
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
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
    fail_msg_writer() << tr("failed to find a suitable way to split transactions");
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
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_all(const std::vector<std::string> &args_)
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

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (2 == local_args.size())
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
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
      return true;
    }
    payment_id_seen = true;
  }

  if (local_args.size() == 0)
  {
    fail_msg_writer() << tr("No address given");
    return true;
  }

  bool has_payment_id;
  crypto::hash8 new_payment_id;
  cryptonote::account_public_address address;
  if (!get_address_from_str(local_args[0], address, has_payment_id, new_payment_id))
    return true;

  if (has_payment_id)
  {
    if (payment_id_seen)
    {
      fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[0];
      return true;
    }

    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, new_payment_id);
    bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    if(!r)
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
    payment_id_seen = true;
  }

  // prompt is there is no payment id and confirmation is required
  if (!payment_id_seen && m_wallet->confirm_missing_payment_id())
  {
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No)"));
     if (std::cin.eof())
       return true;
     if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
     {
       fail_msg_writer() << tr("transaction cancelled.");

       // would like to return false, because no tx made, but everything else returns true
       // and I don't know what returning false might adversely affect.  *sigh*
       return true; 
     }
  }

  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_transactions_all(address, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra, m_trusted_daemon);

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No outputs found");
      return true;
    }

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = 0, total_sent = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;
      for (const auto &vin: ptx_vector[n].tx.vin)
      {
        if (vin.type() == typeid(txin_to_key))
          total_sent += boost::get<txin_to_key>(vin).amount;
      }
    }

    std::string prompt_str;
    if (ptx_vector.size() > 1) {
      prompt_str = (boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_sent) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee)).str();
    }
    else {
      prompt_str = (boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_sent) %
        print_money(total_fee)).str();
    }
    std::string accepted = command_line::input_line(prompt_str);
    if (std::cin.eof())
      return true;
    if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
    {
      fail_msg_writer() << tr("transaction cancelled.");

      // would like to return false, because no tx made, but everything else returns true
      // and I don't know what returning false might adversely affect.  *sigh*
      return true;
    }

    // actually commit the transactions
    if (m_wallet->watch_only())
    {
      bool r = m_wallet->save_tx(ptx_vector, "unsigned_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_monero_tx";
      }
    }
    else while (!ptx_vector.empty())
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
    fail_msg_writer() << tr("daemon is busy. Please try again later.");
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << tr("no connection to daemon. Please make sure daemon is running.");
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("RPC error: " << e.to_string());
    fail_msg_writer() << tr("RPC error: ") << e.what();
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix");
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << tr("transaction was not constructed");
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) << e.status();
    std::string reason = e.reason();
    if (!reason.empty())
      fail_msg_writer() << tr("Reason: ") << reason;
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
    fail_msg_writer() << tr("failed to find a suitable way to split transactions");
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
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const tools::wallet2::unsigned_tx_set &txs)
{
  // gather info to ask the user
  uint64_t amount = 0, amount_to_dests = 0, change = 0;
  size_t min_mixin = ~0;
  std::unordered_map<std::string, uint64_t> dests;
  const std::string wallet_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());
  for (size_t n = 0; n < txs.txes.size(); ++n)
  {
    const tools::wallet2::tx_construction_data &cd = txs.txes[n];
    for (size_t s = 0; s < cd.sources.size(); ++s)
    {
      amount += cd.sources[s].amount;
      size_t mixin = cd.sources[s].outputs.size() - 1;
      if (mixin < min_mixin)
        min_mixin = mixin;
    }
    for (size_t d = 0; d < cd.destinations.size(); ++d)
    {
      const tx_destination_entry &entry = cd.destinations[d];
      std::string address = get_account_address_as_str(m_wallet->testnet(), entry.addr);
      std::unordered_map<std::string,uint64_t>::iterator i = dests.find(address);
      if (i == dests.end())
        dests.insert(std::make_pair(address, entry.amount));
      else
        i->second += entry.amount;
      amount_to_dests += entry.amount;
    }
    if (cd.change_dts.amount > 0)
    {
      dests.insert(std::make_pair(get_account_address_as_str(m_wallet->testnet(), cd.change_dts.addr), cd.change_dts.amount));
      amount_to_dests += cd.change_dts.amount;
      change += cd.change_dts.amount;
    }
  }
  std::string dest_string;
  for (std::unordered_map<std::string, uint64_t>::const_iterator i = dests.begin(); i != dests.end(); )
  {
    dest_string += (boost::format(tr("sending %s to %s")) % print_money(i->second) % i->first).str();
    ++i;
    if (i != dests.end())
      dest_string += ", ";
  }
  if (dest_string.empty())
    dest_string = tr("with no destinations");

  uint64_t fee = amount - amount_to_dests;
  std::string prompt_str = (boost::format(tr("Loaded %lu transactions, for %s, fee %s, change %s, %s, with min mixin %lu (full details in log file). Is this okay? (Y/Yes/N/No)")) % (unsigned long)txs.txes.size() % print_money(amount) % print_money(fee) % print_money(change) % dest_string % (unsigned long)min_mixin).str();
  std::string accepted = command_line::input_line(prompt_str);
  return is_it_true(accepted);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args_)
{
  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }

  try
  {
    bool r = m_wallet->sign_tx("unsigned_monero_tx", "signed_monero_tx", [&](const tools::wallet2::unsigned_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to sign transaction");
      return true;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to sign transaction: ") << e.what();
    return true;
  }

  success_msg_writer(true) << tr("Transaction successfully signed to file: ") << "signed_monero_tx";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  try
  {
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    bool r = m_wallet->load_tx("signed_monero_tx", ptx_vector);
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load transaction from file");
      return true;
    }

    // if more than one tx necessary, prompt user to confirm
    if (m_wallet->always_confirm_transfers())
    {
        uint64_t total_fee = 0;
        uint64_t dust_not_in_fee = 0;
        uint64_t dust_in_fee = 0;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          total_fee += ptx_vector[n].fee;

          if (ptx_vector[n].dust_added_to_fee)
            dust_in_fee += ptx_vector[n].dust;
          else
            dust_not_in_fee += ptx_vector[n].dust;
        }

        std::stringstream prompt;
        if (ptx_vector.size() > 1)
        {
          prompt << boost::format(tr("Your transaction needs to be split into %llu transactions.  "
            "This will result in a transaction fee being applied to each transaction, for a total fee of %s")) %
            ((unsigned long long)ptx_vector.size()) % print_money(total_fee);
        }
        else
        {
          prompt << boost::format(tr("The transaction fee is %s")) %
            print_money(total_fee);
        }
        if (dust_in_fee != 0) prompt << boost::format(tr(", of which %s is dust from change")) % print_money(dust_in_fee);
        if (dust_not_in_fee != 0)  prompt << tr(".") << ENDL << boost::format(tr("A total of %s from dust change will be sent to dust address"))
                                                   % print_money(dust_not_in_fee);
        prompt << tr(".") << ENDL << tr("Is this okay?  (Y/Yes/N/No)");

        std::string accepted = command_line::input_line(prompt.str());
        if (std::cin.eof())
          return true;
        if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
        {
          fail_msg_writer() << tr("transaction cancelled.");

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
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.second;
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
    fail_msg_writer() << tr("usage: get_tx_key <txid>");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  LOCK_IDLE_SCOPE();

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> amount_keys;
  if (m_wallet->get_tx_key(txid, tx_key))
  {
    success_msg_writer() << tr("Tx key: ") << epee::string_tools::pod_to_hex(tx_key);
    return true;
  }
  else
  {
    fail_msg_writer() << tr("no tx keys found for this txid");
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 3) {
    fail_msg_writer() << tr("usage: check_tx_key <txid> <txkey> <address>");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[0], txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  LOCK_IDLE_SCOPE();

  if (local_args[1].size() < 64 || local_args[1].size() % 64)
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  crypto::secret_key tx_key;
  cryptonote::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[1], tx_key_data))
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!get_account_integrated_address_from_str(address, has_payment_id, payment_id, m_wallet->testnet(), local_args[2]))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json_remote_command2(m_daemon_address + "/gettransactions", req, res, m_http_client) ||
      (res.txs.size() != 1 && res.txs_as_hex.size() != 1))
  {
    fail_msg_writer() << tr("failed to get transaction from daemon");
    return true;
  }
  cryptonote::blobdata tx_data;
  bool ok;
  if (res.txs.size() == 1)
    ok = string_tools::parse_hexstr_to_binbuff(res.txs.front().as_hex, tx_data);
  else
    ok = string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data);
  if (!ok)
  {
    fail_msg_writer() << tr("failed to parse transaction from daemon");
    return true;
  }
  crypto::hash tx_hash, tx_prefix_hash;
  cryptonote::transaction tx;
  if (!cryptonote::parse_and_validate_tx_from_blob(tx_data, tx, tx_hash, tx_prefix_hash))
  {
    fail_msg_writer() << tr("failed to validate transaction from daemon");
    return true;
  }
  if (tx_hash != txid)
  {
    fail_msg_writer() << tr("failed to get the right transaction from daemon");
    return true;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
  {
    fail_msg_writer() << tr("failed to generate key derivation from supplied parameters");
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
      {
        uint64_t amount;
        if (tx.version == 1)
        {
          amount = tx.vout[n].amount;
        }
        else
        {
          try
          {
            rct::key Ctmp;
            //rct::key amount_key = rct::hash_to_scalar(rct::scalarmultKey(rct::pk2rct(address.m_view_public_key), rct::sk2rct(tx_key)));
            crypto::key_derivation derivation;
            bool r = crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation);
            if (!r)
            {
              LOG_ERROR("Failed to generate key derivation to decode rct output " << n);
              amount = 0;
            }
            else
            {
              crypto::secret_key scalar1;
              crypto::derivation_to_scalar(derivation, n, scalar1);
              rct::ecdhTuple ecdh_info = tx.rct_signatures.ecdhInfo[n];
              rct::ecdhDecode(ecdh_info, rct::sk2rct(scalar1));
              rct::key C = tx.rct_signatures.outPk[n].mask;
              rct::addKeys2(Ctmp, ecdh_info.mask, ecdh_info.amount, rct::H);
              if (rct::equalKeys(C, Ctmp))
                amount = rct::h2d(ecdh_info.amount);
              else
                amount = 0;
            }
          }
          catch (...) { amount = 0; }
        }
        received += amount;
      }
    }
  }
  catch(const std::exception &e)
  {
    LOG_ERROR("error: " << e.what());
    fail_msg_writer() << tr("error: ") << e.what();
    return true;
  }

  if (received > 0)
  {
    success_msg_writer() << get_account_address_as_str(m_wallet->testnet(), address) << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << txid;
  }
  else
  {
    fail_msg_writer() << get_account_address_as_str(m_wallet->testnet(), address) << " " << tr("received nothing in txid") << " " << txid;
  }
  if (res.txs.front().in_pool)
  {
    success_msg_writer() << tr("WARNING: this transaction is not yet included in the blockchain!");
  }
  else
  {
    std::string err;
    uint64_t bc_height = get_daemon_blockchain_height(err);
    if (err.empty())
    {
      uint64_t confirmations = bc_height - (res.txs.front().block_height + 1);
      success_msg_writer() << boost::format(tr("This transaction has %u confirmations")) % confirmations;
    }
    else
    {
      success_msg_writer() << tr("WARNING: failed to determine number of confirmations!");
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
static std::string get_human_readable_timestamp(uint64_t ts)
{
  char buffer[64];
  if (ts < 1234567890)
    return "<unknown>";
  time_t tt = ts;
  struct tm tm;
#ifdef WIN32
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  uint64_t now = time(NULL);
  uint64_t diff = ts > now ? ts - now : now - ts;
  if (diff > 24*3600)
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
  else
    strftime(buffer, sizeof(buffer), "%I:%M:%S %p", &tm);
  return std::string(buffer);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfers(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;
  bool in = true;
  bool out = true;
  bool pending = true;
  bool failed = true;
  bool pool = true;
  uint64_t min_height = 0;
  uint64_t max_height = (uint64_t)-1;

  if(local_args.size() > 3) {
    fail_msg_writer() << tr("usage: show_transfers [in|out|all|pending|failed] [<min_height> [<max_height>]]");
    return true;
  }

  LOCK_IDLE_SCOPE();
  
  // optional in/out selector
  if (local_args.size() > 0) {
    if (local_args[0] == "in" || local_args[0] == "incoming") {
      out = pending = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "out" || local_args[0] == "outgoing") {
      in = pool = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pending") {
      in = out = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "failed") {
      in = out = pending = pool = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pool") {
      in = out = pending = failed = false;
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
      fail_msg_writer() << tr("bad min_height parameter:") << " " << local_args[0];
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
      fail_msg_writer() << tr("bad max_height parameter:") << " " << local_args[0];
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
      std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(true, (boost::format("%16.16s %20.20s %s %s %s %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % "-" % note).str())));
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
      std::string note = m_wallet->get_tx_note(i->first);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(false, (boost::format("%16.16s %20.20s %s %s %14.14s %s %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount_in - change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % dests % note).str())));
    }
  }

  // print in and out sorted by height
  for (std::map<uint64_t, std::pair<bool, std::string>>::const_iterator i = output.begin(); i != output.end(); ++i) {
    message_writer(i->second.first ? epee::log_space::console_color_green : epee::log_space::console_color_magenta, false) <<
      boost::format("%8.8llu %6.6s %s") %
      ((unsigned long long)i->first) % (i->second.first ? tr("in") : tr("out")) % i->second.second;
  }

  if (pool) {
    try
    {
      m_wallet->update_pool_state();
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
      m_wallet->get_unconfirmed_payments(payments);
      for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
          payment_id = payment_id.substr(0,16);
        std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
        message_writer() << (boost::format("%8.8s %6.6s %16.16s %20.20s %s %s %s %s") % "pool" % "in" % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % "-" % note).str();
      }
    }
    catch (...)
    {
      fail_msg_writer() << "Failed to get pool state";
    }
  }

  // print unconfirmed last
  if (pending || failed) {
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
      uint64_t amount = pd.m_amount_in;
      uint64_t fee = amount - pd.m_amount_out - pd.m_change;
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(i->first);
      bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;
      if ((failed && is_failed) || (!is_failed && pending)) {
        message_writer() << (boost::format("%8.8s %6.6s %16.16s %20.20s %s %s %14.14s %s") % (is_failed ? tr("failed") : tr("pending")) % tr("out") % get_human_readable_timestamp(pd.m_timestamp) % print_money(amount - pd.m_change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % note).str();
      }
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::rescan_blockchain(const std::vector<std::string> &args_)
{
  return refresh_main(0, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::wallet_idle_thread()
{
  while (true)
  {
    boost::unique_lock<boost::mutex> lock(m_idle_mutex);
    if (!m_idle_run.load(std::memory_order_relaxed))
      break;

    // auto refresh
    if (m_auto_refresh_enabled)
    {
      m_auto_refresh_refreshing = true;
      try
      {
        uint64_t fetched_blocks;
        if (try_connect_to_daemon(true))
          m_wallet->refresh(0, fetched_blocks);
      }
      catch(...) {}
      m_auto_refresh_refreshing = false;
    }

    if (!m_idle_run.load(std::memory_order_relaxed))
      break;
    m_idle_cond.wait_for(lock, boost::chrono::seconds(90));
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  // check and display warning, but go on anyway
  try_connect_to_daemon();

  refresh_main(0, false);

  m_auto_refresh_enabled = m_wallet->auto_refresh();
  m_idle_thread = boost::thread([&]{wallet_idle_thread();});

  std::string addr_start = m_wallet->get_account().get_public_address_str(m_wallet->testnet()).substr(0, 6);
  message_writer(epee::log_space::console_color_green, false) << "Background refresh thread started";
  return m_cmd_binder.run_handling(std::string("[") + tr("wallet") + " " + addr_start + "]: ", "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();

  m_idle_run.store(false, std::memory_order_relaxed);
  m_wallet->stop();
  // make the background refresh thread quit
  boost::unique_lock<boost::mutex> lock(m_idle_mutex);
  m_idle_cond.notify_one();
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
    fail_msg_writer() << tr("usage: integrated_address [payment ID]");
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
        success_msg_writer() << boost::format(tr("Integrated address: account %s, payment ID %s")) %
          get_account_address_as_str(m_wallet->testnet(),addr) % epee::string_tools::pod_to_hex(payment_id);
      }
      else
      {
        success_msg_writer() << tr("Standard address: ") << get_account_address_as_str(m_wallet->testnet(),addr);
      }
      return true;
    }
  }
  fail_msg_writer() << tr("failed to parse payment ID or address");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_tx_note(const std::vector<std::string> &args)
{
  if (args.size() == 0)
  {
    fail_msg_writer() << tr("usage: set_tx_note [txid] free text note");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  std::string note = "";
  for (size_t n = 1; n < args.size(); ++n)
  {
    if (n > 1)
      note += " ";
    note += args[n];
  }
  m_wallet->set_tx_note(txid, note);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_note(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: get_tx_note [txid]");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  std::string note = m_wallet->get_tx_note(txid);
  if (note.empty())
    success_msg_writer() << "no note found";
  else
    success_msg_writer() << "note found: " << note;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::status(const std::vector<std::string> &args)
{
  uint64_t local_height = m_wallet->get_blockchain_current_height();
  if (!try_connect_to_daemon())
  {
    success_msg_writer() << "Refreshed " << local_height << "/?, no daemon connected";
    return true;
  }

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
  {
    bool synced = local_height == bc_height;
    success_msg_writer() << "Refreshed " << local_height << "/" << bc_height << ", " << (synced ? "synced" : "syncing");
  }
  else
  {
    fail_msg_writer() << "Refreshed " << local_height << "/?, daemon connection error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: sign <filename>");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot sign");
    return true;
  }
  std::string filename = args[0];
  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }
  std::string signature = m_wallet->sign(data);
  success_msg_writer() << signature;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::verify(const std::vector<std::string> &args)
{
  if (args.size() != 3)
  {
    fail_msg_writer() << tr("usage: verify <filename> <address> <signature>");
    return true;
  }
  std::string filename = args[0];
  std::string address_string = args[1];
  std::string signature= args[2];

  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!get_account_integrated_address_from_str(address, has_payment_id, payment_id, m_wallet->testnet(), address_string))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  r = m_wallet->verify(data, address, signature);
  if (!r)
  {
    fail_msg_writer() << tr("Bad signature from ") << address_string;
  }
  else
  {
    success_msg_writer() << tr("Good signature from ") << address_string;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_key_images(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_key_images <filename>");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot export key images");
    return true;
  }
  std::string filename = args[0];

  try
  {
    std::vector<std::pair<crypto::key_image, crypto::signature>> ski = m_wallet->export_key_images();
    std::string data(KEY_IMAGE_EXPORT_FILE_MAGIC, strlen(KEY_IMAGE_EXPORT_FILE_MAGIC));
    const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
    data += std::string((const char *)&keys.m_spend_public_key, sizeof(crypto::public_key));
    data += std::string((const char *)&keys.m_view_public_key, sizeof(crypto::public_key));
    for (const auto &i: ski)
    {
      data += std::string((const char *)&i.first, sizeof(crypto::key_image));
      data += std::string((const char *)&i.second, sizeof(crypto::signature));
    }
    bool r = epee::file_io_utils::save_string_to_file(filename, data);
    if (!r)
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    fail_msg_writer() << "Error exporting key images: " << e.what();
    return true;
  }

  success_msg_writer() << tr("Signed key images exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_key_images(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: import_key_images <filename>");
    return true;
  }
  std::string filename = args[0];

  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }
  const size_t magiclen = strlen(KEY_IMAGE_EXPORT_FILE_MAGIC);
  if (data.size() < magiclen || memcmp(data.data(), KEY_IMAGE_EXPORT_FILE_MAGIC, magiclen))
  {
    fail_msg_writer() << "Bad key image export file magic in " << filename;
    return true;
  }
  const size_t headerlen = magiclen + 2 * sizeof(crypto::public_key);
  if (data.size() < headerlen)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[magiclen];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[magiclen + sizeof(crypto::public_key)];
  const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
  if (public_spend_key != keys.m_spend_public_key || public_view_key != keys.m_view_public_key)
  {
    fail_msg_writer() << "Key images from " << filename << " are for a different account";
    return true;
  }

  const size_t record_size = sizeof(crypto::key_image) + sizeof(crypto::signature);
  if ((data.size() - headerlen) % record_size)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  size_t nki = (data.size() - headerlen) / record_size;

  std::vector<std::pair<crypto::key_image, crypto::signature>> ski;
  ski.reserve(nki);
  for (size_t n = 0; n < nki; ++n)
  {
    crypto::key_image key_image = *reinterpret_cast<const crypto::key_image*>(&data[headerlen + n * record_size]);
    crypto::signature signature = *reinterpret_cast<const crypto::signature*>(&data[headerlen + n * record_size + sizeof(crypto::key_image)]);

    ski.push_back(std::make_pair(key_image, signature));
  }

  try
  {
    uint64_t spent = 0, unspent = 0;
    uint64_t height = m_wallet->import_key_images(ski, spent, unspent);
    success_msg_writer() << "Signed key images imported to height " << height << ", "
        << print_money(spent) << " spent, " << print_money(unspent) << " unspent";
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import key images: " << e.what();
    return true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_command(const std::vector<std::string> &args)
{
  return m_cmd_binder.process_command_vec(args);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::interrupt()
{
  if (m_in_manual_refresh.load(std::memory_order_relaxed))
  {
    m_wallet->stop();
  }
  else
  {
    stop();
  }
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#ifdef WIN32
  _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

  //TRY_ENTRY();

  std::string lang = i18n_get_language();
  tools::sanitize_locale();
  tools::set_strict_default_file_permissions(true);

  string_tools::set_module_name_and_folder(argv[0]);

  po::options_description desc_general(sw::tr("General options"));
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params(sw::tr("Wallet options"));
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_generate_from_keys);
  command_line::add_arg(desc_params, arg_generate_from_json);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_password_file);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);
  command_line::add_arg(desc_params, arg_max_concurrency);

  bf::path default_log {log_space::log_singletone::get_default_log_folder()};
  std::string log_file_name = log_space::log_singletone::get_default_log_file();
  if (log_file_name.empty())
  {
    // Sanity check: File path should also be empty if file name is. If not,
    // this would be a problem in epee's discovery of current process's file
    // path.
    if (! default_log.empty())
    {
      fail_msg_writer() << sw::tr("unexpected empty log file name in presence of non-empty file path");
      return false;
    }
    // epee didn't find path to executable from argv[0], so use this default file name.
    log_file_name = "monero-wallet-cli.log";
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
  command_line::add_arg(desc_params, arg_allow_mismatched_daemon_version);
  command_line::add_arg(desc_params, arg_restore_height);
  tools::wallet_rpc_server::init_options(desc_params);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  i18n_set_language("translations", "monero", lang);

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  cryptonote::simple_wallet w;
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      success_msg_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
      success_msg_writer() << sw::tr("Usage:") << " monero-wallet-cli [--wallet-file=<file>|--generate-new-wallet=<file>] [--daemon-address=<host>:<port>] [<COMMAND>]";
      success_msg_writer() << desc_all;
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      success_msg_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
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
  //   default: < argv[0] directory >/monero-wallet-cli.log
  //     so if ran as "monero-wallet-cli" (no path), log file will be in cwd
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

  if(command_line::has_arg(vm, arg_max_concurrency))
    tools::set_max_concurrency(command_line::get_arg(vm, arg_max_concurrency));

  message_writer(epee::log_space::console_color_white, true) << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";

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

    bool testnet = command_line::get_arg(vm, arg_testnet);
    bool restricted = command_line::get_arg(vm, arg_restricted);
    std::string wallet_file     = command_line::get_arg(vm, arg_wallet_file);
 
    tools::password_container pwd_container(wallet_file.empty());
    if (!cryptonote::simple_wallet::get_password(vm, false, pwd_container))
      return 1;
    std::string daemon_address  = command_line::get_arg(vm, arg_daemon_address);
    std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
    int daemon_port = command_line::get_arg(vm, arg_daemon_port);
    if (daemon_host.empty())
      daemon_host = "localhost";
    if (!daemon_port)
      daemon_port = testnet ? config::testnet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    std::string password;
    const std::string gfj = command_line::get_arg(vm, arg_generate_from_json);
    if (!gfj.empty()) {
      if (!w.generate_from_json(vm, wallet_file, password))
        return 1;
    }
    else {
      password = pwd_container.password();
    }

    tools::wallet2 wal(testnet,restricted);
    bool quit = false;
    tools::signal_handler::install([&wal, &quit](int) {
      quit = true;
      wal.stop();
    });
    try
    {
      LOG_PRINT_L0(sw::tr("Loading wallet..."));
      wal.load(wallet_file, password);
      wal.init(daemon_address);
      wal.refresh();
      // if we ^C during potentially length load/refresh, there's no server loop yet
      if (quit)
      {
        LOG_PRINT_L0(sw::tr("Storing wallet..."));
        wal.store();
        LOG_PRINT_GREEN(sw::tr("Stored ok"), LOG_LEVEL_0);
        return 1;
      }
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
    tools::signal_handler::install([&wrpc, &wal](int) {
      wrpc.send_stop_signal();
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
      tools::signal_handler::install([&w](int type) {
#ifdef WIN32
        if (type == CTRL_C_EVENT)
#else
        if (type == SIGINT)
#endif
        {
          // if we're pressing ^C when refreshing, just stop refreshing
          w.interrupt();
        }
        else
        {
          w.stop();
        }
      });
      w.run();

      w.deinit();
    }
  }
  return 0;
  //CATCH_ENTRY_L0("main", 1);
}
