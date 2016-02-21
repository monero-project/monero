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

unsigned int epee::g_test_dbg_lock_sleep = 0;

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
    std::string seed() const;
    std::string getSeedLanguage() const;
    void setSeedLanguage(const std::string &arg);

private:
    //std::unique_ptr<tools::wallet2> m_wallet;
    tools::wallet2 * m_wallet;

};


WalletImpl::WalletImpl()
    :m_wallet(nullptr)
{

}

WalletImpl::~WalletImpl()
{
    delete m_wallet;
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language)
{
    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    // TODO: figure out how to setup logger;
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);


    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {

        LOG_ERROR("attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.");
        return false;

    }
    // TODO: validate language
    // TODO: create wallet
    //m_wallet.reset(new tools::wallet2());
    m_wallet = new tools::wallet2();
    m_wallet->set_seed_language(language);

    crypto::secret_key recovery_val, secret_key;
    try {
        recovery_val = m_wallet->generate(path, password, secret_key, false, false);
    } catch (const std::exception& e) {
        // TODO: log exception
        return false;
    }

    return true;
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


///////////////////////// WalletManager implementation /////////////////////////
class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language);
    Wallet * openWallet(const std::string &path, const std::string &password);
    bool walletExists(const std::string &path);
    int lastError() const;

private:
    WalletManagerImpl() {}
    friend struct WalletManagerFactory;
};

Wallet *WalletManagerImpl::createWallet(const std::string &path, const std::string &password,
                                    const std::string &language)
{
    WalletImpl * wallet = new WalletImpl();
    // TODO open wallet, set password, etc
    if (!wallet->create(path, password, language)) {
        delete wallet;
        wallet = nullptr;
    }
    return wallet;
}


Wallet *WalletManagerImpl::openWallet(const std::string &path, const std::string &password)
{
    return nullptr;
}

bool WalletManagerImpl::walletExists(const std::string &path)
{
    return false;
}

int WalletManagerImpl::lastError() const
{
    return 0;
}


///////////////////// WalletManagerFactory implementation //////////////////////
WalletManager *WalletManagerFactory::getWalletManager()
{
    if  (!g_walletManager) {
        g_walletManager = new WalletManagerImpl();
    }

    return g_walletManager;
}





}
