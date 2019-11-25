// Copyright (c) 2014-2019, The Monero Project
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
#include "include_base_utils.h"
using namespace epee;

#include "version.h"
#include "wallet_rpc_server.h"
#include "wallet/wallet_args.h"
#include "common/command_line.h"
#include "common/i18n.h"
#include "cryptonote_config.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/account.h"
#include "multisig/multisig.h"
#include "wallet_rpc_server_commands_defs.h"
#include "misc_language.h"
#include "string_coding.h"
#include "string_tools.h"
#include "crypto/hash.h"
#include "mnemonics/electrum-words.h"
#include "rpc/rpc_args.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "daemonizer/daemonizer.h"
#include "wallet/message_store.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

#define DEFAULT_AUTO_REFRESH_PERIOD 20 // seconds

namespace
{
  const command_line::arg_descriptor<std::string, true> arg_rpc_bind_port = {"rpc-bind-port", "Sets bind port for server"};
  const command_line::arg_descriptor<bool> arg_disable_rpc_login = {"disable-rpc-login", "Disable HTTP authentication for RPC connections served by this process"};
  const command_line::arg_descriptor<bool> arg_restricted = {"restricted-rpc", "Restricts to view-only commands", false};
  const command_line::arg_descriptor<std::string> arg_wallet_dir = {"wallet-dir", "Directory for newly created wallets"};
  const command_line::arg_descriptor<bool> arg_prompt_for_password = {"prompt-for-password", "Prompts for password when not provided", false};

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
  void set_confirmations(tools::wallet_rpc::transfer_entry &entry, uint64_t blockchain_height, uint64_t block_reward)
  {
    if (entry.height >= blockchain_height || (entry.height == 0 && (!strcmp(entry.type.c_str(), "pending") || !strcmp(entry.type.c_str(), "pool"))))
      entry.confirmations = 0;
    else
      entry.confirmations = blockchain_height - entry.height;

    if (block_reward == 0)
      entry.suggested_confirmations_threshold = 0;
    else
      entry.suggested_confirmations_threshold = (entry.amount + block_reward - 1) / block_reward;
  }
}

namespace tools
{
  const char* wallet_rpc_server::tr(const char* str)
  {
    return i18n_translate(str, "tools::wallet_rpc_server");
  }

  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server():m_wallet(NULL), rpc_login_file(), m_stop(false), m_restricted(false), m_vm(NULL)
  {
  }
  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::~wallet_rpc_server()
  {
    if (m_wallet)
      delete m_wallet;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::set_wallet(wallet2 *cr)
  {
    m_wallet = cr;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::run()
  {
    m_stop = false;
    m_net_server.add_idle_handler([this](){
      if (m_auto_refresh_period == 0) // disabled
        return true;
      if (boost::posix_time::microsec_clock::universal_time() < m_last_auto_refresh_time + boost::posix_time::seconds(m_auto_refresh_period))
        return true;
      try {
        if (m_wallet) m_wallet->refresh(m_wallet->is_trusted_daemon());
      } catch (const std::exception& ex) {
        LOG_ERROR("Exception at while refreshing, what=" << ex.what());
      }
      m_last_auto_refresh_time = boost::posix_time::microsec_clock::universal_time();
      return true;
    }, 1000);
    m_net_server.add_idle_handler([this](){
      if (m_stop.load(std::memory_order_relaxed))
      {
        send_stop_signal();
        return false;
      }
      return true;
    }, 500);

    //DO NOT START THIS SERVER IN MORE THEN 1 THREADS WITHOUT REFACTORING
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::run(1, true);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::stop()
  {
    if (m_wallet)
    {
      m_wallet->store();
      m_wallet->deinit();
      delete m_wallet;
      m_wallet = NULL;
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
        rpc_login_file = tools::private_file::create(temp);
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

    m_auto_refresh_period = DEFAULT_AUTO_REFRESH_PERIOD;
    m_last_auto_refresh_time = boost::posix_time::min_date_time;

    check_background_mining();

    m_net_server.set_threads_prefix("RPC");
    auto rng = [](size_t len, uint8_t *ptr) { return crypto::rand(len, ptr); };
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::init(
      rng, std::move(bind_port), std::move(rpc_config->bind_ip),
      std::move(rpc_config->bind_ipv6_address), std::move(rpc_config->use_ipv6), std::move(rpc_config->require_ipv4),
      std::move(rpc_config->access_control_origins), std::move(http_login),
      std::move(rpc_config->ssl_options)
    );
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::check_background_mining()
  {
    if (!m_wallet)
      return;

    tools::wallet2::BackgroundMiningSetupType setup = m_wallet->setup_background_mining();
    if (setup == tools::wallet2::BackgroundMiningNo)
    {
      MLOG_RED(el::Level::Warning, "Background mining not enabled. Run \"set setup-background-mining 1\" in monero-wallet-cli to change.");
      return;
    }

    if (!m_wallet->is_trusted_daemon())
    {
      MDEBUG("Using an untrusted daemon, skipping background mining check");
      return;
    }

    cryptonote::COMMAND_RPC_MINING_STATUS::request req;
    cryptonote::COMMAND_RPC_MINING_STATUS::response res;
    bool r = m_wallet->invoke_http_json("/mining_status", req, res);
    if (!r || res.status != CORE_RPC_STATUS_OK)
    {
      MERROR("Failed to query mining status: " << (r ? res.status : "No connection to daemon"));
      return;
    }
    if (res.active || res.is_background_mining_enabled)
      return;

    if (setup == tools::wallet2::BackgroundMiningMaybe)
    {
      MINFO("The daemon is not set up to background mine.");
      MINFO("With background mining enabled, the daemon will mine when idle and not on battery.");
      MINFO("Enabling this supports the network you are using, and makes you eligible for receiving new monero");
      MINFO("Set setup-background-mining to 1 in monero-wallet-cli to change.");
      return;
    }

    cryptonote::COMMAND_RPC_START_MINING::request req2;
    cryptonote::COMMAND_RPC_START_MINING::response res2;
    req2.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    req2.threads_count = 1;
    req2.do_background_mining = true;
    req2.ignore_battery = false;
    r = m_wallet->invoke_http_json("/start_mining", req2, res);
    if (!r || res2.status != CORE_RPC_STATUS_OK)
    {
      MERROR("Failed to setup background mining: " << (r ? res.status : "No connection to daemon"));
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
  bool wallet_rpc_server::restricted(epee::json_rpc::error& er)
  {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const crypto::hash &payment_id, const tools::wallet2::payment_details &pd)
  {
    entry.txid = string_tools::pod_to_hex(pd.m_tx_hash);
    entry.payment_id = string_tools::pod_to_hex(payment_id);
    if (entry.payment_id.substr(16).find_first_not_of('0') == std::string::npos)
      entry.payment_id = entry.payment_id.substr(0,16);
    entry.height = pd.m_block_height;
    entry.timestamp = pd.m_timestamp;
    entry.amount = pd.m_amount;
    entry.unlock_time = pd.m_unlock_time;
    entry.locked = !m_wallet->is_transfer_unlocked(pd.m_unlock_time, pd.m_block_height);
    entry.fee = pd.m_fee;
    entry.note = m_wallet->get_tx_note(pd.m_tx_hash);
    entry.type = pd.m_coinbase ? "block" : "in";
    entry.subaddr_index = pd.m_subaddr_index;
    entry.subaddr_indices.push_back(pd.m_subaddr_index);
    entry.address = m_wallet->get_subaddress_as_str(pd.m_subaddr_index);
    set_confirmations(entry, m_wallet->get_blockchain_current_height(), m_wallet->get_last_block_reward());
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::confirmed_transfer_details &pd)
  {
    entry.txid = string_tools::pod_to_hex(txid);
    entry.payment_id = string_tools::pod_to_hex(pd.m_payment_id);
    if (entry.payment_id.substr(16).find_first_not_of('0') == std::string::npos)
      entry.payment_id = entry.payment_id.substr(0,16);
    entry.height = pd.m_block_height;
    entry.timestamp = pd.m_timestamp;
    entry.unlock_time = pd.m_unlock_time;
    entry.locked = !m_wallet->is_transfer_unlocked(pd.m_unlock_time, pd.m_block_height);
    entry.fee = pd.m_amount_in - pd.m_amount_out;
    uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
    entry.amount = pd.m_amount_in - change - entry.fee;
    entry.note = m_wallet->get_tx_note(txid);

    for (const auto &d: pd.m_dests) {
      entry.destinations.push_back(wallet_rpc::transfer_destination());
      wallet_rpc::transfer_destination &td = entry.destinations.back();
      td.amount = d.amount;
      td.address = d.address(m_wallet->nettype(), pd.m_payment_id);
    }

    entry.type = "out";
    entry.subaddr_index = { pd.m_subaddr_account, 0 };
    for (uint32_t i: pd.m_subaddr_indices)
      entry.subaddr_indices.push_back({pd.m_subaddr_account, i});
    entry.address = m_wallet->get_subaddress_as_str({pd.m_subaddr_account, 0});
    set_confirmations(entry, m_wallet->get_blockchain_current_height(), m_wallet->get_last_block_reward());
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::unconfirmed_transfer_details &pd)
  {
    bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;
    entry.txid = string_tools::pod_to_hex(txid);
    entry.payment_id = string_tools::pod_to_hex(pd.m_payment_id);
    entry.payment_id = string_tools::pod_to_hex(pd.m_payment_id);
    if (entry.payment_id.substr(16).find_first_not_of('0') == std::string::npos)
      entry.payment_id = entry.payment_id.substr(0,16);
    entry.height = 0;
    entry.timestamp = pd.m_timestamp;
    entry.fee = pd.m_amount_in - pd.m_amount_out;
    entry.amount = pd.m_amount_in - pd.m_change - entry.fee;
    entry.unlock_time = pd.m_tx.unlock_time;
    entry.locked = true;
    entry.note = m_wallet->get_tx_note(txid);

    for (const auto &d: pd.m_dests) {
      entry.destinations.push_back(wallet_rpc::transfer_destination());
      wallet_rpc::transfer_destination &td = entry.destinations.back();
      td.amount = d.amount;
      td.address = d.address(m_wallet->nettype(), pd.m_payment_id);
    }

    entry.type = is_failed ? "failed" : "pending";
    entry.subaddr_index = { pd.m_subaddr_account, 0 };
    for (uint32_t i: pd.m_subaddr_indices)
      entry.subaddr_indices.push_back({pd.m_subaddr_account, i});
    entry.address = m_wallet->get_subaddress_as_str({pd.m_subaddr_account, 0});
    set_confirmations(entry, m_wallet->get_blockchain_current_height(), m_wallet->get_last_block_reward());
  }
  //------------------------------------------------------------------------------------------------------------------------------
  void wallet_rpc_server::fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &payment_id, const tools::wallet2::pool_payment_details &ppd)
  {
    const tools::wallet2::payment_details &pd = ppd.m_pd;
    entry.txid = string_tools::pod_to_hex(pd.m_tx_hash);
    entry.payment_id = string_tools::pod_to_hex(payment_id);
    if (entry.payment_id.substr(16).find_first_not_of('0') == std::string::npos)
      entry.payment_id = entry.payment_id.substr(0,16);
    entry.height = 0;
    entry.timestamp = pd.m_timestamp;
    entry.amount = pd.m_amount;
    entry.unlock_time = pd.m_unlock_time;
    entry.locked = true;
    entry.fee = pd.m_fee;
    entry.note = m_wallet->get_tx_note(pd.m_tx_hash);
    entry.double_spend_seen = ppd.m_double_spend_seen;
    entry.type = "pool";
    entry.subaddr_index = pd.m_subaddr_index;
    entry.subaddr_indices.push_back(pd.m_subaddr_index);
    entry.address = m_wallet->get_subaddress_as_str(pd.m_subaddr_index);
    set_confirmations(entry, m_wallet->get_blockchain_current_height(), m_wallet->get_last_block_reward());
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    try
    {
      res.balance = req.all_accounts ? m_wallet->balance_all(req.strict) : m_wallet->balance(req.account_index, req.strict);
      res.unlocked_balance = req.all_accounts ? m_wallet->unlocked_balance_all(req.strict, &res.blocks_to_unlock) : m_wallet->unlocked_balance(req.account_index, req.strict, &res.blocks_to_unlock);
      res.multisig_import_needed = m_wallet->multisig() && m_wallet->has_multisig_partial_key_images();
      std::map<uint32_t, std::map<uint32_t, uint64_t>> balance_per_subaddress_per_account;
      std::map<uint32_t, std::map<uint32_t, std::pair<uint64_t, uint64_t>>> unlocked_balance_per_subaddress_per_account;
      if (req.all_accounts)
      {
        for (uint32_t account_index = 0; account_index < m_wallet->get_num_subaddress_accounts(); ++account_index)
        {
          balance_per_subaddress_per_account[account_index] = m_wallet->balance_per_subaddress(account_index, req.strict);
          unlocked_balance_per_subaddress_per_account[account_index] = m_wallet->unlocked_balance_per_subaddress(account_index, req.strict);
        }
      }
      else
      {
        balance_per_subaddress_per_account[req.account_index] = m_wallet->balance_per_subaddress(req.account_index, req.strict);
        unlocked_balance_per_subaddress_per_account[req.account_index] = m_wallet->unlocked_balance_per_subaddress(req.account_index, req.strict);
      }
      std::vector<tools::wallet2::transfer_details> transfers;
      m_wallet->get_transfers(transfers);
      for (const auto& p : balance_per_subaddress_per_account)
      {
        uint32_t account_index = p.first;
        std::map<uint32_t, uint64_t> balance_per_subaddress = p.second;
        std::map<uint32_t, std::pair<uint64_t, uint64_t>> unlocked_balance_per_subaddress = unlocked_balance_per_subaddress_per_account[account_index];
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
          cryptonote::subaddress_index index = {info.account_index, info.address_index};
          info.address = m_wallet->get_subaddress_as_str(index);
          info.balance = balance_per_subaddress[i];
          info.unlocked_balance = unlocked_balance_per_subaddress[i].first;
          info.blocks_to_unlock = unlocked_balance_per_subaddress[i].second;
          info.label = m_wallet->get_subaddress_label(index);
          info.num_unspent_outputs = std::count_if(transfers.begin(), transfers.end(), [&](const tools::wallet2::transfer_details& td) { return !td.m_spent && td.m_subaddr_index == index; });
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
    if (!m_wallet) return not_open(er);
    try
    {
      THROW_WALLET_EXCEPTION_IF(req.account_index >= m_wallet->get_num_subaddress_accounts(), error::account_index_outofbound);
      res.addresses.clear();
      std::vector<uint32_t> req_address_index;
      if (req.address_index.empty())
      {
        for (uint32_t i = 0; i < m_wallet->get_num_subaddresses(req.account_index); ++i)
          req_address_index.push_back(i);
      }
      else
      {
        req_address_index = req.address_index;
      }
      tools::wallet2::transfer_container transfers;
      m_wallet->get_transfers(transfers);
      for (uint32_t i : req_address_index)
      {
        THROW_WALLET_EXCEPTION_IF(i >= m_wallet->get_num_subaddresses(req.account_index), error::address_index_outofbound);
        res.addresses.resize(res.addresses.size() + 1);
        auto& info = res.addresses.back();
        const cryptonote::subaddress_index index = {req.account_index, i};
        info.address = m_wallet->get_subaddress_as_str(index);
        info.label = m_wallet->get_subaddress_label(index);
        info.address_index = index.minor;
        info.used = std::find_if(transfers.begin(), transfers.end(), [&](const tools::wallet2::transfer_details& td) { return td.m_subaddr_index == index; }) != transfers.end();
      }
      res.address = m_wallet->get_subaddress_as_str({req.account_index, 0});
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
    if (!m_wallet) return not_open(er);
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_wallet->nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }
    auto index = m_wallet->get_subaddress_index(info.address);
    if (!index)
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Address doesn't belong to the wallet";
      return false;
    }
    res.index = *index;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_create_address(const wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->add_subaddress(req.account_index, req.label);
      res.address_index = m_wallet->get_num_subaddresses(req.account_index) - 1;
      res.address = m_wallet->get_subaddress_as_str({req.account_index, res.address_index});
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->set_subaddress_label(req.index, req.label);
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
    if (!m_wallet) return not_open(er);
    try
    {
      res.total_balance = 0;
      res.total_unlocked_balance = 0;
      cryptonote::subaddress_index subaddr_index = {0,0};
      const std::pair<std::map<std::string, std::string>, std::vector<std::string>> account_tags = m_wallet->get_account_tags();
      if (!req.tag.empty() && account_tags.first.count(req.tag) == 0)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = (boost::format(tr("Tag %s is unregistered.")) % req.tag).str();
        return false;
      }
      for (; subaddr_index.major < m_wallet->get_num_subaddress_accounts(); ++subaddr_index.major)
      {
        if (!req.tag.empty() && req.tag != account_tags.second[subaddr_index.major])
          continue;
        wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::subaddress_account_info info;
        info.account_index = subaddr_index.major;
        info.base_address = m_wallet->get_subaddress_as_str(subaddr_index);
        info.balance = m_wallet->balance(subaddr_index.major, req.strict_balances);
        info.unlocked_balance = m_wallet->unlocked_balance(subaddr_index.major, req.strict_balances);
        info.label = m_wallet->get_subaddress_label(subaddr_index);
        info.tag = account_tags.second[subaddr_index.major];
        res.subaddress_accounts.push_back(info);
        res.total_balance += info.balance;
        res.total_unlocked_balance += info.unlocked_balance;
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->add_subaddress_account(req.label);
      res.account_index = m_wallet->get_num_subaddress_accounts() - 1;
      res.address = m_wallet->get_subaddress_as_str({res.account_index, 0});
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->set_subaddress_label({req.account_index, 0}, req.label);
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
    if (!m_wallet) return not_open(er);
    const std::pair<std::map<std::string, std::string>, std::vector<std::string>> account_tags = m_wallet->get_account_tags();
    for (const std::pair<std::string, std::string>& p : account_tags.first)
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->set_account_tag(req.accounts, req.tag);
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->set_account_tag(req.accounts, "");
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
    if (!m_wallet) return not_open(er);
    try
    {
      m_wallet->set_account_tag_description(req.tag, req.description);
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
    if (!m_wallet) return not_open(er);
    try
    {
      res.height = m_wallet->get_blockchain_current_height();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::validate_transfer(const std::list<wallet_rpc::transfer_destination>& destinations, const std::string& payment_id, std::vector<cryptonote::tx_destination_entry>& dsts, std::vector<uint8_t>& extra, bool at_least_one_destination, epee::json_rpc::error& er)
  {
    crypto::hash8 integrated_payment_id = crypto::null_hash8;
    std::string extra_nonce;
    for (auto it = destinations.begin(); it != destinations.end(); it++)
    {
      cryptonote::address_parse_info info;
      cryptonote::tx_destination_entry de;
      er.message = "";
      if(!get_account_address_from_str_or_url(info, m_wallet->nettype(), it->address,
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

      de.original = it->address;
      de.addr = info.address;
      de.is_subaddress = info.is_subaddress;
      de.amount = it->amount;
      de.is_integrated = info.has_payment_id;
      dsts.push_back(de);

      if (info.has_payment_id)
      {
        if (!payment_id.empty() || integrated_payment_id != crypto::null_hash8)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "A single payment id is allowed per transaction";
          return false;
        }
        integrated_payment_id = info.payment_id;
        cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, integrated_payment_id);

        /* Append Payment ID data into extra */
        if (!cryptonote::add_extra_nonce_to_tx_extra(extra, extra_nonce)) {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Something went wrong with integrated payment_id.";
          return false;
        }
      }
    }

    if (at_least_one_destination && dsts.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_ZERO_DESTINATION;
      er.message = "No destinations for this transfer";
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
  static std::string ptx_to_string(const tools::wallet2::pending_tx &ptx)
  {
    std::ostringstream oss;
    boost::archive::portable_binary_oarchive ar(oss);
    try
    {
      ar << ptx;
    }
    catch (...)
    {
      return "";
    }
    return epee::string_tools::buff_to_hex_nodelimer(oss.str());
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
  static uint64_t total_amount(const tools::wallet2::pending_tx &ptx)
  {
    uint64_t amount = 0;
    for (const auto &dest: ptx.dests) amount += dest.amount;
    return amount;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  template<typename Ts, typename Tu>
  bool wallet_rpc_server::fill_response(std::vector<tools::wallet2::pending_tx> &ptx_vector,
      bool get_tx_key, Ts& tx_key, Tu &amount, Tu &fee, std::string &multisig_txset, std::string &unsigned_txset, bool do_not_relay,
      Ts &tx_hash, bool get_tx_hex, Ts &tx_blob, bool get_tx_metadata, Ts &tx_metadata, epee::json_rpc::error &er)
  {
    for (const auto & ptx : ptx_vector)
    {
      if (get_tx_key)
      {
        epee::wipeable_string s = epee::to_hex::wipeable_string(ptx.tx_key);
        for (const crypto::secret_key& additional_tx_key : ptx.additional_tx_keys)
          s += epee::to_hex::wipeable_string(additional_tx_key);
        fill(tx_key, std::string(s.data(), s.size()));
      }
      // Compute amount leaving wallet in tx. By convention dests does not include change outputs
      fill(amount, total_amount(ptx));
      fill(fee, ptx.fee);
    }

    if (m_wallet->multisig())
    {
      multisig_txset = epee::string_tools::buff_to_hex_nodelimer(m_wallet->save_multisig_tx(ptx_vector));
      if (multisig_txset.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Failed to save multisig tx set after creation";
        return false;
      }
    }
    else
    {
      if (m_wallet->watch_only()){
        unsigned_txset = epee::string_tools::buff_to_hex_nodelimer(m_wallet->dump_tx_to_str(ptx_vector));
        if (unsigned_txset.empty())
        {
          er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
          er.message = "Failed to save unsigned tx set after creation";
          return false;
        }
      }
      else if (!do_not_relay)
        m_wallet->commit_tx(ptx_vector);

      // populate response with tx hashes
      for (auto & ptx : ptx_vector)
      {
        bool r = fill(tx_hash, epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(ptx.tx)));
        r = r && (!get_tx_hex || fill(tx_blob, epee::string_tools::buff_to_hex_nodelimer(tx_to_blob(ptx.tx))));
        r = r && (!get_tx_metadata || fill(tx_metadata, ptx_to_string(ptx)));
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

    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    LOG_PRINT_L3("on_transfer starts");
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    // validate the transfer requested and populate dsts & extra
    if (!validate_transfer(req.destinations, req.payment_id, dsts, extra, true, er))
    {
      return false;
    }

    try
    {
      uint64_t mixin = m_wallet->adjust_mixin(req.ring_size ? req.ring_size - 1 : 0);
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_2(dsts, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices);

      if (ptx_vector.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
        er.message = "No transaction created";
        return false;
      }

      // reject proposed transactions if there are more than one.  see on_transfer_split below.
      if (ptx_vector.size() != 1)
      {
        er.code = WALLET_RPC_ERROR_CODE_TX_TOO_LARGE;
        er.message = "Transaction would be too large.  try /transfer_split.";
        return false;
      }

      return fill_response(ptx_vector, req.get_tx_key, res.tx_key, res.amount, res.fee, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash, req.get_tx_hex, res.tx_blob, req.get_tx_metadata, res.tx_metadata, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {

    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    // validate the transfer requested and populate dsts & extra; RPC_TRANSFER::request and RPC_TRANSFER_SPLIT::request are identical types.
    if (!validate_transfer(req.destinations, req.payment_id, dsts, extra, true, er))
    {
      return false;
    }

    try
    {
      uint64_t mixin = m_wallet->adjust_mixin(req.ring_size ? req.ring_size - 1 : 0);
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      LOG_PRINT_L2("on_transfer_split calling create_transactions_2");
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_2(dsts, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices);
      LOG_PRINT_L2("on_transfer_split called create_transactions_2");

      if (ptx_vector.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
        er.message = "No transaction created";
        return false;
      }

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash_list, req.get_tx_hex, res.tx_blob_list, req.get_tx_metadata, res.tx_metadata_list, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign_transfer(const wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->key_on_device())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    if(m_wallet->watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "command not supported by watch-only wallet";
      return false;
    }

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.unsigned_txset, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    tools::wallet2::unsigned_tx_set exported_txs;
    if(!m_wallet->parse_unsigned_tx_from_str(blob, exported_txs))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
      er.message = "cannot load unsigned_txset";
      return false;
    }

    std::vector<tools::wallet2::pending_tx> ptxs;
    try
    {
      tools::wallet2::signed_tx_set signed_txs;
      std::string ciphertext = m_wallet->sign_tx_dump_to_str(exported_txs, ptxs, signed_txs);
      if (ciphertext.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_SIGN_UNSIGNED;
        er.message = "Failed to sign unsigned tx";
        return false;
      }

      res.signed_txset = epee::string_tools::buff_to_hex_nodelimer(ciphertext);
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_SIGN_UNSIGNED;
      er.message = std::string("Failed to sign unsigned tx: ") + e.what();
      return false;
    }

    for (auto &ptx: ptxs)
    {
      res.tx_hash_list.push_back(epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(ptx.tx)));
      if (req.get_tx_keys)
      {
        res.tx_key_list.push_back(epee::string_tools::pod_to_hex(ptx.tx_key));
        for (const crypto::secret_key& additional_tx_key : ptx.additional_tx_keys)
          res.tx_key_list.back() += epee::string_tools::pod_to_hex(additional_tx_key);
      }
    }

    if (req.export_raw)
    {
      for (auto &ptx: ptxs)
      {
        res.tx_raw_list.push_back(epee::string_tools::buff_to_hex_nodelimer(cryptonote::tx_to_blob(ptx.tx)));
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_describe_transfer(const wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->key_on_device())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }
    if(m_wallet->watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "command not supported by watch-only wallet";
      return false;
    }
    if(req.unsigned_txset.empty() && req.multisig_txset.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "no txset provided";
      return false;
    }

    std::vector <wallet2::tx_construction_data> tx_constructions;
    if (!req.unsigned_txset.empty()) {
      try {
        tools::wallet2::unsigned_tx_set exported_txs;
        cryptonote::blobdata blob;
        if (!epee::string_tools::parse_hexstr_to_binbuff(req.unsigned_txset, blob)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
          er.message = "Failed to parse hex.";
          return false;
        }
        if (!m_wallet->parse_unsigned_tx_from_str(blob, exported_txs)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
          er.message = "cannot load unsigned_txset";
          return false;
        }
        tx_constructions = exported_txs.txes;
      }
      catch (const std::exception &e) {
        er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
        er.message = "failed to parse unsigned transfers: " + std::string(e.what());
        return false;
      }
    } else if (!req.multisig_txset.empty()) {
      try {
        tools::wallet2::multisig_tx_set exported_txs;
        cryptonote::blobdata blob;
        if (!epee::string_tools::parse_hexstr_to_binbuff(req.multisig_txset, blob)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
          er.message = "Failed to parse hex.";
          return false;
        }
        if (!m_wallet->parse_multisig_tx_from_str(blob, exported_txs)) {
          er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
          er.message = "cannot load multisig_txset";
          return false;
        }

        for (size_t n = 0; n < exported_txs.m_ptx.size(); ++n) {
          tx_constructions.push_back(exported_txs.m_ptx[n].construction_data);
        }
      }
      catch (const std::exception &e) {
        er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
        er.message = "failed to parse multisig transfers: " + std::string(e.what());
        return false;
      }
    }

    std::vector<tools::wallet2::pending_tx> ptx;
    try
    {
      // gather info to ask the user
      std::unordered_map<cryptonote::account_public_address, std::pair<std::string, uint64_t>> dests;
      int first_known_non_zero_change_index = -1;
      for (size_t n = 0; n < tx_constructions.size(); ++n)
      {
        const tools::wallet2::tx_construction_data &cd = tx_constructions[n];
        res.desc.push_back({0, 0, std::numeric_limits<uint32_t>::max(), 0, {}, "", 0, "", 0, 0, ""});
        wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::transfer_description &desc = res.desc.back();

        std::vector<cryptonote::tx_extra_field> tx_extra_fields;
        bool has_encrypted_payment_id = false;
        crypto::hash8 payment_id8 = crypto::null_hash8;
        if (cryptonote::parse_tx_extra(cd.extra, tx_extra_fields))
        {
          cryptonote::tx_extra_nonce extra_nonce;
          if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
          {
            crypto::hash payment_id;
            if(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
            {
              if (payment_id8 != crypto::null_hash8)
              {
                desc.payment_id = epee::string_tools::pod_to_hex(payment_id8);
                has_encrypted_payment_id = true;
              }
            }
            else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
            {
              desc.payment_id = epee::string_tools::pod_to_hex(payment_id);
            }
          }
        }

        for (size_t s = 0; s < cd.sources.size(); ++s)
        {
          desc.amount_in += cd.sources[s].amount;
          size_t ring_size = cd.sources[s].outputs.size();
          if (ring_size < desc.ring_size)
            desc.ring_size = ring_size;
        }
        for (size_t d = 0; d < cd.splitted_dsts.size(); ++d)
        {
          const cryptonote::tx_destination_entry &entry = cd.splitted_dsts[d];
          std::string address = cryptonote::get_account_address_as_str(m_wallet->nettype(), entry.is_subaddress, entry.addr);
          if (has_encrypted_payment_id && !entry.is_subaddress && address != entry.original)
            address = cryptonote::get_account_integrated_address_as_str(m_wallet->nettype(), entry.addr, payment_id8);
          auto i = dests.find(entry.addr);
          if (i == dests.end())
            dests.insert(std::make_pair(entry.addr, std::make_pair(address, entry.amount)));
          else
            i->second.second += entry.amount;
          desc.amount_out += entry.amount;
        }
        if (cd.change_dts.amount > 0)
        {
          auto it = dests.find(cd.change_dts.addr);
          if (it == dests.end())
          {
            er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
            er.message = "Claimed change does not go to a paid address";
            return false;
          }
          if (it->second.second < cd.change_dts.amount)
          {
            er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
            er.message = "Claimed change is larger than payment to the change address";
            return false;
          }
          if (cd.change_dts.amount > 0)
          {
            if (first_known_non_zero_change_index == -1)
              first_known_non_zero_change_index = n;
            const tools::wallet2::tx_construction_data &cdn = tx_constructions[first_known_non_zero_change_index];
            if (memcmp(&cd.change_dts.addr, &cdn.change_dts.addr, sizeof(cd.change_dts.addr)))
            {
              er.code = WALLET_RPC_ERROR_CODE_BAD_UNSIGNED_TX_DATA;
              er.message = "Change goes to more than one address";
              return false;
            }
          }
          desc.change_amount += cd.change_dts.amount;
          it->second.second -= cd.change_dts.amount;
          if (it->second.second == 0)
            dests.erase(cd.change_dts.addr);
        }

        size_t n_dummy_outputs = 0;
        for (auto i = dests.begin(); i != dests.end(); )
        {
          if (i->second.second > 0)
          {
            desc.recipients.push_back({i->second.first, i->second.second});
          }
          else
            ++desc.dummy_outputs;
          ++i;
        }

        if (desc.change_amount > 0)
        {
          const tools::wallet2::tx_construction_data &cd0 = tx_constructions[0];
          desc.change_address = get_account_address_as_str(m_wallet->nettype(), cd0.subaddr_account > 0, cd0.change_dts.addr);
        }

        desc.fee = desc.amount_in - desc.amount_out;
        desc.unlock_time = cd.unlock_time;
        desc.extra = epee::to_hex::string({cd.extra.data(), cd.extra.size()});
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->key_on_device())
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

    std::vector<tools::wallet2::pending_tx> ptx_vector;
    try
    {
      bool r = m_wallet->parse_tx_from_str(blob, ptx_vector, NULL);
      if (!r)
      {
        er.code = WALLET_RPC_ERROR_CODE_BAD_SIGNED_TX_DATA;
        er.message = "Failed to parse signed tx data.";
        return false;
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_SIGNED_TX_DATA;
      er.message = std::string("Failed to parse signed tx: ") + e.what();
      return false;
    }

    try
    {
      for (auto &ptx: ptx_vector)
      {
        m_wallet->commit_tx(ptx);
        res.tx_hash_list.push_back(epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(ptx.tx)));
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_SIGNED_SUBMISSION;
      er.message = std::string("Failed to submit signed tx: ") + e.what();
      return false;
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_dust(const wallet_rpc::COMMAND_RPC_SWEEP_DUST::request& req, wallet_rpc::COMMAND_RPC_SWEEP_DUST::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_unmixable_sweep_transactions();

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash_list, req.get_tx_hex, res.tx_blob_list, req.get_tx_metadata, res.tx_metadata_list, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_all(const wallet_rpc::COMMAND_RPC_SWEEP_ALL::request& req, wallet_rpc::COMMAND_RPC_SWEEP_ALL::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    // validate the transfer requested and populate dsts & extra
    std::list<wallet_rpc::transfer_destination> destination;
    destination.push_back(wallet_rpc::transfer_destination());
    destination.back().amount = 0;
    destination.back().address = req.address;
    if (!validate_transfer(destination, req.payment_id, dsts, extra, true, er))
    {
      return false;
    }

    if (req.outputs < 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = "Amount of outputs should be greater than 0.";
      return  false;
    }

    try
    {
      uint64_t mixin = m_wallet->adjust_mixin(req.ring_size ? req.ring_size - 1 : 0);
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_all(req.below_amount, dsts[0].addr, dsts[0].is_subaddress, req.outputs, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices);

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash_list, req.get_tx_hex, res.tx_blob_list, req.get_tx_metadata, res.tx_metadata_list, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR);
      return false;
    }
    return true;
  }
//------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sweep_single(const wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::request& req, wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    if (req.outputs < 1)
    {
      er.code = WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE;
      er.message = "Amount of outputs should be greater than 0.";
      return  false;
    }

    // validate the transfer requested and populate dsts & extra
    std::list<wallet_rpc::transfer_destination> destination;
    destination.push_back(wallet_rpc::transfer_destination());
    destination.back().amount = 0;
    destination.back().address = req.address;
    if (!validate_transfer(destination, req.payment_id, dsts, extra, true, er))
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

    try
    {
      uint64_t mixin = m_wallet->adjust_mixin(req.ring_size ? req.ring_size - 1 : 0);
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_single(ki, dsts[0].addr, dsts[0].is_subaddress, req.outputs, mixin, req.unlock_time, priority, extra);

      if (ptx_vector.empty())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "No outputs found";
        return false;
      }
      if (ptx_vector.size() > 1)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Multiple transactions are created, which is not supposed to happen";
        return false;
      }
      const wallet2::pending_tx &ptx = ptx_vector[0];
      if (ptx.selected_transfers.size() > 1)
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "The transaction uses multiple inputs, which is not supposed to happen";
        return false;
      }

      return fill_response(ptx_vector, req.get_tx_key, res.tx_key, res.amount, res.fee, res.multisig_txset, res.unsigned_txset, req.do_not_relay,
          res.tx_hash, req.get_tx_hex, res.tx_blob, req.get_tx_metadata, res.tx_metadata, er);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR);
      return false;
    }
    catch (...)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_relay_tx(const wallet_rpc::COMMAND_RPC_RELAY_TX::request& req, wallet_rpc::COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    tools::wallet2::pending_tx ptx;
    try
    {
      std::istringstream iss(blob);
      boost::archive::portable_binary_iarchive ar(iss);
      ar >> ptx;
    }
    catch (...)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_TX_METADATA;
      er.message = "Failed to parse tx metadata.";
      return false;
    }

    try
    {
      m_wallet->commit_tx(ptx);
    }
    catch(const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR;
      er.message = "Failed to commit tx.";
      return false;
    }

    res.tx_hash = epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(ptx.tx));

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    try
    {
      crypto::hash8 payment_id;
      if (req.payment_id.empty())
      {
        payment_id = crypto::rand<crypto::hash8>();
      }
      else
      {
        if (!tools::wallet2::parse_short_payment_id(req.payment_id,payment_id))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Invalid payment ID";
          return false;
        }
      }

      if (req.standard_address.empty())
      {
        res.integrated_address = m_wallet->get_integrated_address_as_str(payment_id);
      }
      else
      {
        cryptonote::address_parse_info info;
        if(!get_account_address_from_str(info, m_wallet->nettype(), req.standard_address))
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
        if (req.payment_id.empty())
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Payment ID shouldn't be left unspecified";
          return false;
        }
        res.integrated_address = get_account_integrated_address_as_str(m_wallet->nettype(), info.address, payment_id);
      }
      res.payment_id = epee::string_tools::pod_to_hex(payment_id);
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
    if (!m_wallet) return not_open(er);
    try
    {
      cryptonote::address_parse_info info;

      if(!get_account_address_from_str(info, m_wallet->nettype(), req.integrated_address))
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
      res.standard_address = get_account_address_as_str(m_wallet->nettype(), info.is_subaddress, info.address);
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      m_wallet->store();
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
    if (!m_wallet) return not_open(er);
    crypto::hash payment_id;
    crypto::hash8 payment_id8;
    cryptonote::blobdata payment_id_blob;
    if(!epee::string_tools::parse_hexstr_to_binbuff(req.payment_id, payment_id_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
      er.message = "Payment ID has invalid format";
      return false;
    }

      if(sizeof(payment_id) == payment_id_blob.size())
      {
        payment_id = *reinterpret_cast<const crypto::hash*>(payment_id_blob.data());
      }
      else if(sizeof(payment_id8) == payment_id_blob.size())
      {
        payment_id8 = *reinterpret_cast<const crypto::hash8*>(payment_id_blob.data());
        memcpy(payment_id.data, payment_id8.data, 8);
        memset(payment_id.data + 8, 0, 24);
      }
      else
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid size: " + req.payment_id;
        return false;
      }

    res.payments.clear();
    std::list<wallet2::payment_details> payment_list;
    m_wallet->get_payments(payment_id, payment_list);
    for (auto & payment : payment_list)
    {
      wallet_rpc::payment_details rpc_payment;
      rpc_payment.payment_id   = req.payment_id;
      rpc_payment.tx_hash      = epee::string_tools::pod_to_hex(payment.m_tx_hash);
      rpc_payment.amount       = payment.m_amount;
      rpc_payment.block_height = payment.m_block_height;
      rpc_payment.unlock_time  = payment.m_unlock_time;
      rpc_payment.locked       = !m_wallet->is_transfer_unlocked(payment.m_unlock_time, payment.m_block_height);
      rpc_payment.subaddr_index = payment.m_subaddr_index;
      rpc_payment.address      = m_wallet->get_subaddress_as_str(payment.m_subaddr_index);
      res.payments.push_back(rpc_payment);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.payments.clear();
    if (!m_wallet) return not_open(er);

    /* If the payment ID list is empty, we get payments to any payment ID (or lack thereof) */
    if (req.payment_ids.empty())
    {
      std::list<std::pair<crypto::hash,wallet2::payment_details>> payment_list;
      m_wallet->get_payments(payment_list, req.min_block_height);

      for (auto & payment : payment_list)
      {
        wallet_rpc::payment_details rpc_payment;
        rpc_payment.payment_id   = epee::string_tools::pod_to_hex(payment.first);
        rpc_payment.tx_hash      = epee::string_tools::pod_to_hex(payment.second.m_tx_hash);
        rpc_payment.amount       = payment.second.m_amount;
        rpc_payment.block_height = payment.second.m_block_height;
        rpc_payment.unlock_time  = payment.second.m_unlock_time;
        rpc_payment.subaddr_index = payment.second.m_subaddr_index;
        rpc_payment.address      = m_wallet->get_subaddress_as_str(payment.second.m_subaddr_index);
        rpc_payment.locked       = !m_wallet->is_transfer_unlocked(payment.second.m_unlock_time, payment.second.m_block_height);
        res.payments.push_back(std::move(rpc_payment));
      }

      return true;
    }

    for (auto & payment_id_str : req.payment_ids)
    {
      crypto::hash payment_id;
      crypto::hash8 payment_id8;
      cryptonote::blobdata payment_id_blob;

      // TODO - should the whole thing fail because of one bad id?
      bool r;
      if (payment_id_str.size() == 2 * sizeof(payment_id))
      {
        r = epee::string_tools::hex_to_pod(payment_id_str, payment_id);
      }
      else if (payment_id_str.size() == 2 * sizeof(payment_id8))
      {
        r = epee::string_tools::hex_to_pod(payment_id_str, payment_id8);
        if (r)
        {
          memcpy(payment_id.data, payment_id8.data, 8);
          memset(payment_id.data + 8, 0, 24);
        }
      }
      else
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid size: " + payment_id_str;
        return false;
      }

      if(!r)
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid format: " + payment_id_str;
        return false;
      }

      std::list<wallet2::payment_details> payment_list;
      m_wallet->get_payments(payment_id, payment_list, req.min_block_height);

      for (auto & payment : payment_list)
      {
        wallet_rpc::payment_details rpc_payment;
        rpc_payment.payment_id   = payment_id_str;
        rpc_payment.tx_hash      = epee::string_tools::pod_to_hex(payment.m_tx_hash);
        rpc_payment.amount       = payment.m_amount;
        rpc_payment.block_height = payment.m_block_height;
        rpc_payment.unlock_time  = payment.m_unlock_time;
        rpc_payment.subaddr_index = payment.m_subaddr_index;
        rpc_payment.address      = m_wallet->get_subaddress_as_str(payment.m_subaddr_index);
        rpc_payment.locked       = !m_wallet->is_transfer_unlocked(payment.m_unlock_time, payment.m_block_height);
        res.payments.push_back(std::move(rpc_payment));
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
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

    wallet2::transfer_container transfers;
    m_wallet->get_transfers(transfers);

    for (const auto& td : transfers)
    {
      if (!filter || available != td.m_spent)
      {
        if (req.account_index != td.m_subaddr_index.major || (!req.subaddr_indices.empty() && req.subaddr_indices.count(td.m_subaddr_index.minor) == 0))
          continue;
        wallet_rpc::transfer_details rpc_transfers;
        rpc_transfers.amount       = td.amount();
        rpc_transfers.spent        = td.m_spent;
        rpc_transfers.global_index = td.m_global_output_index;
        rpc_transfers.tx_hash      = epee::string_tools::pod_to_hex(td.m_txid);
        rpc_transfers.subaddr_index = {td.m_subaddr_index.major, td.m_subaddr_index.minor};
        rpc_transfers.key_image    = td.m_key_image_known ? epee::string_tools::pod_to_hex(td.m_key_image) : "";
        rpc_transfers.block_height = td.m_block_height;
        rpc_transfers.frozen       = td.m_frozen;
        rpc_transfers.unlocked     = m_wallet->is_transfer_unlocked(td);
        res.transfers.push_back(rpc_transfers);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_query_key(const wallet_rpc::COMMAND_RPC_QUERY_KEY::request& req, wallet_rpc::COMMAND_RPC_QUERY_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
      if (!m_wallet) return not_open(er);
      if (m_restricted)
      {
        er.code = WALLET_RPC_ERROR_CODE_DENIED;
        er.message = "Command unavailable in restricted mode.";
        return false;
      }

      if (req.key_type.compare("mnemonic") == 0)
      {
        epee::wipeable_string seed;
        bool ready;
        if (m_wallet->multisig(&ready))
        {
          if (!ready)
          {
            er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
            er.message = "This wallet is multisig, but not yet finalized";
            return false;
          }
          if (!m_wallet->get_multisig_seed(seed))
          {
            er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
            er.message = "Failed to get multisig seed.";
            return false;
          }
        }
        else
        {
          if (m_wallet->watch_only())
          {
            er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
            er.message = "The wallet is watch-only. Cannot retrieve seed.";
            return false;
          }
          if (!m_wallet->is_deterministic())
          {
            er.code = WALLET_RPC_ERROR_CODE_NON_DETERMINISTIC;
            er.message = "The wallet is non-deterministic. Cannot display seed.";
            return false;
          }
          if (!m_wallet->get_seed(seed))
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
          epee::wipeable_string key = epee::to_hex::wipeable_string(m_wallet->get_account().get_keys().m_view_secret_key);
          res.key = std::string(key.data(), key.size());
      }
      else if(req.key_type.compare("spend_key") == 0)
      {
          if (m_wallet->watch_only())
          {
            er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
            er.message = "The wallet is watch-only. Cannot retrieve spend key.";
            return false;
          }
          epee::wipeable_string key = epee::to_hex::wipeable_string(m_wallet->get_account().get_keys().m_spend_secret_key);
          res.key = std::string(key.data(), key.size());
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      m_wallet->rescan_blockchain(req.hard);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign(const wallet_rpc::COMMAND_RPC_SIGN::request& req, wallet_rpc::COMMAND_RPC_SIGN::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    res.signature = m_wallet->sign(req.data);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_verify(const wallet_rpc::COMMAND_RPC_VERIFY::request& req, wallet_rpc::COMMAND_RPC_VERIFY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    cryptonote::address_parse_info info;
    er.message = "";
    if(!get_account_address_from_str_or_url(info, m_wallet->nettype(), req.address,
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

    res.good = m_wallet->verify(req.data, info.address, req.signature);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_stop_wallet(const wallet_rpc::COMMAND_RPC_STOP_WALLET::request& req, wallet_rpc::COMMAND_RPC_STOP_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      m_wallet->store();
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

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

      crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_blob.data());
      txids.push_back(txid);
    }

    std::list<crypto::hash>::const_iterator il = txids.begin();
    std::list<std::string>::const_iterator in = req.notes.begin();
    while (il != txids.end())
    {
      m_wallet->set_tx_note(*il++, *in++);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_tx_notes(const wallet_rpc::COMMAND_RPC_GET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_GET_TX_NOTES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.notes.clear();
    if (!m_wallet) return not_open(er);

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

      crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_blob.data());
      txids.push_back(txid);
    }

    std::list<crypto::hash>::const_iterator il = txids.begin();
    while (il != txids.end())
    {
      res.notes.push_back(m_wallet->get_tx_note(*il++));
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_set_attribute(const wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    m_wallet->set_attribute(req.key, req.value);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_attribute(const wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    if (!m_wallet->get_attribute(req.key, res.value))
    {
      er.code = WALLET_RPC_ERROR_CODE_ATTRIBUTE_NOT_FOUND;
      er.message = "Attribute not found.";
      return false;
    }
    return true;
  }
  bool wallet_rpc_server::on_get_tx_key(const wallet_rpc::COMMAND_RPC_GET_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_GET_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    if (!m_wallet->get_tx_key(txid, tx_key, additional_tx_keys))
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_TXKEY;
      er.message = "No tx secret key is stored for this tx";
      return false;
    }

    epee::wipeable_string s;
    s += epee::to_hex::wipeable_string(tx_key);
    for (size_t i = 0; i < additional_tx_keys.size(); ++i)
      s += epee::to_hex::wipeable_string(additional_tx_keys[i]);
    res.tx_key = std::string(s.data(), s.size());
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_tx_key(const wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);

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
    if(!get_account_address_from_str(info, m_wallet->nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      m_wallet->check_tx_key(txid, tx_key, additional_tx_keys, info.address, res.received, res.in_pool, res.confirmations);
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
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_wallet->nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      res.signature = m_wallet->get_tx_proof(txid, info.address, info.is_subaddress, req.message);
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
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_wallet->nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_ADDRESS;
      er.message = "Invalid address";
      return false;
    }

    try
    {
      res.good = m_wallet->check_tx_proof(txid, info.address, info.is_subaddress, req.message, req.signature, res.received, res.in_pool, res.confirmations);
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
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    try
    {
      res.signature = m_wallet->get_spend_proof(txid, req.message);
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
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    try
    {
      res.good = m_wallet->check_spend_proof(txid, req.message, req.signature);
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
    if (!m_wallet) return not_open(er);

    boost::optional<std::pair<uint32_t, uint64_t>> account_minreserve;
    if (!req.all)
    {
      if (req.account_index >= m_wallet->get_num_subaddress_accounts())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Account index is out of bound";
        return false;
      }
      account_minreserve = std::make_pair(req.account_index, req.amount);
    }

    try
    {
      res.signature = m_wallet->get_reserve_proof(account_minreserve, req.message);
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
    if (!m_wallet) return not_open(er);

    cryptonote::address_parse_info info;
    if (!get_account_address_from_str(info, m_wallet->nettype(), req.address))
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
      res.good = m_wallet->check_reserve_proof(info.address, req.message, req.signature, res.total, res.spent);
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

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

    if (req.in)
    {
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
      m_wallet->get_payments(payments, min_height, max_height, account_index, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.in.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.in.back(), i->second.m_tx_hash, i->first, i->second);
      }
    }

    if (req.out)
    {
      std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments;
      m_wallet->get_payments_out(payments, min_height, max_height, account_index, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.out.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.out.back(), i->first, i->second);
      }
    }

    if (req.pending || req.failed) {
      std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
      m_wallet->get_unconfirmed_payments_out(upayments, account_index, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
        const tools::wallet2::unconfirmed_transfer_details &pd = i->second;
        bool is_failed = pd.m_state == tools::wallet2::unconfirmed_transfer_details::failed;
        if (!((req.failed && is_failed) || (!is_failed && req.pending)))
          continue;
        std::list<wallet_rpc::transfer_entry> &entries = is_failed ? res.failed : res.pending;
        entries.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(entries.back(), i->first, i->second);
      }
    }

    if (req.pool)
    {
      std::vector<std::pair<cryptonote::transaction, bool>> process_txs;
      m_wallet->update_pool_state(process_txs);
      if (!process_txs.empty())
        m_wallet->process_pool_state(process_txs);

      std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> payments;
      m_wallet->get_unconfirmed_payments(payments, account_index, subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.pool.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.pool.back(), i->first, i->second);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_transfer_by_txid(const wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    crypto::hash txid;
    cryptonote::blobdata txid_blob;
    if(!epee::string_tools::parse_hexstr_to_binbuff(req.txid, txid_blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "Transaction ID has invalid format";
      return false;
    }

    if(sizeof(txid) == txid_blob.size())
    {
      txid = *reinterpret_cast<const crypto::hash*>(txid_blob.data());
    }
    else
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "Transaction ID has invalid size: " + req.txid;
      return false;
    }

    if (req.account_index >= m_wallet->get_num_subaddress_accounts())
    {
      er.code = WALLET_RPC_ERROR_CODE_ACCOUNT_INDEX_OUT_OF_BOUNDS;
      er.message = "Account index is out of bound";
      return false;
    }

    std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
    m_wallet->get_payments(payments, 0, (uint64_t)-1, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
      if (i->second.m_tx_hash == txid)
      {
        res.transfers.resize(res.transfers.size() + 1);
        fill_transfer_entry(res.transfers.back(), i->second.m_tx_hash, i->first, i->second);
      }
    }

    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments_out;
    m_wallet->get_payments_out(payments_out, 0, (uint64_t)-1, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments_out.begin(); i != payments_out.end(); ++i) {
      if (i->first == txid)
      {
        res.transfers.resize(res.transfers.size() + 1);
        fill_transfer_entry(res.transfers.back(), i->first, i->second);
      }
    }

    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      if (i->first == txid)
      {
        res.transfers.resize(res.transfers.size() + 1);
        fill_transfer_entry(res.transfers.back(), i->first, i->second);
      }
    }

    std::vector<std::pair<cryptonote::transaction, bool>> process_txs;
    m_wallet->update_pool_state(process_txs);
    if (!process_txs.empty())
      m_wallet->process_pool_state(process_txs);

    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pool_payments;
    m_wallet->get_unconfirmed_payments(pool_payments, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = pool_payments.begin(); i != pool_payments.end(); ++i) {
      if (i->second.m_pd.m_tx_hash == txid)
      {
        res.transfers.resize(res.transfers.size() + 1);
        fill_transfer_entry(res.transfers.back(), i->first, i->second);
      }
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->key_on_device())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }

    try
    {
      res.outputs_data_hex = epee::string_tools::buff_to_hex_nodelimer(m_wallet->export_outputs_to_str(req.all));
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->key_on_device())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "command not supported by HW wallet";
      return false;
    }

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.outputs_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    try
    {
      res.num_imported = m_wallet->import_outputs_from_str(blob);
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
    if (!m_wallet) return not_open(er);
    try
    {
      std::pair<size_t, std::vector<std::pair<crypto::key_image, crypto::signature>>> ski = m_wallet->export_key_images(req.all);
      res.offset = ski.first;
      res.signed_key_images.resize(ski.second.size());
      for (size_t n = 0; n < ski.second.size(); ++n)
      {
         res.signed_key_images[n].key_image = epee::string_tools::pod_to_hex(ski.second[n].first);
         res.signed_key_images[n].signature = epee::string_tools::pod_to_hex(ski.second[n].second);
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_wallet->is_trusted_daemon())
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "This command requires a trusted daemon.";
      return false;
    }
    try
    {
      std::vector<std::pair<crypto::key_image, crypto::signature>> ski;
      ski.resize(req.signed_key_images.size());
      for (size_t n = 0; n < ski.size(); ++n)
      {
        if (!epee::string_tools::hex_to_pod(req.signed_key_images[n].key_image, ski[n].first))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
          er.message = "failed to parse key image";
          return false;
        }

        if (!epee::string_tools::hex_to_pod(req.signed_key_images[n].signature, ski[n].second))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_SIGNATURE;
          er.message = "failed to parse signature";
          return false;
        }
      }
      uint64_t spent = 0, unspent = 0;
      uint64_t height = m_wallet->import_key_images(ski, req.offset, spent, unspent);
      res.spent = spent;
      res.unspent = unspent;
      res.height = height;
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
    if (!m_wallet) return not_open(er);
    std::string error;
    std::string uri = m_wallet->make_uri(req.address, req.payment_id, req.amount, req.tx_description, req.recipient_name, error);
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
    if (!m_wallet) return not_open(er);
    std::string error;
    if (!m_wallet->parse_uri(req.uri, res.uri.address, res.uri.payment_id, res.uri.amount, res.uri.tx_description, res.uri.recipient_name, res.unknown_parameters, error))
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
    if (!m_wallet) return not_open(er);
    const auto ab = m_wallet->get_address_book();
    if (req.entries.empty())
    {
      uint64_t idx = 0;
      for (const auto &entry: ab)
        res.entries.push_back(wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::entry{idx++, get_account_address_as_str(m_wallet->nettype(), entry.m_is_subaddress, entry.m_address), epee::string_tools::pod_to_hex(entry.m_payment_id), entry.m_description});
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
        res.entries.push_back(wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::entry{idx, get_account_address_as_str(m_wallet->nettype(), entry.m_is_subaddress, entry.m_address), epee::string_tools::pod_to_hex(entry.m_payment_id), entry.m_description});
      }
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_add_address_book(const wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    cryptonote::address_parse_info info;
    crypto::hash payment_id = crypto::null_hash;
    er.message = "";
    if(!get_account_address_from_str_or_url(info, m_wallet->nettype(), req.address,
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
        er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + req.address;
      return false;
    }
    if (info.has_payment_id)
    {
      memcpy(payment_id.data, info.payment_id.data, 8);
      memset(payment_id.data + 8, 0, 24);
    }
    if (!req.payment_id.empty())
    {
      if (info.has_payment_id)
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Separate payment ID given with integrated address";
        return false;
      }

      crypto::hash long_payment_id;

      if (!wallet2::parse_long_payment_id(req.payment_id, payment_id))
      {
        if (!wallet2::parse_short_payment_id(req.payment_id, info.payment_id))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Payment id has invalid format: \"" + req.payment_id + "\", expected 64 character string";
          return false;
        }
        else
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Payment id has invalid format: standalone short payment IDs are forbidden, they must be part of an integrated address";
          return false;
        }
      }
    }
    if (!m_wallet->add_address_book_row(info.address, payment_id, req.description, info.is_subaddress))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to add address book entry";
      return false;
    }
    res.index = m_wallet->get_address_book().size() - 1;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_edit_address_book(const wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    const auto ab = m_wallet->get_address_book();
    if (req.index >= ab.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_INDEX;
      er.message = "Index out of range: " + std::to_string(req.index);
      return false;
    }

    tools::wallet2::address_book_row entry = ab[req.index];

    cryptonote::address_parse_info info;
    crypto::hash payment_id = crypto::null_hash;
    if (req.set_address)
    {
      er.message = "";
      if(!get_account_address_from_str_or_url(info, m_wallet->nettype(), req.address,
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
          er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + req.address;
        return false;
      }
      entry.m_address = info.address;
      entry.m_is_subaddress = info.is_subaddress;
      if (info.has_payment_id)
      {
        memcpy(entry.m_payment_id.data, info.payment_id.data, 8);
        memset(entry.m_payment_id.data + 8, 0, 24);
      }
    }

    if (req.set_payment_id)
    {
      if (req.payment_id.empty())
      {
        payment_id = crypto::null_hash;
      }
      else
      {
        if (req.set_address && info.has_payment_id)
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Separate payment ID given with integrated address";
          return false;
        }

        if (!wallet2::parse_long_payment_id(req.payment_id, payment_id))
        {
          crypto::hash8 spid;
          if (!wallet2::parse_short_payment_id(req.payment_id, spid))
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = "Payment id has invalid format: \"" + req.payment_id + "\", expected 64 character string";
            return false;
          }
          else
          {
            er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
            er.message = "Payment id has invalid format: standalone short payment IDs are forbidden, they must be part of an integrated address";
            return false;
          }
        }
      }

      entry.m_payment_id = payment_id;
    }

    if (req.set_description)
      entry.m_description = req.description;

    if (!m_wallet->set_address_book_row(req.index, entry.m_address, entry.m_payment_id, entry.m_description, entry.m_is_subaddress))
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    const auto ab = m_wallet->get_address_book();
    if (req.index >= ab.size())
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_INDEX;
      er.message = "Index out of range: " + std::to_string(req.index);
      return false;
    }
    if (!m_wallet->delete_address_book_row(req.index))
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    try
    {
      m_wallet->refresh(m_wallet->is_trusted_daemon(), req.start_height, res.blocks_fetched, res.received_money);
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
      m_auto_refresh_period = req.enable ? req.period ? req.period : DEFAULT_AUTO_REFRESH_PERIOD : 0;
      MINFO("Auto refresh now " << (m_auto_refresh_period ? std::to_string(m_auto_refresh_period) + " seconds" : std::string("disabled")));
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
  bool wallet_rpc_server::on_rescan_spent(const wallet_rpc::COMMAND_RPC_RESCAN_SPENT::request& req, wallet_rpc::COMMAND_RPC_RESCAN_SPENT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    try
    {
      m_wallet->rescan_spent();
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
    if (!m_wallet) return not_open(er);
    if (!m_wallet->is_trusted_daemon())
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

    cryptonote::COMMAND_RPC_START_MINING::request daemon_req = AUTO_VAL_INIT(daemon_req); 
    daemon_req.miner_address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    daemon_req.threads_count        = req.threads_count;
    daemon_req.do_background_mining = req.do_background_mining;
    daemon_req.ignore_battery       = req.ignore_battery;

    cryptonote::COMMAND_RPC_START_MINING::response daemon_res;
    bool r = m_wallet->invoke_http_json("/start_mining", daemon_req, daemon_res);
    if (!r || daemon_res.status != CORE_RPC_STATUS_OK)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Couldn't start mining due to unknown error.";
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_stop_mining(const wallet_rpc::COMMAND_RPC_STOP_MINING::request& req, wallet_rpc::COMMAND_RPC_STOP_MINING::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    cryptonote::COMMAND_RPC_STOP_MINING::request daemon_req;
    cryptonote::COMMAND_RPC_STOP_MINING::response daemon_res;
    bool r = m_wallet->invoke_http_json("/stop_mining", daemon_req, daemon_res);
    if (!r || daemon_res.status != CORE_RPC_STATUS_OK)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Couldn't stop mining due to unknown error.";
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
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    namespace po = boost::program_options;
    po::variables_map vm2;
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
      std::vector<std::string> languages;
      crypto::ElectrumWords::get_language_list(languages, false);
      std::vector<std::string>::iterator it;

      it = std::find(languages.begin(), languages.end(), req.language);
      if (it == languages.end())
      {
        crypto::ElectrumWords::get_language_list(languages, true);
        it = std::find(languages.begin(), languages.end(), req.language);
      }
      if (it == languages.end())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Unknown language: " + req.language;
        return false;
      }
    }
    {
      po::options_description desc("dummy");
      const command_line::arg_descriptor<std::string, true> arg_password = {"password", "password"};
      const char *argv[4];
      int argc = 3;
      argv[0] = "wallet-rpc";
      argv[1] = "--password";
      argv[2] = req.password.c_str();
      argv[3] = NULL;
      vm2 = *m_vm;
      command_line::add_arg(desc, arg_password);
      po::store(po::parse_command_line(argc, argv, desc), vm2);
    }
    std::unique_ptr<tools::wallet2> wal = tools::wallet2::make_new(vm2, true, nullptr).first;
    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to create wallet";
      return false;
    }
    wal->set_seed_language(req.language);
    cryptonote::COMMAND_RPC_GET_HEIGHT::request hreq;
    cryptonote::COMMAND_RPC_GET_HEIGHT::response hres;
    hres.height = 0;
    bool r = wal->invoke_http_json("/getheight", hreq, hres);
    if (r)
      wal->set_refresh_from_block_height(hres.height);
    crypto::secret_key dummy_key;
    try {
      wal->generate(wallet_file, req.password, dummy_key, false, false);
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to generate wallet";
      return false;
    }

    if (m_wallet)
    {
      try
      {
        m_wallet->store();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
      delete m_wallet;
    }
    m_wallet = wal.release();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_open_wallet(const wallet_rpc::COMMAND_RPC_OPEN_WALLET::request& req, wallet_rpc::COMMAND_RPC_OPEN_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (m_wallet_dir.empty())
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_WALLET_DIR;
      er.message = "No wallet dir configured";
      return false;
    }

    namespace po = boost::program_options;
    po::variables_map vm2;
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
    if (m_wallet && req.autosave_current)
    {
      try
      {
        m_wallet->store();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    std::string wallet_file = m_wallet_dir + "/" + req.filename;
    {
      po::options_description desc("dummy");
      const command_line::arg_descriptor<std::string, true> arg_password = {"password", "password"};
      const char *argv[4];
      int argc = 3;
      argv[0] = "wallet-rpc";
      argv[1] = "--password";
      argv[2] = req.password.c_str();
      argv[3] = NULL;
      vm2 = *m_vm;
      command_line::add_arg(desc, arg_password);
      po::store(po::parse_command_line(argc, argv, desc), vm2);
    }
    std::unique_ptr<tools::wallet2> wal = nullptr;
    try {
      wal = tools::wallet2::make_from_file(vm2, true, wallet_file, nullptr).first;
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
    }
    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to open wallet";
      return false;
    }

    if (m_wallet)
      delete m_wallet;
    m_wallet = wal.release();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_close_wallet(const wallet_rpc::COMMAND_RPC_CLOSE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CLOSE_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);

    if (req.autosave_current)
    {
      try
      {
        m_wallet->store();
      }
      catch (const std::exception& e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }
    delete m_wallet;
    m_wallet = NULL;
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_change_wallet_password(const wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::request& req, wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->verify_password(req.old_password))
    {
      try
      {
        m_wallet->change_password(m_wallet->get_wallet_file(), req.old_password, req.new_password);
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
      er.message = e.what();
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
    catch (const error::no_connection_to_bitmessage& e)
    {
        er.code = WALLET_RPC_ERROR_CODE_MMS_NO_BITMESSAGE_CONNECTION;
        er.message = e.what();
    }
    catch (const error::bitmessage_api_error& e)
    {
        er.code = WALLET_RPC_ERROR_CODE_MMS_BITMESSAGE_API_ERROR;
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

    namespace po = boost::program_options;
    po::variables_map vm2;
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

    {
      po::options_description desc("dummy");
      const command_line::arg_descriptor<std::string, true> arg_password = {"password", "password"};
      const char *argv[4];
      int argc = 3;
      argv[0] = "wallet-rpc";
      argv[1] = "--password";
      argv[2] = req.password.c_str();
      argv[3] = NULL;
      vm2 = *m_vm;
      command_line::add_arg(desc, arg_password);
      po::store(po::parse_command_line(argc, argv, desc), vm2);
    }

    auto rc = tools::wallet2::make_new(vm2, true, nullptr);
    std::unique_ptr<wallet2> wal;
    wal = std::move(rc.first);
    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to create wallet";
      return false;
    }

    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, wal->nettype(), req.address))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to parse public address";
      return false;
    }

    epee::wipeable_string password = rc.second.password();
    epee::wipeable_string viewkey_string = req.viewkey;
    crypto::secret_key viewkey;
    if (!viewkey_string.hex_to_pod(unwrap(unwrap(viewkey))))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to parse view key secret key";
      return false;
    }

    if (m_wallet && req.autosave_current)
    {
      try
      {
        if (!wallet_file.empty())
          m_wallet->store();
      }
      catch (const std::exception &e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }

    try
    {
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
        wal->generate(wallet_file, std::move(rc.second).password(), info.address, spendkey, viewkey, false);
        res.info = "Wallet has been generated successfully.";
      }
      else
      {
        wal->generate(wallet_file, std::move(rc.second).password(), info.address, viewkey, false);
        res.info = "Watch-only wallet has been generated successfully.";
      }
      MINFO("Wallet has been generated.\n");
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to generate wallet";
      return false;
    }

    // set blockheight if given
    try
    {
      wal->set_refresh_from_block_height(req.restore_height);
      wal->rewrite(wallet_file, password);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    if (m_wallet)
      delete m_wallet;
    m_wallet = wal.release();
    res.address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_restore_deterministic_wallet(const wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::request &req, wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::response &res, epee::json_rpc::error &er, const connection_context *ctx)
  {
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

    namespace po = boost::program_options;
    po::variables_map vm2;
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
    {
      if (!crypto::ElectrumWords::words_to_bytes(req.seed, recovery_key, old_language))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Electrum-style word list failed verification";
        return false;
      }
    }
    if (m_wallet && req.autosave_current)
    {
      try
      {
        m_wallet->store();
      }
      catch (const std::exception &e)
      {
        handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
        return false;
      }
    }

    // process seed_offset if given
    {
      if (!req.seed_offset.empty())
      {
        recovery_key = cryptonote::decrypt_key(recovery_key, req.seed_offset);
      }
    }
    {
      po::options_description desc("dummy");
      const command_line::arg_descriptor<std::string, true> arg_password = {"password", "password"};
      const char *argv[4];
      int argc = 3;
      argv[0] = "wallet-rpc";
      argv[1] = "--password";
      argv[2] = req.password.c_str();
      argv[3] = NULL;
      vm2 = *m_vm;
      command_line::add_arg(desc, arg_password);
      po::store(po::parse_command_line(argc, argv, desc), vm2);
    }

    auto rc = tools::wallet2::make_new(vm2, true, nullptr);
    std::unique_ptr<wallet2> wal;
    wal = std::move(rc.first);
    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to create wallet";
      return false;
    }

    epee::wipeable_string password = rc.second.password();

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
      std::vector<std::string> language_list;
      std::vector<std::string> language_list_en;
      crypto::ElectrumWords::get_language_list(language_list);
      crypto::ElectrumWords::get_language_list(language_list_en, true);
      if (std::find(language_list.begin(), language_list.end(), req.language) == language_list.end() &&
          std::find(language_list_en.begin(), language_list_en.end(), req.language) == language_list_en.end())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Wallet was using the old seed language, and the specified new seed language is invalid.";
        return false;
      }
      mnemonic_language = req.language;
    }

    wal->set_seed_language(mnemonic_language);

    crypto::secret_key recovery_val;
    try
    {
      recovery_val = wal->generate(wallet_file, std::move(rc.second).password(), recovery_key, true, false, false);
      MINFO("Wallet has been restored.\n");
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    // // Convert the secret key back to seed
    epee::wipeable_string electrum_words;
    if (!crypto::ElectrumWords::bytes_to_words(recovery_val, electrum_words, mnemonic_language))
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to encode seed";
      return false;
    }
    res.seed = std::string(electrum_words.data(), electrum_words.size());

    if (!wal)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Failed to generate wallet";
      return false;
    }

    // set blockheight if given
    try
    {
      wal->set_refresh_from_block_height(req.restore_height);
      wal->rewrite(wallet_file, password);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }

    if (m_wallet)
      delete m_wallet;
    m_wallet = wal.release();
    res.address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
    res.info = "Wallet has been restored successfully.";
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_is_multisig(const wallet_rpc::COMMAND_RPC_IS_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IS_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    res.multisig = m_wallet->multisig(&res.ready, &res.threshold, &res.total);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_prepare_multisig(const wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->multisig())
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is already multisig";
      return false;
    }
    if (m_wallet->watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "wallet is watch-only and cannot be made multisig";
      return false;
    }

    res.multisig_info = m_wallet->get_multisig_info();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_make_multisig(const wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (m_wallet->multisig())
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is already multisig";
      return false;
    }
    if (m_wallet->watch_only())
    {
      er.code = WALLET_RPC_ERROR_CODE_WATCH_ONLY;
      er.message = "wallet is watch-only and cannot be made multisig";
      return false;
    }

    try
    {
      res.multisig_info = m_wallet->make_multisig(req.password, req.multisig_info, req.threshold);
      res.address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    if (!m_wallet->multisig(&ready))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }

    cryptonote::blobdata info;
    try
    {
      info = m_wallet->export_multisig();
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = e.what();
      return false;
    }

    res.info = epee::string_tools::buff_to_hex_nodelimer(info);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_import_multisig(const wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }

    if (req.info.size() < threshold - 1)
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
      res.n_outputs = m_wallet->import_multisig(info);
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = "Error calling import_multisig";
      return false;
    }

    if (m_wallet->is_trusted_daemon())
    {
      try
      {
        m_wallet->rescan_spent();
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

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_finalize_multisig(const wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is multisig, and already finalized";
      return false;
    }

    if (req.multisig_info.size() < 1 || req.multisig_info.size() > total)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Needs multisig info from more participants";
      return false;
    }

    try
    {
      if (!m_wallet->finalize_multisig(req.password, req.multisig_info))
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Error calling finalize_multisig";
        return false;
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
      er.message = std::string("Error calling finalize_multisig: ") + e.what();
      return false;
    }
    res.address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_exchange_multisig_keys(const wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::request& req, wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }

    if (ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG;
      er.message = "This wallet is multisig, and already finalized";
      return false;
    }

    if (req.multisig_info.size() < 1 || req.multisig_info.size() > total)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Needs multisig info from more participants";
      return false;
    }

    try
    {
      res.multisig_info = m_wallet->exchange_multisig_keys(req.password, req.multisig_info);
      if (res.multisig_info.empty())
      {
        res.address = m_wallet->get_account().get_public_address_str(m_wallet->nettype());
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
  bool wallet_rpc_server::on_sign_multisig(const wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.tx_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    tools::wallet2::multisig_tx_set txs;
    bool r = m_wallet->load_multisig_tx(blob, txs, NULL);
    if (!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
      er.message = "Failed to parse multisig tx data.";
      return false;
    }

    std::vector<crypto::hash> txids;
    try
    {
      bool r = m_wallet->sign_multisig_tx(txs, txids);
      if (!r)
      {
        er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SIGNATURE;
        er.message = "Failed to sign multisig tx";
        return false;
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SIGNATURE;
      er.message = std::string("Failed to sign multisig tx: ") + e.what();
      return false;
    }

    res.tx_data_hex = epee::string_tools::buff_to_hex_nodelimer(m_wallet->save_multisig_tx(txs));
    if (!txids.empty())
    {
      for (const crypto::hash &txid: txids)
        res.tx_hash_list.push_back(epee::string_tools::pod_to_hex(txid));
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_submit_multisig(const wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    bool ready;
    uint32_t threshold, total;
    if (!m_wallet->multisig(&ready, &threshold, &total))
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is not multisig";
      return false;
    }
    if (!ready)
    {
      er.code = WALLET_RPC_ERROR_CODE_NOT_MULTISIG;
      er.message = "This wallet is multisig, but not yet finalized";
      return false;
    }

    cryptonote::blobdata blob;
    if (!epee::string_tools::parse_hexstr_to_binbuff(req.tx_data_hex, blob))
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_HEX;
      er.message = "Failed to parse hex.";
      return false;
    }

    tools::wallet2::multisig_tx_set txs;
    bool r = m_wallet->load_multisig_tx(blob, txs, NULL);
    if (!r)
    {
      er.code = WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA;
      er.message = "Failed to parse multisig tx data.";
      return false;
    }

    if (txs.m_signers.size() < threshold)
    {
      er.code = WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED;
      er.message = "Not enough signers signed this transaction.";
      return false;
    }

    try
    {
      for (auto &ptx: txs.m_ptx)
      {
        m_wallet->commit_tx(ptx);
        res.tx_hash_list.push_back(epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(ptx.tx)));
      }
    }
    catch (const std::exception &e)
    {
      er.code = WALLET_RPC_ERROR_CODE_MULTISIG_SUBMISSION;
      er.message = std::string("Failed to submit multisig tx: ") + e.what();
      return false;
    }

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
    if (!req.any_net_type && !m_wallet) return not_open(er);
    for (const auto &net_type: net_types)
    {
      if (!req.any_net_type && (!m_wallet || net_type.type != m_wallet->nettype()))
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
    if (!m_wallet) return not_open(er);
    if (m_restricted)
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
   
    std::vector<std::vector<uint8_t>> ssl_allowed_fingerprints;
    ssl_allowed_fingerprints.reserve(req.ssl_allowed_fingerprints.size());
    for (const std::string &fp: req.ssl_allowed_fingerprints)
    {
      ssl_allowed_fingerprints.push_back({});
      std::vector<uint8_t> &v = ssl_allowed_fingerprints.back();
      for (auto c: fp)
        v.push_back(c);
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

    ssl_options.auth = epee::net_utils::ssl_authentication_t{
      std::move(req.ssl_private_key_path), std::move(req.ssl_certificate_path)
    };

    const bool verification_required =
      ssl_options.verification != epee::net_utils::ssl_verification_t::none &&
      ssl_options.support == epee::net_utils::ssl_support_t::e_ssl_support_enabled;

    if (verification_required && !ssl_options.has_strong_verification(boost::string_ref{}))
    {
      er.code = WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION;
      er.message = "SSL is enabled but no user certificate or fingerprints were provided";
      return false;
    }

    if (!m_wallet->set_daemon(req.address, boost::none, req.trusted, std::move(ssl_options)))
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
  bool wallet_rpc_server::on_get_version(const wallet_rpc::COMMAND_RPC_GET_VERSION::request& req, wallet_rpc::COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    res.version = WALLET_RPC_VERSION;
    res.release = MONERO_VERSION_IS_RELEASE;
    return true;
  }

  // MMS -------------------------------------------------------------------------------------------------------------------------

  const char* network_type_to_string(cryptonote::network_type type)
  {
    switch (type)
    {
    case cryptonote::MAINNET:
        return "mainnet";
      case cryptonote::TESTNET:
        return "testnet";
      case cryptonote::STAGENET:
        return "stagenet";
      default:
        return "unknown_network_type";
    }
  }

  const char* mms_message_type_to_string(mms::message_type type)
  {
    switch (type)
    {
      case mms::message_type::key_set:
        return "key_set";
      case mms::message_type::additional_key_set:
        return "additional_key_set";
      case mms::message_type::multisig_sync_data:
        return "multisig_sync_data";
      case mms::message_type::partially_signed_tx:
        return "partially_signed_tx";
      case mms::message_type::fully_signed_tx:
        return "fully_signed_tx";
      case mms::message_type::note:
        return "note";
      case mms::message_type::signer_config:
        return "signer_config";
      case mms::message_type::auto_config_data:
        return "auto_config_data";
      default:
        // Should of course not happen, but do not let everything crash and burn just
        // because "message_type" was extended, but this code here did not yet get
        // adjusted (same "philosophy" is used by the other similar methods here):
        return "unknown_message_type";
    }
  }

  bool string_to_mms_message_type(const std::string &s, mms::message_type &type)
  {
    if (s == "key_set")
    {
      type = mms::message_type::key_set;
      return true;
    }
    else if (s == "additional_key_set")
    {
      type = mms::message_type::additional_key_set;
      return true;
    }
    else if (s == "multisig_sync_data")
    {
      type = mms::message_type::multisig_sync_data;
      return true;
    }
    else if (s == "partially_signed_tx")
    {
      type = mms::message_type::partially_signed_tx;
      return true;
    }
    else if (s == "fully_signed_tx")
    {
      type = mms::message_type::fully_signed_tx;
      return true;
    }
    else if (s == "note")
    {
      type = mms::message_type::note;
      return true;
    }
    else if (s == "signer_config")
    {
      type = mms::message_type::signer_config;
      return true;
    }
    else if (s == "auto_config_data")
    {
      type = mms::message_type::auto_config_data;
      return true;
    }
    else
    {
      return false;
    }
  }

  const char* mms_message_direction_to_string(mms::message_direction direction)
  {
    switch (direction)
    {
      case mms::message_direction::in:
        return "in";
      case mms::message_direction::out:
        return "out";
      default:
        return "unknown_message_direction";
    }
  }

  bool string_to_mms_message_direction(const std::string &s, mms::message_direction &direction)
  {
    if (s == "in")
    {
      direction = mms::message_direction::in;
      return true;
    }
    else if (s == "out")
    {
      direction = mms::message_direction::out;
      return true;
    }
    else
    {
      return false;
    }
  }

  const char* mms_message_state_to_string(mms::message_state state)
  {
    switch (state)
    {
      case mms::message_state::ready_to_send:
        return "ready_to_send";
      case mms::message_state::sent:
        return "sent";
      case mms::message_state::waiting:
        return "waiting";
      case mms::message_state::processed:
        return "processed";
      case mms::message_state::cancelled:
        return "cancelled";
      default:
        return "unknown_message_state";
    }
  }

  const char* mms_message_processing_to_string(mms::message_processing processing)
  {
    switch (processing)
    {
      case mms::message_processing::prepare_multisig:
        return "prepare_multisig";
      case mms::message_processing::make_multisig:
        return "make_multisig";
      case mms::message_processing::exchange_multisig_keys:
        return "exchange_multisig_keys";
      case mms::message_processing::create_sync_data:
        return "create_sync_data";
      case mms::message_processing::process_sync_data:
        return "process_sync_data";
      case mms::message_processing::sign_tx:
        return "sign_tx";
      case mms::message_processing::send_tx:
        return "send_tx";
      case mms::message_processing::submit_tx:
        return "submit_tx";
      case mms::message_processing::process_signer_config:
        return "process_signer_config";
      case mms::message_processing::process_auto_config_data:
        return "process_auto_config_data";
      default:
        return "unknown_message_processing";
    }
  }

  bool hex_mms_message_content(mms::message_type type)
  {
    switch (type)
    {
      case mms::message_type::key_set:
      case mms::message_type::additional_key_set:
      case mms::message_type::note:
        // Do NOT convert message content to / from hex because it's textual
        // and is expected as such and delivered as such by clients;
        // the CLI wallet writes such data textually to console, e.g. "MultisigV1..."
        // strings as a result of a "prepare_multisig" command.
        return false;
      default:
        // For all other messages types convert content to / from hex because
        // it's binary data that we should not give back as such over RPC;
        // the CLI wallet writes such data to files and reads it from files.
        return true;
    }
  }

  void convert_mms_message(const mms::message &orig, wallet_rpc::mms_message &conv)
  {
    conv.id = orig.id;
    conv.type = mms_message_type_to_string(orig.type);
    conv.direction = mms_message_direction_to_string(orig.direction);
    if (hex_mms_message_content(orig.type))
    {
      conv.content = epee::string_tools::buff_to_hex_nodelimer(orig.content);
    }
    else
    {
      conv.content = orig.content;
    }
    conv.created = orig.created;
    conv.modified = orig.modified;
    conv.sent = orig.sent;
    conv.signer_index = orig.signer_index;
    conv.hash = epee::string_tools::pod_to_hex(orig.hash);
    conv.state = mms_message_state_to_string(orig.state);
    conv.wallet_height = orig.wallet_height;
    conv.round = orig.round;
    conv.signature_count = orig.signature_count;
    conv.transport_id = orig.transport_id;
  }

  void convert_mms_authorized_signer(cryptonote::network_type nettype, const mms::authorized_signer &orig, wallet_rpc::mms_authorized_signer &conv)
  {
    conv.label = orig.label;
    conv.transport_address = orig.transport_address;
    if (orig.monero_address_known)
    {
      conv.monero_address =  cryptonote::get_account_address_as_str(nettype, false, orig.monero_address);
    }
    else
    {
      // We don't have to work with a "monero_address_known" boolean because an empty string meaning
      // "Monero address unknown" works well enough
      conv.monero_address.clear();
    }
    conv.me = orig.me;
    conv.index = orig.index;
    conv.auto_config_token = orig.auto_config_token;
    conv.auto_config_running = orig.auto_config_running;
    // Don't give back "auto_config_public_key", "auto_config_secret_key" and "auto_config_transport_address",
    // as it seems no client will have use for this info; (already in "mms::authorized_signer" it's public
    // mostly to simplify the MMS implementation ...)
  }

  bool wallet_rpc_server::mms_call_ok(epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_restricted) return restricted(er);
    return true;
  }

  //------------------------------------------------------------------------------------------------------------------------------
  // Initialize the MMS for use with this wallet and set the MMS to "active"
  // Note about the "active" flag of the MMS: In principle one should only use any MMS related methods
  // if the MMS is set to "active" on a particular wallet and configured. The CLI wallet already enforces
  // this and refuses to execute any MMS command if it's not active. The calls here don't care yet about
  // "active", and neither do the methods in "message_store.cpp" themselves. This is of course not good.
  // The way forward will probably be not to add checks here, but to add them directly to the MMS methods
  // themselves.
  bool wallet_rpc_server::on_mms_init(const wallet_rpc::COMMAND_RPC_MMS_INIT::request& req, wallet_rpc::COMMAND_RPC_MMS_INIT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.init(m_wallet->get_multisig_wallet_state(), req.own_label, req.own_transport_address, req.num_authorized_signers, req.num_required_signers);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_info(const wallet_rpc::COMMAND_RPC_MMS_GET_INFO::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_INFO::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    res.active = message_store.get_active();
    if (res.active)
    {
      res.num_authorized_signers = message_store.get_num_authorized_signers();
      res.num_required_signers = message_store.get_num_required_signers();
      res.signer_config_complete = message_store.signer_config_complete();
      res.signer_labels_complete = message_store.signer_labels_complete();
      res.auto_send = message_store.get_auto_send();
    }
    else
    {
      res.num_authorized_signers = 0;
      res.num_required_signers = 0;
      res.signer_config_complete = false;
      res.signer_labels_complete = false;
      res.auto_send = false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  // A number of MMS methods have a parameter of type "mms::multisig_wallet_state". The corresponding
  // MMS RPC methods here get this state themselves and hand it to the MMS methods; it would not make
  // sense to offer an RPC method to get it only to give it back as parameter for those methods.
  // The sense of this method here that returns most what is in "mms::multisig_wallet_state" is
  // simply to offer a convenient way to get various multisig-related info that otherwise would need
  // several different RPC calls to fetch them, or are not otherwise public at all.
  bool wallet_rpc_server::on_mms_get_multisig_wallet_state(const wallet_rpc::COMMAND_RPC_MMS_GET_MULTISIG_WALLET_STATE::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_MULTISIG_WALLET_STATE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::multisig_wallet_state state = m_wallet->get_multisig_wallet_state();

    res.state.address =  cryptonote::get_account_address_as_str(state.nettype, false, state.address);
    res.state.nettype = network_type_to_string(state.nettype);
    // Don't give back "view_secret_key": Not needed, and somewhat problematic security-wise
    res.state.multisig = state.multisig;
    res.state.multisig_is_ready = state.multisig_is_ready;
    res.state.has_multisig_partial_key_images = state.has_multisig_partial_key_images;
    res.state.multisig_rounds_passed = state.multisig_rounds_passed;
    res.state.num_transfer_details = (uint64_t)state.num_transfer_details;
    // Don't give back the name of the MMS file on disk: Not needed
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_set_options(const wallet_rpc::COMMAND_RPC_MMS_SET_OPTIONS::request& req, wallet_rpc::COMMAND_RPC_MMS_SET_OPTIONS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      if (req.set_active)
      {
        message_store.set_active(req.active);
      }
      if (req.set_auto_send)
      {
        message_store.set_auto_send(req.auto_send);
      }
      if (req.set_bitmessage_options)
      {
        message_store.set_options(req.bitmessage_address, req.bitmessage_login);
      }
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_set_signer(const wallet_rpc::COMMAND_RPC_MMS_SET_SIGNER::request& req, wallet_rpc::COMMAND_RPC_MMS_SET_SIGNER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    boost::optional<std::string> label;
    boost::optional<std::string> transport_address;
    boost::optional<cryptonote::account_public_address> monero_address;
    if (req.set_label)
    {
      label = req.label;
    }
    if (req.set_transport_address)
    {
      transport_address = req.transport_address;
    }
    if (req.set_monero_address)
    {
      er.message = "";
      cryptonote::address_parse_info info;
      if(!cryptonote::get_account_address_from_str_or_url(info, m_wallet->nettype(), req.monero_address,
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
          er.message = std::string("WALLET_RPC_ERROR_CODE_WRONG_ADDRESS: ") + req.monero_address;
        return false;
      }
      monero_address = info.address;
    }

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.set_signer(m_wallet->get_multisig_wallet_state(), req.index, label, transport_address, monero_address);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_signer(const wallet_rpc::COMMAND_RPC_MMS_GET_SIGNER::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_SIGNER::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      const mms::authorized_signer &signer = message_store.get_signer(req.index);
      convert_mms_authorized_signer(m_wallet->nettype(), signer, res.signer);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_all_signers(const wallet_rpc::COMMAND_RPC_MMS_GET_ALL_SIGNERS::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_ALL_SIGNERS::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    cryptonote::network_type nettype = m_wallet->nettype();
    const std::vector<mms::authorized_signer> &signers = message_store.get_all_signers();
    res.signers.clear();
    for (size_t i = 0; i < signers.size(); ++i)
    {
      const mms::authorized_signer &orig_signer = signers[i];
      wallet_rpc::mms_authorized_signer conv_signer;
      convert_mms_authorized_signer(nettype, orig_signer, conv_signer);
      res.signers.push_back(conv_signer);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_signer_config(const wallet_rpc::COMMAND_RPC_MMS_GET_SIGNER_CONFIG::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_SIGNER_CONFIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    std::string signer_config;
    mms::message_store &message_store = m_wallet->get_message_store();
    message_store.get_signer_config(signer_config);
    res.signer_config = epee::string_tools::buff_to_hex_nodelimer(signer_config);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_unpack_signer_config(const wallet_rpc::COMMAND_RPC_MMS_UNPACK_SIGNER_CONFIG::request& req, wallet_rpc::COMMAND_RPC_MMS_UNPACK_SIGNER_CONFIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      std::string signer_config;
      if (!epee::string_tools::parse_hexstr_to_binbuff(req.signer_config, signer_config))
      {
        er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_CONTENT;
        er.message = std::string("Bad MMS message content.");
        return false;
      }
      std::vector<mms::authorized_signer> signers;
      message_store.unpack_signer_config(m_wallet->get_multisig_wallet_state(), signer_config, signers);

      cryptonote::network_type nettype = m_wallet->nettype();
      res.signers.clear();
      for (size_t i = 0; i < signers.size(); ++i)
      {
        const mms::authorized_signer &orig_signer = signers[i];
        wallet_rpc::mms_authorized_signer conv_signer;
        convert_mms_authorized_signer(nettype, orig_signer, conv_signer);
        res.signers.push_back(conv_signer);
      }
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_process_signer_config(const wallet_rpc::COMMAND_RPC_MMS_PROCESS_SIGNER_CONFIG::request& req, wallet_rpc::COMMAND_RPC_MMS_PROCESS_SIGNER_CONFIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      std::string signer_config;
      if (!epee::string_tools::parse_hexstr_to_binbuff(req.signer_config, signer_config))
      {
        er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_CONTENT;
        er.message = std::string("Bad MMS message content.");
        return false;
      }
      message_store.process_signer_config(m_wallet->get_multisig_wallet_state(), signer_config);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_start_auto_config(const wallet_rpc::COMMAND_RPC_MMS_START_AUTO_CONFIG::request& req, wallet_rpc::COMMAND_RPC_MMS_START_AUTO_CONFIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.start_auto_config(m_wallet->get_multisig_wallet_state());
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_check_auto_config_token(const wallet_rpc::COMMAND_RPC_MMS_CHECK_AUTO_CONFIG_TOKEN::request& req, wallet_rpc::COMMAND_RPC_MMS_CHECK_AUTO_CONFIG_TOKEN::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    res.token_valid = message_store.check_auto_config_token(req.raw_token, res.adjusted_token);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_add_auto_config_data_message(const wallet_rpc::COMMAND_RPC_MMS_ADD_AUTO_CONFIG_DATA_MESSAGE::request& req, wallet_rpc::COMMAND_RPC_MMS_ADD_AUTO_CONFIG_DATA_MESSAGE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.add_auto_config_data_message(m_wallet->get_multisig_wallet_state(), req.auto_config_token);
      // Ignore the index in the message array that the MMS gives back
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_process_auto_config_data_message(const wallet_rpc::COMMAND_RPC_MMS_PROCESS_AUTO_CONFIG_DATA_MESSAGE::request& req, wallet_rpc::COMMAND_RPC_MMS_PROCESS_AUTO_CONFIG_DATA_MESSAGE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.process_auto_config_data_message(req.id);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_stop_auto_config(const wallet_rpc::COMMAND_RPC_MMS_STOP_AUTO_CONFIG::request& req, wallet_rpc::COMMAND_RPC_MMS_STOP_AUTO_CONFIG::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.stop_auto_config();
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_process_wallet_created_data(const wallet_rpc::COMMAND_RPC_MMS_PROCESS_WALLET_CREATED_DATA::request& req, wallet_rpc::COMMAND_RPC_MMS_PROCESS_WALLET_CREATED_DATA::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_type type;
    if (!string_to_mms_message_type(req.type, type))
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    std::string content;
    if (hex_mms_message_content(type))
    {
      // The RPC call that gave this data to the client hexed it, so we have to
      // convert it to binary again as the MMS expects it in binary form
      if (!epee::string_tools::parse_hexstr_to_binbuff(req.content, content))
      {
        er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_CONTENT;
        er.message = std::string("Bad MMS message content.");
        return false;
      }
    }
    else
    {
      content = req.content;
    }

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.process_wallet_created_data(m_wallet->get_multisig_wallet_state(), type, content);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_processable_messages(const wallet_rpc::COMMAND_RPC_MMS_GET_PROCESSABLE_MESSAGES::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_PROCESSABLE_MESSAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    std::vector<mms::processing_data> data_list;
    mms::message_store &message_store = m_wallet->get_message_store();
    res.available = message_store.get_processable_messages(m_wallet->get_multisig_wallet_state(), req.force_sync, data_list, res.wait_reason);
    res.data_list.clear();
    for (size_t i = 0; i < data_list.size(); ++i)
    {
      mms::processing_data &orig_data = data_list[i];
      wallet_rpc::mms_processing_data conv_data;
      conv_data.processing = mms_message_processing_to_string(orig_data.processing);
      conv_data.message_ids = orig_data.message_ids;
      conv_data.receiving_signer_index = orig_data.receiving_signer_index;
      res.data_list.push_back(conv_data);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_set_messages_processed(const wallet_rpc::COMMAND_RPC_MMS_SET_MESSAGES_PROCESSED::request& req, wallet_rpc::COMMAND_RPC_MMS_SET_MESSAGES_PROCESSED::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      mms::processing_data data;
      // Cheat a little, don't do a full convert, we know only the ids will be needed:
      data.message_ids = req.data.message_ids;
      message_store.set_messages_processed(data);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_add_message(const wallet_rpc::COMMAND_RPC_MMS_ADD_MESSAGE::request& req, wallet_rpc::COMMAND_RPC_MMS_ADD_MESSAGE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    mms::message_type type;
    if (!string_to_mms_message_type(req.type, type))
    {
      er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_TYPE;
      er.message = std::string("Bad MMS message type: ") + req.type;
      return false;
    }
    mms::message_direction direction;
    if (!string_to_mms_message_direction(req.direction, direction))
    {
      er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_DIRECTION;
      er.message = std::string("Bad MMS message direction: ") + req.direction;
      return false;
    }
    std::string content;
    if (hex_mms_message_content(type))
    {
      if (!epee::string_tools::parse_hexstr_to_binbuff(req.content, content))
      {
        er.code = WALLET_RPC_ERROR_CODE_MMS_BAD_MESSAGE_CONTENT;
        er.message = std::string("Bad MMS message content.");
        return false;
      }
    }
    // In case of message type "note" don't put any restriction on content (unlike
    // the CLI wallet) to enable transport of any kind of content by RPC; note that
    // you will have to hex / unhex any binary content yourself as the assumption here
    // is that "note" content is textual and thus does not need such a conversion.

    try
    {
      message_store.add_message(m_wallet->get_multisig_wallet_state(), req.signer_index, type, direction, content);
      // The return parameter (the new total number of messages) probably does not
      // interest RPC clients, so don't give it back as result
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_all_messages(const wallet_rpc::COMMAND_RPC_MMS_GET_ALL_MESSAGES::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_ALL_MESSAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    const std::vector<mms::message> messages = message_store.get_all_messages();

    res.messages.clear();
    for (size_t i = 0; i < messages.size(); ++i)
    {
      wallet_rpc::mms_message conv;
      convert_mms_message(messages[i], conv);
      res.messages.push_back(conv);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_get_message_by_id(const wallet_rpc::COMMAND_RPC_MMS_GET_MESSAGE_BY_ID::request& req, wallet_rpc::COMMAND_RPC_MMS_GET_MESSAGE_BY_ID::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    mms::message_store &message_store = m_wallet->get_message_store();
    mms::message message;
    res.found = message_store.get_message_by_id(req.id, message);
    if (res.found)
    {
      convert_mms_message(message, res.message);
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_set_message_processed_or_sent(const wallet_rpc::COMMAND_RPC_MMS_SET_MESSAGE_PROCESSED_OR_SENT::request& req, wallet_rpc::COMMAND_RPC_MMS_SET_MESSAGE_PROCESSED_OR_SENT::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.set_message_processed_or_sent(req.id);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_delete_message(const wallet_rpc::COMMAND_RPC_MMS_DELETE_MESSAGE::request& req, wallet_rpc::COMMAND_RPC_MMS_DELETE_MESSAGE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.delete_message(req.id);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_delete_all_messages(const wallet_rpc::COMMAND_RPC_MMS_DELETE_ALL_MESSAGES::request& req, wallet_rpc::COMMAND_RPC_MMS_DELETE_ALL_MESSAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.delete_all_messages();
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_send_message(const wallet_rpc::COMMAND_RPC_MMS_SEND_MESSAGE::request& req, wallet_rpc::COMMAND_RPC_MMS_SEND_MESSAGE::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      message_store.send_message(m_wallet->get_multisig_wallet_state(), req.id);
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_mms_check_for_messages(const wallet_rpc::COMMAND_RPC_MMS_CHECK_FOR_MESSAGES::request& req, wallet_rpc::COMMAND_RPC_MMS_CHECK_FOR_MESSAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx)
  {
    if (!mms_call_ok(er)) return false;

    res.received = false;
    res.messages.clear();
    try
    {
      mms::message_store &message_store = m_wallet->get_message_store();
      std::vector<mms::message> messages;
      res.received = message_store.check_for_messages(m_wallet->get_multisig_wallet_state(), messages);

      if (res.received)
      {
        for (size_t i = 0; i < messages.size(); ++i)
        {
          wallet_rpc::mms_message conv;
          convert_mms_message(messages[i], conv);
          res.messages.push_back(conv);
        }
      }
    }
    catch (const std::exception &e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_MMS_PROCESSING_ERROR);
      return false;
    }
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
    std::unique_ptr<tools::wallet2> wal;
    try
    {
      const bool testnet = tools::wallet2::has_testnet_option(vm);
      const bool stagenet = tools::wallet2::has_stagenet_option(vm);
      if (testnet && stagenet)
      {
        MERROR(tools::wallet_rpc_server::tr("Can't specify more than one of --testnet and --stagenet"));
        return false;
      }

      const auto arg_wallet_file = wallet_args::arg_wallet_file();
      const auto arg_from_json = wallet_args::arg_generate_from_json();
      const auto arg_rpc_client_secret_key = wallet_args::arg_rpc_client_secret_key();

      const auto wallet_file = command_line::get_arg(vm, arg_wallet_file);
      const auto from_json = command_line::get_arg(vm, arg_from_json);
      const auto wallet_dir = command_line::get_arg(vm, arg_wallet_dir);
      const auto prompt_for_password = command_line::get_arg(vm, arg_prompt_for_password);
      const auto password_prompt = prompt_for_password ? password_prompter : nullptr;

      if(!wallet_file.empty() && !from_json.empty())
      {
        LOG_ERROR(tools::wallet_rpc_server::tr("Can't specify more than one of --wallet-file and --generate-from-json"));
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
        wal = tools::wallet2::make_from_file(vm, true, wallet_file, password_prompt).first;
      }
      else
      {
        try
        {
          auto rc = tools::wallet2::make_from_json(vm, true, from_json, password_prompt);
          wal = std::move(rc.first);
        }
        catch (const std::exception &e)
        {
          MERROR("Error creating wallet: " << e.what());
          return false;
        }
      }
      if (!wal)
      {
        return false;
      }

      if (!command_line::is_arg_defaulted(vm, arg_rpc_client_secret_key))
      {
        crypto::secret_key client_secret_key;
        if (!epee::string_tools::hex_to_pod(command_line::get_arg(vm, arg_rpc_client_secret_key), client_secret_key))
        {
          MERROR(arg_rpc_client_secret_key.name << ": RPC client secret key should be 32 byte in hex format");
          return false;
        }
        wal->set_rpc_client_secret_key(client_secret_key);
      }

      bool quit = false;
      tools::signal_handler::install([&wal, &quit](int) {
        assert(wal);
        quit = true;
        wal->stop();
      });

      wal->refresh(wal->is_trusted_daemon());
      // if we ^C during potentially length load/refresh, there's no server loop yet
      if (quit)
      {
        MINFO(tools::wallet_rpc_server::tr("Saving wallet..."));
        wal->store();
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
  const auto arg_rpc_client_secret_key = wallet_args::arg_rpc_client_secret_key();

  po::options_description hidden_options("Hidden");

  po::options_description desc_params(wallet_args::tr("Wallet options"));
  tools::wallet2::init_options(desc_params);
  command_line::add_arg(desc_params, arg_rpc_bind_port);
  command_line::add_arg(desc_params, arg_disable_rpc_login);
  command_line::add_arg(desc_params, arg_restricted);
  cryptonote::rpc_args::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_from_json);
  command_line::add_arg(desc_params, arg_wallet_dir);
  command_line::add_arg(desc_params, arg_prompt_for_password);
  command_line::add_arg(desc_params, arg_rpc_client_secret_key);

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
    [](const std::string &s, bool emphasis){ epee::set_console_color(emphasis ? epee::console_color_white : epee::console_color_default, true); std::cout << s << std::endl; if (emphasis) epee::reset_console_color(); },
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
