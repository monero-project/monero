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
#include <boost/format.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <cstdint>
#include <chrono>
#include <cstring>
#include "include_base_utils.h"
using namespace epee;

#include "version.h"
#include "wallet_rpc_server.h"
#include "wallet/wallet_args.h"
#include "common/command_line.h"
#include "common/i18n.h"
#include "common/scoped_message_writer.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/account.h"
#include "multisig/multisig.h"
#include "net/parse.h"
#include "wallet_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "string_coding.h"
#include "string_tools.h"
#include "crypto/hash.h"
#include "mnemonics/electrum-words.h"
#include "rpc/rpc_args.h"
#include "daemonizer/daemonizer.h"
#include "fee_priority.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

#define DEFAULT_AUTO_REFRESH_PERIOD 20 // seconds
#define REFRESH_INDICATIVE_BLOCK_CHUNK_SIZE 256    // just to split refresh in separate calls to play nicer with other threads

#define CHECK_MULTISIG_ENABLED() \
  do \
  { \
    if (m_wallet_impl->multisig().isMultisig && !m_wallet_impl->getEnableMultisig()) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_DISABLED; \
      er.message = "This wallet is multisig, and multisig is disabled. Multisig is an experimental feature and may have bugs. Things that could go wrong include: funds sent to a multisig wallet can't be spent at all, can only be spent with the participation of a malicious group member, or can be stolen by a malicious group member. You can enable it by running this once in monero-wallet-cli: set enable-multisig-experimental 1"; \
      return false; \
    } \
  } while(0)

#define CHECK_IF_BACKGROUND_SYNCING() \
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING_BASE(false)

#define CHECK_IF_RESTRICTED_BACKGROUND_SYNCING() \
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING_BASE(true)

#define CHECK_IF_RESTRICTED_BACKGROUND_SYNCING_BASE(check_restricted) \
  do \
  { \
    if (check_restricted && m_restricted) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_DENIED; \
      er.message = "Command unavailable in restricted mode."; \
      return false; \
    } \
    if (!m_wallet_impl) { return not_open(er); } \
    if (m_wallet_impl->isBackgroundWallet()) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_IS_BACKGROUND_WALLET; \
      er.message = "This command is disabled for background wallets."; \
      return false; \
    } \
    if (m_wallet_impl->isBackgroundSyncing()) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_IS_BACKGROUND_SYNCING; \
      er.message = "This command is disabled while background syncing. Stop background syncing to use this command."; \
      return false; \
    } \
  } while(0)

#define PRE_VALIDATE_BACKGROUND_SYNC() \
  do \
  { \
    if (m_restricted) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_DENIED; \
      er.message = "Command unavailable in restricted mode."; \
      return false; \
    } \
    if (!m_wallet_impl) { return not_open(er); } \
    if (m_wallet_impl->getDeviceState().key_on_device) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR; \
      er.message = "Command not supported by HW wallet"; \
      return false; \
    } \
    if (m_wallet_impl->multisig().isMultisig) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR; \
      er.message = "Multisig wallet cannot enable background sync"; \
      return false; \
    } \
    if (m_wallet_impl->watchOnly()) \
    { \
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY; \
      er.message = "Watch-only wallet cannot enable background sync"; \
      return false; \
    } \
  } while (0)

#define THROW_WALLET_EXCEPTION_ON_API_ERROR() \
    do \
    { \
      int error_code, extended_error_code; \
      std::string error_message; \
      m_wallet_impl->statusWithErrorString(error_code, error_message, &extended_error_code); \
      if (error_code != Wallet::Status::Status_Ok) \
      { \
        LOG_ERROR("Wallet API error: " << error_message); \
        switch (extended_error_code) { \
          case Wallet::ExtendedStatus_WalletInternalError: \
            tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message); \
            break; \
          case Wallet::ExtendedStatus_WalletAlreadyExists: \
            tools::error::throw_wallet_ex<tools::error::file_exists>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* file_name (ignored) */ ""); \
            break; \
          case Wallet::ExtendedStatus_InvalidPassword: \
            tools::error::throw_wallet_ex<tools::error::invalid_password>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          case Wallet::ExtendedStatus_NoDaemonConnection: \
            tools::error::throw_wallet_ex<tools::error::no_connection_to_daemon>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* request (ignored) */ ""); \
            break; \
          case Wallet::ExtendedStatus_DaemonIsBusy: \
            tools::error::throw_wallet_ex<tools::error::daemon_busy>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* request (ignored) */ ""); \
            break; \
          case Wallet::ExtendedStatus_AccountIndexOutOfBounds: \
            tools::error::throw_wallet_ex<tools::error::account_index_outofbound>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          case Wallet::ExtendedStatus_AddressIndexOutOfBounds: \
            tools::error::throw_wallet_ex<tools::error::address_index_outofbound>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          case Wallet::ExtendedStatus_NotEnoughMoney: \
            tools::error::throw_wallet_ex<tools::error::not_enough_money>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* available (ignored) */ 0, /* tx_amount (ignored) */ 0, /* fee (ignored) */ 0); \
            break; \
          case Wallet::ExtendedStatus_NotEnoughUnlockedMoney: \
            tools::error::throw_wallet_ex<tools::error::not_enough_unlocked_money>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* available (ignored) */ 0, /* tx_amount (ignored) */ 0, /* fee (ignored) */ 0); \
            break; \
          case Wallet::ExtendedStatus_NotEnoughOutsToMix: \
          { \
            std::unordered_map<std::uint64_t, std::uint64_t> scanty_outs = {}; \
            tools::error::throw_wallet_ex<tools::error::not_enough_outs_to_mix>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), scanty_outs /* (ignored) */, /* mixin_count (ignored) */ 0); \
            break; \
          } \
          case Wallet::ExtendedStatus_ZeroAmount: \
            tools::error::throw_wallet_ex<tools::error::zero_amount>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          case Wallet::ExtendedStatus_ZeroDestination: \
            tools::error::throw_wallet_ex<tools::error::zero_destination>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          case Wallet::ExtendedStatus_TxNotPossible: \
            tools::error::throw_wallet_ex<tools::error::tx_not_possible>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* available (ignored) */ 0, /* tx_amount (ignored) */ 0, /* fee (ignored) */ 0); \
            break; \
          case Wallet::ExtendedStatus_WrongSignature: \
            tools::error::throw_wallet_ex<tools::error::signature_check_failed>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), /* message (ignored) */ ""); \
            break; \
          case Wallet::ExtendedStatus_NonZeroUnlockTime: \
            tools::error::throw_wallet_ex<tools::error::nonzero_unlock_time>(std::string(__FILE__ ":" STRINGIZE(__LINE__))); \
            break; \
          default: \
            MERROR("unexpected extended_error_code"); \
            tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message); \
            break; \
        } \
      } \
    } while (0)

namespace
{
  const command_line::arg_descriptor<std::string, true> arg_rpc_bind_port = {"rpc-bind-port", "Sets bind port for server"};
  const command_line::arg_descriptor<bool> arg_disable_rpc_login = {"disable-rpc-login", "Disable HTTP authentication for RPC connections served by this process"};
  const command_line::arg_descriptor<bool> arg_restricted = {"restricted-rpc", "Restricts to view-only commands", false};
  const command_line::arg_descriptor<std::string> arg_wallet_dir = {"wallet-dir", "Directory for newly created wallets"};
  const command_line::arg_descriptor<bool> arg_prompt_for_password = {"prompt-for-password", "Prompts for password when not provided", false};
  const command_line::arg_descriptor<bool> arg_no_initial_sync = {"no-initial-sync", "Skips the initial sync before listening for connections", false};
  const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections_per_public_ip = {"rpc-max-connections-per-public-ip", "Max RPC connections per public IP permitted", DEFAULT_RPC_MAX_CONNECTIONS_PER_PUBLIC_IP};
  const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections_per_private_ip = {"rpc-max-connections-per-private-ip", "Max RPC connections per private and localhost IP permitted", DEFAULT_RPC_MAX_CONNECTIONS_PER_PRIVATE_IP};
  const command_line::arg_descriptor<std::size_t> arg_rpc_max_connections = {"rpc-max-connections", "Max RPC connections permitted", DEFAULT_RPC_MAX_CONNECTIONS};
  const command_line::arg_descriptor<std::size_t> arg_rpc_response_soft_limit = {"rpc-response-soft-limit", "Max response bytes that can be queued, enforced at next response attempt", DEFAULT_RPC_SOFT_LIMIT_SIZE};
  const command_line::arg_descriptor<std::string> arg_daemon_address = {"daemon-address", tr("Use daemon instance at <host>:<port>"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_host = {"daemon-host", tr("Use daemon instance at host <arg> instead of localhost"), ""};
  const command_line::arg_descriptor<std::string> arg_proxy = {"proxy", tr("[<ip>:]<port> socks proxy to use for daemon connections"), {}, true};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", tr("Enable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<bool> arg_untrusted_daemon = {"untrusted-daemon", tr("Disable commands which rely on a trusted daemon"), false};
  const command_line::arg_descriptor<std::string> arg_password = {"password", tr("Wallet password (escape/quote as needed)"), "", true};
  const command_line::arg_descriptor<std::string> arg_password_file = wallet_args::arg_password_file();
  const command_line::arg_descriptor<int> arg_daemon_port = {"daemon-port", tr("Use daemon instance at port <arg> instead of 18081"), 0};
  const command_line::arg_descriptor<std::string> arg_daemon_login = {"daemon-login", tr("Specify username[:password] for daemon RPC client"), "", true};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl = {"daemon-ssl", tr("Enable SSL on daemon RPC connections: enabled|disabled|autodetect"), "autodetect"};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_private_key = {"daemon-ssl-private-key", tr("Path to a PEM format private key"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_certificate = {"daemon-ssl-certificate", tr("Path to a PEM format certificate"), ""};
  const command_line::arg_descriptor<std::string> arg_daemon_ssl_ca_certificates = {"daemon-ssl-ca-certificates", tr("Path to file containing concatenated PEM format certificate(s) to replace system CA(s).")};
  const command_line::arg_descriptor<std::vector<std::string>> arg_daemon_ssl_allowed_fingerprints = {"daemon-ssl-allowed-fingerprints", tr("List of valid fingerprints of allowed RPC servers")};
  const command_line::arg_descriptor<bool> arg_daemon_ssl_allow_any_cert = {"daemon-ssl-allow-any-cert", tr("Allow any SSL certificate from the daemon"), false};
  const command_line::arg_descriptor<bool> arg_daemon_ssl_allow_chained = {"daemon-ssl-allow-chained", tr("Allow user (via --daemon-ssl-ca-certificates) chain certificates"), false};
  const command_line::arg_descriptor<bool> arg_allow_mismatched_daemon_version = {"allow-mismatched-daemon-version", tr("Allow communicating with a daemon that uses a different version"), false};
  const command_line::arg_descriptor<bool> arg_testnet = {"testnet", tr("For testnet. Daemon must also be launched with --testnet flag"), false};
  const command_line::arg_descriptor<bool> arg_stagenet = {"stagenet", tr("For stagenet. Daemon must also be launched with --stagenet flag"), false};
  const command_line::arg_descriptor<uint64_t> arg_kdf_rounds = {"kdf-rounds", tr("Number of rounds for the key derivation function"), 1};
  const command_line::arg_descriptor<std::string> arg_hw_device = {"hw-device", tr("HW device to use"), ""};
  const command_line::arg_descriptor<std::string> arg_hw_device_derivation_path = {"hw-device-deriv-path", tr("HW device wallet derivation path (e.g., SLIP-10)"), ""};
  const command_line::arg_descriptor<std::string> arg_tx_notify = { "tx-notify" , tr("Run a program for each new incoming transaction, '%s' will be replaced by the transaction hash") , "" };
  const command_line::arg_descriptor<bool> arg_offline = {"offline", tr("Do not connect to a daemon, nor use DNS"), false};
  const command_line::arg_descriptor<std::string> arg_extra_entropy = {"extra-entropy", tr("File containing extra entropy to initialize the PRNG (any data, aim for 256 bits of entropy to be useful, which typically means more than 256 bits of data)")};


  constexpr const char default_rpc_username[] = "monero";

  boost::optional<tools::password_container> password_prompter(const char *prompt, bool verify)
  {
    auto pwd_container = tools::password_container::prompt(verify, prompt);
    if (!pwd_container)
    {
      MERROR("failed to read wallet password");
    }
    return pwd_container;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void set_confirmations(tools::wallet_rpc::transfer_entry &entry, uint64_t blockchain_height, uint64_t block_reward, uint64_t unlock_time)
  {
    if (entry.height >= blockchain_height || (entry.height == 0 && (!strcmp(entry.type.c_str(), "pending") || !strcmp(entry.type.c_str(), "pool"))))
      entry.confirmations = 0;
    else
      entry.confirmations = blockchain_height - entry.height;

    if (block_reward == 0)
      entry.suggested_confirmations_threshold = 0;
    else
      entry.suggested_confirmations_threshold = (entry.amount + block_reward - 1) / block_reward;

    if (unlock_time < CRYPTONOTE_MAX_BLOCK_NUMBER)
    {
      if (unlock_time > blockchain_height)
        entry.suggested_confirmations_threshold = std::max(entry.suggested_confirmations_threshold, unlock_time - blockchain_height);
    }
    else
    {
      const uint64_t now = time(NULL);
      if (unlock_time > now)
        entry.suggested_confirmations_threshold = std::max(entry.suggested_confirmations_threshold, (unlock_time - now + DIFFICULTY_TARGET_V2 - 1) / DIFFICULTY_TARGET_V2);
    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool get_nettype(const boost::program_options::variables_map &vm, NetworkType &nettype_out)
  {
    bool is_testnet = command_line::has_arg(vm, arg_testnet);
    bool is_stagenet = command_line::has_arg(vm, arg_stagenet);
    if (is_testnet && is_stagenet)
    {
      MERROR(tr("Can't specify more than one of --testnet and --stagenet"));
      return false;
    }
    nettype_out = is_testnet ? NetworkType::TESTNET :
                  is_stagenet ? NetworkType::STAGENET :
                  NetworkType::MAINNET;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  // get password for creating/opening a wallet file on wallet-rpc startup, from either:
  //    --password, --password-file, or --prompt-for-password
  epee::wipeable_string get_password(const boost::program_options::variables_map& vm, const char *prompt, bool verify)
  {
    epee::wipeable_string pw_out;
    const bool is_pw_defaulted = !command_line::has_arg(vm, arg_password);
    const bool is_pw_file_defaulted = command_line::is_arg_defaulted(vm, arg_password_file);
    const bool prompt_for_password = command_line::get_arg(vm, arg_prompt_for_password);
    const auto pw_arg = is_pw_defaulted ? "" : command_line::get_arg(vm, arg_password);
    const auto password_file = command_line::get_arg(vm, arg_password_file);

    if (!is_pw_defaulted + !is_pw_file_defaulted + prompt_for_password != 1)
      THROW_WALLET_EXCEPTION(tools::error::wallet_internal_error, tools::wallet_rpc_server::tr("need to specify exactly one of --prompt-for-password, --password or --password-file"));

    // --password
    if (!is_pw_defaulted)
      pw_out = epee::wipeable_string(pw_arg.data(), pw_arg.size());
    // --password-file
    else if (!is_pw_file_defaulted)
    {
      std::string tmp_pw;
      bool r = epee::file_io_utils::load_file_to_string(password_file, tmp_pw);
      THROW_WALLET_EXCEPTION_IF(!r, tools::error::file_read_error, tools::wallet_rpc_server::tr("the password file specified could not be read"));

      // Remove line breaks the user might have inserted
      boost::trim_right_if(tmp_pw, boost::is_any_of("\r\n"));
      epee::misc_utils::auto_scope_leave_caller tmp_pw_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
        memwipe(&tmp_pw[0], tmp_pw.size());
      });
      pw_out = epee::wipeable_string(tmp_pw.data(), tmp_pw.size());
    }
    // --prompt-for-password
    else
    {
      const auto pwd_container = password_prompter(prompt, verify);
      pw_out = pwd_container->password();
    }

    return pw_out;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool get_daemon_address(const boost::program_options::variables_map &vm, std::string &daemon_address, std::string &daemon_username, std::string &daemon_password)
  {
    daemon_address = command_line::get_arg(vm, arg_daemon_address);
    auto daemon_host = command_line::get_arg(vm, arg_daemon_host);
    auto daemon_port = command_line::get_arg(vm, arg_daemon_port);

    THROW_WALLET_EXCEPTION_IF(!daemon_address.empty() && !daemon_host.empty() && 0 != daemon_port,
        tools::error::wallet_internal_error, tr("can't specify daemon host or port more than once"));

    if (daemon_host.empty())
      daemon_host = "localhost";

    if (!daemon_port)
    {
      NetworkType nettype;
      if (!get_nettype(vm, nettype))
        return false;
      daemon_port = get_config(static_cast<cryptonote::network_type>(nettype)).RPC_DEFAULT_PORT;
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
  //------------------------------------------------------------------------------------------------------------------------------
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
         tr("Invalid argument for ") + std::string(arg_daemon_ssl.name));
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
        tr("Enabling --") + std::string{use_proxy ? arg_proxy.name : arg_daemon_ssl.name} + tr(" requires --") +
          arg_daemon_ssl_allow_any_cert.name + tr(" or --") +
          arg_daemon_ssl_ca_certificates.name + tr(" or --") + arg_daemon_ssl_allowed_fingerprints.name + tr(" or use of a .onion/.i2p domain")
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
      tools::error::wallet_internal_error, tr("--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted"));

    // set --trusted-daemon if local and not overridden
    if (!trusted_daemon)
    {
      try
      {
        trusted_daemon = false;
        if (tools::is_local_address(daemon_address))
        {
          MINFO(tr("Daemon is local, assuming trusted"));
          trusted_daemon = true;
        }
      }
      catch (const std::exception &e) { }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool init_wallet(const boost::program_options::variables_map &vm, std::unique_ptr<Wallet> &wal, std::unique_ptr<WalletManager> &wallet_manager)
  {
    std::string daemon_address, daemon_username, daemon_password;
    boost::optional<bool> is_trusted_daemon;
    Monero::Wallet::SSLSupport ssl_support = Wallet::SSLSupport::SSLSupport_Autodetect;
    std::string ssl_private_key_path, ssl_certificate_path, ssl_ca_file_path;
    std::vector<std::string> ssl_allowed_fingerprints_str;
    bool ssl_allow_any_cert = false;
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
    catch (const std::exception &e) {
      MERROR(tr("failed to initialize wallet: ") << e.what());
      return false;
    }

    if (command_line::get_arg(vm, arg_offline))
      wal->setOffline(true);

    const std::string tx_notify = command_line::get_arg(vm, arg_tx_notify);
    if (!tx_notify.empty())
      wal->setTxNotify(tx_notify);

    const std::string hw_device_name = command_line::get_arg(vm, arg_hw_device);
    if (!hw_device_name.empty())
      wal->setDeviceName(hw_device_name);
    const std::string device_derivation_path = command_line::get_arg(vm, arg_hw_device_derivation_path);
    if (!device_derivation_path.empty())
      wal->setDeviceDerivationPath(device_derivation_path);

    if (command_line::get_arg(vm, arg_allow_mismatched_daemon_version))
      wal->setAllowMismatchedDaemonVersion(true);

    std::pair<std::string, std::string> daemon_login = std::make_pair(daemon_username, daemon_password);
    wallet_manager->setDaemonAddress(daemon_address, daemon_username.empty() ? nullptr : &daemon_login);
    wal->init(daemon_address,
              /* upper_transaction_size_limit */ 0,
              daemon_username,
              daemon_password,
              /* use_ssl */ false,
              /* lightWallet */ false);

    return wal->setDaemon(daemon_address,
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
}

namespace tools
{
  const char* wallet_rpc_server::tr(const char* str)
  {
    return i18n_translate(str, "tools::wallet_rpc_server");
  }

  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server():rpc_login_file(), m_stop(false), m_restricted(false), m_vm(NULL)
  {
  }
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::~wallet_rpc_server()
  {
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::set_wallet(Wallet *cr)
  { m_wallet_impl.reset(cr); }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::set_wallet_manager(WalletManager *wallet_manager)
  { m_wallet_manager.reset(wallet_manager); }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::run()
  {
    m_stop = false;

    const auto auto_refresh_evaluation_ms = std::chrono::milliseconds(200);

    m_net_server.add_idle_handler([=] { // Implicit capture of this-pointer deprecated in C++20.
      const auto auto_refresh_period = m_auto_refresh_period.load(std::memory_order_relaxed);
      if (auto_refresh_period == 0) // disabled
        return true;

      // Check if m_auto_refresh_period seconds have passed since the last refresh attempt.
      const auto auto_refresh_interval_ms = std::chrono::milliseconds(auto_refresh_period * 1'000);
      if (auto_refresh_interval_ms <= auto_refresh_evaluation_ms)
      {
        LOG_PRINT_L0((boost::format(tr("The auto wallet sync evaluation interval of %i ms must be larger than the refresh interval of %i ms"))
          % auto_refresh_evaluation_ms.count()
          % auto_refresh_interval_ms.count()).str());
        return true;
      }

      const auto now = std::chrono::steady_clock::now();
      if (now < m_last_auto_refresh_time + auto_refresh_interval_ms)
        return true;

      uint64_t blocks_fetched = 0;
      bool refresh_success = false;
      const auto start = std::chrono::steady_clock::now();

      try
      {
        bool received_money = false;
        if (m_wallet_impl) m_wallet_impl->refresh(/* start_height */ 0,
                                                  /* check_pool */ true,
                                                  /* try_incremental */ true,
                                                  REFRESH_INDICATIVE_BLOCK_CHUNK_SIZE,
                                                  /* skip_refresh_if_daemon_not_synced */ false,
                                                  &blocks_fetched,
                                                  &received_money);
        refresh_success = true;
      }
      catch (const std::exception& ex)
      {
        LOG_ERROR("Exception at while refreshing, what=" << ex.what());
      }

      const auto end = std::chrono::steady_clock::now();
      const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

      if (refresh_success)
        LOG_PRINT_L3((boost::format(tr("Automated wallet block refresh took %i ms")) % elapsed.count()).str());

      const bool syncing_against_tip_of_chain = blocks_fetched < REFRESH_INDICATIVE_BLOCK_CHUNK_SIZE;
      if (syncing_against_tip_of_chain)
      {
        // At this point, we can poll for a refresh every m_auto_refresh_period seconds.
        m_last_auto_refresh_time = end;
      }
      else
      {
        // We are in a state of synchronization, blasting through the maximum chunks of blocks
        // because we are not at the tip of the chain. In this case, if we update m_last_auto_refresh_time,
        // we'll need to wait an entire m_refresh_interval_ms before processing the next batch. On the other hand,
        // if we do not update m_last_auto_refresh_time, we'll never yield (other calls to the RPC will hang)
        // in the case that elapsed > auto_refresh_evaluation_ms since we'll immediately be scheduled for another block sync.
        const bool over_one_refresh_period_passed = end > m_last_auto_refresh_time + auto_refresh_interval_ms;
        if (over_one_refresh_period_passed)
        {
          // auto_refresh_interval_ms of straight-blasting through blocks has elapsed without end.
          // Let's free up the network thread for between 200ms to 300ms (non-deterministic) to handle other requests.
          const auto refresh_throttle = auto_refresh_evaluation_ms + std::chrono::milliseconds(100);
          m_last_auto_refresh_time = end - auto_refresh_interval_ms + refresh_throttle;
          LOG_PRINT_L3((boost::format(tr("Temporarily throttling wallet block refresh by around %i ms")) % refresh_throttle.count()).str());
        }
      }
      return true;
    }, auto_refresh_evaluation_ms);
    m_net_server.add_idle_handler([this](){
      if (m_stop.load(std::memory_order_relaxed))
      {
        send_stop_signal();
        return false;
      }
      return true;
    }, std::chrono::milliseconds{500});

    //DO NOT START THIS SERVER IN MORE THEN 1 THREADS WITHOUT REFACTORING
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::run(1, true);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::stop()
  {
    if (m_wallet_impl)
    {
      m_wallet_impl->stop();

      if (!m_wallet_manager->closeWallet(m_wallet_impl.get(), /* store */ true, /* do_delete_pointer */ false))
      {
        MERROR(tr("failed to deinitialize wallet"));
        std::string error_msg = m_wallet_manager->errorString();
        if (!error_msg.empty())
          MERROR(error_msg);
      }

    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::init(const boost::program_options::variables_map *vm)
  {
    auto rpc_config = cryptonote::rpc_args::process(*vm);
    if (!rpc_config)
      return false;

    m_vm = vm;

    boost::optional<epee::net_utils::http::login> http_login{};
    std::string bind_port = command_line::get_arg(*m_vm, arg_rpc_bind_port);
    const bool disable_auth = command_line::get_arg(*m_vm, arg_disable_rpc_login);
    m_restricted = command_line::get_arg(*m_vm, arg_restricted);
    if (!command_line::is_arg_defaulted(*m_vm, arg_wallet_dir))
    {
      if (!command_line::is_arg_defaulted(*m_vm, wallet_args::arg_wallet_file()))
      {
        MERROR(arg_wallet_dir.name << " and " << wallet_args::arg_wallet_file().name << " are incompatible, use only one of them");
        return false;
      }
      m_wallet_dir = command_line::get_arg(*m_vm, arg_wallet_dir);
#ifdef _WIN32
#define MKDIR(path, mode)    mkdir(path)
#else
#define MKDIR(path, mode)    mkdir(path, mode)
#endif
      if (!m_wallet_dir.empty() && MKDIR(m_wallet_dir.c_str(), 0700) < 0 && errno != EEXIST)
      {
#ifdef _WIN32
        LOG_ERROR(tr("Failed to create directory ") + m_wallet_dir);
#else
        LOG_ERROR((boost::format(tr("Failed to create directory %s: %s")) % m_wallet_dir % strerror(errno)).str());
#endif
        return false;
      }
    }

    if (disable_auth)
    {
      if (rpc_config->login)
      {
        const cryptonote::rpc_args::descriptors arg{};
        LOG_ERROR(tr("Cannot specify --") << arg_disable_rpc_login.name << tr(" and --") << arg.rpc_login.name);
        return false;
      }
    }
    else // auth enabled
    {
      if (!rpc_config->login)
      {
        std::array<std::uint8_t, 16> rand_128bit{{}};
        crypto::rand(rand_128bit.size(), rand_128bit.data());
        http_login.emplace(
          default_rpc_username,
          string_encoding::base64_encode(rand_128bit.data(), rand_128bit.size())
        );

        std::string temp = "monero-wallet-rpc." + bind_port + ".login";
        rpc_login_file = tools::private_file::drop_and_recreate(temp);
        if (!rpc_login_file.handle())
        {
          LOG_ERROR(tr("Failed to create file ") << temp << tr(". Check permissions or remove file"));
          return false;
        }
        std::fputs(http_login->username.c_str(), rpc_login_file.handle());
        std::fputc(':', rpc_login_file.handle());
        const epee::wipeable_string password = http_login->password;
        std::fwrite(password.data(), 1, password.size(), rpc_login_file.handle());
        std::fflush(rpc_login_file.handle());
        if (std::ferror(rpc_login_file.handle()))
        {
          LOG_ERROR(tr("Error writing to file ") << temp);
          return false;
        }
        LOG_PRINT_L0(tr("RPC username/password is stored in file ") << temp);
      }
      else // chosen user/pass
      {
        http_login.emplace(
          std::move(rpc_config->login->username), std::move(rpc_config->login->password).password()
        );
      }
      assert(bool(http_login));
    } // end auth enabled

    m_auto_refresh_period.store(DEFAULT_AUTO_REFRESH_PERIOD, std::memory_order_relaxed);
    const auto over_one_period_ago = std::chrono::steady_clock::now() - std::chrono::seconds(m_auto_refresh_period.load(std::memory_order_relaxed) * 2);
    m_last_auto_refresh_time = over_one_period_ago;

    check_background_mining();

    const auto max_connections_public = command_line::get_arg(vm, arg_rpc_max_connections_per_public_ip);
    const auto max_connections_private = command_line::get_arg(vm, arg_rpc_max_connections_per_private_ip);
    const auto max_connections = command_line::get_arg(vm, arg_rpc_max_connections);

    if (max_connections < max_connections_public)
    {
      MFATAL(arg_rpc_max_connections_per_public_ip.name << " is bigger than " << arg_rpc_max_connections.name);
      return false;
    }
    if (max_connections < max_connections_private)
    {
      MFATAL(arg_rpc_max_connections_per_private_ip.name << " is bigger than " << arg_rpc_max_connections.name);
      return false;
    }

    m_net_server.set_threads_prefix("RPC");
    m_net_server.get_config_object().m_max_content_length = MAX_RPC_CONTENT_LENGTH * 100;
    auto rng = [](size_t len, uint8_t *ptr) { return crypto::rand(len, ptr); };
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::init(
      rng, std::move(bind_port), std::move(rpc_config->bind_ip),
      std::move(rpc_config->bind_ipv6_address), std::move(rpc_config->use_ipv6), std::move(rpc_config->require_ipv4),
      std::move(rpc_config->access_control_origins), std::move(http_login),
      std::move(rpc_config->ssl_options),
      max_connections_public, max_connections_private, max_connections,
      command_line::get_arg(vm, arg_rpc_response_soft_limit)
    );
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::check_background_mining()
  {
    if (!m_wallet_impl)
      return;
    // Background mining can be toggled from the main wallet
    if (m_wallet_impl->isBackgroundWallet() || m_wallet_impl->isBackgroundSyncing())
      return;

    Wallet::BackgroundMiningSetupType setup = m_wallet_impl->getSetupBackgroundMining();
    if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_No)
    {
      MLOG_RED(el::Level::Warning, "Background mining not enabled. Run \"set setup-background-mining 1\" in monero-wallet-cli to change.");
      return;
    }

    if (!m_wallet_impl->trustedDaemon())
    {
      MDEBUG("Using an untrusted daemon, skipping background mining check");
      return;
    }

    if (m_wallet_manager->isMining() || m_wallet_manager->isBackgroundMiningEnabled())
        return;
    else if (!m_wallet_manager->errorString().empty())
    {
      MERROR("Failed to query mining status: " << m_wallet_manager->errorString());
      return;
    }

    if (setup == Wallet::BackgroundMiningSetupType::BackgroundMining_Maybe)
    {
      MINFO("The daemon is not set up to background mine.");
      MINFO("With background mining enabled, the daemon will mine when idle and not on battery.");
      MINFO("Enabling this supports the network you are using, and makes you eligible for receiving new monero");
      MINFO("Set setup-background-mining to 1 in monero-wallet-cli to change.");
      return;
    }

    if (!m_wallet_manager->startMining(m_wallet_impl->address(),
                                       /* threads */ 1,
                                       /* background_mining */ true,
                                       /* ignore_battery */ false))
    {
      std::string err = m_wallet_manager->errorString();
      MERROR("Failed to setup background mining: " << (!err.empty() ? err : "No connection to daemon"));
      return;
    }

    MINFO("Background mining enabled. The daemon will mine when idle and not on battery.");
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::not_open(epee::json_rpc::error& er)
  {
      er.code = WALLET_RPC_ERROR_CODE_NOT_OPEN;
      er.message = "No wallet file";
      return false;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const TransactionInfo &tx_info)
  {
    entry.txid = tx_info.hash();
    entry.payment_id = tx_info.paymentId();
    if (entry.payment_id.substr(16).find_first_not_of('0') == std::string::npos)
      entry.payment_id = entry.payment_id.substr(0,16);
    entry.height = tx_info.blockHeight();
    entry.timestamp = tx_info.timestamp();
    entry.amount = tx_info.amount();
    if (tx_info.direction() == TransactionInfo::Direction_In) {
      const auto eds = m_wallet_impl->getEnoteDetails();
      for (const auto &ed : eds) {
        if (ed->txId() == tx_info.hash())
          entry.amounts.push_back(ed->amount());
      }
      entry.subaddr_index = { tx_info.subaddrAccount(), *tx_info.subaddrIndex().begin() };
      entry.type = tx_info.isPending() ? "pool" : tx_info.isCoinbase() ? "block" : "in";
    }
    else if (tx_info.direction() == TransactionInfo::Direction_Out) {
      for (const auto &t: tx_info.transfers()) {
        entry.destinations.push_back(wallet_rpc::transfer_destination());
        wallet_rpc::transfer_destination &td = entry.destinations.back();
        td.amount = t.amount;
        td.address = t.address;
      }
      entry.type = tx_info.isPending() ? "pending" : tx_info.isFailed() ? "failed" : "out";
    }
    for (uint32_t i: tx_info.subaddrIndex())
      entry.subaddr_indices.push_back({ tx_info.subaddrAccount(), i });
    entry.unlock_time = tx_info.unlockTime();
    entry.locked = !tx_info.isUnlocked();
    entry.fee = tx_info.fee();
    entry.note = tx_info.description();
    entry.address = m_wallet_impl->address(tx_info.subaddrAccount(), *tx_info.subaddrIndex().begin());
    set_confirmations(entry, m_wallet_impl->blockChainHeight(), m_wallet_impl->getLastBlockReward(), tx_info.unlockTime());
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      res.balance = req.all_accounts ? m_wallet_impl->balanceAll(req.strict) : m_wallet_impl->balance(req.account_index, req.strict);
      res.unlocked_balance = req.all_accounts ? m_wallet_impl->unlockedBalanceAll(&res.blocks_to_unlock, &res.time_to_unlock, req.strict) : m_wallet_impl->unlockedBalance(req.account_index, &res.blocks_to_unlock, &res.time_to_unlock, req.strict);
      res.multisig_import_needed = m_wallet_impl->multisig().isMultisig && m_wallet_impl->hasMultisigPartialKeyImages();
      std::map<uint32_t, std::map<uint32_t, uint64_t>> balance_per_subaddress_per_account;
      std::map<uint32_t, std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>>> unlocked_balance_per_subaddress_per_account;
      if (req.all_accounts)
      {
        for (uint32_t account_index = 0; account_index < m_wallet_impl->numSubaddressAccounts(); ++account_index)
        {
          balance_per_subaddress_per_account[account_index] = m_wallet_impl->balancePerSubaddress(account_index, req.strict);
          unlocked_balance_per_subaddress_per_account[account_index] = m_wallet_impl->unlockedBalancePerSubaddress(account_index, req.strict);
        }
      }
      else
      {
        balance_per_subaddress_per_account[req.account_index] = m_wallet_impl->balancePerSubaddress(req.account_index, req.strict);
        unlocked_balance_per_subaddress_per_account[req.account_index] = m_wallet_impl->unlockedBalancePerSubaddress(req.account_index, req.strict);
      }
      std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet_impl->getEnoteDetails();
      for (const auto& p : balance_per_subaddress_per_account)
      {
        uint32_t account_index = p.first;
        std::map<uint32_t, uint64_t> balance_per_subaddress = p.second;
        std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> unlocked_balance_per_subaddress = unlocked_balance_per_subaddress_per_account[account_index];
        std::set<uint32_t> address_indices;
        if (!req.all_accounts && !req.address_indices.empty())
        {
          address_indices = req.address_indices;
        }
        else
        {
          for (const auto& i : balance_per_subaddress)
            address_indices.insert(i.first);
        }
        for (uint32_t i : address_indices)
        {
          wallet_rpc::COMMAND_RPC_GET_BALANCE::per_subaddress_info info;
          info.account_index = account_index;
          info.address_index = i;
          info.address = m_wallet_impl->address(info.account_index, info.address_index);
          info.balance = balance_per_subaddress[i];
          info.unlocked_balance = unlocked_balance_per_subaddress[i].first;
          info.blocks_to_unlock = unlocked_balance_per_subaddress[i].second.first;
          info.time_to_unlock = unlocked_balance_per_subaddress[i].second.second;
          info.label = m_wallet_impl->getSubaddressLabel(info.account_index, info.address_index);
          info.num_unspent_outputs = std::count_if(eds.begin(), eds.end(), [&](const std::unique_ptr<EnoteDetails> &ed) {
              return !ed->isSpent() && ed->subaddressIndexMajor() == info.account_index && ed->subaddressIndexMinor() && info.address_index;
          });
          res.per_subaddress.emplace_back(std::move(info));
        }
      }
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      THROW_WALLET_EXCEPTION_IF(req.account_index >= m_wallet_impl->numSubaddressAccounts(), error::account_index_outofbound);
      res.addresses.clear();
      std::vector<uint32_t> req_address_index;
      if (req.address_index.empty())
      {
        for (uint32_t i = 0; i < m_wallet_impl->numSubaddresses(req.account_index); ++i)
          req_address_index.push_back(i);
      }
      else
      {
        req_address_index = req.address_index;
      }
      std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet_impl->getEnoteDetails();
      for (uint32_t i : req_address_index)
      {
        THROW_WALLET_EXCEPTION_IF(i >= m_wallet_impl->numSubaddresses(req.account_index), error::address_index_outofbound);
        res.addresses.resize(res.addresses.size() + 1);
        auto& info = res.addresses.back();
        info.address = m_wallet_impl->address(req.account_index, i);
        info.label = m_wallet_impl->getSubaddressLabel(req.account_index, i);
        info.address_index = i;
        info.used = std::find_if(eds.begin(), eds.end(), [&](const std::unique_ptr<EnoteDetails> &ed) { return ed->subaddressIndexMajor() == req.account_index && ed->subaddressIndexMinor() == i; }) != eds.end();
      }
      res.address = m_wallet_impl->address(req.account_index, 0);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getaddress_index(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_INDEX::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_INDEX::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }
    int error_code;
    std::string error_msg;
    auto index = m_wallet_impl->getSubaddressIndex(req.address);
    m_wallet_impl->statusWithErrorString(error_code, error_msg);
    if (error_code != Wallet::Status::Status_Ok)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Address doesn't belong to the wallet";
      return false;
    }
    res.index = {index.first, index.second};
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_subaddr_lookahead(const wallet_rpc::COMMAND_RPC_SET_SUBADDR_LOOKAHEAD::request& req, wallet_rpc::COMMAND_RPC_SET_SUBADDR_LOOKAHEAD::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    const std::string wallet_file = m_wallet_impl->filename();
    if (wallet_file == "" || m_wallet_impl->verifyPassword(req.password))
    {
      try
      {
        m_wallet_impl->setSubaddressLookahead(req.major_idx, req.minor_idx);
        THROW_WALLET_EXCEPTION_ON_API_ERROR();

        m_wallet_impl->rewriteWalletFile(wallet_file, req.password);
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    else
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_PASSWORD;
      er.message = "Invalid password.";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_create_address(const wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      if (req.count < 1 || req.count > 65536) {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Count must be between 1 and 65536.";
        return false;
      }
      THROW_WALLET_EXCEPTION_IF(req.account_index > m_wallet_impl->numSubaddressAccounts(), error::account_index_outofbound);

      std::vector<std::string> addresses;
      std::vector<uint32_t>    address_indices;

      addresses.reserve(req.count);
      address_indices.reserve(req.count);

      for (uint32_t i = 0; i < req.count; i++) {
        m_wallet_impl->addSubaddress(req.account_index, req.label);
        THROW_WALLET_EXCEPTION_ON_API_ERROR();

        uint32_t new_address_index = m_wallet_impl->numSubaddresses(req.account_index) - 1;
        address_indices.push_back(new_address_index);
        addresses.push_back(m_wallet_impl->address(req.account_index, new_address_index));
      }

      res.address = addresses[0];
      res.address_index = address_indices[0];
      res.addresses = addresses;
      res.address_indices = address_indices;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_label_address(const wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      THROW_WALLET_EXCEPTION_IF(req.index.major > m_wallet_impl->numSubaddressAccounts(), error::account_index_outofbound);
      THROW_WALLET_EXCEPTION_IF(req.index.minor > m_wallet_impl->numSubaddresses(req.index.major), error::address_index_outofbound);
      m_wallet_impl->setSubaddressLabel(req.index.major, req.index.minor, req.label);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_accounts(const wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      res.total_balance = 0;
      res.total_unlocked_balance = 0;
      cryptonote::subaddress_index subaddr_index = {0,0};
      const std::pair<std::map<std::string, std::string>, std::vector<std::string>> account_tags = m_wallet_impl->getAccountTags();
      if (!req.tag.empty() && account_tags.first.count(req.tag) == 0 && !req.regexp)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = (boost::format(tr("Tag %s is unregistered.")) % req.tag).str();
        return false;
      }
      for (; subaddr_index.major < m_wallet_impl->numSubaddressAccounts(); ++subaddr_index.major)
      {
        bool no_match = !req.regexp ? (!req.tag.empty() && req.tag != account_tags.second[subaddr_index.major])
          : (!req.tag.empty() && !boost::regex_match(account_tags.second[subaddr_index.major], boost::regex(req.tag)));
        if (no_match)
          continue;
        wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::subaddress_account_info info;
        info.account_index = subaddr_index.major;
        info.base_address = m_wallet_impl->address(subaddr_index.major, subaddr_index.minor);
        info.balance = m_wallet_impl->balance(subaddr_index.major, req.strict_balances);
        info.unlocked_balance = m_wallet_impl->unlockedBalance(subaddr_index.major, /* blocks_to_unlock */ NULL, /* time_to_unlock */ NULL, req.strict_balances);
        info.label = m_wallet_impl->getSubaddressLabel(subaddr_index.major, subaddr_index.minor);
        info.tag = account_tags.second[subaddr_index.major];
        res.subaddress_accounts.push_back(info);
        res.total_balance += info.balance;
        res.total_unlocked_balance += info.unlocked_balance;
      }
      if (res.subaddress_accounts.size() == 0 && req.regexp)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = (boost::format(tr("No matches for regex filter %s .")) % req.tag).str();
        return false;
      }
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_create_account(const wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      m_wallet_impl->addSubaddressAccount(req.label);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      res.account_index = m_wallet_impl->numSubaddressAccounts() - 1;
      res.address = m_wallet_impl->address(res.account_index, /* minor_idx */ 0);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_label_account(const wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      THROW_WALLET_EXCEPTION_IF(req.account_index > m_wallet_impl->numSubaddressAccounts(), error::account_index_outofbound);
      m_wallet_impl->setSubaddressLabel(req.account_index, /* minor_idx */ 0, req.label);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_account_tags(const wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();
    const std::pair<std::map<std::string, std::string>, std::vector<std::string>> account_tags = m_wallet_impl->getAccountTags();
    for (const std::pair<const std::string, std::string>& p : account_tags.first)
    {
      res.account_tags.resize(res.account_tags.size() + 1);
      auto& info = res.account_tags.back();
      info.tag = p.first;
      info.label = p.second;
      for (size_t i = 0; i < account_tags.second.size(); ++i)
      {
        if (account_tags.second[i] == info.tag)
          info.accounts.push_back(i);
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_tag_accounts(const wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      m_wallet_impl->setAccountTag(req.accounts, req.tag);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_untag_accounts(const wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      m_wallet_impl->setAccountTag(req.accounts, "");
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_account_tag_description(const wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::request& req, wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      m_wallet_impl->setAccountTagDescription(req.tag, req.description);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getheight(const wallet_rpc::COMMAND_RPC_GET_HEIGHT::request& req, wallet_rpc::COMMAND_RPC_GET_HEIGHT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      res.height = m_wallet_impl->blockChainHeight();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_freeze(const wallet_rpc::COMMAND_RPC_FREEZE::request& req, wallet_rpc::COMMAND_RPC_FREEZE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      if (req.key_image.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = std::string("Must specify key image to freeze");
        return false;
      }
      crypto::key_image ki;
      if (!epee::string_tools::hex_to_pod(req.key_image, ki))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
        er.message = "failed to parse key image";
        return false;
      }
      m_wallet_impl->freeze(req.key_image);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_thaw(const wallet_rpc::COMMAND_RPC_THAW::request& req, wallet_rpc::COMMAND_RPC_THAW::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      if (req.key_image.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = std::string("Must specify key image to thaw");
        return false;
      }
      crypto::key_image ki;
      if (!epee::string_tools::hex_to_pod(req.key_image, ki))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
        er.message = "failed to parse key image";
        return false;
      }
      m_wallet_impl->thaw(req.key_image);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_frozen(const wallet_rpc::COMMAND_RPC_FROZEN::request& req, wallet_rpc::COMMAND_RPC_FROZEN::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();
    try
    {
      if (req.key_image.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = std::string("Must specify key image to check if frozen");
        return false;
      }
      crypto::key_image ki;
      if (!epee::string_tools::hex_to_pod(req.key_image, ki))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
        er.message = "failed to parse key image";
        return false;
      }
      res.frozen = m_wallet_impl->isFrozen(req.key_image);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::validate_transfer(const std::list<wallet_rpc::transfer_destination>& destinations, const std::string& payment_id, std::vector<std::string>& dsts, std::vector<std::uint64_t>& amounts_per_dst, bool at_least_one_destination, epee::json_rpc::error& er)
  {
    CHECK_IF_BACKGROUND_SYNCING();

    crypto::hash8 integrated_payment_id = crypto::null_hash8;
    for (auto it = destinations.begin(); it != destinations.end(); it++)
    {
      cryptonote::address_parse_info info;
      er.message = "";
      if(!get_account_address_from_str_or_url(info, nettype(), it->address,
        [&er](const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)->std::string {
          if (!dnssec_valid)
          {
            er.message = std::string("Invalid DNSSEC for ") + url;
            return {};
          }
          if (addresses.empty())
          {
            er.message = std::string("No Monero address found at ") + url;
            return {};
          }
          return addresses[0];
        }))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        if (er.message.empty())
          er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + it->address;
        return false;
      }

      dsts.push_back(it->address);
      amounts_per_dst.push_back(it->amount);

      if (info.has_payment_id)
      {
        if (!payment_id.empty() || integrated_payment_id != crypto::null_hash8)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "A single payment id is allowed per transaction";
          return false;
        }
        integrated_payment_id = info.payment_id;
      }
    }

    if (at_least_one_destination && dsts.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_ZERO_DESTINATION;
      er.message = "Transaction has no destination";
      return false;
    }

    if (!payment_id.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = "Standalone payment IDs are obsolete. Use subaddresses or integrated addresses instead";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template<typename T> static bool is_error_value(const T &val) { return false; }
  static bool is_error_value(const std::string &s) { return s.empty(); }
  //------------------------------------------------------------------------------------------------------------------------------
  template<typename T, typename V>
  static bool fill(T &where, V s)
  {
    if (is_error_value(s)) return false;
    where = std::move(s);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template<typename T, typename V>
  static bool fill(std::list<T> &where, V s)
  {
    if (is_error_value(s)) return false;
    where.emplace_back(std::move(s));
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  static uint64_t total_amount(const std::vector<std::uint64_t> &amounts)
  {
    uint64_t amount = 0;
    for (const auto &amt: amounts) amount += amt;
    return amount;
  }
  //------------------------------------------------------------------------------------------------------------------------------
template<typename Ts, typename Tu, typename Tk, typename Ta>
  bool wallet_rpc_server::fill_response(std::unique_ptr<PendingTransaction> ptx_vector,
      bool get_tx_key, Ts& tx_key, Tu &amount, Ta &amounts_by_dest, Tu &fee, Tu &weight, std::string &multisig_txset, std::string &unsigned_txset, bool do_not_relay,
      Ts &tx_hash, bool get_tx_hex, Ts &tx_blob, bool get_tx_metadata, Ts &tx_metadata, Tk &spent_key_images, epee::json_rpc::error &er)
  {
    const size_t tx_count = ptx_vector->txCount();
    const std::vector<std::string> &tx_ids = ptx_vector->txid();
    const std::vector<std::vector<std::uint64_t>> &amounts = ptx_vector->amountsPerDestination();
    const std::vector<std::uint64_t> &fees = ptx_vector->fees();
    const std::vector<std::uint64_t> &tx_weights = ptx_vector->txWeights();
    const std::vector<std::string> &tx_hex_strings = ptx_vector->asHexStr();
    const std::vector<std::string> &tx_blobs = ptx_vector->convertTxToRawBlobStr();
    const std::vector<std::string> &tx_key_str = ptx_vector->getTxKeys();
    const std::vector<std::vector<std::unique_ptr<EnoteDetails>>> &eds = ptx_vector->getEnoteDetailsIn();
    for (size_t i = 0; i < tx_count; ++i)
    {
      if (get_tx_key)
      {
        fill(tx_key, tx_key_str[i]);
      }
      // Compute amount leaving wallet in tx. By convention dests does not include change outputs
      fill(amount, total_amount(amounts[i]));
      fill(fee, fees[i]);
      fill(weight, tx_weights[i]);

      // add amounts by destination
      tools::wallet_rpc::amounts_list abd;
      for (const auto& dst : amounts[i])
        abd.amounts.push_back(dst);
      fill(amounts_by_dest, abd);

      // add spent key images
      tools::wallet_rpc::key_image_list key_image_list;
      for (const auto &ed : eds[i])
        key_image_list.key_images.push_back(ed->keyImage());
      fill(spent_key_images, key_image_list);
    }

    if (m_wallet_impl->multisig().isMultisig)
    {
      multisig_txset = epee::string_tools::buff_to_hex_nodelimer(m_wallet_impl->convertMultisigTxToStr(*ptx_vector));
      if (multisig_txset.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to save multisig tx set after creation";
        return false;
      }
    }
    else
    {
      if (m_wallet_impl->watchOnly()){
        unsigned_txset = epee::string_tools::buff_to_hex_nodelimer(ptx_vector->convertTxToStr());
        if (unsigned_txset.empty())
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Failed to save unsigned tx set after creation";
          return false;
        }
      }
      else if (!do_not_relay)
        ptx_vector->commit();

      // populate response with tx hashes
      for (size_t i = 0; i < tx_count; ++i)
      {
        bool r = fill(tx_hash, tx_ids[i]);
        r = r && (!get_tx_hex || fill(tx_blob, epee::string_tools::buff_to_hex_nodelimer(tx_blobs[i])));
        r = r && (!get_tx_metadata || fill(tx_metadata, tx_hex_strings[i]));
        if (!r)
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Failed to save tx info";
          return false;
        }
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {

    std::vector<std::string> dst_addr{};
    std::vector<std::uint64_t> amt_per_dst{};

    LOG_PRINT_L3("on_transfer starts");
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    else if (req.unlock_time)
    {
      er.code = WALLET_RPC_ERROR_CODE_NONZERO_UNLOCK_TIME;
      er.message = "Transaction cannot have non-zero unlock time";
      return false;
    }
    else if (!fee_priority_utilities::is_valid(req.priority))
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_FEE_PRIORITY;
      er.message = "Invalid priority value. Must be between 0 and 4.";
      return false;
    }

    CHECK_MULTISIG_ENABLED();

    // validate the transfer requested and populate dsts and amounts
    if (!validate_transfer(req.destinations, req.payment_id, dst_addr, amt_per_dst, /* at_least_one_destination */ true, er))
    {
      return false;
    }

    try
    {
      std::unique_ptr<PendingTransaction> ptx(
              m_wallet_impl->createTransactionMultDest(dst_addr,
                                                       /* [deprecated] payment_id */ "",
                                                       amt_per_dst,
                                                       req.ring_size ? req.ring_size - 1 : 0,
                                                       static_cast<PendingTransaction::Priority>(req.priority),
                                                       req.account_index,
                                                       req.subaddr_indices,
                                                       req.subtract_fee_from_outputs));
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      if (!ptx)
      {
        er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
        er.message = "No transaction created";
        return false;
      }

      // reject proposed transactions if there are more than one.  see on_transfer_split below.
      if (ptx->txCount() != 1)
      {
        er.code = WALLET_RPC_ERROR_CODE_TX_TOO_LARGE;
        er.message = "Transaction would be too large.  try /transfer_split.";
        return false;
      }
      return fill_response(std::move(ptx), req.get_tx_key, res.tx_key, res.amount, res.amounts_by_dest, res.fee, res.weight, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash, req.get_tx_hex, res.tx_blob, req.get_tx_metadata, res.tx_metadata, res.spent_key_images, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    std::vector<std::string> dst_addr{};
    std::vector<std::uint64_t> amt_per_dst{};

    if (!m_wallet_impl) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    else if (req.unlock_time)
    {
      er.code = WALLET_RPC_ERROR_CODE_NONZERO_UNLOCK_TIME;
      er.message = "Transaction cannot have non-zero unlock time";
      return false;
    }
    else if (!fee_priority_utilities::is_valid(req.priority))
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_FEE_PRIORITY;
      er.message = "Invalid priority value. Must be between 0 and 4.";
      return false;
    }

    CHECK_MULTISIG_ENABLED();

    // validate the transfer requested and populate dsts; RPC_TRANSFER::request and RPC_TRANSFER_SPLIT::request are identical types.
    if (!validate_transfer(req.destinations, req.payment_id, dst_addr, amt_per_dst, /* at_least_one_destination */ true, er))
    {
      return false;
    }

    LOG_PRINT_L2("on_transfer_split calling create_transactions_2");
    std::unique_ptr<PendingTransaction> ptx(
            m_wallet_impl->createTransactionMultDest(dst_addr,
                                                     /* [deprecated] payment_id */ "",
                                                     amt_per_dst,
                                                     req.ring_size ? req.ring_size - 1 : 0,
                                                     static_cast<PendingTransaction::Priority>(req.priority),
                                                     req.account_index,
                                                     req.subaddr_indices));
    LOG_PRINT_L2("on_transfer_split called create_transactions_2");
    THROW_WALLET_EXCEPTION_ON_API_ERROR();
    if (!ptx)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = "No transaction created";
      return false;
    }

    return fill_response(std::move(ptx), req.get_tx_keys, res.tx_key_list, res.amount_list, res.amounts_by_dest_list, res.fee_list, res.weight_list, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash_list, req.get_tx_hex, res.tx_blob_list, req.get_tx_metadata, res.tx_metadata_list, res.spent_key_images_list, er);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign_transfer(const wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_impl->getDeviceState().key_on_device)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    if(m_wallet_impl->watchOnly())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "command not supported by watch-only wallet";
      return false;
    }

    CHECK_MULTISIG_ENABLED();
    CHECK_IF_BACKGROUND_SYNCING();

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.unsigned_txset, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    auto utx = m_wallet_impl->loadUnsignedTxFromStr(blob);
    THROW_WALLET_EXCEPTION_ON_API_ERROR();
    if(!utx)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
      er.message = "cannot load unsigned_txset";
      return false;
    }

    std::string ciphertext = utx->signAsString();
    if (ciphertext.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_SIGN_UNSIGNED;
      er.message = "Failed to sign unsigned tx" + (utx->errorString().empty() ? "" : (": " + utx->errorString()));
      return false;
    }
    res.signed_txset = epee::string_tools::buff_to_hex_nodelimer(ciphertext);

    auto ptx = m_wallet_impl->parseTxFromStr(ciphertext);
    if (ptx->status() != PendingTransaction::Status::Status_Ok)
    {
        er.code = WALLET_RPC_ERROR_CODE_BAD_SIGNED_TX_DATA;
        er.message = "Failed to parse signed tx" + (ptx->errorString().empty() ? "" : (": " + ptx->errorString()));
        return false;
    }
    std::vector<std::string> tx_ids = ptx->txid();
    for (size_t i = 0; i < ptx->txCount(); ++i)
    {
      res.tx_hash_list.push_back(tx_ids[i]);
      if (req.get_tx_keys)
      {
        res.tx_key_list.push_back(m_wallet_impl->getTxKey(tx_ids[i]));
      }
    }

    if (req.export_raw)
    {
      for (auto &raw_tx_blob_str : ptx->convertTxToRawBlobStr())
      {
        res.tx_raw_list.push_back(raw_tx_blob_str);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_describe_transfer(const wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_impl->getDeviceState().key_on_device)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    if(m_wallet_impl->watchOnly())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "command not supported by watch-only wallet";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();
    if(req.unsigned_txset.empty() && req.multisig_txset.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "no txset provided";
      return false;
    }

    std::unique_ptr<TransactionDescription> tx_desc{};
    if (!req.unsigned_txset.empty()) {
      try {
        cryptonote::blobdata blob;
        if (!epee::string_tools::parse_hexstr_to_binbuff(req.unsigned_txset, blob)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
          er.message = "Failed to parse hex.";
          return false;
        }
        std::shared_ptr<UnsignedTransaction> utx = std::shared_ptr<UnsignedTransaction>(m_wallet_impl->loadUnsignedTxFromStr(blob));
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
        if(!utx)
        {
          er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
          er.message = "cannot load unsigned_txset";
          return false;
        }
        tx_desc = utx->getTransactionDescription();
        if (utx->status() != UnsignedTransaction::Status::Status_Ok)
        {
          er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
          er.message = utx->errorString();
          return false;
        }
      }
      catch (const std::exception &e) {
        er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
        er.message = "failed to parse unsigned transfers: " + std::string(e.what());
        return false;
      }
    } else if (!req.multisig_txset.empty()) {
      try {
        cryptonote::blobdata blob;
        if (!epee::string_tools::parse_hexstr_to_binbuff(req.multisig_txset, blob)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
          er.message = "Failed to parse hex.";
          return false;
        }
        std::shared_ptr<PendingTransaction> ptx = std::shared_ptr<PendingTransaction>(m_wallet_impl->parseMultisigTxFromStr(blob));
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
        if(!ptx)
        {
          er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
          er.message = "cannot load multisig_txset";
          return false;
        }
        tx_desc = ptx->getTransactionDescription();
        if (ptx->status() != PendingTransaction::Status::Status_Ok)
        {
          er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
          er.message = ptx->errorString();
          return false;
        }
      }
      catch (const std::exception &e) {
        er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
        er.message = "failed to parse multisig transfers: " + std::string(e.what());
        return false;
      }
    }

    try
    {
      for (const auto &cd : tx_desc->tx_descriptions)
      {
        res.desc.push_back({
            cd.amount_in,
            cd.amount_out,
            cd.ring_size,
            cd.unlock_time,
            /* sources */ {},
            /* recipients */ {},
            cd.payment_id,
            cd.change_amount,
            cd.change_address,
            cd.fee,
            cd.dummy_outputs,
            cd.extra
        });
        for (const auto &src : cd.sources)
            res.desc.back().sources.push_back({src.amount, src.global_index, src.rct, src.pubkey});
        for (const auto &recipient : cd.recipients)
            res.desc.back().recipients.push_back({recipient.address, recipient.amount});

        // summary
        const auto &tx_sum = tx_desc->tx_summary;
        res.summary = wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::txset_summary{
            tx_sum.amount_in,
            tx_sum.amount_out,
            /* recipients */ {},
            tx_sum.change_amount,
            tx_sum.change_address,
            tx_sum.fee,
        };
        for (const auto &recipient : tx_sum.recipients)
            res.summary.recipients.push_back({recipient.address, recipient.amount});
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
      er.message = "failed to parse unsigned transfers";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_submit_transfer(const wallet_rpc::COMMAND_RPC_SUBMIT_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_impl->getDeviceState().key_on_device)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.tx_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    auto ptx = m_wallet_impl->parseTxFromStr(blob);
    if (ptx->status() != PendingTransaction::Status::Status_Ok)
    {
        er.code = WALLET_RPC_ERROR_CODE_BAD_SIGNED_TX_DATA;
        er.message = "Failed to parse signed tx data" + (ptx->errorString().empty() ? "." : (": " + ptx->errorString()));
        return false;
    }

    std::vector<std::string> txids = ptx->txid();
    if (ptx->commit())
      for (auto &txid: txids)
        res.tx_hash_list.push_back(txid);
    else
    {
      er.code = WALLET_RPC_ERROR_CODE_SIGNED_SUBMISSION;
      er.message = "Failed to submit signed tx" + (ptx->errorString().empty() ? "." : (": " + ptx->errorString()));
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_all(const wallet_rpc::COMMAND_RPC_SWEEP_ALL::request& req, wallet_rpc::COMMAND_RPC_SWEEP_ALL::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    std::vector<std::string> dst_addr{};
    std::vector<std::uint64_t> amt_per_dst{};

    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    else if (req.unlock_time)
    {
      er.code = WALLET_RPC_ERROR_CODE_NONZERO_UNLOCK_TIME;
      er.message = "Transaction cannot have non-zero unlock time";
      return false;
    }
    else if (!fee_priority_utilities::is_valid(req.priority))
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_FEE_PRIORITY;
      er.message = "Invalid priority value. Must be between 0 and 4.";
      return false;
    }

    CHECK_MULTISIG_ENABLED();

    // validate the transfer requested and populate dsts
    std::list<wallet_rpc::transfer_destination> destination;
    destination.push_back(wallet_rpc::transfer_destination());
    destination.back().amount = 0;
    destination.back().address = req.address;
    if (!validate_transfer(destination, req.payment_id, dst_addr, amt_per_dst, /* at_least_one_destination */ true, er))
    {
      return false;
    }

    if (req.outputs < 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = "Amount of outputs should be greater than 0.";
      return  false;
    }

    std::set<uint32_t> subaddr_indices;
    if (req.subaddr_indices_all)
    {
      for (uint32_t i = 0; i < m_wallet_impl->numSubaddresses(req.account_index); ++i)
        subaddr_indices.insert(i);
    }
    else
    {
      subaddr_indices= req.subaddr_indices;
    }

    std::unique_ptr<PendingTransaction> ptx(
            m_wallet_impl->createTransactionMultDest({dst_addr},
                                                     /* [deprecated] payment_id */ "",
                                                     /* amount */ Monero::optional<std::vector<uint64_t>>(),
                                                     req.ring_size ? req.ring_size - 1 : 0,
                                                     static_cast<PendingTransaction::Priority>(req.priority),
                                                     req.account_index,
                                                     subaddr_indices,
                                                     /* subtract_fee_from_outputs */ {},
                                                     /* key_image */ "",
                                                     req.outputs,
                                                     req.below_amount));
    THROW_WALLET_EXCEPTION_ON_API_ERROR();
    if (ptx->txCount() == 0)
    {
      fail_msg_writer() << tr("No outputs found, or daemon is not ready");
      return true;
    }

    return fill_response(std::move(ptx), req.get_tx_keys, res.tx_key_list, res.amount_list, res.amounts_by_dest_list, res.fee_list, res.weight_list, res.multisig_txset, res.unsigned_txset, req.do_not_relay, res.tx_hash_list, req.get_tx_hex, res.tx_blob_list, req.get_tx_metadata, res.tx_metadata_list, res.spent_key_images_list, er);
  }
//------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_single(const wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::request& req, wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    std::vector<std::string> dst_addr{};
    std::vector<std::uint64_t> amt_per_dst{};

    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    else if (req.unlock_time)
    {
      er.code = WALLET_RPC_ERROR_CODE_NONZERO_UNLOCK_TIME;
      er.message = "Transaction cannot have non-zero unlock time";
      return false;
    }
    else if (!fee_priority_utilities::is_valid(req.priority))
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_FEE_PRIORITY;
      er.message = "Invalid priority value. Must be between 0 and 4.";
      return false;
    }

    if (req.outputs < 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = "Amount of outputs should be greater than 0.";
      return  false;
    }

    CHECK_MULTISIG_ENABLED();

    // validate the transfer requested and populate dsts
    std::list<wallet_rpc::transfer_destination> destination;
    destination.push_back(wallet_rpc::transfer_destination());
    destination.back().amount = 0;
    destination.back().address = req.address;
    if (!validate_transfer(destination, req.payment_id, dst_addr, amt_per_dst, /* at_least_one_destination */ true, er))
    {
      return false;
    }

    crypto::key_image ki;
    if (!epee::string_tools::hex_to_pod(req.key_image, ki))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
      er.message = "failed to parse key image";
      return false;
    }
    std::unique_ptr<PendingTransaction> ptx(
            m_wallet_impl->createTransactionMultDest({dst_addr},
                                                     /* [deprecated] payment_id */ "",
                                                     /* amount */ Monero::optional<std::vector<uint64_t>>(),
                                                     req.ring_size ? req.ring_size - 1 : 0,
                                                     static_cast<PendingTransaction::Priority>(req.priority),
                                                     /* account_index */ 0,
                                                     /* subaddr_indices */ {},
                                                     /* subtract_fee_from_outputs */ {},
                                                     req.key_image,
                                                     req.outputs,
                                                     /* below */ 0));
    THROW_WALLET_EXCEPTION_ON_API_ERROR();
    if (ptx->txCount() == 0)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "No outputs found";
      return false;
    }
    if (ptx->txCount() > 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Multiple transactions are created, which is not supposed to happen";
      return false;
    }
    if (ptx->getEnoteDetailsIn()[0].size() > 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "The transaction uses multiple inputs, which is not supposed to happen";
      return false;
    }

    return fill_response(std::move(ptx), req.get_tx_key, res.tx_key, res.amount, res.amounts_by_dest, res.fee, res.weight, res.multisig_txset, res.unsigned_txset, req.do_not_relay, res.tx_hash, req.get_tx_hex, res.tx_blob, req.get_tx_metadata, res.tx_metadata, res.spent_key_images, er);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_relay_tx(const wallet_rpc::COMMAND_RPC_RELAY_TX::request& req, wallet_rpc::COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    std::unique_ptr<PendingTransaction> ptx = m_wallet_impl->deserializePtxFromBlobStr(blob);
    if (!ptx || ptx->status() != PendingTransaction::Status::Status_Ok)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_TX_METADATA;
      er.message = "Failed to parse tx metadata.";
      return false;
    }

    // commit() pops ptx, that's why we pre store txid here
    std::string tmp_txid = ptx->txid()[0];
    ptx->commit();
    if (ptx->status() != PendingTransaction::Status::Status_Ok)
    {
      er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR;
      er.message = "Failed to commit tx.";
      return false;
    }

    res.tx_hash = tmp_txid;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      crypto::hash8 payment_id;
      std::string payment_id_str;
      if (req.payment_id.empty())
      {
        payment_id = crypto::rand<crypto::hash8>();
        payment_id_str = epee::string_tools::pod_to_hex(payment_id);
      }
      else
      {
        if (!tools::wallet2::parse_short_payment_id(req.payment_id, payment_id))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Invalid payment ID";
          return false;
        }
        payment_id_str = req.payment_id;
      }

      if (req.standard_address.empty())
      {
        res.integrated_address = m_wallet_impl->integratedAddress(payment_id_str);
      }
      else
      {
        cryptonote::address_parse_info info;
        if(!get_account_address_from_str(info, nettype(), req.standard_address))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
          er.message = "Invalid address";
          return false;
        }
        if (info.is_subaddress)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
          er.message = "Subaddress shouldn't be used";
          return false;
        }
        if (info.has_payment_id)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
          er.message = "Already integrated address";
          return false;
        }
        res.integrated_address = get_account_integrated_address_as_str(nettype(), info.address, payment_id);
      }
      res.payment_id = payment_id_str;
      return true;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_split_integrated_address(const wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      cryptonote::address_parse_info info;

      if(!get_account_address_from_str(info, nettype(), req.integrated_address))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        er.message = "Invalid address";
        return false;
      }
      if(!info.has_payment_id)
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        er.message = "Address is not an integrated address";
        return false;
      }
      res.standard_address = get_account_address_as_str(nettype(), info.is_subaddress, info.address);
      res.payment_id = epee::string_tools::pod_to_hex(info.payment_id);
      return true;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    try
    {
      m_wallet_impl->store(/* path */ "");
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    crypto::hash payment_id;
    crypto::hash8 payment_id8;
    std::string payment_id_str;

    bool r;
    if (req.payment_id.size() == 2 * sizeof(payment_id))
    {
      r = epee::string_tools::hex_to_pod(req.payment_id, payment_id);
      if (r)
      {
        std::string pid_suffix_substr = req.payment_id.substr(16);
        payment_id_str = std::count(pid_suffix_substr.begin(), pid_suffix_substr.end(), '0') == 48 ? req.payment_id.substr(0, 16) : req.payment_id;
      }
    }
    else if (req.payment_id.size() == 2 * sizeof(payment_id8))
    {
      r = epee::string_tools::hex_to_pod(req.payment_id, payment_id8);
      if (r)
        payment_id_str = req.payment_id;
    }
    else
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = "Payment ID has invalid size: " + req.payment_id;
      return false;
    }

    if(!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = "Payment ID has invalid format: " + req.payment_id;
      return false;
    }

    res.payments.clear();
    const std::vector<TransactionInfo *> &txs = m_wallet_impl->history()->getAll(/* do_refresh */ true);
    for (const auto &tx : txs)
    {
      if (tx->direction() != TransactionInfo::Direction::Direction_In || payment_id_str != tx->paymentId())
          continue;
      cryptonote::subaddress_index subaddress_index = { tx->subaddrAccount(), *tx->subaddrIndex().begin() };
      wallet_rpc::payment_details rpc_payment;
      rpc_payment.payment_id   = tx->paymentId();
      rpc_payment.tx_hash      = tx->hash();
      rpc_payment.amount       = tx->amount();
      rpc_payment.block_height = tx->blockHeight();
      rpc_payment.unlock_time  = tx->unlockTime();
      rpc_payment.locked       = !tx->isUnlocked();
      rpc_payment.subaddr_index = subaddress_index;
      rpc_payment.address      = m_wallet_impl->address(subaddress_index.major, subaddress_index.minor);
      res.payments.push_back(rpc_payment);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.payments.clear();
    if (!m_wallet_impl) return not_open(er);

    const std::vector<TransactionInfo *> &txs = m_wallet_impl->history()->getAll(/* do_refresh */ true);
    /* If the payment ID list is empty, we get payments to any payment ID (or lack thereof) */
    if (req.payment_ids.empty())
    {
      for (const auto &tx : txs)
      {
        if (tx->direction() != TransactionInfo::Direction::Direction_In || tx->blockHeight() < req.min_block_height)
          continue;
        cryptonote::subaddress_index subaddress_index = { tx->subaddrAccount(), *tx->subaddrIndex().begin() };
        wallet_rpc::payment_details rpc_payment;
        rpc_payment.payment_id   = tx->paymentId();
        rpc_payment.tx_hash      = tx->hash();
        rpc_payment.amount       = tx->amount();
        rpc_payment.block_height = tx->blockHeight();
        rpc_payment.unlock_time  = tx->unlockTime();
        rpc_payment.subaddr_index = subaddress_index;
        rpc_payment.address      = m_wallet_impl->address(subaddress_index.major, subaddress_index.minor);
        rpc_payment.locked       = !tx->isUnlocked();
        res.payments.push_back(std::move(rpc_payment));
      }

      return true;
    }

    std::string payment_id_str = "";
    for (auto & pid_str : req.payment_ids)
    {
      crypto::hash payment_id;
      crypto::hash8 payment_id8;

      bool r;
      if (pid_str.size() == 2 * sizeof(payment_id))
      {
        r = epee::string_tools::hex_to_pod(pid_str, payment_id);
        if (r)
        {
          std::string pid_suffix_substr = pid_str.substr(16);
          payment_id_str = std::count(pid_suffix_substr.begin(), pid_suffix_substr.end(), '0') == 48 ? pid_str.substr(0, 16) : pid_str;
        }
      }
      else if (pid_str.size() == 2 * sizeof(payment_id8))
      {
        r = epee::string_tools::hex_to_pod(pid_str, payment_id8);
        if (r)
          payment_id_str = pid_str;
      }
      else
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid size: " + pid_str;
        return false;
      }

      if(!r)
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid format: " + pid_str;
        return false;
      }

      for (const auto &tx : txs)
      {
        if (tx->direction() != TransactionInfo::Direction::Direction_In
            || tx->blockHeight() < req.min_block_height
            || tx->paymentId() != payment_id_str)
          continue;
        cryptonote::subaddress_index subaddress_index = { tx->subaddrAccount(), *tx->subaddrIndex().begin() };
        wallet_rpc::payment_details rpc_payment;
        rpc_payment.payment_id   = payment_id_str;
        rpc_payment.tx_hash      = tx->hash();
        rpc_payment.amount       = tx->amount();
        rpc_payment.block_height = tx->blockHeight();
        rpc_payment.unlock_time  = tx->unlockTime();
        rpc_payment.subaddr_index = subaddress_index;
        rpc_payment.address      = m_wallet_impl->address(subaddress_index.major, subaddress_index.minor);
        rpc_payment.locked       = !tx->isUnlocked();
        res.payments.push_back(std::move(rpc_payment));
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    if(req.transfer_type.compare("all") != 0 && req.transfer_type.compare("available") != 0 && req.transfer_type.compare("unavailable") != 0)
    {
      er.code = WALLET_RPC_ERROR_CODE_TRANSFER_TYPE;
      er.message = "Transfer type must be one of: all, available, or unavailable";
      return false;
    }

    bool filter = false;
    bool available = false;
    if (req.transfer_type.compare("available") == 0)
    {
      filter = true;
      available = true;
    }
    else if (req.transfer_type.compare("unavailable") == 0)
    {
      filter = true;
      available = false;
    }

    const std::vector<std::unique_ptr<EnoteDetails>> &eds = m_wallet_impl->getEnoteDetails();

    for (const auto& ed : eds)
    {
      if (!filter || available != ed->isSpent())
      {
        if (req.account_index != ed->subaddressIndexMajor() || (!req.subaddr_indices.empty() && req.subaddr_indices.count(ed->subaddressIndexMinor()) == 0))
          continue;

        wallet_rpc::transfer_details rpc_transfers;
        rpc_transfers.amount       = ed->amount();
        rpc_transfers.spent        = ed->isSpent();
        rpc_transfers.global_index = ed->globalEnoteIndex();
        rpc_transfers.tx_hash      = ed->txId();
        rpc_transfers.subaddr_index = {ed->subaddressIndexMajor(), ed->subaddressIndexMinor()};
        rpc_transfers.key_image    = ed->isKeyImageKnown() ? ed->keyImage() : "";
        rpc_transfers.pubkey       = ed->onetimeAddress();
        rpc_transfers.block_height = ed->blockHeight();
        rpc_transfers.frozen       = ed->isFrozen();
        rpc_transfers.unlocked     = ed->isUnlocked();
        res.transfers.push_back(rpc_transfers);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_query_key(const wallet_rpc::COMMAND_RPC_QUERY_KEY::request& req, wallet_rpc::COMMAND_RPC_QUERY_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
      if (m_restricted)
      {
        er.code = WALLET_RPC_ERROR_CODE_DENIED;
        er.message = "Command unavailable in restricted mode.";
        return false;
      }
      if (!m_wallet_impl) return not_open(er);

      if (req.key_type.compare("mnemonic") == 0)
      {
        epee::wipeable_string seed;
        const MultisigState &ms_status{m_wallet_impl->multisig()};

        if (ms_status.isMultisig)
        {
          if (!ms_status.isReady)
          {
            er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
            er.message = "This wallet is multisig, but not yet finalized";
            return false;
          }
          seed = m_wallet_impl->getMultisigSeed(/* seed_offset */ "");
          if (seed.empty())
          {
            er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
            er.message = "Failed to get multisig seed.";
            return false;
          }
        }
        else
        {
          if (m_wallet_impl->watchOnly())
          {
            er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
            er.message = "The wallet is watch-only. Cannot retrieve seed.";
            return false;
          }
          CHECK_IF_BACKGROUND_SYNCING();
          if (!m_wallet_impl->isDeterministic())
          {
            er.code = WALLET_RPC_ERROR_CODE_NON_DETERMINISTIC;
            er.message = "The wallet is non-deterministic. Cannot display seed.";
            return false;
          }
          seed = m_wallet_impl->seed(/* seed_offset */ "");
          if (seed.empty())
          {
            er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
            er.message = "Failed to get seed.";
            return false;
          }
        }
        res.key = std::string(seed.data(), seed.size()); // send to the network, then wipe RAM :D
      }
      else if(req.key_type.compare("view_key") == 0)
      {
          res.key = m_wallet_impl->secretViewKey();
      }
      else if(req.key_type.compare("spend_key") == 0)
      {
          if (m_wallet_impl->watchOnly())
          {
            er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
            er.message = "The wallet is watch-only. Cannot retrieve spend key.";
            return false;
          }
          CHECK_IF_BACKGROUND_SYNCING();
          res.key = m_wallet_impl->secretSpendKey();
      }
      else
      {
          er.message = "key_type " + req.key_type + " not found";
          return false;
      }

      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_rescan_blockchain(const wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::request& req, wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    if (req.hard && req.keep_key_images)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Cannot preserve key images on hard rescan";
      return false;
    }

    try
    {
      m_wallet_impl->rescanBlockchain(req.hard, req.keep_key_images);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_setup_background_sync(const wallet_rpc::COMMAND_RPC_SETUP_BACKGROUND_SYNC::request& req, wallet_rpc::COMMAND_RPC_SETUP_BACKGROUND_SYNC::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    try
    {
      PRE_VALIDATE_BACKGROUND_SYNC();
      Wallet::BackgroundSyncType background_sync_type;
      if (req.background_sync_type == "off") background_sync_type = Wallet::BackgroundSyncType::BackgroundSync_Off;
      else if (req.background_sync_type == "reuse-wallet-password") background_sync_type = Wallet::BackgroundSyncType::BackgroundSync_ReusePassword;
      else if (req.background_sync_type == "custom-background-password") background_sync_type = Wallet::BackgroundSyncType::BackgroundSync_CustomPassword;
      else throw std::logic_error("Unknown background sync type");

      optional<std::string> background_cache_password = optional<std::string>();
      if (background_sync_type == Wallet::BackgroundSyncType::BackgroundSync_CustomPassword)
          background_cache_password = optional<std::string>(req.background_cache_password);
      m_wallet_impl->setupBackgroundSync(background_sync_type, req.wallet_password, background_cache_password);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (...)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_start_background_sync(const wallet_rpc::COMMAND_RPC_START_BACKGROUND_SYNC::request& req, wallet_rpc::COMMAND_RPC_START_BACKGROUND_SYNC::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    try
    {
      PRE_VALIDATE_BACKGROUND_SYNC();
      m_wallet_impl->startBackgroundSync();
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (...)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_stop_background_sync(const wallet_rpc::COMMAND_RPC_STOP_BACKGROUND_SYNC::request& req, wallet_rpc::COMMAND_RPC_STOP_BACKGROUND_SYNC::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    try
    {
      PRE_VALIDATE_BACKGROUND_SYNC();
      crypto::secret_key spend_secret_key = crypto::null_skey;

      // Load the spend key from seed if seed is provided
      if (!req.seed.empty())
      {
        crypto::secret_key recovery_key;
        std::string language;

        if (!crypto::ElectrumWords::words_to_bytes(req.seed, recovery_key, language))
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Electrum-style word list failed verification";
          return false;
        }

        if (!req.seed_offset.empty())
          recovery_key = cryptonote::decrypt_key(recovery_key, req.seed_offset);

        // generate spend key
        cryptonote::account_base account;
        account.generate(recovery_key, true, false);
        spend_secret_key = account.get_keys().m_spend_secret_key;
      }
      std::string spend_secret_key_str = epee::string_tools::pod_to_hex(unwrap(unwrap(spend_secret_key)));

      epee::misc_utils::auto_scope_leave_caller ssk_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
        memwipe(&spend_secret_key_str[0], spend_secret_key_str.size());
      });
      std::string_view ssk{spend_secret_key_str};
      m_wallet_impl->stopBackgroundSync(req.wallet_password, &ssk);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (...)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign(const wallet_rpc::COMMAND_RPC_SIGN::request& req, wallet_rpc::COMMAND_RPC_SIGN::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    if (req.signature_type != "spend" && req.signature_type != "" && req.signature_type != "view")
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_SIGNATURE_TYPE;
      er.message = "Invalid signature type requested";
      return false;
    }
    res.signature = m_wallet_impl->signMessage(req.data, m_wallet_impl->address(req.account_index, req.address_index), req.signature_type == "view");
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_verify(const wallet_rpc::COMMAND_RPC_VERIFY::request& req, wallet_rpc::COMMAND_RPC_VERIFY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    cryptonote::address_parse_info info;
    er.message = "";
    if(!get_account_address_from_str_or_url(info, nettype(), req.address,
      [&er](const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)->std::string {
        if (!dnssec_valid)
        {
          er.message = std::string("Invalid DNSSEC for ") + url;
          return {};
        }
        if (addresses.empty())
        {
          er.message = std::string("No Monero address found at ") + url;
          return {};
        }
        return addresses[0];
      }))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      return false;
    }

    res.good = m_wallet_impl->verifySignedMessage(req.data, req.address, req.signature, &res.old, &res.signature_type, &res.version);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_stop_wallet(const wallet_rpc::COMMAND_RPC_STOP_WALLET::request& req, wallet_rpc::COMMAND_RPC_STOP_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    try
    {
      m_wallet_impl->store(/* path */ "");
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      m_stop.store(true, std::memory_order_relaxed);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_tx_notes(const wallet_rpc::COMMAND_RPC_SET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_SET_TX_NOTES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    if (req.txids.size() != req.notes.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Different amount of txids and notes";
      return false;
    }

    std::list<crypto::hash> txids;
    std::list<std::string>::const_iterator i = req.txids.begin();
    while (i != req.txids.end())
    {
      cryptonote::blobdata txid_blob;
      if(!epee::string_tools::parse_hexstr_to_binbuff(*i++, txid_blob) || txid_blob.size() != sizeof(crypto::hash))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
        er.message = "TX ID has invalid format";
        return false;
      }

      crypto::hash txid;
      memcpy(&txid, txid_blob.data(), sizeof(txid));
      txids.push_back(txid);
    }

    std::list<std::string>::const_iterator il = req.txids.begin();
    std::list<std::string>::const_iterator in = req.notes.begin();
    while (il != req.txids.end())
    {
      m_wallet_impl->setUserNote(*il++, *in++);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_tx_notes(const wallet_rpc::COMMAND_RPC_GET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_GET_TX_NOTES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.notes.clear();
    if (!m_wallet_impl) return not_open(er);

    std::list<crypto::hash> txids;
    std::list<std::string>::const_iterator i = req.txids.begin();
    while (i != req.txids.end())
    {
      cryptonote::blobdata txid_blob;
      if(!epee::string_tools::parse_hexstr_to_binbuff(*i++, txid_blob) || txid_blob.size() != sizeof(crypto::hash))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
        er.message = "TX ID has invalid format";
        return false;
      }

      crypto::hash txid;
      memcpy(&txid, txid_blob.data(), sizeof(txid));
      txids.push_back(txid);
    }

    std::list<std::string>::const_iterator il = req.txids.begin();
    while (il != req.txids.end())
    {
      res.notes.push_back(m_wallet_impl->getUserNote(*il++));
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_attribute(const wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    m_wallet_impl->setCacheAttribute(req.key, req.value);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_attribute(const wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    res.value = m_wallet_impl->getCacheAttribute(req.key);
    if (res.value.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_ATTRIBUTE_NOT_FOUND;
      er.message = "Attribute not found.";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_tx_key(const wallet_rpc::COMMAND_RPC_GET_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_GET_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    try
    {
      res.tx_key = m_wallet_impl->getTxKey(req.txid);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_tx_key(const wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    epee::wipeable_string tx_key_str = req.tx_key;
    if (tx_key_str.size() < 64 || tx_key_str.size() % 64)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY;
      er.message = "Tx key has invalid format";
      return false;
    }
    const char *data = tx_key_str.data();
    crypto::secret_key tx_key;
    if (!epee::wipeable_string(data, 64).hex_to_pod(unwrap(unwrap(tx_key))))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY;
      er.message = "Tx key has invalid format";
      return false;
    }
    size_t offset = 64;
    std::vector<crypto::secret_key> additional_tx_keys;
    while (offset < tx_key_str.size())
    {
      additional_tx_keys.resize(additional_tx_keys.size() + 1);
      if (!epee::wipeable_string(data + offset, 64).hex_to_pod(unwrap(unwrap(additional_tx_keys.back()))))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY;
        er.message = "Tx key has invalid format";
        return false;
      }
      offset += 64;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      m_wallet_impl->checkTxKey(req.txid, req.tx_key, req.address, res.received, res.in_pool, res.confirmations);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_tx_proof(const wallet_rpc::COMMAND_RPC_GET_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_TX_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      res.signature = m_wallet_impl->getTxProof(req.txid, req.address, req.message);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_tx_proof(const wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      m_wallet_impl->checkTxProof(req.txid, req.address, req.message, req.signature, res.good, res.received, res.in_pool, res.confirmations);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_spend_proof(const wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    try
    {
      res.signature = m_wallet_impl->getSpendProof(req.txid, req.message);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_spend_proof(const wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    try
    {
      m_wallet_impl->checkSpendProof(req.txid, req.message, req.signature, res.good);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_reserve_proof(const wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();

    if (!req.all)
    {
      if (req.account_index >= m_wallet_impl->numSubaddressAccounts())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Account index is out of bound";
        return false;
      }
    }

    try
    {
      res.signature = m_wallet_impl->getReserveProof(req.all, req.account_index, req.amount, req.message);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_reserve_proof(const wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    cryptonote::address_parse_info info;
    if (!get_account_address_from_str(info, nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }
    if (info.is_subaddress)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Address must not be a subaddress";
      return false;
    }

    try
    {
      m_wallet_impl->checkReserveProof(req.address, req.message, req.signature, res.good, res.total, res.spent);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_transfers(const wallet_rpc::COMMAND_RPC_GET_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFERS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    uint64_t min_height = 0, max_height = CRYPTONOTE_MAX_BLOCK_NUMBER;
    if (req.filter_by_height)
    {
      min_height = req.min_height;
      max_height = req.max_height <= max_height ? req.max_height : max_height;
    }

    boost::optional<uint32_t> account_index = req.account_index;
    std::set<uint32_t> subaddr_indices = req.subaddr_indices;
    if (req.all_accounts)
    {
      account_index = boost::none;
      subaddr_indices.clear();
    }

    const std::vector<TransactionInfo *> &txs = m_wallet_impl->history()->getAll(/* do_refresh */ true);
    for (const auto &tx : txs)
    {
      if (req.in
       && tx->direction() == TransactionInfo::Direction_In
       && !tx->isPending()
       && tx->blockHeight() > min_height
       && tx->blockHeight() <= max_height
       && (!account_index || tx->subaddrAccount() == account_index)
       && (subaddr_indices.empty() || *tx->subaddrIndex().begin() == *subaddr_indices.begin())) {
        res.in.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.in.back(), *tx);
        continue;
      }

      if (req.out
       && tx->direction() == TransactionInfo::Direction_Out
       && !tx->isPending()
       && tx->blockHeight() > min_height
       && tx->blockHeight() <= max_height
       && (!account_index || tx->subaddrAccount() == account_index)
       && (subaddr_indices.empty() || std::count_if(tx->subaddrIndex().begin(), tx->subaddrIndex().end(), [&subaddr_indices](uint32_t index) { return subaddr_indices.count(index) == 1; }) != 0)) {
        res.out.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.out.back(), *tx);
        continue;
      }

      if (((req.pending && tx->isPending()) || (req.failed && tx->isFailed()))
       && tx->direction() == TransactionInfo::Direction_Out
       && (!account_index || tx->subaddrAccount() == account_index)
       && (subaddr_indices.empty() || std::count_if(tx->subaddrIndex().begin(), tx->subaddrIndex().end(), [&subaddr_indices](uint32_t index) { return subaddr_indices.count(index) == 1; }) != 0)) {
        std::list<wallet_rpc::transfer_entry> &entries = tx->isFailed() ? res.failed : res.pending;
        entries.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(entries.back(), *tx);
        continue;
      }

      if (req.pool
       && tx->direction() == TransactionInfo::Direction_In
       && tx->isPending()
       && (!account_index || tx->subaddrAccount() == account_index)
       && (subaddr_indices.empty() || std::count_if(tx->subaddrIndex().begin(), tx->subaddrIndex().end(), [&subaddr_indices](uint32_t index) { return subaddr_indices.count(index) == 1; }) != 0)) {
        res.pool.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.pool.back(), *tx);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_transfer_by_txid(const wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);

    crypto::hash txid;
    cryptonote::blobdata txid_blob;
    if(!epee::string_tools::parse_hexstr_to_binbuff(req.txid, txid_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "Transaction ID has invalid format";
      return false;
    }

    if(sizeof(txid) != txid_blob.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "Transaction ID has invalid size: " + req.txid;
      return false;
    }

    if (req.account_index >= m_wallet_impl->numSubaddressAccounts())
    {
      er.code = WALLET_RPC_ERROR_CODE_ACCOUNT_INDEX_OUT_OF_BOUNDS;
      er.message = "Account index is out of bound";
      return false;
    }

    const std::vector<TransactionInfo *> &txs = m_wallet_impl->history()->getAll(/* do_refresh */ true);
    for (const auto &tx : txs)
    {
      if (req.txid != tx->hash() || req.account_index != tx->subaddrAccount())
          continue;
      res.transfers.resize(res.transfers.size() + 1);
      fill_transfer_entry(res.transfers.back(), *tx);
    }

    if (!res.transfers.empty())
    {
      res.transfer = res.transfers.front(); // backward compat
      return true;
    }

    er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
    er.message = "Transaction not found.";
    return false;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_export_outputs(const wallet_rpc::COMMAND_RPC_EXPORT_OUTPUTS::request& req, wallet_rpc::COMMAND_RPC_EXPORT_OUTPUTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (m_wallet_impl->getDeviceState().key_on_device)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();

    try
    {
      res.outputs_data_hex = epee::string_tools::buff_to_hex_nodelimer(m_wallet_impl->exportEnotesToStr(req.all, req.start, req.count));
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_import_outputs(const wallet_rpc::COMMAND_RPC_IMPORT_OUTPUTS::request& req, wallet_rpc::COMMAND_RPC_IMPORT_OUTPUTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (m_wallet_impl->getDeviceState().key_on_device)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.outputs_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    try
    {
      res.num_imported = m_wallet_impl->importEnotesFromStr(blob);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_export_key_images(const wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    try
    {
      std::vector<std::pair<std::string, std::string>> key_images_and_signatures;
      std::uint64_t offset;
      m_wallet_impl->exportKeyImages(req.all, offset, key_images_and_signatures);
      res.offset = offset;
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      res.signed_key_images.resize(key_images_and_signatures.size());
      for (size_t n = 0; n < key_images_and_signatures.size(); ++n)
      {
         res.signed_key_images[n].key_image = key_images_and_signatures[n].first;
         res.signed_key_images[n].signature = key_images_and_signatures[n].second;
      }
    }

    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_import_key_images(const wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (!m_wallet_impl->trustedDaemon())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "This command requires a trusted daemon.";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();
    try
    {
      std::vector<std::pair<std::string, std::string>> ski;
      crypto::key_image tmp_ki;
      crypto::signature tmp_sig;
      ski.resize(req.signed_key_images.size());
      for (size_t n = 0; n < ski.size(); ++n)
      {
        if (!epee::string_tools::hex_to_pod(req.signed_key_images[n].key_image, tmp_ki))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
          er.message = "failed to parse key image";
          return false;
        }
        ski[n].first = req.signed_key_images[n].key_image;

        if (!epee::string_tools::hex_to_pod(req.signed_key_images[n].signature, tmp_sig))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_SIGNATURE;
          er.message = "failed to parse signature";
          return false;
        }
        ski[n].second = req.signed_key_images[n].signature;
      }

      res.height = m_wallet_impl->importKeyImages(ski, req.offset, res.spent, res.unspent);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }

    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_uri(const wallet_rpc::COMMAND_RPC_MAKE_URI::request& req, wallet_rpc::COMMAND_RPC_MAKE_URI::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    std::string error;
    std::string uri = m_wallet_impl->make_uri(req.address, req.payment_id, req.amount, req.tx_description, req.recipient_name, error);
    if (uri.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_URI;
      er.message = std::string("Cannot make URI from supplied parameters: ") + error;
      return false;
    }

    res.uri = uri;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_parse_uri(const wallet_rpc::COMMAND_RPC_PARSE_URI::request& req, wallet_rpc::COMMAND_RPC_PARSE_URI::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    std::string error;
    if (!m_wallet_impl->parse_uri(req.uri, res.uri.address, res.uri.payment_id, res.uri.amount, res.uri.tx_description, res.uri.recipient_name, res.unknown_parameters, error))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_URI;
      er.message = "Error parsing URI: " + error;
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_address_book(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_BACKGROUND_SYNCING();
    const auto ab = m_wallet_impl->addressBook()->getAll();
    if (req.entries.empty())
    {
      uint64_t idx = 0;
      for (const auto &entry: ab)
      {
        res.entries.push_back(wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::entry{idx++, entry->getAddress(), entry->getDescription()});
      }
    }
    else
    {
      for (uint64_t idx: req.entries)
      {
        if (idx >= ab.size())
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_INDEX;
          er.message = "Index out of range: " + std::to_string(idx);
          return false;
        }
        const auto &entry = ab[idx];
        std::string address;
        res.entries.push_back(wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::entry{idx, entry->getAddress(), entry->getDescription()});
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_add_address_book(const wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    cryptonote::address_parse_info info;
    er.message = "";
    std::string address{};
    if(!get_account_address_from_str_or_url(info, nettype(), req.address,
      [&er, &address](const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)->std::string {
        if (!dnssec_valid)
        {
          er.message = std::string("Invalid DNSSEC for ") + url;
          return {};
        }
        if (addresses.empty())
        {
          er.message = std::string("No Monero address found at ") + url;
          return {};
        }
        address = addresses[0];
        return addresses[0];
      }))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      if (er.message.empty())
        er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + req.address;
      return false;
    }
    AddressBook *address_book = m_wallet_impl->addressBook();
    address = address.empty() ? req.address : address;
    if (!address_book->addRow(address, /* [deprecated] payment_id */ "", req.description))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to add address book entry";
      return false;
    }
    res.index = address_book->getAll().size() - 1;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_edit_address_book(const wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    const auto ab = m_wallet_impl->addressBook();
    const auto ab_entries = ab->getAll();
    if (req.index >= ab_entries.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_INDEX;
      er.message = "Index out of range: " + std::to_string(req.index);
      return false;
    }

    bool success = true;
    cryptonote::address_parse_info info;
    std::string address{};
    if (req.set_address)
    {
      er.message = "";
      if(!get_account_address_from_str_or_url(info, nettype(), req.address,
        [&er, &address](const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)->std::string {
          if (!dnssec_valid)
          {
            er.message = std::string("Invalid DNSSEC for ") + url;
            return {};
          }
          if (addresses.empty())
          {
            er.message = std::string("No Monero address found at ") + url;
            return {};
          }
          address = addresses[0];
          return addresses[0];
        }))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
        if (er.message.empty())
          er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + req.address;
        return false;
      }
      address = address.empty() ? req.address : address;
      success = ab->setAddress(req.index, address);
    }

    if (success && req.set_description)
      success = ab->setDescription(req.index, req.description);

    if (!success)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to edit address book entry";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_delete_address_book(const wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

    if (req.index >= m_wallet_impl->addressBook()->getAll().size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_INDEX;
      er.message = "Index out of range: " + std::to_string(req.index);
      return false;
    }
    if (!m_wallet_impl->addressBook()->deleteRow(req.index))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to delete address book entry";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_refresh(const wallet_rpc::COMMAND_RPC_REFRESH::request& req, wallet_rpc::COMMAND_RPC_REFRESH::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    try
    {
      m_wallet_impl->refresh(req.start_height,
                             /* check_pool */ true,
                             /* try_incremental */ true,
                             /* max_blocks */ std::numeric_limits<std::uint64_t>::max(),
                             /* skip_refresh_if_daemon_not_synced */ false,
                             &res.blocks_fetched,
                             &res.received_money);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      return true;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_auto_refresh(const wallet_rpc::COMMAND_RPC_AUTO_REFRESH::request& req, wallet_rpc::COMMAND_RPC_AUTO_REFRESH::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    try
    {
      const auto new_period = req.enable ? req.period ? req.period : DEFAULT_AUTO_REFRESH_PERIOD : 0;
      m_auto_refresh_period.store(new_period, std::memory_order_relaxed);
      MINFO("Auto refresh now " << (new_period ? std::to_string(new_period) + " seconds" : std::string("disabled")));
      return true;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_scan_tx(const wallet_rpc::COMMAND_RPC_SCAN_TX::request& req, wallet_rpc::COMMAND_RPC_SCAN_TX::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
      CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();

      std::vector<std::string> txids;
      std::list<std::string>::const_iterator i = req.txids.begin();
      while (i != req.txids.end())
      {
          cryptonote::blobdata txid_blob;
          if(!epee::string_tools::parse_hexstr_to_binbuff(*i, txid_blob) || txid_blob.size() != sizeof(crypto::hash))
          {
              er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
              er.message = "TX ID has invalid format";
              return false;
          }

          txids.push_back(*i++);
      }

      try {
          bool wont_reprocess_recent_txs_via_untrusted_daemon;
          m_wallet_impl->scanTransactions(txids, &wont_reprocess_recent_txs_via_untrusted_daemon);
          if (wont_reprocess_recent_txs_via_untrusted_daemon)
              throw wont_reprocess_recent_txs_via_untrusted_daemon;
          THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }  catch (const tools::error::wont_reprocess_recent_txs_via_untrusted_daemon &e) {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = e.what() + std::string(". Either connect to a trusted daemon or rescan the chain.");
          return false;
      } catch (const std::exception &e) {
          handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
          return false;
      }
      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_rescan_spent(const wallet_rpc::COMMAND_RPC_RESCAN_SPENT::request& req, wallet_rpc::COMMAND_RPC_RESCAN_SPENT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    if (!m_wallet_impl->trustedDaemon())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "This command requires a trusted daemon.";
      return false;
    }
    try
    {
      m_wallet_impl->rescanSpent();
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      return true;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_start_mining(const wallet_rpc::COMMAND_RPC_START_MINING::request& req, wallet_rpc::COMMAND_RPC_START_MINING::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (!m_wallet_impl->trustedDaemon())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "This command requires a trusted daemon.";
      return false;
    }

    size_t max_mining_threads_count = (std::max)(tools::get_max_concurrency(), static_cast<unsigned>(2));
    if (req.threads_count < 1 || max_mining_threads_count < req.threads_count)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "The specified number of threads is inappropriate.";
      return false;
    }

    if (!m_wallet_manager->startMining(m_wallet_impl->address(),
                                       req.threads_count,
                                       req.do_background_mining,
                                       req.ignore_battery))
    {
      std::string err = m_wallet_manager->errorString();
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Couldn't start mining: " + (!err.empty() ? err : "due to unknown error.");
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_stop_mining(const wallet_rpc::COMMAND_RPC_STOP_MINING::request& req, wallet_rpc::COMMAND_RPC_STOP_MINING::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    if (!m_wallet_manager->stopMining())
    {
      std::string err = m_wallet_manager->errorString();
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Couldn't stop mining: " + (!err.empty() ? err : "due to unknown error.");
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_languages(const wallet_rpc::COMMAND_RPC_GET_LANGUAGES::request& req, wallet_rpc::COMMAND_RPC_GET_LANGUAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    crypto::ElectrumWords::get_language_list(res.languages, true);
    crypto::ElectrumWords::get_language_list(res.languages_local, false);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_create_wallet(const wallet_rpc::COMMAND_RPC_CREATE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CREATE_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    const char *ptr = strchr(req.filename.c_str(), '/');
#ifdef _WIN32
    if (!ptr)
      ptr = strchr(req.filename.c_str(), '\\');
    if (!ptr)
      ptr = strchr(req.filename.c_str(), ':');
#endif
    if (ptr)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Invalid filename";
      return false;
    }
    std::string wallet_file = req.filename.empty() ? "" : (m_wallet_dir + "/" + req.filename);
    {
      if (!crypto::ElectrumWords::is_valid_language(req.language))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Unknown language: " + req.language;
        return false;
      }
    }

    std::unique_ptr<Wallet> wal;
    try {
      NetworkType nettype;
      if (!get_nettype(m_vm, nettype))
        throw std::runtime_error("unexpected nettype");
      wal.reset(
        m_wallet_manager->createWallet(
          wallet_file,
          req.password,
          req.language,
          nettype,
          command_line::get_arg(m_vm, arg_kdf_rounds),
          /* create_address_file */ false,
          /* non_deterministic */ false,
          /* unattended */ true,
          command_line::get_arg(m_vm, arg_extra_entropy))
      );
      if (!wal)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to create wallet";
        return false;
      }
      int error_code, extended_error_code;
      std::string error_message;
      wal->statusWithErrorString(error_code, error_message, &extended_error_code);
      if (error_code != Wallet::Status::Status_Ok)
          tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    m_wallet_impl = std::move(wal);
    if (m_wallet_impl)
    {
      try
      {
        m_wallet_impl->store(/* path */ "");
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_open_wallet(const wallet_rpc::COMMAND_RPC_OPEN_WALLET::request& req, wallet_rpc::COMMAND_RPC_OPEN_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    const char *ptr = strchr(req.filename.c_str(), '/');
#ifdef _WIN32
    if (!ptr)
      ptr = strchr(req.filename.c_str(), '\\');
    if (!ptr)
      ptr = strchr(req.filename.c_str(), ':');
#endif
    if (ptr)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Invalid filename";
      return false;
    }
    if (m_wallet_impl && req.autosave_current)
    {
      try
      {
        m_wallet_impl->store(/* path */ "");
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    std::string wallet_file = m_wallet_dir + "/" + req.filename;

    std::unique_ptr<Wallet> wal;
    try {
      NetworkType nettype;
      if (!get_nettype(m_vm, nettype))
        throw std::runtime_error("unexpected nettype");
      wal.reset(
        m_wallet_manager->openWallet(
          wallet_file,
          req.password,
          nettype,
          command_line::get_arg(m_vm, arg_kdf_rounds))
      );
      if (!wal)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to open wallet : " + (!er.message.empty() ? er.message : "Unknown.");
        return false;
      }
      int error_code, extended_error_code;
      std::string error_message;
      wal->statusWithErrorString(error_code, error_message, &extended_error_code);
      if (error_code != Wallet::Status::Status_Ok)
          tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    int error_code;
    std::string error_msg;
    wal->statusWithErrorString(error_code, error_msg);
    if (error_code)
    {
      MERROR(tr("failed to load wallet: ") << error_msg);
      return {};
    }

    if (!init_wallet(m_vm, wal, m_wallet_manager))
    {
      wal->statusWithErrorString(error_code, error_msg);
      if (error_code)
        MERROR(tr("failed to initialize wallet: ") << error_msg);
      return {};
    }

    m_wallet_impl = std::move(wal);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_close_wallet(const wallet_rpc::COMMAND_RPC_CLOSE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CLOSE_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    if (req.autosave_current)
    {
      try
      {
        m_wallet_impl->store(/* path */ "");
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    m_wallet_manager->closeWallet(m_wallet_impl.release(), /* store */ false);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_change_wallet_password(const wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::request& req, wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    CHECK_IF_RESTRICTED_BACKGROUND_SYNCING();
    if (m_wallet_impl->verifyPassword(req.old_password))
    {
      try
      {
        m_wallet_impl->setPassword(req.old_password, req.new_password);
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
        LOG_PRINT_L0("Wallet password changed.");
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    else
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_PASSWORD;
      er.message = "Invalid original password.";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::handle_rpc_exception(const std::exception_ptr& e, epee::json_rpc::error& er, int default_error_code) {
    try
    {
      std::rethrow_exception(e);
    }
    catch (const tools::error::no_connection_to_daemon& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
      er.message = e.what();
    }
    catch (const tools::error::daemon_busy& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY;
      er.message = e.what();
    }
    catch (const tools::error::zero_amount& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_ZERO_AMOUNT;
      er.message = e.what();
    }
    catch (const tools::error::zero_destination& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_ZERO_DESTINATION;
      er.message = e.what();
    }
    catch (const tools::error::not_enough_money& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_ENOUGH_MONEY;
      er.message = e.what();
    }
    catch (const tools::error::not_enough_unlocked_money& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_ENOUGH_UNLOCKED_MONEY;
      er.message = e.what();
    }
    catch (const tools::error::tx_not_possible& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = (boost::format(tr("Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)")) %
        cryptonote::print_money(e.available()) %
        cryptonote::print_money(e.tx_amount() + e.fee())  %
        cryptonote::print_money(e.tx_amount()) %
        cryptonote::print_money(e.fee())).str();
    }
    catch (const tools::error::not_enough_outs_to_mix& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_ENOUGH_OUTS_TO_MIX;
      er.message = e.what() + std::string(" Please use sweep_dust.");
    }
    catch (const error::file_exists& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_WALLET_ALREADY_EXISTS;
      er.message = "Cannot create wallet. Already exists.";
    }
    catch (const error::invalid_password& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_PASSWORD;
      er.message = "Invalid password.";
    }
    catch (const error::account_index_outofbound& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_ACCOUNT_INDEX_OUT_OF_BOUNDS;
      er.message = e.what();
    }
    catch (const error::address_index_outofbound& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_ADDRESS_INDEX_OUT_OF_BOUNDS;
      er.message = e.what();
    }
    catch (const error::signature_check_failed& e)
    {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_SIGNATURE;
        er.message = e.what();
    }
    catch (const std::exception& e)
    {
      er.code = default_error_code;
      er.message = e.what();
    }
    catch (...)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
    }
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_generate_from_keys(const wallet_rpc::COMMAND_RPC_GENERATE_FROM_KEYS::request &req, wallet_rpc::COMMAND_RPC_GENERATE_FROM_KEYS::response &res, epee::json_rpc::error &er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    // early check for mandatory fields
    if (req.viewkey.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "field 'viewkey' is mandatory. Please provide a view key you want to restore from.";
      return false;
    }
    if (req.address.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "field 'address' is mandatory. Please provide a public address.";
      return false;
    }

    const char *ptr = strchr(req.filename.c_str(), '/');
  #ifdef _WIN32
    if (!ptr)
      ptr = strchr(req.filename.c_str(), '\\');
    if (!ptr)
      ptr = strchr(req.filename.c_str(), ':');
  #endif
    if (ptr)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Invalid filename";
      return false;
    }
    std::string wallet_file = req.filename.empty() ? "" : (m_wallet_dir + "/" + req.filename);
    // check if wallet file already exists
    if (!wallet_file.empty())
    {
      try
      {
        boost::system::error_code ignored_ec;
        THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(wallet_file, ignored_ec), error::file_exists, wallet_file);
      }
      catch (const std::exception &e)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Wallet already exists.";
        return false;
      }
    }

    cryptonote::address_parse_info info;
    std::unique_ptr<Wallet> wal;
    try {
      NetworkType nettype;
      if (!get_nettype(m_vm, nettype))
        throw std::runtime_error("unexpected nettype");

      if(!get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), req.address))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to parse public address";
        return false;
      }

      epee::wipeable_string viewkey_string = req.viewkey;
      crypto::secret_key viewkey;
      if (!viewkey_string.hex_to_pod(unwrap(unwrap(viewkey))))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to parse view key secret key";
        return false;
      }

      hw::device &hwdev = hw::get_device("default");
      if (!hwdev.verify_keys(viewkey, info.address.m_view_public_key))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "view secret key does not match main address";
        return false;
      }

      if (!req.spendkey.empty())
      {
        epee::wipeable_string spendkey_string = req.spendkey;
        crypto::secret_key spendkey;
        if (!spendkey_string.hex_to_pod(unwrap(unwrap(spendkey))))
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Failed to parse spend key secret key";
          return false;
        }

        if (!hwdev.verify_keys(spendkey, info.address.m_spend_public_key))
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "spend secret key does not match main address";
          return false;
        }
      }

      std::uint64_t kdf_rounds = command_line::get_arg(m_vm, arg_kdf_rounds);
      // Note : we skip seed language here and apply it ourself below
      //        recoverFromKeysWithPassword() only sets seed language for
      //            - spend-key only, deterministic (restore by spend-key only is not allowed in the wallet-rpc)
      //        and does not set it for
      //            - view-key + spend-key, deterministic
      //            - view-key + spend-key, non-deterministic (not needed, doesn't have a seed)
      //            - view-key only (not needed, doesn't have a seed)
      wal.reset(
        m_wallet_manager->createWalletFromKeys(
          wallet_file,
          req.password,
          /* language */ "",
          nettype,
          req.restore_height,
          req.address,
          req.viewkey,
          req.spendkey,
          kdf_rounds)
      );
      if (!wal)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to create wallet";
        return false;
      }
      int error_code, extended_error_code;
      std::string error_message;
      wal->statusWithErrorString(error_code, error_message, &extended_error_code);
      if (error_code != Wallet::Status::Status_Ok)
          tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message);

      if (!req.language.empty())
      {
        if (!crypto::ElectrumWords::is_valid_language(req.language))
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "The specified seed language is invalid.";
          return false;
        }
        wal->setSeedLanguage(req.language);
      }

      std::string prefix = req.spendkey.empty() ? "View-only " : wal->isDeterministic() ? "" : "Non-deterministic ";
      res.info = prefix + "Wallet has been generated successfully.";
      MINFO(prefix + "Wallet has been generated.\n");
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    if (!init_wallet(m_vm, wal, m_wallet_manager))
    {
      int error_code;
      std::string error_msg;
      wal->statusWithErrorString(error_code, error_msg);
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to initialize wallet: " + error_msg;
      return false;
    }

    // Store current wallet, if one was already open
    if (m_wallet_impl && req.autosave_current)
    {
      try
      {
        if (!wallet_file.empty())
        {
          m_wallet_impl->store(/* path */ "");
          THROW_WALLET_EXCEPTION_ON_API_ERROR();
        }
      }
      catch (const std::exception &e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    // Store newly generated wallet
    m_wallet_impl = std::move(wal);
    if (m_wallet_impl)
    {
      try
      {
        m_wallet_impl->store(/* path */ "");
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }

    res.address = m_wallet_impl->address();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_restore_deterministic_wallet(const wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::request &req, wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::response &res, epee::json_rpc::error &er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    // early check for mandatory fields
    if (req.seed.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "field 'seed' is mandatory. Please provide a seed you want to restore from.";
      return false;
    }

    const char *ptr = strchr(req.filename.c_str(), '/');
  #ifdef _WIN32
    if (!ptr)
      ptr = strchr(req.filename.c_str(), '\\');
    if (!ptr)
      ptr = strchr(req.filename.c_str(), ':');
  #endif
    if (ptr)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Invalid filename";
      return false;
    }
    std::string wallet_file = req.filename.empty() ? "" : (m_wallet_dir + "/" + req.filename);
    // check if wallet file already exists
    if (!wallet_file.empty())
    {
      try
      {
        boost::system::error_code ignored_ec;
        THROW_WALLET_EXCEPTION_IF(boost::filesystem::exists(wallet_file, ignored_ec), error::file_exists, wallet_file);
      }
      catch (const std::exception &e)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Wallet already exists.";
        return false;
      }
    }
    crypto::secret_key recovery_key;
    std::string old_language;

    // check the given seed
    if (!req.enable_multisig_experimental) {
      if (!crypto::ElectrumWords::words_to_bytes(req.seed, recovery_key, old_language))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Electrum-style word list failed verification";
        return false;
      }
    }
    bool was_deprecated_wallet = ((old_language == crypto::ElectrumWords::old_language_name) ||
                                  crypto::ElectrumWords::get_is_old_style_seed(req.seed));

    std::string mnemonic_language = old_language;
    if (was_deprecated_wallet)
    {
      // The user had used an older version of the wallet with old style mnemonics.
      res.was_deprecated = true;
    }

    if (old_language == crypto::ElectrumWords::old_language_name)
    {
      if (req.language.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Wallet was using the old seed language. You need to specify a new seed language.";
        return false;
      }
      if (!crypto::ElectrumWords::is_valid_language(req.language))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Wallet was using the old seed language, and the specified new seed language is invalid.";
        return false;
      }
      mnemonic_language = req.language;
    }

    // process seed_offset if given
    if (!req.seed_offset.empty())
    {
      if (req.enable_multisig_experimental)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Multisig seeds are not compatible with seed offsets";
        return false;
      }

      recovery_key = cryptonote::decrypt_key(recovery_key, req.seed_offset);
    }
    std::unique_ptr<Wallet> wal;
    try {
      NetworkType nettype;
      if (!get_nettype(m_vm, nettype))
        throw std::runtime_error("unexpected nettype");

      std::uint64_t kdf_rounds = command_line::get_arg(m_vm, arg_kdf_rounds);
      int error_code, extended_error_code;
      std::string error_message;
      std::string prefix = "";
      if (req.enable_multisig_experimental)
      {
        // Parse multisig seed into raw multisig data
        epee::wipeable_string multisig_data;
        multisig_data.resize(req.seed.size() / 2);
        if (!epee::from_hex::to_buffer(epee::to_mut_byte_span(multisig_data), req.seed))
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Multisig seed not represented as hexadecimal string";
          return false;
        }

        // Generate multisig wallet
        wal.reset(
          m_wallet_manager->createWalletFromMultisigSeed(
            wallet_file,
            req.password,
            mnemonic_language,
            nettype,
            req.restore_height,
            std::string(multisig_data.data(), multisig_data.size()),
            req.seed_offset,
            kdf_rounds)
        );
        prefix = "Multisig ";
      }
      else
      {
        // Generate normal wallet
        wal.reset(
          m_wallet_manager->recoveryWallet(
            wallet_file,
            req.password,
            req.seed,
            nettype,
            req.restore_height,
            kdf_rounds,
            req.seed_offset)
        );
      }
      if (!wal)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to create wallet";
        return false;
      }
      wal->statusWithErrorString(error_code, error_message, &extended_error_code);
      if (error_code != Wallet::Status::Status_Ok)
        tools::error::throw_wallet_ex<tools::error::wallet_internal_error>(std::string(__FILE__ ":" STRINGIZE(__LINE__)), error_message);

      if (req.enable_multisig_experimental)
        wal->setEnableMultisig(true);

      res.info = prefix + "Wallet has been restored successfully.";
      res.address = wal->address();
      MINFO(prefix + "Wallet has been restored.\n");
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    if (!init_wallet(m_vm, wal, m_wallet_manager))
    {
      int error_code;
      std::string error_msg;
      wal->statusWithErrorString(error_code, error_msg);
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to initialize wallet: " + error_msg;
      return false;
    }

    // // Convert the secret key back to seed
    epee::wipeable_string electrum_words;
    crypto::secret_key secret_spend_key = crypto::null_skey;
    epee::string_tools::hex_to_pod(wal->secretSpendKey(), secret_spend_key);
    if (!req.enable_multisig_experimental && !crypto::ElectrumWords::bytes_to_words(secret_spend_key, electrum_words, mnemonic_language))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to encode seed";
      return false;
    }
    res.seed = std::string(electrum_words.data(), electrum_words.size());

    // Store current wallet, if one was already open
    if (m_wallet_impl && req.autosave_current)
    {
      try
      {
        if (!wallet_file.empty())
        {
          m_wallet_impl->store(/* path */ "");
          THROW_WALLET_EXCEPTION_ON_API_ERROR();
        }
      }
      catch (const std::exception &e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    // Store newly generated wallet
    m_wallet_impl = std::move(wal);
    if (m_wallet_impl)
    {
      try
      {
        m_wallet_impl->store(/* path */ "");
        THROW_WALLET_EXCEPTION_ON_API_ERROR();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_is_multisig(const wallet_rpc::COMMAND_RPC_IS_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IS_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    const auto &ms_status = m_wallet_impl->multisig();

    res.multisig = ms_status.isMultisig;
    res.kex_is_done = ms_status.kexIsDone;
    res.ready = ms_status.isReady;
    res.threshold = ms_status.threshold;
    res.total = ms_status.total;

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_prepare_multisig(const wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (m_wallet_impl->multisig().isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is already multisig";
      return false;
    }
    if (req.enable_multisig_experimental)
      m_wallet_impl->setEnableMultisig(true);
    CHECK_MULTISIG_ENABLED();
    if (m_wallet_impl->watchOnly())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "wallet is watch-only and cannot be made multisig";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();

    res.multisig_info = m_wallet_impl->getMultisigInfo();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_multisig(const wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    if (m_wallet_impl->multisig().isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is already multisig";
      return false;
    }
    CHECK_MULTISIG_ENABLED();
    if (m_wallet_impl->watchOnly())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "wallet is watch-only and cannot be made multisig";
      return false;
    }
    CHECK_IF_BACKGROUND_SYNCING();

    try
    {
      res.multisig_info = m_wallet_impl->makeMultisig(req.multisig_info, req.threshold, req.password);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      res.address = m_wallet_impl->address();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_export_multisig(const wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    const MultisigState ms_status{m_wallet_impl->multisig()};

    if (!ms_status.isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ms_status.isReady)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }
    CHECK_MULTISIG_ENABLED();

    try
    {
      m_wallet_impl->exportMultisigImages(res.info);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_import_multisig(const wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    const MultisigState ms_status{m_wallet_impl->multisig()};

    if (!ms_status.isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ms_status.isReady)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }
    CHECK_MULTISIG_ENABLED();

    if (req.info.size() + 1 < ms_status.threshold)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Needs multisig export info from more participants";
      return false;
    }

    std::vector<cryptonote::blobdata> info;
    info.resize(req.info.size());
    for (size_t n = 0; n < info.size(); ++n)
    {
      if (!epee::string_tools::parse_hexstr_to_binbuff(req.info[n], info[n]))
      {
        er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
        er.message = "Failed to parse hex.";
        return false;
      }
    }

    try
    {
      res.n_outputs = m_wallet_impl->importMultisigImages(req.info);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = std::string{"Error calling import_multisig: "} + e.what();
      return false;
    }

    if (req.refresh_after_import)
    {
      if (m_wallet_impl->trustedDaemon())
      {
        try
        {
          m_wallet_impl->rescanSpent();
          THROW_WALLET_EXCEPTION_ON_API_ERROR();
        }
        catch (const std::exception &e)
        {
          er.message = std::string("Success, but failed to update spent status after import multisig info: ") + e.what();
        }
      }
      else
      {
        er.message = "Success, but cannot update spent status after import multisig info as daemon is untrusted";
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_exchange_multisig_keys(const wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::request& req, wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    MultisigState ms_status{m_wallet_impl->multisig()};

    if (!ms_status.isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    CHECK_MULTISIG_ENABLED();

    if (req.multisig_info.size() + 1 < ms_status.total)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Needs multisig info from more participants";
      return false;
    }

    try
    {
      res.multisig_info = m_wallet_impl->exchangeMultisigKeys(req.multisig_info, req.password, req.force_update_use_with_caution);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      ms_status = m_wallet_impl->multisig();
      if (ms_status.isReady)
      {
        res.address = m_wallet_impl->address();
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = std::string("Error calling exchange_multisig_info: ") + e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_multisig_key_exchange_booster(const wallet_rpc::COMMAND_RPC_GET_MULTISIG_KEY_EXCHANGE_BOOSTER::request& req, wallet_rpc::COMMAND_RPC_GET_MULTISIG_KEY_EXCHANGE_BOOSTER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    const MultisigState ms_status{m_wallet_impl->multisig()};

    if (ms_status.isReady)
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is multisig, and already finalized";
      return false;
    }

    if (req.multisig_info.size() == 0)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Needs multisig info from more participants";
      return false;
    }

    try
    {
      res.multisig_info = m_wallet_impl->getMultisigKeyExchangeBooster(
        req.multisig_info,
        req.threshold,
        req.num_signers,
        req.password);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = std::string("Error calling exchange_multisig_info_booster: ") + e.what();
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign_multisig(const wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    const MultisigState ms_status{m_wallet_impl->multisig()};

    if (!ms_status.isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ms_status.isReady)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }
    CHECK_MULTISIG_ENABLED();

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.tx_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    std::vector<std::string> txids;
    PendingTransaction *ptx;
    try
    {
      ptx = m_wallet_impl->restoreMultisigTransaction(req.tx_data_hex);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      if (!ptx)
      {
        er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
        er.message = "Failed to parse multisig tx data.";
        return false;
      }

      ptx->signMultisigTx(&txids);
      if (ptx->status() != PendingTransaction::Status::Status_Ok)
      {
        er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SIGNATURE;
        er.message = "Failed to sign multisig tx: " + ptx->errorString();
        return false;
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SIGNATURE;
      er.message = std::string("Failed to sign multisig tx: ") + e.what();
      return false;
    }

    res.tx_data_hex = ptx->multisigSignData();
    if (!txids.empty())
    {
      for (const std::string &txid: txids)
        res.tx_hash_list.push_back(txid);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_submit_multisig(const wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);
    const MultisigState ms_status{m_wallet_impl->multisig()};

    if (!ms_status.isMultisig)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ms_status.isReady)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }
    CHECK_MULTISIG_ENABLED();

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.tx_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    PendingTransaction *ptx;
    try
    {
      ptx = m_wallet_impl->restoreMultisigTransaction(req.tx_data_hex);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      if (!ptx)
      {
        er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
        er.message = "Failed to parse multisig tx data.";
        return false;
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SUBMISSION;
      er.message = std::string("Failed to submit multisig tx: ") + e.what();
      return false;
    }

    if (ptx->signersKeys().size() < ms_status.threshold)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Not enough signers signed this transaction.";
      return false;
    }

    auto txids = ptx->txid();
    try
    {
      ptx->commit();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SUBMISSION;
      er.message = std::string("Failed to submit multisig tx: ") + e.what();
      return false;
    }

    for (const std::string &txid : txids)
        res.tx_hash_list.push_back(txid);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_validate_address(const wallet_rpc::COMMAND_RPC_VALIDATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_VALIDATE_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    cryptonote::address_parse_info info;
    static const struct { cryptonote::network_type type; const char *stype; } net_types[] = {
      { cryptonote::MAINNET, "mainnet" },
      { cryptonote::TESTNET, "testnet" },
      { cryptonote::STAGENET, "stagenet" },
    };
    if (!req.any_net_type && !m_wallet_impl) return not_open(er);
    for (const auto &net_type: net_types)
    {
      if (!req.any_net_type && (!m_wallet_impl || static_cast<NetworkType>(net_type.type) != m_wallet_impl->nettype()))
        continue;
      if (req.allow_openalias)
      {
        std::string address;
        res.valid = get_account_address_from_str_or_url(info, net_type.type, req.address,
          [&er, &address](const std::string &url, const std::vector<std::string> &addresses, bool dnssec_valid)->std::string {
            if (!dnssec_valid)
            {
              er.message = std::string("Invalid DNSSEC for ") + url;
              return {};
            }
            if (addresses.empty())
            {
              er.message = std::string("No Monero address found at ") + url;
              return {};
            }
            address = addresses[0];
            return address;
          });
        if (res.valid)
          res.openalias_address = address;
      }
      else
      {
        res.valid = cryptonote::get_account_address_from_str(info, net_type.type, req.address);
      }
      if (res.valid)
      {
        res.integrated = info.has_payment_id;
        res.subaddress = info.is_subaddress;
        res.nettype = net_type.stype;
        return true;
      }
    }

    res.valid = false;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_daemon(const wallet_rpc::COMMAND_RPC_SET_DAEMON::request& req, wallet_rpc::COMMAND_RPC_SET_DAEMON::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet_impl) return not_open(er);

    if (m_wallet_impl->getWalletState().has_proxy_flag && !req.proxy.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_PROXY_ALREADY_DEFINED;
      er.message = "It is not possible to set daemon specific proxy when --proxy is defined.";
      return false;
    }
   
    std::vector<std::vector<uint8_t>> ssl_allowed_fingerprints;
    ssl_allowed_fingerprints.reserve(req.ssl_allowed_fingerprints.size());
    for (const std::string &fp: req.ssl_allowed_fingerprints)
    {
      std::vector<uint8_t> decoded;
      try
      {
        decoded = epee::from_hex_locale::to_vector(fp);
      }
      catch (const std::exception &)
      {
        er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
        er.message = "ssl_allowed_fingerprints[] entries must be hex-encoded SHA-256 values";
        return false;
      }

      if (decoded.size() != SSL_FINGERPRINT_SIZE)
      {
        er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
        er.message = "Each ssl_allowed_fingerprints[] entry must decode to exactly " BOOST_PP_STRINGIZE(SSL_FINGERPRINT_SIZE) " bytes";
        return false;
      }
      ssl_allowed_fingerprints.emplace_back(std::move(decoded));
    }

    epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_enabled;
    if (req.ssl_allow_any_cert)
      ssl_options.verification = epee::net_utils::ssl_verification_t::none;
    else if (!ssl_allowed_fingerprints.empty() || !req.ssl_ca_file.empty())
      ssl_options = epee::net_utils::ssl_options_t{std::move(ssl_allowed_fingerprints), std::move(req.ssl_ca_file)};

    if (!epee::net_utils::ssl_support_from_string(ssl_options.support, req.ssl_support))
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
      er.message = std::string("Invalid ssl support mode");
      return false;
    }

    const bool verification_required =
      ssl_options.verification != epee::net_utils::ssl_verification_t::none &&
      ssl_options.support == epee::net_utils::ssl_support_t::e_ssl_support_enabled;

    if (verification_required && !ssl_options.has_strong_verification(boost::string_ref{}))
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
      er.message = "SSL is enabled but no user certificate or fingerprints were provided";
      return false;
    }

    boost::optional<epee::net_utils::http::login> daemon_login{};
    if (!req.username.empty() || !req.password.empty())
      daemon_login.emplace(req.username, req.password);

    if (!m_wallet_impl->setDaemon(req.address,
                                  req.username,
                                  req.password,
                                  req.trusted,
                                  static_cast<Wallet::SSLSupport>(ssl_options.support),
                                  req.ssl_private_key_path,
                                  req.ssl_certificate_path,
                                  req.ssl_ca_file,
                                  req.ssl_allowed_fingerprints,
                                  req.ssl_allow_any_cert,
                                  req.proxy))
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
      er.message = std::string("Unable to set daemon");
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_log_level(const wallet_rpc::COMMAND_RPC_SET_LOG_LEVEL::request& req, wallet_rpc::COMMAND_RPC_SET_LOG_LEVEL::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    if (req.level < 0 || req.level > 4)
    {
      er.code = WALLET_RPC_ERROR_CODE_INVALID_LOG_LEVEL;
      er.message = "Error: log level not valid";
      return false;
    }
    mlog_set_log_level(req.level);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_log_categories(const wallet_rpc::COMMAND_RPC_SET_LOG_CATEGORIES::request& req, wallet_rpc::COMMAND_RPC_SET_LOG_CATEGORIES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    mlog_set_log(req.categories.c_str());
    res.categories = mlog_get_categories();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_estimate_tx_size_and_weight(const wallet_rpc::COMMAND_RPC_ESTIMATE_TX_SIZE_AND_WEIGHT::request& req, wallet_rpc::COMMAND_RPC_ESTIMATE_TX_SIZE_AND_WEIGHT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      size_t extra_size = 34 /* pubkey */ + 10 /* encrypted payment id */; // typical makeup
      const std::pair<size_t, uint64_t> sw = m_wallet_impl->estimateTxSizeAndWeight(req.rct, req.n_inputs, req.ring_size, req.n_outputs, extra_size);
      THROW_WALLET_EXCEPTION_ON_API_ERROR();
      res.size = sw.first;
      res.weight = sw.second;
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to determine size and weight";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_default_fee_priority(const wallet_rpc::COMMAND_RPC_GET_DEFAULT_FEE_PRIORITY::request& req, wallet_rpc::COMMAND_RPC_GET_DEFAULT_FEE_PRIORITY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet_impl) return not_open(er);
    try
    {
      uint32_t default_priority = m_wallet_impl->getDefaultPriority();
      res.priority = m_wallet_impl->adjustPriority(default_priority);
      if (res.priority == default_priority)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to get adjusted fee priority";
        return false;
      }
    }
    catch (const std::exception& e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to get adjusted fee priority";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_version(const wallet_rpc::COMMAND_RPC_GET_VERSION::request& req, wallet_rpc::COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.version = WALLET_RPC_VERSION;
    res.release = MONERO_VERSION_IS_RELEASE;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
}

class t_daemon
{
private:
  const boost::program_options::variables_map& vm;

  std::unique_ptr<tools::wallet_rpc_server> wrpc;

public:
  t_daemon(boost::program_options::variables_map const & _vm)
    : vm(_vm)
    , wrpc(new tools::wallet_rpc_server)
  {
  }

  bool run()
  {
    std::unique_ptr<Wallet> wal;
    std::unique_ptr<WalletManager> wallet_manager;
    try
    {
      NetworkType nettype;
      if (!get_nettype(vm, nettype))
        return false;

      const auto kdf_rounds = command_line::get_arg(vm, arg_kdf_rounds);

      const auto arg_wallet_file = wallet_args::arg_wallet_file();
      const auto arg_from_json = wallet_args::arg_generate_from_json();
      const auto arg_password_file = wallet_args::arg_password_file();

      const auto wallet_file = command_line::get_arg(vm, arg_wallet_file);
      const auto from_json = command_line::get_arg(vm, arg_from_json);
      const auto wallet_dir = command_line::get_arg(vm, arg_wallet_dir);
      const auto password_file = command_line::get_arg(vm, arg_password_file);
      const auto no_initial_sync = command_line::get_arg(vm, arg_no_initial_sync);

      wallet_manager.reset(WalletManagerFactory::getWalletManager());

      if(!wallet_file.empty() && !from_json.empty())
      {
        LOG_ERROR(tools::wallet_rpc_server::tr("Can't specify more than one of --wallet-file and --generate-from-json"));
        return false;
      }

      if(!wallet_dir.empty() && !password_file.empty())
      {
        LOG_ERROR(tools::wallet_rpc_server::tr("--password-file is not allowed in combination with --wallet-dir"));
        return false;
      }

      if (!wallet_dir.empty())
      {
        wal = NULL;
        goto just_dir;
      }

      if (wallet_file.empty() && from_json.empty())
      {
        LOG_ERROR(tools::wallet_rpc_server::tr("Must specify --wallet-file or --generate-from-json or --wallet-dir"));
        return false;
      }

      LOG_PRINT_L0(tools::wallet_rpc_server::tr("Loading wallet..."));
      if(!wallet_file.empty())
      {
        epee::wipeable_string password = get_password(vm, tools::wallet_rpc_server::tr("Wallet password"), /* verify */ false);
        std::string tmp_pw = std::string(password.data(), password.size());
        epee::misc_utils::auto_scope_leave_caller tmp_pw_scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){
          memwipe(&tmp_pw[0], tmp_pw.size());
        });

        wal.reset(
          wallet_manager->openWallet(
            wallet_file,
            tmp_pw,
            nettype,
            kdf_rounds,
            /* listener */ nullptr,
            /* unattended */ true)
        );
      }
      else
      {
        try
        {
          std::string pw_out;
          wal.reset(
            wallet_manager->createWalletFromJson(
              from_json,
              nettype,
              pw_out,
              kdf_rounds)
          );
        }
        catch (const std::exception &e)
        {
          MERROR("Error creating wallet: " << e.what());
          return false;
        }
      }
      if (!wal)
      {
        MERROR("Failed to create wallet");
        return false;
      }
      int error_code;
      std::string error_msg;
      wal->statusWithErrorString(error_code, error_msg);
      if (error_code)
      {
        MERROR(tr("failed to load wallet: ") << error_msg);
        return {};
      }

      if (!init_wallet(vm, wal, wallet_manager))
      {
        wal->statusWithErrorString(error_code, error_msg);
        if (error_code)
          MERROR(tr("failed to initialize wallet: ") << error_msg);
        return {};
      }

      bool quit = false;
      tools::signal_handler::install([&wal, &quit](int) {
        assert(wal);
        quit = true;
        wal->stop();
      });

      if (!no_initial_sync)
      {
        if (wal->connected() != Wallet::ConnectionStatus_Connected)
            LOG_ERROR(tools::wallet_rpc_server::tr("Initial refresh failed: no connection to daemon"));
        if (!wal->refresh())
        {
            int error_code{};
            std::string error_msg{};
            wal->statusWithErrorString(error_code, error_msg);
            LOG_ERROR(tools::wallet_rpc_server::tr("Initial refresh failed: ") << error_msg);
        }
      }
      // if we ^C during potentially length load/refresh, there's no server loop yet
      if (quit)
      {
        MINFO(tools::wallet_rpc_server::tr("Saving wallet..."));
        wal->store("");
        MINFO(tools::wallet_rpc_server::tr("Successfully saved"));
        return false;
      }
      MINFO(tools::wallet_rpc_server::tr("Successfully loaded"));
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(tools::wallet_rpc_server::tr("Wallet initialization failed: ") << e.what());
      return false;
    }
  just_dir:
    if (wal) wrpc->set_wallet(wal.release());
    if (wallet_manager) wrpc->set_wallet_manager(wallet_manager.release());
    bool r = wrpc->init(&vm);
    CHECK_AND_ASSERT_MES(r, false, tools::wallet_rpc_server::tr("Failed to initialize wallet RPC server"));
    tools::signal_handler::install([this](int) {
      wrpc->send_stop_signal();
    });
    LOG_PRINT_L0(tools::wallet_rpc_server::tr("Starting wallet RPC server"));
    try
    {
      wrpc->run();
    }
    catch (const std::exception &e)
    {
      LOG_ERROR(tools::wallet_rpc_server::tr("Failed to run wallet: ") << e.what());
      return false;
    }
    LOG_PRINT_L0(tools::wallet_rpc_server::tr("Stopped wallet RPC server"));
    try
    {
      LOG_PRINT_L0(tools::wallet_rpc_server::tr("Saving wallet..."));
      wrpc->stop();
      LOG_PRINT_L0(tools::wallet_rpc_server::tr("Successfully saved"));
    }
    catch (const std::exception& e)
    {
      LOG_ERROR(tools::wallet_rpc_server::tr("Failed to save wallet: ") << e.what());
      return false;
    }
    return true;
  }

  void stop()
  {
    wrpc->send_stop_signal();
  }
};

class t_executor final
{
public:
  static std::string const NAME;

  typedef ::t_daemon t_daemon;

  std::string const & name() const
  {
    return NAME;
  }

  t_daemon create_daemon(boost::program_options::variables_map const & vm)
  {
    return t_daemon(vm);
  }

  bool run_non_interactive(boost::program_options::variables_map const & vm)
  {
    return t_daemon(vm).run();
  }

  bool run_interactive(boost::program_options::variables_map const & vm)
  {
    return t_daemon(vm).run();
  }
};

std::string const t_executor::NAME = "Wallet RPC Daemon";

int main(int argc, char** argv) {
  TRY_ENTRY();

  namespace po = boost::program_options;

  const auto arg_wallet_file = wallet_args::arg_wallet_file();
  const auto arg_from_json = wallet_args::arg_generate_from_json();

  po::options_description hidden_options("Hidden");

  po::options_description desc_params(wallet_args::tr("Wallet options"));
  command_line::add_arg(desc_params, arg_rpc_bind_port);
  command_line::add_arg(desc_params, arg_disable_rpc_login);
  command_line::add_arg(desc_params, arg_restricted);
  cryptonote::rpc_args::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_from_json);
  command_line::add_arg(desc_params, arg_wallet_dir);
  command_line::add_arg(desc_params, arg_prompt_for_password);
  command_line::add_arg(desc_params, arg_no_initial_sync);
  command_line::add_arg(desc_params, arg_rpc_max_connections_per_public_ip);
  command_line::add_arg(desc_params, arg_rpc_max_connections_per_private_ip);
  command_line::add_arg(desc_params, arg_rpc_max_connections);
  command_line::add_arg(desc_params, arg_rpc_response_soft_limit);
  command_line::add_arg(hidden_options, daemonizer::arg_non_interactive);
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
  command_line::add_arg(desc_params, arg_offline );
  command_line::add_arg(desc_params, arg_extra_entropy );

  daemonizer::init_options(hidden_options, desc_params);
  desc_params.add(hidden_options);

  boost::optional<po::variables_map> vm;
  bool should_terminate = false;
  std::tie(vm, should_terminate) = wallet_args::main(
    argc, argv,
    "monero-wallet-rpc [--wallet-file=<file>|--generate-from-json=<file>|--wallet-dir=<directory>] [--rpc-bind-port=<port>]",
    tools::wallet_rpc_server::tr("This is the RPC monero wallet. It needs to connect to a monero\ndaemon to work correctly."),
    desc_params,
    po::positional_options_description(),
    [](const std::string &s, bool emphasis){ tools::scoped_message_writer(emphasis ? epee::console_color_white : epee::console_color_default, true) << s; },
    "monero-wallet-rpc.log",
    true
  );
  if (!vm)
  {
    return 1;
  }
  if (should_terminate)
  {
    return 0;
  }

  return daemonizer::daemonize(argc, const_cast<const char**>(argv), t_executor{}, *vm) ? 0 : 1;
  CATCH_ENTRY_L0("main", 1);
}
