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
#include "common/dns_utils.h"
#include "common/base58.h"
#include "common/scoped_message_writer.h"
#include "p2p/net_node.h"
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
#include "wallet/wallet_args.h"
#include <stdexcept>

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

#define DEFAULT_MIX 4

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
  const auto allowed_priority_strings = {"default", "unimportant", "normal", "elevated", "priority"};
  const auto arg_wallet_file = wallet_args::arg_wallet_file();
  const command_line::arg_descriptor<std::string> arg_generate_new_wallet = {"generate-new-wallet", sw::tr("Generate new wallet and save it to <arg>"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_view_key = {"generate-from-view-key", sw::tr("Generate incoming-only wallet from view key"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_keys = {"generate-from-keys", sw::tr("Generate wallet from private keys"), ""};
  const command_line::arg_descriptor<std::string> arg_generate_from_multisig_keys = {"generate-from-multisig-keys", sw::tr("Generate a master wallet from multisig wallet keys"), ""};
  const auto arg_generate_from_json = wallet_args::arg_generate_from_json();
  const command_line::arg_descriptor<std::string> arg_mnemonic_language = {"mnemonic-language", sw::tr("Language for mnemonic"), ""};
  const command_line::arg_descriptor<std::string> arg_electrum_seed = {"electrum-seed", sw::tr("Specify Electrum seed for wallet recovery/creation"), ""};
  const command_line::arg_descriptor<bool> arg_restore_deterministic_wallet = {"restore-deterministic-wallet", sw::tr("Recover wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("Create non-deterministic view and spend keys"), false};
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

  bool is_it_true(const std::string& s)
  {
    if (s == "1" || command_line::is_yes(s))
      return true;

    boost::algorithm::is_iequal ignore_case{};
    if (boost::algorithm::equals("true", s, ignore_case))
      return true;
    if (boost::algorithm::equals(simple_wallet::tr("true"), s, ignore_case))
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
    std::string confirm_dns_ok = command_line::input_line(prompt.str());
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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  // don't log
  std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key) << std::endl;
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
  std::cout << "secret: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key) << std::endl;
  std::cout << "public: " << string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key) << std::endl;

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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
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
  const auto pwd_container = tools::wallet2::password_prompt(true);

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
  message_writer() << (boost::format(tr("Current fee is %s monero per kB")) % print_money(per_kb_fee)).str();

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

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->always_confirm_transfers(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_print_ring_members(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->print_ring_members(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
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
    m_wallet->store_tx_info(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
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
      fail_msg_writer() << tr("ring size must be an integer >= 3");
      return true;
    }
    uint32_t ring_size = boost::lexical_cast<uint32_t>(args[1]);
    if (ring_size < 3 && ring_size != 0)
    {
      fail_msg_writer() << tr("ring size must be an integer >= 3");
      return true;
    }
    if (ring_size == 0)
      ring_size = DEFAULT_MIX + 1;
 
    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet->default_mixin(ring_size - 1);
      m_wallet->rewrite(m_wallet_file, pwd_container->password());
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("ring size must be an integer >= 3");
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
        fail_msg_writer() << tr("priority must be 0, 1, 2, 3,or 4");
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
    fail_msg_writer() << tr("priority must be 0, 1, 2 3,or 4");
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
    const bool auto_refresh = is_it_true(args[1]);
    m_wallet->auto_refresh(auto_refresh);
    m_idle_mutex.lock();
    m_auto_refresh_enabled.store(auto_refresh, std::memory_order_relaxed);
    m_idle_cond.notify_one();
    m_idle_mutex.unlock();

    m_wallet->rewrite(m_wallet_file, pwd_container->password());
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
    m_wallet->confirm_missing_payment_id(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_ask_password(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->ask_password(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
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
    m_wallet->set_default_decimal_point(decimal_point);
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
    m_wallet->merge_destinations(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
  }
  return true;
}

bool simple_wallet::set_confirm_backlog(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet->confirm_backlog(is_it_true(args[1]));
    m_wallet->rewrite(m_wallet_file, pwd_container->password());
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

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << get_commands_str();
  return true;
}

simple_wallet::simple_wallet()
  : m_allow_mismatched_daemon_version(false)
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
  m_cmd_binder.set_handler("transfer_original", boost::bind(&simple_wallet::transfer, this, _1), tr("Same as transfer, but using an older transaction building algorithm"));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::transfer_new, this, _1), tr("transfer [<priority>] [<ring_size>] <address> <amount> [<payment_id>] - Transfer <amount> to <address>. <priority> is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <ring_size> is the number of inputs to include for untraceability. Multiple payments can be made at once by adding <address_2> <amount_2> etcetera (before the payment ID, if it's included)"));
  m_cmd_binder.set_handler("locked_transfer", boost::bind(&simple_wallet::locked_transfer, this, _1), tr("locked_transfer [<ring_size>] <addr> <amount> <lockblocks>(Number of blocks to lock the transaction for, max 1000000) [<payment_id>]"));
  m_cmd_binder.set_handler("sweep_unmixable", boost::bind(&simple_wallet::sweep_unmixable, this, _1), tr("Send all unmixable outputs to yourself with ring_size 1"));
  m_cmd_binder.set_handler("sweep_all", boost::bind(&simple_wallet::sweep_all, this, _1), tr("sweep_all [ring_size] address [payment_id] - Send all unlocked balance to an address"));
  m_cmd_binder.set_handler("sweep_below", boost::bind(&simple_wallet::sweep_below, this, _1), tr("sweep_below <amount_threshold> [ring_size] address [payment_id] - Send all unlocked outputs below the threshold to an address"));
  m_cmd_binder.set_handler("donate", boost::bind(&simple_wallet::donate, this, _1), tr("donate [<ring_size>] <amount> [payment_id] - Donate <amount> to the development team (donate.getmonero.org)"));
  m_cmd_binder.set_handler("sign_transfer", boost::bind(&simple_wallet::sign_transfer, this, _1), tr("Sign a transaction from a file"));
  m_cmd_binder.set_handler("submit_transfer", boost::bind(&simple_wallet::submit_transfer, this, _1), tr("Submit a signed transaction from a file"));
  m_cmd_binder.set_handler("set_log", boost::bind(&simple_wallet::set_log, this, _1), tr("set_log <level>|<categories> - Change current log detail (level must be <0-4>)"));
  m_cmd_binder.set_handler("address", boost::bind(&simple_wallet::print_address, this, _1), tr("Show current wallet public address"));
  m_cmd_binder.set_handler("integrated_address", boost::bind(&simple_wallet::print_integrated_address, this, _1), tr("integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID"));
  m_cmd_binder.set_handler("address_book", boost::bind(&simple_wallet::address_book, this, _1), tr("address_book [(add (<address> [pid <long or short payment id>])|<integrated address> [<description possibly with whitespaces>])|(delete <index>)] - Print all entries in the address book, optionally adding/deleting an entry to/from it"));
  m_cmd_binder.set_handler("save", boost::bind(&simple_wallet::save, this, _1), tr("Save wallet data"));
  m_cmd_binder.set_handler("save_watch_only", boost::bind(&simple_wallet::save_watch_only, this, _1), tr("Save a watch-only keys file"));
  m_cmd_binder.set_handler("viewkey", boost::bind(&simple_wallet::viewkey, this, _1), tr("Display private view key"));
  m_cmd_binder.set_handler("spendkey", boost::bind(&simple_wallet::spendkey, this, _1), tr("Display private spend key"));
  m_cmd_binder.set_handler("seed", boost::bind(&simple_wallet::seed, this, _1), tr("Display Electrum-style mnemonic seed"));
  m_cmd_binder.set_handler("set", boost::bind(&simple_wallet::set_variable, this, _1), tr("Available options: seed language - set wallet seed language; always-confirm-transfers <1|0> - whether to confirm unsplit txes; print-ring-members <1|0> - whether to print detailed information about ring members during confirmation; store-tx-info <1|0> - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-ring-size <n> - set default ring size (default is 5); auto-refresh <1|0> - whether to automatically sync new blocks from the daemon; refresh-type <full|optimize-coinbase|no-coinbase|default> - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id <1|0>; ask-password <1|0>; unit <monero|millinero|micronero|nanonero|piconero> - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value; merge-destinations <1|0> - whether to merge multiple payments to the same destination address; confirm-backlog <1|0> - whether to warn if there is transaction backlog; refresh-from-block-height [n] - set height before which to ignore blocks"));
  m_cmd_binder.set_handler("rescan_spent", boost::bind(&simple_wallet::rescan_spent, this, _1), tr("Rescan blockchain for spent outputs"));
  m_cmd_binder.set_handler("get_tx_key", boost::bind(&simple_wallet::get_tx_key, this, _1), tr("Get transaction key (r) for a given <txid>"));
  m_cmd_binder.set_handler("check_tx_key", boost::bind(&simple_wallet::check_tx_key, this, _1), tr("Check amount going to <address> in <txid>"));
  m_cmd_binder.set_handler("get_tx_proof", boost::bind(&simple_wallet::get_tx_proof, this, _1), tr("Generate a signature to prove payment to <address> in <txid> using the transaction secret key (r) without revealing it"));
  m_cmd_binder.set_handler("check_tx_proof", boost::bind(&simple_wallet::check_tx_proof, this, _1), tr("Check tx proof for payment going to <address> in <txid>"));
  m_cmd_binder.set_handler("show_transfers", boost::bind(&simple_wallet::show_transfers, this, _1), tr("show_transfers [in|out|pending|failed|pool] [<min_height> [<max_height>]] - Show incoming/outgoing transfers within an optional height range"));
  m_cmd_binder.set_handler("unspent_outputs", boost::bind(&simple_wallet::unspent_outputs, this, _1), tr("unspent_outputs [<min_amount> <max_amount>] - Show unspent outputs within an optional amount range"));
  m_cmd_binder.set_handler("rescan_bc", boost::bind(&simple_wallet::rescan_blockchain, this, _1), tr("Rescan blockchain from scratch"));
  m_cmd_binder.set_handler("set_tx_note", boost::bind(&simple_wallet::set_tx_note, this, _1), tr("Set an arbitrary string note for a txid"));
  m_cmd_binder.set_handler("get_tx_note", boost::bind(&simple_wallet::get_tx_note, this, _1), tr("Get a string note for a txid"));
  m_cmd_binder.set_handler("status", boost::bind(&simple_wallet::status, this, _1), tr("Show wallet status information"));
  m_cmd_binder.set_handler("sign", boost::bind(&simple_wallet::sign, this, _1), tr("Sign the contents of a file"));
  m_cmd_binder.set_handler("verify", boost::bind(&simple_wallet::verify, this, _1), tr("Verify a signature on the contents of a file"));
  m_cmd_binder.set_handler("export_key_images", boost::bind(&simple_wallet::export_key_images, this, _1), tr("Export a signed set of key images"));
  m_cmd_binder.set_handler("import_key_images", boost::bind(&simple_wallet::import_key_images, this, _1), tr("Import signed key images list and verify their spent status"));
  m_cmd_binder.set_handler("export_outputs", boost::bind(&simple_wallet::export_outputs, this, _1), tr("Export a set of outputs owned by this wallet"));
  m_cmd_binder.set_handler("import_outputs", boost::bind(&simple_wallet::import_outputs, this, _1), tr("Import set of outputs owned by this wallet"));
  m_cmd_binder.set_handler("show_transfer", boost::bind(&simple_wallet::show_transfer, this, _1), tr("Show information about a transfer to/from this address"));
  m_cmd_binder.set_handler("password", boost::bind(&simple_wallet::change_password, this, _1), tr("Change wallet password"));
  m_cmd_binder.set_handler("payment_id", boost::bind(&simple_wallet::payment_id, this, _1), tr("Generate a new random full size payment id - these will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids"));
  m_cmd_binder.set_handler("fee", boost::bind(&simple_wallet::print_fee_info, this, _1), tr("Print information about fee and current transaction backlog"));
  m_cmd_binder.set_handler("help", boost::bind(&simple_wallet::help, this, _1), tr("Show this help"));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    success_msg_writer() << "seed = " << m_wallet->get_seed_language();
    success_msg_writer() << "always-confirm-transfers = " << m_wallet->always_confirm_transfers();
    success_msg_writer() << "print-ring-members = " << m_wallet->print_ring_members();
    success_msg_writer() << "store-tx-info = " << m_wallet->store_tx_info();
    success_msg_writer() << "default-ring-size = " << (m_wallet->default_mixin() ? m_wallet->default_mixin() + 1 : 0);
    success_msg_writer() << "auto-refresh = " << m_wallet->auto_refresh();
    success_msg_writer() << "refresh-type = " << get_refresh_type_name(m_wallet->get_refresh_type());
    success_msg_writer() << "priority = " << m_wallet->get_default_priority();
    success_msg_writer() << "confirm-missing-payment-id = " << m_wallet->confirm_missing_payment_id();
    success_msg_writer() << "ask-password = " << m_wallet->ask_password();
    success_msg_writer() << "unit = " << cryptonote::get_unit(m_wallet->get_default_decimal_point());
    success_msg_writer() << "min-outputs-count = " << m_wallet->get_min_output_count();
    success_msg_writer() << "min-outputs-value = " << cryptonote::print_money(m_wallet->get_min_output_value());
    success_msg_writer() << "merge-destinations = " << m_wallet->merge_destinations();
    success_msg_writer() << "confirm-backlog = " << m_wallet->confirm_backlog();
    success_msg_writer() << "refresh-from-block-height = " << m_wallet->get_refresh_from_block_height();
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
    CHECK_SIMPLE_VARIABLE("default-ring-size", set_default_ring_size, tr("integer >= 3"));
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
    CHECK_SIMPLE_VARIABLE("refresh-from-block-height", set_refresh_from_block_height, tr("block height"));
  }
  fail_msg_writer() << tr("set: unrecognized argument(s)");
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() != 1)
  {
    fail_msg_writer() << tr("usage: set_log <log_level_number_0-4> | <categories>");
    return true;
  }
  mlog_set_log(args[0].c_str());
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
      wallet_path = command_line::input_line(
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
            confirm_creation = command_line::input_line(tr("(Y/Yes/N/No): "));
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
  success_msg_writer(true) << "\n" << tr("PLEASE NOTE: the following 25 words can be used to recover access to your wallet. "
    "Please write them down and store them somewhere safe and secure. Please do not store them in "
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
  if (!handle_command_line(vm))
    return false;

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_view_key.empty()) + (!m_generate_from_keys.empty()) + (!m_generate_from_multisig_keys.empty()) + (!m_generate_from_json.empty()) > 1)
  {
    fail_msg_writer() << tr("can't specify more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\", --generate-from-view-key=\"wallet_name\", --generate-from-keys=\"wallet_name\", --generate-from-multisig-keys=\"wallet_name\" and --generate-from-json=\"jsonfilename\"");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_view_key.empty() && m_generate_from_keys.empty() && m_generate_from_multisig_keys.empty() && m_generate_from_json.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  if (!m_generate_new.empty() || m_restoring)
  {
    std::string old_language;
    // check for recover flag.  if present, require electrum word list (only recovery option for now).
    if (m_restore_deterministic_wallet)
    {
      if (m_non_deterministic)
      {
        fail_msg_writer() << tr("can't specify both --restore-deterministic-wallet and --non-deterministic");
        return false;
      }
      if (!m_wallet_file.empty())
      {
        fail_msg_writer() << tr("--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file");
        return false;
      }

      if (m_electrum_seed.empty())
      {
        m_electrum_seed = "";
        do
        {
          const char *prompt = m_electrum_seed.empty() ? "Specify Electrum seed: " : "Electrum seed continued: ";
          std::string electrum_seed = command_line::input_line(prompt);
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

      if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
      {
        fail_msg_writer() << tr("Electrum-style word list failed verification");
        return false;
      }
    }
    if (!m_generate_from_view_key.empty())
    {
      m_wallet_file = m_generate_from_view_key;
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
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, tools::wallet2::has_testnet_option(vm), address_string))
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
      if (address.m_view_public_key != pkey) {
        fail_msg_writer() << tr("view key does not match standard address");
        return false;
      }

      bool r = new_wallet(vm, address, boost::none, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    else if (!m_generate_from_keys.empty())
    {
      m_wallet_file = m_generate_from_keys;
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
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, tools::wallet2::has_testnet_option(vm), address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }

      // parse spend secret key
      std::string spendkey_string = command_line::input_line("Secret spend key: ");
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
      std::string viewkey_string = command_line::input_line("Secret view key: ");
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
      bool r = new_wallet(vm, address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    
    // Asks user for all the data required to merge secret keys from multisig wallets into one master wallet, which then gets full control of the multisig wallet. The resulting wallet will be the same as any other regular wallet.
    else if (!m_generate_from_multisig_keys.empty())
    {
      m_wallet_file = m_generate_from_multisig_keys;
      unsigned int multisig_m;
      unsigned int multisig_n;
      
      // parse multisig type
      std::string multisig_type_string = command_line::input_line("Multisig type (input as M/N with M <= N and M > 1): ");
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
      std::string address_string = command_line::input_line("Multisig wallet address: ");
      if (std::cin.eof())
        return false;
      if (address_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      cryptonote::account_public_address address;
      bool has_payment_id;
      crypto::hash8 new_payment_id;
      if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, tools::wallet2::has_testnet_option(vm), address_string))
      {
          fail_msg_writer() << tr("failed to parse address");
          return false;
      }
      
      // parse secret view key
      std::string viewkey_string = command_line::input_line("Secret view key: ");
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
      if (address.m_view_public_key != pkey)
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
          spendkey_string = command_line::input_line(tr((boost::format(tr("Secret spend key (%u of %u):")) % (i+i) % multisig_m).str().c_str()));
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
      if (address.m_spend_public_key != pkey)
      {
        fail_msg_writer() << tr("spend key does not match standard address");
        return false;
      }
      
      // create wallet
      bool r = new_wallet(vm, address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    
    else if (!m_generate_from_json.empty())
    {
      m_wallet_file = m_generate_from_json;
      m_wallet = tools::wallet2::make_from_json(vm, m_wallet_file);
      if (!m_wallet)
        return false;
    }
    else
    {
      if (m_generate_new.empty()) {
        fail_msg_writer() << tr("specify a wallet path with --generate-new-wallet (not --wallet-file)");
        return false;
      }
      m_wallet_file = m_generate_new;
      bool r = new_wallet(vm, m_recovery_key, m_restore_deterministic_wallet, m_non_deterministic, old_language);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
    }
    if (!m_restore_height && m_restoring)
    {
      uint32_t version;
      bool connected = try_connect_to_daemon(false, &version);
      while (true)
      {
        std::string heightstr;
        if (!connected || version < MAKE_CORE_RPC_VERSION(1, 6))
          heightstr = command_line::input_line("Restore from specific blockchain height (optional, default 0): ");
        else
          heightstr = command_line::input_line("Restore from specific blockchain height (optional, default 0),\nor alternatively from specific date (YYYY-MM-DD): ");
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
            // lexical_cast<uint8_t> won't work becasue uint8_t is treated as character type
            month = boost::lexical_cast<uint16_t>(heightstr.substr(5,2));
            day   = boost::lexical_cast<uint16_t>(heightstr.substr(8,2));
            m_restore_height = m_wallet->get_blockchain_height_by_date(year, month, day);
            success_msg_writer() << tr("Restore height is: ") << m_restore_height;
            std::string confirm = command_line::input_line(tr("Is this okay?  (Y/Yes/N/No): "));
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
      m_wallet->set_refresh_from_block_height(m_restore_height);
    }
  }
  else
  {
    assert(!m_wallet_file.empty());
    bool r = open_wallet(vm);
    CHECK_AND_ASSERT_MES(r, false, tr("failed to open account"));
  }
  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return false;
  }

  // set --trusted-daemon if local
  try
  {
    if (tools::is_local_address(m_wallet->get_daemon_address()))
    {
      MINFO(tr("Daemon is local, assuming trusted"));
      m_trusted_daemon = true;
    }
  }
  catch (const std::exception &e) { }

  m_http_client.set_server(m_wallet->get_daemon_address(), m_wallet->get_daemon_login());
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
  m_generate_from_view_key        = command_line::get_arg(vm, arg_generate_from_view_key);
  m_generate_from_keys            = command_line::get_arg(vm, arg_generate_from_keys);
  m_generate_from_multisig_keys   = command_line::get_arg(vm, arg_generate_from_multisig_keys);
  m_generate_from_json            = command_line::get_arg(vm, arg_generate_from_json);
  m_mnemonic_language             = command_line::get_arg(vm, arg_mnemonic_language);
  m_electrum_seed                 = command_line::get_arg(vm, arg_electrum_seed);
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  m_trusted_daemon                = command_line::get_arg(vm, arg_trusted_daemon);
  m_allow_mismatched_daemon_version = command_line::get_arg(vm, arg_allow_mismatched_daemon_version);
  m_restore_height                = command_line::get_arg(vm, arg_restore_height);
  m_restoring                     = !m_generate_from_view_key.empty() ||
                                    !m_generate_from_keys.empty() ||
                                    !m_generate_from_multisig_keys.empty() ||
                                    !m_generate_from_json.empty() ||
                                    m_restore_deterministic_wallet;

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
        "Please make sure daemon is running or restart the wallet with the correct daemon address.");
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
    catch (const std::exception &e)
    {
      fail_msg_writer() << tr("invalid language choice passed. Please try again.\n");
    }
  }
  return language_list[language_number];
}
//----------------------------------------------------------------------------------------------------
boost::optional<tools::password_container> simple_wallet::get_and_verify_password() const
{
  auto pwd_container = tools::wallet2::password_prompt(m_wallet_file.empty());
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
  auto rc = tools::wallet2::make_new(vm);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
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
  if ((!two_random) && (mnemonic_language.empty()) && (!m_restore_deterministic_wallet || was_deprecated_wallet))
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

  crypto::secret_key recovery_val;
  try
  {
    recovery_val = m_wallet->generate(m_wallet_file, std::move(rc.second).password(), recovery_key, recover, two_random);
    message_writer(console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
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
bool simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const cryptonote::account_public_address& address, const boost::optional<crypto::secret_key>& spendkey,
  const crypto::secret_key& viewkey)
{
  auto rc = tools::wallet2::make_new(vm);
  m_wallet = std::move(rc.first);
  if (!m_wallet)
  {
    return false;
  }
  if (m_restore_height)
    m_wallet->set_refresh_from_block_height(m_restore_height);

  try
  {
    if (spendkey)
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, *spendkey, viewkey);
    }
    else
    {
      m_wallet->generate(m_wallet_file, std::move(rc.second).password(), address, viewkey);
    }
    message_writer(console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
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
  std::string password;
  try
  {
    auto rc = tools::wallet2::make_from_file(vm, m_wallet_file);
    m_wallet = std::move(rc.first);
    password = std::move(rc.second).password();
    if (!m_wallet)
    {
      return false;
    }

    message_writer(console_color_white, true) <<
      (m_wallet->watch_only() ? tr("Opened watch-only wallet") : tr("Opened wallet")) << ": "
      << m_wallet->get_account().get_public_address_str(m_wallet->testnet());
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
    // only suggest removing cache if the password was actually correct
    if (m_wallet && m_wallet->verify_password(password))
      fail_msg_writer() << boost::format(tr("You may want to remove the file \"%s\" and try again")) % m_wallet_file;
    return false;
  }
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
  const auto pwd_container = tools::password_container::prompt(true, tr("Password for new watch-only wallet"));

  if (!pwd_container)
  {
    fail_msg_writer() << tr("failed to read wallet password");
    return true;
  }

  m_wallet->write_watch_only_wallet(m_wallet_file, pwd_container->password());
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

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }
  COMMAND_RPC_START_MINING::request req = AUTO_VAL_INIT(req); 
  req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());

  bool ok = true;
  size_t max_mining_threads_count = (std::max)(tools::get_max_concurrency(), static_cast<unsigned>(2));
  size_t arg_size = args.size();
  if(arg_size >= 3) req.ignore_battery = args[2] == "true";
  if(arg_size >= 2) req.do_background_mining = args[1] == "true";
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
  bool r = net_utils::invoke_http_json("/start_mining", req, res, m_http_client);
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
  bool r = net_utils::invoke_http_json("/stop_mining", req, res, m_http_client);
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

  if (!m_wallet)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }
  COMMAND_RPC_SAVE_BC::request req;
  COMMAND_RPC_SAVE_BC::response res;
  bool r = net_utils::invoke_http_json("/save_bc", req, res, m_http_client);
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
void simple_wallet::on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount)
{
  message_writer(console_color_green, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << txid << ", " <<
    tr("received ") << print_money(amount);
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount)
{
  // Not implemented in CLI wallet
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx)
{
  message_writer(console_color_magenta, false) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << txid << ", " <<
    tr("spent ") << print_money(amount);
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(height, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx)
{
  message_writer(console_color_red, true) << "\r" <<
    tr("Height ") << height << ", " <<
    tr("transaction ") << txid << ", " <<
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
  bool verbose = false;
  for (const auto &arg: args)
  {
    if (arg == "available")
    {
      filter = true;
      available = true;
    }
    else if (arg == "unavailable")
    {
      filter = true;
      available = false;
    }
    else if (arg == "verbose")
    {
      verbose = true;
    }
  }

  PAUSE_READLINE();

  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);

  bool transfers_found = false;
  for (const auto& td : transfers)
  {
    if (!filter || available != td.m_spent)
    {
      if (!transfers_found)
      {
        std::string verbose_string;
        if (verbose)
          verbose_string = (boost::format("%68s%68s") % tr("pubkey") % tr("key image")).str();
        message_writer() << boost::format("%21s%8s%12s%8s%16s%68s%s") % tr("amount") % tr("spent") % tr("unlocked") % tr("ringct") % tr("global index") % tr("tx id") % verbose_string;
        transfers_found = true;
      }
      std::string verbose_string;
      if (verbose)
        verbose_string = (boost::format("%68s%68s") % td.get_public_key() % (td.m_key_image_known ? epee::string_tools::pod_to_hex(td.m_key_image) : std::string('?', 64))).str();
      message_writer(td.m_spent ? console_color_magenta : console_color_green, false) <<
        boost::format("%21s%8s%12s%8s%16u%68s%s") %
        print_money(td.amount()) %
        (td.m_spent ? tr("T") : tr("F")) %
        (m_wallet->is_transfer_unlocked(td) ? tr("unlocked") : tr("locked")) %
        (td.is_rct() ? tr("RingCT") : tr("-")) %
        td.m_global_output_index %
        td.m_txid %
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
    fail_msg_writer() << tr("expected at least one payment_id");
    return true;
  }

  LOCK_IDLE_SCOPE();

  PAUSE_READLINE();

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
  if (!m_wallet)
  {
    throw std::runtime_error("simple_wallet null wallet");
  }

  COMMAND_RPC_GET_HEIGHT::request req;
  COMMAND_RPC_GET_HEIGHT::response res = boost::value_initialized<COMMAND_RPC_GET_HEIGHT::response>();
  bool r = net_utils::invoke_http_json("/getheight", req, res, m_http_client);
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
    std::vector<int>          spent_key_height(tx.vin.size());
    std::vector<crypto::hash> spent_key_txid  (tx.vin.size());
    for (size_t i = 0; i < tx.vin.size(); ++i)
    {
      if (tx.vin[i].type() != typeid(cryptonote::txin_to_key))
        continue;
      const cryptonote::txin_to_key& in_key = boost::get<cryptonote::txin_to_key>(tx.vin[i]);
      const cryptonote::tx_source_entry& source = construction_data.sources[i];
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
      bool r = net_utils::invoke_http_bin("/get_outs.bin", req, res, m_http_client);
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
        if (std::abs(spent_key_height[i] - spent_key_height[j]) < 5)
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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  LOCK_IDLE_SCOPE();

  std::vector<std::string> local_args = args_;

  int priority = 0;
  if(local_args.size() > 0) {
    auto priority_pos = std::find(
      allowed_priority_strings.begin(),
      allowed_priority_strings.end(),
      local_args[0]);
    if(priority_pos != allowed_priority_strings.end()) {
      local_args.erase(local_args.begin());
      priority = std::distance(allowed_priority_strings.begin(), priority_pos);
    }
  }

  size_t fake_outs_count;
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
  for (size_t i = 0; i < local_args.size(); i += 2)
  {
    cryptonote::tx_destination_entry de;
    bool has_payment_id;
    crypto::hash8 new_payment_id;
    if (!cryptonote::get_account_address_from_str_or_url(de.addr, has_payment_id, new_payment_id, m_wallet->testnet(), local_args[i], oa_prompter))
    {
      fail_msg_writer() << tr("failed to parse address");
      return true;
    }

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
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
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
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, unlock_block /* unlock_time */, priority, extra, m_trusted_daemon);
      break;
      case TransferNew:
        ptx_vector = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, m_trusted_daemon);
      break;
      default:
        LOG_ERROR("Unknown transfer method, using original");
        /* FALLTHRU */
      case TransferOriginal:
        ptx_vector = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, priority, extra, m_trusted_daemon);
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
      uint64_t size = 0, fee = 0;
      for (size_t n = 0; n < ptx_vector.size(); ++n)
      {
        const uint64_t blob_size = cryptonote::tx_to_blob(ptx_vector[n].tx).size();
        const double fee_per_byte = ptx_vector[n].fee / (double)blob_size;
        if (fee_per_byte < worst_fee_per_byte)
        {
          worst_fee_per_byte = fee_per_byte;
          fee = ptx_vector[n].fee;
        }
        size += blob_size;
      }
      try
      {
        std::vector<std::pair<uint64_t, uint64_t>> nblocks = m_wallet->estimate_backlog(size, size, {fee});
        if (nblocks.size() != 1)
        {
          prompt << "Internal error checking for backlog. " << tr("Is this okay anyway?  (Y/Yes/N/No): ");
        }
        else
        {
          if (nblocks[0].first > 0)
            prompt << (boost::format(tr("There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No)")) % nblocks[0].first).str();
        }
      }
      catch (const std::exception &e)
      {
        prompt << tr("Failed to check for backlog: ") << e.what() << ENDL << tr("Is this okay anyway?  (Y/Yes/N/No): ");
      }

      std::string prompt_str = prompt.str();
      if (!prompt_str.empty())
      {
        std::string accepted = command_line::input_line(prompt_str);
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
        prompt << ENDL << tr("Is this okay?  (Y/Yes/N/No): ");
        
        std::string accepted = command_line::input_line(prompt.str());
        if (std::cin.eof())
          return true;
        if (!command_line::is_yes(accepted))
        {
          fail_msg_writer() << tr("transaction cancelled.");

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
      success_msg_writer(true) << tr("Transaction successfully submitted, transaction ") << get_transaction_hash(ptx.tx) << ENDL
      << tr("You can check its status by using the `show_transfers` command.");

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
  catch (const tools::error::get_random_outs_error &e)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix: ") << e.what();
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
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
    std::string accepted = command_line::input_line(prompt_str);
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");

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
  catch (const tools::error::get_random_outs_error &e)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix: ") << e.what();
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
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
bool simple_wallet::sweep_main(uint64_t below, const std::vector<std::string> &args_)
{
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  size_t fake_outs_count;
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
  if (!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, new_payment_id, m_wallet->testnet(), local_args[0], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

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
     std::string accepted = command_line::input_line(tr("No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): "));
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
    auto ptx_vector = m_wallet->create_transactions_all(below, address, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra, m_trusted_daemon);

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
    if (!print_ring_members(ptx_vector, prompt))
      return true;
    if (ptx_vector.size() > 1) {
      prompt << boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): ")) %
        print_money(total_sent) %
        ((unsigned long long)ptx_vector.size()) %
        print_money(total_fee);
    }
    else {
      prompt << boost::format(tr("Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)")) %
        print_money(total_sent) %
        print_money(total_fee);
    }
    std::string accepted = command_line::input_line(prompt.str());
    if (std::cin.eof())
      return true;
    if (!command_line::is_yes(accepted))
    {
      fail_msg_writer() << tr("transaction cancelled.");

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
  catch (const tools::error::get_random_outs_error &e)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix: ") << e.what();
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
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
  return sweep_main(0, args_);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_below(const std::vector<std::string> &args_)
{
  uint64_t below = 0;
  if (args_.size() < 1)
  {
    fail_msg_writer() << tr("missing amount threshold");
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
  std::vector<std::string> local_args = args_;
  if(local_args.empty() || local_args.size() > 3)
  {
     fail_msg_writer() << tr("wrong number of arguments");
     return true;
  }
  std::string ring_size_str;
  // Hardcode Monero's donation address (see #1447)
  const std::string address_str = "44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A";
  std::string amount_str;
  std::string payment_id_str;
  // check payment id
  crypto::hash payment_id;
  crypto::hash8 payment_id8;
  if (tools::wallet2::parse_long_payment_id (local_args.back(), payment_id ) ||
      tools::wallet2::parse_short_payment_id(local_args.back(), payment_id8))
  {
    payment_id_str = local_args.back();
    local_args.pop_back();
  }
  // check ring size
  if (local_args.size() > 1)
  {
    ring_size_str = local_args[0];
    local_args.erase(local_args.begin());
  }
  amount_str = local_args[0];
  // refill args as necessary
  local_args.clear();
  if (!ring_size_str.empty())
    local_args.push_back(ring_size_str);
  local_args.push_back(address_str);
  local_args.push_back(amount_str);
  if (!payment_id_str.empty())
    local_args.push_back(payment_id_str);
  message_writer() << tr("Donating ") << amount_str << " XMR to The Monero Project (donate.getmonero.org/44AFFq5kSiGBoZ4NMDwYtN18obc8AemS33DBLWs3H7otXft3XjrpDtQGv7SqSsaBYBb98uNbr2VBBEt7f2wfn3RVGQBEP3A).";
  transfer_new(local_args);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::accept_loaded_tx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message)
{
  // gather info to ask the user
  uint64_t amount = 0, amount_to_dests = 0, change = 0;
  size_t min_ring_size = ~0;
  std::unordered_map<std::string, std::pair<std::string, uint64_t>> dests;
  const std::string wallet_address = m_wallet->get_account().get_public_address_str(m_wallet->testnet());
  int first_known_non_zero_change_index = -1;
  std::string payment_id_string = "";
  for (size_t n = 0; n < get_num_txes(); ++n)
  {
    const tools::wallet2::tx_construction_data &cd = get_tx(n);

    std::vector<tx_extra_field> tx_extra_fields;
    bool has_encrypted_payment_id = false;
    crypto::hash8 payment_id8 = cryptonote::null_hash8;
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
      std::string address, standard_address = get_account_address_as_str(m_wallet->testnet(), entry.addr);
      if (has_encrypted_payment_id)
      {
        address = get_account_integrated_address_as_str(m_wallet->testnet(), entry.addr, payment_id8);
        address += std::string(" (" + standard_address + " with encrypted payment id " + epee::string_tools::pod_to_hex(payment_id8) + ")");
      }
      else
        address = standard_address;
      std::unordered_map<std::string,std::pair<std::string,uint64_t>>::iterator i = dests.find(standard_address);
      if (i == dests.end())
        dests.insert(std::make_pair(standard_address, std::make_pair(address, entry.amount)));
      else
        i->second.second += entry.amount;
      amount_to_dests += entry.amount;
    }
    if (cd.change_dts.amount > 0)
    {
      std::unordered_map<std::string, std::pair<std::string, uint64_t>>::iterator it = dests.find(get_account_address_as_str(m_wallet->testnet(), cd.change_dts.addr));
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
        dests.erase(get_account_address_as_str(m_wallet->testnet(), cd.change_dts.addr));
    }
  }

  if (payment_id_string.empty())
    payment_id_string = "no payment ID";

  std::string dest_string;
  for (std::unordered_map<std::string, std::pair<std::string, uint64_t>>::const_iterator i = dests.begin(); i != dests.end(); )
  {
    dest_string += (boost::format(tr("sending %s to %s")) % print_money(i->second.second) % i->second.first).str();
    ++i;
    if (i != dests.end())
      dest_string += ", ";
  }
  if (dest_string.empty())
    dest_string = tr("with no destinations");

  std::string change_string;
  if (change > 0)
  {
    std::string address = get_account_address_as_str(m_wallet->testnet(), get_tx(0).change_dts.addr);
    change_string += (boost::format(tr("%s change to %s")) % print_money(change) % address).str();
  }
  else
    change_string += tr("no change");

  uint64_t fee = amount - amount_to_dests;
  std::string prompt_str = (boost::format(tr("Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): ")) % (unsigned long)get_num_txes() % print_money(amount) % print_money(fee) % dest_string % change_string % (unsigned long)min_ring_size % payment_id_string % extra_message).str();
  return command_line::is_yes(command_line::input_line(prompt_str));
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
  if(m_wallet->watch_only())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  std::vector<tools::wallet2::pending_tx> ptx;
  try
  {
    bool r = m_wallet->sign_tx("unsigned_monero_tx", "signed_monero_tx", ptx, [&](const tools::wallet2::unsigned_tx_set &tx){ return accept_loaded_tx(tx); });
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
    bool r = m_wallet->load_tx("signed_monero_tx", ptx_vector, [&](const tools::wallet2::signed_tx_set &tx){ return accept_loaded_tx(tx); });
    if (!r)
    {
      fail_msg_writer() << tr("Failed to load transaction from file");
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
  catch (const tools::error::get_random_outs_error &e)
  {
    fail_msg_writer() << tr("failed to get random outputs to mix: ") << e.what();
  }
  catch (const tools::error::not_enough_money& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, sent amount %s") %
      print_money(e.available()) %
      print_money(e.tx_amount()));
    fail_msg_writer() << tr("Not enough money in unlocked balance");
  }
  catch (const tools::error::tx_not_possible& e)
  {
    LOG_PRINT_L0(boost::format("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)") %
      print_money(e.available()) %
      print_money(e.tx_amount() + e.fee())  %
      print_money(e.tx_amount()) %
      print_money(e.fee()));
    fail_msg_writer() << tr("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees");
  }
  catch (const tools::error::not_enough_outs_to_mix& e)
  {
    auto writer = fail_msg_writer();
    writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
    for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
    {
      writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
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
bool simple_wallet::get_tx_proof(const std::vector<std::string> &args)
{
  if(args.size() != 2 && args.size() != 3) {
    fail_msg_writer() << tr("usage: get_tx_proof <txid> <dest_address> [<tx_key>]");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args[0], txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, payment_id, m_wallet->testnet(), args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  LOCK_IDLE_SCOPE();

  crypto::secret_key tx_key, tx_key2;
  bool r = m_wallet->get_tx_key(txid, tx_key);
  cryptonote::blobdata tx_key_data;
  if (args.size() == 3)
  {
    if(!epee::string_tools::parse_hexstr_to_binbuff(args[2], tx_key_data) || tx_key_data.size() != sizeof(crypto::secret_key))
    {
      fail_msg_writer() << tr("failed to parse tx_key");
      return true;
    }
    tx_key2 = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());
  }
  if (r)
  {
    if (args.size() == 3 && tx_key != rct::sk2rct(tx_key2))
    {
      fail_msg_writer() << tr("Tx secret key was found for the given txid, but you've also provided another tx secret key which doesn't match the found one.");
      return true;
    }
  }
  else
  {
    if (tx_key_data.empty())
    {
      fail_msg_writer() << tr("Tx secret key wasn't found in the wallet file. Provide it as the optional third parameter if you have it elsewhere.");
      return true;
    }
    tx_key = tx_key2;
  }

  crypto::public_key R;
  crypto::secret_key_to_public_key(tx_key, R);
  crypto::public_key rA = rct::rct2pk(rct::scalarmultKey(rct::pk2rct(address.m_view_public_key), rct::sk2rct(tx_key)));
  crypto::signature sig;
  try
  {
    crypto::generate_tx_proof(txid, R, address.m_view_public_key, rA, tx_key, sig);
  }
  catch (const std::runtime_error &e)
  {
    fail_msg_writer() << e.what();
    return true;
  }

  std::string sig_str = std::string("ProofV1") +
    tools::base58::encode(std::string((const char *)&rA, sizeof(crypto::public_key))) +
    tools::base58::encode(std::string((const char *)&sig, sizeof(crypto::signature)));

  success_msg_writer() << tr("Signature: ") << sig_str;
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
  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[0], txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  if (local_args[1].size() < 64 || local_args[1].size() % 64)
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  crypto::secret_key tx_key;
  cryptonote::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(local_args[1], tx_key_data) || tx_key_data.size() != sizeof(crypto::secret_key))
  {
    fail_msg_writer() << tr("failed to parse tx key");
    return true;
  }
  tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, payment_id, m_wallet->testnet(), local_args[2], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
  {
    fail_msg_writer() << tr("failed to generate key derivation from supplied parameters");
    return true;
  }

  return check_tx_key_helper(txid, address, derivation);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key_helper(const crypto::hash &txid, const cryptonote::account_public_address &address, const crypto::key_derivation &derivation)
{
  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json("/gettransactions", req, res, m_http_client) ||
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
bool simple_wallet::check_tx_proof(const std::vector<std::string> &args)
{
  if(args.size() != 3) {
    fail_msg_writer() << tr("usage: check_tx_proof <txid> <address> <signature>");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  // parse txid
  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args[0], txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  // parse address
  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, payment_id, m_wallet->testnet(), args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  // parse pubkey r*A & signature
  std::string sig_str = args[2];
  const size_t header_len = strlen("ProofV1");
  if (sig_str.size() < header_len || sig_str.substr(0, header_len) != "ProofV1")
  {
    fail_msg_writer() << tr("Signature header check error");
    return true;
  }
  crypto::public_key rA;
  crypto::signature sig;
  const size_t rA_len = tools::base58::encode(std::string((const char *)&rA, sizeof(crypto::public_key))).size();
  const size_t sig_len = tools::base58::encode(std::string((const char *)&sig, sizeof(crypto::signature))).size();
  std::string rA_decoded;
  std::string sig_decoded;
  if (!tools::base58::decode(sig_str.substr(header_len, rA_len), rA_decoded))
  {
    fail_msg_writer() << tr("Signature decoding error");
    return true;
  }
  if (!tools::base58::decode(sig_str.substr(header_len + rA_len, sig_len), sig_decoded))
  {
    fail_msg_writer() << tr("Signature decoding error");
    return true;
  }
  if (sizeof(crypto::public_key) != rA_decoded.size() || sizeof(crypto::signature) != sig_decoded.size())
  {
    fail_msg_writer() << tr("Signature decoding error");
    return true;
  }
  memcpy(&rA, rA_decoded.data(), sizeof(crypto::public_key));
  memcpy(&sig, sig_decoded.data(), sizeof(crypto::signature));

  // fetch tx pubkey from the daemon
  COMMAND_RPC_GET_TRANSACTIONS::request req;
  COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!net_utils::invoke_http_json("/gettransactions", req, res, m_http_client) ||
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
  crypto::public_key R = get_tx_pub_key_from_extra(tx);
  if (R == null_pkey)
  {
    fail_msg_writer() << tr("Tx pubkey was not found");
    return true;
  }

  // check signature
  if (crypto::check_tx_proof(txid, R, address.m_view_public_key, rA, sig))
  {
    success_msg_writer() << tr("Good signature");
  }
  else
  {
    fail_msg_writer() << tr("Bad signature");
    return true;
  }

  // obtain key derivation by multiplying scalar 1 to the pubkey r*A included in the signature
  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(rA, rct::rct2sk(rct::I), derivation))
  {
    fail_msg_writer() << tr("failed to generate key derivation");
    return true;
  }

  return check_tx_key_helper(txid, address, derivation);
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

  PAUSE_READLINE();

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
      uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
      uint64_t fee = pd.m_amount_in - pd.m_amount_out;
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
    message_writer(i->second.first ? console_color_green : console_color_magenta, false) <<
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
      uint64_t fee = amount - pd.m_amount_out;
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
bool simple_wallet::unspent_outputs(const std::vector<std::string> &args_)
{
  if(!args_.empty() && args_.size() != 2) {
    fail_msg_writer() << tr("usage: unspent_outputs [<min_amount> <max_amount>]");
    return true;
  }
  uint64_t min_amount = 0;
  uint64_t max_amount = std::numeric_limits<uint64_t>::max();
  if (args_.size() == 2)
  {
    if (!cryptonote::parse_amount(min_amount, args_[0]) || !cryptonote::parse_amount(max_amount, args_[1]))
    {
      fail_msg_writer() << tr("amount is wrong: ") << args_[0] << ' ' << args_[1] <<
        ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
      return true;
    }
    if (min_amount > max_amount)
    {
      fail_msg_writer() << tr("<min_amount> should be smaller than <max_amount>");
      return true;
    }
  }
  tools::wallet2::transfer_container transfers;
  m_wallet->get_transfers(transfers);
  if (transfers.empty())
  {
    success_msg_writer() << "There is no unspent output in this wallet.";
    return true;
  }
  std::map<uint64_t, tools::wallet2::transfer_container> amount_to_tds;
  uint64_t min_height = std::numeric_limits<uint64_t>::max();
  uint64_t max_height = 0;
  uint64_t found_min_amount = std::numeric_limits<uint64_t>::max();
  uint64_t found_max_amount = 0;
  uint64_t count = 0;
  for (const auto& td : transfers)
  {
    uint64_t amount = td.amount();
    if (td.m_spent || amount < min_amount || amount > max_amount)
      continue;
    amount_to_tds[amount].push_back(td);
    if (min_height > td.m_block_height) min_height = td.m_block_height;
    if (max_height < td.m_block_height) max_height = td.m_block_height;
    if (found_min_amount > amount) found_min_amount = amount;
    if (found_max_amount < amount) found_max_amount = amount;
    ++count;
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
  std::string addr_start = m_wallet->get_account().get_public_address_str(m_wallet->testnet()).substr(0, 6);
  std::string prompt = std::string("[") + tr("wallet") + " " + addr_start;
  uint32_t version;
  if (!m_wallet->check_connection(&version))
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

  refresh_main(0, false);

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
    cryptonote::account_public_address address;
    bool has_payment_id;
    crypto::hash8 payment_id8;
    if(!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, payment_id8, m_wallet->testnet(), args[1], oa_prompter))
    {
      fail_msg_writer() << tr("failed to parse address");
      return true;
    }
    crypto::hash payment_id = null_hash;
    size_t description_start = 2;
    if (has_payment_id)
    {
      memcpy(payment_id.data, payment_id8.data, 8);
    }
    else if (!has_payment_id && args.size() >= 4 && args[2] == "pid")
    {
      if (tools::wallet2::parse_long_payment_id(args[3], payment_id))
      {
        description_start += 2;
      }
      else if (tools::wallet2::parse_short_payment_id(args[3], payment_id8))
      {
        memcpy(payment_id.data, payment_id8.data, 8);
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
    m_wallet->add_address_book_row(address, payment_id, description);
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
      success_msg_writer() << tr("Address: ") << get_account_address_as_str(m_wallet->testnet(), row.m_address);
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

  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!cryptonote::get_account_address_from_str_or_url(address, has_payment_id, payment_id, m_wallet->testnet(), address_string, oa_prompter))
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
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  std::string filename = args[0];

  try
  {
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
  if (!m_trusted_daemon)
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
  if (args.size() != 1)
  {
    fail_msg_writer() << tr("usage: export_outputs <filename>");
    return true;
  }
  if (m_wallet->ask_password() && !get_and_verify_password()) { return true; }
  std::string filename = args[0];

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
  m_wallet->get_payments(payments, 0);
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
      success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
      return true;
    }
  }

  std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments_out;
  m_wallet->get_payments_out(payments_out, 0);
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
        dests +=  get_account_address_as_str(m_wallet->testnet(), d.addr) + ": " + print_money(d.amount);
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
    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> pool_payments;
    m_wallet->get_unconfirmed_payments(pool_payments);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = pool_payments.begin(); i != pool_payments.end(); ++i) {
      const tools::wallet2::payment_details &pd = i->second;
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
        success_msg_writer() << "Note: " << m_wallet->get_tx_note(txid);
        return true;
      }
    }
  }
  catch (...)
  {
    fail_msg_writer() << "Failed to get pool state";
  }

  std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
  m_wallet->get_unconfirmed_payments_out(upayments);
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
int main(int argc, char* argv[])
{
  po::options_description desc_params(wallet_args::tr("Wallet options"));
  tools::wallet2::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_generate_from_keys);
  command_line::add_arg(desc_params, arg_generate_from_multisig_keys);
  command_line::add_arg(desc_params, arg_generate_from_json);
  command_line::add_arg(desc_params, arg_mnemonic_language);
  command_line::add_arg(desc_params, arg_command);

  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_electrum_seed );
  command_line::add_arg(desc_params, arg_trusted_daemon);
  command_line::add_arg(desc_params, arg_allow_mismatched_daemon_version);
  command_line::add_arg(desc_params, arg_restore_height);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  const auto vm = wallet_args::main(
   argc, argv,
   "monero-wallet-cli [--wallet-file=<file>|--generate-new-wallet=<file>] [<COMMAND>]",
    desc_params,
    positional_options,
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
  return 0;
  //CATCH_ENTRY_L0("main", 1);
}
