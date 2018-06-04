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

namespace Monero {

Wallet *WalletManagerImpl::createWallet(const std::string &path, const std::string &password,
                                    const std::string &language, NetworkType nettype)
{
    WalletImpl * wallet = new WalletImpl(nettype);
    wallet->create(path, password, language);
    return wallet;
}

Wallet *WalletManagerImpl::openWallet(const std::string &path, const std::string &password, NetworkType nettype)
{
    WalletImpl * wallet = new WalletImpl(nettype);
    wallet->open(path, password);
    //Refresh addressBook
    wallet->addressBook()->refresh(); 
    return wallet;
}

Wallet *WalletManagerImpl::recoveryWallet(const std::string &path, const std::string &mnemonic, NetworkType nettype, uint64_t restoreHeight)
{
    return recoveryWallet(path, "", mnemonic, nettype, restoreHeight);
}

Wallet *WalletManagerImpl::createWalletFromKeys(const std::string &path,
                                                const std::string &language,
                                                NetworkType nettype,
                                                uint64_t restoreHeight,
                                                const std::string &addressString,
                                                const std::string &viewKeyString,
                                                const std::string &spendKeyString)
{
    return createWalletFromKeys(path, "", language, nettype, restoreHeight,
                                addressString, viewKeyString, spendKeyString);
}

Wallet *WalletManagerImpl::recoveryWallet(const std::string &path,
                                                const std::string &password,
                                                const std::string &mnemonic,
                                                NetworkType nettype,
                                                uint64_t restoreHeight)
{
    WalletImpl * wallet = new WalletImpl(nettype);
    if(restoreHeight > 0){
        wallet->setRefreshFromBlockHeight(restoreHeight);
    }
    wallet->recover(path, password, mnemonic);
    return wallet;
}

Wallet *WalletManagerImpl::createWalletFromKeys(const std::string &path,
                                                const std::string &password,
                                                const std::string &language,
                                                NetworkType nettype, 
                                                uint64_t restoreHeight,
                                                const std::string &addressString,
                                                const std::string &viewKeyString,
                                                const std::string &spendKeyString)
{
    WalletImpl * wallet = new WalletImpl(nettype);
    if(restoreHeight > 0){
        wallet->setRefreshFromBlockHeight(restoreHeight);
    }
    wallet->recoverFromKeysWithPassword(path, password, language, addressString, viewKeyString, spendKeyString);
    return wallet;
}

Wallet *WalletManagerImpl::createWalletFromDevice(const std::string &path,
                                                  const std::string &password,
                                                  NetworkType nettype,
                                                  const std::string &deviceName,
                                                  uint64_t restoreHeight,
                                                  const std::string &subaddressLookahead)
{
    WalletImpl * wallet = new WalletImpl(nettype);
    if(restoreHeight > 0){
        wallet->setRefreshFromBlockHeight(restoreHeight);
    }
    auto lookahead = tools::parse_subaddress_lookahead(subaddressLookahead);
    if (lookahead)
    {
        wallet->setSubaddressLookahead(lookahead->first, lookahead->second);
    }
    wallet->recoverFromDevice(path, password, deviceName);
    return wallet;
}

bool WalletManagerImpl::closeWallet(Wallet *wallet, bool store)
{
    WalletImpl * wallet_ = dynamic_cast<WalletImpl*>(wallet);
    if (!wallet_)
        return false;
    bool result = wallet_->close(store);
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

bool WalletManagerImpl::verifyWalletPassword(const std::string &keys_file_name, const std::string &password, bool no_spend_key) const
{
	    return tools::wallet2::verify_password(keys_file_name, password, no_spend_key, hw::get_device("default"));
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
    if(m_http_client.is_connected())
        m_http_client.disconnect();
    m_http_client.set_server(address, boost::none);
}

bool WalletManagerImpl::connected(uint32_t *version)
{
    epee::json_rpc::request<cryptonote::COMMAND_RPC_GET_VERSION::request> req_t = AUTO_VAL_INIT(req_t);
    epee::json_rpc::response<cryptonote::COMMAND_RPC_GET_VERSION::response, std::string> resp_t = AUTO_VAL_INIT(resp_t);
    req_t.jsonrpc = "2.0";
    req_t.id = epee::serialization::storage_entry(0);
    req_t.method = "get_version";
    if (!epee::net_utils::invoke_http_json("/json_rpc", req_t, resp_t, m_http_client))
      return false;

    if (version)
        *version = resp_t.result.version;
    return true;
}

uint64_t WalletManagerImpl::blockchainHeight()
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!epee::net_utils::invoke_http_json("/getinfo", ireq, ires, m_http_client))
      return 0;
    return ires.height;
}

uint64_t WalletManagerImpl::blockchainTargetHeight()
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!epee::net_utils::invoke_http_json("/getinfo", ireq, ires, m_http_client))
      return 0;
    return ires.target_height >= ires.height ? ires.target_height : ires.height;
}

uint64_t WalletManagerImpl::networkDifficulty()
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!epee::net_utils::invoke_http_json("/getinfo", ireq, ires, m_http_client))
      return 0;
    return ires.difficulty;
}

double WalletManagerImpl::miningHashRate()
{
    cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
    cryptonote::COMMAND_RPC_MINING_STATUS::response mres;

    epee::net_utils::http::http_simple_client http_client;
    if (!epee::net_utils::invoke_http_json("/mining_status", mreq, mres, m_http_client))
      return 0.0;
    if (!mres.active)
      return 0.0;
    return mres.speed;
}

uint64_t WalletManagerImpl::blockTarget()
{
    cryptonote::COMMAND_RPC_GET_INFO::request ireq;
    cryptonote::COMMAND_RPC_GET_INFO::response ires;

    if (!epee::net_utils::invoke_http_json("/getinfo", ireq, ires, m_http_client))
        return 0;
    return ires.target;
}

bool WalletManagerImpl::isMining()
{
    cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
    cryptonote::COMMAND_RPC_MINING_STATUS::response mres;

    if (!epee::net_utils::invoke_http_json("/mining_status", mreq, mres, m_http_client))
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

    if (!epee::net_utils::invoke_http_json("/start_mining", mreq, mres, m_http_client))
      return false;
    return mres.status == CORE_RPC_STATUS_OK;
}

bool WalletManagerImpl::stopMining()
{
    cryptonote::COMMAND_RPC_STOP_MINING::request mreq;
    cryptonote::COMMAND_RPC_STOP_MINING::response mres;

    if (!epee::net_utils::invoke_http_json("/stop_mining", mreq, mres, m_http_client))
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

std::tuple<bool, std::string, std::string, std::string, std::string> WalletManager::checkUpdates(const std::string &software, std::string subdir)
{
#ifdef BUILD_TAG
    static const char buildtag[] = BOOST_PP_STRINGIZE(BUILD_TAG);
#else
    static const char buildtag[] = "source";
    // Override the subdir string when built from source
    subdir = "source";
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
