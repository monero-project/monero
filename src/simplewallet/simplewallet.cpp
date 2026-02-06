// Copyright (c) 2014-2024, The Monero Project
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

// use boost bind placeholders for now
#include <memory>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS 1
#include <boost/bind.hpp>

#include <locale.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string_view>
#include <ctype.h>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/filesystem.hpp>
#include "include_base_utils.h"
#include "console_handler.h"
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
#include "wallet/api/enote_details.h"
#include "wallet/wallet_args.h"
#include "wallet/fee_algorithm.h"
#include "wallet/fee_priority.h"
#include "version.h"
#include <stdexcept>
#include "wallet/message_store.h"
#include "wallet/wallet_errors.h"
#include "net/parse.h"
#include "QrCode.hpp"
#include "string_tools.h"

#ifdef WIN32
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#endif

#ifdef HAVE_READLINE
#include "readline_buffer.h"
#endif

using namespace std;
using namespace epee;
using namespace cryptonote;
using namespace Monero;
using boost::lexical_cast;
namespace po = boost::program_options;
typedef cryptonote::simple_wallet sw;
using tools::fee_priority;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.simplewallet"

#define EXTENDED_LOGS_FILE "wallet_details.log"

#define OLD_AGE_WARN_THRESHOLD (30 * 86400 / DIFFICULTY_TARGET_V2) // 30 days

#define LOCK_IDLE_SCOPE() \
  bool auto_refresh_enabled = m_auto_refresh_enabled.load(std::memory_order_relaxed); \
  m_auto_refresh_enabled.store(false, std::memory_order_relaxed); \
  /* stop any background refresh and other processes, and take over */ \
  m_wallet_impl->stop(); \
  boost::unique_lock<boost::mutex> lock(m_idle_mutex); \
  m_idle_cond.notify_all(); \
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ \
    /* m_idle_mutex is still locked here */ \
    m_auto_refresh_enabled.store(auto_refresh_enabled, std::memory_order_relaxed); \
    m_idle_cond.notify_one(); \
  })

#define SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(code) \
  LOCK_IDLE_SCOPE(); \
  boost::optional<tools::password_container> pwd_container = boost::none; \
  if (m_wallet_impl->getAskPasswordType() != Wallet::AskPasswordType::AskPassword_Never && !(pwd_container = get_and_verify_password())) { code; } \
  std::unique_ptr<Monero::WalletKeysDecryptGuard> unlocker = \
          m_wallet_impl->createKeysDecryptGuard(std::string_view(pwd_container ? pwd_container->password().data() : "", pwd_container ? pwd_container->password().size() : 0))

#define SCOPED_WALLET_UNLOCK() SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return true;)

#define PRINT_USAGE(usage_help) fail_msg_writer() << boost::format(tr("usage: %s")) % usage_help;

#define LONG_PAYMENT_ID_SUPPORT_CHECK() \
  do { \
    fail_msg_writer() << tr("Error: Long payment IDs are obsolete."); \
    fail_msg_writer() << tr("Long payment IDs were not encrypted on the blockchain and would harm your privacy."); \
    fail_msg_writer() << tr("If the party you're sending to still requires a long payment ID, please notify them."); \
    return true; \
  } while(0)

#define SHORT_PAYMENT_ID_WITHOUT_INTEGRATED_ADDRESS_SUPPORT_CHECK() \
  do { \
    fail_msg_writer() << tr("Short payment IDs are supposed to be used with integrated addresses."); \
    fail_msg_writer() << tr("If you want to make a payment with a certain payment_id, the receiver has to generate an address with `integrated_address <payment_id>`"); \
    return true; \
  } while(0)

#define REFRESH_PERIOD 90 // seconds

#define MAX_MNEW_ADDRESSES 65536

#define CHECK_MULTISIG_ENABLED() \
  do \
  { \
    if (!m_wallet_impl->getEnableMultisig()) \
    { \
      fail_msg_writer() << tr("Multisig is disabled."); \
      fail_msg_writer() << tr("Multisig is an experimental feature and may have bugs. Things that could go wrong include: funds sent to a multisig wallet can't be spent at all, can only be spent with the participation of a malicious group member, or can be stolen by a malicious group member."); \
      fail_msg_writer() << tr("You can enable it with:"); \
      fail_msg_writer() << tr("  set enable-multisig-experimental 1"); \
      return false; \
    } \
  } while(0)

#define CHECK_MULTISIG_READY() \
  const MultisigState ms_status{m_wallet_impl->multisig()}; \
  do \
  { \
    if (m_wallet_impl->getDeviceState().key_on_device) \
    { \
      fail_msg_writer() << tr("command not supported by HW wallet"); \
      return false; \
    } \
    if (!ms_status.isMultisig) \
    { \
      fail_msg_writer() << tr("This wallet is not multisig"); \
      return false; \
    } \
    if (!ms_status.isReady) \
    { \
      fail_msg_writer() << tr("This multisig wallet is not yet finalized"); \
      return false; \
    } \
  } while(0)

#define CHECK_IF_BACKGROUND_SYNCING(msg) \
  do \
  { \
    if (m_wallet_impl->isBackgroundWallet() || m_wallet_impl->isBackgroundSyncing()) \
    { \
      std::string type = m_wallet_impl->isBackgroundWallet() ? "background wallet" : "background syncing wallet"; \
      fail_msg_writer() << boost::format(tr("%s %s")) % type % msg; \
      return false; \
    } \
  } while (0)

// Copied from wallet2, used by save_to_file() and load_from_file()
static const std::string ASCII_OUTPUT_MAGIC = "MoneroAsciiDataV1";

static std::string get_human_readable_timespan(std::chrono::seconds seconds);
static std::string get_human_readable_timespan(uint64_t seconds);

namespace
{
  constexpr std::array<std::string_view, 5> allowed_priority_strings = tools::fee_priority_utilities::fee_priority_strings;
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
  const command_line::arg_descriptor<bool> arg_restore_from_seed = {"restore-from-seed", sw::tr("alias for --restore-deterministic-wallet"), false};
  const command_line::arg_descriptor<bool> arg_restore_multisig_wallet = {"restore-multisig-wallet", sw::tr("Recover multisig wallet using Electrum-style mnemonic seed"), false};
  const command_line::arg_descriptor<bool> arg_non_deterministic = {"non-deterministic", sw::tr("Generate non-deterministic view and spend keys"), false};
  const command_line::arg_descriptor<uint64_t> arg_restore_height = {"restore-height", sw::tr("Restore from specific blockchain height"), 0};
  const command_line::arg_descriptor<std::string> arg_restore_date = {"restore-date", sw::tr("Restore from estimated blockchain height on specified date"), ""};
  const command_line::arg_descriptor<bool> arg_do_not_relay = {"do-not-relay", sw::tr("The newly created transaction will not be relayed to the monero network"), false};
  const command_line::arg_descriptor<bool> arg_create_address_file = {"create-address-file", sw::tr("Create an address file for new wallets"), false};
  const command_line::arg_descriptor<std::string> arg_subaddress_lookahead = {"subaddress-lookahead", sw::tr("Set subaddress lookahead sizes to <major>:<minor>"), ""};
  const command_line::arg_descriptor<bool> arg_use_english_language_names = {"use-english-language-names", sw::tr("Display English language names"), false};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", sw::tr("Use daemon instance at <host>:<port>"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", sw::tr("Use daemon instance at host <arg> instead of localhost"), ""};
  const command_line::arg_descriptor<std::string> arg_proxy = {"proxy", sw::tr("[<ip>:]<port> socks proxy to use for daemon connections"), {}, true};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", sw::tr("Enable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_untrusted_daemon = {"untrusted-daemon", sw::tr("Disable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<std::string> arg_password = {"password", sw::tr("Wallet password (escape/quote as needed)"), "", true};
  const command_line::arg_descriptor<std::string> arg_password_file = wallet_args::arg_password_file();
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", sw::tr("Use daemon instance at port <arg> instead of 18081"), 0};
  const command_line::arg_descriptor<std::string> arg_daemon_login = {"daemon-login", sw::tr("Specify username[:password] for daemon RPC client"), "", true};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl = {"daemon-ssl", sw::tr("Enable SSL on daemon RPC connections: enabled|disabled|autodetect"), "autodetect"};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_private_key = {"daemon-ssl-private-key", sw::tr("Path to a PEM format private key"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_certificate = {"daemon-ssl-certificate", sw::tr("Path to a PEM format certificate"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_ca_certificates = {"daemon-ssl-ca-certificates", sw::tr("Path to file containing concatenated PEM format certificate(s) to replace system CA(s).")};
  const command_line::arg_descriptor<std::vector<std::string>> arg_daemon_ssl_allowed_fingerprints = {"daemon-ssl-allowed-fingerprints", sw::tr("List of valid fingerprints of allowed RPC servers")};
  const command_line::arg_descriptor<bool> arg_daemon_ssl_allow_any_cert = {"daemon-ssl-allow-any-cert", sw::tr("Allow any SSL certificate from the daemon"), false};
  const command_line::arg_descriptor<bool> arg_daemon_ssl_allow_chained = {"daemon-ssl-allow-chained", sw::tr("Allow user (via --daemon-ssl-ca-certificates) chain certificates"), false};
  const command_line::arg_descriptor<bool> arg_allow_mismatched_daemon_version = {"allow-mismatched-daemon-version", sw::tr("Allow communicating with a daemon that uses a different version"), false};
  const command_line::arg_descriptor<bool> arg_testnet = {"testnet", sw::tr("For testnet. Daemon must also be launched with --testnet flag"), false};
  const command_line::arg_descriptor<bool> arg_stagenet = {"stagenet", sw::tr("For stagenet. Daemon must also be launched with --stagenet flag"), false};
  const command_line::arg_descriptor<uint64_t> arg_kdf_rounds = {"kdf-rounds", sw::tr("Number of rounds for the key derivation function"), 1};
  const command_line::arg_descriptor<std::string> arg_hw_device = {"hw-device", sw::tr("HW device to use"), ""};
  const command_line::arg_descriptor<std::string> arg_hw_device_derivation_path = {"hw-device-deriv-path", sw::tr("HW device wallet derivation path (e.g., SLIP-10)"), ""};
  const command_line::arg_descriptor<std::string> arg_tx_notify = { "tx-notify" , sw::tr("Run a program for each new incoming transaction, '%s' will be replaced by the transaction hash") , "" };
  const command_line::arg_descriptor<bool> arg_no_dns = {"no-dns", sw::tr("Do not use DNS"), false};
  const command_line::arg_descriptor<bool> arg_offline = {"offline", sw::tr("Do not connect to a daemon, nor use DNS"), false};
  const command_line::arg_descriptor<std::string> arg_extra_entropy = {"extra-entropy", sw::tr("File containing extra entropy to initialize the PRNG (any data, aim for 256 bits of entropy to be useful, which typically means more than 256 bits of data)")};

  const command_line::arg_descriptor< std::vector<std::string> > arg_command = {"command", ""};

  const char* USAGE_START_MINING("start_mining [<number_of_threads>] [bg_mining] [ignore_battery]");
  const char* USAGE_SET_DAEMON("set_daemon <host>[:<port>] [trusted|untrusted|this-is-probably-a-spy-node] [login=<user>[:<password>]] [proxy=[<ip>:]<port>]");
  const char* USAGE_SHOW_BALANCE("balance [detail]");
  const char* USAGE_INCOMING_TRANSFERS("incoming_transfers [available|unavailable] [verbose] [uses] [index=<N1>[,<N2>[,...]]]");
  const char* USAGE_PAYMENTS("payments <PID_1> [<PID_2> ... <PID_N>]");
  const char* USAGE_PAYMENT_ID("payment_id");
  const char* USAGE_TRANSFER("transfer [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] (<URI> | <address> <amount>) [subtractfeefrom=<D0>[,<D1>,all,...]]");
  const char* USAGE_SWEEP_ALL("sweep_all [index=<N1>[,<N2>,...] | index=all] [<priority>] [<ring_size>] [outputs=<N>] <address>");
  const char* USAGE_SWEEP_ACCOUNT("sweep_account <account> [index=<N1>[,<N2>,...] | index=all] [<priority>] [<ring_size>] [outputs=<N>] <address>");
  const char* USAGE_SWEEP_BELOW("sweep_below <amount_threshold> [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] [outputs=<N>] <address>");
  const char* USAGE_SWEEP_SINGLE("sweep_single [<priority>] [<ring_size>] [outputs=<N>] <key_image> <address>");
  const char* USAGE_DONATE("donate [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <amount>");
  const char* USAGE_SIGN_TRANSFER("sign_transfer [export_raw] [<filename>]");
  const char* USAGE_SET_LOG("set_log <level>|{+,-,}<categories>");
  const char* USAGE_ACCOUNT("account\n"
                            "  account new <label text with white spaces allowed>\n"
                            "  account switch <index> \n"
                            "  account label <index> <label text with white spaces allowed>\n"
                            "  account tag <tag_name> <account_index_1> [<account_index_2> ...]\n"
                            "  account untag <account_index_1> [<account_index_2> ...]\n"
                            "  account tag_description <tag_name> <description>");
  const char* USAGE_ADDRESS("address [ new <label text with white spaces allowed> | mnew <amount of new addresses> | all | <index_min> [<index_max>] | label <index> <label text with white spaces allowed> | device [<index>] | one-off <account> <subaddress>]");
  const char* USAGE_INTEGRATED_ADDRESS("integrated_address [device] [<payment_id> | <address>]");
  const char* USAGE_ADDRESS_BOOK("address_book [(add (<address>|<integrated address>) [<description possibly with whitespaces>])|(delete <index>)]");
  const char* USAGE_SET_VARIABLE("set <option> [<value>]");
  const char* USAGE_GET_TX_KEY("get_tx_key <txid>");
  const char* USAGE_SET_TX_KEY("set_tx_key <txid> <tx_key> [<subaddress>]");
  const char* USAGE_CHECK_TX_KEY("check_tx_key <txid> <txkey> <address>");
  const char* USAGE_GET_TX_PROOF("get_tx_proof <txid> <address> [<message>]");
  const char* USAGE_CHECK_TX_PROOF("check_tx_proof <txid> <address> ( <signature_file> | <signature> ) [<message>]");
  const char* USAGE_GET_SPEND_PROOF("get_spend_proof <txid> [<message>]");
  const char* USAGE_CHECK_SPEND_PROOF("check_spend_proof <txid> <signature_file> [<message>]");
  const char* USAGE_GET_RESERVE_PROOF("get_reserve_proof (all|<amount>) [<message>]");
  const char* USAGE_CHECK_RESERVE_PROOF("check_reserve_proof <address> <signature_file> [<message>]");
  const char* USAGE_SHOW_TRANSFERS("show_transfers [in|out|all|pending|failed|pool|coinbase] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]");
  const char* USAGE_UNSPENT_OUTPUTS("unspent_outputs [index=<N1>[,<N2>,...]] [<min_amount> [<max_amount>]]");
  const char* USAGE_RESCAN_BC("rescan_bc [hard|soft|keep_ki] [start_height=0]");
  const char* USAGE_SET_TX_NOTE("set_tx_note <txid> [free text note]");
  const char* USAGE_GET_TX_NOTE("get_tx_note <txid>");
  const char* USAGE_GET_DESCRIPTION("get_description");
  const char* USAGE_SET_DESCRIPTION("set_description [free text note]");
  const char* USAGE_SIGN("sign [<account_index>,<address_index>] [--spend|--view] <filename>");
  const char* USAGE_VERIFY("verify <filename> <address> <signature>");
  const char* USAGE_EXPORT_KEY_IMAGES("export_key_images [all] <filename>");
  const char* USAGE_IMPORT_KEY_IMAGES("import_key_images <filename>");
  const char* USAGE_HW_KEY_IMAGES_SYNC("hw_key_images_sync");
  const char* USAGE_HW_RECONNECT("hw_reconnect");
  const char* USAGE_EXPORT_OUTPUTS("export_outputs [all] <filename>");
  const char* USAGE_IMPORT_OUTPUTS("import_outputs <filename>");
  const char* USAGE_SHOW_TRANSFER("show_transfer <txid>");
  const char* USAGE_MAKE_MULTISIG("make_multisig <threshold> <string1> [<string>...]");
  const char* USAGE_EXCHANGE_MULTISIG_KEYS("exchange_multisig_keys [force-update-use-with-caution] <string> [<string>...]");
  const char* USAGE_EXPORT_MULTISIG_INFO("export_multisig_info <filename>");
  const char* USAGE_IMPORT_MULTISIG_INFO("import_multisig_info <filename> [<filename>...]");
  const char* USAGE_SIGN_MULTISIG("sign_multisig <filename>");
  const char* USAGE_SUBMIT_MULTISIG("submit_multisig <filename>");
  const char* USAGE_EXPORT_RAW_MULTISIG_TX("export_raw_multisig_tx <filename>");
  const char* USAGE_PRINT_RING("print_ring <key_image> | <txid>");
  const char* USAGE_SET_RING("set_ring <filename> | ( <key_image> absolute|relative <index> [<index>...] )");
  const char* USAGE_FREEZE("freeze <key_image> | <enote_pubkey>");
  const char* USAGE_THAW("thaw <key_image> | <enote_pubkey>");
  const char* USAGE_FROZEN("frozen [<key_image> | <enote_pubkey>]");
  const char* USAGE_LOCK("lock");
  const char* USAGE_NET_STATS("net_stats");
  const char* USAGE_PUBLIC_NODES("public_nodes");
  const char* USAGE_WELCOME("welcome");
  const char* USAGE_SHOW_QR_CODE("show_qr_code [<subaddress_index>]");
  const char* USAGE_VERSION("version");
  const char* USAGE_HELP("help [<command> | all]");
  const char* USAGE_APROPOS("apropos <keyword> [<keyword> ...]");
  const char* USAGE_SCAN_TX("scan_tx <txid> [<txid> ...]");

  std::string input_line(const std::string& prompt, bool yesno = false)
  {
    PAUSE_READLINE();
    std::cout << prompt;
    if (yesno)
      std::cout << "  (Y/Yes/N/No)";
    std::cout << ": " << std::flush;

    std::string buf;
#ifdef _WIN32
    buf = tools::input_line_win();
#else
    std::getline(std::cin, buf);
#endif

    return epee::string_tools::trim(buf);
  }

  epee::wipeable_string input_secure_line(const char *prompt)
  {
    PAUSE_READLINE();
    auto pwd_container = tools::password_container::prompt(false, prompt, false);
    if (!pwd_container)
    {
      MERROR("Failed to read secure line");
      return "";
    }

    epee::wipeable_string buf = pwd_container->password();

    buf.trim();
    return buf;
  }

  boost::optional<tools::password_container> password_prompter(const char *prompt, bool verify)
  {
    PAUSE_READLINE();
    auto pwd_container = tools::password_container::prompt(verify, prompt);
    if (!pwd_container)
    {
      tools::fail_msg_writer() << sw::tr("failed to read password");
    }
    return pwd_container;
  }

  boost::optional<tools::password_container> default_password_prompter(bool verify)
  {
    return password_prompter(verify ? sw::tr("Enter a new password for the wallet") : sw::tr("Wallet password"), verify);
  }

  boost::optional<tools::password_container> background_sync_cache_password_prompter(bool verify)
  {
    return password_prompter(verify ? sw::tr("Enter a custom password for the background sync cache") : sw::tr("Background sync cache password"), verify);
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
      fail_msg_writer() << sw::tr("invalid argument: must be either 0/1, true/false, y/n, yes/no");
      return false;
    }
  }

  const struct
  {
    const char *name;
    Wallet::RefreshType refresh_type;
  } refresh_type_names[] =
  {
    { "full", Wallet::RefreshType::Refresh_Full },
    { "optimize-coinbase", Wallet::RefreshType::Refresh_OptimizeCoinbase },
    { "optimized-coinbase", Wallet::RefreshType::Refresh_OptimizeCoinbase },
    { "no-coinbase", Wallet::RefreshType::Refresh_NoCoinbase },
    { "default", Wallet::RefreshType::Refresh_Default },
  };

  bool parse_refresh_type(const std::string &s, Wallet::RefreshType &refresh_type)
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

  std::string get_refresh_type_name(Wallet::RefreshType type)
  {
    for (size_t n = 0; n < sizeof(refresh_type_names) / sizeof(refresh_type_names[0]); ++n)
    {
      if (type == refresh_type_names[n].refresh_type)
        return refresh_type_names[n].name;
    }
    return "invalid";
  }

  const struct
  {
    const char *name;
    Wallet::BackgroundSyncType background_sync_type;
  } background_sync_type_names[] =
  {
    { "off", Wallet::BackgroundSync_Off },
    { "reuse-wallet-password", Wallet::BackgroundSync_ReusePassword },
    { "custom-background-password", Wallet::BackgroundSync_CustomPassword },
  };

  bool parse_background_sync_type(const std::string &s, Wallet::BackgroundSyncType &background_sync_type)
  {
    for (size_t n = 0; n < sizeof(background_sync_type_names) / sizeof(background_sync_type_names[0]); ++n)
    {
      if (s == background_sync_type_names[n].name)
      {
        background_sync_type = background_sync_type_names[n].background_sync_type;
        return true;
      }
    }
    fail_msg_writer() << cryptonote::simple_wallet::tr("failed to parse background sync type");
    return false;
  }

  std::string get_background_sync_type_name(Wallet::BackgroundSyncType type)
  {
    for (size_t n = 0; n < sizeof(background_sync_type_names) / sizeof(background_sync_type_names[0]); ++n)
    {
      if (type == background_sync_type_names[n].background_sync_type)
        return background_sync_type_names[n].name;
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
      dnssec_str = sw::tr("DNSSEC validation passed");
    }
    else
    {
      dnssec_str = sw::tr("WARNING: DNSSEC validation was unsuccessful, this address may not be correct!");
    }
    std::stringstream prompt;
    prompt << sw::tr("For URL: ") << url
           << ", " << dnssec_str << std::endl
           << sw::tr(" Monero Address = ") << addresses[0]
           << std::endl
           << sw::tr("Is this OK?")
    ;
    // prompt the user for confirmation given the dns query and dnssec status
    std::string confirm_dns_ok = input_line(prompt.str(), true);
    if (std::cin.eof())
    {
      return {};
    }
    if (!command_line::is_yes(confirm_dns_ok))
    {
      std::cout << sw::tr("you have cancelled the transfer request") << std::endl;
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
        fail_msg_writer() << sw::tr("failed to parse index: ") << subaddr_index_str;
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
      fail_msg_writer() << sw::tr("invalid format for subaddress lookahead; must be <major>:<minor>");
    return r;
  }

  static constexpr std::string_view SFFD_ARG_NAME{"subtractfeefrom="};

  bool parse_subtract_fee_from_outputs
  (
    const std::string& arg,
    std::set<uint32_t>& subtract_fee_from_outputs,
    bool& subtract_fee_from_all,
    bool& matches
  )
  {
    matches = false;
    if (!boost::string_ref{arg}.starts_with(SFFD_ARG_NAME.data())) // if arg doesn't match
      return true;
    matches = true;

    const char* arg_end = arg.c_str() + arg.size();
    for (const char* p = arg.c_str() + SFFD_ARG_NAME.size(); p < arg_end;)
    {
      const char* new_p = nullptr;
      const unsigned long dest_index = strtoul(p, const_cast<char**>(&new_p), 10);
      if (dest_index == 0 && new_p == p) // numerical conversion failed
      {
        if (0 != strncmp(p, "all", 3))
        {
          fail_msg_writer() << tr("Failed to parse subtractfeefrom list");
          return false;
        }
        subtract_fee_from_all = true;
        break;
      }
      else if (dest_index > std::numeric_limits<uint32_t>::max())
      {
        fail_msg_writer() << tr("Destination index is too large") << ": " << dest_index;
        return false;
      }
      else
      {
        subtract_fee_from_outputs.insert(dest_index);
        p = new_p + 1; // skip the comma
      }
    }

    return true;
  }

  bool get_nettype(const boost::program_options::variables_map &vm, NetworkType &nettype_out)
  {
    bool is_testnet = command_line::has_arg(vm, arg_testnet);
    bool is_stagenet = command_line::has_arg(vm, arg_stagenet);
    if (is_testnet && is_stagenet)
    {
      fail_msg_writer() << tr("Can't specify more than one of --testnet and --stagenet");
      return false;
    }
    nettype_out = is_testnet ? NetworkType::TESTNET :
                  is_stagenet ? NetworkType::STAGENET :
                  NetworkType::MAINNET;
    return true;
  }

  bool get_daemon_address(const boost::program_options::variables_map &vm, std::string &daemon_address, std::string &daemon_username, std::string &daemon_password)
  {
    daemon_address = command_line::get_arg(vm, arg_daemon_address);
    auto daemon_host = command_line::get_arg(vm, arg_daemon_host);
    auto daemon_port = command_line::get_arg(vm, arg_daemon_port);

    THROW_WALLET_EXCEPTION_IF(!daemon_address.empty() && !daemon_host.empty() && 0 != daemon_port,
        tools::error::wallet_internal_error, sw::tr("can't specify daemon host or port more than once"));

    if (daemon_host.empty())
      daemon_host = "localhost";

    if (!daemon_port)
    {
      NetworkType nettype;
      if (!get_nettype(vm, nettype))
        return false;
      daemon_port = get_config(static_cast<network_type>(nettype)).RPC_DEFAULT_PORT;
    }

    if (daemon_address.empty())
      daemon_address = std::string("http://") + daemon_host + ":" + std::to_string(daemon_port);

    if (command_line::has_arg(vm, arg_daemon_login))
    {
      std::function<boost::optional<tools::password_container>(const char *, bool)> pw_prompter = password_prompter;
      auto parsed = tools::login::parse(
        command_line::get_arg(vm, arg_daemon_login), false, [pw_prompter](bool verify) {
          if (!pw_prompter)
          {
            MERROR("Password needed without prompt function");
            return boost::optional<tools::password_container>();
          }
          return pw_prompter("Daemon client password", verify);
        }
      );
      if (!parsed)
        return false;

      daemon_username = std::move(parsed->username);
      epee::wipeable_string daemon_pw = std::move(parsed->password).password();
      daemon_password = std::string(daemon_pw.data(), daemon_pw.size());
    }

    return true;
  }

  bool get_daemon_init_settings(const boost::program_options::variables_map &vm,
                                std::string &daemon_address,
                                std::string &daemon_username,
                                std::string &daemon_password,
                                boost::optional<bool> &trusted_daemon,
                                Monero::Wallet::SSLSupport &ssl_support,
                                std::string &ssl_private_key_path,
                                std::string &ssl_certificate_path,
                                std::string &ssl_ca_file_path,
                                std::vector<std::string> &ssl_allowed_fingerprints_str,
                                bool &ssl_allow_any_cert,
                                std::string &proxy)
  {
    if (!get_daemon_address(vm, daemon_address, daemon_username, daemon_password))
      return false;

    std::string ssl_support_str = command_line::get_arg(vm, arg_daemon_ssl);
    ssl_private_key_path = command_line::get_arg(vm, arg_daemon_ssl_private_key);
    ssl_certificate_path = command_line::get_arg(vm, arg_daemon_ssl_certificate);
    ssl_ca_file_path     = command_line::get_arg(vm, arg_daemon_ssl_ca_certificates);
    ssl_allowed_fingerprints_str = command_line::get_arg(vm, arg_daemon_ssl_allowed_fingerprints);
    ssl_allow_any_cert   = command_line::get_arg(vm, arg_daemon_ssl_allow_any_cert);

    const bool use_proxy = command_line::has_arg(vm, arg_proxy);

    // user specified CA file or fingeprints implies enabled SSL by default
    epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_enabled;
    if (ssl_allow_any_cert)
      ssl_options.verification = epee::net_utils::ssl_verification_t::none;
    else if (!ssl_ca_file_path.empty() || !ssl_allowed_fingerprints_str.empty())
    {
      std::vector<std::vector<uint8_t>> ssl_allowed_fingerprints{ ssl_allowed_fingerprints_str.size() };
      std::transform(ssl_allowed_fingerprints_str.begin(), ssl_allowed_fingerprints_str.end(), ssl_allowed_fingerprints.begin(), epee::from_hex_locale::to_vector);
      for (const auto &fpr: ssl_allowed_fingerprints)
      {
        THROW_WALLET_EXCEPTION_IF(fpr.size() != SSL_FINGERPRINT_SIZE, tools::error::wallet_internal_error,
            "SHA-256 fingerprint should be " BOOST_PP_STRINGIZE(SSL_FINGERPRINT_SIZE) " bytes long.");
      }

      ssl_options = epee::net_utils::ssl_options_t{
        std::move(ssl_allowed_fingerprints), std::move(ssl_ca_file_path)
      };

      if (command_line::get_arg(vm, arg_daemon_ssl_allow_chained))
        ssl_options.verification = epee::net_utils::ssl_verification_t::user_ca;
    }

    if (ssl_options.verification != epee::net_utils::ssl_verification_t::user_certificates || !command_line::is_arg_defaulted(vm, arg_daemon_ssl))
    {
      THROW_WALLET_EXCEPTION_IF(!epee::net_utils::ssl_support_from_string(ssl_options.support, ssl_support_str), tools::error::wallet_internal_error,
         sw::tr("Invalid argument for ") + std::string(arg_daemon_ssl.name));
      ssl_support = static_cast<Wallet::SSLSupport>(ssl_options.support);
    }

    ssl_options.auth = epee::net_utils::ssl_authentication_t{
      std::move(ssl_private_key_path), std::move(ssl_certificate_path)
    };

    {
      const boost::string_ref real_daemon = boost::string_ref{daemon_address}.substr(0, daemon_address.rfind(':'));

      // If SSL or proxy is enabled, then a specific cert, CA or fingerprint must
      // be specified. This is specific to the wallet.
      const bool verification_required =
        ssl_options.verification != epee::net_utils::ssl_verification_t::none &&
        (ssl_options.support == epee::net_utils::ssl_support_t::e_ssl_support_enabled || use_proxy);

      THROW_WALLET_EXCEPTION_IF(
        verification_required && !ssl_options.has_strong_verification(real_daemon),
        tools::error::wallet_internal_error,
        sw::tr("Enabling --") + std::string{use_proxy ? arg_proxy.name : arg_daemon_ssl.name} + sw::tr(" requires --") +
          arg_daemon_ssl_allow_any_cert.name + sw::tr(" or --") +
          arg_daemon_ssl_ca_certificates.name + sw::tr(" or --") + arg_daemon_ssl_allowed_fingerprints.name + sw::tr(" or use of a .onion/.i2p domain")
      );
    }

    if (use_proxy)
    {
      proxy = command_line::get_arg(vm, arg_proxy);
      THROW_WALLET_EXCEPTION_IF(
        !net::get_tcp_endpoint(proxy),
        tools::error::wallet_internal_error,
        std::string{"Invalid address specified for --"} + arg_proxy.name);
    }

    if (!command_line::is_arg_defaulted(vm, arg_trusted_daemon) || !command_line::is_arg_defaulted(vm, arg_untrusted_daemon))
      trusted_daemon = command_line::get_arg(vm, arg_trusted_daemon) && !command_line::get_arg(vm, arg_untrusted_daemon);
    THROW_WALLET_EXCEPTION_IF(!command_line::is_arg_defaulted(vm, arg_trusted_daemon) && !command_line::is_arg_defaulted(vm, arg_untrusted_daemon),
      tools::error::wallet_internal_error, sw::tr("--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted"));

    // set --trusted-daemon if local and not overridden
    if (!trusted_daemon)
    {
      try
      {
        trusted_daemon = false;
        if (tools::is_local_address(daemon_address))
        {
          MINFO(sw::tr("Daemon is local, assuming trusted"));
          trusted_daemon = true;
        }
      }
      catch (const std::exception &e) { }
    }

    return true;
  }

  bool get_fake_outs_count(const std::unique_ptr<Wallet> &w, std::vector<std::string> &local_args, size_t &fake_outs_count)
  {
    const size_t min_ring_size = w->getMinRingSize();
    fake_outs_count = std::max<size_t>(min_ring_size, 1) - 1;
    if(local_args.size() > 0) {
      size_t ring_size;
      if(!epee::string_tools::get_xtype_from_string(ring_size, local_args[0]))
      {
      }
      else if (ring_size == 0 && min_ring_size > 0)
      {
        fail_msg_writer() << tr("Ring size must not be 0");
        return false;
      }
      else if (min_ring_size == 0 && w->getMaxRingSize() == 0)
      {
        if (ring_size != 0)
        {
          fail_msg_writer() << tr("Ring size must be 0");
          return false;
        }
        fake_outs_count = 0;
        local_args.erase(local_args.begin());
        return true;
      }
      else
      {
        fake_outs_count = ring_size - 1;
        local_args.erase(local_args.begin());
      }
    }
    uint64_t adjusted_fake_outs_count = w->adjustMixin(fake_outs_count);
    if (adjusted_fake_outs_count > fake_outs_count)
    {
      fail_msg_writer() << (boost::format(tr("ring size %u is too small, minimum is %u")) % (fake_outs_count+1) % (adjusted_fake_outs_count+1)).str();
      return false;
    }
    if (adjusted_fake_outs_count < fake_outs_count)
    {
      fail_msg_writer() << (boost::format(tr("ring size %u is too large, maximum is %u")) % (fake_outs_count+1) % (adjusted_fake_outs_count+1)).str();
      return false;
    }
    return true;
  }

  bool is_short_payment_id(const std::string &payment_id)
  {
    // 8 bytes = 16 hex chars
    return payment_id.size() == 16 && Wallet::paymentIdValid(payment_id);
  }
  bool is_long_payment_id(const std::string &payment_id)
  {
    // 32 bytes = 64 hex chars
    return payment_id.size() == 64 && Wallet::paymentIdValid(payment_id);
  }

//----------------------------------------------------------------------------------------------------

} // anonymous namespace

namespace
{
  bool check_file_overwrite(const std::string &filename)
  {
    boost::system::error_code errcode;
    if (boost::filesystem::exists(filename, errcode))
    {
      if (boost::ends_with(filename, ".keys"))
      {
        fail_msg_writer() << boost::format(sw::tr("File %s likely stores wallet private keys! Use a different file name.")) % filename;
        return false;
      }
      return command_line::is_yes(input_line((boost::format(sw::tr("File %s already exists. Are you sure to overwrite it?")) % filename).str(), true));
    }
    return true;
  }

  void print_secret_key(const crypto::secret_key &k)
  {
    static constexpr const char hex[] = u8"0123456789abcdef";
    const uint8_t *ptr = (const uint8_t*)k.data;
    for (size_t i = 0, sz = sizeof(k); i < sz; ++i)
    {
      putchar(hex[*ptr >> 4]);
      putchar(hex[*ptr & 15]);
      ++ptr;
    }
  }
}

bool parse_priority(const std::string& arg, std::uint32_t& priority)
{
  const auto priority_optional = tools::fee_priority_utilities::from_string(arg);
  if (!priority_optional.has_value())
    return false;
  priority = tools::fee_priority_utilities::as_integral(priority_optional.value());
  return true;
}

std::string join_priority_strings(const char *delimiter)
{
  std::string s;
  for (size_t n = 0; n < allowed_priority_strings.size(); ++n)
  {
    if (!s.empty())
      s += delimiter;
    s += allowed_priority_strings[n];
  }
  return s;
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
  // don't log
  PAUSE_READLINE();
  if (m_wallet_impl->getDeviceState().key_on_device) {
    std::cout << "secret: On device. Not available" << std::endl;
  } else {
    SCOPED_WALLET_UNLOCK();
    printf("secret: ");
    crypto::secret_key view_key;
    if (!epee::string_tools::hex_to_pod(m_wallet_impl->secretViewKey(), view_key))
        return false;
    print_secret_key(view_key);
    putchar('\n');
  }
  std::cout << "public: " << m_wallet_impl->publicViewKey() << std::endl;

  return true;
}

bool simple_wallet::spendkey(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no spend key");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("has no spend key");
  // don't log
  PAUSE_READLINE();
  if (m_wallet_impl->getDeviceState().key_on_device) {
    std::cout << "secret: On device. Not available" << std::endl;
  } else {
    SCOPED_WALLET_UNLOCK();
    printf("secret: ");
    crypto::secret_key spend_key;
    if (!epee::string_tools::hex_to_pod(m_wallet_impl->secretSpendKey(), spend_key))
        return false;
    print_secret_key(spend_key);
    putchar('\n');
  }
  std::cout << "public: " << m_wallet_impl->publicSpendKey() << std::endl;

  return true;
}

bool simple_wallet::print_seed(bool encrypted)
{
  bool success =  false;
  epee::wipeable_string seed;

  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("has no seed");

  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (ms_status.isMultisig)
  {
    if (!ms_status.isReady)
    {
      fail_msg_writer() << tr("wallet is multisig but not yet finalized");
      return true;
    }
  }

  SCOPED_WALLET_UNLOCK();

  if (!ms_status.isMultisig && !m_wallet_impl->isDeterministic())
  {
    fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
    return true;
  }

  epee::wipeable_string seed_pass;
  if (encrypted)
  {
    auto pwd_container = password_prompter(tr("Enter optional seed offset passphrase, empty to see raw seed"), true);
    if (std::cin.eof() || !pwd_container)
      return true;
    seed_pass = pwd_container->password();
  }

  if (ms_status.isMultisig)
  {
    seed = m_wallet_impl->getMultisigSeed(std::string(seed_pass.data(), seed_pass.size()));
    success = seed != "";
  }
  else if (m_wallet_impl->isDeterministic())
  {
    seed = m_wallet_impl->seed(std::string(seed_pass.data(), seed_pass.size()));
    success = seed != "";
  }

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

bool simple_wallet::restore_height(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  success_msg_writer() << m_wallet_impl->getRefreshFromBlockHeight();
  return true;
}

bool simple_wallet::seed_set_language(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("wallet is multisig and has no seed");
    return true;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and has no seed");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("has no seed");

  epee::wipeable_string password;
  {
    SCOPED_WALLET_UNLOCK();

    if (!m_wallet_impl->isDeterministic())
    {
      fail_msg_writer() << tr("wallet is non-deterministic and has no seed");
      return true;
    }

    // we need the password, even if ask-password is unset
    if (!pwd_container)
    {
      pwd_container = get_and_verify_password();
      if (pwd_container == boost::none)
      {
        fail_msg_writer() << tr("Incorrect password");
        return true;
      }
    }
    password = pwd_container->password();
  }

  std::string mnemonic_language = get_mnemonic_language();
  if (mnemonic_language.empty())
    return true;

  m_wallet_impl->setSeedLanguage(std::move(mnemonic_language));
  m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(password.data(), password.size()));
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

  if (!m_wallet_impl->setPassword(std::string_view(orig_pwd_container->password().data(),
                                                   orig_pwd_container->password().size()),
                                  std::string_view(pwd_container->password().data(),
                                                   pwd_container->password().size())))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("Error with wallet rewrite: ") << error_msg;
  }

  return true;
}

bool simple_wallet::payment_id(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  LONG_PAYMENT_ID_SUPPORT_CHECK();
}

bool simple_wallet::print_fee_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (!try_connect_to_daemon())
    return true;
  int error_code;
  std::string error_msg;
  const bool per_byte = m_wallet_impl->useForkRules(HF_VERSION_PER_BYTE_FEE, 0);
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Error trying to determine hard-fork rules: ") << error_msg;
    return true;
  }

  const uint64_t base_fee = m_wallet_impl->getBaseFee();
  const char *base = per_byte ? "byte" : "kB";
  const uint64_t typical_size = per_byte ? 2500 : 13;
  const uint64_t size_granularity = per_byte ? 1 : 1024;
  message_writer() << (boost::format(tr("Current fee is %s %s per %s")) % print_money(base_fee) % cryptonote::get_unit(cryptonote::get_default_decimal_point()) % base).str();

  std::vector<uint64_t> fees;
  for (const auto priority : tools::fee_priority_utilities::enums)
  {
    if (priority == fee_priority::Default)
      continue;
    uint64_t mult = m_wallet_impl->getFeeMultiplier(tools::fee_priority_utilities::as_integral(priority), static_cast<int>(tools::fee_algorithm::Unset));
    fees.push_back(base_fee * typical_size * mult);
  }
  std::vector<std::pair<uint64_t, uint64_t>> blocks;
  uint64_t base_size = typical_size * size_granularity;
  std::vector<std::pair<double, double>> fee_levels;
  for (uint64_t fee: fees)
  {
    double our_fee_byte_min = fee / (double)base_size, our_fee_byte_max = fee / (double)(base_size + size_granularity);
    fee_levels.emplace_back(our_fee_byte_min, our_fee_byte_max);
  }
  blocks = m_wallet_impl->estimateBacklog(fee_levels);
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Error trying to estimate backlog: ") << error_msg;
    return true;
  }
  if (blocks.size() != 4)
  {
    fail_msg_writer() << tr("Error: bad estimated backlog array size");
    return true;
  }

  for (const auto priority : tools::fee_priority_utilities::enums)
  {
    if (priority == tools::fee_priority::Default)
      continue;

    const auto lower_priority = tools::fee_priority_utilities::decrease(priority);
    const auto lower_priority_index = tools::fee_priority_utilities::as_integral(lower_priority);
    const auto current_priority_index = tools::fee_priority_utilities::as_integral(priority);
    uint64_t nblocks_low = blocks[lower_priority_index].first;
    uint64_t nblocks_high = blocks[lower_priority_index].second;
    if (nblocks_low > 0)
    {
      std::string msg;
      auto default_priority = tools::fee_priority_utilities::from_integral(m_wallet_impl->getDefaultPriority());
      if (priority == default_priority || (default_priority == fee_priority::Default && priority == fee_priority::Normal))
        msg = tr(" (current)");
      uint64_t minutes_low = nblocks_low * DIFFICULTY_TARGET_V2 / 60, minutes_high = nblocks_high * DIFFICULTY_TARGET_V2 / 60;
      if (nblocks_high == nblocks_low)
        message_writer() << (boost::format(tr("%u block (%u minutes) backlog at priority %u%s")) % nblocks_low % minutes_low % current_priority_index % msg).str();
      else
        message_writer() << (boost::format(tr("%u to %u block (%u to %u minutes) backlog at priority %u")) % nblocks_low % nblocks_high % minutes_low % minutes_high % current_priority_index).str();
    }
    else
      message_writer() << tr("No backlog at priority ") << current_priority_index;
  }
  return true;
}

bool simple_wallet::prepare_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return false;
  }
  if (m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("This wallet is already multisig");
    return false;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot be made multisig");
    return false;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot be made multisig");

  if(m_wallet_impl->getWalletState().n_enotes)
  {
    fail_msg_writer() << tr("This wallet has been used before, please use a new wallet to create a multisig wallet");
    return false;
  }

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  int error_code;
  std::string error_msg;
  std::string multisig_info = m_wallet_impl->getMultisigInfo();
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Error trying to get multisig info") << error_msg;
    return false;
  }
  success_msg_writer() << multisig_info;
  success_msg_writer() << tr("Send this multisig info to all other participants, then use make_multisig <threshold> <info1> [<info2>...] with others' multisig info");
  success_msg_writer() << tr("This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet's participants ");

  return true;
}

bool simple_wallet::make_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return false;
  }
  if (m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("This wallet is already multisig");
    return false;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot be made multisig");
    return false;
  }

  if(m_wallet_impl->getWalletState().n_enotes)
  {
    fail_msg_writer() << tr("This wallet has been used before, please use a new wallet to create a multisig wallet");
    return false;
  }

  if (args.size() < 2)
  {
    PRINT_USAGE(USAGE_MAKE_MULTISIG);
    return false;
  }

  // parse threshold
  uint32_t threshold;
  if (!string_tools::get_xtype_from_string(threshold, args[0]))
  {
    fail_msg_writer() << tr("Invalid threshold");
    return false;
  }

  const auto orig_pwd_container = get_and_verify_password();
  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your original password was incorrect.");
    return false;
  }

  LOCK_IDLE_SCOPE();

  auto local_args = args;
  local_args.erase(local_args.begin());
  std::string multisig_extra_info = m_wallet_impl->makeMultisig(local_args, threshold, std::string_view(orig_pwd_container->password().data(), orig_pwd_container->password().size()));
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Error creating multisig: ") << error_msg;
    return false;
  }

  if (!m_wallet_impl->multisig().isReady)
  {
    success_msg_writer() << tr("Another step is needed");
    success_msg_writer() << multisig_extra_info;
    success_msg_writer() << tr("Send this multisig info to all other participants, then use exchange_multisig_keys <info1> [<info2>...] with others' multisig info");
    return true;
  }

  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (!ms_status.isMultisig)
  {
    fail_msg_writer() << tr("Error creating multisig: new wallet is not multisig");
    return false;
  }
  success_msg_writer() << std::to_string(ms_status.threshold) << "/" << ms_status.total << tr(" multisig address: ")
      << m_wallet_impl->address();

  return true;
}

bool simple_wallet::exchange_multisig_keys(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return false;
  }
  if (!ms_status.isMultisig)
  {
    fail_msg_writer() << tr("This wallet is not multisig");
    return false;
  }
  if (ms_status.isReady)
  {
    fail_msg_writer() << tr("This wallet is already finalized");
    return false;
  }

  bool force_update_use_with_caution = false;

  auto local_args = args;
  if (args.size() >= 1 && local_args[0] == "force-update-use-with-caution")
  {
    force_update_use_with_caution = true;
    local_args.erase(local_args.begin());
  }

  const auto orig_pwd_container = get_and_verify_password();
  if(orig_pwd_container == boost::none)
  {
    fail_msg_writer() << tr("Your original password was incorrect.");
    return false;
  }

  std::string multisig_extra_info = m_wallet_impl->exchangeMultisigKeys(args,
                                                                        std::string_view(orig_pwd_container->password().data(),
                                                                                         orig_pwd_container->password().size()),
                                                                        force_update_use_with_caution);
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Failed to perform multisig keys exchange: ") << error_msg;
    return false;
  }

  if (!m_wallet_impl->multisig().isReady)
  {
    message_writer() << tr("Another step is needed");
    message_writer() << multisig_extra_info;
    message_writer() << tr("Send this multisig info to all other participants, then use exchange_multisig_keys <info1> [<info2>...] with others' multisig info");
    return true;
  } else {
    const MultisigState ms_status_new{m_wallet_impl->multisig()};
    success_msg_writer() << tr("Multisig wallet has been successfully created. Current wallet type: ") << ms_status_new.threshold << "/" << ms_status_new.total;
    success_msg_writer() << tr("Multisig address: ") << m_wallet_impl->address();
  }

  return true;
}

bool simple_wallet::export_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  CHECK_MULTISIG_READY();
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_EXPORT_MULTISIG_INFO);
    return false;
  }

  const std::string filename = args[0];
  if (m_wallet_impl->getConfirmExportOverwrite() && !check_file_overwrite(filename))
    return true;

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  std::string ciphertext;
  if (!m_wallet_impl->exportMultisigImages(ciphertext))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("Error exporting multisig info: ") << error_msg;
    return false;
  }
  bool r = save_to_file(filename, ciphertext);
  if (!r)
  {
    fail_msg_writer() << tr("failed to save file ") << filename;
    return false;
  }

  success_msg_writer() << tr("Multisig info exported to ") << filename;
  return true;
}

bool simple_wallet::import_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  CHECK_MULTISIG_READY();
  if (args.size() + 1 < ms_status.threshold)
  {
    PRINT_USAGE(USAGE_IMPORT_MULTISIG_INFO);
    return false;
  }
  int error_code;
  std::string error_msg;

  std::vector<cryptonote::blobdata> info;
  for (size_t n = 0; n < args.size(); ++n)
  {
    const std::string &filename = args[n];
    std::string data;
    bool r = load_from_file(filename, data);
    if (!r)
    {
      fail_msg_writer() << tr("failed to read file ") << filename;
      return false;
    }
    info.push_back(std::move(data));
  }

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  // all read and parsed, actually import
  {
    m_in_manual_refresh.store(true, std::memory_order_relaxed);
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){m_in_manual_refresh.store(false, std::memory_order_relaxed);});
    size_t n_outputs = m_wallet_impl->importMultisigImages(info);

    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
    {
      fail_msg_writer() << tr("Failed to import multisig info: ") << error_msg;
      return false;
    }

    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer() << tr("Multisig info imported. Number of outputs updated: ") << n_outputs;
  }
  if (m_wallet_impl->trustedDaemon())
  {
    if (!m_wallet_impl->rescanSpent())
    {
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
      message_writer() << tr("Failed to update spent status after importing multisig info: ") << error_msg;
      return false;
    }
  }
  else
  {
    message_writer() << tr("Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run \"rescan_spent\"");
    return false;
  }
  return true;
}

bool simple_wallet::sign_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  CHECK_MULTISIG_READY();
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_SIGN_MULTISIG);
    return false;
  }

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  std::string filename = args[0];
  std::vector<std::string> txids;
  uint32_t signers = 0;
  int error_code;
  std::string error_msg;
  std::string multisig_tx_data;
  std::string ciphertext;

  if (!load_from_file(filename, multisig_tx_data))
  {
    fail_msg_writer() << tr("Failed to load multisig tx data from file");
    return false;
  }
  PendingTransaction *ptx = m_wallet_impl->restoreMultisigTransaction(multisig_tx_data, /* ask_for_confirmation */ true);
  if (!ptx)
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return false;
  }
  // Confirm loaded tx
  if (!ptx->confirmationMessage().empty() && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }
  ptx->finishRestoringMultisigTransaction();

  ptx->signMultisigTx(&txids);
  if (!ptx->status())
  {
    fail_msg_writer() << ptx->errorString();
    return false;
  }
  ciphertext = ptx->multisigSignData();
  if (ciphertext.empty())
  {
    fail_msg_writer() << tr("Failed to get multisig sign data") << ptx->errorString();
    return false;
  }
  signers = ptx->signersKeys().size();

  if (txids.empty())
  {
    uint32_t signers_needed = ms_status.threshold - signers - 1;
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
      txids_as_text += txid;
    }
    success_msg_writer(true) << tr("Transaction successfully signed to file ") << filename << ", txid " << txids_as_text;
    success_msg_writer(true) << tr("It may be relayed to the network with submit_multisig");
  }
  return true;
}

bool simple_wallet::submit_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  CHECK_MULTISIG_READY();
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_SUBMIT_MULTISIG);
    return false;
  }

  if (!try_connect_to_daemon())
    return false;

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  std::string filename = args[0];
  int error_code;
  std::string error_msg;
  std::string multisig_tx_data;
  if (load_from_file(filename, multisig_tx_data))
  {
    fail_msg_writer() << tr("Failed to load multisig transaction from file");
    return false;
  }
  PendingTransaction *ptx = m_wallet_impl->restoreMultisigTransaction(multisig_tx_data, /* ask_for_confirmation */ true);
  if (!ptx)
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return false;
  }
  // Confirm loaded tx
  if (!ptx->confirmationMessage().empty() && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }
  ptx->finishRestoringMultisigTransaction();

  std::size_t n_signers = ptx->signersKeys().size();
  if (n_signers < ms_status.threshold)
  {
    fail_msg_writer() << (boost::format(tr("Multisig transaction signed by only %u signers, needs %u more signatures"))
        % n_signers % (ms_status.threshold - n_signers)).str();
    return false;
  }

  // actually commit or save the transactions
  commit_or_save(*ptx, /* do_not_relay */ false);
  if (ptx->status())
  {
    return false;
  }

  return true;
}

bool simple_wallet::export_raw_multisig(const std::vector<std::string> &args)
{
  CHECK_MULTISIG_ENABLED();
  CHECK_MULTISIG_READY();
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_EXPORT_RAW_MULTISIG_TX);
    return true;
  }

  std::string filename = args[0];
  if (m_wallet_impl->getConfirmExportOverwrite() && !check_file_overwrite(filename))
    return true;

  SCOPED_WALLET_UNLOCK();

  int error_code;
  std::string error_msg;
  std::string multisig_tx_data;
  if (load_from_file(filename, multisig_tx_data))
  {
    fail_msg_writer() << tr("Failed to load multisig transaction from file");
    return true;
  }
  PendingTransaction *ptx = m_wallet_impl->restoreMultisigTransaction(multisig_tx_data, /* ask_for_confirmation */ true);
  if (!ptx)
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return true;
  }
  // Confirm loaded tx
  if (!ptx->confirmationMessage().empty() && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }
  ptx->finishRestoringMultisigTransaction();

  std::size_t n_signers = ptx->signersKeys().size();
  if (n_signers < ms_status.threshold)
  {
    fail_msg_writer() << (boost::format(tr("Multisig transaction signed by only %u signers, needs %u more signatures"))
        % n_signers % (ms_status.threshold - n_signers)).str();
    return true;
  }

  // save the transactions
  std::string filenames;
  const std::vector<std::string> tx_ids = ptx->txid();
  const std::vector<std::string> tx_blobs = ptx->convertTxToRawBlobStr();
  CHECK_AND_ASSERT_MES(tx_ids.size() == tx_blobs.size(), true, tr("unequal amount of txids and blobs"));
  for (std::size_t i = 0; i < ptx->txid().size(); ++i)
  {
    const std::string filename = std::string("raw_multisig_monero_tx_") + tx_ids[i];
    if (!filenames.empty())
      filenames += ", ";
    filenames += filename;
    if (!save_to_file(filename, tx_blobs[i]))
    {
      fail_msg_writer() << tr("Failed to export multisig transaction to file ") << filename;
      return true;
    }
  }
  success_msg_writer() << tr("Saved exported multisig transaction file(s): ") << filenames;
  return true;
}

bool simple_wallet::print_ring(const std::vector<std::string> &args)
{
  crypto::key_image key_image;
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_PRINT_RING);
    return true;
  }
  std::string key_image_or_tx_id = args[0];

  // sanity check if provided arg is a 32 byte hex string
  if (!epee::string_tools::hex_to_pod(key_image_or_tx_id, key_image))
  {
    fail_msg_writer() << tr("Invalid key image or txid");
    return true;
  }

  std::vector<uint64_t> ring;
  std::vector<std::pair<std::string, std::vector<uint64_t>>> rings;
  if (m_wallet_impl->getRing(key_image_or_tx_id, ring))
    rings.push_back({key_image_or_tx_id, ring});
  else if (!m_wallet_impl->getRings(key_image_or_tx_id, rings))
  {
    fail_msg_writer() << tr("Key image either not spent, or spent with ring size 1");
    return true;
  }

  for (const auto &ring: rings)
  {
    std::stringstream str;
    for (const auto &x: ring.second)
      str << x<< " ";
    // do NOT translate this "absolute" below, the lin can be used as input to set_ring
    success_msg_writer() << ring.first <<  " absolute " << str.str();
  }

  return true;
}

bool simple_wallet::set_ring(const std::vector<std::string> &args)
{
  crypto::key_image key_image;

  // try filename first
  if (args.size() == 1)
  {
    if (!epee::file_io_utils::is_file_exist(args[0]))
    {
      fail_msg_writer() << tr("File doesn't exist");
      return true;
    }

    char str[4096];
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
        char key_image_str[65], type_str[9];
        int read_after_key_image = 0, read = 0;
        int fields = sscanf(str, "%64[abcdefABCDEF0123456789] %n%8s %n", key_image_str, &read_after_key_image, type_str, &read);
        if (fields != 2)
        {
          fail_msg_writer() << tr("Invalid ring specification: ") << str;
          continue;
        }
        key_image_str[64] = 0;
        type_str[8] = 0;
        if (read_after_key_image == 0 || !epee::string_tools::hex_to_pod(key_image_str, key_image))
        {
          fail_msg_writer() << tr("Invalid key image: ") << str;
          continue;
        }
        if (read == read_after_key_image+8 || (strcmp(type_str, "absolute") && strcmp(type_str, "relative")))
        {
          fail_msg_writer() << tr("Invalid ring type, expected relative or abosolute: ") << str;
          continue;
        }
        bool relative = !strcmp(type_str, "relative");
        if (read < 0 || (size_t)read > strlen(str))
        {
          fail_msg_writer() << tr("Error reading line: ") << str;
          continue;
        }
        bool valid = true;
        std::vector<uint64_t> ring;
        const char *ptr = str + read;
        while (*ptr)
        {
          unsigned long offset;
          int elements = sscanf(ptr, "%lu %n", &offset, &read);
          if (elements == 0 || read <= 0 || (size_t)read > strlen(str))
          {
            fail_msg_writer() << tr("Error reading line: ") << str;
            valid = false;
            break;
          }
          ring.push_back(offset);
          ptr += read;
        }
        if (!valid)
          continue;
        if (ring.empty())
        {
          fail_msg_writer() << tr("Invalid ring: ") << str;
          continue;
        }
        if (relative)
        {
          for (size_t n = 1; n < ring.size(); ++n)
          {
            if (ring[n] <= 0)
            {
              fail_msg_writer() << tr("Invalid relative ring: ") << str;
              valid = false;
              break;
            }
          }
        }
        else
        {
          for (size_t n = 1; n < ring.size(); ++n)
          {
            if (ring[n] <= ring[n-1])
            {
              fail_msg_writer() << tr("Invalid absolute ring: ") << str;
              valid = false;
              break;
            }
          }
        }
        if (!valid)
          continue;
        if (!m_wallet_impl->setRing(key_image_str, ring, relative))
          fail_msg_writer() << tr("Failed to set ring for key image: ") << key_image_str << ". " << tr("Continuing.");
      }
      f.reset();
    }
    return true;
  }

  if (args.size() < 3)
  {
    PRINT_USAGE(USAGE_SET_RING);
    return true;
  }

  std::string key_image_str = args[0];
  if (!epee::string_tools::hex_to_pod(key_image_str, key_image))
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
  if (!m_wallet_impl->setRing(key_image_str, ring, relative))
  {
    fail_msg_writer() << tr("failed to set ring");
    return true;
  }

  return true;
}

bool simple_wallet::freeze_thaw(const std::vector<std::string> &args, bool freeze)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot freeze/thaw");
  if (args.empty())
  {
    fail_msg_writer() << boost::format(tr("usage: %s <key_image>|<pubkey>")) % (freeze ? "freeze" : "thaw");
    return true;
  }
  int error_code;
  std::string error_msg;
  if (freeze)
  {
    m_wallet_impl->freeze(args[0]);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
    {
      m_wallet_impl->freezeByPubKey(args[0]);
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
    }
    if (error_code)
      fail_msg_writer() << tr("failed to freeze. Make sure argument is either a valid key image or output public key.");
  }
  else
  {
    m_wallet_impl->thaw(args[0]);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
    {
      m_wallet_impl->thawByPubKey(args[0]);
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
    }
    if (error_code)
      fail_msg_writer() << tr("failed to thaw. Make sure argument is either a valid key image or output public key.");
  }

  return true;
}

bool simple_wallet::freeze(const std::vector<std::string> &args)
{
  return freeze_thaw(args, true);
}

bool simple_wallet::thaw(const std::vector<std::string> &args)
{
  return freeze_thaw(args, false);
}

bool simple_wallet::frozen(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot see frozen key images");
  if (args.empty())
  {
    size_t ntd = m_wallet_impl->getWalletState().n_enotes;
    std::vector<std::unique_ptr<EnoteDetails>> ed = m_wallet_impl->getEnoteDetails();
    for (size_t i = 0; i < ntd; ++i)
    {
      if (!m_wallet_impl->isFrozen(ed[i]->keyImage()))
        continue;
      message_writer() << tr("Frozen: ") << ed[i]->keyImage() << " " << cryptonote::print_money(ed[i]->amount());
    }
  }
  else
  {
    crypto::key_image ki_or_pubkey;
    if (!epee::string_tools::hex_to_pod(args[0], ki_or_pubkey))
    {
      fail_msg_writer() << tr("failed to parse key image");
      return true;
    }
    std::unique_ptr<EnoteDetails> ed = m_wallet_impl->getEnoteDetails(args[0]);
    if (ed)
    {
      if (ed->isFrozen())
        message_writer() << tr("Frozen: ") << "<"+ed->keyImage()+"> / " << ki_or_pubkey;
      else
        message_writer() << tr("Not frozen: ") << "<"+ed->keyImage()+"> / " << ki_or_pubkey;
    }
    else if (m_wallet_impl->isFrozen(args[0]))
      message_writer() << tr("Frozen: ") << ki_or_pubkey;
    else
    {
      int error_code;
      std::string error_msg;
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
      if (!error_code)
        message_writer() << tr("Not frozen: ") << ki_or_pubkey;
      else
        fail_msg_writer() << tr("failed to parse key image");
    }
  }
  return true;
}

bool simple_wallet::lock(const std::vector<std::string> &args)
{
  m_locked = true;
  check_for_inactivity_lock(true);
  return true;
}

bool simple_wallet::net_stats(const std::vector<std::string> &args)
{
  message_writer() << std::to_string(m_wallet_impl->getBytesSent()) + tr(" bytes sent");
  message_writer() << std::to_string(m_wallet_impl->getBytesReceived()) + tr(" bytes received");
  return true;
}

bool simple_wallet::public_nodes(const std::vector<std::string> &args)
{
  // [host_ip, host_rpc_port, last_seen]
  std::vector<std::tuple<std::string, std::uint16_t, std::uint64_t>> nodes = m_wallet_impl->getPublicNodes(false);
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << error_msg;
    return true;
  }
  if (nodes.empty())
  {
    fail_msg_writer() << tr("No known public nodes");
    return true;
  }

  const uint64_t now = time(NULL);
  message_writer() << boost::format("%32s %16s") % tr("address") % tr("last_seen");
  for (const auto &node: nodes)
  {
    const std::string last_seen = std::get<2>(node) == 0 ? tr("never") : get_human_readable_timespan(std::chrono::seconds(now - std::get<2>(node)));
    std::string host = std::get<0>(node) + ":" + std::to_string(std::get<1>(node));
    message_writer() << boost::format("%32s %16s") % host % last_seen;
  }
  message_writer(console_color_red, true) << tr("Most of these nodes are probably spies. You should not use them unless connecting via Tor or I2P");
  return true;
}

bool simple_wallet::welcome(const std::vector<std::string> &args)
{
  message_writer() << tr("Welcome to Monero, the private cryptocurrency.");
  message_writer() << "";
  message_writer() << tr("Monero, like Bitcoin, is a cryptocurrency. That is, it is digital money.");
  message_writer() << tr("Unlike Bitcoin, your Monero transactions and balance stay private and are not visible to the world by default.");
  message_writer() << tr("However, you have the option of making those available to select parties if you choose to.");
  message_writer() << "";
  message_writer() << tr("Monero protects your privacy on the blockchain, and while Monero strives to improve all the time,");
  message_writer() << tr("no privacy technology can be 100% perfect, Monero included.");
  message_writer() << tr("Monero cannot protect you from malware, and it may not be as effective as we hope against powerful adversaries.");
  message_writer() << tr("Flaws in Monero may be discovered in the future, and attacks may be developed to peek under some");
  message_writer() << tr("of the layers of privacy Monero provides. Be safe and practice defense in depth.");
  message_writer() << "";
  message_writer() << tr("Welcome to Monero and financial privacy. For more information see https://GetMonero.org");
  return true;
}

bool simple_wallet::version(const std::vector<std::string> &args)
{
  message_writer() << "Monero '" << MONERO_RELEASE_NAME << "' (v" << MONERO_VERSION_FULL << ")";
  return true;
}

bool simple_wallet::on_unknown_command(const std::vector<std::string> &args)
{
  if (args[0] == "exit" || args[0] == "q") // backward compat
    return false;
  fail_msg_writer() << boost::format(tr("Unknown command '%s', try 'help'")) % args.front();
  return true;
}

bool simple_wallet::on_empty_command()
{
  return true;
}

bool simple_wallet::on_cancelled_command()
{
  check_for_inactivity_lock(false);
  return true;
}

bool simple_wallet::show_qr_code(const std::vector<std::string> &args)
{
  uint32_t subaddress_index = 0;
  if (args.size() >= 1)
  {
    if (!string_tools::get_xtype_from_string(subaddress_index, args[0]))
    {
      fail_msg_writer() << tr("invalid index: must be an unsigned integer");
      return true;
    }
    if (subaddress_index >= m_wallet_impl->numSubaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("<subaddress_index> is out of bounds");
      return true;
    }
  }

#ifdef _WIN32
#define PRINT_UTF8(pre, x) std::wcout << pre ## x
#define WTEXTON() _setmode(_fileno(stdout), _O_WTEXT)
#define WTEXTOFF() _setmode(_fileno(stdout), _O_TEXT)
#else
#define PRINT_UTF8(pre, x) std::cout << x
#define WTEXTON()
#define WTEXTOFF()
#endif

  WTEXTON();
  try
  {
    const std::string address = "monero:" + m_wallet_impl->address(m_current_subaddress_account, subaddress_index);
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(address.c_str(), qrcodegen::QrCode::Ecc::LOW);
    for (int y = -2; y < qr.getSize() + 2; y+=2)
    {
      for (int x = -2; x < qr.getSize() + 2; x++)
      {
        if (qr.getModule(x, y) && qr.getModule(x, y + 1))
          PRINT_UTF8(L, "\u2588");
        else if (qr.getModule(x, y) && !qr.getModule(x, y + 1))
          PRINT_UTF8(L, "\u2580");
        else if (!qr.getModule(x, y) && qr.getModule(x, y + 1))
          PRINT_UTF8(L, "\u2584");
        else
          PRINT_UTF8(L, " ");
      }
      PRINT_UTF8(, std::endl);
    }
  }
  catch (const std::length_error&)
  {
    fail_msg_writer() << tr("Failed to generate QR code, input too large");
  }
  WTEXTOFF();
  return true;
}

bool simple_wallet::set_always_confirm_transfers(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setAlwaysConfirmTransfers(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
      m_wallet_impl->setPrintRingMembers(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::set_store_tx_info(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot transfer");
    return true;
  }
 
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setStoreTxInfo(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::set_default_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  uint32_t priority = 0;
  try
  {
    if (strchr(args[1].c_str(), '-'))
    {
      fail_msg_writer() << tr("priority must be either 0, 1, 2, 3, or 4, or one of: ") << join_priority_strings(", ");
      return true;
    }
    if (args[1] == "0")
    {
      priority = 0;
    }
    else
    {
      bool found = false;
      for (size_t n = 0; n < allowed_priority_strings.size(); ++n)
      {
        if (allowed_priority_strings[n] == args[1])
        {
          found = true;
          priority = n;
        }
      }
      if (!found)
      {
        priority = boost::lexical_cast<int>(args[1]);
        if (priority < tools::fee_priority_utilities::as_integral(fee_priority::Unimportant) || priority > tools::fee_priority_utilities::as_integral(fee_priority::Priority))
        {
          fail_msg_writer() << tr("priority must be either 0, 1, 2, 3, or 4, or one of: ") << join_priority_strings(", ");
          return true;
        }
      }
    }
 
    const auto pwd_container = get_and_verify_password();
    if (pwd_container)
    {
      m_wallet_impl->setDefaultPriority(priority);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    }
    return true;
  }
  catch(const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("priority must be either 0, 1, 2, 3, or 4, or one of: ") << join_priority_strings(", ");
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
      m_auto_refresh_enabled.store(false, std::memory_order_relaxed);
      m_wallet_impl->setAutoRefresh(auto_refresh);
      m_idle_mutex.lock();
      m_auto_refresh_enabled.store(auto_refresh, std::memory_order_relaxed);
      m_idle_cond.notify_one();
      m_idle_mutex.unlock();

      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::set_refresh_type(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  Wallet::RefreshType refresh_type;
  if (!parse_refresh_type(args[1], refresh_type))
  {
    return true;
  }
 
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet_impl->setRefreshType(refresh_type);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_ask_password(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    Wallet::AskPasswordType ask = Wallet::AskPasswordType::AskPassword_ToDecrypt;
    if (args[1] == "never" || args[1] == "0")
      ask = Wallet::AskPasswordType::AskPassword_Never;
    else if (args[1] == "action" || args[1] == "1")
      ask = Wallet::AskPasswordType::AskPassword_OnAction;
    else if (args[1] == "encrypt" || args[1] == "decrypt" || args[1] == "2")
      ask = Wallet::AskPasswordType::AskPassword_ToDecrypt;
    else
    {
      fail_msg_writer() << tr("invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt");
      return true;
    }

    const Wallet::AskPasswordType cur_ask = m_wallet_impl->getAskPasswordType();
    if (!m_wallet_impl->watchOnly())
    {
      if (cur_ask == Wallet::AskPasswordType::AskPassword_ToDecrypt && ask != Wallet::AskPasswordType::AskPassword_ToDecrypt)
        m_wallet_impl->decryptKeys(std::string_view(pwd_container->password().data(), pwd_container->password().size()));
      else if (cur_ask != Wallet::AskPasswordType::AskPassword_ToDecrypt && ask == Wallet::AskPasswordType::AskPassword_ToDecrypt)
        m_wallet_impl->encryptKeys(std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    }
    m_wallet_impl->setAskPasswordType(ask);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_max_reorg_depth(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  uint64_t depth;
  if (!epee::string_tools::get_xtype_from_string(depth, args[1]))
  {
    fail_msg_writer() << tr("invalid value");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet_impl->setMaxReorgDepth(depth);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->setMinOutputCount(count);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->setMinOutputValue(value);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_merge_destinations(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setMergeDestinations(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
      m_wallet_impl->setConfirmBacklog(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->setConfirmBacklogThreshold(threshold);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_confirm_export_overwrite(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setConfirmExportOverwrite(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->setRefreshFromBlockHeight(height);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_auto_low_priority(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setAutoLowPriority(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
      m_wallet_impl->setSegregatePreForkOutputs(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
      m_wallet_impl->setKeyReuseMitigation2(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
      m_wallet_impl->setSubaddressLookahead(lookahead->first, lookahead->second);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
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
    m_wallet_impl->setSegregationHeight(height);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_ignore_fractional_outputs(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setIgnoreFractionalOutputs(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}


bool simple_wallet::set_ignore_outputs_above(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    uint64_t amount;
    if (!cryptonote::parse_amount(amount, args[1]))
    {
      fail_msg_writer() << tr("Invalid amount");
      return true;
    }
    if (amount == 0)
      amount = MONEY_SUPPLY;
    m_wallet_impl->setIgnoreOutputsAbove(amount);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_ignore_outputs_below(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    uint64_t amount;
    if (!cryptonote::parse_amount(amount, args[1]))
    {
      fail_msg_writer() << tr("Invalid amount");
      return true;
    }
    m_wallet_impl->setIgnoreOutputsBelow(amount);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_track_uses(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setTrackUses(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::setup_background_sync(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("background sync not implemented for multisig wallet");
    return true;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("background sync not implemented for watch only wallet");
    return true;
  }
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }

  Wallet::BackgroundSyncType background_sync_type;
  if (!parse_background_sync_type(args[1], background_sync_type))
  {
    fail_msg_writer() << tr("invalid option");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (!pwd_container)
    return true;

  std::string wallet_password = std::string(pwd_container->password().data(), pwd_container->password().size());
  Monero::optional<std::string> background_cache_password = Monero::optional<std::string>();
  epee::misc_utils::auto_scope_leave_caller bgc_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    memwipe(&wallet_password[0], wallet_password.size());
    if (background_cache_password)
      memwipe(&(*background_cache_password)[0], (*background_cache_password).size());
  });
  if (background_sync_type == Wallet::BackgroundSync_CustomPassword)
  {
    const auto background_pwd_container = background_sync_cache_password_prompter(true);
    if (!background_pwd_container)
      return true;
    background_cache_password = std::string(background_pwd_container->password().data(), background_pwd_container->password().size());
  }

  LOCK_IDLE_SCOPE();
  m_wallet_impl->setupBackgroundSync(background_sync_type, wallet_password, background_cache_password);

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Error setting background sync type: ") << error_code;
    return true;
  }

  return true;
}

bool simple_wallet::set_show_wallet_name_when_locked(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setShowWalletNameWhenLocked(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::set_inactivity_lock_timeout(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
#ifdef _WIN32
  tools::fail_msg_writer() << tr("Inactivity lock timeout disabled on Windows");
  return true;
#endif
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    uint32_t r;
    if (epee::string_tools::get_xtype_from_string(r, args[1]))
    {
      m_wallet_impl->setInactivityLockTimeout(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    }
    else
    {
      tools::fail_msg_writer() << tr("Invalid number of seconds");
    }
  }
  return true;
}

bool simple_wallet::set_setup_background_mining(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    Wallet::BackgroundMiningSetupType setup = Wallet::BackgroundMiningSetupType::BackgroundMining_No;
    if (args[1] == "yes" || args[1] == "1")
      setup = Wallet::BackgroundMiningSetupType::BackgroundMining_Yes;
    else if (args[1] == "no" || args[1] == "0")
      setup = Wallet::BackgroundMiningSetupType::BackgroundMining_No;
    else
    {
      fail_msg_writer() << tr("invalid argument: must be either 1/yes or 0/no");
      return true;
    }
    m_wallet_impl->setSetupBackgroundMining(setup);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_Yes)
      start_background_mining();
    else
      stop_background_mining();
  }
  return true;
}

bool simple_wallet::set_device_name(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    if (args.size() == 0){
      fail_msg_writer() << tr("Device name not specified");
      return true;
    }

    m_wallet_impl->setDeviceName(args[1]);
    if (!m_wallet_impl->reconnectDevice()){
      fail_msg_writer() << tr("Device reconnect failed");
      int error_code;
      std::string error_msg;
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
      if (error_code)
          fail_msg_writer() << error_msg;
    }
    else {
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    }
  }
  return true;
}

bool simple_wallet::set_export_format(const std::vector<std::string> &args/* = std::vector<std::string()*/)
{
  if (args.size() < 2)
  {
    fail_msg_writer() << tr("Export format not specified");
    return true;
  }

  if (boost::algorithm::iequals(args[1], "ascii"))
  {
    m_wallet_impl->setExportFormat(Wallet::ExportFormat::ExportFormat_Ascii);
  }
  else if (boost::algorithm::iequals(args[1], "binary"))
  {
    m_wallet_impl->setExportFormat(Wallet::ExportFormat::ExportFormat_Binary);
  }
  else
  {
    fail_msg_writer() << tr("Export format not recognized.");
    return true;
  }
  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
  }
  return true;
}

bool simple_wallet::set_load_deprecated_formats(const std::vector<std::string> &args/* = std::vector<std::string()*/)
{
  if (args.size() < 2)
  {
    fail_msg_writer() << tr("Value not specified");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      if (r)
        fail_msg_writer() << tr("Warning: deprecated formats use boost serialization, which has buffer overflows and crashes. Support for them has been discontinued.");
    });
  }
  return true;
}

bool simple_wallet::set_enable_multisig(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (args.size() < 2)
  {
    fail_msg_writer() << tr("Value not specified");
    return true;
  }

  const auto pwd_container = get_and_verify_password();
  if (pwd_container)
  {
    parse_bool_and_use(args[1], [&](bool r) {
      m_wallet_impl->setEnableMultisig(r);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd_container->password().data(), pwd_container->password().size()));
    });
  }
  return true;
}

bool simple_wallet::help(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if(args.empty())
  {
    message_writer() << "";
    message_writer() << tr("Important commands:");
    message_writer() << "";
    message_writer() << tr("\"welcome\" - Show welcome message.");
    message_writer() << tr("\"help all\" - Show the list of all available commands.");
    message_writer() << tr("\"help <command>\" - Show a command's documentation.");
    message_writer() << tr("\"apropos <keyword>\" - Show commands related to a keyword.");
    message_writer() << "";
    message_writer() << tr("\"wallet_info\" - Show wallet main address and other info.");
    message_writer() << tr("\"balance\" - Show balance.");
    message_writer() << tr("\"address all\" - Show all addresses.");
    message_writer() << tr("\"address new\" - Create new subaddress.");
    message_writer() << tr("\"transfer <address> <amount>\" - Send XMR to an address.");
    message_writer() << tr("\"show_transfers [in|out|pending|failed|pool]\" - Show transactions.");
    message_writer() << tr("\"sweep_all <address>\" - Send whole balance to another wallet.");
    message_writer() << tr("\"seed\" - Show secret 25 words that can be used to recover this wallet.");
    message_writer() << tr("\"refresh\" - Synchronize wallet with the Monero network.");
    message_writer() << tr("\"status\" - Check current status of wallet.");
    message_writer() << tr("\"version\" - Check software version.");
    message_writer() << tr("\"exit\" - Exit wallet.");
    message_writer() << "";
    message_writer() << tr("\"donate <amount>\" - Donate XMR to the development team.");
    message_writer() << "";
  }
  else if ((args.size() == 1) && (args.front() == "all"))
  {
    success_msg_writer() << get_commands_str();
  }
  else
  {
    success_msg_writer() << get_command_usage(args);
  }
  return true;
}

bool simple_wallet::apropos(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    PRINT_USAGE(USAGE_APROPOS);
    return true;
  }
  const std::vector<std::string>& command_list = m_cmd_binder.get_command_list(args);
  if (command_list.empty())
  {
    fail_msg_writer() << tr("No commands found mentioning keyword(s)");
    return true;
  }

  success_msg_writer() << "";
  for(auto const& command:command_list)
  {
    std::vector<std::string> cmd;
    cmd.push_back(command);
    std::pair<std::string, std::string> documentation = m_cmd_binder.get_documentation(cmd);
    success_msg_writer() << "  " << documentation.first;
  }
  success_msg_writer() << "";

  return true;
}

bool simple_wallet::scan_tx(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot scan tx");
  if (args.empty())
  {
    PRINT_USAGE(USAGE_SCAN_TX);
    return true;
  }

  if (!m_wallet_impl->trustedDaemon()) {
    message_writer(console_color_red, true) << tr("WARNING: this operation may reveal the txids to the remote node and affect your privacy");
    if (!command_line::is_yes(input_line("Do you want to continue?", true))) {
      message_writer() << tr("You have canceled the operation");
      return true;
    }
  }

  LOCK_IDLE_SCOPE();
  m_in_manual_refresh.store(true);
  bool wont_reprocess_recent_txs_via_untrusted_daemon = false;
  m_wallet_impl->scanTransactions(args, &wont_reprocess_recent_txs_via_untrusted_daemon);
  int error;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error, error_msg);
  if (error)
  {
    fail_msg_writer() << error_msg << (wont_reprocess_recent_txs_via_untrusted_daemon
                                       ? ". Either connect to a trusted daemon by passing --trusted-daemon when starting the wallet, or use rescan_bc to rescan the chain."
                                       : "");
  }
  m_in_manual_refresh.store(false);
  return true;
}

simple_wallet::simple_wallet()
  : m_refresh_progress_reporter(*this)
  , m_idle_run(true)
  , m_auto_refresh_enabled(false)
  , m_auto_refresh_refreshing(false)
  , m_in_manual_refresh(false)
  , m_current_subaddress_account(0)
  , m_last_activity_time(time(NULL))
  , m_locked(false)
  , m_in_command(false)
{
  m_cmd_binder.set_handler("start_mining",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::start_mining, _1),
                           tr(USAGE_START_MINING),
                           tr("Start mining in the daemon (bg_mining and ignore_battery are optional booleans)."));
  m_cmd_binder.set_handler("stop_mining",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::stop_mining, _1),
                           tr("Stop mining in the daemon."));
  m_cmd_binder.set_handler("set_daemon",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_daemon, _1),
                           tr(USAGE_SET_DAEMON),
                           tr("Set another daemon to connect to. If it's not yours, it'll probably spy on you."));
  m_cmd_binder.set_handler("save_bc",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::save_bc, _1),
                           tr("Save the current blockchain data."));
  m_cmd_binder.set_handler("refresh",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::refresh, _1),
                           tr("Synchronize the transactions and balance."));
  m_cmd_binder.set_handler("balance",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_balance, _1),
                           tr(USAGE_SHOW_BALANCE),
                           tr("Show the wallet's balance of the currently selected account."));
  m_cmd_binder.set_handler("incoming_transfers",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_incoming_transfers,_1),
                           tr(USAGE_INCOMING_TRANSFERS),
                           tr("Show the incoming transfers, all or filtered by availability and address index.\n\n"
                              "Output format:\n"
                              "Amount, Spent(\"T\"|\"F\"), \"frozen\"|\"locked\"|\"unlocked\", RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] "));
  m_cmd_binder.set_handler("payments",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_payments,_1),
                           tr(USAGE_PAYMENTS),
                           tr("Show the payments for the given payment IDs."));
  m_cmd_binder.set_handler("bc_height",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_blockchain_height, _1),
                           tr("Show the blockchain height."));
  m_cmd_binder.set_handler("transfer", boost::bind(&simple_wallet::on_command, this, &simple_wallet::transfer, _1),
                           tr(USAGE_TRANSFER),
                           tr("Transfer <address> <amount>. If the parameter \"index=<N1>[,<N2>,...]\" is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. <priority> is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command \"set priority\") is used. <ring_size> is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or <address_2> <amount_2> etcetera. The \"subtractfeefrom=\" list allows you to choose which destinations to fund the tx fee from instead of the change output. The fee will be split across the chosen destinations proportionally equally. For example, to make 3 transfers where the fee is taken from the first and third destinations, one could do: \"transfer <addr1> 3 <addr2> 0.5 <addr3> 1 subtractfeefrom=0,2\". Let's say the tx fee is 0.1. The balance would drop by exactly 4.5 XMR including fees, and addr1 & addr3 would receive 2.925 & 0.975 XMR, respectively. Use \"subtractfeefrom=all\" to spread the fee across all destinations."));
  m_cmd_binder.set_handler("sweep_all", boost::bind(&simple_wallet::on_command, this, &simple_wallet::sweep_all, _1),
                           tr(USAGE_SWEEP_ALL),
                           tr("Send all unlocked balance to an address. If the parameter \"index=<N1>[,<N2>,...]\" or \"index=all\" is specified, the wallet sweeps outputs received by those or all address indices, respectively. If omitted, the wallet randomly chooses an address index to be used. If the parameter \"outputs=<N>\" is specified and  N > 0, wallet splits the transaction into N even outputs."));
  m_cmd_binder.set_handler("sweep_account", boost::bind(&simple_wallet::on_command, this, &simple_wallet::sweep_account, _1),
                           tr(USAGE_SWEEP_ACCOUNT),
                           tr("Send all unlocked balance from a given account to an address. If the parameter \"index=<N1>[,<N2>,...]\" or \"index=all\" is specified, the wallet sweeps outputs received by those or all address indices, respectively. If omitted, the wallet randomly chooses an address index to be used. If the parameter \"outputs=<N>\" is specified and  N > 0, wallet splits the transaction into N even outputs."));
  m_cmd_binder.set_handler("sweep_below",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::sweep_below, _1),
                           tr(USAGE_SWEEP_BELOW),
                           tr("Send all unlocked outputs below the threshold to an address."));
  m_cmd_binder.set_handler("sweep_single",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::sweep_single, _1),
                           tr(USAGE_SWEEP_SINGLE),
                           tr("Send a single output of the given key image to an address without change."));
  m_cmd_binder.set_handler("donate",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::donate, _1),
                           tr(USAGE_DONATE),
                           tr("Donate <amount> to the development team (donate.getmonero.org)."));
  m_cmd_binder.set_handler("sign_transfer",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::sign_transfer, _1),
                           tr(USAGE_SIGN_TRANSFER),
                           tr("Sign a transaction from a file. If the parameter \"export_raw\" is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.\n"
                              "Use the parameter <filename> to specify the file to read from. If not specified, the default \"unsigned_monero_tx\" will be used."));
  m_cmd_binder.set_handler("submit_transfer",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::submit_transfer, _1),
                           tr("Submit a signed transaction from a file."));
  m_cmd_binder.set_handler("set_log",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_log, _1),
                           tr(USAGE_SET_LOG),
                           tr("Change the current log detail (level must be <0-4>)."));
  m_cmd_binder.set_handler("account",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::account, _1),
                           tr(USAGE_ACCOUNT),
                           tr("If no arguments are specified, the wallet shows all the existing accounts along with their balances.\n"
                              "If the \"new\" argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).\n"
                              "If the \"switch\" argument is specified, the wallet switches to the account specified by <index>.\n"
                              "If the \"label\" argument is specified, the wallet sets the label of the account specified by <index> to the provided label text.\n"
                              "If the \"tag\" argument is specified, a tag <tag_name> is assigned to the specified accounts <account_index_1>, <account_index_2>, ....\n"
                              "If the \"untag\" argument is specified, the tags assigned to the specified accounts <account_index_1>, <account_index_2> ..., are removed.\n"
                              "If the \"tag_description\" argument is specified, the tag <tag_name> is assigned an arbitrary text <description>."));
  m_cmd_binder.set_handler("address",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::print_address, _1),
                           tr(USAGE_ADDRESS),
                           tr("If no arguments are specified or <index> is specified, the wallet shows the default or specified address. If \"all\" is specified, the wallet shows all the existing addresses in the currently selected account. If \"new \" is specified, the wallet creates a new address with the provided label text (which can be empty). If \"mnew\" is specified, the wallet creates as many new addresses as specified by the argument; the default label is set for the new addresses. If \"label\" is specified, the wallet sets the label of the address specified by <index> to the provided label text. If \"one-off\" is specified, the address for the specified index is generated and displayed, and remembered by the wallet"));
  m_cmd_binder.set_handler("integrated_address",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::print_integrated_address, _1),
                           tr(USAGE_INTEGRATED_ADDRESS),
                           tr("Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID"));
  m_cmd_binder.set_handler("address_book",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::address_book,_1),
                           tr(USAGE_ADDRESS_BOOK),
                           tr("Print all entries in the address book, optionally adding/deleting an entry to/from it."));
  m_cmd_binder.set_handler("save",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::save, _1),
                           tr("Save the wallet data."));
  m_cmd_binder.set_handler("save_watch_only",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::save_watch_only, _1),
                           tr("Save a watch-only keys file."));
  m_cmd_binder.set_handler("viewkey",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::viewkey, _1),
                           tr("Display the private view key."));
  m_cmd_binder.set_handler("spendkey",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::spendkey, _1),
                           tr("Display the private spend key."));
  m_cmd_binder.set_handler("seed",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::seed, _1),
                           tr("Display the Electrum-style mnemonic seed"));
  m_cmd_binder.set_handler("restore_height",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::restore_height, _1),
                           tr("Display the restore height"));
  m_cmd_binder.set_handler("set",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_variable, _1),
                           tr(USAGE_SET_VARIABLE),
                           tr("Available options:\n "
                                  "seed language\n "
                                  "  Set the wallet's seed language.\n "
                                  "always-confirm-transfers <1|0>\n "
                                  "  Whether to confirm unsplit txes.\n "
                                  "print-ring-members <1|0>\n "
                                  "  Whether to print detailed information about ring members during confirmation.\n "
                                  "store-tx-info <1|0>\n "
                                  "  Whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference.\n "
                                  "auto-refresh <1|0>\n "
                                  "  Whether to automatically synchronize new blocks from the daemon.\n "
                                  "refresh-type <full|optimize-coinbase|no-coinbase|default>\n "
                                  "  Set the wallet's refresh behaviour.\n "
                                  "priority [0|1|2|3|4]\n "
                                  "  Set the fee to default/unimportant/normal/elevated/priority.\n "
                                  "ask-password <0|1|2   (or never|action|decrypt)>\n "
                                  "  action: ask the password before many actions such as transfer, etc\n "
                                  "  decrypt: same as action, but keeps the spend key encrypted in memory when not needed\n "
                                  "unit <monero|millinero|micronero|nanonero|piconero>\n "
                                  "  Set the default monero (sub-)unit.\n "
                                  "max-reorg-depth <unsigned int>\n "
                                  "  Set the maximum amount of blocks to accept in a reorg.\n "
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
                                  "confirm-export-overwrite <1|0>\n "
                                  "  Whether to warn if the file to be exported already exists.\n "
                                  "refresh-from-block-height [n]\n "
                                  "  Set the height before which to ignore blocks.\n "
                                  "auto-low-priority <1|0>\n "
                                  "  Whether to automatically use the low priority fee level when it's safe to do so.\n "
                                  "segregate-pre-fork-outputs <1|0>\n "
                                  "  Set this if you intend to spend outputs on both Monero AND a key reusing fork.\n "
                                  "key-reuse-mitigation2 <1|0>\n "
                                  "  Set this if you are not sure whether you will spend on a key reusing Monero fork later.\n "
                                  "subaddress-lookahead <major>:<minor>\n "
                                  "  Set the lookahead sizes for the subaddress hash table.\n "
                                  "segregation-height <n>\n "
                                  "  Set to the height of a key reusing fork you want to use, 0 to use default.\n "
                                  "ignore-fractional-outputs <1|0>\n "
                                  "  Whether to ignore fractional outputs that result in net loss when spending due to fee.\n "
                                  "ignore-outputs-above <amount>\n "
                                  "  Ignore outputs of amount above this threshold when spending. Value 0 is translated to the maximum value (18 million) which disables this filter.\n "
                                  "ignore-outputs-below <amount>\n "
                                  "  Ignore outputs of amount below this threshold when spending.\n "
                                  "track-uses <1|0>\n "
                                  "  Whether to keep track of owned outputs uses.\n "
                                  "background-sync <off|reuse-wallet-password|custom-background-password>\n "
                                  "  Set this to enable scanning in the background with just the view key while the wallet is locked.\n "
                                  "setup-background-mining <1|0>\n "
                                  "  Whether to enable background mining. Set this to support the network and to get a chance to receive new monero.\n "
                                  "device-name <device_name[:device_spec]>\n "
                                  "  Device name for hardware wallet.\n "
                                  "export-format <\"binary\"|\"ascii\">\n "
                                  "  Save all exported files as binary (cannot be copied and pasted) or ascii (can be).\n "
                                  "load-deprecated-formats <1|0>\n "
                                  "  Whether to enable importing data in deprecated formats.\n "
                                  "show-wallet-name-when-locked <1|0>\n "
                                  "  Set this if you would like to display the wallet name when locked.\n "
                                  "enable-multisig-experimental <1|0>\n "
                                  "  Set this to allow multisig commands. Multisig may currently be exploitable if parties do not trust each other.\n "
                                  "inactivity-lock-timeout <unsigned int>\n "
                                  "  How many seconds to wait before locking the wallet (0 to disable)."));
  m_cmd_binder.set_handler("encrypted_seed",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::encrypted_seed, _1),
                           tr("Display the encrypted Electrum-style mnemonic seed."));
  m_cmd_binder.set_handler("rescan_spent",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::rescan_spent, _1),
                           tr("Rescan the blockchain for spent outputs."));
  m_cmd_binder.set_handler("get_tx_key",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_tx_key, _1),
                           tr(USAGE_GET_TX_KEY),
                           tr("Get the transaction key (r) for a given <txid>."));
  m_cmd_binder.set_handler("set_tx_key",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_tx_key, _1),
                           tr(USAGE_SET_TX_KEY),
                           tr("Set the transaction key (r) for a given <txid> in case the tx was made by some other device or 3rd party wallet."));
  m_cmd_binder.set_handler("check_tx_key",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::check_tx_key, _1),
                           tr(USAGE_CHECK_TX_KEY),
                           tr("Check the amount going to <address> in <txid>."));
  m_cmd_binder.set_handler("get_tx_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_tx_proof, _1),
                           tr(USAGE_GET_TX_PROOF),
                           tr("Generate a signature proving funds sent to <address> in <txid>, optionally with a challenge string <message>, using either the transaction secret key (when <address> is not your wallet's address) or the view secret key (otherwise), which does not disclose the secret key."));
  m_cmd_binder.set_handler("check_tx_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::check_tx_proof, _1),
                           tr(USAGE_CHECK_TX_PROOF),
                           tr("Check the proof for funds going to <address> in <txid> with the challenge string <message> if any. (When using <signature_file> make sure the file name does NOT start with `InProof` or `OutProof`.)"));
  m_cmd_binder.set_handler("get_spend_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_spend_proof, _1),
                           tr(USAGE_GET_SPEND_PROOF),
                           tr("Generate a signature proving that you generated <txid> using the spend secret key, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("check_spend_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::check_spend_proof, _1),
                           tr(USAGE_CHECK_SPEND_PROOF),
                           tr("Check a signature proving that the signer generated <txid>, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("get_reserve_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_reserve_proof, _1),
                           tr(USAGE_GET_RESERVE_PROOF),
                           tr("Generate a signature proving that you own at least this much, optionally with a challenge string <message>.\n"
                              "If 'all' is specified, you prove the entire sum of all of your existing accounts' balances.\n"
                              "Otherwise, you prove the reserve of the smallest possible amount above <amount> available in your current account."));
  m_cmd_binder.set_handler("check_reserve_proof",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::check_reserve_proof, _1),
                           tr(USAGE_CHECK_RESERVE_PROOF),
                           tr("Check a signature proving that the owner of <address> holds at least this much, optionally with a challenge string <message>."));
  m_cmd_binder.set_handler("show_transfers",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_transfers, _1),
                           tr(USAGE_SHOW_TRANSFERS),
                           // Seemingly broken formatting to compensate for the backslash before the quotes.
                           tr("Show the incoming/outgoing transfers within an optional height range.\n\n"
                              "Output format:\n"
                              "In or Coinbase:    Block Number, \"block\"|\"in\",              Time, Amount,  Transaction Hash, Payment ID, Subaddress Index,                     \"-\", Note\n"
                              "Out:               Block Number, \"out\",                     Time, Amount*, Transaction Hash, Payment ID, Fee, Destinations, Input addresses**, \"-\", Note\n"
                              "Pool:                            \"pool\", \"in\",              Time, Amount,  Transaction Hash, Payment Id, Subaddress Index,                     \"-\", Note, Double Spend Note\n"
                              "Pending or Failed:               \"failed\"|\"pending\", \"out\", Time, Amount*, Transaction Hash, Payment ID, Fee, Input addresses**,               \"-\", Note\n\n"
                              "* Excluding change and fee.\n"
                              "** Set of address indices used as inputs in this transfer."));
  m_cmd_binder.set_handler("export_transfers",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::export_transfers, _1),
                           tr("export_transfers [in|out|all|pending|failed|pool|coinbase] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]] [output=<filepath>] [option=<with_keys>]"),
                           tr("Export to CSV the incoming/outgoing transfers within an optional height range."));
  m_cmd_binder.set_handler("unspent_outputs",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::unspent_outputs, _1),
                           tr(USAGE_UNSPENT_OUTPUTS),
                           tr("Show the unspent outputs of a specified address within an optional amount range."));
  m_cmd_binder.set_handler("rescan_bc",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::rescan_blockchain, _1),
                           tr(USAGE_RESCAN_BC),
                           tr("Rescan the blockchain from scratch. If \"hard\" is specified, you will lose any information which can not be recovered from the blockchain itself."));
  m_cmd_binder.set_handler("set_tx_note",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_tx_note, _1),
                           tr(USAGE_SET_TX_NOTE),
                           tr("Set an arbitrary string note for a <txid>."));
  m_cmd_binder.set_handler("get_tx_note",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_tx_note, _1),
                           tr(USAGE_GET_TX_NOTE),
                           tr("Get a string note for a txid."));
  m_cmd_binder.set_handler("set_description",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_description, _1),
                           tr(USAGE_SET_DESCRIPTION),
                           tr("Set an arbitrary description for the wallet."));
  m_cmd_binder.set_handler("get_description",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::get_description, _1),
                           tr(USAGE_GET_DESCRIPTION),
                           tr("Get the description of the wallet."));
  m_cmd_binder.set_handler("status",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::status, _1),
                           tr("Show the wallet's status."));
  m_cmd_binder.set_handler("wallet_info",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::wallet_info, _1),
                           tr("Show the wallet's information."));
  m_cmd_binder.set_handler("sign",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::sign, _1),
                           tr(USAGE_SIGN),
                           tr("Sign the contents of a file with the given subaddress (or the main address if not specified)"));
  m_cmd_binder.set_handler("verify",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::verify, _1),
                           tr(USAGE_VERIFY),
                           tr("Verify a signature on the contents of a file."));
  m_cmd_binder.set_handler("export_key_images",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::export_key_images, _1),
                           tr(USAGE_EXPORT_KEY_IMAGES),
                           tr("Export a signed set of key images to a <filename>."));
  m_cmd_binder.set_handler("import_key_images",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::import_key_images, _1),
                           tr(USAGE_IMPORT_KEY_IMAGES),
                           tr("Import a signed key images list and verify their spent status."));
  m_cmd_binder.set_handler("hw_key_images_sync",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::hw_key_images_sync, _1),
                           tr(USAGE_HW_KEY_IMAGES_SYNC),
                           tr("Synchronizes key images with the hw wallet."));
  m_cmd_binder.set_handler("hw_reconnect",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::hw_reconnect, _1),
                           tr(USAGE_HW_RECONNECT),
                           tr("Attempts to reconnect HW wallet."));
  m_cmd_binder.set_handler("export_outputs",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::export_outputs, _1),
                           tr(USAGE_EXPORT_OUTPUTS),
                           tr("Export a set of outputs owned by this wallet."));
  m_cmd_binder.set_handler("import_outputs",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::import_outputs, _1),
                           tr(USAGE_IMPORT_OUTPUTS),
                           tr("Import a set of outputs owned by this wallet."));
  m_cmd_binder.set_handler("show_transfer",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_transfer, _1),
                           tr(USAGE_SHOW_TRANSFER),
                           tr("Show information about a transfer to/from this address."));
  m_cmd_binder.set_handler("password",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::change_password, _1),
                           tr("Change the wallet's password."));
  m_cmd_binder.set_handler("payment_id",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::payment_id, _1),
                           tr(USAGE_PAYMENT_ID),
                           tr("Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids."));
  m_cmd_binder.set_handler("fee",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::print_fee_info, _1),
                           tr("Print the information about the current fee and transaction backlog."));
  m_cmd_binder.set_handler("prepare_multisig", boost::bind(&simple_wallet::on_command, this, &simple_wallet::prepare_multisig, _1),
                           tr("Export data needed to create a multisig wallet"));
  m_cmd_binder.set_handler("make_multisig", boost::bind(&simple_wallet::on_command, this, &simple_wallet::make_multisig, _1),
                           tr(USAGE_MAKE_MULTISIG),
                           tr("Turn this wallet into a multisig wallet"));
  m_cmd_binder.set_handler("exchange_multisig_keys",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::exchange_multisig_keys, _1),
                           tr(USAGE_EXCHANGE_MULTISIG_KEYS),
                           tr("Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets"));
  m_cmd_binder.set_handler("export_multisig_info",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::export_multisig, _1),
                           tr(USAGE_EXPORT_MULTISIG_INFO),
                           tr("Export multisig info for other participants"));
  m_cmd_binder.set_handler("import_multisig_info",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::import_multisig, _1),
                           tr(USAGE_IMPORT_MULTISIG_INFO),
                           tr("Import multisig info from other participants"));
  m_cmd_binder.set_handler("sign_multisig",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::sign_multisig, _1),
                           tr(USAGE_SIGN_MULTISIG),
                           tr("Sign a multisig transaction from a file"));
  m_cmd_binder.set_handler("submit_multisig",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::submit_multisig, _1),
                           tr(USAGE_SUBMIT_MULTISIG),
                           tr("Submit a signed multisig transaction from a file"));
  m_cmd_binder.set_handler("export_raw_multisig_tx",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::export_raw_multisig, _1),
                           tr(USAGE_EXPORT_RAW_MULTISIG_TX),
                           tr("Export a signed multisig transaction to a file"));
  m_cmd_binder.set_handler("print_ring",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::print_ring, _1),
                           tr(USAGE_PRINT_RING),
                           tr("Print the ring(s) used to spend a given key image or transaction (if the ring size is > 1)\n\n"
                              "Output format:\n"
                              "Key Image, \"absolute\", list of rings"));
  m_cmd_binder.set_handler("set_ring",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::set_ring, _1),
                           tr(USAGE_SET_RING),
                           tr("Set the ring used for a given key image, so it can be reused in a fork"));
  m_cmd_binder.set_handler("freeze",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::freeze, _1),
                           tr(USAGE_FREEZE),
                           tr("Freeze a single enote by key image or enote public key so it will not be used"));
  m_cmd_binder.set_handler("thaw",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::thaw, _1),
                           tr(USAGE_THAW),
                           tr("Thaw a single enote by key image or enote public key so it may be used again"));
  m_cmd_binder.set_handler("frozen",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::frozen, _1),
                           tr(USAGE_FROZEN),
                           tr("Checks whether a given enote is currently frozen by key image or enote public key, or print all frozen enotes"));
  m_cmd_binder.set_handler("lock",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::lock, _1),
                           tr(USAGE_LOCK),
                           tr("Lock the wallet console, requiring the wallet password to continue"));
  m_cmd_binder.set_handler("net_stats",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::net_stats, _1),
                           tr(USAGE_NET_STATS),
                           tr("Prints simple network stats"));
  m_cmd_binder.set_handler("public_nodes",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::public_nodes, _1),
                           tr(USAGE_PUBLIC_NODES),
                           tr("Lists known public nodes"));
  m_cmd_binder.set_handler("welcome",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::welcome, _1),
                           tr(USAGE_WELCOME),
                           tr("Prints basic info about Monero for first time users"));
  m_cmd_binder.set_handler("version",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::version, _1),
                           tr(USAGE_VERSION),
                           tr("Returns version information"));
  m_cmd_binder.set_handler("show_qr_code",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::show_qr_code, _1),
                           tr(USAGE_SHOW_QR_CODE),
                           tr("Show address as QR code"));
  m_cmd_binder.set_handler("help",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::help, _1),
                           tr(USAGE_HELP),
                           tr("Show the help section or the documentation about a <command>."));
 m_cmd_binder.set_handler("apropos",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::apropos, _1),
                           tr(USAGE_APROPOS),
                           tr("Search all command descriptions for keyword(s)"));
 m_cmd_binder.set_handler("scan_tx",
                           boost::bind(&simple_wallet::on_command, this, &simple_wallet::scan_tx, _1),
                           tr(USAGE_SCAN_TX),
                           tr("Scan the transactions given by <txid>(s), processing them and looking for outputs"));
  m_cmd_binder.set_unknown_command_handler(boost::bind(&simple_wallet::on_command, this, &simple_wallet::on_unknown_command, _1));
  m_cmd_binder.set_empty_command_handler(boost::bind(&simple_wallet::on_empty_command, this));
  m_cmd_binder.set_cancel_handler(boost::bind(&simple_wallet::on_cancelled_command, this));
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_variable(const std::vector<std::string> &args)
{
  if (args.empty())
  {
    std::string seed_language = m_wallet_impl->getSeedLanguage();
    if (m_use_english_language_names)
      seed_language = crypto::ElectrumWords::get_english_name_for(seed_language);
    std::string priority_string = "invalid";
    const auto priority_index = m_wallet_impl->getDefaultPriority();
    const fee_priority priority = tools::fee_priority_utilities::from_integral(priority_index);
    priority_string = tools::fee_priority_utilities::to_string(priority);
    std::string ask_password_string = "invalid";
    switch (m_wallet_impl->getAskPasswordType())
    {
      case Wallet::AskPasswordType::AskPassword_Never: ask_password_string = "never"; break;
      case Wallet::AskPasswordType::AskPassword_OnAction: ask_password_string = "action"; break;
      case Wallet::AskPasswordType::AskPassword_ToDecrypt: ask_password_string = "decrypt"; break;
    }
    std::string setup_background_mining_string = "invalid";
    switch (m_wallet_impl->getSetupBackgroundMining())
    {
      case Wallet::BackgroundMiningSetupType::BackgroundMining_Maybe: setup_background_mining_string = "maybe"; break;
      case Wallet::BackgroundMiningSetupType::BackgroundMining_Yes: setup_background_mining_string = "yes"; break;
      case Wallet::BackgroundMiningSetupType::BackgroundMining_No: setup_background_mining_string = "no"; break;
    }
    success_msg_writer() << "seed = " << seed_language;
    success_msg_writer() << "always-confirm-transfers = " << m_wallet_impl->getAlwaysConfirmTransfers();
    success_msg_writer() << "print-ring-members = " << m_wallet_impl->getPrintRingMembers();
    success_msg_writer() << "store-tx-info = " << m_wallet_impl->getStoreTxInfo();
    success_msg_writer() << "auto-refresh = " << m_wallet_impl->getAutoRefresh();
    success_msg_writer() << "refresh-type = " << get_refresh_type_name(m_wallet_impl->getRefreshType());
    success_msg_writer() << "priority = " << priority_index << " (" << priority_string << ")";
    success_msg_writer() << "ask-password = " << static_cast<int>(m_wallet_impl->getAskPasswordType()) << " (" << ask_password_string << ")";
    success_msg_writer() << "unit = " << cryptonote::get_unit(cryptonote::get_default_decimal_point());
    success_msg_writer() << "max-reorg-depth = " << m_wallet_impl->getMaxReorgDepth();
    success_msg_writer() << "min-outputs-count = " << m_wallet_impl->getMinOutputCount();
    success_msg_writer() << "min-outputs-value = " << cryptonote::print_money(m_wallet_impl->getMinOutputValue());
    success_msg_writer() << "merge-destinations = " << m_wallet_impl->getMergeDestinations();
    success_msg_writer() << "confirm-backlog = " << m_wallet_impl->getConfirmBacklog();
    success_msg_writer() << "confirm-backlog-threshold = " << m_wallet_impl->getConfirmBacklogThreshold();
    success_msg_writer() << "confirm-export-overwrite = " << m_wallet_impl->getConfirmExportOverwrite();
    success_msg_writer() << "refresh-from-block-height = " << m_wallet_impl->getRefreshFromBlockHeight();
    success_msg_writer() << "auto-low-priority = " << m_wallet_impl->getAutoLowPriority();
    success_msg_writer() << "segregate-pre-fork-outputs = " << m_wallet_impl->getSegregatePreForkOutputs();
    success_msg_writer() << "key-reuse-mitigation2 = " << m_wallet_impl->getKeyReuseMitigation2();
    const std::pair<size_t, size_t> lookahead = m_wallet_impl->getSubaddressLookahead();
    success_msg_writer() << "subaddress-lookahead = " << lookahead.first << ":" << lookahead.second;
    success_msg_writer() << "segregation-height = " << m_wallet_impl->getSegregationHeight();
    success_msg_writer() << "ignore-fractional-outputs = " << m_wallet_impl->getIgnoreFractionalOutputs();
    success_msg_writer() << "ignore-outputs-above = " << cryptonote::print_money(m_wallet_impl->getIgnoreOutputsAbove());
    success_msg_writer() << "ignore-outputs-below = " << cryptonote::print_money(m_wallet_impl->getIgnoreOutputsBelow());
    success_msg_writer() << "track-uses = " << m_wallet_impl->getTrackUses();
    success_msg_writer() << "background-sync = " << get_background_sync_type_name(m_wallet_impl->getBackgroundSyncType());
    success_msg_writer() << "setup-background-mining = " << setup_background_mining_string;
    success_msg_writer() << "device-name = " << m_wallet_impl->getDeviceState().device_name;
    success_msg_writer() << "export-format = " << (m_wallet_impl->getExportFormat() == Wallet::ExportFormat::ExportFormat_Ascii ? "ascii" : "binary");
    success_msg_writer() << "show-wallet-name-when-locked = " << m_wallet_impl->getShowWalletNameWhenLocked();
    success_msg_writer() << "inactivity-lock-timeout = " << m_wallet_impl->getInactivityLockTimeout()
#ifdef _WIN32
        << " (disabled on Windows)"
#endif
        ;
    success_msg_writer() << "enable-multisig-experimental = " << m_wallet_impl->getEnableMultisig();
    return true;
  }
  else
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot change wallet settings");

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
    CHECK_SIMPLE_VARIABLE("auto-refresh", set_auto_refresh, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("refresh-type", set_refresh_type, tr("full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)"));
    CHECK_SIMPLE_VARIABLE("priority", set_default_priority, tr("0, 1, 2, 3, or 4, or one of ") << join_priority_strings(", "));
    CHECK_SIMPLE_VARIABLE("ask-password", set_ask_password, tr("0|1|2 (or never|action|decrypt)"));
    CHECK_SIMPLE_VARIABLE("unit", set_unit, tr("monero, millinero, micronero, nanonero, piconero"));
    CHECK_SIMPLE_VARIABLE("max-reorg-depth", set_max_reorg_depth, tr("unsigned integer"));
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
    CHECK_SIMPLE_VARIABLE("ignore-fractional-outputs", set_ignore_fractional_outputs, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("ignore-outputs-above", set_ignore_outputs_above, tr("amount"));
    CHECK_SIMPLE_VARIABLE("ignore-outputs-below", set_ignore_outputs_below, tr("amount"));
    CHECK_SIMPLE_VARIABLE("track-uses", set_track_uses, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("background-sync", setup_background_sync, tr("off (default); reuse-wallet-password (reuse the wallet password to encrypt the background cache); custom-background-password (use a custom background password to encrypt the background cache)"));
    CHECK_SIMPLE_VARIABLE("show-wallet-name-when-locked", set_show_wallet_name_when_locked, tr("1 or 0"));
    CHECK_SIMPLE_VARIABLE("inactivity-lock-timeout", set_inactivity_lock_timeout, tr("unsigned integer (seconds, 0 to disable)"));
    CHECK_SIMPLE_VARIABLE("setup-background-mining", set_setup_background_mining, tr("1/yes or 0/no"));
    CHECK_SIMPLE_VARIABLE("device-name", set_device_name, tr("<device_name[:device_spec]>"));
    CHECK_SIMPLE_VARIABLE("export-format", set_export_format, tr("\"binary\" or \"ascii\""));
    CHECK_SIMPLE_VARIABLE("load-deprecated-formats", set_load_deprecated_formats, tr("0 or 1"));
    CHECK_SIMPLE_VARIABLE("enable-multisig-experimental", set_enable_multisig, tr("0 or 1"));
  }
  fail_msg_writer() << tr("set: unrecognized argument(s)");
  return true;
}

//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_log(const std::vector<std::string> &args)
{
  if(args.size() > 1)
  {
    PRINT_USAGE(USAGE_SET_LOG);
    return true;
  }
  if(!args.empty())
  {
    uint16_t level = 0;
    if(epee::string_tools::get_xtype_from_string(level, args[0]))
    {
      if(4 < level)
      {
        fail_msg_writer() << boost::format(tr("wrong number range, use: %s")) % USAGE_SET_LOG;
        return true;
      }
      mlog_set_log_level(level);
    }
    else
    {
      mlog_set_log(args[0].c_str());
    }
  }
  
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
        "Wallet file name (or Ctrl-C to quit)" :
        "Specify wallet file name (e.g., MyWallet). If the wallet doesn't exist, it will be created.\n"
        "Wallet file name (or Ctrl-C to quit)")
      );
      if(std::cin.eof())
      {
        LOG_ERROR("Unexpected std::cin.eof() - Exited simple_wallet::ask_wallet_create_if_needed()");
        return false;
      }
      if(wallet_path.empty())
      {
        fail_msg_writer() << tr("Wallet name not valid. Please try again or use Ctrl-C to quit.");
        wallet_name_valid = false;
      }
      else
      {
        Wallet::walletExists(wallet_path, keys_file_exists, wallet_file_exists);
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
            message_writer() << tr("Looking for filename: ") << boost::filesystem::absolute(wallet_path);
            message_writer() << tr("No wallet found with that name. Confirm creation of new wallet named: ") << wallet_path;
            confirm_creation = input_line("", true);
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
void simple_wallet::print_seed(const epee::wipeable_string &seed)
{
  success_msg_writer(true) << "\n" << boost::format(tr("NOTE: the following %s can be used to recover access to your wallet. "
    "Write them down and store them somewhere safe and secure. Please do not store them in "
    "your email or on file storage services outside of your immediate control.\n")) % (m_wallet_impl->multisig().isMultisig ? tr("string") : tr("25 words"));
  // don't log
  int space_index = 0;
  size_t len  = seed.size();
  for (const char *ptr = seed.data(); len--; ++ptr)
  {
    if (*ptr == ' ')
    {
      if (space_index == 15 || space_index == 7)
        putchar('\n');
      else
        putchar(*ptr);
      ++space_index;
    }
    else
      putchar(*ptr);
  }
  putchar('\n');
  fflush(stdout);
}
//----------------------------------------------------------------------------------------------------
static bool might_be_partial_seed(const epee::wipeable_string &words)
{
  std::vector<epee::wipeable_string> seed;

  words.split(seed);
  return seed.size() < 24;
}
//----------------------------------------------------------------------------------------------------
static bool datestr_to_int(const std::string &heightstr, uint16_t &year, uint8_t &month, uint8_t &day)
{
  if (heightstr.size() != 10 || heightstr[4] != '-' || heightstr[7] != '-')
  {
    fail_msg_writer() << tr("date format must be YYYY-MM-DD");
    return false;
  }
  try
  {
    year  = boost::lexical_cast<uint16_t>(heightstr.substr(0,4));
    // lexical_cast<uint8_t> won't work because uint8_t is treated as character type
    month = boost::lexical_cast<uint16_t>(heightstr.substr(5,2));
    day   = boost::lexical_cast<uint16_t>(heightstr.substr(8,2));
  }
  catch (const boost::bad_lexical_cast &)
  {
    fail_msg_writer() << tr("bad height parameter: ") << heightstr;
    return false;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init(const boost::program_options::variables_map& vm)
{
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    m_electrum_seed.wipe();
  });

  m_wallet_manager.reset(WalletManagerFactory::getWalletManager());

  epee::wipeable_string multisig_keys;
  epee::wipeable_string password;
  epee::wipeable_string seed_pass;

  if (!handle_command_line(vm))
    return false;

  network_type nettype;
  {
      NetworkType nettype_tmp;
      if (!get_nettype(vm, nettype_tmp))
        return false;
      nettype = static_cast<network_type>(nettype_tmp);
  }

  bool welcome = false;

  if((!m_generate_new.empty()) + (!m_wallet_file.empty()) + (!m_generate_from_device.empty()) + (!m_generate_from_view_key.empty()) + (!m_generate_from_spend_key.empty()) + (!m_generate_from_keys.empty()) + (!m_generate_from_multisig_keys.empty()) + (!m_generate_from_json.empty()) > 1)
  {
    fail_msg_writer() << tr("can't specify more than one of --generate-new-wallet=\"wallet_name\", --wallet-file=\"wallet_name\", --generate-from-view-key=\"wallet_name\", --generate-from-spend-key=\"wallet_name\", --generate-from-keys=\"wallet_name\", --generate-from-multisig-keys=\"wallet_name\", --generate-from-json=\"jsonfilename\" and --generate-from-device=\"wallet_name\"");
    return false;
  }
  else if (m_generate_new.empty() && m_wallet_file.empty() && m_generate_from_device.empty() && m_generate_from_view_key.empty() && m_generate_from_spend_key.empty() && m_generate_from_keys.empty() && m_generate_from_multisig_keys.empty() && m_generate_from_json.empty())
  {
    if(!ask_wallet_create_if_needed()) return false;
  }

  bool enable_multisig = false;
  if (m_restore_multisig_wallet) {
    fail_msg_writer() << tr("Multisig is disabled.");
    fail_msg_writer() << tr("Multisig is an experimental feature and may have bugs. Things that could go wrong include: funds sent to a multisig wallet can't be spent at all, can only be spent with the participation of a malicious group member, or can be stolen by a malicious group member.");
    if (!command_line::is_yes(input_line("Do you want to continue restoring a multisig wallet?", true))) {
      message_writer() << tr("You have canceled restoring a multisig wallet.");
      return false;
    }
    enable_multisig = true;
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
            const char *prompt = "Specify multisig seed";
            m_electrum_seed = input_secure_line(prompt);
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
            const char *prompt = m_electrum_seed.empty() ? "Specify Electrum seed" : "Electrum seed continued";
            epee::wipeable_string electrum_seed = input_secure_line(prompt);
            if (std::cin.eof())
              return false;
            if (electrum_seed.empty())
            {
              fail_msg_writer() << tr("specify a recovery parameter with the --electrum-seed=\"words list here\"");
              return false;
            }
            m_electrum_seed += electrum_seed;
            m_electrum_seed += ' ';
          } while (might_be_partial_seed(m_electrum_seed));
        }
      }

      if (m_restore_multisig_wallet)
      {
        const boost::optional<epee::wipeable_string> parsed = m_electrum_seed.parse_hexstr();
        if (!parsed)
        {
          fail_msg_writer() << tr("Multisig seed failed verification");
          return false;
        }
        multisig_keys = *parsed;
      }
      else
      {
        if (!crypto::ElectrumWords::words_to_bytes(m_electrum_seed, m_recovery_key, old_language))
        {
          fail_msg_writer() << tr("Electrum-style word list failed verification");
          return false;
        }
      }

      auto pwd_container = password_prompter(tr("Enter seed offset passphrase, empty if none"), false);
      if (std::cin.eof() || !pwd_container)
        return false;
      seed_pass = pwd_container->password();
      if (!seed_pass.empty() && !m_restore_multisig_wallet)
        m_recovery_key = cryptonote::decrypt_key(m_recovery_key, seed_pass);
    }
    if (!m_generate_from_view_key.empty())
    {
      m_wallet_file = m_generate_from_view_key;
      // parse address
      std::string address_string = input_line("Standard address");
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
      epee::wipeable_string viewkey_string = input_secure_line("Secret view key");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      crypto::secret_key viewkey;
      if (!viewkey_string.hex_to_pod(unwrap(unwrap(viewkey))))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }

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

      // --generate-from-view-key
      auto r = new_wallet(vm, info.address, boost::none, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
    }
    else if (!m_generate_from_spend_key.empty())
    {
      m_wallet_file = m_generate_from_spend_key;
      // parse spend secret key
      epee::wipeable_string spendkey_string = input_secure_line("Secret spend key");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      if (!spendkey_string.hex_to_pod(unwrap(unwrap(m_recovery_key))))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }
      // --generate-from-spend-key
      auto r = new_wallet(vm, m_recovery_key, true, false, "");
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
    }
    else if (!m_generate_from_keys.empty())
    {
      m_wallet_file = m_generate_from_keys;
      // parse address
      std::string address_string = input_line("Standard address");
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
      epee::wipeable_string spendkey_string = input_secure_line("Secret spend key");
      if (std::cin.eof())
        return false;
      if (spendkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      crypto::secret_key spendkey;
      if (!spendkey_string.hex_to_pod(unwrap(unwrap(spendkey))))
      {
        fail_msg_writer() << tr("failed to parse spend key secret key");
        return false;
      }

      // parse view secret key
      epee::wipeable_string viewkey_string = input_secure_line("Secret view key");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty()) {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      crypto::secret_key viewkey;
      if(!viewkey_string.hex_to_pod(unwrap(unwrap(viewkey))))
      {
        fail_msg_writer() << tr("failed to parse view key secret key");
        return false;
      }

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
      // --generate-from-keys
      auto r = new_wallet(vm, info.address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
    }
    
    // Asks user for all the data required to merge secret keys from multisig wallets into one master wallet, which then gets full control of the multisig wallet. The resulting wallet will be the same as any other regular wallet.
    else if (!m_generate_from_multisig_keys.empty())
    {
      m_wallet_file = m_generate_from_multisig_keys;
      unsigned int multisig_m;
      unsigned int multisig_n;
      
      // parse multisig type
      std::string multisig_type_string = input_line("Multisig type (input as M/N with M <= N and M > 1)");
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
      std::string address_string = input_line("Multisig wallet address");
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
      epee::wipeable_string viewkey_string = input_secure_line("Secret view key");
      if (std::cin.eof())
        return false;
      if (viewkey_string.empty())
      {
        fail_msg_writer() << tr("No data supplied, cancelled");
        return false;
      }
      crypto::secret_key viewkey;
      if(!viewkey_string.hex_to_pod(unwrap(unwrap(viewkey))))
      {
        fail_msg_writer() << tr("failed to parse secret view key");
        return false;
      }
      
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
        epee::wipeable_string spendkey_string;
        cryptonote::blobdata spendkey_data;
        // get N secret spend keys from user
        for(unsigned int i=0; i<multisig_n; ++i)
        {
          spendkey_string = input_secure_line(tr((boost::format(tr("Secret spend key (%u of %u)")) % (i+1) % multisig_m).str().c_str()));
          if (std::cin.eof())
            return false;
          if (spendkey_string.empty())
          {
            fail_msg_writer() << tr("No data supplied, cancelled");
            return false;
          }
          if(!spendkey_string.hex_to_pod(unwrap(unwrap(multisig_secret_spendkeys[i]))))
          {
            fail_msg_writer() << tr("failed to parse spend key secret key");
            return false;
          }
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
      
      // create wallet --generate-from-multisig-keys
      auto r = new_wallet(vm, info.address, spendkey, viewkey);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
    }
    
    else if (!m_generate_from_json.empty())
    {
      // --generate-from-json
      auto r = new_wallet(vm, m_generate_from_json);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      m_wallet_file = m_wallet_impl->path();
    }
    else if (!m_generate_from_device.empty())
    {
      m_wallet_file = m_generate_from_device;
      // create wallet --generate-from-device
      auto r = new_wallet(vm);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
      // if no block_height is specified, assume its a new account and start it "now"
      if (command_line::is_arg_defaulted(vm, arg_restore_height)) {
        {
          tools::scoped_message_writer wrt = tools::msg_writer();
          wrt << tr("No restore height is specified.") << " ";
          wrt << tr("Assumed you are creating a new account, restore will be done from current estimated blockchain height.") << " ";
          wrt << tr("Use --restore-height or --restore-date if you want to restore an already setup account from a specific height.");
        }
        std::string confirm = input_line(tr("Is this okay?"), true);
        if (std::cin.eof() || !command_line::is_yes(confirm))
          CHECK_AND_ASSERT_MES(false, false, tr("account creation aborted"));

        m_wallet_impl->setRefreshFromBlockHeight(m_wallet_impl->estimateBlockChainHeight() > 0 ? m_wallet_impl->estimateBlockChainHeight() - 1 : 0);
        m_wallet_impl->setExplicitRefreshFromBlockHeight(true);
        m_restore_height = m_wallet_impl->getRefreshFromBlockHeight();
      }
    }
    else
    {
      if (m_generate_new.empty()) {
        fail_msg_writer() << tr("specify a wallet path with --generate-new-wallet (not --wallet-file)");
        return false;
      }
      m_wallet_file = m_generate_new;
      boost::optional<epee::wipeable_string> r;
      if (m_restore_multisig_wallet) // --restore-multisig-wallet
        r = new_wallet(vm, multisig_keys, seed_pass, old_language);
      else                           // { --generate-new-wallet | --non-deterministic | --restore-deterministic-wallet / --restore-from-seed }
        r = new_wallet(vm, m_recovery_key, m_restore_deterministic_wallet, m_non_deterministic, old_language);
      CHECK_AND_ASSERT_MES(r, false, tr("account creation failed"));
      password = *r;
      welcome = true;
    }

    if (m_restoring && m_generate_from_json.empty() && m_generate_from_device.empty())
    {
      m_wallet_impl->setExplicitRefreshFromBlockHeight(!(command_line::is_arg_defaulted(vm, arg_restore_height) &&
        command_line::is_arg_defaulted(vm, arg_restore_date)));
      if (command_line::is_arg_defaulted(vm, arg_restore_height) && !command_line::is_arg_defaulted(vm, arg_restore_date))
      {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        if (!datestr_to_int(m_restore_date, year, month, day))
          return false;
        m_restore_height = m_wallet_impl->getBlockchainHeightByDate(year, month, day);
        int error_code;
        std::string error_msg;
        m_wallet_impl->statusWithErrorString(error_code, error_msg);
        if (!error_code)
          success_msg_writer() << tr("Restore height is: ") << m_restore_height;
        else
        {
          fail_msg_writer() << error_msg;
          return false;
        }
      }
    }
    if (!m_wallet_impl->getExplicitRefreshFromBlockHeight() && m_restoring)
    {
      uint32_t version;
      bool connected = try_connect_to_daemon(false, &version);
      while (true)
      {
        std::string heightstr;
        if (!connected || version < MAKE_CORE_RPC_VERSION(1, 6))
          heightstr = input_line("Restore from specific blockchain height (optional, default 0)");
        else
          heightstr = input_line("Restore from specific blockchain height (optional, default 0),\nor alternatively from specific date (YYYY-MM-DD)");
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
          uint16_t year;
          uint8_t month;  // 1, 2, ..., 12
          uint8_t day;    // 1, 2, ..., 31
          try
          {
            if (!datestr_to_int(heightstr, year, month, day))
              return false;
            m_restore_height = m_wallet_impl->getBlockchainHeightByDate(year, month, day);
            success_msg_writer() << tr("Restore height is: ") << m_restore_height;
            std::string confirm = input_line(tr("Is this okay?"), true);
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
      uint64_t estimate_height = m_wallet_impl->estimateBlockChainHeight();
      if (m_restore_height >= estimate_height)
      {
        success_msg_writer() << tr("Restore height ") << m_restore_height << (" is not yet reached. The current estimated height is ") << estimate_height;
        std::string confirm = input_line(tr("Still apply restore height?"), true);
        if (std::cin.eof() || command_line::is_no(confirm))
          m_restore_height = 0;
      }
      m_wallet_impl->setRefreshFromBlockHeight(m_restore_height);
    }
    if (enable_multisig)
      m_wallet_impl->setEnableMultisig(true);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(password.data(), password.size()));
  }
  else
  {
    assert(!m_wallet_file.empty());
    if (!m_subaddress_lookahead.empty())
    {
      fail_msg_writer() << tr("can't specify --subaddress-lookahead and --wallet-file at the same time");
      return false;
    }
    auto r = open_wallet(vm);
    CHECK_AND_ASSERT_MES(r, false, tr("failed to open account"));
    password = *r;
  }
  if (!m_wallet_impl)
  {
    fail_msg_writer() << tr("wallet is null");
    return false;
  }
  if (!m_wallet_impl->trustedDaemon())
  {
    message_writer(console_color_red, true) << (boost::format(tr("Warning: using an untrusted daemon at %s")) % m_wallet_impl->getWalletState().daemon_address).str();
    message_writer(console_color_red, true) << boost::format(tr("Using a third party daemon can be detrimental to your security and privacy"));
    bool ssl = false;
    if (m_wallet_impl->connectToDaemon(NULL, &ssl) && !ssl)
      message_writer(console_color_red, true) << boost::format(tr("Using your own without SSL exposes your RPC traffic to monitoring"));
    message_writer(console_color_red, true) << boost::format(tr("You are strongly encouraged to connect to the Monero network using your own daemon"));
    message_writer(console_color_red, true) << boost::format(tr("If you or someone you trust are operating this daemon, you can use --trusted-daemon"));

    if (m_wallet_manager->wasBootstrapEverUsed())
      message_writer(console_color_red, true) << boost::format(tr("Moreover, a daemon is also less secure when running in bootstrap mode"));
  }

  if (m_wallet_impl->getWalletState().ring_database.empty())
    fail_msg_writer() << tr("Failed to initialize ring database: privacy enhancing features will be inactive");

  m_wallet_impl->setListener(this);

  bool skip_check_background_mining = !command_line::get_arg(vm, arg_command).empty();
  if (!skip_check_background_mining)
    check_background_mining(password);

  if (welcome)
    message_writer(console_color_yellow, true) << tr("If you are new to Monero, type \"welcome\" for a brief overview.");

  // the Wallet API does a refresh of the tx history in WalletImpl::doRefresh()
  // which does not happpen if a) there is no connection to a daemon, or b) the local daemon is not synced
  // and that leads to an empty history, therefore we do a manual refresh here
  m_wallet_impl->history()->refresh();

  m_last_activity_time = time(NULL);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::deinit()
{
  if (!m_wallet_impl.get())
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
  m_restore_deterministic_wallet  = command_line::get_arg(vm, arg_restore_deterministic_wallet) || command_line::get_arg(vm, arg_restore_from_seed);
  m_restore_multisig_wallet       = command_line::get_arg(vm, arg_restore_multisig_wallet);
  m_non_deterministic             = command_line::get_arg(vm, arg_non_deterministic);
  m_restore_height                = command_line::get_arg(vm, arg_restore_height);
  m_restore_date                  = command_line::get_arg(vm, arg_restore_date);
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
  if (!command_line::is_arg_defaulted(vm, arg_restore_date))
  {
    uint16_t year;
    uint8_t month, day;
    if (!datestr_to_int(m_restore_date, year, month, day))
      return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::try_connect_to_daemon(bool silent, uint32_t* version)
{
  uint32_t version_ = 0;
  if (!version)
    version = &version_;
  bool wallet_is_outdated = false, daemon_is_outdated = false;
  std::string daemon_address = m_wallet_impl->getWalletState().daemon_address;
  if (!m_wallet_impl->connectToDaemon(version, NULL, 200000, &wallet_is_outdated, &daemon_is_outdated))
  {
    if (!silent)
    {
      if (m_wallet_impl->isOffline())
        fail_msg_writer() << tr("wallet failed to connect to daemon, because it is set to offline mode");
      else if (wallet_is_outdated)
        fail_msg_writer() << tr("wallet failed to connect to daemon, because it is not up to date. ") <<
          tr("Please make sure you are running the latest wallet.");
      else if (daemon_is_outdated)
        fail_msg_writer() << tr("wallet failed to connect to daemon: ") << daemon_address << ". " <<
          tr("Daemon is not up to date. "
          "Please make sure the daemon is running the latest version or change the daemon address using the 'set_daemon' command.");
      else
        fail_msg_writer() << tr("wallet failed to connect to daemon: ") << daemon_address << ". " <<
          tr("Daemon either is not started or wrong port was passed. "
          "Please make sure daemon is running or change the daemon address using the 'set_daemon' command.");
    }
    return false;
  }
  if (!m_wallet_impl->getAllowMismatchedDaemonVersion() && ((*version >> 16) != CORE_RPC_VERSION_MAJOR))
  {
    if (!silent)
      fail_msg_writer() << boost::format(tr("Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.")) % (*version>>16) % CORE_RPC_VERSION_MAJOR % daemon_address;
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
  std::vector<std::string> language_list_self, language_list_english;
  const std::vector<std::string> &language_list = m_use_english_language_names ? language_list_english : language_list_self;
  std::string language_choice;
  int language_number = -1;
  crypto::ElectrumWords::get_language_list(language_list_self, false);
  crypto::ElectrumWords::get_language_list(language_list_english, true);
  std::cout << tr("List of available languages for your wallet's seed:") << std::endl;
  std::cout << tr("If your display freezes, exit blind with ^C, then run again with --use-english-language-names") << std::endl;
  int ii;
  std::vector<std::string>::const_iterator it;
  for (it = language_list.begin(), ii = 0; it != language_list.end(); it++, ii++)
  {
    std::cout << ii << " : " << *it << std::endl;
  }
  while (language_number < 0)
  {
    language_choice = input_line(tr("Enter the number corresponding to the language of your choice"));
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
  return language_list_self[language_number];
}
//----------------------------------------------------------------------------------------------------
boost::optional<tools::password_container> simple_wallet::get_and_verify_password() const
{
  const bool verify = m_wallet_file.empty();
  auto pwd_container = (m_wallet_impl->isBackgroundWallet() && m_wallet_impl->getBackgroundSyncType() == Wallet::BackgroundSync_CustomPassword)
    ? background_sync_cache_password_prompter(verify)
    : default_password_prompter(verify);
  if (!pwd_container)
    return boost::none;

  if (!m_wallet_impl->verifyPassword(std::string_view(pwd_container->password().data(), pwd_container->password().size())))
  {
    fail_msg_writer() << tr("invalid password");
    return boost::none;
  }
  return pwd_container;
}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const crypto::secret_key& recovery_key, bool recover, bool two_random, const std::string &old_language)
{
  const auto pwd = default_password_prompter(true);
  epee::wipeable_string password = pwd->password();

  bool was_deprecated_wallet = m_restore_deterministic_wallet && ((old_language == crypto::ElectrumWords::old_language_name) ||
    crypto::ElectrumWords::get_is_old_style_seed(m_electrum_seed));

  std::string mnemonic_language = old_language;

  std::vector<std::string> language_list;
  crypto::ElectrumWords::get_language_list(language_list);
  if (mnemonic_language.empty() && std::find(language_list.begin(), language_list.end(), m_mnemonic_language) != language_list.end())
  {
    mnemonic_language = m_mnemonic_language;
  }

  NetworkType nettype;
  if (!get_nettype(vm, nettype))
    return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

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
      return {};
  }

  const bool create_address_file = command_line::get_arg(vm, arg_create_address_file);

  // convert rng value to electrum-style word list
  epee::wipeable_string electrum_words;
  if (recover) // { --generate-from-spend-key | --restore-deterministic-wallet / --restore-from-seed }
  {
    crypto::ElectrumWords::bytes_to_words(m_recovery_key, electrum_words, mnemonic_language);
    // note: if a seed_offset passphrase was set we already decrypted m_recovery_key with it at this time, so we ignore the arg here
    m_wallet_impl.reset(m_wallet_manager->recoveryWallet(m_wallet_file,
                                                         std::string(password.data(), password.size()),
                                                         std::string(electrum_words.data(), electrum_words.size()),
                                                         nettype,
                                                         kdf_rounds,
                                                         create_address_file,
                                                         /*seed_offset*/ {},
                                                         /*unattended*/ false));
  }
  else // --generate-new-wallet [ --non-deterministic ]
  {
    m_wallet_impl.reset(m_wallet_manager->createWallet(m_wallet_file,
                                                       std::string(password.data(), password.size()),
                                                       mnemonic_language,
                                                       nettype,
                                                       kdf_rounds,
                                                       create_address_file,
                                                       two_random,
                                                       /*unattended*/ false));
    if (!two_random)
    {
      // decrypt keys, else the seed willl not match
      std::unique_ptr<Monero::WalletKeysDecryptGuard> unlocker = std::unique_ptr<Monero::WalletKeysDecryptGuard>(
              m_wallet_impl->createKeysDecryptGuard(std::string_view(password.data(), password.size())));
      crypto::secret_key spend_key;
      epee::string_tools::hex_to_pod(m_wallet_impl->secretSpendKey(), spend_key);
      crypto::ElectrumWords::bytes_to_words(spend_key, electrum_words, mnemonic_language);
    }
  }

  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet_impl->setSubaddressLookahead(lookahead->first, lookahead->second);
  }

  message_writer(console_color_white, true) << tr("Generated new wallet: ")
    << m_wallet_impl->address();
  PAUSE_READLINE();
  std::cout << tr("View key: ") << m_wallet_impl->secretViewKey();
  putchar('\n');

  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Your wallet has been generated!\n"
    "To start synchronizing with the daemon, use the \"refresh\" command.\n"
    "Use the \"help\" command to see a simplified list of available commands.\n"
    "Use \"help all\" command to see the list of all available commands.\n"
    "Use \"help <command>\" to see a command's documentation.\n"
    "Always use the \"exit\" command when closing monero-wallet-cli to save \n"
    "your current session's state. Otherwise, you might need to synchronize \n"
    "your wallet again (your wallet keys are NOT at risk in any case).\n")
  ;
  success_msg_writer() << tr("Filename: ") << boost::filesystem::absolute(m_wallet_impl->keysFilename());

  if (!two_random)
  {
    print_seed(electrum_words);
  }
  success_msg_writer() << "**********************************************************************";

  return password;
}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
  const cryptonote::account_public_address& address, const boost::optional<crypto::secret_key>& spendkey,
  const crypto::secret_key& viewkey)
{
  const auto pwd = default_password_prompter(true);
  epee::wipeable_string password = pwd->password();

  NetworkType nettype;
  if (!get_nettype(vm, nettype))
    return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

  // skip mnemonic language for view-only wallet
  std::string mnemonic_language = crypto::ElectrumWords::old_language_name;
  if (spendkey)
  {
    mnemonic_language = get_mnemonic_language();;
    if (mnemonic_language.empty())
      return {};
  }

  const bool create_address_file = command_line::get_arg(vm, arg_create_address_file);
  // { --generate-from-view-key | --generate-from-keys | --generate-from-multisig-keys }
  m_wallet_impl.reset(m_wallet_manager->createWalletFromKeys(m_wallet_file,
                                                             std::string(password.data(), password.size()),
                                                             mnemonic_language,
                                                             nettype,
                                                             m_restore_height,
                                                             cryptonote::get_account_address_as_str(static_cast<network_type>(nettype), /* subaddress */ false, address),
                                                             epee::string_tools::pod_to_hex(unwrap(unwrap(viewkey))),
                                                             spendkey ? epee::string_tools::pod_to_hex(unwrap(unwrap(*spendkey))) : "",
                                                             kdf_rounds,
                                                             create_address_file,
                                                             /* unattended */ false));

  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet_impl->setSubaddressLookahead(lookahead->first, lookahead->second);
  }

  if (m_restore_height)
    m_wallet_impl->setRefreshFromBlockHeight(m_restore_height);

  message_writer(console_color_white, true) << tr("Generated new wallet: ")
      << m_wallet_impl->address();

  return password;
}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::new_wallet(const boost::program_options::variables_map& vm)
{
  const auto pwd = default_password_prompter(true);
  epee::wipeable_string password = pwd->password();

  NetworkType nettype;
  if (!get_nettype(vm, nettype))
    return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

  // --generate-from-device
  const std::string hw_device_name = command_line::get_arg(vm, arg_hw_device);
  const bool create_address_file = command_line::get_arg(vm, arg_create_address_file);
  const std::string device_derivation_path = command_line::get_arg(vm, arg_hw_device_derivation_path);
  m_wallet_impl.reset(m_wallet_manager->createWalletFromDevice(m_wallet_file,
                                                             std::string(password.data(), password.size()),
                                                             nettype,
                                                             hw_device_name.empty() ? "Ledger" : hw_device_name,
                                                             m_restore_height,
                                                             m_subaddress_lookahead,
                                                             kdf_rounds,
                                                             /* listener */ nullptr,
                                                             device_derivation_path,
                                                             create_address_file,
                                                             /* unattended */ false));

  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  message_writer(console_color_white, true) << tr("Generated new wallet on hw device: ")
      << m_wallet_impl->address();

  return password;

}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
    const epee::wipeable_string &multisig_keys, const epee::wipeable_string &seed_pass, const std::string &old_language)
{
  // --restore-multisig-wallet
  const auto pwd = default_password_prompter(true);
  epee::wipeable_string password = pwd->password();

  NetworkType nettype;
  if (!get_nettype(vm, nettype))
      return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

  std::string mnemonic_language = old_language;

  std::vector<std::string> language_list;
  crypto::ElectrumWords::get_language_list(language_list);
  if (mnemonic_language.empty() && std::find(language_list.begin(), language_list.end(), m_mnemonic_language) != language_list.end())
  {
    mnemonic_language = m_mnemonic_language;
  }

  const bool create_address_file = command_line::get_arg(vm, arg_create_address_file);
  m_wallet_impl.reset(m_wallet_manager->createWalletFromMultisigSeed(
                            m_wallet_file,
                            std::string(password.data(), password.size()),
                            mnemonic_language,
                            nettype,
                            m_restore_height,
                            std::string(multisig_keys.data(), multisig_keys.size()),
                            std::string(seed_pass.data(), seed_pass.size()),
                            kdf_rounds,
                            create_address_file,
                            /* unattended */ false));

  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to generate new wallet: ") << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet_impl->setSubaddressLookahead(lookahead->first, lookahead->second);
  }

  const MultisigState ms_status{m_wallet_impl->multisig()};

  if (!ms_status.isMultisig || !ms_status.isReady)
  {
    fail_msg_writer() << tr("failed to generate new mutlisig wallet");
    return {};
  }
  message_writer(console_color_white, true) << boost::format(tr("Generated new %u/%u multisig wallet: ")) % ms_status.threshold % ms_status.total
    << m_wallet_impl->address();

  return password;
}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::new_wallet(const boost::program_options::variables_map& vm,
                                                                 const std::string &json_file)
{
  // --generate-from-json
  NetworkType nettype;
  if (!get_nettype(vm, nettype))
      return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

  std::string pw = "";
  epee::misc_utils::auto_scope_leave_caller pw_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    memwipe(&pw[0], pw.size());
  });
  m_wallet_impl.reset(m_wallet_manager->createWalletFromJson(
                            json_file,
                            nettype,
                            pw,
                            kdf_rounds,
                            /* unattended */ false));

  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  if (!m_subaddress_lookahead.empty())
  {
    auto lookahead = parse_subaddress_lookahead(m_subaddress_lookahead);
    assert(lookahead);
    m_wallet_impl->setSubaddressLookahead(lookahead->first, lookahead->second);
  }

  message_writer(console_color_white, true) << tr("Generated new wallet: ")
    << m_wallet_impl->address();

  return epee::wipeable_string(pw.data(), pw.size());
;
}
//----------------------------------------------------------------------------------------------------
boost::optional<epee::wipeable_string> simple_wallet::open_wallet(const boost::program_options::variables_map& vm)
{
  if (m_wallet_file.empty())
  {
    fail_msg_writer() << tr("wallet file path empty");
    return {};
  }

  bool keys_file_exists;
  bool wallet_file_exists;

  Wallet::walletExists(m_wallet_file, keys_file_exists, wallet_file_exists);
  if(!keys_file_exists)
  {
    fail_msg_writer() << tr("Key file not found. Failed to open wallet");
    return {};
  }
  
  const auto pwd = default_password_prompter(false);

  NetworkType nettype;
  if (!get_nettype(vm, nettype))
    return {};

  const uint64_t kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);
  THROW_WALLET_EXCEPTION_IF(kdf_rounds == 0, tools::error::wallet_internal_error, "KDF rounds must not be 0");

  {
    std::string tmp_pw = std::string(pwd->password().data(), pwd->password().size());
    epee::misc_utils::auto_scope_leave_caller tmp_pw_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
      memwipe(&tmp_pw[0], tmp_pw.size());
    });
    m_wallet_impl.reset(m_wallet_manager->openWallet(m_wallet_file,
                                                     tmp_pw,
                                                     nettype,
                                                     kdf_rounds,
                                                     /* listener */ nullptr,
                                                     /* unattended */ false));
  }
  if (!m_wallet_impl)
    return {};

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to load wallet: ") << error_msg;
    return {};
  }

  if (!init_wallet(vm))
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
      fail_msg_writer() << tr("failed to initialize wallet: ") << error_msg;
    return {};
  }

  std::string prefix;
  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (m_wallet_impl->watchOnly())
    prefix = tr("Opened watch-only wallet");
  else if (ms_status.isMultisig)
    prefix = (boost::format(tr("Opened %u/%u multisig wallet%s")) % ms_status.threshold % ms_status.total % (ms_status.isReady ? "" : " (not yet finalized)")).str();
  else if (m_wallet_impl->isBackgroundWallet())
    prefix = tr("Opened background wallet");
  else
    prefix = tr("Opened wallet");
  message_writer(console_color_white, true) <<
    prefix << ": " << m_wallet_impl->address();
  if (m_wallet_impl->getDeviceType()) {
     message_writer(console_color_white, true) << "Wallet is on device: " << m_wallet_impl->getDeviceState().device_name;
  }

  // If the wallet file is deprecated, we should ask for mnemonic language again and store
  // everything in the new format.
  // NOTE: this is_deprecated refers to the wallet file format before becoming JSON. It does not refer to the "old english" seed words form of "deprecated" used elsewhere.
  if (m_wallet_impl->getWalletState().is_deprecated)
  {
    SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return {};);
    if (m_wallet_impl->isDeterministic())
    {
      message_writer(console_color_green, false) << "\n" << tr("You had been using "
        "a deprecated version of the wallet. Please proceed to upgrade your wallet.\n");
      std::string mnemonic_language = get_mnemonic_language();
      if (mnemonic_language.empty())
        return {};
      m_wallet_impl->setSeedLanguage(mnemonic_language);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd->password().data(), pwd->password().size()));

      // Display the seed
      std::string seed = m_wallet_impl->seed();
      epee::misc_utils::auto_scope_leave_caller seed_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
          memwipe(&seed[0], seed.size());
      });
      print_seed(epee::wipeable_string(seed));
    }
    else
    {
      message_writer(console_color_green, false) << "\n" << tr("You had been using "
        "a deprecated version of the wallet. Your wallet file format is being upgraded now.\n");
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(pwd->password().data(), pwd->password().size()));
    }
  }
  success_msg_writer() <<
    "**********************************************************************\n" <<
    tr("Use the \"help\" command to see a simplified list of available commands.\n") <<
    tr("Use \"help all\" to see the list of all available commands.\n") <<
    tr("Use \"help <command>\" to see a command's documentation.\n") <<
    "**********************************************************************";
  return boost::optional<epee::wipeable_string>(pwd->password());
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::close_wallet()
{
  if (m_idle_run.load(std::memory_order_relaxed))
  {
    m_idle_run.store(false, std::memory_order_relaxed);
    m_wallet_impl->stop();
    {
      boost::unique_lock<boost::mutex> lock(m_idle_mutex);
      m_idle_cond.notify_one();
    }
    m_idle_thread.join();
  }

  if (!m_wallet_manager->closeWallet(m_wallet_impl.get(), /* store */ true, /* do_delete_pointer */ false))
  {
    fail_msg_writer() << tr("failed to deinitialize wallet");
    std::string error_msg = m_wallet_manager->errorString();
    if (!error_msg.empty())
      fail_msg_writer() << error_msg;
    return false;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::init_wallet(const boost::program_options::variables_map &vm)
{
  std::string daemon_address, daemon_username, daemon_password;
  boost::optional<bool> is_trusted_daemon;
  Monero::Wallet::SSLSupport ssl_support;
  std::string ssl_private_key_path, ssl_certificate_path, ssl_ca_file_path;
  std::vector<std::string> ssl_allowed_fingerprints_str;
  bool ssl_allow_any_cert;
  std::string proxy;
  try {
    get_daemon_init_settings(vm,
                             daemon_address,
                             daemon_username,
                             daemon_password,
                             is_trusted_daemon,
                             ssl_support,
                             ssl_private_key_path,
                             ssl_certificate_path,
                             ssl_ca_file_path,
                             ssl_allowed_fingerprints_str,
                             ssl_allow_any_cert,
                             proxy);
  }
  catch (const exception &e) {
    fail_msg_writer() << tr("failed to initialize wallet: ") << e.what();
    return false;
  }

  if (command_line::get_arg(vm, arg_offline))
    m_wallet_impl->setOffline(true);

  m_wallet_manager->setDaemonAddress(daemon_address);
  m_wallet_impl->init(daemon_address,
                      /* upper_transaction_size_limit */ 0,
                      daemon_username,
                      daemon_password,
                      /* use_ssl */ false,
                      /* lightWallet */ false);

  return m_wallet_impl->setDaemon(daemon_address,
                                  daemon_username,
                                  daemon_password,
                                  is_trusted_daemon ? *is_trusted_daemon : false,
                                  ssl_support,
                                  ssl_private_key_path,
                                  ssl_certificate_path,
                                  ssl_ca_file_path,
                                  ssl_allowed_fingerprints_str,
                                  ssl_allow_any_cert,
                                  proxy);

}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save(const std::vector<std::string> &args)
{
  LOCK_IDLE_SCOPE();
  if (m_wallet_impl->store(""))
    success_msg_writer() << tr("Wallet data saved");
  else
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_watch_only(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  if (m_wallet_impl->multisig().isMultisig)
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

  std::string new_keys_filename;
  m_wallet_impl->writeWatchOnlyWallet(std::string_view(pwd_container->password().data(), pwd_container->password().size()), new_keys_filename);
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (!error_code)
    success_msg_writer() << tr("Watch only wallet saved as: ") << new_keys_filename;
  else
    fail_msg_writer() << error_msg;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::start_background_mining()
{
  if (!m_wallet_manager->isBackgroundMiningEnabled())
  {
    if (!m_wallet_manager->startMining(m_wallet_impl->address(),
                                      /* threads */ 1,
                                      /* do_background_mining */ true,
                                      /* ignore_battery */ false))
    {
      fail_msg_writer() << tr("Failed to setup background mining: ") << m_wallet_manager->errorString();
      return;
    }
  }
  success_msg_writer() << tr("Background mining enabled. Thank you for supporting the Monero network.");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop_background_mining()
{
  if (m_wallet_manager->isBackgroundMiningEnabled())
  {
    if (!m_wallet_manager->stopMining())
    {
      fail_msg_writer() << tr("Failed to setup background mining: ") << m_wallet_manager->errorString();
      return;
    }
  }
  message_writer(console_color_red, false) << tr("Background mining not enabled. Run \"set setup-background-mining 1\" to change.");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::check_background_mining(const epee::wipeable_string &password)
{
  // Background mining can be toggled from the main wallet
  if (m_wallet_impl->isBackgroundWallet() || m_wallet_impl->isBackgroundSyncing())
    return;

  Wallet::BackgroundMiningSetupType setup = m_wallet_impl->getSetupBackgroundMining();
  if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_No)
  {
    message_writer(console_color_red, false) << tr("Background mining not enabled. Run \"set setup-background-mining 1\" to change.");
    return;
  }

  if (!m_wallet_impl->trustedDaemon())
  {
    message_writer() << tr("Using an untrusted daemon, skipping background mining check");
    return;
  }

  if (m_wallet_manager->isBackgroundMiningEnabled())
  {
    // already active, nice
    if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_Maybe)
    {
      m_wallet_impl->setSetupBackgroundMining(Wallet::BackgroundMiningSetupType::BackgroundMining_Yes);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(password.data(), password.size()));
    }
    start_background_mining();
    return;
  }
  if (m_wallet_manager->isMining())
    return;

  if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_Maybe)
  {
    message_writer() << tr("The daemon is not set up to background mine.");
    message_writer() << tr("With background mining enabled, the daemon will mine when idle and not on battery.");
    message_writer() << tr("Enabling this supports the network you are using, and makes you eligible for receiving new monero");
    std::string accepted = input_line(tr("Do you want to do it now? (Y/Yes/N/No): "));
    if (std::cin.eof() || !command_line::is_yes(accepted)) {
      m_wallet_impl->setSetupBackgroundMining(Wallet::BackgroundMiningSetupType::BackgroundMining_No);
      m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(password.data(), password.size()));
      message_writer(console_color_red, false) << tr("Background mining not enabled. Set setup-background-mining to 1 to change.");
      return;
    }
    m_wallet_impl->setSetupBackgroundMining(Wallet::BackgroundMiningSetupType::BackgroundMining_Yes);
    m_wallet_impl->rewriteWalletFile(m_wallet_file, std::string_view(password.data(), password.size()));
    start_background_mining();
  }
  else
  {
    // the setting is already enabled, and the daemon is not mining yet, so start it
    start_background_mining();
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::start_mining(const std::vector<std::string>& args)
{
  if (!m_wallet_impl)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }

  if (!m_wallet_impl->trustedDaemon())
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  bool ok = true;
  size_t arg_size = args.size();
  bool do_ignore_battery;
  bool do_background_mining;
  uint32_t n_threads;
  if(arg_size >= 3)
  {
    if (!parse_bool_and_use(args[2], [&](bool r) { do_ignore_battery = r; }))
      return true;
  }
  if(arg_size >= 2)
  {
    if (!parse_bool_and_use(args[1], [&](bool r) { do_background_mining = r; }))
      return true;
  }
  if(arg_size >= 1)
  {
    uint16_t num = 1;
    ok = string_tools::get_xtype_from_string(num, args[0]);
    ok = ok && 1 <= num;
    n_threads = num;
  }
  else
  {
    n_threads = 1;
  }

  if (!ok)
  {
    PRINT_USAGE(USAGE_START_MINING);
    return true;
  }

  bool r = m_wallet_manager->startMining(m_wallet_impl->address(), n_threads, do_background_mining, do_ignore_battery);
  if (r)
    success_msg_writer() << tr("Mining started in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been started: ") << m_wallet_manager->errorString();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::stop_mining(const std::vector<std::string>& args)
{
  if (!m_wallet_impl)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  bool r = m_wallet_manager->stopMining();
  if (r)
    success_msg_writer() << tr("Mining stopped in daemon");
  else
    fail_msg_writer() << tr("mining has NOT been stopped: ") << m_wallet_manager->errorString();
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_daemon(const std::vector<std::string>& args)
{
  std::string daemon_url;

  if (args.size() < 1 || args.size() > 4)
  {
    PRINT_USAGE(USAGE_SET_DAEMON);
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
      uint16_t daemon_port = get_config(static_cast<network_type>(m_wallet_impl->nettype())).RPC_DEFAULT_PORT;
      daemon_url = match[1] + match[2] + std::string(":") + std::to_string(daemon_port);
    } else {
      daemon_url = args[0];
    }

    epee::net_utils::http::url_content parsed{};
    const bool r = epee::net_utils::parse_url(daemon_url, parsed);
    if (!r)
    {
      fail_msg_writer() << tr("Failed to parse address");
      return true;
    }

    std::string trusted;
    if (args.size() >= 2)
    {
      if (args[1] == "trusted")
        trusted = "trusted";
      else if (args[1] == "untrusted")
        trusted = "untrusted";
      else if (args[1] == "this-is-probably-a-spy-node")
        trusted = "this-is-probably-a-spy-node";
      else
      {
        fail_msg_writer() << tr("Expected trusted, untrusted or this-is-probably-a-spy-node got ") << args[1];
        return true;
      }
    }

    std::string daemon_username;
    std::string daemon_password;
    std::string proxy_address;
    for (size_t i = 2; i < args.size(); ++i)
    {
      // daemon RPC login
      if (args[i].rfind("login=", 0) == 0 && args[i].find(":") != std::string::npos)
      {
        std::function<boost::optional<tools::password_container>(const char *, bool)> pw_prompter = password_prompter;
        auto parsed = tools::login::parse(args[i].substr(6), /* verify */ false, [pw_prompter](bool verify) {
            if (!pw_prompter)
            {
              MERROR("Password needed without prompt function");
              return boost::optional<tools::password_container>();
            }
            return pw_prompter("Daemon client password", verify);
          }
        );

        if (!parsed)
        {
          fail_msg_writer() << tr("Failed to parse daemon rpc login");
          return true;
        }
        daemon_username = std::move(parsed->username);
        epee::wipeable_string daemon_pw = std::move(parsed->password).password();
        daemon_password = std::string(daemon_pw.data(), daemon_pw.size());
      }
      // proxy address
      else if (args[i].rfind("proxy=", 0) == 0)
          proxy_address = args[i].substr(6);
      else
      {
        fail_msg_writer() << tr("Expected either `") << "login=" << tr("<username>[:<password>]` or `") << "proxy=" << tr("[<ip>:]<port>` got ") << args[i];
        return true;
      }
    }

    bool use_ssl = false;
    if (!tools::is_privacy_preserving_network(parsed.host) && !tools::is_local_address(parsed.host))
    {
      if (trusted == "untrusted" || trusted == "")
      {
        fail_msg_writer() << tr("This is not Tor/I2P address, and is not a trusted daemon.");
        fail_msg_writer() << tr("Either use your own trusted node, connect via Tor or I2P, or pass this-is-probably-a-spy-node and be spied on.");
        return true;
      }

      if (parsed.schema != "https")
        message_writer(console_color_red) << tr("Warning: connecting to a non-local daemon without SSL, passive adversaries will be able to spy on you.");
      else
          use_ssl = true;
    }

    LOCK_IDLE_SCOPE();
    if (!m_wallet_impl->init(daemon_url,
                             /* upper_transaction_size_limit */ 0,
                             daemon_username,
                             daemon_password,
                             use_ssl,
                             /* light_wallet */ false,
                             proxy_address))
    {
        fail_msg_writer() << tr("Failed to set daemon");
        return true;
    }

    if (!trusted.empty())
    {
      m_wallet_impl->setTrustedDaemon(trusted == "trusted");
    }
    else if (Utils::isAddressLocal(m_wallet_impl->getWalletState().daemon_address))
      MINFO(tr("Daemon is local, assuming trusted"));

    if (!try_connect_to_daemon())
    {
      fail_msg_writer() << tr("Failed to connect to daemon");
      return true;
    }

    success_msg_writer() << boost::format("Daemon set to %s, %s") % daemon_url % (m_wallet_impl->trustedDaemon() ? tr("trusted") : tr("untrusted"));
  } else {
    fail_msg_writer() << tr("This does not seem to be a valid daemon URL.");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_bc(const std::vector<std::string>& args)
{
  if (!m_wallet_impl)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  m_wallet_manager->saveBlockchain();
  std::string err = m_wallet_manager->errorString();
  if (err.empty())
    success_msg_writer() << tr("Blockchain saved");
  else
    fail_msg_writer() << tr("blockchain can't be saved: ") << err;
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::newBlock(uint64_t height)
{
  if (m_locked)
    return;
  if (!m_auto_refresh_refreshing)
    m_refresh_progress_reporter.update(height, false);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::moneyReceived(const std::string &txid,
                                  uint64_t amount,
                                  const uint64_t burnt,
                                  const std::string &enote_pub_key,
                                  const bool is_change /* = false */,
                                  const bool is_coinbase /* = false */)
{
  if (m_locked)
    return;

  // Get latest enote details
  std::unique_ptr<EnoteDetails> ed = m_wallet_impl->getEnoteDetails(enote_pub_key);

  if (!ed || ed->txId() != txid)
  {
    fail_msg_writer() << "money received, but failed to get more information about tx <" << txid << ">";
    return;
  }

  // Console output
  std::stringstream burn;
  if (burnt != 0) {
    burn << " (" << print_money(amount + burnt) << " yet " << print_money(burnt) << " was burnt)";
  }
  message_writer(console_color_green, false) << "\r" <<
    tr("Height ") << ed->blockHeight() << ", " <<
    tr("txid <") << txid << ">, " <<
    print_money(amount) << burn.str() << ", " <<
    tr("idx ") << ed->subaddressIndexMajor() << "/" << ed->subaddressIndexMinor();

  // Warnings
  NetworkType nettype = m_wallet_impl->nettype();
  const uint64_t warn_height = nettype == NetworkType::TESTNET  ? 1000000 :
                               nettype == NetworkType::STAGENET ? 50000 : 1650000;
  if (ed->blockHeight() >= warn_height && !is_change)
  {
    // 8 bytes, encrypted payment id
    if (ed->paymentId().size() == 16 && ed->paymentId() != "0000000000000000")
      message_writer() <<
        tr("NOTE: this transaction uses an encrypted payment ID: consider using subaddresses instead");

    // 32 bytes, unencrypted payment id
    if (ed->paymentId().size() == 64)
      message_writer(console_color_red, false) <<
        tr("WARNING: this transaction uses an unencrypted payment ID: these are obsolete and ignored. Use subaddresses instead.");
  }
  if (ed->unlockTime() && !is_coinbase)
    message_writer() << tr("NOTE: This transaction is locked, see details with: show_transfer ") + txid;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(ed->blockHeight(), true);

  m_wallet_impl->history()->refresh();
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::unconfirmedMoneyReceived(const std::string &txid, uint64_t amount)
{
  if (m_locked)
    return;
  // Not implemented in CLI wallet
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::moneySpent(const std::string &txid,
                               uint64_t amount,
                               const std::string &enote_pub_key,
                               std::pair<uint32_t, uint32_t> subaddr_index /* = {} */)
{
  if (m_locked)
    return;

  // Get latest enote details
  std::unique_ptr<EnoteDetails> ed = m_wallet_impl->getEnoteDetails(enote_pub_key);

  if (!ed)
  {
    fail_msg_writer() << "money spent, but failed to get more information about tx <" << txid << ">";
    return;
  }

  message_writer(console_color_magenta, false) << "\r" <<
    tr("Height ") << ed->blockHeight() << ", " <<
    tr("txid <") << txid << ">, " <<
    tr("spent ") << print_money(amount) << ", " <<
    tr("idx ") << subaddr_index.first << "/" << subaddr_index.second;
  if (m_auto_refresh_refreshing)
    m_cmd_binder.print_prompt();
  else
    m_refresh_progress_reporter.update(ed->blockHeight(), true);

  m_wallet_impl->history()->refresh();
}
//----------------------------------------------------------------------------------------------------
Monero::optional<std::string> simple_wallet::onGetPassword(const char *reason)
{
  if (m_locked)
    return Monero::optional<std::string>{};
  // can't ask for password from a background thread
  if (!m_in_manual_refresh.load(std::memory_order_relaxed))
  {
    message_writer(console_color_red, false) << boost::format(tr("Password needed (%s) - use the refresh command")) % reason;
    m_cmd_binder.print_prompt();
    return Monero::optional<std::string>{};
  }

  PAUSE_READLINE();
  std::string msg = tr("Enter password");
  if (reason && *reason)
    msg += std::string(" (") + reason + ")";
  auto pwd_container = tools::password_container::prompt(false, msg.c_str());
  if (!pwd_container)
  {
    MERROR("Failed to read password");
    return Monero::optional<std::string>{};
  }

  return Monero::optional<std::string>{std::string(pwd_container->password().data(), pwd_container->password().size())};
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::onDeviceButtonRequest(uint64_t code)
{
  message_writer(console_color_white, false) << tr("Device requires attention");
}
//----------------------------------------------------------------------------------------------------
Monero::optional<std::string> simple_wallet::onDevicePinRequest()
{
  PAUSE_READLINE();
  std::string msg = tr("Enter device PIN");
  auto pwd_container = tools::password_container::prompt(false, msg.c_str());
  THROW_WALLET_EXCEPTION_IF(!pwd_container, tools::error::password_entry_failed, tr("Failed to read device PIN"));
  return Monero::optional<std::string>{std::string(pwd_container->password().data(), pwd_container->password().size())};
}
//----------------------------------------------------------------------------------------------------
Monero::optional<std::string> simple_wallet::onDevicePassphraseRequest(bool & on_device)
{
  if (on_device) {
    std::string accepted = input_line(tr(
        "Device asks for passphrase. Do you want to enter the passphrase on device (Y) (or on the host (N))?"));
    if (std::cin.eof() || command_line::is_yes(accepted)) {
      message_writer(console_color_white, true) << tr("Please enter the device passphrase on the device");
      return Monero::optional<std::string>{};
    }
  }

  PAUSE_READLINE();
  on_device = false;
  std::string msg = tr("Enter device passphrase");
  auto pwd_container = tools::password_container::prompt(false, msg.c_str());
  THROW_WALLET_EXCEPTION_IF(!pwd_container, tools::error::password_entry_failed, tr("Failed to read device passphrase"));
  return Monero::optional<std::string>{std::string(pwd_container->password().data(), pwd_container->password().size())};
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::updated() {}
//----------------------------------------------------------------------------------------------------
void simple_wallet::refreshed() {}
//----------------------------------------------------------------------------------------------------
void simple_wallet::onReorg(uint64_t height, uint64_t blocks_detached, size_t transfers_detached) {}
//----------------------------------------------------------------------------------------------------
void simple_wallet::onPoolTxRemoved(const std::string &txid) {}
//----------------------------------------------------------------------------------------------------
void simple_wallet::on_refresh_finished(uint64_t start_height, uint64_t fetched_blocks, bool is_init, bool received_money)
{
  const uint64_t rfbh = m_wallet_impl->getRefreshFromBlockHeight();
  std::string err;
  int err_code;
  const uint64_t dh = m_wallet_impl->daemonBlockChainHeight();
  m_wallet_impl->statusWithErrorString(err_code, err);
  if (err.empty() && rfbh > dh)
  {
    message_writer(console_color_yellow, false) << tr("The wallet's refresh-from-block-height setting is higher than the daemon's height: this may mean your wallet will skip over transactions");
  }

  // Key image sync after the first refresh
  Wallet::DeviceState device_state = m_wallet_impl->getDeviceState();
  if (!device_state.has_tx_cold_sign || device_state.has_ki_live_refresh) {
    return;
  }

  if (!received_money || device_state.last_ki_sync != 0) {
    return;
  }

  // Finished first refresh for HW device and money received -> KI sync
  message_writer() << "\n" << tr("The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. ");

  std::string accepted = input_line(tr("Do you want to do it now? (Y/Yes/N/No): "));
  if (std::cin.eof() || !command_line::is_yes(accepted)) {
    message_writer(console_color_red, false) << tr("hw_key_images_sync skipped. Run command manually before a transfer.");
    return;
  }

  key_images_sync_intern();
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::refresh_main(uint64_t start_height, enum ResetType reset, bool is_init)
{
  if (!try_connect_to_daemon(is_init))
    return true;

  LOCK_IDLE_SCOPE();

  std::string transfer_hash_pre{};
  uint64_t height_pre = 0, height_post;

  PAUSE_READLINE();

  message_writer() << tr("Starting refresh...");

  if (reset != ResetNone)
  {
    if (reset == ResetSoftKeepKI)
      height_pre = m_wallet_impl->hashEnotes(0, transfer_hash_pre);

    m_wallet_impl->rescanBlockchain(reset == ResetHard, reset == ResetSoftKeepKI, /* do_skip_refresh */ true);
  }

  uint64_t fetched_blocks = 0;
  bool received_money = false;
  // fake loop, so we can break out of the block on error
  do {
    m_in_manual_refresh.store(true, std::memory_order_relaxed);
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){m_in_manual_refresh.store(false, std::memory_order_relaxed);});
    // For manual refresh don't allow incremental checking of the pool: Because we did not process the txs
    // for us in the pool during automatic refresh we could miss some of them if we checked the pool
    // incrementally here
    if (!m_wallet_impl->refresh(start_height,
                                /* check_pool */ true,
                                /* try_incremental */ false,
                                /* max_blocks */ std::numeric_limits<uint64_t>::max(),
                                &fetched_blocks,
                                &received_money))
    {
      int error_code;
      std::string error_msg;
      m_wallet_impl->statusWithErrorString(error_code, error_msg);
      fail_msg_writer() << tr("refresh failed: ") << error_msg << ". " << tr("Blocks received: ") << fetched_blocks;
      break;
    }

    if (reset == ResetSoftKeepKI)
    {
      m_wallet_impl->finishRescanBcKeepKeyImages(height_pre, transfer_hash_pre);

      height_post = m_wallet_impl->getWalletState().n_enotes;
      if (height_pre != height_post)
      {
        message_writer() << tr("New transfer received since rescan was started. Key images are incomplete.");
      }
    }

    // Clear line "Height xxx of xxx"
    std::cout << "\r                                                                \r";
    success_msg_writer(true) << tr("Refresh done, blocks received: ") << fetched_blocks;
    if (is_init)
      print_accounts();
    show_balance_unlocked();
    on_refresh_finished(start_height, fetched_blocks, is_init, received_money);
  } while (false);

  // prevent it from triggering the idle screen due to waiting for a foreground refresh
  m_last_activity_time = time(NULL);

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
  return refresh_main(start_height, ResetNone);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance_unlocked(bool detailed)
{
  std::string extra;
  if (m_wallet_impl->hasMultisigPartialKeyImages())
    extra = tr(" (Some owned outputs have partial key images - import_multisig_info needed)");
  else if (m_wallet_impl->hasUnknownKeyImages())
    extra += tr(" (Some owned outputs have missing key images - export_outputs, import_outputs, export_key_images, and import_key_images needed)");
  success_msg_writer() << tr("Currently selected account: [") << m_current_subaddress_account << tr("] ") << m_wallet_impl->getSubaddressLabel(m_current_subaddress_account, 0);
  const std::string tag = m_wallet_impl->getAccountTags().second[m_current_subaddress_account];
  success_msg_writer() << tr("Tag: ") << (tag.empty() ? std::string{tr("(No tag assigned)")} : tag);
  uint64_t blocks_to_unlock, time_to_unlock;
  uint64_t unlocked_balance = m_wallet_impl->unlockedBalance(m_current_subaddress_account, &blocks_to_unlock, &time_to_unlock);
  std::string unlock_time_message;
  if (blocks_to_unlock > 0 && time_to_unlock > 0)
    unlock_time_message = (boost::format(" (%lu block(s) and %s to unlock)") % blocks_to_unlock % get_human_readable_timespan(time_to_unlock)).str();
  else if (blocks_to_unlock > 0)
    unlock_time_message = (boost::format(" (%lu block(s) to unlock)") % blocks_to_unlock).str();
  else if (time_to_unlock > 0)
    unlock_time_message = (boost::format(" (%s to unlock)") % get_human_readable_timespan(time_to_unlock)).str();
  success_msg_writer() << tr("Balance: ") << print_money(m_wallet_impl->balance(m_current_subaddress_account)) << ", "
    << tr("unlocked balance: ") << print_money(unlocked_balance) << unlock_time_message << extra;
  std::map<uint32_t, uint64_t> balance_per_subaddress = m_wallet_impl->balancePerSubaddress(m_current_subaddress_account);
  std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> unlocked_balance_per_subaddress = m_wallet_impl->unlockedBalancePerSubaddress(m_current_subaddress_account);
  if (!detailed || balance_per_subaddress.empty())
    return true;
  success_msg_writer() << tr("Balance per address:");
  success_msg_writer() << boost::format("%15s %21s %21s %7s %21s") % tr("Address") % tr("Balance") % tr("Unlocked balance") % tr("Outputs") % tr("Label");
  std::vector<std::unique_ptr<EnoteDetails>> enote_details = m_wallet_impl->getEnoteDetails();
  for (const auto& i : balance_per_subaddress)
  {
    cryptonote::subaddress_index subaddr_index = {m_current_subaddress_account, i.first};
    std::string address_str = m_wallet_impl->address(m_current_subaddress_account, i.first).substr(0, 6);
    uint64_t num_unspent_outputs = std::count_if(enote_details.begin(), enote_details.end(), [&subaddr_index](const std::unique_ptr<EnoteDetails>& ed) { return !ed->isSpent() && ed->subaddressIndexMajor() == subaddr_index.major && ed->subaddressIndexMinor() == subaddr_index.minor; });
    success_msg_writer() << boost::format(tr("%8u %6s %21s %21s %7u %21s")) % i.first % address_str % print_money(i.second) % print_money(unlocked_balance_per_subaddress[i.first].first) % num_unspent_outputs % m_wallet_impl->getSubaddressLabel(m_current_subaddress_account, i.first);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_balance(const std::vector<std::string>& args/* = std::vector<std::string>()*/)
{
  if (args.size() > 1 || (args.size() == 1 && args[0] != "detail"))
  {
    PRINT_USAGE(USAGE_SHOW_BALANCE);
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
    PRINT_USAGE(USAGE_INCOMING_TRANSFERS);
    return true;
  }
  auto local_args = args;
  LOCK_IDLE_SCOPE();

  std::set<uint32_t> subaddr_indices;
  bool filter = false;
  bool available = false;
  bool verbose = false;
  bool uses = false;
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
  while (local_args.size() > 0)
  {
    if (local_args[0] == "verbose")
      verbose = true;
    else if (local_args[0] == "uses")
      uses = true;
    else if (local_args[0].substr(0, 6) == "index=")
    {
      if (!parse_subaddress_indices(local_args[0], subaddr_indices))
        return true;
    }
    else
    {
      fail_msg_writer() << tr("Invalid keyword: ") << local_args.front();
      break;
    }
    local_args.erase(local_args.begin());
  }

  const uint64_t blockchain_height = m_wallet_impl->blockChainHeight();

  PAUSE_READLINE();

  if (local_args.size() > 0)
  {
    PRINT_USAGE(USAGE_INCOMING_TRANSFERS);
    return true;
  }

  std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet_impl->getEnoteDetails();

  size_t transfers_found = 0;
  for (const auto& ed : eds)
  {
    if (!filter || available != ed->isSpent())
    {
      if (m_current_subaddress_account != ed->subaddressIndexMajor()
              || (!subaddr_indices.empty() && subaddr_indices.count(ed->subaddressIndexMinor()) == 0))
        continue;
      if (!transfers_found)
      {
        std::string verbose_string;
        if (verbose)
          verbose_string = (boost::format("%68s%68s") % tr("pubkey") % tr("key image")).str();
        message_writer() << boost::format("%21s%8s%12s%8s%16s%68s%16s%s") % tr("amount") % tr("spent") % tr("unlocked") % tr("ringct") % tr("global index") % tr("tx id") % tr("addr index") % verbose_string;
      }
      std::string extra_string;
      if (verbose)
        extra_string += (boost::format("  <%64s>%68s") % ed->onetimeAddress() % (ed->isKeyImageKnown() ? ed->keyImage() : ed->isKeyImagePartial() ? (ed->keyImage() + "/p") : std::string(64, '?'))).str();
      if (uses)
      {
        std::vector<uint64_t> heights;
        uint64_t idx = 0;
        for (const auto &e: ed->uses())
        {
          heights.push_back(e.first);
          if (e.first < ed->spentHeight())
            ++idx;
        }
        const std::pair<std::string, std::string> line = show_outputs_line(heights, blockchain_height, idx);
        extra_string += std::string("\n    ") + tr("Used at heights: ") + line.first + "\n    " + line.second;
      }
      message_writer(ed->isSpent() ? console_color_magenta : console_color_green, false) <<
        boost::format("%21s%8s%12s%8s%16u  <%64s>%16u%s") %
        print_money(ed->amount()) %
        (ed->isSpent() ? tr("T") : tr("F")) %
        (ed->isFrozen() ? tr("[frozen]") : ed->isUnlocked() ? tr("unlocked") : tr("locked")) %
        (ed->protocolVersion() == EnoteDetails::TxProtocol::TxProtocol_RingCT ? tr("RingCT") : tr("-")) %
        ed->globalEnoteIndex() %
        ed->txId() %
        ed->subaddressIndexMinor() %
        extra_string;
      ++transfers_found;
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
  else
  {
    success_msg_writer() << boost::format("Found %u/%u transfers") % transfers_found % eds.size();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_payments(const std::vector<std::string> &args)
{
  if(args.empty())
  {
    PRINT_USAGE(USAGE_PAYMENTS);
    return true;
  }

  LOCK_IDLE_SCOPE();

  PAUSE_READLINE();

  message_writer() << boost::format("%68s%68s%12s%21s%16s%16s") %
    tr("payment") % tr("transaction") % tr("height") % tr("amount") % tr("unlock time") % tr("addr index");

  std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet_impl->getEnoteDetails();
  std::vector<EnoteDetails*> payments;
  bool payments_found = false;
  for(const std::string &payment_id : args)
  {
    payments.clear();
    if(Wallet::paymentIdValid(payment_id))
    {
      for (const auto &ed : eds)
      {
        if (ed->paymentId() == payment_id)
          payments.push_back(ed.get());
      }
      if(payments.empty())
      {
        success_msg_writer() << tr("No payments with id ") << payment_id;
        continue;
      }

      for (const auto &ed : payments)
      {
        if(!payments_found)
        {
          payments_found = true;
        }
        success_msg_writer(true) <<
          boost::format("%68s%68s%12s%21s%16s%16s") %
          payment_id %
          ed->txId() %
          ed->blockHeight() %
          print_money(ed->amount()) %
          ed->unlockTime() %
          ed->subaddressIndexMinor();
      }
    }
    else
    {
      fail_msg_writer() << tr("payment ID has invalid format, expected 16 or 64 character hex string: ") << payment_id;
    }
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
uint64_t simple_wallet::get_daemon_blockchain_height(std::string& err)
{
  if (!m_wallet_impl)
  {
    throw std::runtime_error("simple_wallet null wallet");
  }
  int error_code;
  uint64_t bc_height = m_wallet_impl->daemonBlockChainHeight();
  m_wallet_impl->statusWithErrorString(error_code, err);
  return bc_height;
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
  CHECK_IF_BACKGROUND_SYNCING("cannot rescan spent");
  if (!m_wallet_impl->trustedDaemon())
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  {
    LOCK_IDLE_SCOPE();
    m_wallet_impl->rescanSpent();
  }
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
    fail_msg_writer() << error_msg;

  return true;
}
//----------------------------------------------------------------------------------------------------
std::pair<std::string, std::string> simple_wallet::show_outputs_line(const std::vector<uint64_t> &heights, uint64_t blockchain_height, uint64_t highlight_idx) const
{
  std::stringstream ostr;

  for (uint64_t h: heights)
    blockchain_height = std::max(blockchain_height, h);

  for (size_t j = 0; j < heights.size(); ++j)
    ostr << (j == highlight_idx ? " *" : " ") << heights[j];

  // visualize the distribution, using the code by moneroexamples onion-monero-viewer
  const uint64_t resolution = 79;
  std::string ring_str(resolution, '_');
  for (size_t j = 0; j < heights.size(); ++j)
  {
    uint64_t pos = (heights[j] * resolution) / blockchain_height;
    ring_str[pos] = 'o';
  }
  if (highlight_idx < heights.size() && heights[highlight_idx] < blockchain_height)
  {
    uint64_t pos = (heights[highlight_idx] * resolution) / blockchain_height;
    ring_str[pos] = '*';
  }

  return std::make_pair(ostr.str(), ring_str);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::process_ring_members(const PendingTransaction &ptx_vector, std::ostream& ostr, bool verbose)
{
  uint32_t version;
  if (!try_connect_to_daemon(false, &version))
  {
    fail_msg_writer() << tr("failed to connect to daemon");
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
  const std::vector<std::string> &tx_ids = ptx_vector.txid();
  const std::vector<std::vector<std::uint64_t>> &vin_amounts = ptx_vector.vinAmounts();
  const std::vector<std::vector<std::vector<std::uint64_t>>> &vin_offsets = ptx_vector.vinOffsets();
  const std::vector<std::vector<std::uint64_t>> &construction_data_real_input_indices = ptx_vector.constructionDataRealOutputIndices();
  if (construction_data_real_input_indices.empty())
  {
    fail_msg_writer() << tr("failed to find construction data for tx input");
    return false;
  }
  std::vector<std::vector<std::unique_ptr<EnoteDetails>>> eds = ptx_vector.getEnoteDetailsIn();
  for (size_t n = 0; n < ptx_vector.txCount(); ++n)
  {
    if (verbose)
      ostr << boost::format(tr("\nTransaction %llu/%llu: txid=%s")) % (n + 1) % ptx_vector.txCount() % tx_ids[n];
    // for each input
    std::vector<uint64_t>     spent_key_height(eds[n].size());
    std::vector<std::string>  spent_key_txid  (eds[n].size());
    for (size_t i = 0; i < eds[n].size(); ++i)
    {
      if (verbose)
        ostr << boost::format(tr("\nInput %llu/%llu (%s): amount=%s")) % (i + 1) % eds[n].size() % eds[n][i]->keyImage() % print_money(eds[n][i]->amount());
      // convert relative offsets of ring member keys into absolute offsets (indices) associated with the amount
      std::vector<uint64_t> absolute_offsets = cryptonote::relative_output_offsets_to_absolute(vin_offsets[n][i]);
      // get block heights from which those ring member keys originated
      std::vector<std::pair<std::uint64_t, std::uint64_t>> req;
      req.resize(absolute_offsets.size());
      for (size_t j = 0; j < absolute_offsets.size(); ++j)
      {
        req[j].first = vin_amounts[n][i];
        req[j].second = absolute_offsets[j];
      }
      std::vector<std::string> ring_pub_keys;
      std::vector<std::string> _rct_key_mask;
      std::vector<bool> _unlocked;
      std::vector<std::uint64_t> ring_heights;
      std::vector<std::string> ring_tx_ids;
      bool r = m_wallet_manager->getOutsBin(req, /* do_get_txid */ true, ring_pub_keys, _rct_key_mask, _unlocked, ring_heights, ring_tx_ids);
      if (!r)
      {
        fail_msg_writer() << tr("failed to get output: ") << m_wallet_manager->errorString();
        return false;
      }
      // make sure that returned block heights are less than blockchain height
      for (auto& height : ring_heights)
      {
        if (height >= blockchain_height)
        {
          fail_msg_writer() << tr("output key's originating block height shouldn't be higher than the blockchain height");
          return false;
        }
      }
      if (verbose)
        ostr << tr("\nOriginating block heights: ");
      spent_key_height[i] = ring_heights[construction_data_real_input_indices[n][i]];
      spent_key_txid  [i] = ring_tx_ids[construction_data_real_input_indices[n][i]];
      std::pair<std::string, std::string> ring_str = show_outputs_line(ring_heights, blockchain_height, construction_data_real_input_indices[n][i]);
      if (verbose)
        ostr << ring_str.first << tr("\n|") << ring_str.second << tr("|\n");
    }
    // warn if rings contain keys originating from the same tx or temporally very close block heights
    bool are_keys_from_same_tx      = false;
    bool are_keys_from_close_height = false;
    for (size_t i = 0; i < eds[n].size(); ++i) {
      for (size_t j = i + 1; j < eds[n].size(); ++j)
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
        << tr(", which can break the anonymity of ring signatures. Make sure this is intentional!");
    }
    ostr << ENDL;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::prompt_if_old(const PendingTransaction &ptx)
{
  // count the number of old outputs
  std::string err;
  uint64_t bc_height = get_daemon_blockchain_height(err);
  if (!err.empty())
    return true;

  int max_n_old = 0;
  std::vector<std::vector<std::unique_ptr<EnoteDetails>>> eds = ptx.getEnoteDetailsIn();
  for (const auto &eds_: eds)
  {
    int n_old = 0;
    for (const auto &ed: eds_)
    {
      uint64_t age = bc_height - ed->blockHeight();
      if (age > OLD_AGE_WARN_THRESHOLD)
        ++n_old;
    }
    max_n_old = std::max(max_n_old, n_old);
  }
  if (max_n_old > 1)
  {
    std::stringstream prompt;
    prompt << tr("Transaction spends more than one very old output. Privacy would be better if they were sent separately.");
    prompt << ENDL << tr("Spend them now anyway?");
    std::string accepted = input_line(prompt.str(), true);
    if (std::cin.eof())
      return false;
    if (!command_line::is_yes(accepted))
    {
      return false;
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::check_for_inactivity_lock(bool user)
{
  if (m_locked)
  {
#ifdef HAVE_READLINE
    PAUSE_READLINE();
    rdln::clear_screen();
#endif
    tools::clear_screen();
    m_in_command = true;
    if (!user)
    {
      const std::string speech = tr("I locked your Monero wallet to protect you while you were away\nsee \"help set\" to configure/disable");
      std::vector<std::pair<std::string, size_t>> lines = tools::split_string_by_width(speech, 45);

      size_t max_len = 0;
      for (const auto &i: lines)
        max_len = std::max(max_len, i.second);
      const size_t n_u = max_len + 2;
      tools::msg_writer() << " " << std::string(n_u, '_');
      for (size_t i = 0; i < lines.size(); ++i)
        tools::msg_writer() << (i == 0 ? "/" : i == lines.size() - 1 ? "\\" : "|") << " " << lines[i].first << std::string(max_len - lines[i].second, ' ') << " " << (i == 0 ? "\\" : i == lines.size() - 1 ? "/" : "|");
      tools::msg_writer() << " " << std::string(n_u, '-') << std::endl <<
          "        \\   (__)" << std::endl <<
          "         \\  (oo)\\_______" << std::endl <<
          "            (__)\\       )\\/\\" << std::endl <<
          "                ||----w |" << std::endl <<
          "                ||     ||" << std::endl <<
          "" << std::endl;
    }

    bool started_background_sync = false;
    if (!m_wallet_impl->isBackgroundWallet() &&
        m_wallet_impl->getBackgroundSyncType() != Wallet::BackgroundSyncType::BackgroundSync_Off)
    {
      LOCK_IDLE_SCOPE();
      m_wallet_impl->startBackgroundSync();
      started_background_sync = true;
    }

    while (1)
    {
      const char *inactivity_msg = user ? "" : tr("Locked due to inactivity.");
      tools::msg_writer() << inactivity_msg << (inactivity_msg[0] ? " " : "") << (
        (m_wallet_impl->isBackgroundWallet() && m_wallet_impl->getBackgroundSyncType() == Wallet::BackgroundSyncType::BackgroundSync_CustomPassword)
            ? tr("The background password is required to unlock the console.")
            : tr("The wallet password is required to unlock the console.")
      );

      if (m_wallet_impl->isBackgroundSyncing())
        tools::msg_writer() << tr("\nSyncing in the background while locked...") << std::endl;

      const bool show_wallet_name = m_wallet_impl->getShowWalletNameWhenLocked();
      if (show_wallet_name)
      {
        tools::msg_writer() << tr("Filename: ") << m_wallet_impl->filename();
        tools::msg_writer() << tr("Network type: ") << (
          m_wallet_impl->nettype() == NetworkType::TESTNET ? tr("Testnet") :
          m_wallet_impl->nettype() == NetworkType::STAGENET ? tr("Stagenet") : tr("Mainnet")
        );
      }
      const auto pwd_container = get_and_verify_password();
      if (pwd_container)
      {
        if (started_background_sync)
        {
          LOCK_IDLE_SCOPE();
          m_wallet_impl->stopBackgroundSync(std::string(pwd_container->password().data(), pwd_container->password().size()));
        }
        break;
      }
    }
    m_last_activity_time = time(NULL);
    m_in_command = false;
    m_locked = false;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::on_command(bool (simple_wallet::*cmd)(const std::vector<std::string>&), const std::vector<std::string> &args)
{
  m_last_activity_time = time(NULL);

  m_in_command = true;
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
    m_last_activity_time = time(NULL);
    m_in_command = false;
  });

  check_for_inactivity_lock(false);
  return (this->*cmd)(args);
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer_main(const std::vector<std::string> &args_)
{
//  "transfer [index=<N1>[,<N2>,...]] [<priority>] [<ring_size>] <address> <amount>"
  CHECK_IF_BACKGROUND_SYNCING("cannot transfer");
  if (!try_connect_to_daemon())
    return false;

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (!parse_subaddress_indices(local_args[0], subaddr_indices))
      return false;
    local_args.erase(local_args.begin());
  }

  uint32_t priority = m_wallet_impl->getDefaultPriority();
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());


  size_t fake_outs_count = 0;
  if (!get_fake_outs_count(m_wallet_impl, local_args, fake_outs_count))
      return false;

  const size_t min_args = 1;
  if(local_args.size() < min_args)
  {
     fail_msg_writer() << tr("wrong number of arguments");
     return false;
  }

  std::vector<uint8_t> extra;
  bool payment_id_seen = false;
  if (!local_args.empty())
  {
    // we do not allow the deprecated payment_id argument anymore,
    // payment IDs are handled by integrated addresses
    if (is_long_payment_id(local_args.back()))
    {
      LONG_PAYMENT_ID_SUPPORT_CHECK();
    }
    else if (is_short_payment_id(local_args.back()))
    {
      SHORT_PAYMENT_ID_WITHOUT_INTEGRATED_ADDRESS_SUPPORT_CHECK();
    }
  }

  // Parse subtractfeefrom destination list
  std::set<uint32_t> subtract_fee_from_outputs;
  bool subtract_fee_from_all = false;
  for (auto it = local_args.begin(); it < local_args.end();)
  {
    bool matches = false;
    if (!parse_subtract_fee_from_outputs(*it, subtract_fee_from_outputs, subtract_fee_from_all, matches))
    {
      return false;
    }
    else if (matches)
    {
      it = local_args.erase(it);
      break;
    }
    else
    {
      ++it;
    }
  }

  vector<cryptonote::address_parse_info> dsts_info;
  vector<std::string> dsts;
  vector<uint64_t> amounts;
  network_type nettype = static_cast<network_type>(m_wallet_impl->nettype());
  for (size_t i = 0; i < local_args.size(); )
  {
    dsts_info.emplace_back();
    cryptonote::address_parse_info & info = dsts_info.back();
    bool r = true;

    // check for a URI
    std::string address_uri, payment_id_uri, tx_description, recipient_name, error;
    std::vector<std::string> unknown_parameters;
    uint64_t amount = 0;
    bool has_uri = m_wallet_impl->parse_uri(local_args[i], address_uri, payment_id_uri, amount, tx_description, recipient_name, unknown_parameters, error);
    if (has_uri)
    {
      r = cryptonote::get_account_address_from_str_or_url(info, nettype, address_uri, oa_prompter);
      if (is_long_payment_id(payment_id_uri))
      {
        LONG_PAYMENT_ID_SUPPORT_CHECK();
      }
      ++i;
    }
    else if (i + 1 < local_args.size())
    {
      r = cryptonote::get_account_address_from_str_or_url(info, nettype, local_args[i], oa_prompter);
      bool ok = cryptonote::parse_amount(amount, local_args[i + 1]);
      if(!ok || 0 == amount)
      {
        fail_msg_writer() << tr("amount is wrong: ") << local_args[i] << ' ' << local_args[i + 1] <<
          ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
        return false;
      }
      i += 2;
    }
    else
    {
      if (boost::starts_with(local_args[i], "monero:"))
        fail_msg_writer() << tr("Invalid last argument: ") << local_args.back() << ": " << error;
      else
        fail_msg_writer() << tr("Invalid last argument: ") << local_args.back();
      return false;
    }

    if (!r)
    {
      fail_msg_writer() << tr("failed to parse address");
      return false;
    }

    if (info.has_payment_id)
    {
      if (payment_id_seen)
      {
        fail_msg_writer() << tr("a single transaction cannot use more than one payment id");
        return false;
      }

      std::string extra_nonce;
      set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
      bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
      if(!r)
      {
        fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
        return false;
      }
      payment_id_seen = true;
    }

    if (info.has_payment_id)
        dsts.push_back(get_account_integrated_address_as_str(nettype, info.address, info.payment_id));
    else
        dsts.push_back(get_account_address_as_str(nettype, info.is_subaddress, info.address));
    amounts.push_back(amount);
  }

  if (subtract_fee_from_all)
  {
    subtract_fee_from_outputs.clear();
    for (decltype(subtract_fee_from_outputs)::value_type i = 0; i < dsts.size(); ++i)
      subtract_fee_from_outputs.insert(i);
  }

  SCOPED_WALLET_UNLOCK_ON_BAD_PASSWORD(return false;);

  // figure out what tx will be necessary
  // Note: short encrypted payment IDs are handled by integrated addresses
  // therefore we just ignore the argument here
  auto ptx = m_wallet_impl->createTransactionMultDest(dsts,
                                                      /* [deprecated] payment_id */ "",
                                                      amounts,
                                                      fake_outs_count,
                                                      static_cast<PendingTransaction::Priority>(priority),
                                                      m_current_subaddress_account,
                                                      subaddr_indices,
                                                      subtract_fee_from_outputs);

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to create transaction: ") << error_msg;
    return false;
  }

  if (ptx->txCount() == 0)
  {
    fail_msg_writer() << tr("No outputs found, or daemon is not ready");
    return false;
  }

  // if we need to check for backlog, check the worst case tx
  if (m_wallet_impl->getConfirmBacklog())
  {
    std::stringstream prompt;
    double worst_fee_per_byte = ptx->getWorstFeePerByte();

    std::vector<std::pair<uint64_t, uint64_t>> nblocks = m_wallet_impl->estimateBacklog({std::make_pair(worst_fee_per_byte, worst_fee_per_byte)});
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
    {
      prompt << error_msg << ENDL << tr("Is this okay anyway?");
    }
    else
    {
      if (nblocks.size() != 1)
      {
        prompt << "Internal error checking for backlog. " << tr("Is this okay anyway?");
      }
      else
      {
        if (nblocks[0].first > m_wallet_impl->getConfirmBacklogThreshold())
          prompt << (boost::format(tr("There is currently a %u block backlog at that fee level. Is this okay?")) % nblocks[0].first).str();
      }
    }
    std::string prompt_str = prompt.str();
    if (!prompt_str.empty())
    {
      std::string accepted = input_line(prompt_str, true);
      if (std::cin.eof())
        return false;
      if (!command_line::is_yes(accepted))
      {
        fail_msg_writer() << tr("transaction cancelled.");

        return false;
      }
    }
  }

  if (!prompt_if_old(*ptx))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return false;
  }

  // if more than one tx necessary, prompt user to confirm
  if (m_wallet_impl->getAlwaysConfirmTransfers() || ptx->txCount() > 1)
  {
      uint64_t total_sent = ptx->amount() + ptx->fee();
      uint64_t total_fee = ptx->fee();
      uint64_t total_change = ptx->change();
      uint64_t dust_not_in_fee = ptx->dust() - ptx->dustInFee();
      uint64_t dust_in_fee = ptx->dustInFee();
      std::vector<std::set<uint32_t>> tmp_subaddr_indices = ptx->subaddrIndices();

      std::stringstream prompt;
      for (size_t n = 0; n < ptx->txCount(); ++n)
      {
        prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx->txCount() << ":\n";
        subaddr_indices.clear();
        for (uint32_t i : tmp_subaddr_indices[n])
          subaddr_indices.insert(i);
        for (uint32_t i : subaddr_indices)
          prompt << boost::format(tr("Spending from address index %d\n")) % i;
        if (subaddr_indices.size() > 1)
          prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
      }
      prompt << boost::format(tr("Sending %s.  ")) % print_money(total_sent-total_fee);
      if (ptx->txCount() > 1)
      {
        prompt << boost::format(tr("Your transaction needs to be split into %llu transactions.  "
          "This will result in a transaction fee being applied to each transaction, for a total fee of %s")) %
          ((unsigned long long)ptx->txCount()) % print_money(total_fee);
      }
      else
      {
        prompt << boost::format(tr("The transaction fee is %s")) %
          print_money(total_fee);
      }
      if (dust_in_fee != 0) prompt << boost::format(tr(", of which %s is dust from change")) % print_money(dust_in_fee);
      if (dust_not_in_fee != 0)  prompt << tr(".") << ENDL << boost::format(tr("A total of %s from dust change will be sent to dust address"))
                                                 % print_money(dust_not_in_fee);
      prompt << boost::format(tr(". The overall total is %s")) %
          print_money(total_sent);
      prompt << boost::format(tr(". The change is %s.")) %
          print_money(total_change);
      if (!process_ring_members(*ptx, prompt, m_wallet_impl->getPrintRingMembers()))
        return false;
      prompt << ENDL << tr("Is this okay?");

      std::string accepted = input_line(prompt.str(), true);
      if (std::cin.eof())
        return false;
      if (!command_line::is_yes(accepted))
      {
        fail_msg_writer() << tr("transaction cancelled.");

        return false;
      }
  }
  // actually commit the transactions
  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (ms_status.isMultisig)
  {
    bool r = m_wallet_impl->saveMultisigTx(*ptx, "multisig_monero_tx");
    if (!r)
    {
      fail_msg_writer() << tr("Failed to write transaction(s) to file");
      return false;
    }
    else
    {
      success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
    }
  }
  else
  {
    if (m_wallet_impl->getDeviceState().has_tx_cold_sign
            && !ptx->confirmationMessage().empty()
            && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
    {
      fail_msg_writer() << tr("transaction cancelled.");
      return true;
    }
    commit_or_save(*ptx, m_do_not_relay);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::transfer(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot transfer");
  if (args_.size() < 1)
  {
    PRINT_USAGE(USAGE_TRANSFER);
    return true;
  }
  transfer_main(args_);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_main(uint32_t account, uint64_t below, const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sweep");
  auto print_usage = [this, account, below]()
  {
    if (below)
    {
      PRINT_USAGE(USAGE_SWEEP_BELOW);
    }
    else if (account == m_current_subaddress_account)
    {
      PRINT_USAGE(USAGE_SWEEP_ALL);
    }
    else
    {
      PRINT_USAGE(USAGE_SWEEP_ACCOUNT);
    }
  };
  if (args_.size() == 0)
  {
    fail_msg_writer() << tr("No address given");
    print_usage();
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
  {
    if (local_args[0] == "index=all")
    {
      for (uint32_t i = 0; i < m_wallet_impl->numSubaddresses(m_current_subaddress_account); ++i)
        subaddr_indices.insert(i);
    }
    else if (!parse_subaddress_indices(local_args[0], subaddr_indices))
    {
      print_usage();
      return true;
    }
    local_args.erase(local_args.begin());
  }

  uint32_t priority = m_wallet_impl->getDefaultPriority();
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());

  priority = m_wallet_impl->adjustPriority(priority);

  size_t fake_outs_count = 0;
  if (!get_fake_outs_count(m_wallet_impl, local_args, fake_outs_count))
      return true;

  size_t outputs = 1;
  if (local_args.size() > 0 && local_args[0].substr(0, 8) == "outputs=")
  {
    if (!epee::string_tools::get_xtype_from_string(outputs, local_args[0].substr(8)))
    {
      fail_msg_writer() << tr("Failed to parse number of outputs");
      return true;
    }
    else if (outputs < 1)
    {
      fail_msg_writer() << tr("Amount of outputs should be greater than 0");
      return true;
    }
    else
    {
      local_args.erase(local_args.begin());
    }
  }

  std::vector<uint8_t> extra;
  if (local_args.size() >= 2)
  {
    // we do not allow the deprecated payment_id argument anymore,
    // payment IDs are handled by integrated addresses
    if(is_long_payment_id(local_args.back()))
    {
      LONG_PAYMENT_ID_SUPPORT_CHECK();
    }
    else if (is_short_payment_id(local_args.back()))
    {
      SHORT_PAYMENT_ID_WITHOUT_INTEGRATED_ADDRESS_SUPPORT_CHECK();
    }
  }

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, static_cast<network_type>(m_wallet_impl->nettype()), local_args[0], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    print_usage();
    return true;
  }
  std::string destination_addr = local_args[0];

  if (info.has_payment_id)
  {
    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    if (!add_extra_nonce_to_tx_extra(extra, extra_nonce))
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
  }

  SCOPED_WALLET_UNLOCK();

  // figure out what tx will be necessary
  // Note: short encrypted payment IDs are handled by integrated addresses,
  // therefore we just ignore the argument here
  auto ptx = m_wallet_impl->createTransactionMultDest({destination_addr},
                                                      /* [deprecated] payment_id */ "",
                                                      /* amount */ Monero::optional<std::vector<uint64_t>>(),
                                                      fake_outs_count,
                                                      static_cast<PendingTransaction::Priority>(priority),
                                                      account,
                                                      subaddr_indices,
                                                      /* subtract_fee_from_outputs */ {},
                                                      /* key_image */ "",
                                                      outputs,
                                                      below);
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to create sweep transaction: ") << error_msg;
    return true;
  }

  if (ptx->txCount() == 0)
  {
    fail_msg_writer() << tr("No outputs found, or daemon is not ready");
    return true;
  }

  if (!prompt_if_old(*ptx))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }

  // give user total and fee, and prompt to confirm
  uint64_t total_fee = ptx->fee();
  uint64_t total_sent = ptx->amount();
  std::vector<std::set<uint32_t>> tmp_subaddr_indices = ptx->subaddrIndices();
  std::ostringstream prompt;
  for (size_t n = 0; n < ptx->txCount(); ++n)
  {
    prompt << tr("\nTransaction ") << (n + 1) << "/" << ptx->txCount() << ":\n";
    subaddr_indices.clear();
    for (uint32_t i : tmp_subaddr_indices[n])
      subaddr_indices.insert(i);
    for (uint32_t i : subaddr_indices)
      prompt << boost::format(tr("Spending from address index %d\n")) % i;
    if (subaddr_indices.size() > 1)
      prompt << tr("WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.\n");
  }
  if (!process_ring_members(*ptx, prompt, m_wallet_impl->getPrintRingMembers()))
    return true;
  if (ptx->txCount() > 1) {
    prompt << boost::format(tr("Sweeping %s in %llu transactions for a total fee of %s, overall total %s.  Is this okay?")) %
      print_money(total_sent) %
      ((unsigned long long)ptx->txCount()) %
      print_money(total_fee) %
      print_money(total_sent+total_fee);
  }
  else {
    prompt << boost::format(tr("Sweeping %s for a total fee of %s, overall total %s.  Is this okay?")) %
      print_money(total_sent) %
      print_money(total_fee) %
      print_money(total_sent+total_fee);
  }
  std::string accepted = input_line(prompt.str(), true);
  if (std::cin.eof())
    return true;
  if (!command_line::is_yes(accepted))
  {
    fail_msg_writer() << tr("transaction cancelled.");

    return true;
  }

  // actually commit the transactions
  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (ms_status.isMultisig)
  {
    bool r = m_wallet_impl->saveMultisigTx(*ptx, "multisig_monero_tx");
    if (!r)
    {
      fail_msg_writer() << tr("Failed to write transaction(s) to file");
      return false;
    }
    else
    {
      success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
    }
  }
  else
  {
    if (m_wallet_impl->getDeviceState().has_tx_cold_sign
            && !ptx->confirmationMessage().empty()
            && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
    {
      fail_msg_writer() << tr("transaction cancelled.");
      return true;
    }
    commit_or_save(*ptx, m_do_not_relay);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_single(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sweep");
  if (!try_connect_to_daemon())
    return true;

  std::vector<std::string> local_args = args_;

  uint32_t priority = m_wallet_impl->getDefaultPriority();
  if (local_args.size() > 0 && parse_priority(local_args[0], priority))
    local_args.erase(local_args.begin());

  priority = m_wallet_impl->adjustPriority(priority);

  size_t fake_outs_count = 0;
  if (!get_fake_outs_count(m_wallet_impl, local_args, fake_outs_count))
      return true;

  size_t outputs = 1;
  if (local_args.size() > 0 && local_args[0].substr(0, 8) == "outputs=")
  {
    if (!epee::string_tools::get_xtype_from_string(outputs, local_args[0].substr(8)))
    {
      fail_msg_writer() << tr("Failed to parse number of outputs");
      return true;
    }
    else if (outputs < 1)
    {
      fail_msg_writer() << tr("Amount of outputs should be greater than 0");
      return true;
    }
    else
    {
      local_args.erase(local_args.begin());
    }
  }

  std::vector<uint8_t> extra;
  if (local_args.size() == 3)
  {
    // we do not allow the deprecated payment_id argument anymore,
    // payment IDs are handled by integrated addresses
    if (is_long_payment_id(local_args.back()))
    {
      LONG_PAYMENT_ID_SUPPORT_CHECK();
    }
    else if (is_short_payment_id(local_args.back()))
    {
      SHORT_PAYMENT_ID_WITHOUT_INTEGRATED_ADDRESS_SUPPORT_CHECK();
    }
  }

  if (local_args.size() != 2)
  {
    PRINT_USAGE(USAGE_SWEEP_SINGLE);
    return true;
  }

  crypto::key_image ki;
  if (!epee::string_tools::hex_to_pod(local_args[0], ki))
  {
    fail_msg_writer() << tr("failed to parse key image");
    return true;
  }
  std::string key_image = local_args[0];

  cryptonote::address_parse_info info;
  if (!cryptonote::get_account_address_from_str_or_url(info, static_cast<network_type>(m_wallet_impl->nettype()), local_args[1], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }
  std::string destination_addr = local_args[1];

  if (info.has_payment_id)
  {
    std::string extra_nonce;
    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
    if (!add_extra_nonce_to_tx_extra(extra, extra_nonce))
    {
      fail_msg_writer() << tr("failed to set up payment id, though it was decoded correctly");
      return true;
    }
  }

  SCOPED_WALLET_UNLOCK();

  // figure out what tx will be necessary
  // Note: short encrypted payment IDs are handled by integrated addresses
  // therefore we just ignore the argument here
  auto ptx = m_wallet_impl->createTransactionMultDest({destination_addr},
                                                      /* [deprecated] payment_id */ "",
                                                      /* amount */ Monero::optional<std::vector<uint64_t>>(),
                                                      fake_outs_count,
                                                      static_cast<PendingTransaction::Priority>(priority),
                                                      m_current_subaddress_account,
                                                      /* subaddr_indices */ {},
                                                      /* subtract_fee_from_outputs */ {},
                                                      key_image,
                                                      outputs,
                                                      /* below */ 0);

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("failed to create sweep single transaction: ") << error_msg;
    return true;
  }

  if (ptx->txCount() == 0)
  {
    fail_msg_writer() << tr("No outputs found");
    return true;
  }
  if (ptx->txCount() > 1)
  {
    fail_msg_writer() << tr("Multiple transactions are created, which is not supposed to happen");
    return true;
  }
  if (ptx->getEnoteDetailsIn().size() != 1 || ptx->getEnoteDetailsIn()[0].size() != 1)
  {
    fail_msg_writer() << tr("The transaction uses multiple or no inputs, which is not supposed to happen");
    return true;
  }
  uint64_t total_fee = ptx->fee();
  uint64_t total_sent = ptx->amount();
  std::vector<std::set<uint32_t>> tmp_subaddr_indices = ptx->subaddrIndices();
  std::ostringstream prompt;
  if (!process_ring_members(*ptx, prompt, m_wallet_impl->getPrintRingMembers()))
    return true;
  prompt << boost::format(tr("Sweeping %s for a total fee of %s, overall total %s.  Is this okay?")) %
    print_money(total_sent) %
    print_money(total_fee) %
    print_money(total_sent+total_fee);
  std::string accepted = input_line(prompt.str(), true);
  if (std::cin.eof())
    return true;
  if (!command_line::is_yes(accepted))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }

  // actually commit the transactions
  const MultisigState ms_status{m_wallet_impl->multisig()};
  if (ms_status.isMultisig)
  {
    bool r = m_wallet_impl->saveMultisigTx(*ptx, "multisig_monero_tx");
    if (!r)
    {
      fail_msg_writer() << tr("Failed to write transaction(s) to file");
      return false;
    }
    else
    {
      success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "multisig_monero_tx";
    }
  }
  else
  {
    if (m_wallet_impl->getDeviceState().has_tx_cold_sign
            && !ptx->confirmationMessage().empty()
            && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
    {
      fail_msg_writer() << tr("transaction cancelled.");
      return true;
    }
    commit_or_save(*ptx, m_do_not_relay);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_all(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sweep");
  sweep_main(m_current_subaddress_account, 0, args_);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_account(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sweep");
  auto local_args = args_;
  if (local_args.empty())
  {
    PRINT_USAGE(USAGE_SWEEP_ACCOUNT);
    return true;
  }
  uint32_t account = 0;
  if (!epee::string_tools::get_xtype_from_string(account, local_args[0]))
  {
    fail_msg_writer() << tr("Invalid account");
    return true;
  }
  local_args.erase(local_args.begin());

  sweep_main(account, 0, local_args);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sweep_below(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sweep");
  uint64_t below = 0;
  if (args_.size() < 1)
  {
    fail_msg_writer() << tr("missing threshold amount");
    PRINT_USAGE(USAGE_SWEEP_BELOW);
    return true;
  }
  if (!cryptonote::parse_amount(below, args_[0]))
  {
    fail_msg_writer() << tr("invalid amount threshold");
    return true;
  }
  sweep_main(m_current_subaddress_account, below, std::vector<std::string>(++args_.begin(), args_.end()));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::donate(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot donate");
  std::vector<std::string> local_args = args_;
  if(local_args.empty() || local_args.size() > 5)
  {
     PRINT_USAGE(USAGE_DONATE);
     return true;
  }
  std::string amount_str;
  if (is_long_payment_id(local_args.back()))
  {
    LONG_PAYMENT_ID_SUPPORT_CHECK();
  }
  else if (is_short_payment_id(local_args.back()))
  {
    SHORT_PAYMENT_ID_WITHOUT_INTEGRATED_ADDRESS_SUPPORT_CHECK();
  }
  // get amount and pop
  uint64_t amount;
  bool ok = cryptonote::parse_amount(amount, local_args.back());
  if (ok && amount != 0)
  {
    amount_str = local_args.back();
    local_args.pop_back();
  }
  else
  { 
    fail_msg_writer() << tr("amount is wrong: ") << local_args.back() << ", " << tr("expected number from 0 to ") << print_money(std::numeric_limits<uint64_t>::max());
    return true;
  }
  // push back address, amount, payment id
  std::string address_str;
  NetworkType nettype = m_wallet_impl->nettype();
  if (nettype != NetworkType::MAINNET)
  {
    // if not mainnet, convert donation address string to the relevant network type
    address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, static_cast<network_type>(NetworkType::MAINNET), MONERO_DONATION_ADDR))
    {
      fail_msg_writer() << tr("Failed to parse donation address: ") << MONERO_DONATION_ADDR;
      return true;
    }
    address_str = cryptonote::get_account_address_as_str(static_cast<network_type>(nettype), info.is_subaddress, info.address);
  }
  else
  {
    address_str = MONERO_DONATION_ADDR;
  }
  local_args.push_back(address_str);
  local_args.push_back(amount_str);
  if (nettype == NetworkType::MAINNET)
    message_writer() << (boost::format(tr("Donating %s %s to The Monero Project (donate.getmonero.org or %s).")) % amount_str % cryptonote::get_unit(cryptonote::get_default_decimal_point()) % MONERO_DONATION_ADDR).str();
  else
    message_writer() << (boost::format(tr("Donating %s %s to %s.")) % amount_str % cryptonote::get_unit(cryptonote::get_default_decimal_point()) % address_str).str();
  transfer(local_args);
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign_transfer(const std::vector<std::string> &args_)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(m_wallet_impl->multisig().isMultisig)
  {
     fail_msg_writer() << tr("This is a multisig wallet, it can only sign with sign_multisig");
     return true;
  }
  if(m_wallet_impl->watchOnly())
  {
     fail_msg_writer() << tr("This is a watch only wallet");
     return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot sign transfer");

  bool export_raw = false;
  std::string unsigned_filename = "unsigned_monero_tx";
  const std::string signed_filename = "signed_monero_tx";
  if (args_.size() > 2 || (args_.size() == 2 && args_[0] != "export_raw"))
  {
    PRINT_USAGE(USAGE_SIGN_TRANSFER);
    return true;
  }
  else if (args_.size() == 2)
  {
    export_raw = true;
    unsigned_filename = args_[1];
  }
  else if (args_.size() == 1)
  {
    if (args_[0] == "export_raw")
      export_raw = true;
    else
      unsigned_filename = args_[0];
  }

  SCOPED_WALLET_UNLOCK();

  UnsignedTransaction *unsigned_tx = m_wallet_impl->loadUnsignedTx(unsigned_filename);
  if (unsigned_tx->status() != UnsignedTransaction::Status_Ok)
  {
    fail_msg_writer() << unsigned_tx->errorString();
    return true;
  }
  // Confirm loaded tx
  if (!unsigned_tx->confirmationMessage().empty() && !command_line::is_yes(input_line(unsigned_tx->confirmationMessage(), /* yesno */ true)))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }

  std::vector<std::string> tx_ids;
  if (!unsigned_tx->sign(signed_filename, export_raw, &tx_ids))
  {
    fail_msg_writer() << unsigned_tx->errorString();
    return true;
  }

  std::string txids_as_text;
  for (const auto &tx_id: tx_ids)
  {
    if (!txids_as_text.empty())
      txids_as_text += (", ");
    txids_as_text += tx_id;
  }
  success_msg_writer(true) << tr("Transaction successfully signed to file ") << "signed_monero_tx" << ", txid " << txids_as_text;
  if (export_raw)
  {
    std::string rawfiles_as_text;
    for (size_t i = 0; i < tx_ids.size(); ++i)
    {
      if (i > 0)
        rawfiles_as_text += ", ";
      rawfiles_as_text += signed_filename + "_raw" + (tx_ids.size() == 1 ? "" : ("_" + std::to_string(i)));
    }
    success_msg_writer(true) << tr("Transaction raw hex data exported to ") << rawfiles_as_text;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::submit_transfer(const std::vector<std::string> &args_)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (!try_connect_to_daemon())
    return true;

  const std::string signed_monero_tx_file = "signed_monero_tx";
  std::string signed_monero_tx_str;
  if (!load_from_file(signed_monero_tx_file, signed_monero_tx_str))
  {
    fail_msg_writer() << tr("failed to load ") << "`" << signed_monero_tx_file << "`";
    return true;
  }

  int error_code;
  std::string error_msg;
  PendingTransaction *ptx = m_wallet_impl->parseTxFromStr(signed_monero_tx_str);

  if (!ptx)
  {
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return true;
  }
  // Confirm loaded tx
  if (!ptx->confirmationMessage().empty() && !command_line::is_yes(input_line(ptx->confirmationMessage(), /* yesno */ true)))
  {
    fail_msg_writer() << tr("transaction cancelled.");
    return true;
  }
  ptx->finishParsingTx();

  commit_or_save(*ptx, /* do_not_relay */ false, /* do_force_commit */ true);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_key(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get tx key");

  std::vector<std::string> local_args = args_;

  if (m_wallet_impl->getDeviceState().key_on_device
      && m_wallet_impl->getDeviceState().device_type != Wallet::DeviceState::DeviceType::DeviceType_Trezor)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(local_args.size() != 1) {
    PRINT_USAGE(USAGE_GET_TX_KEY);
    return true;
  }

  SCOPED_WALLET_UNLOCK();

  std::string tx_key_stream = m_wallet_impl->getTxKey(args_[0]);
  if (tx_key_stream.empty())
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return true;
  }

  success_msg_writer() << tr("Tx key: ") << tx_key_stream;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_tx_key(const std::vector<std::string> &args_)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot set tx key");

  std::vector<std::string> local_args = args_;

  if(local_args.size() != 2 && local_args.size() != 3) {
    PRINT_USAGE(USAGE_SET_TX_KEY);
    return true;
  }

  std::string single_destination_subaddress;
  if (local_args.size() > 1)
  {
    cryptonote::address_parse_info info;
    if (cryptonote::get_account_address_from_str_or_url(info, static_cast<network_type>(m_wallet_impl->nettype()), local_args.back(), oa_prompter))
    {
      if (!info.is_subaddress)
      {
        fail_msg_writer() << tr("Last argument is an address, but not a subaddress");
        return true;
      }
      single_destination_subaddress = local_args.back();
      local_args.pop_back();
    }
  }

  std::string tx_id = local_args[0];
  std::string tx_key = local_args[1].substr(0, 64);
  std::vector<std::string> additional_tx_keys;
  while(true)
  {
    // additional_tx_keys are concatenated onto tx_keys, each one is 32 bytes (64 char hex string)
    // local_args[1]: "tx_key [| additional_tx_key_1 | ... | additional_tx_key_n ]"
    local_args[1] = local_args[1].substr(64);
    if (local_args[1].empty())
      break;
    additional_tx_keys.push_back(local_args[1].substr(0, 64));
  }

  LOCK_IDLE_SCOPE();

  m_wallet_impl->setTxKey(tx_id, tx_key, additional_tx_keys, single_destination_subaddress);

  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << tr("Failed to store tx key: ") << error_msg;
    if (single_destination_subaddress.empty())
      fail_msg_writer() << tr("It could be because the transfer was to a subaddress. If this is the case, pass the subaddress last");
    return true;
  }

  success_msg_writer() << tr("Tx key successfully stored.");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_proof(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get tx proof");

  if (args.size() != 2 && args.size() != 3)
  {
    PRINT_USAGE(USAGE_GET_TX_PROOF);
    return true;
  }

  SCOPED_WALLET_UNLOCK();

  std::string sig_str = m_wallet_impl->getTxProof(/* txid */ args[0],
                                                  /* address */ args[1],
                                                  /* message */ args.size() == 3 ? args[2] : "");
  if (sig_str.empty())
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to get tx proof: ") << error_msg;
    return true;
  }
  const std::string filename = "monero_tx_proof";
  if (save_to_file(filename, sig_str, /* is_printable */ true))
    success_msg_writer() << tr("signature file saved to: ") << filename;
  else
    fail_msg_writer() << tr("failed to save signature file");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_key(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() != 3) {
    PRINT_USAGE(USAGE_CHECK_TX_KEY);
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  if (!m_wallet_impl)
  {
    fail_msg_writer() << tr("wallet is null");
    return true;
  }

  std::string tx_id = local_args[0];
  std::string tx_key = local_args[1].substr(0, 64);
  std::vector<std::string> additional_tx_keys;
  while(true)
  {
    // additional_tx_keys are concatenated onto tx_keys, each one is 32 bytes (64 char hex string)
    // local_args[1]: "tx_key [| additional_tx_key_1 | ... | additional_tx_key_n ]"
    local_args[1] = local_args[1].substr(64);
    if (local_args[1].empty())
      break;
    additional_tx_keys.push_back(local_args[1].substr(0, 64));
  }

  cryptonote::address_parse_info info;
  network_type nettype = static_cast<network_type>(m_wallet_impl->nettype());
  if(!cryptonote::get_account_address_from_str_or_url(info, nettype, local_args[2], oa_prompter))
  {
    fail_msg_writer() << tr("failed to parse address");
    return true;
  }

  string address = epee::string_tools::pod_to_hex(info.address);
  uint64_t received, confirmations;
  bool is_in_pool;
  if (!m_wallet_impl->checkTxKey(tx_id,
                                 tx_key,
                                 address,
                                 received,
                                 is_in_pool,
                                 confirmations))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to check tx proof: ") << error_msg;
    return true;
  }

  if (received > 0)
  {
    success_msg_writer() << get_account_address_as_str(nettype, info.is_subaddress, info.address) << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << tx_id;
    if (is_in_pool)
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
    fail_msg_writer() << get_account_address_as_str(nettype, info.is_subaddress, info.address) << " " << tr("received nothing in txid") << " " << tx_id;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_tx_proof(const std::vector<std::string> &args)
{
  if(args.size() != 3 && args.size() != 4) {
    PRINT_USAGE(USAGE_CHECK_TX_PROOF);
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  std::string sig_str;
  // read signature string
  if (args[2].substr(0, 7) == "InProof" || args[2].substr(0, 8) == "OutProof")
      sig_str = args[2];
  else
  {
    // read signature file
    if (!load_from_file(args[2], sig_str))
    {
      fail_msg_writer() << tr("failed to load signature file");
      return true;
    }
    // strip whitespace
    sig_str.erase(remove_if(sig_str.begin(), sig_str.end(), ::isspace), sig_str.end());
  }

  string txid = args[0];
  string address = args[1];
  uint64_t received, confirmations;
  bool is_proof_verified, is_in_pool;
  if (!m_wallet_impl->checkTxProof(txid,
                                   address,
                                   /* message */ args.size() == 4 ? args[3] : "",
                                   sig_str,
                                   is_proof_verified,
                                   received,
                                   is_in_pool,
                                   confirmations))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to check tx proof: ") << error_msg;
    return true;
  }
  if (is_proof_verified)
  {
    success_msg_writer() << tr("Good signature");
    if (received > 0)
    {
      success_msg_writer() << address << " " << tr("received") << " " << print_money(received) << " " << tr("in txid") << " " << txid;
      if (is_in_pool)
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
      fail_msg_writer() << address << " " << tr("received nothing in txid") << " " << txid;
    }
  }
  else
  {
    fail_msg_writer() << tr("Bad signature");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_spend_proof(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get spend proof");
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(args.size() != 1 && args.size() != 2) {
    PRINT_USAGE(USAGE_GET_SPEND_PROOF);
    return true;
  }

  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot generate the proof");
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  SCOPED_WALLET_UNLOCK();

  const std::string sig_str = m_wallet_impl->getSpendProof(/* txid */ args[0],
                                                           /* message */ args.size() == 2 ? args[1] : "");
  if (sig_str.empty())
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to get spend proof: ") << error_msg;
    return true;
  }
  const std::string filename = "monero_spend_proof";
  if (save_to_file(filename, sig_str, true))
    success_msg_writer() << tr("signature file saved to: ") << filename;
  else
    fail_msg_writer() << tr("failed to save signature file");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_spend_proof(const std::vector<std::string> &args)
{
  if(args.size() != 2 && args.size() != 3) {
    PRINT_USAGE(USAGE_CHECK_SPEND_PROOF);
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  std::string sig_str;
  if (!load_from_file(args[1], sig_str))
  {
    fail_msg_writer() << tr("failed to load signature file");
    return true;
  }
  // strip whitespace
  sig_str.erase(remove_if(sig_str.begin(), sig_str.end(), ::isspace), sig_str.end());

  bool is_proof_verified;
  if (!m_wallet_impl->checkSpendProof(/* txid */ args[0],
                                      /* message */ args.size() == 3 ? args[2] : "",
                                      sig_str,
                                      is_proof_verified))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to check spend proof: ") << error_msg;
    return true;
  }
  if (is_proof_verified)
    success_msg_writer() << tr("Good signature");
  else
    fail_msg_writer() << tr("Bad signature");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_reserve_proof(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get reserve proof");
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if(args.size() != 1 && args.size() != 2) {
    PRINT_USAGE(USAGE_GET_RESERVE_PROOF);
    return true;
  }

  if (m_wallet_impl->watchOnly() || m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("The reserve proof can be generated only by a full wallet");
    return true;
  }

  uint64_t amount;
  if (args[0] != "all")
  {
    if (!cryptonote::parse_amount(amount, args[0]))
    {
      fail_msg_writer() << tr("amount is wrong: ") << args[0];
      return true;
    }
  }

  if (!try_connect_to_daemon())
    return true;

  SCOPED_WALLET_UNLOCK();

  const std::string sig_str = m_wallet_impl->getReserveProof(args[0] == "all",
                                                             m_current_subaddress_account,
                                                             amount,
                                                             args.size() == 2 ? args[1] : "");
  if (sig_str.empty())
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to get reserve proof: ") << error_msg;
    return true;
  }
  const std::string filename = "monero_reserve_proof";
  if (save_to_file(filename, sig_str, /* is_printable */ true))
    success_msg_writer() << tr("signature file saved to: ") << filename;
  else
    fail_msg_writer() << tr("failed to save signature file");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_reserve_proof(const std::vector<std::string> &args)
{
  if(args.size() != 2 && args.size() != 3) {
    PRINT_USAGE(USAGE_CHECK_RESERVE_PROOF);
    return true;
  }

  if (!try_connect_to_daemon())
    return true;

  std::string sig_str;
  if (!load_from_file(args[1], sig_str))
  {
    fail_msg_writer() << tr("failed to load signature file");
    return true;
  }
  // strip whitespace
  sig_str.erase(remove_if(sig_str.begin(), sig_str.end(), ::isspace), sig_str.end());

  LOCK_IDLE_SCOPE();

  uint64_t total, spent;
  bool is_proof_verified;
  if (!m_wallet_impl->checkReserveProof(/* address */ args[0],
                                        /* message */ args.size() == 3 ? args[2] : "",
                                        sig_str,
                                        is_proof_verified,
                                        total,
                                        spent))
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << tr("failed to check reserve proof: ") << error_msg;
    return true;
  }
  if (is_proof_verified)
  {
    success_msg_writer() << boost::format(tr("Good signature -- total: %s, spent: %s, unspent: %s")) % print_money(total) % print_money(spent) % print_money(total - spent);
  }
  else
  {
    fail_msg_writer() << tr("Bad signature");
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
static std::string get_human_readable_timespan(std::chrono::seconds seconds)
{
  uint64_t ts = seconds.count();
  if (ts < 60)
    return std::to_string(ts) + sw::tr(" seconds");
  if (ts < 3600)
    return std::to_string((uint64_t)(ts / 60)) + sw::tr(" minutes");
  if (ts < 3600 * 24)
    return std::to_string((uint64_t)(ts / 3600)) + sw::tr(" hours");
  if (ts < 3600 * 24 * 30.5)
    return std::to_string((uint64_t)(ts / (3600 * 24))) + sw::tr(" days");
  if (ts < 3600 * 24 * 365.25)
    return std::to_string((uint64_t)(ts / (3600 * 24 * 30.5))) + sw::tr(" months");
  return sw::tr("a long time");
}
//----------------------------------------------------------------------------------------------------
static std::string get_human_readable_timespan(uint64_t seconds)
{
  return get_human_readable_timespan(std::chrono::seconds(seconds));
}
//----------------------------------------------------------------------------------------------------
// mutates local_args as it parses and consumes arguments
bool simple_wallet::get_transfers(std::vector<std::string>& local_args, std::vector<transfer_view>& transfers)
{
  bool in = true;
  bool out = true;
  bool pending = true;
  bool failed = true;
  bool pool = true;
  bool coinbase = true;
  uint64_t min_height = 0;
  uint64_t max_height = (uint64_t)-1;

  // optional in/out selector
  if (local_args.size() > 0) {
    if (local_args[0] == "in" || local_args[0] == "incoming") {
      out = pending = failed = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "out" || local_args[0] == "outgoing") {
      in = pool = coinbase = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pending") {
      in = out = failed = coinbase = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "failed") {
      in = out = pending = pool = coinbase = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "pool") {
      in = out = pending = failed = coinbase = false;
      local_args.erase(local_args.begin());
    }
    else if (local_args[0] == "coinbase") {
      in = out = pending = failed = pool = false;
      coinbase = true;
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
      return false;
    local_args.erase(local_args.begin());
  }

  // min height
  if (local_args.size() > 0 && local_args[0].find('=') == std::string::npos) {
    try {
      min_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (const boost::bad_lexical_cast &) {
      fail_msg_writer() << tr("bad min_height parameter:") << " " << local_args[0];
      return false;
    }
    local_args.erase(local_args.begin());
  }

  // max height
  if (local_args.size() > 0 && local_args[0].find('=') == std::string::npos) {
    try {
      max_height = boost::lexical_cast<uint64_t>(local_args[0]);
    }
    catch (const boost::bad_lexical_cast &) {
      fail_msg_writer() << tr("bad max_height parameter:") << " " << local_args[0];
      return false;
    }
    local_args.erase(local_args.begin());
  }

  const uint64_t last_block_height = m_wallet_impl->blockChainHeight();

  TransactionHistory *tx_history = m_wallet_impl->history();
  tx_history->refresh();
  if (in || coinbase) {
    std::vector<TransactionInfo *> payments;
    for (const auto &tx_info : tx_history->getAll())
    {
      if (tx_info->direction() == Monero::TransactionInfo::Direction_In
              && min_height < tx_info->blockHeight() && max_height >= tx_info->blockHeight()
              && m_current_subaddress_account == tx_info->subaddrAccount()
              && (subaddr_indices.empty() || subaddr_indices.count(*tx_info->subaddrIndex().begin()) == 1))
        payments.push_back(tx_info);
    }
    for (const auto &tx_info : payments) {
      if (!tx_info->isCoinbase() && !in)
        continue;
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string destination = m_wallet_impl->address(m_current_subaddress_account, *tx_info->subaddrIndex().begin());
      const std::string type = tx_info->isCoinbase() ? tr("block") : tr("in");
      const bool unlocked = tx_info->isUnlocked();
      std::string locked_msg = "unlocked";
      if (!unlocked)
      {
        locked_msg = "locked";
        if (tx_info->unlockTime() < CRYPTONOTE_MAX_BLOCK_NUMBER)
        {
          uint64_t bh = std::max(tx_info->unlockTime(), tx_info->blockHeight() + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
          if (bh >= last_block_height)
            locked_msg = std::to_string(bh - last_block_height) + " blks";
        }
        else
        {
          const uint64_t adjusted_time = m_wallet_impl->getDaemonAdjustedTime();
          uint64_t threshold = adjusted_time + (m_wallet_impl->useForkRules(2, 0) ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2 : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1);
          if (threshold < tx_info->unlockTime())
            locked_msg = get_human_readable_timespan(std::chrono::seconds(tx_info->unlockTime() - threshold));
        }
      }
      crypto::hash tx_id;
      epee::string_tools::hex_to_pod(tx_info->hash(), tx_id);
      transfers.push_back({
        type,
        tx_info->blockHeight(),
        (uint64_t) tx_info->timestamp(),
        /* direction */ type,
        /* confirmed */ true,
        tx_info->amount(),
        tx_id,
        payment_id,
        /* fee */ 0,
        {{destination, tx_info->amount()}},
        tx_info->subaddrIndex(),
        tx_info->description(),
        locked_msg
      });
    }
  }

  if (out) {
    std::vector<TransactionInfo *> payments;
    for (const auto &tx_info : tx_history->getAll())
    {
      if (tx_info->direction() == Monero::TransactionInfo::Direction_Out
              && tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Confirmed
              && min_height < tx_info->blockHeight() && max_height >= tx_info->blockHeight()
              && m_current_subaddress_account == tx_info->subaddrAccount()
              && (subaddr_indices.empty() || std::count_if(tx_info->subaddrIndex().begin(), tx_info->subaddrIndex().end(), [&subaddr_indices](uint32_t index) { return subaddr_indices.count(index) == 1; }) != 0))
          payments.push_back(tx_info);
    }
    for (const auto &tx_info : payments) {
      std::vector<std::pair<std::string, uint64_t>> destinations;
      for (const auto &d: tx_info->transfers()) {
        destinations.push_back({d.address, d.amount});
      }
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      crypto::hash tx_id;
      epee::string_tools::hex_to_pod(tx_info->hash(), tx_id);
      transfers.push_back({
        /* type */ "out",
        tx_info->blockHeight(),
        (uint64_t) tx_info->timestamp(),
        /* direction */ "out",
        /* confirmed */ true,
        tx_info->amount(),
        tx_id,
        payment_id,
        tx_info->fee(),
        destinations,
        tx_info->subaddrIndex(),
        tx_info->description(),
        /* locked_msg */ "-"
      });
    }
  }

  if (pool) {
    std::vector<TransactionInfo *> payments;
    for (const auto &tx_info : tx_history->getAll())
    {
      if (tx_info->direction() == Monero::TransactionInfo::Direction_In
              && tx_info->txState() == Monero::TransactionInfo::TxState::TxState_PendingInPool
              && m_current_subaddress_account == tx_info->subaddrAccount()
              && (subaddr_indices.empty() || subaddr_indices.count(*tx_info->subaddrIndex().begin()) == 1))
        payments.push_back(tx_info);
    }
    for (const auto &tx_info : payments) {
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      std::string destination = m_wallet_impl->address(m_current_subaddress_account, *tx_info->subaddrIndex().begin());
      std::string double_spend_note;
      if (tx_info->isDoubleSpendSeen())
        double_spend_note = tr("[Double spend seen on the network: this transaction may or may not end up being mined] ");
      crypto::hash tx_id;
      epee::string_tools::hex_to_pod(tx_info->hash(), tx_id);
      transfers.push_back({
        /* type */ "pool",
        /* block_height */ "pool",
        (uint64_t) tx_info->timestamp(),
        /* direction */ "in",
        /* confirmed */ false,
        tx_info->amount(),
        tx_id,
        payment_id,
        /* fee */ 0,
        {{destination, tx_info->amount()}},
        tx_info->subaddrIndex(),
        tx_info->description() + double_spend_note,
        /* locked_msg */ "locked"
      });
    }
  }

  // print unconfirmed last
  if (pending || failed) {
    std::vector<TransactionInfo *> payments;
    for (const auto &tx_info : tx_history->getAll())
    {
      if (tx_info->direction() == Monero::TransactionInfo::Direction_Out
              && (tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Pending
              ||  tx_info->txState() == Monero::TransactionInfo::TxState::TxState_PendingInPool
              ||  tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Failed)
              && m_current_subaddress_account == tx_info->subaddrAccount()
              && (subaddr_indices.empty()
              ||  std::count_if(tx_info->subaddrIndex().begin(),
                                tx_info->subaddrIndex().end(),
                                [&subaddr_indices](uint32_t index) { return subaddr_indices.count(index) == 1; }) != 0))
        payments.push_back(tx_info);
    }
    for (const auto &tx_info : payments) {
      std::vector<std::pair<std::string, uint64_t>> destinations;
      for (const auto &d: tx_info->transfers()) {
        destinations.push_back({d.address, d.amount});
      }
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      crypto::hash tx_id;
      epee::string_tools::hex_to_pod(tx_info->hash(), tx_id);
      bool is_failed = tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Failed;
      if ((failed && is_failed) || (!is_failed && pending)) {
        transfers.push_back({
          /* type */ (is_failed ? "failed" : "pending"),
          /* block_height */ (is_failed ? "failed" : "pending"),
          (uint64_t) tx_info->timestamp(),
          /* direction */ "out",
          /* confirmed */ false,
          tx_info->amount(),
          tx_id,
          payment_id,
          tx_info->fee(),
          destinations,
          tx_info->subaddrIndex(),
          tx_info->description(),
          /* locked_msg */ "-"
        });
      }
    }
  }
  // sort by block, then by timestamp (unconfirmed last)
  std::sort(transfers.begin(), transfers.end(), [](const transfer_view& a, const transfer_view& b) -> bool {
    if (a.confirmed && !b.confirmed)
      return true;
    if (a.block == b.block)
      return a.timestamp < b.timestamp;
    return a.block < b.block;
  });

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfers(const std::vector<std::string> &args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() > 4) {
    fail_msg_writer() << tr("usage: show_transfers [in|out|all|pending|failed|pool|coinbase] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]]");
    return true;
  }

  LOCK_IDLE_SCOPE();

  std::vector<transfer_view> all_transfers;

  if (!get_transfers(local_args, all_transfers))
    return true;

  PAUSE_READLINE();

  auto formatter = boost::format("%8.8s %6.6s %8.8s %25.25s %20.20s %64.64s %16.16s %14.14s %s %s - %s");
  message_writer(console_color_default, false) << formatter
  % "Block"
  % "In/Out"
  % "Locked?"
  % "Timestamp"
  % "Amount"
  % "Tx Hash"
  % "Tx Payment ID"
  % "Tx Fee"
  % "Destination(s)"
  % "Index"
  % "Tx Note";

  formatter = boost::format("%8.8llu %6.6s %8.8s %25.25s %20.20s %64.64s %16.16s %14.14s %s %s - %s");

  for (const auto& transfer : all_transfers)
  {
    const auto color = transfer.type == "failed" ? console_color_red : transfer.confirmed ? ((transfer.direction == "in" || transfer.direction == "block") ? console_color_green : console_color_magenta) : console_color_default;

    std::string destinations = "-";
    if (!transfer.outputs.empty())
    {
      destinations = "";
      for (const auto& output : transfer.outputs)
      {
        if (!destinations.empty())
          destinations += ", ";
        destinations += (transfer.direction == "in" ? output.first.substr(0, 6) : output.first) + ":" + print_money(output.second);
      }
    }

    message_writer(color, false) << formatter
      % transfer.block
      % transfer.direction
      % transfer.unlocked
      % tools::get_human_readable_timestamp(transfer.timestamp)
      % print_money(transfer.amount)
      % string_tools::pod_to_hex(transfer.hash)
      % transfer.payment_id
      % print_money(transfer.fee)
      % destinations
      % boost::algorithm::join(transfer.index | boost::adaptors::transformed([](uint32_t i) { return std::to_string(i); }), ", ")
      % transfer.note;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_transfers(const std::vector<std::string>& args_)
{
  std::vector<std::string> local_args = args_;

  if(local_args.size() > 6) {
    fail_msg_writer() << tr("usage: export_transfers [in|out|all|pending|failed|pool|coinbase] [index=<N1>[,<N2>,...]] [<min_height> [<max_height>]] [output=<path>] [option=<with_keys>]");
    return true;
  }

  std::vector<transfer_view> all_transfers;

  // might consumes arguments in local_args
  if (!get_transfers(local_args, all_transfers))
    return true;

  // output filename
  std::string filename = (boost::format("output%u.csv") % m_current_subaddress_account).str();
  if (local_args.size() > 0 && local_args[0].substr(0, 7) == "output=")
  {
    filename = local_args[0].substr(7, -1);
    local_args.erase(local_args.begin());
  }
  // check for export with tx keys
  bool export_keys = false;
  if (local_args.size() > 0 && local_args[0].substr(0, 7) == "option=")
  {
    export_keys = local_args[0].substr(7, -1) == "with_keys";
    local_args.erase(local_args.begin());
  }
  if (export_keys)
  {
    if (m_wallet_impl->getDeviceState().key_on_device && m_wallet_impl->getDeviceState().device_type != Wallet::DeviceState::DeviceType::DeviceType_Trezor)
    {
      fail_msg_writer() << tr("command not supported by HW wallet");
      return true;
    }
    SCOPED_WALLET_UNLOCK();
  } else 
  {
    LOCK_IDLE_SCOPE();
  }

  std::ofstream file(filename);
  if(file.fail()) {
    fail_msg_writer() << boost::format(tr("Failed to open %s for writing")) % filename;
    return true;
  }

  // header
  file <<
      boost::format("%8.8s,%9.9s,%8.8s,%25.25s,%20.20s,%20.20s,%64.64s,%16.16s,%14.14s,%106.106s,%20.20s,%s,%s,%s") %
      tr("block") % tr("direction") % tr("unlocked") % tr("timestamp") % tr("amount") % tr("running balance") % tr("hash") % tr("payment ID") % tr("fee") % tr("destination") % tr("amount") % tr("index") % tr("note") % tr("tx key")
      << std::endl;

  uint64_t running_balance = 0;
  auto formatter = boost::format("%8.8llu,%9.9s,%8.8s,%25.25s,%20.20s,%20.20s,%64.64s,%16.16s,%14.14s,%106.106s,%20.20s,\"%s\",%s,%s");

  for (const auto& transfer : all_transfers)
  {
    // ignore unconfirmed transfers in running balance
    if (transfer.confirmed)
    {
      if (transfer.direction == "in" || transfer.direction == "block")
        running_balance += transfer.amount;
      else
        running_balance -= transfer.amount + transfer.fee;
    }

    std::string key_string;
    if (export_keys)
    {
      key_string = m_wallet_impl->getTxKey(epee::string_tools::pod_to_hex(transfer.hash));
      if (key_string.empty())
      {
        int error_code;
        std::string error_msg;
        m_wallet_impl->statusWithErrorString(error_code, error_msg);
        fail_msg_writer() << error_msg;
      }
    }

    file << formatter
      % transfer.block
      % transfer.direction
      % transfer.unlocked
      % tools::get_human_readable_timestamp(transfer.timestamp)
      % print_money(transfer.amount)
      % print_money(running_balance)
      % string_tools::pod_to_hex(transfer.hash)
      % transfer.payment_id
      % print_money(transfer.fee)
      % (transfer.outputs.size() ? transfer.outputs[0].first : "-")
      % (transfer.outputs.size() ? print_money(transfer.outputs[0].second) : "")
      % boost::algorithm::join(transfer.index | boost::adaptors::transformed([](uint32_t i) { return std::to_string(i); }), ", ")
      % transfer.note
      % key_string
      << std::endl;

    for (size_t i = 1; i < transfer.outputs.size(); ++i)
    {
      file << formatter
        % ""
        % ""
        % ""
        % ""
        % ""
        % ""
        % ""
        % ""
        % ""
        % transfer.outputs[i].first
        % print_money(transfer.outputs[i].second)
        % ""
        % ""
        % ""
        << std::endl;
    }
  }
  file.close();

  if(file.fail()) {
    fail_msg_writer() << tr("Failed to export CSV to ") << filename;
  } else {
    success_msg_writer() << tr("CSV exported to ") << filename;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::unspent_outputs(const std::vector<std::string> &args_)
{
  if(args_.size() > 3)
  {
    PRINT_USAGE(USAGE_UNSPENT_OUTPUTS);
    return true;
  }
  auto local_args = args_;

  std::set<uint32_t> subaddr_indices;
  if (local_args.size() > 0 && local_args[0].substr(0, 6) == "index=")
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
  std::vector<std::unique_ptr<EnoteDetails>> enote_details = m_wallet_impl->getEnoteDetails();
  std::map<uint64_t, std::vector<std::unique_ptr<EnoteDetails>>> amount_to_eds;
  uint64_t min_height = std::numeric_limits<uint64_t>::max();
  uint64_t max_height = 0;
  uint64_t found_min_amount = std::numeric_limits<uint64_t>::max();
  uint64_t found_max_amount = 0;
  uint64_t found_sum_amount = 0;
  uint64_t count = 0;
  for (auto& ed : enote_details)
  {
    uint64_t amount = ed->amount();
    if (ed->isSpent()
            || amount < min_amount
            || amount > max_amount
            || ed->subaddressIndexMajor() != m_current_subaddress_account
            || (subaddr_indices.count(ed->subaddressIndexMinor()) == 0 && !subaddr_indices.empty()))
      continue;
    std::uint64_t block_height = ed->blockHeight();
    amount_to_eds[amount].push_back(std::move(ed));
    if (min_height > block_height) min_height = block_height;
    if (max_height < block_height) max_height = block_height;
    if (found_min_amount > amount) found_min_amount = amount;
    if (found_max_amount < amount) found_max_amount = amount;
    found_sum_amount += amount;
    ++count;
  }
  if (amount_to_eds.empty())
  {
    success_msg_writer() << tr("There is no unspent output in the specified address");
    return true;
  }
  for (const auto& amount_eds : amount_to_eds)
  {
    auto& eds = amount_eds.second;
    success_msg_writer() << tr("\nAmount: ") << print_money(amount_eds.first) << tr(", number of keys: ") << eds.size();
    for (size_t i = 0; i < eds.size(); )
    {
      std::ostringstream oss;
      for (size_t j = 0; j < 8 && i < eds.size(); ++i, ++j)
        oss << eds[i]->blockHeight() << tr(" ");
      success_msg_writer() << oss.str();
    }
  }
  success_msg_writer()
    << tr("\nMin block height: ") << min_height
    << tr("\nMax block height: ") << max_height
    << tr("\nMin amount found: ") << print_money(found_min_amount)
    << tr("\nMax amount found: ") << print_money(found_max_amount)
    << tr("\nSum amount found: ") << print_money(found_sum_amount)
    << tr("\nTotal count: ") << count;
  const size_t histogram_height = 10;
  const size_t histogram_width  = 50;
  double bin_size = (max_height - min_height + 1.0) / histogram_width;
  size_t max_bin_count = 0;
  std::vector<size_t> histogram(histogram_width, 0);
  for (const auto& amount_eds : amount_to_eds)
  {
    for (auto& ed : amount_eds.second)
    {
      uint64_t bin_index = (ed->blockHeight() - min_height + 1) / bin_size;
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
  CHECK_IF_BACKGROUND_SYNCING("cannot rescan");

  uint64_t start_height = 0;
  ResetType reset_type = ResetSoft;

  if (!args_.empty())
  {
    if (args_[0] == "hard")
    {
      reset_type = ResetHard;
    }
    else if (args_[0] == "soft")
    {
      reset_type = ResetSoft;
    }
    else if (args_[0] == "keep_ki")
    {
      reset_type = ResetSoftKeepKI;
    }
    else
    {
      PRINT_USAGE(USAGE_RESCAN_BC);
      return true;
    }

    if (args_.size() > 1)
    {
      try
      {
        start_height = boost::lexical_cast<uint64_t>( args_[1] );
      }
      catch(const boost::bad_lexical_cast &)
      {
        start_height = 0;
      }
    }
  }

  if (reset_type == ResetHard)
  {
    message_writer() << tr("Warning: this will lose any information which can not be recovered from the blockchain.");
    message_writer() << tr("This includes destination addresses, tx secret keys, tx notes, etc");
    std::string confirm = input_line(tr("Rescan anyway?"), true);
    if(!std::cin.eof())
    {
      if (!command_line::is_yes(confirm))
        return true;
    }
  }

  const uint64_t wallet_from_height = m_wallet_impl->getRefreshFromBlockHeight();
  if (start_height > wallet_from_height)
  {
    message_writer() << tr("Warning: your restore height is higher than wallet restore height: ") << wallet_from_height;
    std::string confirm = input_line(tr("Rescan anyway ? (Y/Yes/N/No): "));
    if(!std::cin.eof())
    {
      if (!command_line::is_yes(confirm))
        return true;
    }
  }

  m_in_manual_refresh.store(true, std::memory_order_relaxed);
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){m_in_manual_refresh.store(false, std::memory_order_relaxed);});
  return refresh_main(start_height, reset_type, true);
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::wallet_idle_thread()
{
  const boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time();
  while (true)
  {
    boost::unique_lock<boost::mutex> lock(m_idle_mutex);
    if (!m_idle_run.load(std::memory_order_relaxed))
      break;

    // if another thread was busy (ie, a foreground refresh thread), we'll end up here at
    // some random time that's not what we slept for, so we should not call refresh now
    // or we'll be leaking that fact through timing
    const boost::posix_time::ptime now0 = boost::posix_time::microsec_clock::universal_time();
    const uint64_t dt_actual = (now0 - start_time).total_microseconds() % 1000000;
#ifdef _WIN32
    static const uint64_t threshold = 10000;
#else
    static const uint64_t threshold = 2000;
#endif
    if (dt_actual < threshold) // if less than a threshold... would a very slow machine always miss it ?
    {
#ifndef _WIN32
      m_inactivity_checker.do_call(boost::bind(&simple_wallet::check_inactivity, this));
#endif
      m_refresh_checker.do_call(boost::bind(&simple_wallet::check_refresh, this));

      if (!m_idle_run.load(std::memory_order_relaxed))
        break;
    }

    // aim for the next multiple of 1 second
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    const auto dt = (now - start_time).total_microseconds();
    const auto wait = 1000000 - dt % 1000000;
    m_idle_cond.wait_for(lock, boost::chrono::microseconds(wait));
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_inactivity()
{
    // inactivity lock
    if (!m_locked && !m_in_command)
    {
      const uint32_t seconds = m_wallet_impl->getInactivityLockTimeout();
      if (seconds > 0 && time(NULL) - m_last_activity_time > seconds)
      {
        m_locked = true;
        m_cmd_binder.cancel_input();
      }
    }
    return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::check_refresh()
{
    // auto refresh
    if (m_auto_refresh_enabled)
    {
      m_auto_refresh_refreshing = true;
      if (try_connect_to_daemon(true))
        m_wallet_impl->refresh(/* start_height */ 0,
                               /* check_pool */ false,
                               /* try_incremental */ true);
      m_auto_refresh_refreshing = false;
    }
    return true;
}
//----------------------------------------------------------------------------------------------------
std::string simple_wallet::get_prompt() const
{
  if (m_locked)
    return std::string("[") + tr("locked due to inactivity") + "]";
  std::string addr_start = m_wallet_impl->address(m_current_subaddress_account, 0).substr(0, 6);
  std::string prompt = std::string("[") + tr("wallet") + " " + addr_start;
  if (!m_wallet_impl->connectToDaemon(NULL))
    prompt += tr(" (no daemon)");
  else if (!m_wallet_impl->daemonSynced())
    prompt += tr(" (out of sync)");
  prompt += "]: ";
  return prompt;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::run()
{
  // check and display warning, but go on anyway
  try_connect_to_daemon();

  refresh_main(0, ResetNone, true);

  m_auto_refresh_enabled = !m_wallet_impl->isOffline() && m_wallet_impl->getAutoRefresh();
  m_idle_thread = boost::thread([&]{wallet_idle_thread();});

  message_writer(console_color_green, false) << "Background refresh thread started";
  return m_cmd_binder.run_handling([this](){return get_prompt();}, "");
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::stop()
{
  m_cmd_binder.stop_handling();
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
  int error_code;
  std::string error_msg;
  local_args.erase(local_args.begin());
  if (command == "new")
  {
    // create a new account and switch to it
    CHECK_IF_BACKGROUND_SYNCING("cannot create new account");
    std::string label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled account)");
    m_wallet_impl->addSubaddressAccount(label);
    m_current_subaddress_account = m_wallet_impl->numSubaddressAccounts() - 1;
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
    if (index_major >= m_wallet_impl->numSubaddressAccounts())
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet_impl->numSubaddressAccounts() - 1);
      return true;
    }
    m_current_subaddress_account = index_major;
    // update_prompt();
    show_balance();
  }
  else if (command == "label" && local_args.size() >= 1)
  {
    // set label of the specified account
    CHECK_IF_BACKGROUND_SYNCING("cannot modify account");
    uint32_t index_major;
    if (!epee::string_tools::get_xtype_from_string(index_major, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
      return true;
    }
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    m_wallet_impl->setSubaddressLabel(index_major, 0, label);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (!error_code)
    {
      LOCK_IDLE_SCOPE();
      print_accounts();
    }
    else
      fail_msg_writer() << error_msg;
  }
  else if (command == "tag" && local_args.size() >= 2)
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot modify account");
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
    m_wallet_impl->setAccountTag(account_indices, tag);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (!error_code)
      print_accounts(tag);
    else
      fail_msg_writer() << error_msg;
  }
  else if (command == "untag" && local_args.size() >= 1)
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot modify account");
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
    m_wallet_impl->setAccountTag(account_indices, "");
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (!error_code)
      print_accounts();
    else
      fail_msg_writer() << error_msg;
  }
  else if (command == "tag_description" && local_args.size() >= 1)
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot modify account");
    const std::string tag = local_args[0];
    std::string description;
    if (local_args.size() > 1)
    {
      local_args.erase(local_args.begin());
      description = boost::join(local_args, " ");
    }
    m_wallet_impl->setAccountTagDescription(tag, description);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (!error_code)
      print_accounts(tag);
    else
      fail_msg_writer() << error_code;
  }
  else
  {
    PRINT_USAGE(USAGE_ACCOUNT);
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::print_accounts()
{
  const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& account_tags = m_wallet_impl->getAccountTags();
  size_t num_untagged_accounts = m_wallet_impl->numSubaddressAccounts();
  for (const std::pair<const std::string, std::string>& p : account_tags.first)
  {
    const std::string& tag = p.first;
    print_accounts(tag);
    num_untagged_accounts -= std::count(account_tags.second.begin(), account_tags.second.end(), tag);
    success_msg_writer() << "";
  }

  if (num_untagged_accounts > 0)
    print_accounts("");

  if (num_untagged_accounts < m_wallet_impl->numSubaddressAccounts())
    success_msg_writer() << tr("\nGrand total:\n  Balance: ") << print_money(m_wallet_impl->balanceAll()) <<
                            tr(", unlocked balance: ") << print_money(m_wallet_impl->unlockedBalanceAll());
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::print_accounts(const std::string& tag)
{
  const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& account_tags = m_wallet_impl->getAccountTags();
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
  for (uint32_t account_index = 0; account_index < m_wallet_impl->numSubaddressAccounts(); ++account_index)
  {
    uint64_t balance = m_wallet_impl->balance(account_index);
    uint64_t unlocked_balance = m_wallet_impl->unlockedBalance(account_index);
    if (account_tags.second[account_index] != tag)
      continue;
    success_msg_writer() << boost::format(tr(" %c%8u %6s %21s %21s %21s"))
      % (m_current_subaddress_account == account_index ? '*' : ' ')
      % account_index
      % m_wallet_impl->address(account_index, 0).substr(0, 6)
      % print_money(balance)
      % print_money(unlocked_balance)
      % m_wallet_impl->getSubaddressLabel(account_index, 0);
    total_balance += balance;
    total_unlocked_balance += unlocked_balance;
  }
  success_msg_writer() << tr("------------------------------------------------------------------------------------");
  success_msg_writer() << boost::format(tr("%15s   %21s %21s")) % "Total" % print_money(total_balance) % print_money(total_unlocked_balance);
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
  //  address device [<index>]

  std::vector<std::string> local_args = args;
  std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet_impl->getEnoteDetails();

  auto print_address_sub = [this, &eds](uint32_t index)
  {
    bool used = std::find_if(
      eds.begin(), eds.end(),
      [this, &index](const std::unique_ptr<EnoteDetails>& ed) {
        return ed->subaddressIndexMajor() ==  m_current_subaddress_account
            && ed->subaddressIndexMinor() == index;
      }) != eds.end();
    success_msg_writer() << index << "  " << m_wallet_impl->address(m_current_subaddress_account, index) << "  " << (index == 0 ? tr("Primary address") : m_wallet_impl->getSubaddressLabel(m_current_subaddress_account, index)) << " " << (used ? tr("(used)") : "");
  };

  uint32_t index = 0;
  if (local_args.empty())
  {
    print_address_sub(index);
  }
  else if (local_args.size() == 1 && local_args[0] == "all")
  {
    local_args.erase(local_args.begin());
    for (; index < m_wallet_impl->numSubaddresses(m_current_subaddress_account); ++index)
      print_address_sub(index);
  }
  else if (local_args[0] == "new")
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot add address");
    local_args.erase(local_args.begin());
    std::string label;
    if (local_args.size() > 0)
      label = boost::join(local_args, " ");
    if (label.empty())
      label = tr("(Untitled address)");
    m_wallet_impl->addSubaddress(m_current_subaddress_account, label);
    print_address_sub(m_wallet_impl->numSubaddresses(m_current_subaddress_account) - 1);
    m_wallet_impl->deviceShowAddress(m_current_subaddress_account, m_wallet_impl->numSubaddresses(m_current_subaddress_account) - 1, /* paymentId */ "");
  }
  else if (local_args[0] == "mnew")
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot add addresses");
    local_args.erase(local_args.begin());
    if (local_args.size() != 1)
    {
      fail_msg_writer() << tr("Expected exactly one argument for the amount of new addresses");
      return true;
    }
    uint32_t n;
    if (!epee::string_tools::get_xtype_from_string(n, local_args[0]))
    {
      fail_msg_writer() << tr("failed to parse the amount of new addresses: ") << local_args[0];
      return true;
    }
    if (n > MAX_MNEW_ADDRESSES)
    {
      fail_msg_writer() << tr("the amount of new addresses must be lower or equal to ") << MAX_MNEW_ADDRESSES;
      return true;
    }
    for (uint32_t i = 0; i < n; ++i)
    {
      m_wallet_impl->addSubaddress(m_current_subaddress_account, tr("(Untitled address)"));
      print_address_sub(m_wallet_impl->numSubaddresses(m_current_subaddress_account) - 1);
    }
  }
  else if (local_args[0] == "one-off")
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot add address");
    local_args.erase(local_args.begin());
    std::string label;
    if (local_args.size() != 2)
    {
      fail_msg_writer() << tr("Expected exactly two arguments for index");
      return true;
    }
    uint32_t major, minor;
    if (!epee::string_tools::get_xtype_from_string(major, local_args[0]) || !epee::string_tools::get_xtype_from_string(minor, local_args[1]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[0] << " " << local_args[1];
      return true;
    }
    m_wallet_impl->createOneOffSubaddress(major, minor);
    success_msg_writer() << boost::format(tr("Address at %u %u: %s")) % major % minor % m_wallet_impl->address(major, minor);
  }
  else if (local_args.size() >= 2 && local_args[0] == "label")
  {
    CHECK_IF_BACKGROUND_SYNCING("cannot modify address");
    if (!epee::string_tools::get_xtype_from_string(index, local_args[1]))
    {
      fail_msg_writer() << tr("failed to parse index: ") << local_args[1];
      return true;
    }
    if (index >= m_wallet_impl->numSubaddresses(m_current_subaddress_account))
    {
      fail_msg_writer() << tr("specify an index between 0 and ") << (m_wallet_impl->numSubaddresses(m_current_subaddress_account) - 1);
      return true;
    }
    local_args.erase(local_args.begin());
    local_args.erase(local_args.begin());
    std::string label = boost::join(local_args, " ");
    m_wallet_impl->setSubaddressLabel(m_current_subaddress_account, index, label);
    print_address_sub(index);
  }
  else if (local_args.size() <= 2 && epee::string_tools::get_xtype_from_string(index, local_args[0]))
  {
    local_args.erase(local_args.begin());
    uint32_t index_min = index;
    uint32_t index_max = index_min;
    size_t n_subaddresses = m_wallet_impl->numSubaddresses(m_current_subaddress_account);
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
    if (index_min >= n_subaddresses)
    {
      fail_msg_writer() << tr("<index_min> is already out of bound");
      return true;
    }
    if (index_max >= n_subaddresses)
    {
      message_writer() << tr("<index_max> exceeds the bound");
      index_max = n_subaddresses - 1;
    }
    for (index = index_min; index <= index_max; ++index)
      print_address_sub(index);
  }
  else if (local_args[0] == "device")
  {
    index = 0;
    local_args.erase(local_args.begin());
    if (local_args.size() > 0)
    {
      if (!epee::string_tools::get_xtype_from_string(index, local_args[0]))
      {
        fail_msg_writer() << tr("failed to parse index: ") << local_args[0];
        return true;
      }
      if (index >= m_wallet_impl->numSubaddresses(m_current_subaddress_account))
      {
        fail_msg_writer() << tr("<index> is out of bounds");
        return true;
      }
    }

    print_address_sub(index);
    m_wallet_impl->deviceShowAddress(m_current_subaddress_account, index, /* paymentId */ "");
  }
  else
  {
    PRINT_USAGE(USAGE_ADDRESS);
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::print_integrated_address(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  std::string payment_id;
  bool display_on_device = false;
  std::vector<std::string> local_args = args;

  if (local_args.size() > 0 && local_args[0] == "device")
  {
    local_args.erase(local_args.begin());
    display_on_device = true;
  }

  auto device_show_integrated = [this, display_on_device](const std::string &payment_id)
  {
    if (display_on_device)
    {
      m_wallet_impl->deviceShowAddress(m_current_subaddress_account, 0, payment_id);
    }
  };

  if (local_args.size() > 1)
  {
    PRINT_USAGE(USAGE_INTEGRATED_ADDRESS);
    return true;
  }
  if (local_args.size() == 0)
  {
    if (m_current_subaddress_account != 0)
    {
      fail_msg_writer() << tr("Integrated addresses can only be created for account 0");
      return true;
    }
    payment_id = Wallet::genPaymentId();
    success_msg_writer() << tr("Random payment ID: ") << payment_id;
    success_msg_writer() << tr("Matching integrated address: ") << m_wallet_impl->integratedAddress(payment_id);
    device_show_integrated(payment_id);
    return true;
  }
  if(m_wallet_impl->paymentIdValid(local_args.back()))
  {
    payment_id = local_args.back();
    if (m_current_subaddress_account != 0)
    {
      fail_msg_writer() << tr("Integrated addresses can only be created for account 0");
      return true;
    }
    success_msg_writer() << m_wallet_impl->integratedAddress(payment_id);
    device_show_integrated(payment_id);
    return true;
  }
  else {
    payment_id = Wallet::paymentIdFromAddress(local_args.back(), m_wallet_impl->nettype());
    std::string account_address = Wallet::getAddressFromIntegrated(local_args.back(), m_wallet_impl->nettype());
    if(!payment_id.empty())
    {
      success_msg_writer() << boost::format(tr("Standard address: %s, payment ID: %s"))
                                % account_address % payment_id;
      device_show_integrated(payment_id);
      return true;
    }
    else if (!account_address.empty())
    {
      success_msg_writer() << (Wallet::isSubaddress(local_args.back(), m_wallet_impl->nettype())
                              ? tr("Subaddress: ") : tr("Standard address: "))
                           << account_address;
      return true;
    }
  }
  fail_msg_writer() << tr("failed to parse payment ID or address");
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::address_book(const std::vector<std::string> &args/* = std::vector<std::string>()*/)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get address book");

  AddressBook *address_book = m_wallet_impl->addressBook();

  if (args.size() == 0)
  {
  }
  else if (args.size() == 1 || (args[0] != "add" && args[0] != "delete"))
  {
    PRINT_USAGE(USAGE_ADDRESS_BOOK);
    return true;
  }
  else if (args[0] == "add")
  {
    if(!Wallet::addressValid(/* address */ args[1], m_wallet_impl->nettype()))
    {
      fail_msg_writer() << tr("failed to parse address");
      return true;
    }
    size_t description_start = 2;
    std::string description;
    for (size_t i = description_start; i < args.size(); ++i)
    {
      if (i > description_start)
        description += " ";
      description += args[i];
    }
    address_book->addRow(args[1],
                         /* [deprecated] payment_id */ "",
                         description);
    if (address_book->errorCode())
    {
      fail_msg_writer() << address_book->errorString();
      return true;
    }
  }
  else
  {
    size_t row_id;
    if(!epee::string_tools::get_xtype_from_string(row_id, args[1]))
    {
      fail_msg_writer() << tr("failed to parse index");
      return true;
    }
    address_book->deleteRow(row_id);
  }
  std::vector<AddressBookRow *> address_book_entries = address_book->getAll();
  if (address_book_entries.empty())
  {
    success_msg_writer() << tr("Address book is empty.");
  }
  else
  {
    for (size_t i = 0; i < address_book_entries.size(); ++i) {
      auto& row = address_book_entries[i];
      success_msg_writer() << tr("Index: ") << i;
      success_msg_writer() << tr("Address: ") << row->getAddress();
      success_msg_writer() << tr("Description: ") << row->getDescription() << "\n";
    }
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_tx_note(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot set tx note");

  if (args.size() == 0)
  {
    PRINT_USAGE(USAGE_SET_TX_NOTE);
    return true;
  }

  std::string note = "";
  for (size_t n = 1; n < args.size(); ++n)
  {
    if (n > 1)
      note += " ";
    note += args[n];
  }
  if (!m_wallet_impl->setUserNote(/* txid */ args.front(), note))
    fail_msg_writer() << m_wallet_impl->errorString();

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_tx_note(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get tx note");

  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_GET_TX_NOTE);
    return true;
  }

  std::string note = m_wallet_impl->getUserNote(/* txid */ args.front());
  if (note.empty())
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
        fail_msg_writer() << error_msg;
    else
        success_msg_writer() << "no note found";
  }
  else
    success_msg_writer() << "note found: " << note;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::set_description(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot set description");

  // 0 arguments allowed, for setting the description to empty string

  std::string description = "";
  for (size_t n = 0; n < args.size(); ++n)
  {
    if (n > 0)
      description += " ";
    description += args[n];
  }
  // Info about "Attribute system": https://github.com/monero-project/monero/pull/2605
  m_wallet_impl->setCacheAttribute(/* key */ "wallet2.description", description);

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::get_description(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot get description");

  if (args.size() != 0)
  {
    PRINT_USAGE(USAGE_GET_DESCRIPTION);
    return true;
  }

  std::string description = m_wallet_impl->getCacheAttribute(/* key */ "wallet2.description");
  if (description.empty())
    success_msg_writer() << tr("no description found");
  else
    success_msg_writer() << tr("description found: ") << description;

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::status(const std::vector<std::string> &args)
{
  uint64_t local_height = m_wallet_impl->blockChainHeight();
  uint32_t version = 0;
  bool ssl = false;
  if (!m_wallet_impl->connectToDaemon(&version, &ssl))
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
        << ", daemon RPC v" << get_version_string(version) << ", " << (ssl ? "SSL" : "no SSL");
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
  MultisigState ms_status = m_wallet_impl->multisig();

  std::string description = m_wallet_impl->getCacheAttribute(/* key */ "wallet2.description");
  if (description.empty())
  {
    description = "<Not set>"; 
  }
  message_writer() << tr("Filename: ") << m_wallet_impl->filename();
  message_writer() << tr("Description: ") << description;
  message_writer() << tr("Address: ") << m_wallet_impl->address();
  std::string type;
  if (m_wallet_impl->watchOnly())
    type = tr("Watch only");
  else if (ms_status.isMultisig)
    type = (boost::format(tr("%u/%u multisig%s")) % ms_status.threshold % ms_status.total % (ms_status.isReady ? "" : " (not yet finalized)")).str();
  else if (m_wallet_impl->isBackgroundWallet())
    type = tr("Background wallet");
  else
    type = tr("Normal");
  message_writer() << tr("Type: ") << type;
  message_writer() << tr("Network type: ") << (
    m_wallet_impl->nettype() == Monero::NetworkType::TESTNET ? tr("Testnet") :
    m_wallet_impl->nettype() == Monero::NetworkType::STAGENET ? tr("Stagenet") : tr("Mainnet"));
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::sign(const std::vector<std::string> &args)
{
  CHECK_IF_BACKGROUND_SYNCING("cannot sign");
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  if (args.size() != 1 && args.size() != 2 && args.size() != 3)
  {
    PRINT_USAGE(USAGE_SIGN);
    return true;
  }
  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot sign");
    return true;
  }
  if (m_wallet_impl->multisig().isMultisig)
  {
    fail_msg_writer() << tr("This wallet is multisig and cannot sign");
    return true;
  }

  bool do_sign_with_spend_key = true;
  subaddress_index index{0, 0};
  for (unsigned int idx = 0; idx + 1 < args.size(); ++idx)
  {
    unsigned int a, b;
    if (sscanf(args[idx].c_str(), "%u,%u", &a, &b) == 2)
    {
      index.major = a;
      index.minor = b;
    }
    else if (args[idx] == "--spend")
    {
      do_sign_with_spend_key = true;
    }
    else if (args[idx] == "--view")
    {
      do_sign_with_spend_key = false;
    }
    else
    {
      fail_msg_writer() << tr("Invalid subaddress index format, and not a signature type: ") << args[idx];
      return true;
    }
  }

  const std::string &filename = args.back();
  std::string data;
  bool r = load_from_file(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }

  SCOPED_WALLET_UNLOCK();

  std::string signature = m_wallet_impl->signMessage(data, m_wallet_impl->address(index.major, index.minor), do_sign_with_spend_key);
  success_msg_writer() << signature;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::verify(const std::vector<std::string> &args)
{
  if (args.size() != 3)
  {
    PRINT_USAGE(USAGE_VERIFY);
    return true;
  }
  std::string filename = args[0];
  std::string address_string = args[1];
  std::string signature= args[2];

  std::string data;
  bool r = load_from_file(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }

  bool is_old;
  std::string signature_type;
  if (!m_wallet_impl->verifySignedMessage(data, address_string, signature, &is_old, &signature_type))
  {
    fail_msg_writer() << tr("Bad signature from ") << address_string;
  }
  else
  {
    success_msg_writer() << tr("Good signature from ") << address_string << (is_old ? " (using old signature algorithm)" : "") << " with " << signature_type;
  }
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_key_images(const std::vector<std::string> &args_)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot export key images");
  auto args = args_;

  if (m_wallet_impl->watchOnly())
  {
    fail_msg_writer() << tr("wallet is watch-only and cannot export key images");
    return true;
  }

  bool all = false;
  if (args.size() >= 2 && args[0] == "all")
  {
    all = true;
    args.erase(args.begin());
  }

  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_EXPORT_KEY_IMAGES);
    return true;
  }

  std::string filename = args[0];
  if (m_wallet_impl->getConfirmExportOverwrite() && !check_file_overwrite(filename))
    return true;

  SCOPED_WALLET_UNLOCK();

  if (!m_wallet_impl->exportKeyImages(filename, all))
  {
    LOG_ERROR("Error exporting key images: " << m_wallet_impl->errorString());
    fail_msg_writer() << tr("Error exporting key images: ") << m_wallet_impl->errorString();
    return true;
  }

  success_msg_writer() << tr("Signed key images exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_key_images(const std::vector<std::string> &args)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot import key images");
  if (!m_wallet_impl->trustedDaemon())
  {
    fail_msg_writer() << tr("this command requires a trusted daemon. Enable with --trusted-daemon");
    return true;
  }

  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_IMPORT_KEY_IMAGES);
    return true;
  }
  std::string filename = args[0];

  LOCK_IDLE_SCOPE();
  uint64_t spent = 0, unspent = 0, height;
  if (m_wallet_impl->importKeyImages(filename, &spent, &unspent, &height))
  {
      success_msg_writer() << "Signed key images imported to height " << height << ", "
          << print_money(spent) << " spent, " << print_money(unspent) << " unspent";
  }
  else
  {
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    fail_msg_writer() << error_msg;
    return true;
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::hw_key_images_sync(const std::vector<std::string> &args)
{
  if (!m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command only supported by HW wallet");
    return true;
  }
  if (!m_wallet_impl->getDeviceState().has_ki_cold_sync)
  {
    fail_msg_writer() << tr("hw wallet does not support cold KI sync");
    return true;
  }

  LOCK_IDLE_SCOPE();
  key_images_sync_intern();
  return true;
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::key_images_sync_intern(){
  try
  {
    message_writer(console_color_white, false) << tr("Please confirm the key image sync on the device");

    uint64_t spent = 0, unspent = 0;
    uint64_t height = m_wallet_impl->coldKeyImageSync(spent, unspent);
    if (height > 0)
    {
      success_msg_writer() << tr("Key images synchronized to height ") << height;
      if (!m_wallet_impl->trustedDaemon())
      {
        message_writer() << tr("Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent");
      } else
      {
        success_msg_writer() << print_money(spent) << tr(" spent, ") << print_money(unspent) << tr(" unspent");
      }
    }
    else {
      fail_msg_writer() << tr("Failed to import key images");
    }
  }
  catch (const std::exception &e)
  {
    fail_msg_writer() << tr("Failed to import key images: ") << e.what();
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::hw_reconnect(const std::vector<std::string> &args)
{
  if (!m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command only supported by HW wallet");
    return true;
  }

  LOCK_IDLE_SCOPE();
  bool r = m_wallet_impl->reconnectDevice();
  if (!r){
    std::stringstream fail_msg;
    fail_msg << tr("Failed to reconnect device");
    int error_code;
    std::string error_msg;
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code)
        fail_msg << ": " << error_msg;
    fail_msg_writer()  << fail_msg.str();
  }

  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::export_outputs(const std::vector<std::string> &args_)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot export outputs");
  auto args = args_;

  bool all = false;
  if (args.size() >= 2 && args[0] == "all")
  {
    all = true;
    args.erase(args.begin());
  }

  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_EXPORT_OUTPUTS);
    return true;
  }

  std::string filename = args[0];
  if (m_wallet_impl->getConfirmExportOverwrite() && !check_file_overwrite(filename))
    return true;


  std::string data;
  {
     SCOPED_WALLET_UNLOCK();
     data = m_wallet_impl->exportEnotesToStr(all);
  }
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << error_msg;
    return true;
  }
  bool r = save_to_file(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to save file ") << filename;
    return true;
  }

  success_msg_writer() << tr("Outputs exported to ") << filename;
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::import_outputs(const std::vector<std::string> &args)
{
  if (m_wallet_impl->getDeviceState().key_on_device)
  {
    fail_msg_writer() << tr("command not supported by HW wallet");
    return true;
  }
  CHECK_IF_BACKGROUND_SYNCING("cannot import outputs");
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_IMPORT_OUTPUTS);
    return true;
  }
  std::string filename = args[0];

  std::string data;
  bool r = load_from_file(filename, data);
  if (!r)
  {
    fail_msg_writer() << tr("failed to read file ") << filename;
    return true;
  }

  size_t n_outputs;
  {
    SCOPED_WALLET_UNLOCK();
    n_outputs = m_wallet_impl->importEnotesFromStr(data);
  }
  int error_code;
  std::string error_msg;
  m_wallet_impl->statusWithErrorString(error_code, error_msg);
  if (error_code)
  {
    fail_msg_writer() << error_msg;
    return true;
  }

  success_msg_writer() << boost::lexical_cast<std::string>(n_outputs) << " outputs imported";
  return true;
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::show_transfer(const std::vector<std::string> &args)
{
  if (args.size() != 1)
  {
    PRINT_USAGE(USAGE_SHOW_TRANSFER);
    return true;
  }

  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(args.front(), txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    fail_msg_writer() << tr("failed to parse txid");
    return true;
  }
  std::string txid = args.front();

  const uint64_t last_block_height = m_wallet_impl->blockChainHeight();
  TransactionHistory *tx_history = m_wallet_impl->history();

  for (const auto &tx_info : tx_history->getAll())
  {
    if (tx_info->hash() != txid)
      continue;

    // Payment = confirmed incoming transfer
    if (tx_info->direction() == Monero::TransactionInfo::Direction_In
          && tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Confirmed)
    {
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      success_msg_writer() << "Incoming transaction found";
      success_msg_writer() << "txid: <" << txid << ">";
      success_msg_writer() << "Height: " << tx_info->blockHeight();
      success_msg_writer() << "Timestamp: " << tools::get_human_readable_timestamp(tx_info->timestamp());
      success_msg_writer() << "Amount: " << print_money(tx_info->amount());
      success_msg_writer() << "Payment ID: " << payment_id;
      if (tx_info->unlockTime() < CRYPTONOTE_MAX_BLOCK_NUMBER)
      {
        uint64_t bh = std::max(tx_info->unlockTime(), tx_info->blockHeight() + CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE);
        uint64_t last_block_reward = m_wallet_impl->getLastBlockReward();
        uint64_t suggested_threshold = last_block_reward ? (tx_info->amount() + last_block_reward - 1) / last_block_reward : 0;
        if (bh >= last_block_height)
          success_msg_writer() << "Locked: " << (bh - last_block_height) << " blocks to unlock";
        else if (suggested_threshold > 0)
          success_msg_writer() << std::to_string(last_block_height - bh) << " confirmations (" << suggested_threshold << " suggested threshold)";
        else
          success_msg_writer() << std::to_string(last_block_height - bh) << " confirmations";
      }
      else
      {
        const uint64_t adjusted_time = m_wallet_impl->getDaemonAdjustedTime();
        uint64_t threshold = adjusted_time + (m_wallet_impl->useForkRules(2, 0) ? CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V2 : CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_SECONDS_V1);
        if (threshold >= tx_info->unlockTime())
          success_msg_writer() << "unlocked for " << get_human_readable_timespan(std::chrono::seconds(threshold - tx_info->unlockTime()));
        else
          success_msg_writer() << "locked for " << get_human_readable_timespan(std::chrono::seconds(tx_info->unlockTime() - threshold));
      }
      success_msg_writer() << "Address index: " << *tx_info->subaddrIndex().begin();
      success_msg_writer() << "Note: " << tx_info->description();
      return true;

    }
    // Confirmed outgoing transfer
    else if (tx_info->direction() == Monero::TransactionInfo::Direction_Out
          && tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Confirmed)
    {
      uint64_t change = tx_info->receivedChangeAmount() == (uint64_t)-1 ? 0 : tx_info->receivedChangeAmount(); // change may not be known
      std::string dests;
      for (const auto &d: tx_info->transfers()) {
        if (!dests.empty())
          dests += ", ";
        dests +=  d.address + ": " + print_money(d.amount);
      }
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      success_msg_writer() << "Outgoing transaction found";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Height: " << tx_info->blockHeight();
      success_msg_writer() << "Timestamp: " << tools::get_human_readable_timestamp(tx_info->timestamp());
      success_msg_writer() << "Amount: " << print_money(tx_info->amount());
      success_msg_writer() << "Payment ID: " << payment_id;
      success_msg_writer() << "Change: " << print_money(change);
      success_msg_writer() << "Fee: " << print_money(tx_info->fee());
      success_msg_writer() << "Destinations: " << dests;
      success_msg_writer() << "Note: " << tx_info->description();
      return true;
    }
    // Unconfirmed incoming transfer in pool
    else if (tx_info->direction() == Monero::TransactionInfo::Direction_In
          && tx_info->txState() == Monero::TransactionInfo::TxState::TxState_PendingInPool)
    {
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      success_msg_writer() << "Unconfirmed incoming transaction found in the txpool";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Timestamp: " << tools::get_human_readable_timestamp(tx_info->timestamp());
      success_msg_writer() << "Amount: " << print_money(tx_info->amount());
      success_msg_writer() << "Payment ID: " << payment_id;
      success_msg_writer() << "Address index: " << *tx_info->subaddrIndex().begin();
      success_msg_writer() << "Note: " << tx_info->description();
      if (tx_info->isDoubleSpendSeen())
        success_msg_writer() << tr("Double spend seen on the network: this transaction may or may not end up being mined");
      return true;
    }
    // Unconfirmed (failed | pending) outgoing transfer
    else if (tx_info->direction() == Monero::TransactionInfo::Direction_Out
          && (tx_info->txState() == Monero::TransactionInfo::TxState::TxState_PendingInPool
           || tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Pending
           || tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Failed))
    {
      uint64_t fee = tx_info->fee();
      std::string payment_id = tx_info->paymentId();
      if (payment_id.substr(16).find_first_not_of('0') == std::string::npos)
        payment_id = payment_id.substr(0,16);
      bool is_failed = tx_info->txState() == Monero::TransactionInfo::TxState::TxState_Failed;

      success_msg_writer() << (is_failed ? "Failed" : "Pending") << " outgoing transaction found";
      success_msg_writer() << "txid: " << txid;
      success_msg_writer() << "Timestamp: " << tools::get_human_readable_timestamp(tx_info->timestamp());
      success_msg_writer() << "Amount: " << print_money(tx_info->amount());
      success_msg_writer() << "Payment ID: " << payment_id;
      success_msg_writer() << "Change: " << print_money(tx_info->receivedChangeAmount());
      success_msg_writer() << "Fee: " << print_money(fee);
      success_msg_writer() << "Note: " << tx_info->description();
      return true;
    }
    else
        fail_msg_writer() << tr("unknown transfer type");

    return true;
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
    m_wallet_impl->stop();
  }
  else
  {
    stop();
  }
}
//----------------------------------------------------------------------------------------------------
void simple_wallet::commit_or_save(PendingTransaction &ptx, bool do_not_relay, bool do_force_commit /* = false */)
{
  if (do_not_relay)
  {
    const std::string filename = "raw_monero_tx";
    if (!check_file_overwrite(filename))
      return;
    std::vector<std::string> tx_blobs = ptx.convertTxToRawBlobStr();
    std::vector<std::string> tx_ids = ptx.txid();
    for (size_t i = 0; i < ptx.txCount(); ++i) {
      const std::string blob_hex = epee::string_tools::buff_to_hex_nodelimer(tx_blobs[i]);
      const std::string filename = "raw_monero_tx" + (ptx.txCount() == 1 ? "" : ("_" + std::to_string(i++)));
      if (save_to_file(filename, blob_hex, /* is_printable */ true))
        success_msg_writer(true) << tr("Transaction successfully saved to ") << filename << tr(", txid <") << tx_ids[i] << ">";
      else
        fail_msg_writer() << tr("Failed to save transaction to ") << filename << tr(", txid <") << tx_ids[i] << "> : " << ptx.errorString();
    }
  }
  else
  {
    std::string error_msg;
    if (m_wallet_impl->watchOnly() && !do_force_commit)
    {
      if (ptx.commit("unsigned_monero_tx"))
        success_msg_writer(true) << tr("Unsigned transaction(s) successfully written to file: ") << "unsigned_monero_tx";
      else
        fail_msg_writer() << ptx.errorString();
    }
    else
    {
      // ptx.commit() pops the pending txs after successfully committing them, therefore we cache the txids before
      std::vector<std::string> txids = ptx.txid();
      if (ptx.commit())
        for (const auto &txid : txids)
          success_msg_writer(true) << tr("Transaction successfully submitted, transaction <") << txid << ">" << ENDL
            << tr("You can check its status by using the `show_transfers` command.");
      else
        fail_msg_writer() << ptx.errorString();
    }
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::save_to_file(const std::string& path_to_file, const std::string& raw, bool is_printable /* = false */) const
{
  if (is_printable || m_wallet_impl->getExportFormat() == Monero::Wallet::ExportFormat::ExportFormat_Binary)
  {
    return epee::file_io_utils::save_string_to_file(path_to_file, raw);
  }

  FILE *fp = fopen(path_to_file.c_str(), "w+");
  if (!fp)
  {
    MERROR("Failed to open wallet file for writing: " << path_to_file << ": " << strerror(errno));
    return false;
  }

  // Save the result b/c we need to close the fp before returning success/failure.
  int write_result = PEM_write(fp, ASCII_OUTPUT_MAGIC.c_str(), "", (const unsigned char *) raw.c_str(), raw.length());
  fclose(fp);

  if (write_result == 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
bool simple_wallet::load_from_file(const std::string& path_to_file, std::string& target_str, size_t max_size /* = 1000000000 */)
{
  std::string data;
  bool r = epee::file_io_utils::load_file_to_string(path_to_file, data, max_size);
  if (!r)
  {
    return false;
  }

  if (!boost::algorithm::contains(boost::make_iterator_range(data.begin(), data.end()), ASCII_OUTPUT_MAGIC))
  {
    // It's NOT our ascii dump.
    target_str = std::move(data);
    return true;
  }

  // Creating a BIO and calling PEM_read_bio instead of simpler PEM_read
  // to avoid reading the file from disk twice.
  BIO* b = BIO_new_mem_buf((const void*) data.data(), data.length());

  char *name = NULL;
  char *header = NULL;
  unsigned char *openssl_data = NULL;
  long len = 0;

  // Save the result b/c we need to free the data before returning success/failure.
  int success = PEM_read_bio(b, &name, &header, &openssl_data, &len);

  try
  {
    target_str = std::string((const char*) openssl_data, len);
  }
  catch (...)
  {
    success = 0;
  }

  OPENSSL_free((void *) name);
  OPENSSL_free((void *) header);
  OPENSSL_free((void *) openssl_data);
  BIO_free(b);

  if (success == 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}
//----------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  TRY_ENTRY();

#ifdef WIN32
  // Activate UTF-8 support for Boost filesystem classes on Windows
  std::locale::global(boost::locale::generator().generate(""));
  boost::filesystem::path::imbue(std::locale());
#endif
  setlocale(LC_CTYPE, "");

  po::options_description desc_params(wallet_args::tr("Wallet options"));
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_generate_new_wallet);
  command_line::add_arg(desc_params, arg_generate_from_device);
  command_line::add_arg(desc_params, arg_generate_from_view_key);
  command_line::add_arg(desc_params, arg_generate_from_spend_key);
  command_line::add_arg(desc_params, arg_generate_from_keys);
  command_line::add_arg(desc_params, arg_generate_from_multisig_keys);
  command_line::add_arg(desc_params, arg_generate_from_json);
  command_line::add_arg(desc_params, arg_mnemonic_language);
  command_line::add_arg(desc_params, arg_electrum_seed );
  command_line::add_arg(desc_params, arg_restore_deterministic_wallet );
  command_line::add_arg(desc_params, arg_restore_from_seed );
  command_line::add_arg(desc_params, arg_restore_multisig_wallet );
  command_line::add_arg(desc_params, arg_non_deterministic );
  command_line::add_arg(desc_params, arg_restore_height);
  command_line::add_arg(desc_params, arg_restore_date);
  command_line::add_arg(desc_params, arg_do_not_relay);
  command_line::add_arg(desc_params, arg_create_address_file);
  command_line::add_arg(desc_params, arg_subaddress_lookahead);
  command_line::add_arg(desc_params, arg_use_english_language_names);

  command_line::add_arg(desc_params, arg_daemon_address );
  command_line::add_arg(desc_params, arg_daemon_host );
  command_line::add_arg(desc_params, arg_proxy );
  command_line::add_arg(desc_params, arg_trusted_daemon );
  command_line::add_arg(desc_params, arg_untrusted_daemon );
  command_line::add_arg(desc_params, arg_password );
  command_line::add_arg(desc_params, arg_password_file );
  command_line::add_arg(desc_params, arg_daemon_port );
  command_line::add_arg(desc_params, arg_daemon_login );
  command_line::add_arg(desc_params, arg_daemon_ssl );
  command_line::add_arg(desc_params, arg_daemon_ssl_private_key );
  command_line::add_arg(desc_params, arg_daemon_ssl_certificate );
  command_line::add_arg(desc_params, arg_daemon_ssl_ca_certificates );
  command_line::add_arg(desc_params, arg_daemon_ssl_allowed_fingerprints );
  command_line::add_arg(desc_params, arg_daemon_ssl_allow_any_cert );
  command_line::add_arg(desc_params, arg_daemon_ssl_allow_chained );

  command_line::add_arg(desc_params, arg_allow_mismatched_daemon_version );
  command_line::add_arg(desc_params, arg_testnet );
  command_line::add_arg(desc_params, arg_stagenet );
  command_line::add_arg(desc_params, arg_kdf_rounds );
  command_line::add_arg(desc_params, arg_hw_device );
  command_line::add_arg(desc_params, arg_hw_device_derivation_path );
  command_line::add_arg(desc_params, arg_tx_notify );
  command_line::add_arg(desc_params, arg_no_dns );
  command_line::add_arg(desc_params, arg_offline );
  command_line::add_arg(desc_params, arg_extra_entropy );
  command_line::add_arg(desc_params, arg_command);

  po::positional_options_description positional_options;
  positional_options.add(arg_command.name, -1);

  boost::optional<po::variables_map> vm;
  bool should_terminate = false;
  std::tie(vm, should_terminate) = wallet_args::main(
   argc, argv,
   "monero-wallet-cli [--wallet-file=<filename>|--generate-new-wallet=<filename>] [<COMMAND>]",
    sw::tr("This is the command line monero wallet. It needs to connect to a monero\ndaemon to work correctly.\nWARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy."),
    desc_params,
    positional_options,
    [](const std::string &s, bool emphasis){ tools::scoped_message_writer(emphasis ? epee::console_color_white : epee::console_color_default, true) << s; },
    "monero-wallet-cli.log"
  );

  if (!vm)
  {
    return 1;
  }

  if (should_terminate)
  {
    return 0;
  }

  cryptonote::simple_wallet w;
  const bool r = w.init(*vm);
  CHECK_AND_ASSERT_MES(r, 1, sw::tr("Failed to initialize wallet"));

  std::vector<std::string> command = command_line::get_arg(*vm, arg_command);
  if (!command.empty())
  {
    if (!w.process_command(command))
      fail_msg_writer() << sw::tr("Unknown command: ") << command.front();
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
  CATCH_ENTRY_L0("main", 1);
}


