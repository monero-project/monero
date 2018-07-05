// Copyright (c) 2014-2018, The Monero Project
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
#include <boost/regex.hpp>
#include "include_base_utils.h"
#include "common/i18n.h"
#include "common/command_line.h"
#include "common/util.h"
#include "common/dns_utils.h"
#include "common/base58.h"
#include "common/scoped_message_writer.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include "simplewallet.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "crypto/crypto.h"  // for crypto::secret_key definition
#include "mnemonics/electrum-words.h"
#include "rapidjson/document.h"
#include "common/json_util.h"
#include "ringct/rctSigs.h"
#include "multisig/multisig.h"
#include "wallet/wallet_args.h"
#include "version.h"
#include <stdexcept>

#ifdef WIN32
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#endif

#ifdef HAVE_READLINE
#include "readline_buffer.h"
#endif

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;
typedef cryptonote::simple_wallet sw;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.simplewallet"

#define EXTENDED_LOGS_FILE "wallet_details.log"

#define DEFAULT_MIX 6

#define MIN_RING_SIZE 7 // Used to inform user about min ring size -- does not track actual protocol

#define OUTPUT_EXPORT_FILE_MAGIC "Monero output export\003"

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
  TransferLocked,
};

namespace
{
  const std::array<const char* const, 5> allowed_priority_strings = {{"default", "unimportant", "normal", "elevated", "priority"}};
  const auto arg_wallet_file = wallet_args::arg_wallet_file();
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", sw::tr("Generate new wallet and save it to <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_device = {"generate-from-device", sw::tr("Generate new wallet from device and save it to <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_view_key = {"generate-from-view-key", sw::tr("Generate incoming-only wallet from view key"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_spend_key = {"generate-from-spend-key", sw::tr("Generate deterministic wallet from spend key"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_keys = {"generate-from-keys", sw::tr("Generate wallet from private keys"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_multisig_keys = {"generate-from-multisig-keys", sw::tr("Generate a master wallet from multisig wallet keys"), ""};
  const auto arg_generate_from_json = wallet_args::arg_generate_from_json();
  const command_line::arg_descriptor<std::string> arg_mnemonic_language = {"mnemonic-language", sw::tr("Language for mnemonic"), ""};
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", sw::tr("Specify Electrum seed for wallet recovery/creation"), ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", sw::tr("Recover wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_restore_multisig_wallet = {"restore-multisig-wallet", sw::tr("Recover multisig wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("Generate non-deterministic view and spend keys"), false};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", sw::tr("Enable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_untrusted_daemon = {"untrusted-daemon", sw::tr("Disable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_allow_mismatched_daemon_version = {"allow-mismatched-daemon-version", sw::tr("Allow communicating with a daemon that uses a different RPC version"), false};
  const command_line::arg_descriptor<uint64_t> arg_restore_height = {"restore-height", sw::tr("Restore from specific blockchain height"), 0};
  const command_line::arg_descriptor<bool> arg_do_not_relay = {"do-not-relay", sw::tr("The newly created transaction will not be relayed to the monero network"), false};
  const command_line::arg_descriptor<bool> arg_create_address_file = {"create-address-file", sw::tr("Create an address file for new wallets"), false};
  const command_line::arg_descriptor<std::string> arg_subaddress_lookahead = {"subaddress-lookahead", tools::wallet2::tr("Set subaddress lookahead sizes to <major>:<minor>"), ""};
  const command_line::arg_descriptor<bool> arg_use_english_language_names = {"use-english-language-names", sw::tr("Display English language names"), false};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

#ifdef WIN32
  // Translate from CP850 to UTF-8;
  // std::getline for a Windows console returns a string in CP437 or CP850; as simplewallet,
  // like all of Monero, is assumed to work internally with UTF-8 throughout, even on Windows
  // (although only implemented partially), a translation to UTF-8 is needed for input.
  //
  // Note that if a program is started inside the MSYS2 shell somebody already translates
  // console input to UTF-8, but it's not clear how one could detect that in order to avoid
  // double-translation; this code here thus breaks UTF-8 input within a MSYS2 shell,
  // unfortunately.
  //
  // Note also that input for passwords is NOT translated, to remain compatible with any
  // passwords containing special characters that predate this switch to UTF-8 support.
  static std::string cp850_to_utf8(const std::string &cp850_str)
  {
    boost::locale::generator gen;
    gen.locale_cache_enabled(true);
    std::locale loc = gen("en_US.CP850");
    return boost::locale::conv::to_utf<char>(cp850_str, loc);
  }
#endif

  std::string input_line(const std::string& prompt)
  {
#ifdef HAVE_READLINE
    rdln::suspend_readline pause_readline;
#endif
    std::cout << prompt;

    std::string buf;
    std::getline(std::cin, buf);
#ifdef WIN32
    buf = cp850_to_utf8(buf);
#endif

    return epee::string_tools::trim(buf);
  }

  boost::optional<tools::password_container> password_prompter(const char *prompt, bool verify)
  {
#ifdef HAVE_READLINE
    rdln::suspend_readline pause_readline;
#endif
    auto pwd_container = tools::password_container::prompt(verify, prompt);
    if (!pwd_container)
    {
      tools::fail_msg_writer() << tr("failed to read wallet password");
    }
    return pwd_container;
  }

  boost::optional<tools::password_container> default_password_prompter(bool verify)
  {
    return password_prompter(verify ? tr("Enter a new password for the wallet") : tr("Wallet password"), verify);
  }

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

  tools::scoped_message_writer success_msg_writer(bool color = false)
  {
    return tools::scoped_message_writer(color ? console_color_green : console_color_default, false, std::string(), el::Level::Info);
  }

  tools::scoped_message_writer message_writer(epee::console_colors color = epee::console_color_default, bool bright = false)
  {
    return tools::scoped_message_writer(color, bright);
  }

  tools::scoped_message_writer fail_msg_writer()
  {
    return tools::scoped_message_writer(console_color_red, true, sw::tr("Error: "), el::Level::Error);
  }

  bool parse_bool(const std::string& s, bool& result)
  {
    if (s == "1" || command_line::is_yes(s))
    {
      result = true;
      return true;
    }
    if (s == "0" || command_line::is_no(s))
    {
      result = false;
      return true;
    }

    boost::algorithm::is_iequal ignore_case{};
    if (boost::algorithm::equals("true", s, ignore_case) || boost::algorithm::equals(simple_wallet::tr("true"), s, ignore_case))
    {
      result = true;
      return true;
    }
    if (boost::algorithm::equals("false", s, ignore_case) || boost::algorithm::equals(simple_wallet::tr("false"), s, ignore_case))
    {
      result = false;
      return true;
    }

    return false;
  }

  template <typename F>
  bool parse_bool_and_use(const std::string& s, F func)
  {
    bool r;
    if (parse_bool(s, r))
    {
      func(r);
      return true;
    }
    else
    {
      fail_msg_writer() << tr("invalid argument: must be either 0/1, true/false, y/n, yes/no");
      return false;
    }
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
    fail_msg_writer() << cryptonote::simple_wallet::tr("failed to parse refresh type");
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

  std::string get_version_string(uint32_t version)
  {
    return boost::lexical_cast<std::string>(version >> 16) + "." + boost::lexical_cast<std::string>(version & 0xffff);
  }

  std::string oa_prompter(const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)
  {
    if (addresses.empty())
      return {};
    // prompt user for confirmation.
    // inform user of DNSSEC validation status as well.
    std::string dnssec_str;
    if (dnssec_valid)
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
           << tr(" Monero Address = ") << addresses[0]
           << std::endl
           << tr("Is this OK? (Y/n) ")
    ;
    // prompt the user for confirmation given the dns query and dnssec status
    std::string confirm_dns_ok = input_line(prompt.str());
    if (std::cin.eof())
    {
      return {};
    }
    if (!command_line::is_yes(confirm_dns_ok))
    {
      std::cout << tr("you have cancelled the transfer request") << std::endl;
      return {};
    }
    return addresses[0];
  }

  bool parse_subaddress_indices(const std::string& arg, std::set<uint32_t>& subaddr_indices)
  {
    subaddr_indices.clear();

    if (arg.substr(0, 6) != "index=")
      return false;
    std::string subaddr_indices_str_unsplit = arg.substr(6, arg.size() - 6);
    std::vector<std::string> subaddr_indices_str;
    boost::split(subaddr_indices_str, subaddr_indices_str_unsplit, boost::is_any_of(","));

    for (const auto& subaddr_index_str : subaddr_indices_str)
    {
      uint32_t subaddr_index;
      if(!epee::string_tools::get_xtype_from_string(subaddr_index, subaddr_index_str))
      {
        fail_msg_writer() << tr("failed to parse index: ") << subaddr_index_str;
        subaddr_indices.clear();
        return false;
      }
      subaddr_indices.insert(subaddr_index);
    }
    return true;
  }

  boost::optional<std::pair<uint32_t, uint32_t>> parse_subaddress_lookahead(const std::string& str)
  {
    auto r = tools::parse_subaddress_lookahead(str);
    if (!r)
      fail_msg_writer() << tr("invalid format for subaddress lookahead; must be <major>:<minor>");
    return r;
  }

  void handle_transfer_exception(const std::exception_ptr &e, bool trusted_daemon)
  {
    bool warn_of_possible_attack = !trusted_daemon;
    try
    {
      std::rethrow_exception(e);
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
    catch (const tools::error::get_random_outs_error &e)
    {
      fail_msg_writer() << tr("failed to get random outputs to mix: ") << e.what();
    }
    catch (const tools::error::not_enough_unlocked_money& e)
    {
      LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
        print_money(e.available()) %
        print_money(e.tx_amount()));
      fail_msg_writer() << tr("Not enough money in unlocked balance");
      warn_of_possible_attack = false;
    }
    catch (const tools::error::not_enough_money& e)
    {
      LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
        print_money(e.available()) %
        print_money(e.tx_amount()));
      fail_msg_writer() << tr("Not enough money in unlocked balance");
      warn_of_possible_attack = false;
    }
    catch (const tools::error::tx_not_possible& e)
    {
      LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
        print_money(e.available()) %
        print_money(e.tx_amount() + e.fee())  %
        print_money(e.tx_amount()) %
        print_money(e.fee()));
      fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
      warn_of_possible_attack = false;
    }
    catch (const tools::error::not_enough_outs_to_mix& e)
    {
      auto writer = fail_msg_writer();
      writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
      for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
      {
        writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
      }
      writer << tr("Please use sweep_unmixable.");
    }
    catch (const tools::error::tx_not_constructed&)
      {
      fail_msg_writer() << tr("transaction was not constructed");
      warn_of_possible_attack = false;
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
      warn_of_possible_attack = false;
    }
    catch (const tools::error::zero_destination&)
    {
      fail_msg_writer() << tr("one of destinations is zero");
      warn_of_possible_attack = false;
    }
    catch (const tools::error::tx_too_big& e)
    {
      fail_msg_writer() << tr("failed to find a suitable way to split transactions");
      warn_of_possible_attack = false;
    }
    catch (const tools::error::transfer_error& e)
    {
      LOG_ERROR("unknown transfer error: " << e.to_string());
      fail_msg_writer() << tr("unknown transfer error: ") << e.what();
    }
    catch (const tools::error::multisig_export_needed& e)
    {
      LOG_ERROR("Multisig error: " << e.to_string());
      fail_msg_writer() << tr("Multisig error: ") << e.what();
      warn_of_possible_attack = false;
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

    if (warn_of_possible_attack)
      fail_msg_writer() << tr("There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a tranasction immediately. Alternatively, connect to another node so the original node cannot correlate information.");
  }

  bool check_file_overwrite(const std::string &filename)
  {
    boost::system::error_code errcode;
    if (boost::filesystem::exists(filename, errcode))
    {
      if (boost::ends_with(filename, ".keys"))
      {
        fail_msg_writer() << boost::format(tr("File %s likely stores wallet private keys! Use a different file name.")) % filename;
        return false;
      }
      return command_line::is_yes(input_line((boost::format(tr("File %s already exists. Are you sure to overwrite it? (Y/Yes/N/No): ")) % filename).str()));
    }
    return true;
  }
}

bool parse_priority(const std::string& arg, uint32_t& priority)
{
  auto priority_pos = std::find(
    allowed_priority_strings.begin(),
    allowed_priority_strings.end(),
    arg);
  if(priority_pos != allowed_priority_strings.end()) {
    priority = std::distance(allowed_priority_strings.begin(), priority_pos);
    return true;
  }
  return false;
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

std::string simple_wallet::get_command_usage(const std::vector<std::string> &args)
{
  std::pair<std::string, std::string> documentation = m_cmd_binder.get_documentation(args);
  std::stringstream ss;
  if(documentation.first.empty())
  {
    ss << tr("Unknown command: ") << args.front();
  }
  else
  {
    std::string usage = documentation.second.empty() ? args.front() : documentation.first;
    std::string description = documentation.second.empty() ? documentation.first : documentation.second;
    usage.insert(0, "  ");
    ss << tr("Command usage: ") << ENDL << usage << ENDL << ENDL;
    boost::replace_all(description, "\n", "\n  ");
    description.insert(0, "  ");
    ss << tr("Command description: ") << ENDL << description << ENDL;
  }
  return ss.str();
}

bool simple_wallet::viewkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  // don't log
  if (m_wallet->key_on_device()) {
    std::cout << "secret: On device. Not available" << std::endl;
  } else {
    std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << std::endl;
  }
  std::cout << "public: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_view_public_key) << std::endl;

  return true;
}

bool simple_wallet::spendkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no spend key");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  // don't log
  if (m_wallet->key_on_device()) {
    std::cout << "secret: On device. Not available" << std::endl;
  } else {
    std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key) << std::endl;
  }
  std::cout << "public: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key) << std::endl;

  return true;
}

bool simple_wallet::print_seed(bool encrypted)
{
  bool success =  false;
  std::string seed;
  bool ready, multisig;

  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  multisig = m_wallet->multisig(&ready);
  if (multisig)
  {
    if (!ready)
    {
      fail_msg_writer() << tr("wallet is multisig but not yet finalized");
      return true;
    }
  }
  else if (!m_wallet->is_deterministic())
  {
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
    return true;
  }

  epee::wipeable_string seed_pass;
  if (encrypted)
  {
    auto pwd_container = password_prompter(tr("Enter optional seed encryption passphrase, empty to see raw seed"), true);
    if (std::cin.eof() || !pwd_container)
      return true;
    seed_pass = pwd_container->password();
  }

  if (multisig)
    success = m_wallet->get_multisig_seed(seed, seed_pass);
  else if (m_wallet->is_deterministic())
    success = m_wallet->get_seed(seed, seed_pass);

  if (success) 
  {
    print_seed(seed);
  }
  else
  {
    fail_msg_writer() << tr("Failed to retrieve seed");
  }
  return true;
}

bool simple_wallet::seed(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  return print_seed(false);
}

bool simple_wallet::encrypted_seed(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  return print_seed(true);
}

bool simple_wallet::seed_set_language(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet->multisig())
  {
    fail_msg_writer() << tr("wallet is multisig and has no seed");
    return true;
  }
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
 
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    std::string mnemonic_language = get_mnemonic_language();
    if (mnemonic_language.empty())
      return true;

    m_wallet->set_seed_language(std::move(mnemonic_language));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::change_password(const std::vector<std::string> &args)
{ 
  const auto orig_pwd_container = get_and_verify_password();

  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your original password was incorrect.");
    return true;
  }

  // prompts for a new password, pass true to verify the password
  const auto pwd_container = default_password_prompter(true);
  if(!pwd_container)
    return true;

  try
  {
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
    m_wallet->store();
  }
  catch (const tools::error::wallet_logic_error& e)
  {
    fail_msg_writer() << tr("Error with wallet rewrite: ") << e.what();
    return true;
  }

  return true;
}

bool simple_wallet::payment_id(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  crypto::hash payment_id;
  if (args.size() > 0)
  {
    fail_msg_writer() << tr("usage: payment_id");
    return true;
  }
  payment_id = crypto::rand<crypto::hash>();
  success_msg_writer() << tr("Random payment ID: ") << payment_id;
  return true;
}

bool simple_wallet::print_fee_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (!try_connect_to_daemon())
  {
    fail_msg_writer() << tr("Cannot connect to daemon");
    return true;
  }
  const uint64_t per_kb_fee = m_wallet->get_per_kb_fee();
  const uint64_t typical_size_kb = 13;
  message_writer() << (boost::format(tr("Current fee is %s %s per kB")) % print_money(per_kb_fee) % cryptonote::get_unit(cryptonote::get_default_decimal_point())).str();

  std::vector<uint64_t> fees;
  for (uint32_t priority = 1; priority <= 4; ++priority)
  {
    uint64_t mult = m_wallet->get_fee_multiplier(priority);
    fees.push_back(per_kb_fee * typical_size_kb * mult);
  }
  std::vector<std::pair<uint64_t, uint64_t>> blocks;
  try
  {
    uint64_t base_size = typical_size_kb * 1024;
    blocks = m_wallet->estimate_backlog(base_size, base_size + 1023, fees);
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Error: failed to estimate backlog array size: ") << e.what();
    return true;
  }
  if (blocks.size() != 4)
  {
    fail_msg_writer() << tr("Error: bad estimated backlog array size");
    return true;
  }

  for (uint32_t priority = 1; priority <= 4; ++priority)
  {
    uint64_t nblocks_low = blocks[priority - 1].first;
    uint64_t nblocks_high = blocks[priority - 1].second;
    if (nblocks_low > 0)
    {
      std::string msg;
      if (priority == m_wallet->get_default_priority() || (m_wallet->get_default_priority() == 0 && priority == 2))
        msg = tr(" (current)");
      uint64_t minutes_low = nblocks_low * DIFFICULTY_TARGET_V2 / 60, minutes_high = nblocks_high * DIFFICULTY_TARGET_V2 / 60;
      if (nblocks_high == nblocks_low)
        message_writer() << (boost::format(tr("%u block (%u minutes) backlog at priority %u%s")) % nblocks_low % minutes_low % priority % msg).str();
      else
        message_writer() << (boost::format(tr("%u to %u block (%u to %u minutes) backlog at priority %u")) % nblocks_low % nblocks_high % minutes_low % minutes_high % priority).str();
    }
    else
      message_writer() << tr("No backlog at priority ") << priority;
  }
  return true;
}

bool simple_wallet::prepare_multisig(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet->multisig())
  {
    fail_msg_writer() << tr("This wallet is already multisig");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot be made multisig");
    return true;
  }

  if(m_wallet->get_num_transfer_details())
  {
    fail_msg_writer() << tr("This wallet has been used before, please use a new wallet to create a multisig wallet");
    return true;
  }

  const auto orig_pwd_container = get_and_verify_password();
  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your password is incorrect.");
    return true;
  }

  std::string multisig_info = m_wallet->get_multisig_info();
  success_msg_writer() << multisig_info;
  success_msg_writer() << tr("Send this multisig info to all other participants, then use make_multisig <threshold> <info1> [<info2>...] with others' multisig info");
  success_msg_writer() << tr("This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet's participants ");
  return true;
}

bool simple_wallet::make_multisig(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet->multisig())
  {
    fail_msg_writer() << tr("This wallet is already multisig");
    return true;
  }
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot be made multisig");
    return true;
  }

  if(m_wallet->get_num_transfer_details())
  {
    fail_msg_writer() << tr("This wallet has been used before, please use a new wallet to create a multisig wallet");
    return true;
  }

  const auto orig_pwd_container = get_and_verify_password();
  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your original password was incorrect.");
    return true;
  }

  if (args.size() < 2)
  {
    fail_msg_writer() << tr("usage: make_multisig <threshold> <multisiginfo1> [<multisiginfo2>...]");
    return true;
  }

  // parse threshold
  uint32_t threshold;
  if (!string_tools::get_xtype_from_string(threshold, args[0]))
  {
    fail_msg_writer() << tr("Invalid threshold");
    return true;
  }

  LOCK_IDLE_SCOPE();

  try
  {
    auto local_args = args;
    local_args.erase(local_args.begin());
    std::string multisig_extra_info = m_wallet->make_multisig(orig_pwd_container->password(), local_args, threshold);
    if (!multisig_extra_info.empty())
    {
      success_msg_writer() << tr("Another step is needed");
      success_msg_writer() << multisig_extra_info;
      success_msg_writer() << tr("Send this multisig info to all other participants, then use finalize_multisig <info1> [<info2>...] with others' multisig info");
      return true;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Error creating multisig: ") << e.what();
    return true;
  }

  uint32_t total;
  if (!m_wallet->multisig(NULL, &threshold, &total))
  {
    fail_msg_writer() << tr("Error creating multisig: new wallet is not multisig");
    return true;
  }
  success_msg_writer() << std::to_string(threshold) << "/" << total << tr(" multisig address: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->nettype());

  return true;
}

bool simple_wallet::finalize_multisig(const std::vector<std::string> &args)
{
  bool ready;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!m_wallet->multisig(&ready))
  {
    fail_msg_writer() << tr("This wallet is not multisig");
    return true;
  }
  if (ready)
  {
    fail_msg_writer() << tr("This wallet is already finalized");
    return true;
  }

  const auto orig_pwd_container = get_and_verify_password();
  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your original password was incorrect.");
    return true;
  }

  if (args.size() < 2)
  {
    fail_msg_writer() << tr("usage: finalize_multisig <multisiginfo1> [<multisiginfo2>...]");
    return true;
  }

  try
  {
    if (!m_wallet->finalize_multisig(orig_pwd_container->password(), args))
    {
      fail_msg_writer() << tr("Failed to finalize multisig");
      return true;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to finalize multisig: ") << e.what();
    return true;
  }

  return true;
}

bool simple_wallet::export_multisig(const std::vector<std::string> &args)
{
  bool ready;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!m_wallet->multisig(&ready))
  {
    fail_msg_writer() << tr("This wallet is not multisig");
    return true;
  }
  if (!ready)
  {
    fail_msg_writer() << tr("This multisig wallet is not yet finalized");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_multisig_info <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password())
    return true;

  const std::string filename = args[0];
  if (m_wallet->confirm_export_overwrite() && !check_file_overwrite(filename))
    return true;
  try
  {
    cryptonote::blobdata ciphertext = m_wallet->export_multisig();

    bool r = epee::file_io_utils::save_string_to_file(filename, ciphertext);
    if (!r)
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting multisig info: " << e.what());
    fail_msg_writer() << tr("Error exporting multisig info: ") << e.what();
    return true;
  }

  success_msg_writer() << tr("Multisig info exported to ") << filename;
  return true;
}

bool simple_wallet::import_multisig(const std::vector<std::string> &args)
{
  bool ready;
  uint32_t threshold, total;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!m_wallet->multisig(&ready, &threshold, &total))
  {
    fail_msg_writer() << tr("This wallet is not multisig");
    return true;
  }
  if (!ready)
  {
    fail_msg_writer() << tr("This multisig wallet is not yet finalized");
    return true;
  }
  if (args.size() < threshold - 1)
  {
    fail_msg_writer() << tr("usage: import_multisig_info <filename1> [<filename2>...] - one for each other participant");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password())
    return true;

  std::vector<cryptonote::blobdata> info;
  for (size_t n = 0; n < args.size(); ++n)
  {
    const std::string filename = args[n];
    std::string data;
    bool r = epee::file_io_utils::load_file_to_string(filename, data);
    if (!r)
    {
      fail_msg_writer() << tr("failed to read file ") << filename;
      return true;
    }
    info.push_back(std::move(data));
  }

  LOCK_IDLE_SCOPE();

  // all read and parsed, actually import
  try
  {
    size_t n_outputs = m_wallet->import_multisig(info);
    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer() << tr("Multisig info imported");
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to import multisig info: ") << e.what();
    return true;
  }
  if (is_daemon_trusted())
  {
    try
    {
      m_wallet->rescan_spent();
    }
    catch (const std::exception &e)
    {
      message_writer() << tr("Failed to update spent status after importing multisig info: ") << e.what();
    }
  }
  else
  {
    message_writer() << tr("Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run \"rescan_spent\"");
  }
  return true;
}

bool simple_wallet::accept_loaded_tx(const tools::wallet2::multisig_tx_set &txs)
{
  std::string extra_message;
  return accept_loaded_tx([&txs](){return txs.m_ptx.size();}, [&txs](size_t n)->const tools::wallet2::tx_construction_data&{return txs.m_ptx[n].construction_data;}, extra_message);
}

bool simple_wallet::sign_multisig(const std::vector<std::string> &args)
{
  bool ready;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(!m_wallet->multisig(&ready))
  {
    fail_msg_writer() << tr("This is not a multisig wallet");
    return true;
  }
  if (!ready)
  {
    fail_msg_writer() << tr("This multisig wallet is not yet finalized");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: sign_multisig <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  std::string filename = args[0];
  std::vector<crypto::hash> txids;
  uint32_t signers = 0;
  try
  {
    bool r = m_wallet->sign_multisig_tx_from_file(filename, txids, [&](const tools::wallet2::multisig_tx_set &tx){ signers = tx.m_signers.size(); return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to sign multisig transaction");
      return true;
    }
  }
  catch (const tools::error::multisig_export_needed& e)
  {
    fail_msg_writer() << tr("Multisig error: ") << e.what();
    return true;
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to sign multisig transaction: ") << e.what();
    return true;
  }

  if (txids.empty())
  {
    uint32_t threshold;
    m_wallet->multisig(NULL, &threshold);
    uint32_t signers_needed = threshold - signers - 1;
    success_msg_writer(true) << tr("Transaction successfully signed to file ") << filename << ", "
        << signers_needed << " more signer(s) needed";
    return true;
  }
  else
  {
    std::string txids_as_text;
    for (const auto &txid: txids)
    {
      if (!txids_as_text.empty())
        txids_as_text += (", ");
      txids_as_text += epee::string_tools::pod_to_hex(txid);
    }
    success_msg_writer(true) << tr("Transaction successfully signed to file ") << filename << ", txid " << txids_as_text;
    success_msg_writer(true) << tr("It may be relayed to the network with submit_multisig");
  }
  return true;
}

bool simple_wallet::submit_multisig(const std::vector<std::string> &args)
{
  bool ready;
  uint32_t threshold;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!m_wallet->multisig(&ready, &threshold))
  {
    fail_msg_writer() << tr("This is not a multisig wallet");
    return true;
  }
  if (!ready)
  {
    fail_msg_writer() << tr("This multisig wallet is not yet finalized");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: submit_multisig <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  if (!try_connect_to_daemon())
    return true;

  std::string filename = args[0];
  try
  {
    tools::wallet2::multisig_tx_set txs;
    bool r = m_wallet->load_multisig_tx_from_file(filename, txs, [&](const tools::wallet2::multisig_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load multisig transaction from file");
      return true;
    }
    if (txs.m_signers.size() < threshold)
    {
      fail_msg_writer() << (boost::format(tr("Multisig transaction signed by only %u signers, needs %u more signatures"))
          % txs.m_signers.size() % (threshold - txs.m_signers.size())).str();
      return true;
    }

    // actually commit the transactions
    for (auto &ptx: txs.m_ptx)
    {
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Transaction successfully submitted, transaction ") << get_transaction_hash(ptx.tx) << ENDL
          << tr("You can check its status by using the `show_transfers` command.");
    }
  }
  catch (const std::exception &e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}

bool simple_wallet::export_raw_multisig(const std::vector<std::string> &args)
{
  bool ready;
  uint32_t threshold;
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!m_wallet->multisig(&ready, &threshold))
  {
    fail_msg_writer() << tr("This is not a multisig wallet");
    return true;
  }
  if (!ready)
  {
    fail_msg_writer() << tr("This multisig wallet is not yet finalized");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_raw_multisig <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  std::string filename = args[0];
  if (m_wallet->confirm_export_overwrite() && !check_file_overwrite(filename))
    return true;
  try
  {
    tools::wallet2::multisig_tx_set txs;
    bool r = m_wallet->load_multisig_tx_from_file(filename, txs, [&](const tools::wallet2::multisig_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load multisig transaction from file");
      return true;
    }
    if (txs.m_signers.size() < threshold)
    {
      fail_msg_writer() << (boost::format(tr("Multisig transaction signed by only %u signers, needs %u more signatures"))
          % txs.m_signers.size() % (threshold - txs.m_signers.size())).str();
      return true;
    }

    // save the transactions
    std::string filenames;
    for (auto &ptx: txs.m_ptx)
    {
      const crypto::hash txid = cryptonote::get_transaction_hash(ptx.tx);
      const std::string filename = std::string("raw_multisig_monero_tx_") + epee::string_tools::pod_to_hex(txid);
      if (!filenames.empty())
        filenames += ", ";
      filenames += filename;
      if (!epee::file_io_utils::save_string_to_file(filename, cryptonote::tx_to_blob(ptx.tx)))
      {
        fail_msg_writer() << tr("Failed to export multisig transaction to file ") << filename;
        return true;
      }
    }
    success_msg_writer() << tr("Saved exported multisig transaction file(s): ") << filenames;
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

bool simple_wallet::print_ring(const std::vector<std::string> &args)
{
  crypto::key_image key_image;
  crypto::hash txid;
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: print_ring <key_image|txid>");
    return true;
  }

  if (!epee::string_tools::hex_to_pod(args[0], key_image))
  {
    fail_msg_writer() << tr("Invalid key image");
    return true;
  }
  // this one will always work, they're all 32 byte hex
  if (!epee::string_tools::hex_to_pod(args[0], txid))
  {
    fail_msg_writer() << tr("Invalid txid");
    return true;
  }

  std::vector<uint64_t> ring;
  std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> rings;
  try
  {
    if (m_wallet->get_ring(key_image, ring))
      rings.push_back({key_image, ring});
    else if (!m_wallet->get_rings(txid, rings))
    {
      fail_msg_writer() << tr("Key image either not spent, or spent with mixin 0");
      return true;
    }

    for (const auto &ring: rings)
    {
      std::stringstream str;
      for (const auto &x: ring.second)
        str << x<< " ";
      // do NOT translate this "absolute" below, the lin can be used as input to set_ring
      success_msg_writer() << epee::string_tools::pod_to_hex(ring.first) <<  " absolute " << str.str();
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to get key image ring: ") << e.what();
  }

  return true;
}

bool simple_wallet::set_ring(const std::vector<std::string> &args)
{
  crypto::key_image key_image;
  if (args.size() < 3)
  {
    fail_msg_writer() << tr("usage: set_ring <key_image> absolute|relative <index> [<index>...]");
    return true;
  }

  if (!epee::string_tools::hex_to_pod(args[0], key_image))
  {
    fail_msg_writer() << tr("Invalid key image");
    return true;
  }

  bool relative;
  if (args[1] == "absolute")
  {
    relative = false;
  }
  else if (args[1] == "relative")
  {
    relative = true;
  }
  else
  {
    fail_msg_writer() << tr("Missing absolute or relative keyword");
    return true;
  }

  std::vector<uint64_t> ring;
  for (size_t n = 2; n < args.size(); ++n)
  {
    ring.resize(ring.size() + 1);
    if (!string_tools::get_xtype_from_string(ring.back(), args[n]))
    {
      fail_msg_writer() << tr("invalid index: must be a strictly positive unsigned integer");
      return true;
    }
    if (relative)
    {
      if (ring.size() > 1 && !ring.back())
      {
        fail_msg_writer() << tr("invalid index: must be a strictly positive unsigned integer");
        return true;
      }
      uint64_t sum = 0;
      for (uint64_t out: ring)
      {
        if (out > std::numeric_limits<uint64_t>::max() - sum)
        {
          fail_msg_writer() << tr("invalid index: indices wrap");
          return true;
        }
        sum += out;
      }
    }
    else
    {
      if (ring.size() > 1 && ring[ring.size() - 2] >= ring[ring.size() - 1])
      {
        fail_msg_writer() << tr("invalid index: indices should be in strictly ascending order");
        return true;
      }
    }
  }
  if (!m_wallet->set_ring(key_image, ring, relative))
  {
    fail_msg_writer() << tr("failed to set ring");
    return true;
  }

  return true;
}

bool simple_wallet::blackball(const std::vector<std::string> &args)
{
  crypto::public_key output;
  if (args.size() == 0)
  {
    fail_msg_writer() << tr("usage: blackball <output_public_key> | <filename> [add]");
    return true;
  }

  try
  {
    if (epee::string_tools::hex_to_pod(args[0], output))
    {
      m_wallet->blackball_output(output);
    }
    else if (epee::file_io_utils::is_file_exist(args[0]))
    {
      std::vector<crypto::public_key> outputs;
      char str[65];

      std::unique_ptr<FILE, tools::close_file> f(fopen(args[0].c_str(), "r"));
      if (f)
      {
        while (!feof(f.get()))
        {
          if (!fgets(str, sizeof(str), f.get()))
            break;
          const size_t len = strlen(str);
          if (len > 0 && str[len - 1] == '\n')
            str[len - 1] = 0;
          if (!str[0])
            continue;
          outputs.push_back(crypto::public_key());
          if (!epee::string_tools::hex_to_pod(str, outputs.back()))
          {
            fail_msg_writer() << tr("Invalid public key: ") << str;
            return true;
          }
        }
        f.reset();
        bool add = false;
        if (args.size() > 1)
        {
          if (args[1] != "add")
          {
            fail_msg_writer() << tr("Bad argument: ") + args[1] + ": " + tr("should be \"add\"");
            return true;
          }
          add = true;
        }
        m_wallet->set_blackballed_outputs(outputs, add);
      }
      else
      {
        fail_msg_writer() << tr("Failed to open file");
        return true;
      }
    }
    else
    {
      fail_msg_writer() << tr("Invalid public key, and file doesn't exist");
      return true;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to blackball output: ") << e.what();
  }

  return true;
}

bool simple_wallet::unblackball(const std::vector<std::string> &args)
{
  crypto::public_key output;
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: unblackball <output_public_key>");
    return true;
  }

  if (!epee::string_tools::hex_to_pod(args[0], output))
  {
    fail_msg_writer() << tr("Invalid public key");
    return true;
  }

  try
  {
    m_wallet->unblackball_output(output);
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to unblackball output: ") << e.what();
  }

  return true;
}

bool simple_wallet::blackballed(const std::vector<std::string> &args)
{
  crypto::public_key output;
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: blackballed <output_public_key>");
    return true;
  }

  if (!epee::string_tools::hex_to_pod(args[0], output))
  {
    fail_msg_writer() << tr("Invalid public key");
    return true;
  }

  try
  {
    if (m_wallet->is_output_blackballed(output))
      message_writer() << tr("Blackballed: ") << output;
    else
      message_writer() << tr("not blackballed: ") << output;
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to unblackball output: ") << e.what();
  }

  return true;
}

bool simple_wallet::save_known_rings(const std::vector<std::string> &args)
{
  try
  {
    LOCK_IDLE_SCOPE();
    m_wallet->find_and_save_rings();
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to save known rings: ") << e.what();
  }
  return true;
}

bool simple_wallet::version(const std::vector<std::string> &args)
{
  message_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
  return true;
}

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->always_confirm_transfers(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_print_ring_members(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->print_ring_members(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_store_tx_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
 
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->store_tx_info(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_default_ring_size(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("ring size must be an integer >= ") << MIN_RING_SIZE;
      return true;
    }
    uint32_t ring_size = boost::lexical_cast<uint32_t>(args[1]);
    if (ring_size < MIN_RING_SIZE && ring_size != 0)
    {
      fail_msg_writer() << tr("ring size must be an integer >= ") << MIN_RING_SIZE;
      return true;
    }
 
    if (ring_size != 0 && ring_size != DEFAULT_MIX+1)
      message_writer() << tr("WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.");

    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet->default_mixin(ring_size > 0 ? ring_size - 1 : 0);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("ring size must be an integer >= ") << MIN_RING_SIZE;
    return true;
  }
  catch(...)
  {
    fail_msg_writer() << tr("could not change default ring size");
    return true;
  }
}

bool simple_wallet::set_default_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  int priority = 0;
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("priority must be 0, 1, 2, 3, or 4 ");
      return true;
    }
    if (args[1] == "0")
    {
      priority = 0;
    }
    else
    {
      priority = boost::lexical_cast<int>(args[1]);
      if (priority < 1 || priority > 4)
      {
        fail_msg_writer() << tr("priority must be 0, 1, 2, 3, or 4");
        return true;
      }
    }
 
    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet->set_default_priority(priority);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("priority must be 0, 1, 2, 3, or 4");
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
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool auto_refresh) {
      m_wallet->auto_refresh(auto_refresh);
      m_idle_mutex.lock();
      m_auto_refresh_enabled.store(auto_refresh, std::memory_order_relaxed);
      m_idle_cond.notify_one();
      m_idle_mutex.unlock();

      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_refresh_type(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  tools::wallet2::RefreshType refresh_type;
  if (!parse_refresh_type(args[1], refresh_type))
  {
    return true;
  }
 
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->set_refresh_type(refresh_type);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_confirm_missing_payment_id(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->confirm_missing_payment_id(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_ask_password(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->ask_password(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_unit(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const std::string &unit = args[1];
  unsigned int decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT;

  if (unit == "monero")
    decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT;
  else if (unit == "millinero")
    decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT - 3;
  else if (unit == "micronero")
    decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT - 6;
  else if (unit == "nanonero")
    decimal_point = CRYPTONOTE_DISPLAY_DECIMAL_POINT - 9;
  else if (unit == "piconero")
    decimal_point = 0;
  else
  {
    fail_msg_writer() << tr("invalid unit");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    cryptonote::set_default_decimal_point(decimal_point);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_min_output_count(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  uint32_t count;
  if (!string_tools::get_xtype_from_string(count, args[1]))
  {
    fail_msg_writer() << tr("invalid count: must be an unsigned integer");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->set_min_output_count(count);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_min_output_value(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  uint64_t value;
  if (!cryptonote::parse_amount(value, args[1]))
  {
    fail_msg_writer() << tr("invalid value");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->set_min_output_value(value);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_merge_destinations(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->merge_destinations(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_confirm_backlog(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->confirm_backlog(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_confirm_backlog_threshold(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  uint32_t threshold;
  if (!string_tools::get_xtype_from_string(threshold, args[1]))
  {
    fail_msg_writer() << tr("invalid count: must be an unsigned integer");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->set_confirm_backlog_threshold(threshold);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_confirm_export_overwrite(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->confirm_export_overwrite(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_refresh_from_block_height(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    uint64_t height;
    if (!epee::string_tools::get_xtype_from_string(height, args[1]))
    {
      fail_msg_writer() << tr("Invalid height");
      return true;
    }
    m_wallet->set_refresh_from_block_height(height);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_auto_low_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->auto_low_priority(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_segregate_pre_fork_outputs(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->segregate_pre_fork_outputs(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_key_reuse_mitigation2(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet->key_reuse_mitigation2(r);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    });
  }
  return true;
}

bool simple_wallet::set_subaddress_lookahead(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    auto lookahead = parse_subaddress_lookahead(args[1]);
    if (lookahead)
    {
      m_wallet->set_subaddress_lookahead(lookahead->first, lookahead->second);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
  }
  return true;
}

bool simple_wallet::set_segregation_height(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    uint64_t height;
    if (!epee::string_tools::get_xtype_from_string(height, args[1]))
    {
      fail_msg_writer() << tr("Invalid height");
      return true;
    }
    m_wallet->segregation_height(height);
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if(args.empty())
  {
    success_msg_writer() << get_commands_str();
  }
  else
  {
    success_msg_writer() << get_command_usage(args);
  }
  return true;
}

simple_wallet::simple_wallet()
  : m_allow_mismatched_daemon_version(false)
  , m_refresh_progress_reporter(*this)
  , m_idle_run(true)
  , m_auto_refresh_enabled(false)
  , m_auto_refresh_refreshing(false)
  , m_in_manual_refresh(false)
  , m_current_subaddress_account(0)
{
  m_cmd_binder.set_handler("start_mining",
                           boost::bind(&simple_wallet::start_mining, this, _1),
                           tr("start_mining [<number_of_threads>] [bg_mining] [ignore_battery]"),
                           tr("Start mining in the daemon (bg_mining and ignore_battery are optional booleans)."));
  m_cmd_binder.set_handler("stop_mining",
                           boost::bind(&simple_wallet::stop_mining, this, _1),
                           tr("Stop mining in the daemon."));
  m_cmd_binder.set_handler("set_daemon",
                           boost::bind(&simple_wallet::set_daemon, this, _1),
                           tr("set_daemon <host>[:<port>] [trusted|untrusted]"),
                           tr("Set another daemon to connect to."));
  m_cmd_binder.set_handler("save_bc",
                           boost::bind(&simple_wallet::save_bc, this, _1),
                           tr("Save the current blockchain data."));
  m_cmd_binder.set_handler("refresh",
                           boost::bind(&simple_wallet::refresh, this, _1),
                           tr("Synchronize the transactions and balance."));
  m_cmd_binder.set_handler("balance",
                           boost::bind(&simple_wallet::show_balance, this, _1),
                           tr("balance [detail]"),
                           tr("Show the wallet's balance of the currently selected account."));
  m_cmd_binder.set_handler("incoming_transfers",
                           boost::bind(&simple_wallet::show_incoming_transfers, this, _1),
                           tr("incoming_transfers [available|unavailable] [verbose] [index=<N1>[,<N2>[,...]]]"),
                           tr("Show the incoming transfers, all or filtered by availability and address index."));
  m_cmd_binder.set_handler("payments",
                           boost::bind(&simple_wallet::show_payments, this, _1),
                           tr("payments <PID_1> [<PID_2> ... <PID_N>]"),
                           tr("Show the payments for the given payment IDs."));
  m_cmd_binder.set_handler("bc_height",
                           boost::bind(&simple_wallet::show_blockchain_height, this, _1),
                           tr("Show the blockchain height."));
  m_cmd_binder.set_handler("transfer_original",
                           boost::bind(&simple_wallet::transfer, this, _1),
                           tr("transfer_original [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> <amount> [<payment_id>]"),
                           tr("Transfer <amount> to <address> using an older transaction building algorithm. If the parameter \"index=<N1>[,<N2>,...]\" is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. <priority> is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <ring_size> is the number of inputs to include for untraceability. Multiple payments can be made at once by adding <address_2> <amount_2> etcetera (before the payment ID, if it's included)"));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer_new, this, _1),
                           tr("transfer [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> <amount> [<payment_id>]"),
                           tr("Transfer <amount> to <address>. If the parameter \"index=<N1>[,<N2>,...]\" is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. <priority> is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <ring_size> is the number of inputs to include for untraceability. Multiple payments can be made at once by adding <address_2> <amount_2> etcetera (before the payment ID, if it's included)"));
  m_cmd_binder.set_handler("locked_transfer",
                           boost::bind(&simple_wallet::locked_transfer, this, _1),
                           tr("locked_transfer [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <addr> <amount> <lockblocks> [<payment_id>]"),
                           tr("Transfer <amount> to <address> and lock it for <lockblocks> (max. 1000000). If the parameter \"index=<N1>[,<N2>,...]\" is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. <priority> is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <ring_size> is the number of inputs to include for untraceability. Multiple payments can be made at once by adding <address_2> <amount_2> etcetera (before the payment ID, if it's included)"));
  m_cmd_binder.set_handler("sweep_unmixable",
                           boost::bind(&simple_wallet::sweep_unmixable, this, _1),
                           tr("Send all unmixable outputs to yourself with ring_size 1"));
  m_cmd_binder.set_handler("sweep_all", boost::bind(&simple_wallet::sweep_all, this, _1),
                           tr("sweep_all [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> [<payment_id>]"),
                           tr("Send all unlocked balance to an address. If the parameter \"index<N1>[,<N2>,...]\" is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used."));
  m_cmd_binder.set_handler("sweep_below",
                           boost::bind(&simple_wallet::sweep_below, this, _1),
                           tr("sweep_below <amount_threshold> [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> [<payment_id>]"),
                           tr("Send all unlocked outputs below the threshold to an address."));
  m_cmd_binder.set_handler("sweep_single",
                           boost::bind(&simple_wallet::sweep_single, this, _1),
                           tr("sweep_single [<priority>] [<ring_size>] <key_image> <address> [<payment_id>]"),
                           tr("Send a single output of the given key image to an address without change."));
  m_cmd_binder.set_handler("donate",
                           boost::bind(&simple_wallet::donate, this, _1),
                           tr("donate [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <amount> [<payment_id>]"),
                           tr("Donate <amount> to the development team (donate.getmonero.org)."));
  m_cmd_binder.set_handler("sign_transfer",
                           boost::bind(&simple_wallet::sign_transfer, this, _1),
                           tr("sign_transfer [export]"),
                           tr("Sign a transaction from a file."));
  m_cmd_binder.set_handler("submit_transfer",
                           boost::bind(&simple_wallet::submit_transfer, this, _1),
                           tr("Submit a signed transaction from a file."));
  m_cmd_binder.set_handler("set_log",
                           boost::bind(&simple_wallet::set_log, this, _1),
                           tr("set_log <level>|{+,-,}<categories>"),
                           tr("Change the current log detail (level must be <0-4>)."));
  m_cmd_binder.set_handler("account",
                           boost::bind(&simple_wallet::account, this, _1),
                           tr("account\n"
                            "  account new <label text with white spaces allowed>\n"
                            "  account switch <index> \n"
                            "  account label <index> <label text with white spaces allowed>\n"
                            "  account tag <tag_name> <account_index_1> [<account_index_2> ...]\n"
                            "  account untag <account_index_1> [<account_index_2> ...]\n"
                            "  account tag_description <tag_name> <description>"),
                           tr("If no arguments are specified, the wallet shows all the existing accounts along with their balances.\n"
                              "If the \"new\" argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).\n"
                              "If the \"switch\" argument is specified, the wallet switches to the account specified by <index>.\n"
                              "If the \"label\" argument is specified, the wallet sets the label of the account specified by <index> to the provided label text.\n"
                              "If the \"tag\" argument is specified, a tag <tag_name> is assigned to the specified accounts <account_index_1>, <account_index_2>, ....\n"
                              "If the \"untag\" argument is specified, the tags assigned to the specified accounts <account_index_1>, <account_index_2> ..., are removed.\n"
                              "If the \"tag_description\" argument is specified, the tag <tag_name> is assigned an arbitrary text <description>."));
  m_cmd_binder.set_handler("address",
                           boost::bind(&simple_wallet::print_address, this, _1),
                           tr("address [ new <label text with white spaces allowed> | all | <index_min> [<index_max>] | label <index> <label text with white spaces allowed>]"),
                           tr("If no arguments are specified or <index> is specified, the wallet shows the default or specified address. If \"all\" is specified, the wallet shows all the existing addresses in the currently selected account. If \"new \" is specified, the wallet creates a new address with the provided label text (which can be empty). If \"label\" is specified, the wallet sets the label of the address specified by <index> to the provided label text."));
  m_cmd_binder.set_handler("integrated_address",
                           boost::bind(&simple_wallet::print_integrated_address, this, _1),
                           tr("integrated_address [<payment_id> | <address>]"),
                           tr("Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID"));
  m_cmd_binder.set_handler("address_book",
                           boost::bind(&simple_wallet::address_book, this, _1),
                           tr("address_book [(add ((<address> [pid <id>])|<integrated address>) [<description possibly with whitespaces>])|(delete <index>)]"),
                           tr("Print all entries in the address book, optionally adding/deleting an entry to/from it."));
  m_cmd_binder.set_handler("save",
                           boost::bind(&simple_wallet::save, this, _1),
                           tr("Save the wallet data."));
  m_cmd_binder.set_handler("save_watch_only",
                           boost::bind(&simple_wallet::save_watch_only, this, _1),
                           tr("Save a watch-only keys file."));
  m_cmd_binder.set_handler("viewkey",
                           boost::bind(&simple_wallet::viewkey, this, _1),
                           tr("Display the private view key."));
  m_cmd_binder.set_handler("spendkey",
                           boost::bind(&simple_wallet::spendkey, this, _1),
                           tr("Display the private spend key."));
  m_cmd_binder.set_handler("seed",
                           boost::bind(&simple_wallet::seed, this, _1),
                           tr("Display the Electrum-style mnemonic seed"));
  m_cmd_binder.set_handler("set",
                           boost::bind(&simple_wallet::set_variable, this, _1),
                           tr("set <option> [<value>]"),
                           tr("Available options:\n "
                                  "seed language\n "
                                  "  Set the wallet's seed language.\n "
                                  "always-confirm-transfers <1|0>\n "
                                  "  Whether to confirm unsplit txes.\n "
                                  "print-ring-members <1|0>\n "
                                  "  Whether to print detailed information about ring members during confirmation.\n "
                                  "store-tx-info <1|0>\n "
                                  "  Whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference.\n "
                                  "default-ring-size <n>\n "
                                  "  Set the default ring size (default and minimum is 5).\n "
                                  "auto-refresh <1|0>\n "
                                  "  Whether to automatically synchronize new blocks from the daemon.\n "
                                  "refresh-type <full|optimize-coinbase|no-coinbase|default>\n "
                                  "  Set the wallet's refresh behaviour.\n "
                                  "priority [0|1|2|3|4]\n "
                                  "  Set the fee too default/unimportant/normal/elevated/priority.\n "
                                  "confirm-missing-payment-id <1|0>\n "
                                  "ask-password <1|0>\n "
                                  "unit <monero|millinero|micronero|nanonero|piconero>\n "
                                  "  Set the default monero (sub-)unit.\n "
                                  "min-outputs-count [n]\n "
                                  "  Try to keep at least that many outputs of value at least min-outputs-value.\n "
                                  "min-outputs-value [n]\n "
                                  "  Try to keep at least min-outputs-count outputs of at least that value.\n "
                                  "merge-destinations <1|0>\n "
                                  "  Whether to merge multiple payments to the same destination address.\n "
                                  "confirm-backlog <1|0>\n "
                                  "  Whether to warn if there is transaction backlog.\n "
                                  "confirm-backlog-threshold [n]\n "
                                  "  Set a threshold for confirm-backlog to only warn if the transaction backlog is greater than n blocks.\n "
                                  "refresh-from-block-height [n]\n "
                                  "  Set the height before which to ignore blocks.\n "
                                  "auto-low-priority <1|0>\n "
                                  "  Whether to automatically use the low priority fee level when it's safe to do so.\n "
                                  "segregate-pre-fork-outputs <1|0>\n "
                                  "  Set this if you intend to spend outputs on both Monero AND a key reusing fork.\n "
                                  "key-reuse-mitigation2 <1|0>\n "
                                  "  Set this if you are not sure whether you will spend on a key reusing Monero fork later.\n"
                                  "subaddress-lookahead <major>:<minor>\n "
                                  "  Set the lookahead sizes for the subaddress hash table.\n "
                                  "  Set this if you are not sure whether you will spend on a key reusing Monero fork later.\n "
                                  "segregation-height <n>\n "
                                  "  Set to the height of a key reusing fork you want to use, 0 to use default."));
  m_cmd_binder.set_handler("encrypted_seed",
                           boost::bind(&simple_wallet::encrypted_seed, this, _1),
                           tr("Display the encrypted Electrum-style mnemonic seed."));
  m_cmd_binder.set_handler("rescan_spent",
                           boost::bind(&simple_wallet::rescan_spent, this, _1),
                           tr("Rescan the blockchain for spent outputs."));
  m_cmd_binder.set_handler("get_tx_key",
                           boost::bind(&simple_wallet::get_tx_key, this, _1),
                           tr("get_tx_key <txid>"),
                           tr("Get the transaction key (r) for a given <txid>."));
  m_cmd_binder.set_handler("check_tx_key",
                           boost::bind(&simple_wallet::check_tx_key, this, _1),
                           tr("check_tx_key <txid> <txkey> <address>"),
                           tr("Check the amount going to <address> in <txid>."));
  m_cmd_binder.set_handler("get_tx_proof",
                           boost::bind(&simple_wallet::get_tx_proof, this, _1),
                           tr("get_tx_proof <txid> <address> [<message>]"),
                           tr("Generate a signature proving funds sent to <address> in <txid>, optionally with a challenge string <message>, using either the transaction secret key (when <address> is not your wallet's address) or the view secret key (otherwise), which does not disclose the secret key."));
  m_cmd_binder.set_handler("check_tx_proof",
                           boost::bind(&simple_wallet::check_tx_proof, this, _1),
                           tr("check_tx_proof <txid> <address> <signature_file> [<message>]"),
                           tr("Check the proof for funds going to <address> in <txid> with the challenge string <message> if any."));
  m_cmd_binder.set_handler("get_spend_proof",
                           boost::bind(&simple_wallet::get_spend_proof, this, _1),
                           tr("get_spend_proof <txid> [<message>]"),
                           tr("Generate a signature proving that you generated <txid> using the spend secret key, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("check_spend_proof",
                           boost::bind(&simple_wallet::check_spend_proof, this, _1),
                           tr("check_spend_proof <txid> <signature_file> [<message>]"),
                           tr("Check a signature proving that the signer generated <txid>, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("get_reserve_proof",
                           boost::bind(&simple_wallet::get_reserve_proof, this, _1),
                           tr("get_reserve_proof (all|<amount>) [<message>]"),
                           tr("Generate a signature proving that you own at least this much, optionally with a challenge string <message>.\n"
                              "If 'all' is specified, you prove the entire sum of all of your existing accounts' balances.\n"
                              "Otherwise, you prove the reserve of the smallest possible amount above <amount> available in your current account."));
  m_cmd_binder.set_handler("check_reserve_proof",
                           boost::bind(&simple_wallet::check_reserve_proof, this, _1),
                           tr("check_reserve_proof <address> <signature_file> [<message>]"),
                           tr("Check a signature proving that the owner of <address> holds at least this much, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("show_transfers",
                           boost::bind(&simple_wallet::show_transfers, this, _1),
                           tr("show_transfers [in|out|pending|failed|pool] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]"),
                           tr("Show the incoming/outgoing transfers within an optional height range."));
  m_cmd_binder.set_handler("unspent_outputs",
                           boost::bind(&simple_wallet::unspent_outputs, this, _1),
                           tr("unspent_outputs [index=<N1>[,<N2>,...]] [<min_amount> [<max_amount>]]"),
                           tr("Show the unspent outputs of a specified address within an optional amount range."));
  m_cmd_binder.set_handler("rescan_bc",
                           boost::bind(&simple_wallet::rescan_blockchain, this, _1),
                           tr("Rescan the blockchain from scratch."));
  m_cmd_binder.set_handler("set_tx_note",
                           boost::bind(&simple_wallet::set_tx_note, this, _1),
                           tr("set_tx_note <txid> [free text note]"),
                           tr("Set an arbitrary string note for a <txid>."));
  m_cmd_binder.set_handler("get_tx_note",
                           boost::bind(&simple_wallet::get_tx_note, this, _1),
                           tr("get_tx_note <txid>"),
                           tr("Get a string note for a txid."));
  m_cmd_binder.set_handler("set_description",
                           boost::bind(&simple_wallet::set_description, this, _1),
                           tr("set_description [free text note]"),
                           tr("Set an arbitrary description for the wallet."));
  m_cmd_binder.set_handler("get_description",
                           boost::bind(&simple_wallet::get_description, this, _1),
                           tr("Get the description of the wallet."));
  m_cmd_binder.set_handler("status",
                           boost::bind(&simple_wallet::status, this, _1),
                           tr("Show the wallet's status."));
  m_cmd_binder.set_handler("wallet_info",
                           boost::bind(&simple_wallet::wallet_info, this, _1),
                           tr("Show the wallet's information."));
  m_cmd_binder.set_handler("sign",
                           boost::bind(&simple_wallet::sign, this, _1),
                           tr("sign <file>"),
                           tr("Sign the contents of a file."));
  m_cmd_binder.set_handler("verify",
                           boost::bind(&simple_wallet::verify, this, _1),
                           tr("verify <filename> <address> <signature>"),
                           tr("Verify a signature on the contents of a file."));
  m_cmd_binder.set_handler("export_key_images",
                           boost::bind(&simple_wallet::export_key_images, this, _1),
                           tr("export_key_images <file>"),
                           tr("Export a signed set of key images to a <file>."));
  m_cmd_binder.set_handler("import_key_images",
                           boost::bind(&simple_wallet::import_key_images, this, _1),
                           tr("import_key_images <file>"),
                           tr("Import a signed key images list and verify their spent status."));
  m_cmd_binder.set_handler("export_outputs",
                           boost::bind(&simple_wallet::export_outputs, this, _1),
                           tr("export_outputs <file>"),
                           tr("Export a set of outputs owned by this wallet."));
  m_cmd_binder.set_handler("import_outputs",
                           boost::bind(&simple_wallet::import_outputs, this, _1),
                           tr("import_outputs <file>"),
                           tr("Import a set of outputs owned by this wallet."));
  m_cmd_binder.set_handler("show_transfer",
                           boost::bind(&simple_wallet::show_transfer, this, _1),
                           tr("show_transfer <txid>"),
                           tr("Show information about a transfer to/from this address."));
  m_cmd_binder.set_handler("password",
                           boost::bind(&simple_wallet::change_password, this, _1),
                           tr("Change the wallet's password."));
  m_cmd_binder.set_handler("payment_id",
                           boost::bind(&simple_wallet::payment_id, this, _1),
                           tr("Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids."));
  m_cmd_binder.set_handler("fee",
                           boost::bind(&simple_wallet::print_fee_info, this, _1),
                           tr("Print the information about the current fee and transaction backlog."));
  m_cmd_binder.set_handler("prepare_multisig", boost::bind(&simple_wallet::prepare_multisig, this, _1),
                           tr("Export data needed to create a multisig wallet"));
  m_cmd_binder.set_handler("make_multisig", boost::bind(&simple_wallet::make_multisig, this, _1),
                           tr("make_multisig <threshold> <string1> [<string>...]"),
                           tr("Turn this wallet into a multisig wallet"));
  m_cmd_binder.set_handler("finalize_multisig",
                           boost::bind(&simple_wallet::finalize_multisig, this, _1),
                           tr("finalize_multisig <string> [<string>...]"),
                           tr("Turn this wallet into a multisig wallet, extra step for N-1/N wallets"));
  m_cmd_binder.set_handler("export_multisig_info",
                           boost::bind(&simple_wallet::export_multisig, this, _1),
                           tr("export_multisig_info <filename>"),
                           tr("Export multisig info for other participants"));
  m_cmd_binder.set_handler("import_multisig_info",
                           boost::bind(&simple_wallet::import_multisig, this, _1),
                           tr("import_multisig_info <filename> [<filename>...]"),
                           tr("Import multisig info from other participants"));
  m_cmd_binder.set_handler("sign_multisig",
                           boost::bind(&simple_wallet::sign_multisig, this, _1),
                           tr("sign_multisig <filename>"),
                           tr("Sign a multisig transaction from a file"));
  m_cmd_binder.set_handler("submit_multisig",
                           boost::bind(&simple_wallet::submit_multisig, this, _1),
                           tr("submit_multisig <filename>"),
                           tr("Submit a signed multisig transaction from a file"));
  m_cmd_binder.set_handler("export_raw_multisig_tx",
                           boost::bind(&simple_wallet::export_raw_multisig, this, _1),
                           tr("export_raw_multisig_tx <filename>"),
                           tr("Export a signed multisig transaction to a file"));
  m_cmd_binder.set_handler("print_ring",
                           boost::bind(&simple_wallet::print_ring, this, _1),
                           tr("print_ring <key_image> | <txid>"),
                           tr("Print the ring(s) used to spend a given key image or transaction (if the ring size is > 1)"));
  m_cmd_binder.set_handler("set_ring",
                           boost::bind(&simple_wallet::set_ring, this, _1),
                           tr("set_ring <key_image> absolute|relative <index> [<index>...]"),
                           tr("Set the ring used for a given key image, so it can be reused in a fork"));
  m_cmd_binder.set_handler("save_known_rings",
                           boost::bind(&simple_wallet::save_known_rings, this, _1),
                           tr("save_known_rings"),
                           tr("Save known rings to the shared rings database"));
  m_cmd_binder.set_handler("blackball",
                           boost::bind(&simple_wallet::blackball, this, _1),
                           tr("blackball <output public key> | <filename> [add]"),
                           tr("Blackball output(s) so they never get selected as fake outputs in a ring"));
  m_cmd_binder.set_handler("unblackball",
                           boost::bind(&simple_wallet::unblackball, this, _1),
                           tr("unblackball <output public key>"),
                           tr("Unblackballs an output so it may get selected as a fake output in a ring"));
  m_cmd_binder.set_handler("blackballed",
                           boost::bind(&simple_wallet::blackballed, this, _1),
                           tr("blackballed <output public key>"),
                           tr("Checks whether an output is blackballed"));
  m_cmd_binder.set_handler("version",
                           boost::bind(&simple_wallet::version, this, _1),
                           tr("version"),
                           tr("Returns version information"));
  m_cmd_binder.set_handler("help",
                           boost::bind(&simple_wallet::help, this, _1),
                           tr("help [<command>]"),
                           tr("Show the help section or the documentation about a <command>."));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    std::string seed_language = m_wallet->get_seed_language();
    if (m_use_english_language_names)
      seed_language = crypto::ElectrumWords::get_english_name_for(seed_language);
    success_msg_writer() << "seed = " << seed_language;
    success_msg_writer() << "always-confirm-transfers = " << m_wallet->always_confirm_transfers();
    success_msg_writer() << "print-ring-members = " << m_wallet->print_ring_members();
    success_msg_writer() << "store-tx-info = " << m_wallet->store_tx_info();
    success_msg_writer() << "default-ring-size = " << (m_wallet->default_mixin() ? m_wallet->default_mixin() + 1 : 0);
    success_msg_writer() << "auto-refresh = " << m_wallet->auto_refresh();
    success_msg_writer() << "refresh-type = " << get_refresh_type_name(m_wallet->get_refresh_type());
    success_msg_writer() << "priority = " << m_wallet->get_default_priority();
    success_msg_writer() << "confirm-missing-payment-id = " << m_wallet->confirm_missing_payment_id();
    success_msg_writer() << "ask-password = " << m_wallet->ask_password();
    success_msg_writer() << "unit = " << cryptonote::get_unit(cryptonote::get_default_decimal_point());
    success_msg_writer() << "min-outputs-count = " << m_wallet->get_min_output_count();
    success_msg_writer() << "min-outputs-value = " << cryptonote::print_money(m_wallet->get_min_output_value());
    success_msg_writer() << "merge-destinations = " << m_wallet->merge_destinations();
    success_msg_writer() << "confirm-backlog = " << m_wallet->confirm_backlog();
    success_msg_writer() << "confirm-backlog-threshold = " << m_wallet->get_confirm_backlog_threshold();
    success_msg_writer() << "confirm-export-overwrite = " << m_wallet->confirm_export_overwrite();
    success_msg_writer() << "refresh-from-block-height = " << m_wallet->get_refresh_from_block_height();
    success_msg_writer() << "auto-low-priority = " << m_wallet->auto_low_priority();
    success_msg_writer() << "segregate-pre-fork-outputs = " << m_wallet->segregate_pre_fork_outputs();
    success_msg_writer() << "key-reuse-mitigation2 = " << m_wallet->key_reuse_mitigation2();
    const std::pair<size_t, size_t> lookahead = m_wallet->get_subaddress_lookahead();
    success_msg_writer() << "subaddress-lookahead = " << lookahead.first << ":" << lookahead.second;
    success_msg_writer() << "segregation-height = " << m_wallet->segregation_height();
    return true;
  }
  else
  {

#define CHECK_SIMPLE_VARIABLE(name, f, help) do \
  if (args[0] == name) { \
    if (args.size() <= 1) \
    { \
      fail_msg_writer() << "set " << #name << ": " << tr("needs an argument") << " (" << help << ")"; \
      return true; \
    } \
    else \
    { \
      f(args); \
      return true; \
    } \
  } while(0)

    if (args[0] == "seed")
    {
      if (args.size() == 1)
      {
        fail_msg_writer() << tr("set seed: needs an argument. available options: language");
        return true;
      }
      else if (args[1] == "language")
      {
        seed_set_language(args);
        return true;
      }
    }
    CHECK_SIMPLE_VARIABLE("always-confirm-transfers", set_always_confirm_transfers, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("print-ring-members", set_print_ring_members, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("store-tx-info", set_store_tx_info, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("default-ring-size", set_default_ring_size, tr("integer >= ") << MIN_RING_SIZE);
    CHECK_SIMPLE_VARIABLE("auto-refresh", set_auto_refresh, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("refresh-type", set_refresh_type, tr("full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)"));
    CHECK_SIMPLE_VARIABLE("priority", set_default_priority, tr("0, 1, 2, 3, or 4"));
    CHECK_SIMPLE_VARIABLE("confirm-missing-payment-id", set_confirm_missing_payment_id, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("ask-password", set_ask_password, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("unit", set_unit, tr("monero, millinero, micronero, nanonero, piconero"));
    CHECK_SIMPLE_VARIABLE("min-outputs-count", set_min_output_count, tr("unsigned integer"));
    CHECK_SIMPLE_VARIABLE("min-outputs-value", set_min_output_value, tr("amount"));
    CHECK_SIMPLE_VARIABLE("merge-destinations", set_merge_destinations, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("confirm-backlog", set_confirm_backlog, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("confirm-backlog-threshold", set_confirm_backlog_threshold, tr("unsigned integer"));
    CHECK_SIMPLE_VARIABLE("confirm-export-overwrite", set_confirm_export_overwrite, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("refresh-from-block-height", set_refresh_from_block_height, tr("block height"));
    CHECK_SIMPLE_VARIABLE("auto-low-priority", set_auto_low_priority, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("segregate-pre-fork-outputs", set_segregate_pre_fork_outputs, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("key-reuse-mitigation2", set_key_reuse_mitigation2, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("subaddress-lookahead", set_subaddress_lookahead, tr("<major>:<minor>"));
    CHECK_SIMPLE_VARIABLE("segregation-height", set_segregation_height, tr("unsigned integer"));
  }
  fail_msg_writer() << tr("set: unrecognized argument(s)");
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() > 1)
  {
    fail_msg_writer() << tr("usage: set_log <log_level_number_0-4> | <categories>");
    return true;
  }
  if (!args.empty())
    mlog_set_log(args[0].c_str());
  success_msg_writer() << "New log categories: " << mlog_get_categories();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::ask_wallet_create_if_needed()
{
  LOG_PRINT_L3("simple_wallet::ask_wallet_create_if_needed() started");
  std::string wallet_path;
  std::string confirm_creation;
  bool wallet_name_valid = false;
  bool keys_file_exists;
  bool wallet_file_exists;

  do{
      LOG_PRINT_L3("User asked to specify wallet file name.");
      wallet_path = input_line(
        tr(m_restoring ? "Specify a new wallet file name for your restored wallet (e.g., MyWallet).\n"
        "Wallet file name (or Ctrl-C to quit): " :
        "Specify wallet file name (e.g., MyWallet). If the wallet doesn't exist, it will be created.\n"
        "Wallet file name (or Ctrl-C to quit): ")
      );
      if(std::cin.eof())
      {
        LOG_ERROR("Unexpected std::cin.eof() - Exited simple_wallet::ask_wallet_create_if_needed()");
        return false;
      }
      if(!tools::wallet2::wallet_valid_path_format(wallet_path))
      {
        fail_msg_writer() << tr("Wallet name not valid. Please try again or use Ctrl-C to quit.");
        wallet_name_valid = false;
      }
      else
      {
        tools::wallet2::wallet_exists(wallet_path, keys_file_exists, wallet_file_exists);
        LOG_PRINT_L3("wallet_path: " << wallet_path << "");
        LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
        << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);

        if((keys_file_exists || wallet_file_exists) && (!m_generate_new.empty() || m_restoring))
        {
          fail_msg_writer() << tr("Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.");
          return false;
        }
        if(wallet_file_exists && keys_file_exists) //Yes wallet, yes keys
        {
          success_msg_writer() << tr("Wallet and key files found, loading...");
          m_wallet_file = wallet_path;
          return true;
        }
        else if(!wallet_file_exists && keys_file_exists) //No wallet, yes keys
        {
          success_msg_writer() << tr("Key file found but not wallet file. Regenerating...");
          m_wallet_file = wallet_path;
          return true;
        }
        else if(wallet_file_exists && !keys_file_exists) //Yes wallet, no keys
        {
          fail_msg_writer() << tr("Key file not found. Failed to open wallet: ") << "\"" << wallet_path << "\". Exiting.";
          return false;
        }
        else if(!wallet_file_exists && !keys_file_exists) //No wallet, no keys
        {
          bool ok = true;
          if (!m_restoring)
          {
            message_writer() << tr("No wallet found with that name. Confirm creation of new wallet named: ") << wallet_path;
            confirm_creation = input_line(tr("(Y/Yes/N/No): "));
            if(std::cin.eof())
            {
              LOG_ERROR("Unexpected std::cin.eof() - Exited simple_wallet::ask_wallet_create_if_needed()");
              return false;
            }
            ok = command_line::is_yes(confirm_creation);
          }
          if (ok)
          {
            success_msg_writer() << tr("Generating new wallet...");
            m_generate_new = wallet_path;
            return true;
          }
        }
      }
    } while(!wallet_name_valid);

  LOG_ERROR("Failed out of do-while loop in ask_wallet_create_if_needed()");
  return false;
}

/*!
 * \brief Prints the seed with a nice message
 * \param seed seed to print
 */
void simple_wallet::print_seed(std::string seed)
{
  success_msg_writer(true) << "\n" << tr("NOTE: the following 25 words can be used to recover access to your wallet. "
    "Write them down and store them somewhere safe and secure. Please do not store them in "
    "your email or on file storage services outside of your immediate control.\n");
  boost::replace_nth(seed, " ", 15, "\n");
  boost::replace_nth(seed, " ", 7, "\n");
  // don't log
  std::cout << seed << std::endl;
}
//----------------------------------------------------------------------------------------------------
static bool might_be_partial_seed(std::string words)
{
  std::vector<std::string> seed;

  boost::algorithm::trim(words);
  boost::split(seed, words, boost::is_any_of(" "), boost::token_compress_on);
  return seed.size() < 24;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  const bool testnet = tools::wallet2::has_testnet_option(vm);
  const bool stagenet = tools::wallet2::has_stagenet_option(vm);
  if (testnet && stagenet)
  {
    fail_msg_writer() << tr("Can't specify more than one of --testnet and --stagenet");
    return false;
  }
  const network_type nettype = testnet ? TESTNET : stagenet ? STAGENET : MAINNET;

  std::string multisig_keys;

  if (!handle_command_line(vm))
    return false;

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_device.empty()) + (!m_generate_from_view_key.empty()) + (!m_generate_from_spend_key.empty()) + (!m_generate_from_keys.empty()) + (!m_generate_from_multisig_keys.empty()) + (!m_generate_from_json.empty()) > 1)
  {
    fail_msg_writer() << tr("can't specify more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\", --generate-from-view-key=\"wallet_name\", --generate-from-spend-key=\"wallet_name\", --generate-from-keys=\"wallet_name\", --generate-from-multisig-keys=\"wallet_name\", --generate-from-json=\"jsonfilename\" and --generate-from-device=\"wallet_name\"");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_device.empty() && m_generate_from_view_key.empty() && m_generate_from_spend_key.empty() && m_generate_from_keys.empty() && m_generate_from_multisig_keys.empty() && m_generate_from_json.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  if (!m_generate_new.empty() || m_restoring)
  {
    if (!m_subaddress_lookahead.empty() && !parse_subaddress_lookahead(m_subaddress_lookahead))
      return false;

    std::string old_language;
    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet || m_restore_multisig_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << tr("can't specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic");
        return false;
      }
      if (!m_wallet_file.empty())
      {
        if (m_restore_multisig_wallet)
          fail_msg_writer() << tr("--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file");
        else
          fail_msg_writer() << tr("--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file");
        return false;
      }

      if (m_electrum_seed.empty())
      {
        if (m_restore_multisig_wallet)
        {
            const char *prompt = "Specify multisig seed: ";
            m_electrum_seed = input_line(prompt);
            if (std::cin.eof())
              return false;
            if (m_electrum_seed.empty())
            {
              fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"multisig seed here\"");
              return false;
            }
        }
        else
        {
          m_electrum_seed = "";
          do
          {
            const char *prompt = m_electrum_seed.empty() ? "Specify Electrum seed: " : "Electrum seed continued: ";
            std::string electrum_seed = input_line(prompt);
            if (std::cin.eof())
              return false;
            if (electrum_seed.empty())
            {
              fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"words list here\"");
              return false;
            }
            m_electrum_seed += electrum_seed + " ";
          } while (might_be_partial_seed(m_electrum_seed));
        }
      }

      if (m_restore_multisig_wallet)
      {
        if (!epee::string_tools::parse_hexstr_to_binbuff(m_electrum_seed, multisig_keys))
        {
          fail_msg_writer() << tr("Multisig seed failed verification");
          return false;
        }
      }
      else
      {
        if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
        {
          fail_msg_writer() << tr("Electrum-style word list failed verification");
          return false;
        }
      }

      auto pwd_container = password_prompter(tr("Enter seed encryption passphrase, empty if none"), false);
      if (std::cin.eof() || !pwd_container)
        return false;
      epee::wipeable_string seed_pass = pwd_container->password();
      if (!seed_pass.empty())
      {
        if (m_restore_multisig_wallet)
        {
          crypto::secret_key key;
          crypto::cn_slow_hash(seed_pass.data(), seed_pass.size(), (crypto::hash&)key);
          sc_reduce32((unsigned char*)key.data);
          multisig_keys = m_wallet->decrypt(multisig_keys, key, true);
        }
        else
          m_recovery_key = cryptonote::decrypt_key(m_recovery_key, seed_pass);
      }
    }
    if (!m_generate_from_view_key.empty())
    {
      m_wallet_file = m_generate_from_view_key;
      // parse address
      std::string address_string = input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::address_parse_info info;
      if(!get_account_address_from_str(info, nettype, address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      if (info.is_subaddress)
      {
        fail_msg_writer() << tr("This address is a subaddress which cannot be used here.");
        return false;
      }

      // parse view secret key
      std::string viewkey_string = input_line("View key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
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
      if (info.address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }

      bool r = new_wallet(vm, info.address, boost::none, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_spend_key.empty())
    {
      m_wallet_file = m_generate_from_spend_key;
      // parse spend secret key
      std::string spendkey_string = input_line("Secret spend key: ");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      if (!epee::string_tools::hex_to_pod(spendkey_string, m_recovery_key))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }
      bool r = new_wallet(vm, m_recovery_key, true, false, "");
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_keys.empty())
    {
      m_wallet_file = m_generate_from_keys;
      // parse address
      std::string address_string = input_line("Standard address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::address_parse_info info;
      if(!get_account_address_from_str(info, nettype, address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      if (info.is_subaddress)
      {
        fail_msg_writer() << tr("This address is a subaddress which cannot be used here.");
        return false;
      }

      // parse spend secret key
      std::string spendkey_string = input_line("Secret spend key: ");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata spendkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }
      crypto::secret_key spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());

      // parse view secret key
      std::string viewkey_string = input_line("Secret view key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
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
      if (info.address.m_spend_public_key != pkey) {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
      if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        fail_msg_writer() << tr("failed to verify view key secret key");
        return false;
      }
      if (info.address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }
      bool r = new_wallet(vm, info.address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    
    // Asks user for all the data required to merge secret keys from multisig wallets into one master wallet, which then gets full control of the multisig wallet. The resulting wallet will be the same as any other regular wallet.
    else if (!m_generate_from_multisig_keys.empty())
    {
      m_wallet_file = m_generate_from_multisig_keys;
      unsigned int multisig_m;
      unsigned int multisig_n;
      
      // parse multisig type
      std::string multisig_type_string = input_line("Multisig type (input as M/N with M <= N and M > 1): ");
      if (std::cin.eof())
        return false;
      if (multisig_type_string.empty())
      {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      if (sscanf(multisig_type_string.c_str(), "%u/%u", &multisig_m, &multisig_n) != 2)
      {
        fail_msg_writer() << tr("Error: expected M/N, but got: ") << multisig_type_string;
        return false;
      }
      if (multisig_m <= 1 || multisig_m > multisig_n)
      {
        fail_msg_writer() << tr("Error: expected N > 1 and N <= M, but got: ") << multisig_type_string;
        return false;
      }
      if (multisig_m != multisig_n)
      {
        fail_msg_writer() << tr("Error: M/N is currently unsupported. ");
        return false;
      }      
      message_writer() << boost::format(tr("Generating master wallet from %u of %u multisig wallet keys")) % multisig_m % multisig_n;
      
      // parse multisig address
      std::string address_string = input_line("Multisig wallet address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::address_parse_info info;
      if(!get_account_address_from_str(info, nettype, address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      
      // parse secret view key
      std::string viewkey_string = input_line("Secret view key: ");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty())
      {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
      {
        fail_msg_writer() << tr("failed to parse secret view key");
        return false;
      }
      crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());
      
      // check that the view key matches the given address
      crypto::public_key pkey;
      if (!crypto::secret_key_to_public_key(viewkey, pkey))
      {
        fail_msg_writer() << tr("failed to verify secret view key");
        return false;
      }
      if (info.address.m_view_public_key != pkey)
      {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }
      
      // parse multisig spend keys
      crypto::secret_key spendkey;
      // parsing N/N
      if(multisig_m == multisig_n)
      {
        std::vector<crypto::secret_key> multisig_secret_spendkeys(multisig_n);
        std::string spendkey_string;
        cryptonote::blobdata spendkey_data;
        // get N secret spend keys from user
        for(unsigned int i=0; i<multisig_n; ++i)
        {
          spendkey_string = input_line(tr((boost::format(tr("Secret spend key (%u of %u):")) % (i+1) % multisig_m).str().c_str()));
          if (std::cin.eof())
            return false;
          if (spendkey_string.empty())
          {
            fail_msg_writer() << tr("No data supplied, cancelled");
            return false;
          }
          if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
          {
            fail_msg_writer() << tr("failed to parse spend key secret key");
            return false;
          }
          multisig_secret_spendkeys[i] = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
        }
        
        // sum the spend keys together to get the master spend key
        spendkey = multisig_secret_spendkeys[0];
        for(unsigned int i=1; i<multisig_n; ++i)
          sc_add(reinterpret_cast<unsigned char*>(&spendkey), reinterpret_cast<unsigned char*>(&spendkey), reinterpret_cast<unsigned char*>(&multisig_secret_spendkeys[i]));
      }
      // parsing M/N
      else
      {
        fail_msg_writer() << tr("Error: M/N is currently unsupported");
        return false;
      }
      
      // check that the spend key matches the given address
      if (!crypto::secret_key_to_public_key(spendkey, pkey))
      {
        fail_msg_writer() << tr("failed to verify spend key secret key");
        return false;
      }
      if (info.address.m_spend_public_key != pkey)
      {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
      
      // create wallet
      bool r = new_wallet(vm, info.address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    
    else if (!m_generate_from_json.empty())
    {
      m_wallet_file = m_generate_from_json;
      try
      {
        m_wallet = tools::wallet2::make_from_json(vm, m_wallet_file, password_prompter);
      }
      catch (const std::exception &e)
      {
        fail_msg_writer() << e.what();
        return false;
      }
      if (!m_wallet)
        return false;
    }
    else if (!m_generate_from_device.empty())
    {
      m_wallet_file = m_generate_from_device;
      // create wallet
      bool r = new_wallet(vm, "Ledger");
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      // if no block_height is specified, assume its a new account and start it "now"
      if(m_wallet->get_refresh_from_block_height() == 0) {
        {
          tools::scoped_message_writer wrt = tools::msg_writer();
          wrt << tr("No restore height is specified.");
          wrt << tr("Assumed you are creating a new account, restore will be done from current estimated blockchain height.");
          wrt << tr("Use --restore-height if you want to restore an already setup account from a specific height");
        }
        std::string confirm = input_line(tr("Is this okay?  (Y/Yes/N/No): "));
        if (std::cin.eof() || !command_line::is_yes(confirm))
          CHECK_AND_ASSERT_MES(false, false, tr("account creation aborted"));

        m_wallet->set_refresh_from_block_height(m_wallet->estimate_blockchain_height()-1);
        m_wallet->explicit_refresh_from_block_height(true);
        m_restore_height = m_wallet->get_refresh_from_block_height();
      }
    }
    else
    {
      if (m_generate_new.empty()) {
        fail_msg_writer() << tr("specify a wallet path with --generate-new-wallet (not --wallet-file)");
        return false;
      }
      m_wallet_file = m_generate_new;
      bool r;
      if (m_restore_multisig_wallet)
        r = new_wallet(vm, multisig_keys, old_language);
      else
        r = new_wallet(vm, m_recovery_key, m_restore_deterministic_wallet, m_non_deterministic, old_language);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }

    if (m_restoring && m_generate_from_json.empty() && m_generate_from_device.empty())
    {
      m_wallet->explicit_refresh_from_block_height(!command_line::is_arg_defaulted(vm, arg_restore_height));
    }
    if (!m_wallet->explicit_refresh_from_block_height() && m_restoring)
    {
      uint32_t version;
      bool connected = try_connect_to_daemon(false, &version);
      while (true)
      {
        std::string heightstr;
        if (!connected || version < MAKE_CORE_RPC_VERSION(1, 6))
          heightstr = input_line("Restore from specific blockchain height (optional, default 0): ");
        else
          heightstr = input_line("Restore from specific blockchain height (optional, default 0),\nor alternatively from specific date (YYYY-MM-DD): ");
        if (std::cin.eof())
          return false;
        if (heightstr.empty())
        {
          m_restore_height = 0;
          break;
        }
        try
        {
          m_restore_height = boost::lexical_cast<uint64_t>(heightstr);
          break;
        }
        catch (const boost::bad_lexical_cast &)
        {
          if (!connected || version < MAKE_CORE_RPC_VERSION(1, 6))
          {
            fail_msg_writer() << tr("bad m_restore_height parameter: ") << heightstr;
            continue;
          }
          if (heightstr.size() != 10 || heightstr[4] != '-' || heightstr[7] != '-')
          {
            fail_msg_writer() << tr("date format must be YYYY-MM-DD");
            continue;
          }
          uint16_t year;
          uint8_t month;  // 1, 2, ..., 12
          uint8_t day;    // 1, 2, ..., 31
          try
          {
            year  = boost::lexical_cast<uint16_t>(heightstr.substr(0,4));
            // lexical_cast<uint8_t> won't work because uint8_t is treated as character type
            month = boost::lexical_cast<uint16_t>(heightstr.substr(5,2));
            day   = boost::lexical_cast<uint16_t>(heightstr.substr(8,2));
            m_restore_height = m_wallet->get_blockchain_height_by_date(year, month, day);
            success_msg_writer() << tr("Restore height is: ") << m_restore_height;
            std::string confirm = input_line(tr("Is this okay?  (Y/Yes/N/No): "));
            if (std::cin.eof())
              return false;
            if(command_line::is_yes(confirm))
              break;
          }
          catch (const boost::bad_lexical_cast &)
          {
            fail_msg_writer() << tr("bad m_restore_height parameter: ") << heightstr;
          }
          catch (const std::runtime_error& e)
          {
            fail_msg_writer() << e.what();
          }
        }
      }
    }
    if (m_restoring)
    {
      uint64_t estimate_height = m_wallet->estimate_blockchain_height();
      if (m_restore_height >= estimate_height)
      {
        success_msg_writer() << tr("Restore height ") << m_restore_height << (" is not yet reached. The current estimated height is ") << estimate_height;
        std::string confirm = input_line(tr("Still apply restore height?  (Y/Yes/N/No): "));
        if (std::cin.eof() || command_line::is_no(confirm))
          m_restore_height = 0;
      }
      m_wallet->set_refresh_from_block_height(m_restore_height);
    }
  }
  else
  {
    assert(!m_wallet_file.empty());
    if (!m_subaddress_lookahead.empty())
    {
      fail_msg_writer() << tr("can't specify --subaddress-lookahead and --wallet-file at the same time");
      return false;
    }
    bool r = open_wallet(vm);
    CHECK_AND_ASSERT_MES(r, false, tr("failed to open account"));
  }
  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return false;
  }

  // set --trusted-daemon if local and not overridden
  if (!m_trusted_daemon)
  {
    try
    {
      m_trusted_daemon = false;
      if (tools::is_local_address(m_wallet->get_daemon_address()))
      {
        MINFO(tr("Daemon is local, assuming trusted"));
        m_trusted_daemon = true;
      }
    }
    catch (const std::exception &e) { }
  }

  if (!is_daemon_trusted())
    message_writer() << (boost::format(tr("Warning: using an untrusted daemon at %s, privacy will be lessened")) % m_wallet->get_daemon_address()).str();

  if (m_wallet->get_ring_database().empty())
    fail_msg_writer() << tr("Failed to initialize ring database: privacy enhancing features will be inactive");

  m_wallet->callback(this);

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
  m_generate_from_device          = command_line::get_arg(vm, arg_generate_from_device);
  m_generate_from_view_key        = command_line::get_arg(vm, arg_generate_from_view_key);
  m_generate_from_spend_key       = command_line::get_arg(vm, arg_generate_from_spend_key);
  m_generate_from_keys            = command_line::get_arg(vm, arg_generate_from_keys);
  m_generate_from_multisig_keys   = command_line::get_arg(vm, arg_generate_from_multisig_keys);
  m_generate_from_json            = command_line::get_arg(vm, arg_generate_from_json);
  m_mnemonic_language             = command_line::get_arg(vm, arg_mnemonic_language);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_restore_multisig_wallet       = command_line::get_arg(vm, arg_restore_multisig_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  if (!command_line::is_arg_defaulted(vm, arg_trusted_daemon) || !command_line::is_arg_defaulted(vm, arg_untrusted_daemon))
    m_trusted_daemon = command_line::get_arg(vm, arg_trusted_daemon) && !command_line::get_arg(vm, arg_untrusted_daemon);
  if (!command_line::is_arg_defaulted(vm, arg_trusted_daemon) && !command_line::is_arg_defaulted(vm, arg_untrusted_daemon))
    message_writer() << tr("--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted");
  m_allow_mismatched_daemon_version = command_line::get_arg(vm, arg_allow_mismatched_daemon_version);
  m_restore_height                = command_line::get_arg(vm, arg_restore_height);
  m_do_not_relay                  = command_line::get_arg(vm, arg_do_not_relay);
  m_subaddress_lookahead          = command_line::get_arg(vm, arg_subaddress_lookahead);
  m_use_english_language_names    = command_line::get_arg(vm, arg_use_english_language_names);
  m_restoring                     = !m_generate_from_view_key.empty() ||
                                    !m_generate_from_spend_key.empty() ||
                                    !m_generate_from_keys.empty() ||
                                    !m_generate_from_multisig_keys.empty() ||
                                    !m_generate_from_json.empty() ||
                                    !m_generate_from_device.empty() ||
                                    m_restore_deterministic_wallet ||
                                    m_restore_multisig_wallet;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon(bool silent, uint32_t* version)
{
  uint32_t version_ = 0;
  if (!version)
    version = &version_;
  if (!m_wallet->check_connection(version))
  {
    if (!silent)
      fail_msg_writer() << tr("wallet failed to connect to daemon: ") << m_wallet->get_daemon_address() << ". " <<
        tr("Daemon either is not started or wrong port was passed. "
        "Please make sure daemon is running or change the daemon address using the 'set_daemon' command.");
    return false;
  }
  if (!m_allow_mismatched_daemon_version && ((*version >> 16) != CORE_RPC_VERSION_MAJOR))
  {
    if (!silent)
      fail_msg_writer() << boost::format(tr("Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.")) % (*version>>16) % CORE_RPC_VERSION_MAJOR % m_wallet->get_daemon_address();
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
  crypto::ElectrumWords::get_language_list(language_list, m_use_english_language_names);
  std::cout << tr("List of available languages for your wallet's seed:") << std::endl;
  std::cout << tr("If your display freezes, exit blind with ^C, then run again with --use-english-language-names") << std::endl;
  int ii;
  std::vector<std::string>::iterator it;
  for (it = language_list.begin(), ii = 0; it != language_list.end(); it++, ii++)
  {
    std::cout << ii << " : " << *it << std::endl;
  }
  while (language_number < 0)
  {
    language_choice = input_line(tr("Enter the number corresponding to the language of your choice: "));
    if (std::cin.eof())
      return std::string();
    try
    {
      language_number = std::stoi(language_choice);
      if (!((language_number >= 0) && (static_cast<unsigned int>(language_number) < language_list.size())))
      {
        language_number = -1;
        fail_msg_writer() << tr("invalid language choice entered. Please try again.\n");
      }
    }
    catch (const std::exception &e)
    {
      fail_msg_writer() << tr("invalid language choice entered. Please try again.\n");
    }
  }
  return language_list[language_number];
}
//----------------------------------------------------------------------------------------------------
boost::optional<tools::password_container> simple_wallet::get_and_verify_password() const
{
  auto pwd_container = default_password_prompter(m_wallet_file.empty());
  if (!pwd_container)
    return boost::none;

  if (!m_wallet->verify_password(pwd_container->password()))
  {
    fail_msg_writer() << tr("invalid password");
    return boost::none;
  }
  return pwd_container;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const crypto::secret_key& recovery_key, bool recover, bool two_random, const std::string &old_language)
{
  auto rc = tools::wallet2::make_new(vm, password_prompter);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet->set_subaddress_lookahead(lookahead->first, lookahead->second);
  }

  bool was_deprecated_wallet = m_restore_deterministic_wallet && ((old_language == crypto::ElectrumWords::old_language_name) ||
    crypto::ElectrumWords::get_is_old_style_seed(m_electrum_seed));

  std::string mnemonic_language = old_language;

  std::vector<std::string> language_list;
  crypto::ElectrumWords::get_language_list(language_list);
  if (mnemonic_language.empty() && std::find(language_list.begin(), language_list.end(), m_mnemonic_language) != language_list.end())
  {
    mnemonic_language = m_mnemonic_language;
  }

  // Ask for seed language if:
  // it's a deterministic wallet AND
  // a seed language is not already specified AND
  // (it is not a wallet restore OR if it was a deprecated wallet
  // that was earlier used before this restore)
  if ((!two_random) && (mnemonic_language.empty() || mnemonic_language == crypto::ElectrumWords::old_language_name) && (!m_restore_deterministic_wallet || was_deprecated_wallet))
  {
    if (was_deprecated_wallet)
    {
      // The user had used an older version of the wallet with old style mnemonics.
      message_writer(console_color_green, false) << "\n" << tr("You had been using "
        "a deprecated version of the wallet. Please use the new seed that we provide.\n");
    }
    mnemonic_language = get_mnemonic_language();
    if (mnemonic_language.empty())
      return false;
  }

  m_wallet->set_seed_language(mnemonic_language);

  bool create_address_file = command_line::get_arg(vm, arg_create_address_file);

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(m_wallet_file, std::move(rc.second).password(), recovery_key, recover, two_random, create_address_file);
    message_writer(console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    std::cout << tr("View key: ") << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << ENDL;
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  // convert rng value to electrum-style word list
  std::string electrum_words;

  crypto::ElectrumWords::bytes_to_words(recovery_val, electrum_words, mnemonic_language);

  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Your wallet has been generated!\n"
    "To start synchronizing with the daemon, use the \"refresh\" command.\n"
    "Use the \"help\" command to see the list of available commands.\n"
    "Use \"help <command>\" to see a command's documentation.\n"
    "Always use the \"exit\" command when closing monero-wallet-cli to save \n"
    "your current session's state. Otherwise, you might need to synchronize \n"
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
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const cryptonote::account_public_address& address, const boost::optional<crypto::secret_key>& spendkey,
  const crypto::secret_key& viewkey)
{
  auto rc = tools::wallet2::make_new(vm, password_prompter);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet->set_subaddress_lookahead(lookahead->first, lookahead->second);
  }

  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  bool create_address_file = command_line::get_arg(vm, arg_create_address_file);

  try
  {
    if (spendkey)
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, *spendkey, viewkey, create_address_file);
    }
    else
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, viewkey, create_address_file);
    }
    message_writer(console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }


  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
                               const std::string &device_name) {
  auto rc = tools::wallet2::make_new(vm, password_prompter);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet->set_subaddress_lookahead(lookahead->first, lookahead->second);
  }

  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  try
  {
    m_wallet->restore(m_wallet_file, std::move(rc.second).password(), device_name);
    message_writer(console_color_white, true) << tr("Generated new wallet on hw device: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
    const std::string &multisig_keys, const std::string &old_language)
{
  auto rc = tools::wallet2::make_new(vm, password_prompter);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet->set_subaddress_lookahead(lookahead->first, lookahead->second);
  }

  std::string mnemonic_language = old_language;

  std::vector<std::string> language_list;
  crypto::ElectrumWords::get_language_list(language_list);
  if (mnemonic_language.empty() && std::find(language_list.begin(), language_list.end(), m_mnemonic_language) != language_list.end())
  {
    mnemonic_language = m_mnemonic_language;
  }

  m_wallet->set_seed_language(mnemonic_language);

  bool create_address_file = command_line::get_arg(vm, arg_create_address_file);

  try
  {
    m_wallet->generate(m_wallet_file, std::move(rc.second).password(), multisig_keys, create_address_file);
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total) || !ready)
    {
      fail_msg_writer() << tr("failed to generate new mutlisig wallet");
      return false;
    }
    message_writer(console_color_white, true) << boost::format(tr("Generated new %u/%u multisig wallet: ")) % threshold % total
      << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << e.what();
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::open_wallet(const boost::program_options::variables_map& vm)
{
  if (!tools::wallet2::wallet_valid_path_format(m_wallet_file))
  {
    fail_msg_writer() << tr("wallet file path not valid: ") << m_wallet_file;
    return false;
  }
  epee::wipeable_string password;
  try
  {
    auto rc = tools::wallet2::make_from_file(vm, m_wallet_file, password_prompter);
    m_wallet = std::move(rc.first);
    password = std::move(std::move(rc.second).password());
    if (!m_wallet)
    {
      return false;
    }

    std::string prefix;
    bool ready;
    uint32_t threshold, total;
    if (m_wallet->watch_only())
      prefix = tr("Opened watch-only wallet");
    else if (m_wallet->multisig(&ready, &threshold, &total))
      prefix = (boost::format(tr("Opened %u/%u multisig wallet%s")) % threshold % total % (ready ? "" : " (not yet finalized)")).str();
    else
      prefix = tr("Opened wallet");
    message_writer(console_color_white, true) <<
      prefix << ": " << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    if (m_wallet->get_account().get_device()) {
       message_writer(console_color_white, true) << "Wallet is on device: " << m_wallet->get_account().get_device().get_name();
    }
    // If the wallet file is deprecated, we should ask for mnemonic language again and store
    // everything in the new format.
    // NOTE: this is_deprecated() refers to the wallet file format before becoming JSON. It does not refer to the "old english" seed words form of "deprecated" used elsewhere.
    if (m_wallet->is_deprecated())
    {
      if (m_wallet->is_deterministic())
      {
        message_writer(console_color_green, false) << "\n" << tr("You had been using "
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
        message_writer(console_color_green, false) << "\n" << tr("You had been using "
          "a deprecated version of the wallet. Your wallet file format is being upgraded now.\n");
        m_wallet->rewrite(m_wallet_file, password);
      }
    }
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << tr("failed to load wallet: ") << e.what();
    if (m_wallet)
    {
      // only suggest removing cache if the password was actually correct
      bool password_is_correct = false;
      try
      {
        password_is_correct = m_wallet->verify_password(password);
      }
      catch (...) { } // guard against I/O errors
      if (password_is_correct)
        fail_msg_writer() << boost::format(tr("You may want to remove the file \"%s\" and try again")) % m_wallet_file;
    }
    return false;
  }
  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Use the \"help\" command to see the list of available commands.\n") <<
    tr("Use \"help <command>\" to see a command's documentation.\n") <<
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
  if (m_wallet->multisig())
  {
    fail_msg_writer() << tr("wallet is multisig and cannot save a watch-only version");
    return true;
  }

  const auto pwd_container = password_prompter(tr("Password for new watch-only wallet"), true);

  if (!pwd_container)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  try
  {
    std::string new_keys_filename;
    m_wallet->write_watch_only_wallet(m_wallet_file, pwd_container->password(), new_keys_filename);
    success_msg_writer() << tr("Watch only wallet saved as: ") << new_keys_filename;
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to save watch only wallet: ") << e.what();
    return true;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!is_daemon_trusted())
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }
  COMMAND_RPC_START_MINING::request req = AUTO_VAL_INIT(req); 
  req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());

  bool ok = true;
  size_t max_mining_threads_count = (std::max)(tools::get_max_concurrency(), static_cast<unsigned>(2));
  size_t arg_size = args.size();
  if(arg_size >= 3)
  {
    if (!parse_bool_and_use(args[2], [&](bool r) { req.ignore_battery = r; }))
      return true;
  }
  if(arg_size >= 2)
  {
    if (!parse_bool_and_use(args[1], [&](bool r) { req.do_background_mining = r; }))
      return true;
  }
  if(arg_size >= 1)
  {
    uint16_t num = 1;
    ok = string_tools::get_xtype_from_string(num, args[0]);
    ok = ok && (1 <= num && num <= max_mining_threads_count);
    req.threads_count = num;
  }
  else
  {
    req.threads_count = 1;
  }

  if (!ok)
  {
    fail_msg_writer() << tr("invalid arguments. Please use start_mining [<number_of_threads>] [do_bg_mining] [ignore_battery], "
      "<number_of_threads> should be from 1 to ") << max_mining_threads_count;
    return true;
  }

  COMMAND_RPC_START_MINING::response res;
  bool r = m_wallet->invoke_http_json("/start_mining", req, res);
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

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }

  COMMAND_RPC_STOP_MINING::request req;
  COMMAND_RPC_STOP_MINING::response res;
  bool r = m_wallet->invoke_http_json("/stop_mining", req, res);
  std::string err = interpret_rpc_response(r, res.status);
  if (err.empty())
    success_msg_writer() << tr("Mining stopped in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been stopped: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_daemon(const std::vector<std::string>& args)
{
  std::string daemon_url;

  if (args.size() < 1)
  {
    fail_msg_writer() << tr("missing daemon URL argument");
    return true;
  }

  boost::regex rgx("^(.*://)?([A-Za-z0-9\\-\\.]+)(:[0-9]+)?");
  boost::cmatch match;
  // If user input matches URL regex
  if (boost::regex_match(args[0].c_str(), match, rgx))
  {
    if (match.length() < 4)
    {
      fail_msg_writer() << tr("Unexpected array length - Exited simple_wallet::set_daemon()");
      return true;
    }
    // If no port has been provided, use the default from config
    if (!match[3].length())
    {
      int daemon_port = m_wallet->nettype() == cryptonote::TESTNET ? config::testnet::RPC_DEFAULT_PORT : m_wallet->nettype() == cryptonote::STAGENET ? config::stagenet::RPC_DEFAULT_PORT : config::RPC_DEFAULT_PORT;
      daemon_url = match[1] + match[2] + std::string(":") + std::to_string(daemon_port);
    } else {
      daemon_url = args[0];
    }
    LOCK_IDLE_SCOPE();
    m_wallet->init(daemon_url);

    if (args.size() == 2)
    {
      if (args[1] == "trusted")
        m_trusted_daemon = true;
      else if (args[1] == "untrusted")
        m_trusted_daemon = false;
      else
      {
        fail_msg_writer() << tr("Expected trusted or untrusted, got ") << args[1] << ": assuming untrusted";
        m_trusted_daemon = false;
      }
    }
    else
    {
      m_trusted_daemon = false;
      try
      {
        if (tools::is_local_address(m_wallet->get_daemon_address()))
        {
          MINFO(tr("Daemon is local, assuming trusted"));
          m_trusted_daemon = true;
        }
      }
      catch (const std::exception &e) { }
    }
    success_msg_writer() << boost::format("Daemon set to %s, %s") % daemon_url % (*m_trusted_daemon ? tr("trusted") : tr("untrusted"));
  } else {
    fail_msg_writer() << tr("This does not seem to be a valid daemon URL.");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_bc(const std::vector<std::string>& args)
{
  if (!try_connect_to_daemon())
    return true;

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }
  COMMAND_RPC_SAVE_BC::request req;
  COMMAND_RPC_SAVE_BC::response res;
  bool r = m_wallet->invoke_http_json("/save_bc", req, res);
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
void simple_wallet::on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
{
  message_writer(console_color_green, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("txid ") << txid << ", " <<
    print_money(amount) << ", " <<
    tr("idx ") << subaddr_index;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
{
  // Not implemented in CLI wallet
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index)
{
  message_writer(console_color_magenta, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("txid ") << txid << ", " <<
    tr("spent ") << print_money(amount) << ", " <<
    tr("idx ") << subaddr_index;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx)
{
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh_main(uint64_t start_height, bool reset, bool is_init)
{
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

  if (reset)
    m_wallet->rescan_blockchain(false);

#ifdef HAVE_READLINE
  rdln::suspend_readline pause_readline;
#endif

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
    if (is_init)
      print_accounts();
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
bool simple_wallet::show_balance_unlocked(bool detailed)
{
  std::string extra;
  if (m_wallet->has_multisig_partial_key_images())
    extra = tr(" (Some owned outputs have partial key images - import_multisig_info needed)");
  success_msg_writer() << tr("Currently selected account: [") << m_current_subaddress_account << tr("] ") << m_wallet->get_subaddress_label({m_current_subaddress_account, 0});
  const std::string tag = m_wallet->get_account_tags().second[m_current_subaddress_account];
  success_msg_writer() << tr("Tag: ") << (tag.empty() ? std::string{tr("(No tag assigned)")} : tag);
  success_msg_writer() << tr("Balance: ") << print_money(m_wallet->balance(m_current_subaddress_account)) << ", "
    << tr("unlocked balance: ") << print_money(m_wallet->unlocked_balance(m_current_subaddress_account)) << extra;
  std::map<uint32_t, uint64_t> balance_per_subaddress = m_wallet->balance_per_subaddress(m_current_subaddress_account);
  std::map<uint32_t, uint64_t> unlocked_balance_per_subaddress = m_wallet->unlocked_balance_per_subaddress(m_current_subaddress_account);
  if (!detailed || balance_per_subaddress.empty())
    return true;
  success_msg_writer() << tr("Balance per address:");
  success_msg_writer() << boost::format("%15s %21s %21s %7s %21s") % tr("Address") % tr("Balance") % tr("Unlocked balance") % tr("Outputs") % tr("Label");
  std::vector<tools::wallet2::transfer_details> transfers;
  m_wallet->get_transfers(transfers);
  for (const auto& i : balance_per_subaddress)
  {
    cryptonote::subaddress_index subaddr_index = {m_current_subaddress_account, i.first};
    std::string address_str = m_wallet->get_subaddress_as_str(subaddr_index).substr(0, 6);
    uint64_t num_unspent_outputs = std::count_if(transfers.begin(), transfers.end(), [&subaddr_index](const tools::wallet2::transfer_details& td) { return !td.m_spent && td.m_subaddr_index == subaddr_index; });
    success_msg_writer() << boost::format(tr("%8u %6s %21s %21s %7u %21s")) % i.first % address_str % print_money(i.second) % print_money(unlocked_balance_per_subaddress[i.first]) % num_unspent_outputs % m_wallet->get_subaddress_label(subaddr_index);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  if (args.size() > 1 || (args.size() == 1 && args[0] != "detail"))
  {
    fail_msg_writer() << tr("usage: balance [detail]");
    return true;
  }
  LOCK_IDLE_SCOPE();
  show_balance_unlocked(args.size() == 1);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_incoming_transfers(const std::vector<std::string>& args)
{
  if (args.size() > 3)
  {
    fail_msg_writer() << tr("usage: incoming_transfers [available|unavailable] [verbose] [index=<N>]");
    return true;
  }
  auto local_args = args;
  LOCK_IDLE_SCOPE();

  bool filter = false;
  bool available = false;
  bool verbose = false;
  if (local_args.size() > 0)
  {
    if (local_args[0] == "available")
    {
      filter = true;
      available = true;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "unavailable")
    {
      filter = true;
      available = false;
      local_args.erase(local_args.begin());
    }
  }
  if (local_args.size() > 0 && local_args[0] == "verbose")
  {
    verbose = true;
    local_args.erase(local_args.begin());
  }

  PAUSE_READLINE();

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  if (local_args.size() > 0)
  {
    fail_msg_writer() << tr("usage: incoming_transfers [available|unavailable] [verbose] [index=<N>]");
    return true;
  }

  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != td.m_spent)
    {
      if (m_current_subaddress_account != td.m_subaddr_index.major || (!subaddr_indices.empty() && subaddr_indices.count(td.m_subaddr_index.minor) == 0))
        continue;
      if (!transfers_found)
      {
        std::string verbose_string;
        if (verbose)
          verbose_string = (boost::format("%68s%68s") % tr("pubkey") % tr("key image")).str();
        message_writer() << boost::format("%21s%8s%12s%8s%16s%68s%16s%s") % tr("amount") % tr("spent") % tr("unlocked") % tr("ringct") % tr("global index") % tr("tx id") % tr("addr index") % verbose_string;
        transfers_found = true;
      }
      std::string verbose_string;
      if (verbose)
        verbose_string = (boost::format("%68s%68s") % td.get_public_key() % (td.m_key_image_known ? epee::string_tools::pod_to_hex(td.m_key_image) : td.m_key_image_partial ? (epee::string_tools::pod_to_hex(td.m_key_image) + "/p") : std::string(64, '?'))).str();
      message_writer(td.m_spent ? console_color_magenta : console_color_green, false) <<
        boost::format("%21s%8s%12s%8s%16u%68s%16u%s") %
        print_money(td.amount()) %
        (td.m_spent ? tr("T") : tr("F")) %
        (m_wallet->is_transfer_unlocked(td) ? tr("unlocked") : tr("locked")) %
        (td.is_rct() ? tr("RingCT") : tr("-")) %
        td.m_global_output_index %
        td.m_txid %
        td.m_subaddr_index.minor %
        verbose_string;
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
    fail_msg_writer() << tr("expected at least one payment ID");
    return true;
  }

  LOCK_IDLE_SCOPE();

  PAUSE_READLINE();

  message_writer() << boost::format("%68s%68s%12s%21s%16s%16s") %
    tr("payment") % tr("transaction") % tr("height") % tr("amount") % tr("unlock time") % tr("addr index");

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
          boost::format("%68s%68s%12s%21s%16s%16s") %
          payment_id %
          pd.m_tx_hash %
          pd.m_block_height %
          print_money(pd.m_amount) %
          pd.m_unlock_time %
          pd.m_subaddr_index.minor;
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
  if (!m_wallet)
  {
    throw std::runtime_error("simple_wallet null wallet");
  }

  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = m_wallet->invoke_http_json("/getheight", req, res);
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
  if (!is_daemon_trusted())
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
bool simple_wallet::print_ring_members(const std::vector<tools::wallet2::pending_tx>& ptx_vector, std::ostream& ostr)
{
  uint32_t version;
  if (!try_connect_to_daemon(false, &version))
  {
    fail_msg_writer() << tr("failed to connect to the daemon");
    return false;
  }
  // available for RPC version 1.4 or higher
  if (version < MAKE_CORE_RPC_VERSION(1, 4))
    return true;
  std::string err;
  uint64_t blockchain_height = get_daemon_blockchain_height(err);
  if (!err.empty())
  {
    fail_msg_writer() << tr("failed to get blockchain height: ") << err;
    return false;
  }
  // for each transaction
  for (size_t n = 0; n < ptx_vector.size(); ++n)
  {
    const cryptonote::transaction& tx = ptx_vector[n].tx;
    const tools::wallet2::tx_construction_data& construction_data = ptx_vector[n].construction_data;
    ostr << boost::format(tr("\nTransaction %llu/%llu: txid=%s")) % (n + 1) % ptx_vector.size() % cryptonote::get_transaction_hash(tx);
    // for each input
    std::vector<uint64_t>     spent_key_height(tx.vin.size());
    std::vector<crypto::hash> spent_key_txid  (tx.vin.size());
    for (size_t i = 0; i < tx.vin.size(); ++i)
    {
      if (tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        continue;
      const cryptonote::txin_to_key& in_key = boost::get<cryptonote::txin_to_key>(tx.vin[i]);
      const tools::wallet2::transfer_details &td = m_wallet->get_transfer_details(construction_data.selected_transfers[i]);
      const cryptonote::tx_source_entry *sptr = NULL;
      for (const auto &src: construction_data.sources)
        if (src.outputs[src.real_output].second.dest == td.get_public_key())
          sptr = &src;
      if (!sptr)
      {
        fail_msg_writer() << tr("failed to find construction data for tx input");
        return false;
      }
      const cryptonote::tx_source_entry& source = *sptr;

      ostr << boost::format(tr("\nInput %llu/%llu: amount=%s")) % (i + 1) % tx.vin.size() % print_money(source.amount);
      // convert relative offsets of ring member keys into absolute offsets (indices) associated with the amount
      std::vector<uint64_t> absolute_offsets = cryptonote::relative_output_offsets_to_absolute(in_key.key_offsets);
      // get block heights from which those ring member keys originated
      COMMAND_RPC_GET_OUTPUTS_BIN::request req = AUTO_VAL_INIT(req);
      req.outputs.resize(absolute_offsets.size());
      for (size_t j = 0; j < absolute_offsets.size(); ++j)
      {
        req.outputs[j].amount = in_key.amount;
        req.outputs[j].index = absolute_offsets[j];
      }
      COMMAND_RPC_GET_OUTPUTS_BIN::response res = AUTO_VAL_INIT(res);
      bool r = m_wallet->invoke_http_bin("/get_outs.bin", req, res);
      err = interpret_rpc_response(r, res.status);
      if (!err.empty())
      {
        fail_msg_writer() << tr("failed to get output: ") << err;
        return false;
      }
      // make sure that returned block heights are less than blockchain height
      for (auto& res_out : res.outs)
      {
        if (res_out.height >= blockchain_height)
        {
          fail_msg_writer() << tr("output key's originating block height shouldn't be higher than the blockchain height");
          return false;
        }
      }
      ostr << tr("\nOriginating block heights: ");
      for (size_t j = 0; j < absolute_offsets.size(); ++j)
        ostr << tr(j == source.real_output ? " *" : " ") << res.outs[j].height;
      spent_key_height[i] = res.outs[source.real_output].height;
      spent_key_txid  [i] = res.outs[source.real_output].txid;
      // visualize the distribution, using the code by moneroexamples onion-monero-viewer
      const uint64_t resolution = 79;
      std::string ring_str(resolution, '_');
      for (size_t j = 0; j < absolute_offsets.size(); ++j)
      {
        uint64_t pos = (res.outs[j].height * resolution) / blockchain_height;
        ring_str[pos] = 'o';
      }
      uint64_t pos = (res.outs[source.real_output].height * resolution) / blockchain_height;
      ring_str[pos] = '*';
      ostr << tr("\n|") << ring_str << tr("|\n");
    }
    // warn if rings contain keys originating from the same tx or temporally very close block heights
    bool are_keys_from_same_tx      = false;
    bool are_keys_from_close_height = false;
    for (size_t i = 0; i < tx.vin.size(); ++i) {
      for (size_t j = i + 1; j < tx.vin.size(); ++j)
      {
        if (spent_key_txid[i] == spent_key_txid[j])
          are_keys_from_same_tx = true;
        if (std::abs((int64_t)(spent_key_height[i] - spent_key_height[j])) < (int64_t)5)
          are_keys_from_close_height = true;
      }
    }
    if (are_keys_from_same_tx || are_keys_from_close_height)
    {
      ostr
        << tr("\nWarning: Some input keys being spent are from ")
        << (are_keys_from_same_tx ? tr("the same transaction") : tr("blocks that are temporally very close"))
        << tr(", which can break the anonymity of ring signature. Make sure this is intentional!");
    }
    ostr << ENDL;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_main(int transfer_type, const std::vector<std::string> &args_)
{
//  "transfer [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> <amount> [<payment_id>]"
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  uint32_t priority = 0;
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());

  priority = m_wallet->adjust_priority(priority);

  size_t fake_outs_count = 0;
  if(local_args.size() > 0) {
    size_t ring_size;
    if(!epee::string_tools::get_xtype_from_string(ring_size, local_args[0]))
    {
      fake_outs_count = m_wallet->default_mixin();
      if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIX;
    }
    else if (ring_size == 0)
    {
      fail_msg_writer() << tr("Ring size must not be 0");
      return true;
    }
    else
    {
      fake_outs_count = ring_size - 1;
      local_args.erase(local_args.begin());
    }
  }
  uint64_t adjusted_fake_outs_count = m_wallet->adjust_mixin(fake_outs_count);
  if (adjusted_fake_outs_count > fake_outs_count)
  {
    fail_msg_writer() << (boost::format(tr("ring size %u is too small, minimum is %u")) % (fake_outs_count+1) % (adjusted_fake_outs_count+1)).str();
    return true;
  }

  const size_t min_args = (transfer_type == TransferLocked) ? 3 : 2;
  if(local_args.size() < min_args)
  {
     fail_msg_writer() << tr("wrong number of arguments");
     return true;
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  bool expect_even = (transfer_type == TransferLocked);
  if ((expect_even ? 0 : 1) == local_args.size() % 2)
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

  uint64_t locked_blocks = 0;
  if (transfer_type == TransferLocked)
  {
    try
    {
      locked_blocks = boost::lexical_cast<uint64_t>(local_args.back());
    }
    catch (const std::exception &e)
    {
      fail_msg_writer() << tr("bad locked_blocks parameter:") << " " << local_args.back();
      return true;
    }
    if (locked_blocks > 1000000)
    {
      fail_msg_writer() << tr("Locked blocks too high, max 1000000 (˜4 yrs)");
      return true;
    }
    local_args.pop_back();
  }

  vector<cryptonote::tx_destination_entry> dsts;
  size_t num_subaddresses = 0;
  for (size_t i = 0; i < local_args.size(); i += 2)
  {
    cryptonote::address_parse_info info;
    cryptonote::tx_destination_entry de;
    if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), local_args[i], oa_prompter))
    {
      fail_msg_writer() << tr("failed to parse address");
      return true;
    }
    de.addr = info.address;
    de.is_subaddress = info.is_subaddress;
    num_subaddresses += info.is_subaddress;

    if (info.has_payment_id)
    {
      if (payment_id_seen)
      {
        fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[i];
        return true;
      }

      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
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
  if (!payment_id_seen && m_wallet->confirm_missing_payment_id() && dsts.size() > num_subaddresses)
  {
     std::string accepted = input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
     if (std::cin.eof())
       return true;
     if (!command_line::is_yes(accepted))
     {
       fail_msg_writer() << tr("transaction cancelled.");

       return true; 
     }
  }

  try
  {
    // figure out what tx will be necessary
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    uint64_t bc_height, unlock_block = 0;
    std::string err;
    switch (transfer_type)
    {
      case TransferLocked:
        bc_height = get_daemon_blockchain_height(err);
        if (!err.empty())
        {
          fail_msg_writer() << tr("failed to get blockchain height: ") << err;
          return true;
        }
        unlock_block = bc_height + locked_blocks;
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, unlock_block /* unlock_time */, priority, extra, m_current_subaddress_account, subaddr_indices, is_daemon_trusted());
      break;
      case TransferNew:
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, m_current_subaddress_account, subaddr_indices, is_daemon_trusted());
      break;
      default:
        LOG_ERROR("Unknown transfer method, using original");
        /* FALLTHRU */
      case TransferOriginal:
        ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, is_daemon_trusted());
        break;
    }

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No outputs found, or daemon is not ready");
      return true;
    }

    // if we need to check for backlog, check the worst case tx
    if (m_wallet->confirm_backlog())
    {
      std::stringstream prompt;
      double worst_fee_per_byte = std::numeric_limits<double>::max();
      for (size_t n = 0; n < ptx_vector.size(); ++n)
      {
        const uint64_t blob_size = cryptonote::tx_to_blob(ptx_vector[n].tx).size();
        const double fee_per_byte = ptx_vector[n].fee / (double)blob_size;
        if (fee_per_byte < worst_fee_per_byte)
        {
          worst_fee_per_byte = fee_per_byte;
        }
      }
      try
      {
        std::vector<std::pair<uint64_t, uint64_t>> nblocks = m_wallet->estimate_backlog({std::make_pair(worst_fee_per_byte, worst_fee_per_byte)});
        if (nblocks.size() != 1)
        {
          prompt << "Internal error checking for backlog. " << tr("Is this okay anyway?  (Y/Yes/N/No): ");
        }
        else
        {
          if (nblocks[0].first > m_wallet->get_confirm_backlog_threshold())
            prompt << (boost::format(tr("There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): ")) % nblocks[0].first).str();
        }
      }
      catch (const std::exception &e)
      {
        prompt << tr("Failed to check for backlog: ") << e.what() << ENDL << tr("Is this okay anyway?  (Y/Yes/N/No): ");
      }

      std::string prompt_str = prompt.str();
      if (!prompt_str.empty())
      {
        std::string accepted = input_line(prompt_str);
        if (std::cin.eof())
          return true;
        if (!command_line::is_yes(accepted))
        {
          fail_msg_writer() << tr("transaction cancelled.");

          return true; 
        }
      }
    }

    // if more than one tx necessary, prompt user to confirm
    if (m_wallet->always_confirm_transfers() || ptx_vector.size() > 1)
    {
        uint64_t total_sent = 0;
        uint64_t total_fee = 0;
        uint64_t dust_not_in_fee = 0;
        uint64_t dust_in_fee = 0;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          total_fee += ptx_vector[n].fee;
          for (auto i: ptx_vector[n].selected_transfers)
            total_sent += m_wallet->get_transfer_details(i).amount();
          total_sent -= ptx_vector[n].change_dts.amount + ptx_vector[n].fee;

          if (ptx_vector[n].dust_added_to_fee)
            dust_in_fee += ptx_vector[n].dust;
          else
            dust_not_in_fee += ptx_vector[n].dust;
        }

        std::stringstream prompt;
        for (size_t n = 0; n < ptx_vector.size(); ++n)
        {
          prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx_vector.size() << ":\n";
          subaddr_indices.clear();
          for (uint32_t i : ptx_vector[n].construction_data.subaddr_indices)
            subaddr_indices.insert(i);
          for (uint32_t i : subaddr_indices)
            prompt << boost::format(tr("Spending from address index %d\n")) % i;
          if (subaddr_indices.size() > 1)
            prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
        }
        prompt << boost::format(tr("Sending %s.  ")) % print_money(total_sent);
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
        if (transfer_type == TransferLocked)
        {
          float days = locked_blocks / 720.0f;
          prompt << boost::format(tr(".\nThis transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)")) % ((unsigned long long)unlock_block) % days;
        }
        if (m_wallet->print_ring_members())
        {
          if (!print_ring_members(ptx_vector, prompt))
            return true;
        }
        bool default_ring_size = true;
        for (const auto &ptx: ptx_vector)
        {
          for (const auto &vin: ptx.tx.vin)
          {
            if (vin.type() == typeid(txin_to_key))
            {
              const txin_to_key& in_to_key = boost::get<txin_to_key>(vin);
              if (in_to_key.key_offsets.size() != DEFAULT_MIX + 1)
                default_ring_size = false;
            }
          }
        }
        if (m_wallet->confirm_non_default_ring_size() && !default_ring_size)
        {
          prompt << tr("WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.");
        }
        prompt << ENDL << tr("Is this okay?  (Y/Yes/N/No): ");
        
        std::string accepted = input_line(prompt.str());
        if (std::cin.eof())
          return true;
        if (!command_line::is_yes(accepted))
        {
          fail_msg_writer() << tr("transaction cancelled.");

          return true; 
        }
    }

    // actually commit the transactions
    if (m_wallet->multisig())
    {
      bool r = m_wallet->save_multisig_tx(ptx_vector, "multisig_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
      }
    }
    else if (m_wallet->watch_only())
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
    else
    {
      commit_or_save(ptx_vector, m_do_not_relay);
    }
  }
  catch (const std::exception &e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
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
  return transfer_main(TransferLocked, args_);
}
//----------------------------------------------------------------------------------------------------

bool simple_wallet::sweep_unmixable(const std::vector<std::string> &args_)
{
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();
  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_unmixable_sweep_transactions(is_daemon_trusted());

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
      for (auto i: ptx_vector[n].selected_transfers)
        total_unmixable += m_wallet->get_transfer_details(i).amount();
    }

    std::string prompt_str = tr("Sweeping ") + print_money(total_unmixable);
    if (ptx_vector.size() > 1) {
      prompt_str = (boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_unmixable) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee)).str();
    }
    else {
      prompt_str = (boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_unmixable) %
        print_money(total_fee)).str();
    }
    std::string accepted = input_line(prompt_str);
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");

      return true;
    }

    // actually commit the transactions
    if (m_wallet->multisig())
    {
      bool r = m_wallet->save_multisig_tx(ptx_vector, "multisig_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
      }
    }
    else if (m_wallet->watch_only())
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
    else
    {
      commit_or_save(ptx_vector, m_do_not_relay);
    }
  }
  catch (const tools::error::not_enough_unlocked_money& e)
  {
    fail_msg_writer() << tr("Not enough money in unlocked balance");
    std::string accepted = input_line((boost::format(tr("Discarding %s of unmixable outputs that cannot be spent, which can be undone by \"rescan_spent\".  Is this okay?  (Y/Yes/N/No): ")) % print_money(e.available())).str());
    if (std::cin.eof())
      return true;
    if (command_line::is_yes(accepted))
    {
      try
      {
        m_wallet->discard_unmixable_outputs(is_daemon_trusted());
      } catch (...) {}
    }
  }
  catch (const std::exception &e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_main(uint64_t below, const std::vector<std::string> &args_)
{
  // sweep_all [index=<N1>[,<N2>,...]] [<ring_size>] <address> [<payment_id>]
  if (args_.size() == 0)
  {
    fail_msg_writer() << tr("No address given");
    return true;
  }

  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  uint32_t priority = 0;
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());

  priority = m_wallet->adjust_priority(priority);

  size_t fake_outs_count = 0;
  if(local_args.size() > 0) {
    size_t ring_size;
    if(!epee::string_tools::get_xtype_from_string(ring_size, local_args[0]))
    {
      fake_outs_count = m_wallet->default_mixin();
      if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIX;
    }
    else if (ring_size == 0)
    {
      fail_msg_writer() << tr("Ring size must not be 0");
      return true;
    }
    else
    {
      fake_outs_count = ring_size - 1;
      local_args.erase(local_args.begin());
    }
  }
  uint64_t adjusted_fake_outs_count = m_wallet->adjust_mixin(fake_outs_count);
  if (adjusted_fake_outs_count > fake_outs_count)
  {
    fail_msg_writer() << (boost::format(tr("ring size %u is too small, minimum is %u")) % (fake_outs_count+1) % (adjusted_fake_outs_count+1)).str();
    return true;
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (2 >= local_args.size())
  {
    std::string payment_id_str = local_args.back();

    crypto::hash payment_id;
    bool r = tools::wallet2::parse_long_payment_id(payment_id_str, payment_id);
    if(r)
    {
      std::string extra_nonce;
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
      r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      payment_id_seen = true;
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
        payment_id_seen = true;
      }
    }

    if(!r && local_args.size() == 3)
    {
      fail_msg_writer() << tr("payment id has invalid format, expected 16 or 64 character hex string: ") << payment_id_str;
      return true;
    }
    if (payment_id_seen)
      local_args.pop_back();
  }

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), local_args[0], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  if (info.has_payment_id)
  {
    if (payment_id_seen)
    {
      fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[0];
      return true;
    }

    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
    if(!r)
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
    payment_id_seen = true;
  }

  // prompt is there is no payment id and confirmation is required
  if (!payment_id_seen && m_wallet->confirm_missing_payment_id() && !info.is_subaddress)
  {
     std::string accepted = input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
     if (std::cin.eof())
       return true;
     if (!command_line::is_yes(accepted))
     {
       fail_msg_writer() << tr("transaction cancelled.");

       return true; 
     }
  }

  LOCK_IDLE_SCOPE();

  try
  {
    // figure out what tx will be necessary
    auto ptx_vector = m_wallet->create_transactions_all(below, info.address, info.is_subaddress, fake_outs_count, 0 /* unlock_time */, priority, extra, m_current_subaddress_account, subaddr_indices, is_daemon_trusted());

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No outputs found, or daemon is not ready");
      return true;
    }

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = 0, total_sent = 0;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      total_fee += ptx_vector[n].fee;
      for (auto i: ptx_vector[n].selected_transfers)
        total_sent += m_wallet->get_transfer_details(i).amount();
    }

    std::ostringstream prompt;
    for (size_t n = 0; n < ptx_vector.size(); ++n)
    {
      prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx_vector.size() << ":\n";
      subaddr_indices.clear();
      for (uint32_t i : ptx_vector[n].construction_data.subaddr_indices)
        subaddr_indices.insert(i);
      for (uint32_t i : subaddr_indices)
        prompt << boost::format(tr("Spending from address index %d\n")) % i;
      if (subaddr_indices.size() > 1)
        prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
    }
    if (m_wallet->print_ring_members() && !print_ring_members(ptx_vector, prompt))
      return true;
    if (ptx_vector.size() > 1) {
      prompt << boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_sent) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee);
    }
    else {
      prompt << boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_sent) %
        print_money(total_fee);
    }
    std::string accepted = input_line(prompt.str());
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");

      return true;
    }

    // actually commit the transactions
    if (m_wallet->multisig())
    {
      bool r = m_wallet->save_multisig_tx(ptx_vector, "multisig_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
      }
    }
    else if (m_wallet->watch_only())
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
    else
    {
      commit_or_save(ptx_vector, m_do_not_relay);
    }
  }
  catch (const std::exception& e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
  }
  catch (...)
  {
    LOG_ERROR("unknown error");
    fail_msg_writer() << tr("unknown error");
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_single(const std::vector<std::string> &args_)
{
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  uint32_t priority = 0;
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());

  priority = m_wallet->adjust_priority(priority);

  size_t fake_outs_count = 0;
  if(local_args.size() > 0) {
    size_t ring_size;
    if(!epee::string_tools::get_xtype_from_string(ring_size, local_args[0]))
    {
      fake_outs_count = m_wallet->default_mixin();
      if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIX;
    }
    else
    {
      fake_outs_count = ring_size - 1;
      local_args.erase(local_args.begin());
    }
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (local_args.size() == 3)
  {
    crypto::hash payment_id;
    crypto::hash8 payment_id8;
    std::string extra_nonce;
    if (tools::wallet2::parse_long_payment_id(local_args.back(), payment_id))
    {
      set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id);
    }
    else if(tools::wallet2::parse_short_payment_id(local_args.back(), payment_id8))
    {
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id8);
    }
    else
    {
      fail_msg_writer() << tr("failed to parse Payment ID");
      return true;
    }

    if (!add_extra_nonce_to_tx_extra(extra, extra_nonce))
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }

    local_args.pop_back();
    payment_id_seen = true;
  }

  if (local_args.size() != 2)
  {
    fail_msg_writer() << tr("usage: sweep_single [<priority>] [<ring_size>] <key_image> <address> [<payment_id>]");
    return true;
  }

  crypto::key_image ki;
  if (!epee::string_tools::hex_to_pod(local_args[0], ki))
  {
    fail_msg_writer() << tr("failed to parse key image");
    return true;
  }

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), local_args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  if (info.has_payment_id)
  {
    if (payment_id_seen)
    {
      fail_msg_writer() << tr("a single transaction cannot use more than one payment id: ") << local_args[0];
      return true;
    }

    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    if (!add_extra_nonce_to_tx_extra(extra, extra_nonce))
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
    payment_id_seen = true;
  }

  // prompt if there is no payment id and confirmation is required
  if (!payment_id_seen && m_wallet->confirm_missing_payment_id() && !info.is_subaddress)
  {
     std::string accepted = input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
     if (std::cin.eof())
       return true;
     if (!command_line::is_yes(accepted))
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
    auto ptx_vector = m_wallet->create_transactions_single(ki, info.address, info.is_subaddress, fake_outs_count, 0 /* unlock_time */, priority, extra, is_daemon_trusted());

    if (ptx_vector.empty())
    {
      fail_msg_writer() << tr("No outputs found");
      return true;
    }
    if (ptx_vector.size() > 1)
    {
      fail_msg_writer() << tr("Multiple transactions are created, which is not supposed to happen");
      return true;
    }
    if (ptx_vector[0].selected_transfers.size() != 1)
    {
      fail_msg_writer() << tr("The transaction uses multiple or no inputs, which is not supposed to happen");
      return true;
    }

    // give user total and fee, and prompt to confirm
    uint64_t total_fee = ptx_vector[0].fee;
    uint64_t total_sent = m_wallet->get_transfer_details(ptx_vector[0].selected_transfers.front()).amount();
    std::ostringstream prompt;
    if (!print_ring_members(ptx_vector, prompt))
      return true;
    prompt << boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
      print_money(total_sent) %
      print_money(total_fee);
    std::string accepted = input_line(prompt.str());
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");
      return true;
    }

    // actually commit the transactions
    if (m_wallet->multisig())
    {
      bool r = m_wallet->save_multisig_tx(ptx_vector, "multisig_monero_tx");
      if (!r)
      {
        fail_msg_writer() << tr("Failed to write transaction(s) to file");
      }
      else
      {
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
      }
    }
    else if (m_wallet->watch_only())
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
    else
    {
      m_wallet->commit_tx(ptx_vector[0]);
      success_msg_writer(true) << tr("Money successfully sent, transaction: ") << get_transaction_hash(ptx_vector[0].tx);
    }

  }
  catch (const std::exception& e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
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
  return sweep_main(0, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_below(const std::vector<std::string> &args_)
{
  uint64_t below = 0;
  if (args_.size() < 1)
  {
    fail_msg_writer() << tr("missing threshold amount");
    return true;
  }
  if (!cryptonote::parse_amount(below, args_[0]))
  {
    fail_msg_writer() << tr("invalid amount threshold");
    return true;
  }
  return sweep_main(below, std::vector<std::string>(++args_.begin(), args_.end()));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::donate(const std::vector<std::string> &args_)
{
  if(m_wallet->nettype() != cryptonote::MAINNET)
  {
    fail_msg_writer() << tr("donations are not enabled on the testnet or on the stagenet");
    return true;
  }

  std::vector<std::string> local_args = args_;
  if(local_args.empty() || local_args.size() > 5)
  {
     fail_msg_writer() << tr("usage: donate [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <amount> [<payment_id>]");
     return true;
  }
  std::string amount_str;
  std::string payment_id_str;
  // get payment id and pop
  crypto::hash payment_id;
  crypto::hash8 payment_id8;
  if (tools::wallet2::parse_long_payment_id (local_args.back(), payment_id ) ||
      tools::wallet2::parse_short_payment_id(local_args.back(), payment_id8))
  {
    payment_id_str = local_args.back();
    local_args.pop_back();
  }
  // get amount and pop
  amount_str = local_args.back();
  local_args.pop_back();
  // push back address, amount, payment id
  local_args.push_back(MONERO_DONATION_ADDR);
  local_args.push_back(amount_str);
  if (!payment_id_str.empty())
    local_args.push_back(payment_id_str);
  message_writer() << tr("Donating ") << amount_str << " to The Monero Project (donate.getmonero.org or "<< MONERO_DONATION_ADDR <<").";
  transfer_new(local_args);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message)
{
  // gather info to ask the user
  uint64_t amount = 0, amount_to_dests = 0, change = 0;
  size_t min_ring_size = ~0;
  std::unordered_map<cryptonote::account_public_address, std::pair<std::string, uint64_t>> dests;
  int first_known_non_zero_change_index = -1;
  std::string payment_id_string = "";
  for (size_t n = 0; n < get_num_txes(); ++n)
  {
    const tools::wallet2::tx_construction_data &cd = get_tx(n);

    std::vector<tx_extra_field> tx_extra_fields;
    bool has_encrypted_payment_id = false;
    crypto::hash8 payment_id8 = crypto::null_hash8;
    if (cryptonote::parse_tx_extra(cd.extra, tx_extra_fields))
    {
      tx_extra_nonce extra_nonce;
      if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
      {
        crypto::hash payment_id;
        if(get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
        {
          if (!payment_id_string.empty())
            payment_id_string += ", ";
          payment_id_string = std::string("encrypted payment ID ") + epee::string_tools::pod_to_hex(payment_id8);
          has_encrypted_payment_id = true;
        }
        else if (get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
          if (!payment_id_string.empty())
            payment_id_string += ", ";
          payment_id_string = std::string("unencrypted payment ID ") + epee::string_tools::pod_to_hex(payment_id);
        }
      }
    }

    for (size_t s = 0; s < cd.sources.size(); ++s)
    {
      amount += cd.sources[s].amount;
      size_t ring_size = cd.sources[s].outputs.size();
      if (ring_size < min_ring_size)
        min_ring_size = ring_size;
    }
    for (size_t d = 0; d < cd.splitted_dsts.size(); ++d)
    {
      const tx_destination_entry &entry = cd.splitted_dsts[d];
      std::string address, standard_address = get_account_address_as_str(m_wallet->nettype(), entry.is_subaddress, entry.addr);
      if (has_encrypted_payment_id && !entry.is_subaddress)
      {
        address = get_account_integrated_address_as_str(m_wallet->nettype(), entry.addr, payment_id8);
        address += std::string(" (" + standard_address + " with encrypted payment id " + epee::string_tools::pod_to_hex(payment_id8) + ")");
      }
      else
        address = standard_address;
      auto i = dests.find(entry.addr);
      if (i == dests.end())
        dests.insert(std::make_pair(entry.addr, std::make_pair(address, entry.amount)));
      else
        i->second.second += entry.amount;
      amount_to_dests += entry.amount;
    }
    if (cd.change_dts.amount > 0)
    {
      auto it = dests.find(cd.change_dts.addr);
      if (it == dests.end())
      {
        fail_msg_writer() << tr("Claimed change does not go to a paid address");
        return false;
      }
      if (it->second.second < cd.change_dts.amount)
      {
        fail_msg_writer() << tr("Claimed change is larger than payment to the change address");
        return false;
      }
      if (cd.change_dts.amount > 0)
      {
        if (first_known_non_zero_change_index == -1)
          first_known_non_zero_change_index = n;
        if (memcmp(&cd.change_dts.addr, &get_tx(first_known_non_zero_change_index).change_dts.addr, sizeof(cd.change_dts.addr)))
        {
          fail_msg_writer() << tr("Change goes to more than one address");
          return false;
        }
      }
      change += cd.change_dts.amount;
      it->second.second -= cd.change_dts.amount;
      if (it->second.second == 0)
        dests.erase(cd.change_dts.addr);
    }
  }

  if (payment_id_string.empty())
    payment_id_string = "no payment ID";

  std::string dest_string;
  size_t n_dummy_outputs = 0;
  for (auto i = dests.begin(); i != dests.end(); )
  {
    if (i->second.second > 0)
    {
      if (!dest_string.empty())
        dest_string += ", ";
      dest_string += (boost::format(tr("sending %s to %s")) % print_money(i->second.second) % i->second.first).str();
    }
    else
      ++n_dummy_outputs;
    ++i;
  }
  if (n_dummy_outputs > 0)
  {
    if (!dest_string.empty())
      dest_string += ", ";
    dest_string += std::to_string(n_dummy_outputs) + tr(" dummy output(s)");
  }
  if (dest_string.empty())
    dest_string = tr("with no destinations");

  std::string change_string;
  if (change > 0)
  {
    std::string address = get_account_address_as_str(m_wallet->nettype(), get_tx(0).subaddr_account > 0, get_tx(0).change_dts.addr);
    change_string += (boost::format(tr("%s change to %s")) % print_money(change) % address).str();
  }
  else
    change_string += tr("no change");

  uint64_t fee = amount - amount_to_dests;
  std::string prompt_str = (boost::format(tr("Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): ")) % (unsigned long)get_num_txes() % print_money(amount) % print_money(fee) % dest_string % change_string % (unsigned long)min_ring_size % payment_id_string % extra_message).str();
  return command_line::is_yes(input_line(prompt_str));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const tools::wallet2::unsigned_tx_set &txs)
{
  std::string extra_message;
  if (!txs.transfers.empty())
    extra_message = (boost::format("%u outputs to import. ") % (unsigned)txs.transfers.size()).str();
  return accept_loaded_tx([&txs](){return txs.txes.size();}, [&txs](size_t n)->const tools::wallet2::tx_construction_data&{return txs.txes[n];}, extra_message);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const tools::wallet2::signed_tx_set &txs)
{
  std::string extra_message;
  if (!txs.key_images.empty())
    extra_message = (boost::format("%u key images to import. ") % (unsigned)txs.key_images.size()).str();
  return accept_loaded_tx([&txs](){return txs.ptx.size();}, [&txs](size_t n)->const tools::wallet2::tx_construction_data&{return txs.ptx[n].construction_data;}, extra_message);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args_)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(m_wallet->multisig())
  {
     fail_msg_writer() << tr("This is a multisig wallet, it can only sign with sign_multisig");
     return true;
  }
  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }
  if (args_.size() > 1 || (args_.size() == 1 && args_[0] != "export"))
  {
    fail_msg_writer() << tr("usage: sign_transfer [export]");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  const bool export_raw = args_.size() == 1;

  std::vector<tools::wallet2::pending_tx> ptx;
  try
  {
    bool r = m_wallet->sign_tx("unsigned_monero_tx", "signed_monero_tx", ptx, [&](const tools::wallet2::unsigned_tx_set &tx){ return accept_loaded_tx(tx); }, export_raw);
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

  std::string txids_as_text;
  for (const auto &t: ptx)
  {
    if (!txids_as_text.empty())
      txids_as_text += (", ");
    txids_as_text += epee::string_tools::pod_to_hex(get_transaction_hash(t.tx));
  }
  success_msg_writer(true) << tr("Transaction successfully signed to file ") << "signed_monero_tx" << ", txid " << txids_as_text;
  if (export_raw)
  {
    std::string rawfiles_as_text;
    for (size_t i = 0; i < ptx.size(); ++i)
    {
      if (i > 0)
        rawfiles_as_text += ", ";
      rawfiles_as_text += "signed_monero_tx_raw" + (ptx.size() == 1 ? "" : ("_" + std::to_string(i)));
    }
    success_msg_writer(true) << tr("Transaction raw hex data exported to ") << rawfiles_as_text;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args_)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!try_connect_to_daemon())
    return true;

  try
  {
    std::vector<tools::wallet2::pending_tx> ptx_vector;
    bool r = m_wallet->load_tx("signed_monero_tx", ptx_vector, [&](const tools::wallet2::signed_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load transaction from file");
      return true;
    }

    commit_or_save(ptx_vector, false);
  }
  catch (const std::exception& e)
  {
    handle_transfer_exception(std::current_exception(), is_daemon_trusted());
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

  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(local_args.size() != 1) {
    fail_msg_writer() << tr("usage: get_tx_key <txid>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  crypto::hash txid;
  if (!epee::string_tools::hex_to_pod(local_args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  LOCK_IDLE_SCOPE();

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  if (m_wallet->get_tx_key(txid, tx_key, additional_tx_keys))
  {
    ostringstream oss;
    oss << epee::string_tools::pod_to_hex(tx_key);
    for (size_t i = 0; i < additional_tx_keys.size(); ++i)
      oss << epee::string_tools::pod_to_hex(additional_tx_keys[i]);
    success_msg_writer() << tr("Tx key: ") << oss.str();
    return true;
  }
  else
  {
    fail_msg_writer() << tr("no tx keys found for this txid");
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_proof(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (args.size() != 2 && args.size() != 3)
  {
    fail_msg_writer() << tr("usage: get_tx_proof <txid> <address> [<message>]");
    return true;
  }

  crypto::hash txid;
  if(!epee::string_tools::hex_to_pod(args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  cryptonote::address_parse_info info;
  if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  try
  {
    std::string sig_str = m_wallet->get_tx_proof(txid, info.address, info.is_subaddress, args.size() == 3 ? args[2] : "");
    const std::string filename = "monero_tx_proof";
    if (epee::file_io_utils::save_string_to_file(filename, sig_str))
      success_msg_writer() << tr("signature file saved to: ") << filename;
    else
      fail_msg_writer() << tr("failed to save signature file");
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("error: ") << e.what();
  }
  return true;
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

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }
  crypto::hash txid;
  if(!epee::string_tools::hex_to_pod(local_args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  crypto::secret_key tx_key;
  std::vector<crypto::secret_key> additional_tx_keys;
  if(!epee::string_tools::hex_to_pod(local_args[1].substr(0, 64), tx_key))
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  local_args[1] = local_args[1].substr(64);
  while (!local_args[1].empty())
  {
    additional_tx_keys.resize(additional_tx_keys.size() + 1);
    if(!epee::string_tools::hex_to_pod(local_args[1].substr(0, 64), additional_tx_keys.back()))
    {
      fail_msg_writer() << tr("failed to parse tx key");
      return true;
    }
    local_args[1] = local_args[1].substr(64);
  }

  cryptonote::address_parse_info info;
  if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), local_args[2], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  try
  {
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    m_wallet->check_tx_key(txid, tx_key, additional_tx_keys, info.address, received, in_pool, confirmations);

    if (received > 0)
    {
      success_msg_writer() << get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address) << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << txid;
      if (in_pool)
      {
        success_msg_writer() << tr("WARNING: this transaction is not yet included in the blockchain!");
      }
      else
      {
        if (confirmations != (uint64_t)-1)
        {
          success_msg_writer() << boost::format(tr("This transaction has %u confirmations")) % confirmations;
        }
        else
        {
          success_msg_writer() << tr("WARNING: failed to determine number of confirmations!");
        }
      }
    }
    else
    {
      fail_msg_writer() << get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address) << " " << tr("received nothing in txid") << " " << txid;
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("error: ") << e.what();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_proof(const std::vector<std::string> &args)
{
  if(args.size() != 3 && args.size() != 4) {
    fail_msg_writer() << tr("usage: check_tx_proof <txid> <address> <signature_file> [<message>]");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  // parse txid
  crypto::hash txid;
  if(!epee::string_tools::hex_to_pod(args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  // parse address
  cryptonote::address_parse_info info;
  if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  // read signature file
  std::string sig_str;
  if (!epee::file_io_utils::load_file_to_string(args[2], sig_str))
  {
    fail_msg_writer() << tr("failed to load signature file");
    return true;
  }

  try
  {
    uint64_t received;
    bool in_pool;
    uint64_t confirmations;
    if (m_wallet->check_tx_proof(txid, info.address, info.is_subaddress, args.size() == 4 ? args[3] : "", sig_str, received, in_pool, confirmations))
    {
      success_msg_writer() << tr("Good signature");
      if (received > 0)
      {
        success_msg_writer() << get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address) << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << txid;
        if (in_pool)
        {
          success_msg_writer() << tr("WARNING: this transaction is not yet included in the blockchain!");
        }
        else
        {
          if (confirmations != (uint64_t)-1)
          {
            success_msg_writer() << boost::format(tr("This transaction has %u confirmations")) % confirmations;
          }
          else
          {
            success_msg_writer() << tr("WARNING: failed to determine number of confirmations!");
          }
        }
      }
      else
      {
        fail_msg_writer() << get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address) << " " << tr("received nothing in txid") << " " << txid;
      }
    }
    else
    {
      fail_msg_writer() << tr("Bad signature");
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("error: ") << e.what();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_spend_proof(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(args.size() != 1 && args.size() != 2) {
    fail_msg_writer() << tr("usage: get_spend_proof <txid> [<message>]");
    return true;
  }

  if (m_wallet->watch_only())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot generate the proof");
    return true;
  }

  crypto::hash txid;
  if (!epee::string_tools::hex_to_pod(args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  if (!try_connect_to_daemon())
  {
    fail_msg_writer() << tr("failed to connect to the daemon");
    return true;
  }

  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  try
  {
    const std::string sig_str = m_wallet->get_spend_proof(txid, args.size() == 2 ? args[1] : "");
    const std::string filename = "monero_spend_proof";
    if (epee::file_io_utils::save_string_to_file(filename, sig_str))
      success_msg_writer() << tr("signature file saved to: ") << filename;
    else
      fail_msg_writer() << tr("failed to save signature file");
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << e.what();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_spend_proof(const std::vector<std::string> &args)
{
  if(args.size() != 2 && args.size() != 3) {
    fail_msg_writer() << tr("usage: check_spend_proof <txid> <signature_file> [<message>]");
    return true;
  }

  crypto::hash txid;
  if (!epee::string_tools::hex_to_pod(args[0], txid))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }

  if (!try_connect_to_daemon())
  {
    fail_msg_writer() << tr("failed to connect to the daemon");
    return true;
  }

  std::string sig_str;
  if (!epee::file_io_utils::load_file_to_string(args[1], sig_str))
  {
    fail_msg_writer() << tr("failed to load signature file");
    return true;
  }

  try
  {
    if (m_wallet->check_spend_proof(txid, args.size() == 3 ? args[2] : "", sig_str))
      success_msg_writer() << tr("Good signature");
    else
      fail_msg_writer() << tr("Bad signature");
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_reserve_proof(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(args.size() != 1 && args.size() != 2) {
    fail_msg_writer() << tr("usage: get_reserve_proof (all|<amount>) [<message>]");
    return true;
  }

  if (m_wallet->watch_only() || m_wallet->multisig())
  {
    fail_msg_writer() << tr("The reserve proof can be generated only by a full wallet");
    return true;
  }

  boost::optional<std::pair<uint32_t, uint64_t>> account_minreserve;
  if (args[0] != "all")
  {
    account_minreserve = std::pair<uint32_t, uint64_t>();
    account_minreserve->first = m_current_subaddress_account;
    if (!cryptonote::parse_amount(account_minreserve->second, args[0]))
    {
      fail_msg_writer() << tr("amount is wrong: ") << args[0];
      return true;
    }
  }

  if (!try_connect_to_daemon())
  {
    fail_msg_writer() << tr("failed to connect to the daemon");
    return true;
  }

  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  LOCK_IDLE_SCOPE();

  try
  {
    const std::string sig_str = m_wallet->get_reserve_proof(account_minreserve, args.size() == 2 ? args[1] : "");
    const std::string filename = "monero_reserve_proof";
    if (epee::file_io_utils::save_string_to_file(filename, sig_str))
      success_msg_writer() << tr("signature file saved to: ") << filename;
    else
      fail_msg_writer() << tr("failed to save signature file");
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << e.what();
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_reserve_proof(const std::vector<std::string> &args)
{
  if(args.size() != 2 && args.size() != 3) {
    fail_msg_writer() << tr("usage: check_reserve_proof <address> <signature_file> [<message>]");
    return true;
  }

  if (!try_connect_to_daemon())
  {
    fail_msg_writer() << tr("failed to connect to the daemon");
    return true;
  }

  cryptonote::address_parse_info info;
  if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), args[0], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }
  if (info.is_subaddress)
  {
    fail_msg_writer() << tr("Address must not be a subaddress");
    return true;
  }

  std::string sig_str;
  if (!epee::file_io_utils::load_file_to_string(args[1], sig_str))
  {
    fail_msg_writer() << tr("failed to load signature file");
    return true;
  }

  LOCK_IDLE_SCOPE();

  try
  {
    uint64_t total, spent;
    if (m_wallet->check_reserve_proof(info.address, args.size() == 3 ? args[2] : "", sig_str, total, spent))
    {
      success_msg_writer() << boost::format(tr("Good signature -- total: %s, spent: %s, unspent: %s")) % print_money(total) % print_money(spent) % print_money(total - spent);
    }
    else
    {
      fail_msg_writer() << tr("Bad signature");
    }
  }
  catch (const std::exception& e)
  {
    fail_msg_writer() << e.what();
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
static std::string get_human_readable_timespan(std::chrono::seconds seconds)
{
  uint64_t ts = seconds.count();
  if (ts < 60)
    return std::to_string(ts) + tr(" seconds");
  if (ts < 3600)
    return std::to_string((uint64_t)(ts / 60)) + tr(" minutes");
  if (ts < 3600 * 24)
    return std::to_string((uint64_t)(ts / 3600)) + tr(" hours");
  if (ts < 3600 * 24 * 30.5)
    return std::to_string((uint64_t)(ts / (3600 * 24))) + tr(" days");
  if (ts < 3600 * 24 * 365.25)
    return std::to_string((uint64_t)(ts / (3600 * 24 * 365.25))) + tr(" months");
  return tr("a long time");
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
  boost::optional<uint32_t> subaddr_index;

  if(local_args.size() > 4) {
    fail_msg_writer() << tr("usage: show_transfers [in|out|all|pending|failed] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]");
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

  // subaddr_index
  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  // min height
  if (local_args.size() > 0) {
    try {
      min_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (const boost::bad_lexical_cast &) {
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
    catch (const boost::bad_lexical_cast &) {
      fail_msg_writer() << tr("bad max_height parameter:") << " " << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
  }

  std::multimap<uint64_t, std::pair<bool,std::string>> output;

  PAUSE_READLINE();

  if (in) {
    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
    m_wallet->get_payments(payments, min_height, max_height, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::payment_details &pd = i->second;
      std::string payment_id = string_tools::pod_to_hex(i->first);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(true, (boost::format("%16.16s %20.20s %s %s %d %s %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % pd.m_subaddr_index.minor % "-" % note).str())));
    }
  }

  auto print_subaddr_indices = [](const std::set<uint32_t>& indices)
  {
    stringstream ss;
    bool first = true;
    for (uint32_t i : indices)
    {
      ss << (first ? "" : ",") << i;
      first = false;
    }
    return ss.str();
  };

  if (out) {
    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments;
    m_wallet->get_payments_out(payments, min_height, max_height, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      const tools::wallet2::confirmed_transfer_details &pd = i->second;
      uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
      uint64_t fee = pd.m_amount_in - pd.m_amount_out;
      std::string dests;
      for (const auto &d: pd.m_dests) {
        if (!dests.empty())
          dests += ", ";
        dests +=  get_account_address_as_str(m_wallet->nettype(), d.is_subaddress, d.addr) + ": " + print_money(d.amount);
      }
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(i->first);
      output.insert(std::make_pair(pd.m_block_height, std::make_pair(false, (boost::format("%16.16s %20.20s %s %s %14.14s %s %s - %s") % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount_in - change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % dests % print_subaddr_indices(pd.m_subaddr_indices) % note).str())));
    }
  }

  // print in and out sorted by height
  for (std::map<uint64_t, std::pair<bool, std::string>>::const_iterator i = output.begin(); i != output.end(); ++i) {
    message_writer(i->second.first ? console_color_green : console_color_magenta, false) <<
      boost::format("%8.8llu %6.6s %s") %
      ((unsigned long long)i->first) % (i->second.first ? tr("in") : tr("out")) % i->second.second;
  }

  if (pool) {
    try
    {
      m_wallet->update_pool_state();
      std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> payments;
      m_wallet->get_unconfirmed_payments(payments, m_current_subaddress_account, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        const tools::wallet2::payment_details &pd = i->second.m_pd;
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
          payment_id = payment_id.substr(0,16);
        std::string note = m_wallet->get_tx_note(pd.m_tx_hash);
        std::string double_spend_note;
        if (i->second.m_double_spend_seen)
          double_spend_note = tr("[Double spend seen on the network: this transaction may or may not end up being mined] ");
        message_writer() << (boost::format("%8.8s %6.6s %16.16s %20.20s %s %s %d %s %s%s") % "pool" % "in" % get_human_readable_timestamp(pd.m_timestamp) % print_money(pd.m_amount) % string_tools::pod_to_hex(pd.m_tx_hash) % payment_id % pd.m_subaddr_index.minor % "-" % note % double_spend_note).str();
      }
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << "Failed to get pool state:" << e.what();
    }
  }

  // print unconfirmed last
  if (pending || failed) {
    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments, m_current_subaddress_account, subaddr_indices);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
      uint64_t amount = pd.m_amount_in;
      uint64_t fee = amount - pd.m_amount_out;
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string note = m_wallet->get_tx_note(i->first);
      bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;
      if ((failed && is_failed) || (!is_failed && pending)) {
        message_writer() << (boost::format("%8.8s %6.6s %16.16s %20.20s %s %s %14.14s %s - %s") % (is_failed ? tr("failed") : tr("pending")) % tr("out") % get_human_readable_timestamp(pd.m_timestamp) % print_money(amount - pd.m_change - fee) % string_tools::pod_to_hex(i->first) % payment_id % print_money(fee) % print_subaddr_indices(pd.m_subaddr_indices) % note).str();
      }
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::unspent_outputs(const std::vector<std::string> &args_)
{
  if(args_.size() > 3)
  {
    fail_msg_writer() << tr("usage: unspent_outputs [index=<N1>[,<N2>,...]] [<min_amount> [<max_amount>]]");
    return true;
  }
  auto local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) != "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return true;
    local_args.erase(local_args.begin());
  }

  uint64_t min_amount = 0;
  uint64_t max_amount = std::numeric_limits<uint64_t>::max();
  if (local_args.size() > 0)
  {
    if (!cryptonote::parse_amount(min_amount, local_args[0]))
    {
      fail_msg_writer() << tr("amount is wrong: ") << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
    if (local_args.size() > 0)
    {
      if (!cryptonote::parse_amount(max_amount, local_args[0]))
      {
        fail_msg_writer() << tr("amount is wrong: ") << local_args[0];
        return true;
      }
      local_args.erase(local_args.begin());
    }
    if (min_amount > max_amount)
    {
      fail_msg_writer() << tr("<min_amount> should be smaller than <max_amount>");
      return true;
    }
  }
  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);
  std::map<uint64_t, tools::wallet2::transfer_container> amount_to_tds;
  uint64_t min_height = std::numeric_limits<uint64_t>::max();
  uint64_t max_height = 0;
  uint64_t found_min_amount = std::numeric_limits<uint64_t>::max();
  uint64_t found_max_amount = 0;
  uint64_t count = 0;
  for (const auto& td : transfers)
  {
    uint64_t amount = td.amount();
    if (td.m_spent || amount < min_amount || amount > max_amount || td.m_subaddr_index.major != m_current_subaddress_account || (subaddr_indices.count(td.m_subaddr_index.minor) == 0 && !subaddr_indices.empty()))
      continue;
    amount_to_tds[amount].push_back(td);
    if (min_height > td.m_block_height) min_height = td.m_block_height;
    if (max_height < td.m_block_height) max_height = td.m_block_height;
    if (found_min_amount > amount) found_min_amount = amount;
    if (found_max_amount < amount) found_max_amount = amount;
    ++count;
  }
  if (amount_to_tds.empty())
  {
    success_msg_writer() << tr("There is no unspent output in the specified address");
    return true;
  }
  for (const auto& amount_tds : amount_to_tds)
  {
    auto& tds = amount_tds.second;
    success_msg_writer() << tr("\nAmount: ") << print_money(amount_tds.first) << tr(", number of keys: ") << tds.size();
    for (size_t i = 0; i < tds.size(); )
    {
      std::ostringstream oss;
      for (size_t j = 0; j < 8 && i < tds.size(); ++i, ++j)
        oss << tds[i].m_block_height << tr(" ");
      success_msg_writer() << oss.str();
    }
  }
  success_msg_writer()
    << tr("\nMin block height: ") << min_height
    << tr("\nMax block height: ") << max_height
    << tr("\nMin amount found: ") << print_money(found_min_amount)
    << tr("\nMax amount found: ") << print_money(found_max_amount)
    << tr("\nTotal count: ") << count;
  const size_t histogram_height = 10;
  const size_t histogram_width  = 50;
  double bin_size = (max_height - min_height + 1.0) / histogram_width;
  size_t max_bin_count = 0;
  std::vector<size_t> histogram(histogram_width, 0);
  for (const auto& amount_tds : amount_to_tds)
  {
    for (auto& td : amount_tds.second)
    {
      uint64_t bin_index = (td.m_block_height - min_height + 1) / bin_size;
      if (bin_index >= histogram_width)
        bin_index = histogram_width - 1;
      histogram[bin_index]++;
      if (max_bin_count < histogram[bin_index])
        max_bin_count = histogram[bin_index];
    }
  }
  for (size_t x = 0; x < histogram_width; ++x)
  {
    double bin_count = histogram[x];
    if (max_bin_count > histogram_height)
      bin_count *= histogram_height / (double)max_bin_count;
    if (histogram[x] > 0 && bin_count < 1.0)
      bin_count = 1.0;
    histogram[x] = bin_count;
  }
  std::vector<std::string> histogram_line(histogram_height, std::string(histogram_width, ' '));
  for (size_t y = 0; y < histogram_height; ++y)
  {
    for (size_t x = 0; x < histogram_width; ++x)
    {
      if (y < histogram[x])
        histogram_line[y][x] = '*';
    }
  }
  double count_per_star = max_bin_count / (double)histogram_height;
  if (count_per_star < 1)
    count_per_star = 1;
  success_msg_writer()
    << tr("\nBin size: ") << bin_size
    << tr("\nOutputs per *: ") << count_per_star;
  ostringstream histogram_str;
  histogram_str << tr("count\n  ^\n");
  for (size_t y = histogram_height; y > 0; --y)
    histogram_str << tr("  |") << histogram_line[y - 1] << tr("|\n");
  histogram_str
    << tr("  +") << std::string(histogram_width, '-') << tr("+--> block height\n")
    << tr("   ^") << std::string(histogram_width - 2, ' ') << tr("^\n")
    << tr("  ") << min_height << std::string(histogram_width - 8, ' ') << max_height;
  success_msg_writer() << histogram_str.str();
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
std::string simple_wallet::get_prompt() const
{
  std::string addr_start = m_wallet->get_subaddress_as_str({m_current_subaddress_account, 0}).substr(0, 6);
  std::string prompt = std::string("[") + tr("wallet") + " " + addr_start;
  if (!m_wallet->check_connection(NULL))
    prompt += tr(" (no daemon)");
  else if (!m_wallet->is_synced())
    prompt += tr(" (out of sync)");
  prompt += "]: ";
  return prompt;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  // check and display warning, but go on anyway
  try_connect_to_daemon();

  refresh_main(0, false, true);

  m_auto_refresh_enabled = m_wallet->auto_refresh();
  m_idle_thread = boost::thread([&]{wallet_idle_thread();});

  message_writer(console_color_green, false) << "Background refresh thread started";
  return m_cmd_binder.run_handling([this](){return get_prompt();}, "");
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
bool simple_wallet::account(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // Usage:
  //   account
  //   account new <label text with white spaces allowed>
  //   account switch <index>
  //   account label <index> <label text with white spaces allowed>
  //   account tag <tag_name> <account_index_1> [<account_index_2> ...]
  //   account untag <account_index_1> [<account_index_2> ...]
  //   account tag_description <tag_name> <description>

  if (args.empty())
  {
    // print all the existing accounts
    LOCK_IDLE_SCOPE();
    print_accounts();
    return true;
  }

  std::vector<std::string> local_args = args;
  std::string command = local_args[0];
  local_args.erase(local_args.begin());
  if (command == "new")
  {
    // create a new account and switch to it
    std::string label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled account)");
    m_wallet->add_subaddress_account(label);
    m_current_subaddress_account = m_wallet->get_num_subaddress_accounts() - 1;
    // update_prompt();
    LOCK_IDLE_SCOPE();
    print_accounts();
  }
  else if (command == "switch" && local_args.size() == 1)
  {
    // switch to the specified account
    uint32_t index_major;
    if (!epee::string_tools::get_xtype_from_string(index_major, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
    if (index_major >= m_wallet->get_num_subaddress_accounts())
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet->get_num_subaddress_accounts() - 1);
      return true;
    }
    m_current_subaddress_account = index_major;
    // update_prompt();
    show_balance();
  }
  else if (command == "label" && local_args.size() >= 1)
  {
    // set label of the specified account
    uint32_t index_major;
    if (!epee::string_tools::get_xtype_from_string(index_major, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    try
    {
      m_wallet->set_subaddress_label({index_major, 0}, label);
      LOCK_IDLE_SCOPE();
      print_accounts();
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << e.what();
    }
  }
  else if (command == "tag" && local_args.size() >= 2)
  {
    const std::string tag = local_args[0];
    std::set<uint32_t> account_indices;
    for (size_t i = 1; i < local_args.size(); ++i)
    {
      uint32_t account_index;
      if (!epee::string_tools::get_xtype_from_string(account_index, local_args[i]))
      {
        fail_msg_writer() << tr("failed to parse index: ") << local_args[i];
        return true;
      }
      account_indices.insert(account_index);
    }
    try
    {
      m_wallet->set_account_tag(account_indices, tag);
      print_accounts(tag);
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << e.what();
    }
  }
  else if (command == "untag" && local_args.size() >= 1)
  {
    std::set<uint32_t> account_indices;
    for (size_t i = 0; i < local_args.size(); ++i)
    {
      uint32_t account_index;
      if (!epee::string_tools::get_xtype_from_string(account_index, local_args[i]))
      {
        fail_msg_writer() << tr("failed to parse index: ") << local_args[i];
        return true;
      }
      account_indices.insert(account_index);
    }
    try
    {
      m_wallet->set_account_tag(account_indices, "");
      print_accounts();
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << e.what();
    }
  }
  else if (command == "tag_description" && local_args.size() >= 1)
  {
    const std::string tag = local_args[0];
    std::string description;
    if (local_args.size() > 1)
    {
      local_args.erase(local_args.begin());
      description = boost::join(local_args, " ");
    }
    try
    {
      m_wallet->set_account_tag_description(tag, description);
      print_accounts(tag);
    }
    catch (const std::exception& e)
    {
      fail_msg_writer() << e.what();
    }
  }
  else
  {
    fail_msg_writer() << tr("usage:\n"
                            "  account\n"
                            "  account new <label text with white spaces allowed>\n"
                            "  account switch <index>\n"
                            "  account label <index> <label text with white spaces allowed>\n"
                            "  account tag <tag_name> <account_index_1> [<account_index_2> ...]\n"
                            "  account untag <account_index_1> [<account_index_2> ...]\n"
                            "  account tag_description <tag_name> <description>");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::print_accounts()
{
  const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& account_tags = m_wallet->get_account_tags();
  size_t num_untagged_accounts = m_wallet->get_num_subaddress_accounts();
  for (const std::pair<std::string, std::string>& p : account_tags.first)
  {
    const std::string& tag = p.first;
    print_accounts(tag);
    num_untagged_accounts -= std::count(account_tags.second.begin(), account_tags.second.end(), tag);
    success_msg_writer() << "";
  }

  if (num_untagged_accounts > 0)
    print_accounts("");

  if (num_untagged_accounts < m_wallet->get_num_subaddress_accounts())
    success_msg_writer() << tr("\nGrand total:\n  Balance: ") << print_money(m_wallet->balance_all()) << tr(", unlocked balance: ") << print_money(m_wallet->unlocked_balance_all());
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::print_accounts(const std::string& tag)
{
  const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& account_tags = m_wallet->get_account_tags();
  if (tag.empty())
  {
    success_msg_writer() << tr("Untagged accounts:");
  }
  else
  {
    if (account_tags.first.count(tag) == 0)
    {
      fail_msg_writer() << boost::format(tr("Tag %s is unregistered.")) % tag;
      return;
    }
    success_msg_writer() << tr("Accounts with tag: ") << tag;
    success_msg_writer() << tr("Tag's description: ") << account_tags.first.find(tag)->second;
  }
  success_msg_writer() << boost::format("  %15s %21s %21s %21s") % tr("Account") % tr("Balance") % tr("Unlocked balance") % tr("Label");
  uint64_t total_balance = 0, total_unlocked_balance = 0;
  for (uint32_t account_index = 0; account_index < m_wallet->get_num_subaddress_accounts(); ++account_index)
  {
    if (account_tags.second[account_index] != tag)
      continue;
    success_msg_writer() << boost::format(tr(" %c%8u %6s %21s %21s %21s"))
      % (m_current_subaddress_account == account_index ? '*' : ' ')
      % account_index
      % m_wallet->get_subaddress_as_str({account_index, 0}).substr(0, 6)
      % print_money(m_wallet->balance(account_index))
      % print_money(m_wallet->unlocked_balance(account_index))
      % m_wallet->get_subaddress_label({account_index, 0});
    total_balance += m_wallet->balance(account_index);
    total_unlocked_balance += m_wallet->unlocked_balance(account_index);
  }
  success_msg_writer() << tr("----------------------------------------------------------------------------------");
  success_msg_writer() << boost::format(tr("%15s %21s %21s")) % "Total" % print_money(total_balance) % print_money(total_unlocked_balance);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  // Usage:
  //  address
  //  address new <label text with white spaces allowed>
  //  address all
  //  address <index_min> [<index_max>]
  //  address label <index> <label text with white spaces allowed>

  std::vector<std::string> local_args = args;
  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  auto print_address_sub = [this, &transfers](uint32_t index)
  {
    bool used = std::find_if(
      transfers.begin(), transfers.end(),
      [this, &index](const tools::wallet2::transfer_details& td) {
        return td.m_subaddr_index == cryptonote::subaddress_index{ m_current_subaddress_account, index };
      }) != transfers.end();
    success_msg_writer() << index << "  " << m_wallet->get_subaddress_as_str({m_current_subaddress_account, index}) << "  " << (index == 0 ? tr("Primary address") : m_wallet->get_subaddress_label({m_current_subaddress_account, index})) << " " << (used ? tr("(used)") : "");
  };

  uint32_t index = 0;
  if (local_args.empty())
  {
    print_address_sub(index);
  }
  else if (local_args.size() == 1 && local_args[0] == "all")
  {
    local_args.erase(local_args.begin());
    for (; index < m_wallet->get_num_subaddresses(m_current_subaddress_account); ++index)
      print_address_sub(index);
  }
  else if (local_args[0] == "new")
  {
    local_args.erase(local_args.begin());
    std::string label;
    if (local_args.size() > 0)
      label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled address)");
    m_wallet->add_subaddress(m_current_subaddress_account, label);
    print_address_sub(m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1);
  }
  else if (local_args.size() >= 2 && local_args[0] == "label")
  {
    if (!epee::string_tools::get_xtype_from_string(index, local_args[1]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[1];
      return true;
    }
    if (index >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1);
      return true;
    }
    local_args.erase(local_args.begin());
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    m_wallet->set_subaddress_label({m_current_subaddress_account, index}, label);
    print_address_sub(index);
  }
  else if (local_args.size() <= 2 && epee::string_tools::get_xtype_from_string(index, local_args[0]))
  {
    local_args.erase(local_args.begin());
    uint32_t index_min = index;
    uint32_t index_max = index_min;
    if (local_args.size() > 0)
    {
      if (!epee::string_tools::get_xtype_from_string(index_max, local_args[0]))
      {
        fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
        return true;
      }
      local_args.erase(local_args.begin());
    }
    if (index_max < index_min)
      std::swap(index_min, index_max);
    if (index_min >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("<index_min> is already out of bound");
      return true;
    }
    if (index_max >= m_wallet->get_num_subaddresses(m_current_subaddress_account))
    {
      message_writer() << tr("<index_max> exceeds the bound");
      index_max = m_wallet->get_num_subaddresses(m_current_subaddress_account) - 1;
    }
    for (index = index_min; index <= index_max; ++index)
      print_address_sub(index);
  }
  else
  {
    fail_msg_writer() << tr("usage: address [ new <label text with white spaces allowed> | all | <index_min> [<index_max>] | label <index> <label text with white spaces allowed> ]");
  }

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
    if (m_current_subaddress_account != 0)
    {
      fail_msg_writer() << tr("Integrated addresses can only be created for account 0");
      return true;
    }
    payment_id = crypto::rand<crypto::hash8>();
    success_msg_writer() << tr("Random payment ID: ") << payment_id;
    success_msg_writer() << tr("Matching integrated address: ") << m_wallet->get_account().get_public_integrated_address_str(payment_id, m_wallet->nettype());
    return true;
  }
  if(tools::wallet2::parse_short_payment_id(args.back(), payment_id))
  {
    if (m_current_subaddress_account != 0)
    {
      fail_msg_writer() << tr("Integrated addresses can only be created for account 0");
      return true;
    }
    success_msg_writer() << m_wallet->get_account().get_public_integrated_address_str(payment_id, m_wallet->nettype());
    return true;
  }
  else {
    address_parse_info info;
    if(get_account_address_from_str(info, m_wallet->nettype(), args.back()))
    {
      if (info.has_payment_id)
      {
        success_msg_writer() << boost::format(tr("Integrated address: %s, payment ID: %s")) %
          get_account_address_as_str(m_wallet->nettype(), false, info.address) % epee::string_tools::pod_to_hex(info.payment_id);
      }
      else
      {
        success_msg_writer() << (info.is_subaddress ? tr("Subaddress: ") : tr("Standard address: ")) << get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address);
      }
      return true;
    }
  }
  fail_msg_writer() << tr("failed to parse payment ID or address");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::address_book(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (args.size() == 0)
  {
  }
  else if (args.size() == 1 || (args[0] != "add" && args[0] != "delete"))
  {
    fail_msg_writer() << tr("usage: address_book [(add (<address> [pid <long or short payment id>])|<integrated address> [<description possibly with whitespaces>])|(delete <index>)]");
    return true;
  }
  else if (args[0] == "add")
  {
    cryptonote::address_parse_info info;
    if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), args[1], oa_prompter))
    {
      fail_msg_writer() << tr("failed to parse address");
      return true;
    }
    crypto::hash payment_id = crypto::null_hash;
    size_t description_start = 2;
    if (info.has_payment_id)
    {
      memcpy(payment_id.data, info.payment_id.data, 8);
    }
    else if (!info.has_payment_id && args.size() >= 4 && args[2] == "pid")
    {
      if (tools::wallet2::parse_long_payment_id(args[3], payment_id))
      {
        description_start += 2;
      }
      else if (tools::wallet2::parse_short_payment_id(args[3], info.payment_id))
      {
        memcpy(payment_id.data, info.payment_id.data, 8);
        description_start += 2;
      }
      else
      {
        fail_msg_writer() << tr("failed to parse payment ID");
        return true;
      }
    }
    std::string description;
    for (size_t i = description_start; i < args.size(); ++i)
    {
      if (i > description_start)
        description += " ";
      description += args[i];
    }
    m_wallet->add_address_book_row(info.address, payment_id, description, info.is_subaddress);
  }
  else
  {
    size_t row_id;
    if(!epee::string_tools::get_xtype_from_string(row_id, args[1]))
    {
      fail_msg_writer() << tr("failed to parse index");
      return true;
    }
    m_wallet->delete_address_book_row(row_id);
  }
  auto address_book = m_wallet->get_address_book();
  if (address_book.empty())
  {
    success_msg_writer() << tr("Address book is empty.");
  }
  else
  {
    for (size_t i = 0; i < address_book.size(); ++i) {
      auto& row = address_book[i];
      success_msg_writer() << tr("Index: ") << i;
      success_msg_writer() << tr("Address: ") << get_account_address_as_str(m_wallet->nettype(), row.m_is_subaddress, row.m_address);
      success_msg_writer() << tr("Payment ID: ") << row.m_payment_id;
      success_msg_writer() << tr("Description: ") << row.m_description << "\n";
    }
  }
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
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
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
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
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
bool simple_wallet::set_description(const std::vector<std::string> &args)
{
  // 0 arguments allowed, for setting the description to empty string

  std::string description = "";
  for (size_t n = 0; n < args.size(); ++n)
  {
    if (n > 0)
      description += " ";
    description += args[n];
  }
  m_wallet->set_description(description);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_description(const std::vector<std::string> &args)
{
  if (args.size() != 0)
  {
    fail_msg_writer() << tr("usage: get_description");
    return true;
  }

  std::string description = m_wallet->get_description();
  if (description.empty())
    success_msg_writer() << tr("no description found");
  else
    success_msg_writer() << tr("description found: ") << description;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::status(const std::vector<std::string> &args)
{
  uint64_t local_height = m_wallet->get_blockchain_current_height();
  uint32_t version = 0;
  if (!m_wallet->check_connection(&version))
  {
    success_msg_writer() << "Refreshed " << local_height << "/?, no daemon connected";
    return true;
  }

  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (err.empty())
  {
    bool synced = local_height == bc_height;
    success_msg_writer() << "Refreshed " << local_height << "/" << bc_height << ", " << (synced ? "synced" : "syncing")
        << ", daemon RPC v" << get_version_string(version);
  }
  else
  {
    fail_msg_writer() << "Refreshed " << local_height << "/?, daemon connection error";
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::wallet_info(const std::vector<std::string> &args)
{
  bool ready;
  uint32_t threshold, total;
  std::string description = m_wallet->get_description();
  if (description.empty())
  {
    description = "<Not set>"; 
  }
  message_writer() << tr("Filename: ") << m_wallet->get_wallet_file();
  message_writer() << tr("Description: ") << description;
  message_writer() << tr("Address: ") << m_wallet->get_account().get_public_address_str(m_wallet->nettype());
  std::string type;
  if (m_wallet->watch_only())
    type = tr("Watch only");
  else if (m_wallet->multisig(&ready, &threshold, &total))
    type = (boost::format(tr("%u/%u multisig%s")) % threshold % total % (ready ? "" : " (not yet finalized)")).str();
  else
    type = tr("Normal");
  message_writer() << tr("Type: ") << type;
  message_writer() << tr("Network type: ") << (
    m_wallet->nettype() == cryptonote::TESTNET ? tr("Testnet") :
    m_wallet->nettype() == cryptonote::STAGENET ? tr("Stagenet") : tr("Mainnet"));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
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
  if (m_wallet->multisig())
  {
    fail_msg_writer() << tr("This wallet is multisig and cannot sign");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
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

  cryptonote::address_parse_info info;
  if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), address_string, oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  r = m_wallet->verify(data, info.address, signature);
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
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  std::string filename = args[0];
  if (m_wallet->confirm_export_overwrite() && !check_file_overwrite(filename))
    return true;

  try
  {
    LOCK_IDLE_SCOPE();
    if (!m_wallet->export_key_images(filename))
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (const std::exception &e)
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
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!is_daemon_trusted())
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: import_key_images <filename>");
    return true;
  }
  std::string filename = args[0];

  LOCK_IDLE_SCOPE();
  try
  {
    uint64_t spent = 0, unspent = 0;
    uint64_t height = m_wallet->import_key_images(filename, spent, unspent);
    if (height > 0)
    {
      success_msg_writer() << "Signed key images imported to height " << height << ", "
          << print_money(spent) << " spent, " << print_money(unspent) << " unspent"; 
    } else {
      fail_msg_writer() << "Failed to import key images";
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import key images: " << e.what();
    return true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_outputs(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_outputs <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  std::string filename = args[0];
  if (m_wallet->confirm_export_overwrite() && !check_file_overwrite(filename))
    return true;

  LOCK_IDLE_SCOPE();
  try
  {
    std::vector<tools::wallet2::transfer_details> outs = m_wallet->export_outputs();

    std::stringstream oss;
    boost::archive::portable_binary_oarchive ar(oss);
    ar << outs;

    std::string magic(OUTPUT_EXPORT_FILE_MAGIC, strlen(OUTPUT_EXPORT_FILE_MAGIC));
    const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
    std::string header;
    header += std::string((const char *)&keys.m_spend_public_key, sizeof(crypto::public_key));
    header += std::string((const char *)&keys.m_view_public_key, sizeof(crypto::public_key));
    std::string ciphertext = m_wallet->encrypt_with_view_secret_key(header + oss.str());
    bool r = epee::file_io_utils::save_string_to_file(filename, magic + ciphertext);
    if (!r)
    {
      fail_msg_writer() << tr("failed to save file ") << filename;
      return true;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting outputs: " << e.what());
    fail_msg_writer() << "Error exporting outputs: " << e.what();
    return true;
  }

  success_msg_writer() << tr("Outputs exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_outputs(const std::vector<std::string> &args)
{
  if (m_wallet->key_on_device())
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: import_outputs <filename>");
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
  const size_t magiclen = strlen(OUTPUT_EXPORT_FILE_MAGIC);
  if (data.size() < magiclen || memcmp(data.data(), OUTPUT_EXPORT_FILE_MAGIC, magiclen))
  {
    fail_msg_writer() << "Bad output export file magic in " << filename;
    return true;
  }

  try
  {
    data = m_wallet->decrypt_with_view_secret_key(std::string(data, magiclen));
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to decrypt " << filename << ": " << e.what();
    return true;
  }

  const size_t headerlen = 2 * sizeof(crypto::public_key);
  if (data.size() < headerlen)
  {
    fail_msg_writer() << "Bad data size from file " << filename;
    return true;
  }
  const crypto::public_key &public_spend_key = *(const crypto::public_key*)&data[0];
  const crypto::public_key &public_view_key = *(const crypto::public_key*)&data[sizeof(crypto::public_key)];
  const cryptonote::account_public_address &keys = m_wallet->get_account().get_keys().m_account_address;
  if (public_spend_key != keys.m_spend_public_key || public_view_key != keys.m_view_public_key)
  {
    fail_msg_writer() << "Outputs from " << filename << " are for a different account";
    return true;
  }

  try
  {
    std::string body(data, headerlen);
    std::stringstream iss;
    iss << body;
    std::vector<tools::wallet2::transfer_details> outputs;
    try
    {
      boost::archive::portable_binary_iarchive ar(iss);
      ar >> outputs;
    }
    catch (...)
    {
      iss.str("");
      iss << body;
      boost::archive::binary_iarchive ar(iss);
      ar >> outputs;
    }
    LOCK_IDLE_SCOPE();
    size_t n_outputs = m_wallet->import_outputs(outputs);
    success_msg_writer() << boost::lexical_cast<std::string>(n_outputs) << " outputs imported";
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << "Failed to import outputs: " << e.what();
    return true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfer(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: show_transfer <txid>");
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  const uint64_t last_block_height = m_wallet->get_blockchain_current_height();

  std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
  m_wallet->get_payments(payments, 0, (uint64_t)-1, m_current_subaddress_account);
  for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
    const tools::wallet2::payment_details &pd = i->second;
    if (pd.m_tx_hash == txid) {
      std::string payment_id = string_tools::pod_to_hex(i->first);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      success_msg_writer() << "Incoming transaction found";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Height: " << pd.m_block_height;
      success_msg_writer() << "Timestamp: " << get_human_readable_timestamp(pd.m_timestamp);
      success_msg_writer() << "Amount: " << print_money(pd.m_amount);
      success_msg_writer() << "Payment ID: " << payment_id;
      if (pd.m_unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
      {
        uint64_t bh = std::max(pd.m_unlock_time, pd.m_block_height + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
        if (bh >= last_block_height)
          success_msg_writer() << "Locked: " << (bh - last_block_height) << " blocks to unlock";
        else
          success_msg_writer() << std::to_string(last_block_height - bh) << " confirmations";
      }
      else
      {
        uint64_t current_time = static_cast<uint64_t>(time(NULL));
        uint64_t threshold = current_time + (m_wallet->use_fork_rules(2, 0) ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2 : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1);
        if (threshold >= pd.m_unlock_time)
          success_msg_writer() << "unlocked for " << get_human_readable_timespan(std::chrono::seconds(threshold - pd.m_unlock_time));
        else
          success_msg_writer() << "locked for " << get_human_readable_timespan(std::chrono::seconds(pd.m_unlock_time - threshold));
      }
      success_msg_writer() << "Address index: " << pd.m_subaddr_index.minor;
      success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
      return true;
    }
  }

  std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments_out;
  m_wallet->get_payments_out(payments_out, 0, (uint64_t)-1, m_current_subaddress_account);
  for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments_out.begin(); i != payments_out.end(); ++i) {
    if (i->first == txid)
    {
      const tools::wallet2::confirmed_transfer_details &pd = i->second;
      uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
      uint64_t fee = pd.m_amount_in - pd.m_amount_out;
      std::string dests;
      for (const auto &d: pd.m_dests) {
        if (!dests.empty())
          dests += ", ";
        dests +=  get_account_address_as_str(m_wallet->nettype(), d.is_subaddress, d.addr) + ": " + print_money(d.amount);
      }
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      success_msg_writer() << "Outgoing transaction found";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Height: " << pd.m_block_height;
      success_msg_writer() << "Timestamp: " << get_human_readable_timestamp(pd.m_timestamp);
      success_msg_writer() << "Amount: " << print_money(pd.m_amount_in - change - fee);
      success_msg_writer() << "Payment ID: " << payment_id;
      success_msg_writer() << "Change: " << print_money(change);
      success_msg_writer() << "Fee: " << print_money(fee);
      success_msg_writer() << "Destinations: " << dests;
      success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
      return true;
    }
  }

  try
  {
    m_wallet->update_pool_state();
    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pool_payments;
    m_wallet->get_unconfirmed_payments(pool_payments, m_current_subaddress_account);
    for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = pool_payments.begin(); i != pool_payments.end(); ++i) {
      const tools::wallet2::payment_details &pd = i->second.m_pd;
      if (pd.m_tx_hash == txid)
      {
        std::string payment_id = string_tools::pod_to_hex(i->first);
        if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
          payment_id = payment_id.substr(0,16);
        success_msg_writer() << "Unconfirmed incoming transaction found in the txpool";
        success_msg_writer() << "txid: " << txid;
        success_msg_writer() << "Timestamp: " << get_human_readable_timestamp(pd.m_timestamp);
        success_msg_writer() << "Amount: " << print_money(pd.m_amount);
        success_msg_writer() << "Payment ID: " << payment_id;
        success_msg_writer() << "Address index: " << pd.m_subaddr_index.minor;
        success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
        if (i->second.m_double_spend_seen)
          success_msg_writer() << tr("Double spend seen on the network: this transaction may or may not end up being mined");
        return true;
      }
    }
  }
  catch (...)
  {
    fail_msg_writer() << "Failed to get pool state";
  }

  std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
  m_wallet->get_unconfirmed_payments_out(upayments, m_current_subaddress_account);
  for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
    if (i->first == txid)
    {
      const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
      uint64_t amount = pd.m_amount_in;
      uint64_t fee = amount - pd.m_amount_out;
      std::string payment_id = string_tools::pod_to_hex(i->second.m_payment_id);
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;

      success_msg_writer() << (is_failed ? "Failed" : "Pending") << " outgoing transaction found";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Timestamp: " << get_human_readable_timestamp(pd.m_timestamp);
      success_msg_writer() << "Amount: " << print_money(amount - pd.m_change - fee);
      success_msg_writer() << "Payment ID: " << payment_id;
      success_msg_writer() << "Change: " << print_money(pd.m_change);
      success_msg_writer() << "Fee: " << print_money(fee);
      success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
      return true;
    }
  }

  fail_msg_writer() << tr("Transaction ID not found");
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
void simple_wallet::commit_or_save(std::vector<tools::wallet2::pending_tx>& ptx_vector, bool do_not_relay)
{
  size_t i = 0;
  while (!ptx_vector.empty())
  {
    auto & ptx = ptx_vector.back();
    const crypto::hash txid = get_transaction_hash(ptx.tx);
    if (do_not_relay)
    {
      cryptonote::blobdata blob;
      tx_to_blob(ptx.tx, blob);
      const std::string blob_hex = epee::string_tools::buff_to_hex_nodelimer(blob);
      const std::string filename = "raw_monero_tx" + (ptx_vector.size() == 1 ? "" : ("_" + std::to_string(i++)));
      if (epee::file_io_utils::save_string_to_file(filename, blob_hex))
        success_msg_writer(true) << tr("Transaction successfully saved to ") << filename << tr(", txid ") << txid;
      else
        fail_msg_writer() << tr("Failed to save transaction to ") << filename << tr(", txid ") << txid;
    }
    else
    {
      m_wallet->commit_tx(ptx);
      success_msg_writer(true) << tr("Transaction successfully submitted, transaction ") << txid << ENDL
      << tr("You can check its status by using the `show_transfers` command.");
    }
    // if no exception, remove element from vector
    ptx_vector.pop_back();
  }
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
#ifdef WIN32
  // Activate UTF-8 support for Boost filesystem classes on Windows
  std::locale::global(boost::locale::generator().generate(""));
  boost::filesystem::path::imbue(std::locale());
#endif

  po::options_description desc_params(wallet_args::tr("Wallet options"));
  tools::wallet2::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_device);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_generate_from_spend_key);
  command_line::add_arg(desc_params, arg_generate_from_keys);
  command_line::add_arg(desc_params, arg_generate_from_multisig_keys);
  command_line::add_arg(desc_params, arg_generate_from_json);
  command_line::add_arg(desc_params, arg_mnemonic_language);
  command_line::add_arg(desc_params, arg_command);

  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_restore_multisig_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_electrum_seed );
  command_line::add_arg(desc_params, arg_trusted_daemon);
  command_line::add_arg(desc_params, arg_untrusted_daemon);
  command_line::add_arg(desc_params, arg_allow_mismatched_daemon_version);
  command_line::add_arg(desc_params, arg_restore_height);
  command_line::add_arg(desc_params, arg_do_not_relay);
  command_line::add_arg(desc_params, arg_create_address_file);
  command_line::add_arg(desc_params, arg_subaddress_lookahead);
  command_line::add_arg(desc_params, arg_use_english_language_names);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  const auto vm = wallet_args::main(
   argc, argv,
   "monero-wallet-cli [--wallet-file=<file>|--generate-new-wallet=<file>] [<COMMAND>]",
    sw::tr("This is the command line monero wallet. It needs to connect to a monero\ndaemon to work correctly.\nWARNING: Do not reuse your Monero keys on an another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy."),
    desc_params,
    positional_options,
    [](const std::string &s, bool emphasis){ tools::scoped_message_writer(emphasis ? epee::console_color_white : epee::console_color_default, true) << s; },
    "monero-wallet-cli.log"
  );

  if (!vm)
  {
    return 1;
  }

  cryptonote::simple_wallet w;
  const bool r = w.init(*vm);
  CHECK_AND_ASSERT_MES(r, 1, sw::tr("Failed to initialize wallet"));

  std::vector<std::string> command = command_line::get_arg(*vm, arg_command);
  if (!command.empty())
  {
    if (!w.process_command(command))
      fail_msg_writer() << tr("Unknown command: ") << command.front();
    w.stop();
    w.deinit();
  }
  else
  {
    tools::signal_handler::install([&w](int type) {
      if (tools::password_container::is_prompting.load())
      {
        // must be prompting for password so return and let the signal stop prompt
        return;
      }
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
  return 0;
  //CATCH_ENTRY_L0("main", 1);
}
