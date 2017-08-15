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


#include "wallet_manager.h"
#include "wallet.h"
#include "common_defines.h"
#include "common/dns_utils.h"
#include "common/util.h"
#include "common/updates.h"
#include "version.h"
#include "net/http_client.h"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "WalletAPI"

namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

namespace {
    template<typename Request, typename Response>
    bool connect_and_invoke(const std::string& address, const std::string& path, const Request& request, Response& response)
    {
        epee::net_utils::http::http_simple_client client{};
        return client.set_server(address, boost::none) && epee::net_utils::invoke_http_json(path, request, response, client);
    }
}

namespace Monero {

Wallet *WalletManagerImpl::createWallet(const std::string &path, const std::string &password,
                                    const std::string &language, bool testnet)
{
    WalletImpl * wallet = new WalletImpl(testnet);
    wallet->create(path, password, language);
    return wallet;
}

Wallet *WalletManagerImpl::openWallet(const std::string &path, const std::string &password, bool testnet)
{
    WalletImpl * wallet = new WalletImpl(testnet);
    wallet->open(path, password);
    //Refresh addressBook
    wallet->addressBook()->refresh(); 
    return wallet;
}

Wallet *WalletManagerImpl::recoveryWallet(const std::string &path, const std::string &memo, bool testnet, uint64_t restoreHeight)
{
    WalletImpl * wallet = new WalletImpl(testnet);
    if(restoreHeight > 0){
        wallet->setRefreshFromBlockHeight(restoreHeight);
    }
    wallet->recover(path, memo);
    return wallet;
}

Wallet *WalletManagerImpl::createWalletFromKeys(const std::string &path, 
                                                const std::string &language,
                                                bool testnet, 
                                                uint64_t restoreHeight,
                                                const std::string &addressString,
                                                const std::string &viewKeyString,
                                                const std::string &spendKeyString)
{
    WalletImpl * wallet = new WalletImpl(testnet);
    if(restoreHeight > 0){
        wallet->setRefreshFromBlockHeight(restoreHeight);
    }
    wallet->recoverFromKeys(path, language, addressString, viewKeyString, spendKeyString);
    return wallet;
}

bool WalletManagerImpl::closeWallet(Wallet *wallet)
{
    WalletImpl * wallet_ = dynamic_cast<WalletImpl*>(wallet);
    bool result = wallet_->close();
    if (!result) {
        m_errorString = wallet_->errorString();
    } else {
        delete wallet_;
    }
    return result;
}

bool WalletManagerImpl::walletExists(const std::string &path)
{
    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    if(keys_file_exists){
        return true;
    }
    return false;
}

bool WalletManagerImpl::verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool watch_only) const
{
	    return tools::wallet2::verify_password(keys_file_name, password, watch_only);
}

std::vector<std::string> WalletManagerImpl::findWallets(const std::string &path)
{
    std::vector<std::string> result;
    boost::filesystem::path work_dir(path);
    // return empty result if path doesn't exist
    if(!boost::filesystem::is_directory(path)){
        return result;
    }
    const boost::regex wallet_rx("(.*)\\.(keys)$"); // searching for <wallet_name>.keys files
    boost::filesystem::recursive_directory_iterator end_itr; // Default ctor yields past-the-end
    for (boost::filesystem::recursive_directory_iterator itr(path); itr != end_itr; ++itr) {
        // Skip if not a file
        if (!boost::filesystem::is_regular_file(itr->status()))
            continue;
        boost::smatch what;
        std::string filename = itr->path().filename().string();

        LOG_PRINT_L3("Checking filename: " << filename);

        bool matched = boost::regex_match(filename, what, wallet_rx);
        if (matched) {
            // if keys file found, checking if there's wallet file itself
            std::string wallet_file = (itr->path().parent_path() /= what[1].str()).string();
            if (boost::filesystem::exists(wallet_file)) {
                LOG_PRINT_L3("Found wallet: " << wallet_file);
                result.push_back(wallet_file);
            }
        }
    }
    return result;
}

std::string WalletManagerImpl::errorString() const
{
    return m_errorString;
}

void WalletManagerImpl::setDaemonAddress(const std::string &address)
{
    m_daemonAddress = address;
}

bool WalletManagerImpl::connected(uint32_t *version) const
{
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
    req_t.jsonrpc = "2.0";
    req_t.id = epee::serialization::storage_entry(0);
    req_t.method = "get_version";
    if (!connect_and_invoke(m_daemonAddress, "/json_rpc", req_t, resp_t))
      return false;

    if (version)
        *version = resp_t.result.version;
    return true;
}

bool WalletManagerImpl::checkPayment(const std::string &address_text, const std::string &txid_text, const std::string &txkey_text, const std::string &daemon_address, uint64_t &received, uint64_t &height, std::string &error) const
{
  error = "";
  cryptonote::blobdata txid_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(txid_text, txid_data) || txid_data.size() != sizeof(crypto::hash))
  {
    error = tr("failed to parse txid");
    return false;
  }
  crypto::hash txid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

  if (txkey_text.size() < 64 || txkey_text.size() % 64)
  {
    error = tr("failed to parse tx key");
    return false;
  }
  crypto::secret_key tx_key;
  cryptonote::blobdata tx_key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(txkey_text, tx_key_data) || tx_key_data.size() != sizeof(crypto::hash))
  {
    error = tr("failed to parse tx key");
    return false;
  }
  tx_key = *reinterpret_cast<const crypto::secret_key*>(tx_key_data.data());

  bool testnet = address_text[0] != '4';
  cryptonote::account_public_address address;
  bool has_payment_id;
  crypto::hash8 payment_id;
  if(!cryptonote::get_account_integrated_address_from_str(address, has_payment_id, payment_id, testnet, address_text))
  {
    error = tr("failed to parse address");
    return false;
  }

  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
  cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;
  req.txs_hashes.push_back(epee::string_tools::pod_to_hex(txid));
  if (!connect_and_invoke(m_daemonAddress, "/gettransactions", req, res) ||
      (res.txs.size() != 1 && res.txs_as_hex.size() != 1))
  {
    error = tr("failed to get transaction from daemon");
    return false;
  }
  cryptonote::blobdata tx_data;
  bool ok;
  if (res.txs.size() == 1)
    ok = epee::string_tools::parse_hexstr_to_binbuff(res.txs.front().as_hex, tx_data);
  else
    ok = epee::string_tools::parse_hexstr_to_binbuff(res.txs_as_hex.front(), tx_data);
  if (!ok)
  {
    error = tr("failed to parse transaction from daemon");
    return false;
  }
  crypto::hash tx_hash, tx_prefix_hash;
  cryptonote::transaction tx;
  if (!cryptonote::parse_and_validate_tx_from_blob(tx_data, tx, tx_hash, tx_prefix_hash))
  {
    error = tr("failed to validate transaction from daemon");
    return false;
  }
  if (tx_hash != txid)
  {
    error = tr("failed to get the right transaction from daemon");
    return false;
  }

  crypto::key_derivation derivation;
  if (!crypto::generate_key_derivation(address.m_view_public_key, tx_key, derivation))
  {
    error = tr("failed to generate key derivation from supplied parameters");
    return false;
  }

  received = 0;
  try {
    for (size_t n = 0; n < tx.vout.size(); ++n)
    {
      if (typeid(cryptonote::txout_to_key) != tx.vout[n].target.type())
        continue;
      const cryptonote::txout_to_key tx_out_to_key = boost::get<cryptonote::txout_to_key>(tx.vout[n].target);
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
    error = std::string(tr("error: ")) + e.what();
    return false;
  }

  if (received > 0)
  {
    LOG_PRINT_L1(get_account_address_as_str(testnet, address) << " " << tr("received") << " " << cryptonote::print_money(received) << " " << tr("in txid") << " " << txid);
  }
  else
  {
    LOG_PRINT_L1(get_account_address_as_str(testnet, address) << " " << tr("received nothing in txid") << " " << txid);
  }
  if (res.txs.front().in_pool)
  {
    height = 0;
  }
  else
  {
    height = res.txs.front().block_height;
  }

  return true;
}

uint64_t WalletManagerImpl::blockchainHeight() const
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!connect_and_invoke(m_daemonAddress, "/getinfo", ireq, ires))
      return 0;
    return ires.height;
}

uint64_t WalletManagerImpl::blockchainTargetHeight() const
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!connect_and_invoke(m_daemonAddress, "/getinfo", ireq, ires))
      return 0;
    return ires.target_height >= ires.height ? ires.target_height : ires.height;
}

uint64_t WalletManagerImpl::networkDifficulty() const
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!connect_and_invoke(m_daemonAddress, "/getinfo", ireq, ires))
      return 0;
    return ires.difficulty;
}

double WalletManagerImpl::miningHashRate() const
{
    cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
    cryptonote::COMMAND_RPC_MINING_STATUS::response mres;

    epee::net_utils::http::http_simple_client http_client;
    if (!connect_and_invoke(m_daemonAddress, "/mining_status", mreq, mres))
      return 0.0;
    if (!mres.active)
      return 0.0;
    return mres.speed;
}

uint64_t WalletManagerImpl::blockTarget() const
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!connect_and_invoke(m_daemonAddress, "/getinfo", ireq, ires))
        return 0;
    return ires.target;
}

bool WalletManagerImpl::isMining() const
{
    cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
    cryptonote::COMMAND_RPC_MINING_STATUS::response mres;

    if (!connect_and_invoke(m_daemonAddress, "/mining_status", mreq, mres))
      return false;
    return mres.active;
}

bool WalletManagerImpl::startMining(const std::string &address, uint32_t threads, bool background_mining, bool ignore_battery)
{
    cryptonote::COMMAND_RPC_START_MINING::request mreq;
    cryptonote::COMMAND_RPC_START_MINING::response mres;

    mreq.miner_address = address;
    mreq.threads_count = threads;
    mreq.ignore_battery = ignore_battery;
    mreq.do_background_mining = background_mining;

    if (!connect_and_invoke(m_daemonAddress, "/start_mining", mreq, mres))
      return false;
    return mres.status == CORE_RPC_STATUS_OK;
}

bool WalletManagerImpl::stopMining()
{
    cryptonote::COMMAND_RPC_STOP_MINING::request mreq;
    cryptonote::COMMAND_RPC_STOP_MINING::response mres;

    if (!connect_and_invoke(m_daemonAddress, "/stop_mining", mreq, mres))
      return false;
    return mres.status == CORE_RPC_STATUS_OK;
}

std::string WalletManagerImpl::resolveOpenAlias(const std::string &address, bool &dnssec_valid) const
{
    std::vector<std::string> addresses = tools::dns_utils::addresses_from_url(address, dnssec_valid);
    if (addresses.empty())
        return "";
    return addresses.front();
}

std::tuple<bool, std::string, std::string, std::string, std::string> WalletManager::checkUpdates(const std::string &software, const std::string &subdir)
{
#ifdef BUILD_TAG
    static const char buildtag[] = BOOST_PP_STRINGIZE(BUILD_TAG);
#else
    static const char buildtag[] = "source";
#endif

    std::string version, hash;
    MDEBUG("Checking for a new " << software << " version for " << buildtag);
    if (!tools::check_updates(software, buildtag, version, hash))
      return std::make_tuple(false, "", "", "", "");

    if (tools::vercmp(version.c_str(), MONERO_VERSION) > 0)
    {
      std::string user_url = tools::get_update_url(software, subdir, buildtag, version, true);
      std::string auto_url = tools::get_update_url(software, subdir, buildtag, version, false);
      MGINFO("Version " << version << " of " << software << " for " << buildtag << " is available: " << user_url << ", SHA256 hash " << hash);
      return std::make_tuple(true, version, hash, user_url, auto_url);
    }
    return std::make_tuple(false, "", "", "", "");
}


///////////////////// WalletManagerFactory implementation //////////////////////
WalletManager *WalletManagerFactory::getWalletManager()
{

    static WalletManagerImpl * g_walletManager = nullptr;

    if  (!g_walletManager) {
        g_walletManager = new WalletManagerImpl();
    }

    return g_walletManager;
}

void WalletManagerFactory::setLogLevel(int level)
{
    mlog_set_log_level(level);
}

void WalletManagerFactory::setLogCategories(const std::string &categories)
{
    mlog_set_log(categories.c_str());
}



}

namespace Bitmonero = Monero;
