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
#include <boost/format.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdint>
#include "include_base_utils.h"
using namespace epee;

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

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

namespace
{
  const command_line::arg_descriptor<std::string, true> arg_rpc_bind_port = {"rpc-bind-port", "Sets bind port for server"};
  const command_line::arg_descriptor<bool> arg_disable_rpc_login = {"disable-rpc-login", "Disable HTTP authentication for RPC connections served by this process"};
  const command_line::arg_descriptor<bool> arg_trusted_daemon = {"trusted-daemon", "Enable commands which rely on a trusted daemon", false};
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
}

namespace tools
{
  const char* wallet_rpc_server::tr(const char* str)
  {
    return i18n_translate(str, "tools::wallet_rpc_server");
  }

  //------------------------------------------------------------------------------------------------------------------------------
  wallet_rpc_server::wallet_rpc_server():m_wallet(NULL), rpc_login_file(), m_stop(false), m_trusted_daemon(false), m_vm(NULL)
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
      try {
        if (m_wallet) m_wallet->refresh();
      } catch (const std::exception& ex) {
        LOG_ERROR("Exception at while refreshing, what=" << ex.what());
      }
      return true;
    }, 20000);
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
    tools::wallet2 *walvars;
    std::unique_ptr<tools::wallet2> tmpwal;

    if (m_wallet)
      walvars = m_wallet;
    else
    {
      tmpwal = tools::wallet2::make_dummy(*m_vm, password_prompter);
      walvars = tmpwal.get();
    }
    boost::optional<epee::net_utils::http::login> http_login{};
    std::string bind_port = command_line::get_arg(*m_vm, arg_rpc_bind_port);
    const bool disable_auth = command_line::get_arg(*m_vm, arg_disable_rpc_login);
    m_trusted_daemon = command_line::get_arg(*m_vm, arg_trusted_daemon);
    if (!command_line::has_arg(*m_vm, arg_trusted_daemon))
    {
      if (tools::is_local_address(walvars->get_daemon_address()))
      {
        MINFO(tr("Daemon is local, assuming trusted"));
        m_trusted_daemon = true;
      }
    }
    if (command_line::has_arg(*m_vm, arg_wallet_dir))
    {
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

    m_net_server.set_threads_prefix("RPC");
    auto rng = [](size_t len, uint8_t *ptr) { return crypto::rand(len, ptr); };
    return epee::http_server_impl_base<wallet_rpc_server, connection_context>::init(
      rng, std::move(bind_port), std::move(rpc_config->bind_ip), std::move(rpc_config->access_control_origins), std::move(http_login)
    );
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::not_open(epee::json_rpc::error& er)
  {
      er.code = WALLET_RPC_ERROR_CODE_NOT_OPEN;
      er.message = "No wallet file";
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
    entry.fee = pd.m_fee;
    entry.note = m_wallet->get_tx_note(pd.m_tx_hash);
    entry.type = "in";
    entry.subaddr_index = pd.m_subaddr_index;
    entry.address = m_wallet->get_subaddress_as_str(pd.m_subaddr_index);
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
    entry.fee = pd.m_amount_in - pd.m_amount_out;
    uint64_t change = pd.m_change == (uint64_t)-1 ? 0 : pd.m_change; // change may not be known
    entry.amount = pd.m_amount_in - change - entry.fee;
    entry.note = m_wallet->get_tx_note(txid);

    for (const auto &d: pd.m_dests) {
      entry.destinations.push_back(wallet_rpc::transfer_destination());
      wallet_rpc::transfer_destination &td = entry.destinations.back();
      td.amount = d.amount;
      td.address = get_account_address_as_str(m_wallet->nettype(), d.is_subaddress, d.addr);
    }

    entry.type = "out";
    entry.subaddr_index = { pd.m_subaddr_account, 0 };
    entry.address = m_wallet->get_subaddress_as_str({pd.m_subaddr_account, 0});
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
    entry.note = m_wallet->get_tx_note(txid);
    entry.type = is_failed ? "failed" : "pending";
    entry.subaddr_index = { pd.m_subaddr_account, 0 };
    entry.address = m_wallet->get_subaddress_as_str({pd.m_subaddr_account, 0});
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
    entry.fee = pd.m_fee;
    entry.note = m_wallet->get_tx_note(pd.m_tx_hash);
    entry.double_spend_seen = ppd.m_double_spend_seen;
    entry.type = "pool";
    entry.subaddr_index = pd.m_subaddr_index;
    entry.address = m_wallet->get_subaddress_as_str(pd.m_subaddr_index);
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    try
    {
      res.balance = m_wallet->balance(req.account_index);
      res.unlocked_balance = m_wallet->unlocked_balance(req.account_index);
      res.multisig_import_needed = m_wallet->multisig() && m_wallet->has_multisig_partial_key_images();
      std::map<uint32_t, uint64_t> balance_per_subaddress = m_wallet->balance_per_subaddress(req.account_index);
      std::map<uint32_t, uint64_t> unlocked_balance_per_subaddress = m_wallet->unlocked_balance_per_subaddress(req.account_index);
      std::vector<tools::wallet2::transfer_details> transfers;
      m_wallet->get_transfers(transfers);
      for (const auto& i : balance_per_subaddress)
      {
        wallet_rpc::COMMAND_RPC_GET_BALANCE::per_subaddress_info info;
        info.address_index = i.first;
        cryptonote::subaddress_index index = {req.account_index, info.address_index};
        info.address = m_wallet->get_subaddress_as_str(index);
        info.balance = i.second;
        info.unlocked_balance = unlocked_balance_per_subaddress[i.first];
        info.label = m_wallet->get_subaddress_label(index);
        info.num_unspent_outputs = std::count_if(transfers.begin(), transfers.end(), [&](const tools::wallet2::transfer_details& td) { return !td.m_spent && td.m_subaddr_index == index; });
        res.per_subaddress.push_back(info);
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
  bool wallet_rpc_server::on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_create_address(const wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_label_address(const wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_accounts(const wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::response& res, epee::json_rpc::error& er)
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
        info.balance = m_wallet->balance(subaddr_index.major);
        info.unlocked_balance = m_wallet->unlocked_balance(subaddr_index.major);
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
  bool wallet_rpc_server::on_create_account(const wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_label_account(const wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_account_tags(const wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_tag_accounts(const wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_untag_accounts(const wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_set_account_tag_description(const wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::request& req, wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_getheight(const wallet_rpc::COMMAND_RPC_GET_HEIGHT::request& req, wallet_rpc::COMMAND_RPC_GET_HEIGHT::response& res, epee::json_rpc::error& er)
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

      de.addr = info.address;
      de.is_subaddress = info.is_subaddress;
      de.amount = it->amount;
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

      /* Just to clarify */
      const std::string& payment_id_str = payment_id;

      crypto::hash long_payment_id;
      crypto::hash8 short_payment_id;

      /* Parse payment ID */
      if (wallet2::parse_long_payment_id(payment_id_str, long_payment_id)) {
        cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, long_payment_id);
      }
      /* or short payment ID */
      else if (wallet2::parse_short_payment_id(payment_id_str, short_payment_id)) {
        cryptonote::set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, short_payment_id);
      }
      else {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment id has invalid format: \"" + payment_id_str + "\", expected 16 or 64 character string";
        return false;
      }

      /* Append Payment ID data into extra */
      if (!cryptonote::add_extra_nonce_to_tx_extra(extra, extra_nonce)) {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Something went wrong with payment_id. Please check its format: \"" + payment_id_str + "\", expected 64-character string";
        return false;
      }

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
      bool get_tx_key, Ts& tx_key, Tu &amount, Tu &fee, std::string &multisig_txset, bool do_not_relay,
      Ts &tx_hash, bool get_tx_hex, Ts &tx_blob, bool get_tx_metadata, Ts &tx_metadata, epee::json_rpc::error &er)
  {
    for (const auto & ptx : ptx_vector)
    {
      if (get_tx_key)
      {
        std::string s = epee::string_tools::pod_to_hex(ptx.tx_key);
        for (const crypto::secret_key& additional_tx_key : ptx.additional_tx_keys)
          s += epee::string_tools::pod_to_hex(additional_tx_key);
        fill(tx_key, s);
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
      if (!do_not_relay)
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
  bool wallet_rpc_server::on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er)
  {

    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    LOG_PRINT_L3("on_transfer starts");
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
      uint64_t mixin;
      if(req.ring_size != 0)
      {
        mixin = m_wallet->adjust_mixin(req.ring_size - 1);
      }
      else
      {
        mixin = m_wallet->adjust_mixin(req.mixin);
      }
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_2(dsts, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices, m_trusted_daemon);

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

      return fill_response(ptx_vector, req.get_tx_key, res.tx_key, res.amount, res.fee, res.multisig_txset, req.do_not_relay,
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
  bool wallet_rpc_server::on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er)
  {

    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
      uint64_t mixin;
      if(req.ring_size != 0)
      {
        mixin = m_wallet->adjust_mixin(req.ring_size - 1);
      }
      else
      {
        mixin = m_wallet->adjust_mixin(req.mixin);
      }
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      LOG_PRINT_L2("on_transfer_split calling create_transactions_2");
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_2(dsts, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices, m_trusted_daemon);
      LOG_PRINT_L2("on_transfer_split called create_transactions_2");

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, req.do_not_relay,
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
  bool wallet_rpc_server::on_sweep_dust(const wallet_rpc::COMMAND_RPC_SWEEP_DUST::request& req, wallet_rpc::COMMAND_RPC_SWEEP_DUST::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_unmixable_sweep_transactions(m_trusted_daemon);

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, req.do_not_relay,
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
  bool wallet_rpc_server::on_sweep_all(const wallet_rpc::COMMAND_RPC_SWEEP_ALL::request& req, wallet_rpc::COMMAND_RPC_SWEEP_ALL::response& res, epee::json_rpc::error& er)
  {
    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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

    try
    {
      uint64_t mixin;
      if(req.ring_size != 0)
      {
        mixin = m_wallet->adjust_mixin(req.ring_size - 1);
      }
      else
      {
        mixin = m_wallet->adjust_mixin(req.mixin);
      }
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_all(req.below_amount, dsts[0].addr, dsts[0].is_subaddress, mixin, req.unlock_time, priority, extra, req.account_index, req.subaddr_indices, m_trusted_daemon);

      return fill_response(ptx_vector, req.get_tx_keys, res.tx_key_list, res.amount_list, res.fee_list, res.multisig_txset, req.do_not_relay,
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
  bool wallet_rpc_server::on_sweep_single(const wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::request& req, wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::response& res, epee::json_rpc::error& er)
  {
    std::vector<cryptonote::tx_destination_entry> dsts;
    std::vector<uint8_t> extra;

    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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

    crypto::key_image ki;
    if (!epee::string_tools::hex_to_pod(req.key_image, ki))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
      er.message = "failed to parse key image";
      return false;
    }

    try
    {
      uint64_t mixin;
      if(req.ring_size != 0)
      {
        mixin = m_wallet->adjust_mixin(req.ring_size - 1);
      }
      else
      {
        mixin = m_wallet->adjust_mixin(req.mixin);
      }
      uint32_t priority = m_wallet->adjust_priority(req.priority);
      std::vector<wallet2::pending_tx> ptx_vector = m_wallet->create_transactions_single(ki, dsts[0].addr, dsts[0].is_subaddress, mixin, req.unlock_time, priority, extra, m_trusted_daemon);

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

      return fill_response(ptx_vector, req.get_tx_key, res.tx_key, res.amount, res.fee, res.multisig_txset, req.do_not_relay,
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
  bool wallet_rpc_server::on_relay_tx(const wallet_rpc::COMMAND_RPC_RELAY_TX::request& req, wallet_rpc::COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er)
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

      res.integrated_address = m_wallet->get_integrated_address_as_str(payment_id);
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
  bool wallet_rpc_server::on_split_integrated_address(const wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er)
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
      rpc_payment.subaddr_index = payment.m_subaddr_index;
      rpc_payment.address      = m_wallet->get_subaddress_as_str(payment.m_subaddr_index);
      res.payments.push_back(rpc_payment);
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er)
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

      if(!epee::string_tools::parse_hexstr_to_binbuff(payment_id_str, payment_id_blob))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
        er.message = "Payment ID has invalid format: " + payment_id_str;
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
        er.message = "Payment ID has invalid size: " + payment_id_str;
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
        res.payments.push_back(std::move(rpc_payment));
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er)
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

    bool transfers_found = false;
    for (const auto& td : transfers)
    {
      if (!filter || available != td.m_spent)
      {
        if (req.account_index != td.m_subaddr_index.major || (!req.subaddr_indices.empty() && req.subaddr_indices.count(td.m_subaddr_index.minor) == 0))
          continue;
        if (!transfers_found)
        {
          transfers_found = true;
        }
        auto txBlob = t_serializable_object_to_blob(td.m_tx);
        wallet_rpc::transfer_details rpc_transfers;
        rpc_transfers.amount       = td.amount();
        rpc_transfers.spent        = td.m_spent;
        rpc_transfers.global_index = td.m_global_output_index;
        rpc_transfers.tx_hash      = epee::string_tools::pod_to_hex(td.m_txid);
        rpc_transfers.tx_size      = txBlob.size();
        rpc_transfers.subaddr_index = td.m_subaddr_index.minor;
        rpc_transfers.key_image    = req.verbose && td.m_key_image_known ? epee::string_tools::pod_to_hex(td.m_key_image) : "";
        res.transfers.push_back(rpc_transfers);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_query_key(const wallet_rpc::COMMAND_RPC_QUERY_KEY::request& req, wallet_rpc::COMMAND_RPC_QUERY_KEY::response& res, epee::json_rpc::error& er)
  {
      if (!m_wallet) return not_open(er);
      if (m_wallet->restricted())
      {
        er.code = WALLET_RPC_ERROR_CODE_DENIED;
        er.message = "Command unavailable in restricted mode.";
        return false;
      }

      if (req.key_type.compare("mnemonic") == 0)
      {
        if (!m_wallet->get_seed(res.key))
        {
            er.message = "The wallet is non-deterministic. Cannot display seed.";
            return false;
        }
      }
      else if(req.key_type.compare("view_key") == 0)
      {
          res.key = string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
      }
      else if(req.key_type.compare("spend_key") == 0)
      {
          res.key = string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key);
      }
      else
      {
          er.message = "key_type " + req.key_type + " not found";
          return false;
      }

      return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_rescan_blockchain(const wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::request& req, wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    try
    {
      m_wallet->rescan_blockchain();
    }
    catch (const std::exception& e)
    {
      handle_rpc_exception(std::current_exception(), er, WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR);
      return false;
    }
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_sign(const wallet_rpc::COMMAND_RPC_SIGN::request& req, wallet_rpc::COMMAND_RPC_SIGN::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    res.signature = m_wallet->sign(req.data);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_verify(const wallet_rpc::COMMAND_RPC_VERIFY::request& req, wallet_rpc::COMMAND_RPC_VERIFY::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_stop_wallet(const wallet_rpc::COMMAND_RPC_STOP_WALLET::request& req, wallet_rpc::COMMAND_RPC_STOP_WALLET::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_set_tx_notes(const wallet_rpc::COMMAND_RPC_SET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_SET_TX_NOTES::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_get_tx_notes(const wallet_rpc::COMMAND_RPC_GET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_GET_TX_NOTES::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_set_attribute(const wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    m_wallet->set_attribute(req.key, req.value);

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_attribute(const wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }

    res.value = m_wallet->get_attribute(req.key);
    return true;
  }
  bool wallet_rpc_server::on_get_tx_key(const wallet_rpc::COMMAND_RPC_GET_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_GET_TX_KEY::response& res, epee::json_rpc::error& er)
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

    std::ostringstream oss;
    oss << epee::string_tools::pod_to_hex(tx_key);
    for (size_t i = 0; i < additional_tx_keys.size(); ++i)
      oss << epee::string_tools::pod_to_hex(additional_tx_keys[i]);
    res.tx_key = oss.str();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_check_tx_key(const wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(req.txid, txid))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
      er.message = "TX ID has invalid format";
      return false;
    }

    std::string tx_key_str = req.tx_key;
    crypto::secret_key tx_key;
    if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), tx_key))
    {
      er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY;
      er.message = "Tx key has invalid format";
      return false;
    }
    tx_key_str = tx_key_str.substr(64);
    std::vector<crypto::secret_key> additional_tx_keys;
    while (!tx_key_str.empty())
    {
      additional_tx_keys.resize(additional_tx_keys.size() + 1);
      if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), additional_tx_keys.back()))
      {
        er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY;
        er.message = "Tx key has invalid format";
        return false;
      }
      tx_key_str = tx_key_str.substr(64);
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
  bool wallet_rpc_server::on_get_tx_proof(const wallet_rpc::COMMAND_RPC_GET_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_TX_PROOF::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_check_tx_proof(const wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::response& res, epee::json_rpc::error& er)
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
      uint64_t received;
      bool in_pool;
      uint64_t confirmations;
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
  bool wallet_rpc_server::on_get_spend_proof(const wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_check_spend_proof(const wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_reserve_proof(const wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_check_reserve_proof(const wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_transfers(const wallet_rpc::COMMAND_RPC_GET_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFERS::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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

    if (req.in)
    {
      std::list<std::pair<crypto::hash, tools::wallet2::payment_details>> payments;
      m_wallet->get_payments(payments, min_height, max_height, req.account_index, req.subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.in.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.in.back(), i->second.m_tx_hash, i->first, i->second);
      }
    }

    if (req.out)
    {
      std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments;
      m_wallet->get_payments_out(payments, min_height, max_height, req.account_index, req.subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.out.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.out.back(), i->first, i->second);
      }
    }

    if (req.pending || req.failed) {
      std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
      m_wallet->get_unconfirmed_payments_out(upayments, req.account_index, req.subaddr_indices);
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
      m_wallet->update_pool_state();

      std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> payments;
      m_wallet->get_unconfirmed_payments(payments, req.account_index, req.subaddr_indices);
      for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = payments.begin(); i != payments.end(); ++i) {
        res.pool.push_back(wallet_rpc::transfer_entry());
        fill_transfer_entry(res.pool.back(), i->first, i->second);
      }
    }

    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_get_transfer_by_txid(const wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
        fill_transfer_entry(res.transfer, i->second.m_tx_hash, i->first, i->second);
        return true;
      }
    }

    std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> payments_out;
    m_wallet->get_payments_out(payments_out, 0, (uint64_t)-1, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>>::const_iterator i = payments_out.begin(); i != payments_out.end(); ++i) {
      if (i->first == txid)
      {
        fill_transfer_entry(res.transfer, i->first, i->second);
        return true;
      }
    }

    std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>> upayments;
    m_wallet->get_unconfirmed_payments_out(upayments, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::unconfirmed_transfer_details>>::const_iterator i = upayments.begin(); i != upayments.end(); ++i) {
      if (i->first == txid)
      {
        fill_transfer_entry(res.transfer, i->first, i->second);
        return true;
      }
    }

    m_wallet->update_pool_state();

    std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>> pool_payments;
    m_wallet->get_unconfirmed_payments(pool_payments, req.account_index);
    for (std::list<std::pair<crypto::hash, tools::wallet2::pool_payment_details>>::const_iterator i = pool_payments.begin(); i != pool_payments.end(); ++i) {
      if (i->second.m_pd.m_tx_hash == txid)
      {
        fill_transfer_entry(res.transfer, i->first, i->second);
        return true;
      }
    }

    er.code = WALLET_RPC_ERROR_CODE_WRONG_TXID;
    er.message = "Transaction not found.";
    return false;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_export_key_images(const wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    try
    {
      std::vector<std::pair<crypto::key_image, crypto::signature>> ski = m_wallet->export_key_images();
      res.signed_key_images.resize(ski.size());
      for (size_t n = 0; n < ski.size(); ++n)
      {
         res.signed_key_images[n].key_image = epee::string_tools::pod_to_hex(ski[n].first);
         res.signed_key_images[n].signature = epee::string_tools::pod_to_hex(ski[n].second);
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
  bool wallet_rpc_server::on_import_key_images(const wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
    {
      er.code = WALLET_RPC_ERROR_CODE_DENIED;
      er.message = "Command unavailable in restricted mode.";
      return false;
    }
    if (!m_trusted_daemon)
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
        cryptonote::blobdata bd;

        if(!epee::string_tools::parse_hexstr_to_binbuff(req.signed_key_images[n].key_image, bd) || bd.size() != sizeof(crypto::key_image))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE;
          er.message = "failed to parse key image";
          return false;
        }
        ski[n].first = *reinterpret_cast<const crypto::key_image*>(bd.data());

        if(!epee::string_tools::parse_hexstr_to_binbuff(req.signed_key_images[n].signature, bd) || bd.size() != sizeof(crypto::signature))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_SIGNATURE;
          er.message = "failed to parse signature";
          return false;
        }
        ski[n].second = *reinterpret_cast<const crypto::signature*>(bd.data());
      }
      uint64_t spent = 0, unspent = 0;
      uint64_t height = m_wallet->import_key_images(ski, spent, unspent);
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
  bool wallet_rpc_server::on_make_uri(const wallet_rpc::COMMAND_RPC_MAKE_URI::request& req, wallet_rpc::COMMAND_RPC_MAKE_URI::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_parse_uri(const wallet_rpc::COMMAND_RPC_PARSE_URI::request& req, wallet_rpc::COMMAND_RPC_PARSE_URI::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_address_book(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_add_address_book(const wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
      crypto::hash8 short_payment_id;

      if (!wallet2::parse_long_payment_id(req.payment_id, payment_id))
      {
        if (!wallet2::parse_short_payment_id(req.payment_id, info.payment_id))
        {
          er.code = WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID;
          er.message = "Payment id has invalid format: \"" + req.payment_id + "\", expected 16 or 64 character string";
          return false;
        }
        else
        {
          memcpy(payment_id.data, info.payment_id.data, 8);
          memset(payment_id.data + 8, 0, 24);
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
  bool wallet_rpc_server::on_delete_address_book(const wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_rescan_spent(const wallet_rpc::COMMAND_RPC_RESCAN_SPENT::request& req, wallet_rpc::COMMAND_RPC_RESCAN_SPENT::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_start_mining(const wallet_rpc::COMMAND_RPC_START_MINING::request& req, wallet_rpc::COMMAND_RPC_START_MINING::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (!m_trusted_daemon)
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
  bool wallet_rpc_server::on_stop_mining(const wallet_rpc::COMMAND_RPC_STOP_MINING::request& req, wallet_rpc::COMMAND_RPC_STOP_MINING::response& res, epee::json_rpc::error& er)
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
  bool wallet_rpc_server::on_get_languages(const wallet_rpc::COMMAND_RPC_GET_LANGUAGES::request& req, wallet_rpc::COMMAND_RPC_GET_LANGUAGES::response& res, epee::json_rpc::error& er)
  {
    crypto::ElectrumWords::get_language_list(res.languages);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_create_wallet(const wallet_rpc::COMMAND_RPC_CREATE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CREATE_WALLET::response& res, epee::json_rpc::error& er)
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
    std::string wallet_file = m_wallet_dir + "/" + req.filename;
    {
      std::vector<std::string> languages;
      crypto::ElectrumWords::get_language_list(languages);
      std::vector<std::string>::iterator it;
      std::string wallet_file;
      char *ptr;

      it = std::find(languages.begin(), languages.end(), req.language);
      if (it == languages.end())
      {
        er.code = WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR;
        er.message = "Unknown language";
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
    std::unique_ptr<tools::wallet2> wal = tools::wallet2::make_new(vm2, nullptr).first;
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
      delete m_wallet;
    m_wallet = wal.release();
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_open_wallet(const wallet_rpc::COMMAND_RPC_OPEN_WALLET::request& req, wallet_rpc::COMMAND_RPC_OPEN_WALLET::response& res, epee::json_rpc::error& er)
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
      wal = tools::wallet2::make_from_file(vm2, wallet_file, nullptr).first;
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
  bool wallet_rpc_server::on_is_multisig(const wallet_rpc::COMMAND_RPC_IS_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IS_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    res.multisig = m_wallet->multisig(&res.ready, &res.threshold, &res.total);
    return true;
  }
  //------------------------------------------------------------------------------------------------------------------------------
  bool wallet_rpc_server::on_prepare_multisig(const wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_make_multisig(const wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_export_multisig(const wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_import_multisig(const wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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

    if (m_trusted_daemon)
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
  bool wallet_rpc_server::on_finalize_multisig(const wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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

    if (req.multisig_info.size() < threshold - 1)
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
  bool wallet_rpc_server::on_sign_multisig(const wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
  bool wallet_rpc_server::on_submit_multisig(const wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::response& res, epee::json_rpc::error& er)
  {
    if (!m_wallet) return not_open(er);
    if (m_wallet->restricted())
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
}

int main(int argc, char** argv) {
  namespace po = boost::program_options;

  const auto arg_wallet_file = wallet_args::arg_wallet_file();
  const auto arg_from_json = wallet_args::arg_generate_from_json();

  po::options_description desc_params(wallet_args::tr("Wallet options"));
  tools::wallet2::init_options(desc_params);
  command_line::add_arg(desc_params, arg_rpc_bind_port);
  command_line::add_arg(desc_params, arg_disable_rpc_login);
  command_line::add_arg(desc_params, arg_trusted_daemon);
  cryptonote::rpc_args::init_options(desc_params);
  command_line::add_arg(desc_params, arg_wallet_file);
  command_line::add_arg(desc_params, arg_from_json);
  command_line::add_arg(desc_params, arg_wallet_dir);
  command_line::add_arg(desc_params, arg_prompt_for_password);

  const auto vm = wallet_args::main(
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

  std::unique_ptr<tools::wallet2> wal;
  try
  {
    const bool testnet = tools::wallet2::has_testnet_option(*vm);
    const bool stagenet = tools::wallet2::has_stagenet_option(*vm);
    if (testnet && stagenet)
    {
      MERROR(tools::wallet_rpc_server::tr("Can't specify more than one of --testnet and --stagenet"));
      return 1;
    }

    const auto wallet_file = command_line::get_arg(*vm, arg_wallet_file);
    const auto from_json = command_line::get_arg(*vm, arg_from_json);
    const auto wallet_dir = command_line::get_arg(*vm, arg_wallet_dir);
    const auto prompt_for_password = command_line::get_arg(*vm, arg_prompt_for_password);
    const auto password_prompt = prompt_for_password ? password_prompter : nullptr;

    if(!wallet_file.empty() && !from_json.empty())
    {
      LOG_ERROR(tools::wallet_rpc_server::tr("Can't specify more than one of --wallet-file and --generate-from-json"));
      return 1;
    }

    if (!wallet_dir.empty())
    {
      wal = NULL;
      goto just_dir;
    }

    if (wallet_file.empty() && from_json.empty())
    {
      LOG_ERROR(tools::wallet_rpc_server::tr("Must specify --wallet-file or --generate-from-json or --wallet-dir"));
      return 1;
    }

    LOG_PRINT_L0(tools::wallet_rpc_server::tr("Loading wallet..."));
    if(!wallet_file.empty())
    {
      wal = tools::wallet2::make_from_file(*vm, wallet_file, password_prompt).first;
    }
    else
    {
      try
      {
        wal = tools::wallet2::make_from_json(*vm, from_json, password_prompt);
      }
      catch (const std::exception &e)
      {
        MERROR("Error creating wallet: " << e.what());
        return 1;
      }
    }
    if (!wal)
    {
      return 1;
    }

    bool quit = false;
    tools::signal_handler::install([&wal, &quit](int) {
      assert(wal);
      quit = true;
      wal->stop();
    });

    wal->refresh();
    // if we ^C during potentially length load/refresh, there's no server loop yet
    if (quit)
    {
      MINFO(tools::wallet_rpc_server::tr("Saving wallet..."));
      wal->store();
      MINFO(tools::wallet_rpc_server::tr("Successfully saved"));
      return 1;
    }
    MINFO(tools::wallet_rpc_server::tr("Successfully loaded"));
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(tools::wallet_rpc_server::tr("Wallet initialization failed: ") << e.what());
    return 1;
  }
just_dir:
  tools::wallet_rpc_server wrpc;
  if (wal) wrpc.set_wallet(wal.release());
  bool r = wrpc.init(&(vm.get()));
  CHECK_AND_ASSERT_MES(r, 1, tools::wallet_rpc_server::tr("Failed to initialize wallet RPC server"));
  tools::signal_handler::install([&wrpc](int) {
    wrpc.send_stop_signal();
  });
  LOG_PRINT_L0(tools::wallet_rpc_server::tr("Starting wallet RPC server"));
  try
  {
    wrpc.run();
  }
  catch (const std::exception &e)
  {
    LOG_ERROR(tools::wallet_rpc_server::tr("Failed to run wallet: ") << e.what());
    return 1;
  }
  LOG_PRINT_L0(tools::wallet_rpc_server::tr("Stopped wallet RPC server"));
  try
  {
    LOG_PRINT_L0(tools::wallet_rpc_server::tr("Saving wallet..."));
    wrpc.stop();
    LOG_PRINT_L0(tools::wallet_rpc_server::tr("Successfully saved"));
  }
  catch (const std::exception& e)
  {
    LOG_ERROR(tools::wallet_rpc_server::tr("Failed to save wallet: ") << e.what());
    return 1;
  }
  return 0;
}
