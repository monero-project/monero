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

#include <thread>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include "include_base_utils.h"
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
#include "crypto/electrum-words.h"
extern "C"
{
#include "crypto/keccak.h"
#include "crypto/crypto-ops.h"
}
#if defined(WIN32)
#include <crtdbg.h>
#endif

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;

#define EXTENDED_LOGS_FILE "wallet_details.log"


namespace
{
  const command_line::arg_descriptor<std::string> arg_wallet_file = {"wallet-file", "Use wallet <arg>", ""};
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", "Generate new wallet and save it to <arg> or <address>.wallet by default", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", "Use daemon instance at <host>:<port>", ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", "Use daemon instance at host <arg> instead of localhost", ""};
  const command_line::arg_descriptor<std::string> arg_password = {"password", "Wallet password", "", true};
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", "Specify electrum seed for wallet recovery/creation", ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", "Recover wallet using electrum-style mnemonic", false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", "creates non-deterministic view and spend keys", false};
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", "Use daemon instance at port <arg> instead of 8081", 0};
  const command_line::arg_descriptor<uint32_t> arg_log_level = {"set_log", "", 0, true};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  inline std::string interpret_rpc_response(bool ok, const std::string& status)
  {
    std::string err;
    if (ok)
    {
      if (status == CORE_RPC_STATUS_BUSY)
      {
        err = "daemon is busy. Please try later";
      }
      else if (status != CORE_RPC_STATUS_OK)
      {
        err = status;
      }
    }
    else
    {
      err = "possible lost connection to daemon";
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
    return message_writer(epee::log_space::console_color_red, true, "Error: ", LOG_LEVEL_0);
  }
}


std::string simple_wallet::get_commands_str()
{
  std::stringstream ss;
  ss << "Commands: " << ENDL;
  std::string usage = m_cmd_binder.get_usage();
  boost::replace_all(usage, "\n", "\n  ");
  usage.insert(0, "  ");
  ss << usage << ENDL;
  return ss.str();
}

bool simple_wallet::seed(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  std::string electrum_words;
  crypto::ElectrumWords::bytes_to_words(m_wallet->get_account().get_keys().m_spend_secret_key, electrum_words);

  crypto::secret_key second;
  keccak((uint8_t *)&m_wallet->get_account().get_keys().m_spend_secret_key, sizeof(crypto::secret_key), (uint8_t *)&second, sizeof(crypto::secret_key));

  sc_reduce32((uint8_t *)&second);
  
  if (memcmp(second.data,m_wallet->get_account().get_keys().m_view_secret_key.data, sizeof(crypto::secret_key))==0) 
  {
    success_msg_writer(true) << "\nPLEASE NOTE: the following 24 words can be used to recover access to your wallet. Please write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.\n";
    std::cout << electrum_words << std::endl;      
  }
  else
  {
      fail_msg_writer() << "The wallet is non-deterministic. Cannot display seed.";
  }
  return true;
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
  m_cmd_binder.set_handler("start_mining", boost::bind(&simple_wallet::start_mining, this, _1), "start_mining [<number_of_threads>] - Start mining in daemon");
  m_cmd_binder.set_handler("stop_mining", boost::bind(&simple_wallet::stop_mining, this, _1), "Stop mining in daemon");
  m_cmd_binder.set_handler("save_bc", boost::bind(&simple_wallet::save_bc, this, _1), "Save current blockchain data");
  m_cmd_binder.set_handler("refresh", boost::bind(&simple_wallet::refresh, this, _1), "Resynchronize transactions and balance");
  m_cmd_binder.set_handler("balance", boost::bind(&simple_wallet::show_balance, this, _1), "Show current wallet balance");
  m_cmd_binder.set_handler("incoming_transfers", boost::bind(&simple_wallet::show_incoming_transfers, this, _1), "incoming_transfers [available|unavailable] - Show incoming transfers - all of them or filter them by availability");
  m_cmd_binder.set_handler("payments", boost::bind(&simple_wallet::show_payments, this, _1), "payments <payment_id_1> [<payment_id_2> ... <payment_id_N>] - Show payments <payment_id_1>, ... <payment_id_N>");
  m_cmd_binder.set_handler("bc_height", boost::bind(&simple_wallet::show_blockchain_height, this, _1), "Show blockchain height");
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer, this, _1), "transfer <mixin_count> <addr_1> <amount_1> [<addr_2> <amount_2> ... <addr_N> <amount_N>] [payment_id] - Transfer <amount_1>,... <amount_N> to <address_1>,... <address_N>, respectively. <mixin_count> is the number of transactions yours is indistinguishable from (from 0 to maximum available)");
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), "set_log <level> - Change current log detalization level, <level> is a number 0-4");
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), "Show current wallet public address");
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), "Save wallet synchronized data");
  m_cmd_binder.set_handler("seed", boost::bind(&simple_wallet::seed, this, _1), "Get deterministic seed");
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), "Show this help");
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1)
  {
    fail_msg_writer() << "use: set_log <log_level_number_0-4>";
    return true;
  }
  uint16_t l = 0;
  if(!epee::string_tools::get_xtype_from_string(l, args[0]))
  {
    fail_msg_writer() << "wrong number format, use: set_log <log_level_number_0-4>";
    return true;
  }
  if(LOG_LEVEL_4 < l)
  {
    fail_msg_writer() << "wrong number range, use: set_log <log_level_number_0-4>";
    return true;
  }

  log_space::log_singletone::get_set_log_detalisation_level(true, l);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::ask_wallet_create_if_needed()
{
  std::string wallet_path;

  wallet_path = command_line::input_line(
      "Specify wallet file name (e.g., wallet.bin). If the wallet doesn't exist, it will be created.\n"
      "Wallet file name: "
  );

  bool keys_file_exists;
  bool wallet_file_exists;
  tools::wallet2::wallet_exists(wallet_path, keys_file_exists, wallet_file_exists);

  // add logic to error out if new wallet requested but named wallet file exists
  if (keys_file_exists || wallet_file_exists)
  {
    if (!m_generate_new.empty() || m_restore_deterministic_wallet)
    {
      fail_msg_writer() << "Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
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
      std::cout << "The wallet doesn't exist, generating new one" << std::endl;
      m_generate_new = wallet_path;
      r = true;
    }else
    {
      fail_msg_writer() << "failed to open wallet \"" << wallet_path << "\". Keys file wasn't found";
      r = false;
    }
  }

  return r;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  handle_command_line(vm);

  if (!m_daemon_address.empty() && !m_daemon_host.empty() && 0 != m_daemon_port)
  {
    fail_msg_writer() << "you can't specify daemon host or port several times";
    return false;
  }

  if(!m_generate_new.empty() && !m_wallet_file.empty())
  {
    fail_msg_writer() << "Specifying both --generate-new-wallet=\"wallet_name\" and --wallet-file=\"wallet_name\" doesn't make sense!";
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  if (m_daemon_host.empty())
    m_daemon_host = "localhost";
  if (!m_daemon_port)
    m_daemon_port = RPC_DEFAULT_PORT;
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
      fail_msg_writer() << "failed to read wallet password";
      return false;
    }
  }

  if (!m_generate_new.empty() || m_restore_deterministic_wallet)
  {
    if (m_wallet_file.empty()) m_wallet_file = m_generate_new;  // alias for simplicity later

    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << "Cannot specify both --restore-deterministic-wallet and --non-deterministic";
        return false;
      }

      if (m_electrum_seed.empty())
      {
        m_electrum_seed = command_line::input_line("Specify electrum seed: ");
        if (m_electrum_seed.empty())
        {
          fail_msg_writer() << "specify a recovery parameter with the --electrum-seed=\"words list here\"";
          return false;
        }
      }

      if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key))
      {
          fail_msg_writer() << "electrum-style word list failed verification";
          return false;
      }
    }
    bool r = new_wallet(m_wallet_file, pwd_container.password(), m_recovery_key, m_restore_deterministic_wallet, m_non_deterministic);
    CHECK_AND_ASSERT_MES(r, false, "account creation failed");
  }
  else
  {
    bool r = open_wallet(m_wallet_file, pwd_container.password());
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
  m_daemon_address                = command_line::get_arg(vm, arg_daemon_address);
  m_daemon_host                   = command_line::get_arg(vm, arg_daemon_host);
  m_daemon_port                   = command_line::get_arg(vm, arg_daemon_port);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon()
{
  if (!m_wallet->check_connection())
  {
    fail_msg_writer() << "wallet failed to connect to daemon (" << m_daemon_address << "). " <<
      "Daemon either is not started or passed wrong port. " <<
      "Please, make sure that daemon is running or restart the wallet with correct daemon address.";
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const string &wallet_file, const std::string& password, const crypto::secret_key& recovery_key, bool recover, bool two_random)
{
  m_wallet_file = wallet_file;

  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this);

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(wallet_file, password, recovery_key, recover, two_random);
    message_writer(epee::log_space::console_color_white, true) << "Generated new wallet: " << m_wallet->get_account().get_public_address_str() << std::endl << "view key: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to generate new wallet: " << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  // convert rng value to electrum-style word list
  std::string electrum_words;
  crypto::ElectrumWords::bytes_to_words(recovery_val, electrum_words);

  std::string print_electrum = "";


  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Your wallet has been generated.\n" <<
    "To start synchronizing with the daemon use \"refresh\" command.\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "Always use \"exit\" command when closing simplewallet to save\n" <<
    "current session's state. Otherwise, you will possibly need to synchronize \n" <<
    "your wallet again. Your wallet key is NOT under risk anyway.\n"
  ;

  if (!two_random)
  {
    success_msg_writer(true) << "\nPLEASE NOTE: the following 24 words can be used to recover access to your wallet. Please write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.\n";
    std::cout << electrum_words << std::endl;
  }
  success_msg_writer() << "**********************************************************************";

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const string &wallet_file, const std::string& password)
{
  m_wallet_file = wallet_file;
  m_wallet.reset(new tools::wallet2());
  m_wallet->callback(this);

  try
  {
    m_wallet->load(m_wallet_file, password);
    message_writer(epee::log_space::console_color_white, true) << "Opened wallet: " << m_wallet->get_account().get_public_address_str();
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << "failed to load wallet: " << e.what();
    return false;
  }

  m_wallet->init(m_daemon_address);

  refresh(std::vector<std::string>());
  success_msg_writer() <<
    "**********************************************************************\n" <<
    "Use \"help\" command to see the list of available commands.\n" <<
    "**********************************************************************";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  bool r = m_wallet->deinit();
  if (!r)
  {
    fail_msg_writer() << "failed to deinit wallet";
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
    success_msg_writer() << "Wallet data saved";
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  COMMAND_RPC_START_MINING::request req;
  req.miner_address = m_wallet->get_account().get_public_address_str();

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
    fail_msg_writer() << "invalid arguments. Please use start_mining [<number_of_threads>], " <<
      "<number_of_threads> should be from 1 to " << max_mining_threads_count;
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = net_utils::invoke_http_json_remote_command2(m_daemon_address + "/start_mining", req, res, m_http_client);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << "Mining started in daemon";
  else
    fail_msg_writer() << "mining has NOT been started: " << err;
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
    success_msg_writer() << "Mining stopped in daemon";
  else
    fail_msg_writer() << "mining has NOT been stopped: " << err;
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
    success_msg_writer() << "Blockchain saved";
  else
    fail_msg_writer() << "Blockchain can't be saved: " << err;
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
    "Height " << height <<
    ", transaction " << get_transaction_hash(tx) <<
    ", received " << print_money(tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const cryptonote::transaction& in_tx, size_t out_index, const cryptonote::transaction& spend_tx)
{
  message_writer(epee::log_space::console_color_magenta, false) <<
    "Height " << height <<
    ", transaction " << get_transaction_hash(spend_tx) <<
    ", spent " << print_money(in_tx.vout[out_index].amount);
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const cryptonote::transaction& tx)
{
  message_writer(epee::log_space::console_color_red, true) <<
    "Height " << height <<
    ", transaction " << get_transaction_hash(tx) <<
    ", unsupported transaction format";
  m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  message_writer() << "Starting refresh...";

  size_t fetched_blocks = 0;
  size_t start_height = 0;
  if(!args.empty()){
    try
    {
        start_height = boost::lexical_cast<int>( args[0] );
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
    success_msg_writer(true) << "Refresh done, blocks received: " << fetched_blocks;
    show_balance();
  }
  catch (const tools::error::daemon_busy&)
  {
    ss << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    ss << "no connection to daemon. Please, make sure daemon is running";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    ss << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::refresh_error& e)
  {
    LOG_ERROR("refresh error: " << e.to_string());
    ss << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    ss << "internal error: " << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    ss << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    ss << "unknown error";
  }

  if (!ok)
  {
    fail_msg_writer() << "refresh failed: " << ss.str() << ". Blocks received: " << fetched_blocks;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << "balance: " << print_money(m_wallet->balance()) << ", unlocked balance: " << print_money(m_wallet->unlocked_balance());
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
        message_writer() << "        amount       \tspent\tglobal index\t                              tx id";
        transfers_found = true;
      }
      message_writer(td.m_spent ? epee::log_space::console_color_magenta : epee::log_space::console_color_green, false) <<
        std::setw(21) << print_money(td.amount()) << '\t' <<
        std::setw(3) << (td.m_spent ? 'T' : 'F') << "  \t" <<
        std::setw(12) << td.m_global_output_index << '\t' <<
        get_transaction_hash(td.m_tx);
    }
  }

  if (!transfers_found)
  {
    if (!filter)
    {
      success_msg_writer() << "No incoming transfers";
    }
    else if (available)
    {
      success_msg_writer() << "No incoming available transfers";
    }
    else
    {
      success_msg_writer() << "No incoming unavailable transfers";
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    fail_msg_writer() << "expected at least one payment_id";
    return true;
  }

  message_writer() << "                            payment                             \t" <<
    "                          transaction                           \t" <<
    "  height\t       amount        \tunlock time";

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
        success_msg_writer() << "No payments with id " << payment_id;
        continue;
      }

      for (const tools::wallet2::payment_details& pd : payments)
      {
        if(!payments_found)
        {
          payments_found = true;
        }
        success_msg_writer(true) <<
          payment_id << '\t' <<
          pd.m_tx_hash << '\t' <<
          std::setw(8)  << pd.m_block_height << '\t' <<
          std::setw(21) << print_money(pd.m_amount) << '\t' <<
          pd.m_unlock_time;
      }
    }
    else
    {
      fail_msg_writer() << "payment id has invalid format: \"" << arg << "\", expected 64-character string";
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
    fail_msg_writer() << "failed to get blockchain height: " << err;
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;
  if(local_args.size() < 3)
  {
    fail_msg_writer() << "wrong number of arguments, expected at least 3, got " << local_args.size();
    return true;
  }

  size_t fake_outs_count;
  if(!epee::string_tools::get_xtype_from_string(fake_outs_count, local_args[0]))
  {
    fail_msg_writer() << "mixin_count should be non-negative integer, got " << local_args[0];
    return true;
  }
  local_args.erase(local_args.begin());

  std::vector<uint8_t> extra;
  if (1 == local_args.size() % 2)
  {
    std::string payment_id_str = local_args.back();
    local_args.pop_back();

    crypto::hash payment_id;
    bool r = tools::wallet2::parse_payment_id(payment_id_str, payment_id);
    if(r)
    {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    }

    if(!r)
    {
      fail_msg_writer() << "payment id has invalid format: \"" << payment_id_str << "\", expected 64-character string";
      return true;
    }
  }

  vector<cryptonote::tx_destination_entry> dsts;
  for (size_t i = 0; i < local_args.size(); i += 2)
  {
    cryptonote::tx_destination_entry de;
    if(!get_account_address_from_str(de.addr, local_args[i]))
    {
      fail_msg_writer() << "wrong address: " << local_args[i];
      return true;
    }

    bool ok = cryptonote::parse_amount(de.amount, local_args[i + 1]);
    if(!ok || 0 == de.amount)
    {
      fail_msg_writer() << "amount is wrong: " << local_args[i] << ' ' << local_args[i + 1] <<
        ", expected number from 0 to " << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }

    dsts.push_back(de);
  }

  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, DEFAULT_FEE, extra);

    // if more than one tx necessary, prompt user to confirm
    if (ptx_vector.size() > 1)
    {
        std::string prompt_str = "Your transaction needs to be split into ";
        prompt_str += std::to_string(ptx_vector.size());
        prompt_str += " transactions.  This will result in a fee of ";
        prompt_str += print_money(ptx_vector.size() * DEFAULT_FEE);
        prompt_str += ".  Is this okay?  (Y/Yes/N/No)";
        std::string accepted = command_line::input_line(prompt_str);
        if (accepted != "Y" && accepted != "y" && accepted != "Yes" && accepted != "yes")
        {
          fail_msg_writer() << "Transaction cancelled.";

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
      success_msg_writer(true) << "Money successfully sent, transaction " << get_transaction_hash(ptx.tx);

      // if no exception, remove element from vector
      ptx_vector.pop_back();
    }
  }
  catch (const tools::error::daemon_busy&)
  {
    fail_msg_writer() << "daemon is busy. Please try later";
  }
  catch (const tools::error::no_connection_to_daemon&)
  {
    fail_msg_writer() << "no connection to daemon. Please, make sure daemon is running.";
  }
  catch (const tools::error::wallet_rpc_error& e)
  {
    LOG_ERROR("Unknown RPC error: " << e.to_string());
    fail_msg_writer() << "RPC error \"" << e.what() << '"';
  }
  catch (const tools::error::get_random_outs_error&)
  {
    fail_msg_writer() << "failed to get random outputs to mix";
  }
  catch (const tools::error::not_enough_money& e)
  {
    fail_msg_writer() << "not enough money to transfer, available only " << print_money(e.available()) <<
      ", transaction amount " << print_money(e.tx_amount() + e.fee()) << " = " << print_money(e.tx_amount()) <<
      " + " << print_money(e.fee()) << " (fee)";
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << "not enough outputs for specified mixin_count = " << e.mixin_count() << ":";
    for (const cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs())
    {
      writer << "\noutput amount = " << print_money(outs_for_amount.amount) << ", fount outputs to mix = " << outs_for_amount.outs.size();
    }
  }
  catch (const tools::error::tx_not_constructed&)
  {
    fail_msg_writer() << "transaction was not constructed";
  }
  catch (const tools::error::tx_rejected& e)
  {
    fail_msg_writer() << "transaction " << get_transaction_hash(e.tx()) << " was rejected by daemon with status \"" << e.status() << '"';
  }
  catch (const tools::error::tx_sum_overflow& e)
  {
    fail_msg_writer() << e.what();
  }
  catch (const tools::error::zero_destination&)
  {
    fail_msg_writer() << "one of destinations is zero";
  }
  catch (const tools::error::tx_too_big& e)
  {
    fail_msg_writer() << "Failed to find a suitable way to split transactions";
  }
  catch (const tools::error::transfer_error& e)
  {
    LOG_ERROR("unknown transfer error: " << e.to_string());
    fail_msg_writer() << "unknown transfer error: " << e.what();
  }
  catch (const tools::error::wallet_internal_error& e)
  {
    LOG_ERROR("internal error: " << e.to_string());
    fail_msg_writer() << "internal error: " << e.what();
  }
  catch (const std::exception& e)
  {
    LOG_ERROR("unexpected error: " << e.what());
    fail_msg_writer() << "unexpected error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR("Unknown error");
    fail_msg_writer() << "unknown error";
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  std::string addr_start = m_wallet->get_account().get_public_address_str().substr(0, 6);
  return m_cmd_binder.run_handling("[wallet " + addr_start + "]: ", "");
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
  success_msg_writer() << m_wallet->get_account().get_public_address_str();
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

  po::options_description desc_general("General options");
  command_line::add_arg(desc_general, command_line::arg_help);
  command_line::add_arg(desc_general, command_line::arg_version);

  po::options_description desc_params("Wallet options");
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_password);
  command_line::add_arg(desc_params, arg_daemon_address);
  command_line::add_arg(desc_params, arg_daemon_host);
  command_line::add_arg(desc_params, arg_daemon_port);
  command_line::add_arg(desc_params, arg_command);
  command_line::add_arg(desc_params, arg_log_level);
  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_electrum_seed );
  tools::wallet_rpc_server::init_options(desc_params);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  po::options_description desc_all;
  desc_all.add(desc_general).add(desc_params);
  cryptonote::simple_wallet w;
  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_all, [&]()
  {
    po::store(command_line::parse_command_line(argc, argv, desc_general, true), vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      success_msg_writer() << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG;
      success_msg_writer() << "Usage: simplewallet [--wallet-file=<file>|--generate-new-wallet=<file>] [--daemon-address=<host>:<port>] [<COMMAND>]";
      success_msg_writer() << desc_all << '\n' << w.get_commands_str();
      return false;
    }
    else if (command_line::get_arg(vm, command_line::arg_version))
    {
      success_msg_writer() << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG;
      return false;
    }

    auto parser = po::command_line_parser(argc, argv).options(desc_params).positional(positional_options);
    po::store(parser.run(), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 0;

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_2);
  //log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_space::log_singletone::get_default_log_file().c_str(),
    log_space::log_singletone::get_default_log_folder().c_str(), LOG_LEVEL_4);

  message_writer(epee::log_space::console_color_white, true) << CRYPTONOTE_NAME << " wallet v" << PROJECT_VERSION_LONG;

  if(command_line::has_arg(vm, arg_log_level))
  {
    LOG_PRINT_L0("Setting log level = " << command_line::get_arg(vm, arg_log_level));
    log_space::get_set_log_detalisation_level(true, command_line::get_arg(vm, arg_log_level));
  }

  if(command_line::has_arg(vm, tools::wallet_rpc_server::arg_rpc_bind_port))
  {
    log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
    //runs wallet with rpc interface
    if(!command_line::has_arg(vm, arg_wallet_file) )
    {
      LOG_ERROR("Wallet file not set.");
      return 1;
    }
    if(!command_line::has_arg(vm, arg_daemon_address) )
    {
      LOG_ERROR("Daemon address not set.");
      return 1;
    }
    if(!command_line::has_arg(vm, arg_password) )
    {
      LOG_ERROR("Wallet password not set.");
      return 1;
    }

    std::string wallet_file     = command_line::get_arg(vm, arg_wallet_file);
    std::string wallet_password = command_line::get_arg(vm, arg_password);
    std::string daemon_address  = command_line::get_arg(vm, arg_daemon_address);
    std::string daemon_host = command_line::get_arg(vm, arg_daemon_host);
    int daemon_port = command_line::get_arg(vm, arg_daemon_port);
    if (daemon_host.empty())
      daemon_host = "localhost";
    if (!daemon_port)
      daemon_port = RPC_DEFAULT_PORT;
    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    tools::wallet2 wal;
    try
    {
      LOG_PRINT_L0("Loading wallet...");
      wal.load(wallet_file, wallet_password);
      wal.init(daemon_address);
      wal.refresh();
      LOG_PRINT_GREEN("Loaded ok", LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Wallet initialize failed: " << e.what());
      return 1;
    }
    tools::wallet_rpc_server wrpc(wal);
    bool r = wrpc.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet rpc server");

    tools::signal_handler::install([&wrpc, &wal] {
      wrpc.send_stop_signal();
      wal.store();
    });
    LOG_PRINT_L0("Starting wallet rpc server");
    wrpc.run();
    LOG_PRINT_L0("Stopped wallet rpc server");
    try
    {
      LOG_PRINT_L0("Storing wallet...");
      wal.store();
      LOG_PRINT_GREEN("Stored ok", LOG_LEVEL_0);
    }
    catch (const std::exception& e)
    {
      LOG_ERROR("Failed to store wallet: " << e.what());
      return 1;
    }
  }else
  {
    //runs wallet with console interface
    r = w.init(vm);
    CHECK_AND_ASSERT_MES(r, 1, "Failed to initialize wallet");

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
