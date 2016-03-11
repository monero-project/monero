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

#include "wallet2_api.h"
#include "wallet2.h"
#include <memory>

namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

namespace Bitmonero {

struct WalletManagerImpl;

namespace {
    static WalletManagerImpl * g_walletManager = nullptr;



}

Wallet::~Wallet() {}

///////////////////////// Wallet implementation ////////////////////////////////
class WalletImpl : public Wallet
{
public:
    WalletImpl();
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language);
    bool open(const std::string &path, const std::string &password);
    bool close();
    std::string seed() const;
    std::string getSeedLanguage() const;
    void setSeedLanguage(const std::string &arg);
    void setListener(Listener *) {}
    int status() const;
    std::string errorString() const;
    bool setPassword(const std::string &password);

private:
    void clearStatus();

private:
    //std::unique_ptr<tools::wallet2> m_wallet;
    tools::wallet2 * m_wallet;
    int  m_status;
    std::string m_errorString;

};

WalletImpl::WalletImpl()
    :m_wallet(nullptr), m_status(Wallet::Status_Ok)
{
    m_wallet = new tools::wallet2();
}

WalletImpl::~WalletImpl()
{
    delete m_wallet;
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language)
{

    clearStatus();

    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    // TODO: figure out how to setup logger;
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);


    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        m_errorString = "attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
        return false;
    }
    // TODO: validate language
    m_wallet->set_seed_language(language);
    crypto::secret_key recovery_val, secret_key;
    try {
        recovery_val = m_wallet->generate(path, password, secret_key, false, false);
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
    return true;
}

bool WalletImpl::open(const std::string &path, const std::string &password)
{
    clearStatus();
    bool result = false;
    try {
        // TODO: handle "deprecated"
        m_wallet->load(path, password);
        result = true;
    } catch (const std::exception &e) {
        LOG_ERROR("Error opening wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return result;
}

bool WalletImpl::close()
{
    bool result = false;
    try {
        m_wallet->store();
        m_wallet->stop();
        result = true;
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
        LOG_ERROR("Error closing wallet: " << e.what());
    }
    return result;
}

std::string WalletImpl::seed() const
{
    std::string seed;
    if (m_wallet)
        m_wallet->get_seed(seed);
    return seed;
}

std::string WalletImpl::getSeedLanguage() const
{
    return m_wallet->get_seed_language();
}

void WalletImpl::setSeedLanguage(const std::string &arg)
{
    m_wallet->set_seed_language(arg);
}

int WalletImpl::status() const
{
    return m_status;
}

std::string WalletImpl::errorString() const
{
    return m_errorString;
}

bool WalletImpl::setPassword(const std::string &password)
{
    bool result  = false;
    try {
        m_wallet->rewrite(m_wallet->get_wallet_file(), password);
        result = true;
    } catch (const std::exception &e) {
        result = false;
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return result;
}

void WalletImpl::clearStatus()
{
    m_status = Status_Ok;
    m_errorString.clear();
}



///////////////////////// WalletManager implementation /////////////////////////
class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language);
    Wallet * openWallet(const std::string &path, const std::string &password);
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &memo, const std::string &language);
    virtual bool closeWallet(Wallet *wallet);
    bool walletExists(const std::string &path);
    std::string errorString() const;


private:
    WalletManagerImpl() {}
    friend struct WalletManagerFactory;

    std::string m_errorString;
};

Wallet *WalletManagerImpl::createWallet(const std::string &path, const std::string &password,
                                    const std::string &language)
{
    WalletImpl * wallet = new WalletImpl();
    wallet->create(path, password, language);
    return wallet;
}

Wallet *WalletManagerImpl::openWallet(const std::string &path, const std::string &password)
{
    WalletImpl * wallet = new WalletImpl();
    wallet->open(path, password);
    return wallet;
}

Wallet *WalletManagerImpl::recoveryWallet(const std::string &path, const std::string &memo, const std::string &language)
{
    return nullptr;

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
    return false;
}

std::string WalletManagerImpl::errorString() const
{
    return m_errorString;
}



///////////////////// WalletManagerFactory implementation //////////////////////
WalletManager *WalletManagerFactory::getWalletManager()
{
    // TODO: initialize logger here
    epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_0);
    if  (!g_walletManager) {
        g_walletManager = new WalletManagerImpl();
    }

    return g_walletManager;
}

}
