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
#include "mnemonics/electrum-words.h"
#include "cryptonote_core/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_basic_impl.h"
#include "cryptonote_core/cryptonote_format_utils.h"


#include <memory>
#include <vector>
#include <sstream>
#include <boost/format.hpp>


#define tr(x) (x)

namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}

namespace Bitmonero {

struct WalletManagerImpl;

namespace {
    static WalletManagerImpl * g_walletManager = nullptr;
    // copy-pasted from
    static const size_t DEFAULT_MIX = 4;

}



using namespace std;
using namespace cryptonote;

Wallet::~Wallet() {}

PendingTransaction::~PendingTransaction() {}


class WalletImpl;

///////////////////////// Transaction implementation ///////////////////////////

class TransactionImpl : public PendingTransaction
{
public:
    TransactionImpl(WalletImpl * wallet);
    ~TransactionImpl();
    int status() const;
    std::string errorString() const;
    bool commit();
    uint64_t amount() const;
    uint64_t dust() const;
    uint64_t fee() const;
    // TODO: continue with interface;

private:
    friend class WalletImpl;
    WalletImpl * m_wallet;

    int  m_status;
    std::string m_errorString;
    std::vector<tools::wallet2::pending_tx> m_pending_tx;
};




/////////////////////////////////////////////////////////////////////////////////
string Wallet::displayAmount(uint64_t amount)
{
    return cryptonote::print_money(amount);
}



///////////////////////// Wallet implementation ////////////////////////////////
class WalletImpl : public Wallet
{
public:
    WalletImpl(bool testnet = false);
    ~WalletImpl();
    bool create(const std::string &path, const std::string &password,
                const std::string &language);
    bool open(const std::string &path, const std::string &password);
    bool recover(const std::string &path, const std::string &seed);
    bool close();
    std::string seed() const;
    std::string getSeedLanguage() const;
    void setSeedLanguage(const std::string &arg);
    void setListener(Listener *) {}
    int status() const;
    std::string errorString() const;
    bool setPassword(const std::string &password);
    std::string address() const;
    bool store(const std::string &path);
    bool init(const std::string &daemon_address, uint64_t upper_transaction_size_limit);
    bool connectToDaemon();
    uint64_t balance() const;
    uint64_t unlockedBalance() const;
    bool refresh();
    PendingTransaction * createTransaction(const std::string &dst_addr, uint64_t amount);
    virtual void disposeTransaction(PendingTransaction * t);

private:
    void clearStatus();

private:
    friend class TransactionImpl;
    tools::wallet2 * m_wallet;
    int  m_status;
    std::string m_errorString;
    std::string m_password;
};

WalletImpl::WalletImpl(bool testnet)
    :m_wallet(nullptr), m_status(Wallet::Status_Ok)
{
    m_wallet = new tools::wallet2(testnet);
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
        m_password = password;
        m_status = Status_Ok;
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
    try {
        // TODO: handle "deprecated"
        m_wallet->load(path, password);

        m_password = password;
    } catch (const std::exception &e) {
        LOG_ERROR("Error opening wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

bool WalletImpl::recover(const std::string &path, const std::string &seed)
{
    clearStatus();
    m_errorString.clear();
    if (seed.empty()) {
        m_errorString = "Electrum seed is empty";
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
        return false;
    }

    crypto::secret_key recovery_key;
    std::string old_language;
    if (!crypto::ElectrumWords::words_to_bytes(seed, recovery_key, old_language)) {
        m_errorString = "Electrum-style word list failed verification";
        m_status = Status_Error;
        return false;
    }

    try {
        m_wallet->set_seed_language(old_language);
        m_wallet->generate(path, "", recovery_key, true, false);
        // TODO: wallet->init(daemon_address);
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

bool WalletImpl::close()
{
    clearStatus();
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
    clearStatus();
    try {
        m_wallet->rewrite(m_wallet->get_wallet_file(), password);
        m_password = password;
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

std::string WalletImpl::address() const
{
    return m_wallet->get_account().get_public_address_str(m_wallet->testnet());
}

bool WalletImpl::store(const std::string &path)
{
    clearStatus();
    try {
        if (path.empty()) {
            m_wallet->store();
        } else {
            m_wallet->store_to(path, m_password);
        }
    } catch (const std::exception &e) {
        LOG_ERROR("Error storing wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
    }

    return m_status == Status_Ok;
}

bool WalletImpl::init(const std::string &daemon_address, uint64_t upper_transaction_size_limit)
{
    clearStatus();
    try {
        m_wallet->init(daemon_address, upper_transaction_size_limit);
    } catch (const std::exception &e) {
        LOG_ERROR("Error initializing wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
    }

    return m_status == Status_Ok;
}

uint64_t WalletImpl::balance() const
{
    return m_wallet->balance();
}

uint64_t WalletImpl::unlockedBalance() const
{
    return m_wallet->unlocked_balance();
}


bool WalletImpl::refresh()
{
    clearStatus();
    try {
        m_wallet->refresh();
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

// TODO:
// 1 - properly handle payment id (add another menthod with explicit 'payment_id' param)
// 2 - check / design how "Transaction" can be single interface
// (instead of few different data structures within wallet2 implementation:
//    - pending_tx;
//    - transfer_details;
//    - payment_details;
//    - unconfirmed_transfer_details;
//    - confirmed_transfer_details)
PendingTransaction *WalletImpl::createTransaction(const string &dst_addr, uint64_t amount)
{
    clearStatus();
    vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::tx_destination_entry de;
    bool has_payment_id;
    crypto::hash8 new_payment_id;

    // TODO:  (https://bitcointalk.org/index.php?topic=753252.msg9985441#msg9985441)
    size_t fake_outs_count = m_wallet->default_mixin();
    if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIX;

    TransactionImpl * transaction = new TransactionImpl(this);

    do {

        if(!cryptonote::get_account_integrated_address_from_str(de.addr, has_payment_id, new_payment_id, m_wallet->testnet(), dst_addr)) {
            // TODO: copy-paste 'if treating as an address fails, try as url' from simplewallet.cpp:1982
            m_status = Status_Error;
            m_errorString = "Invalid destination address";
            break;
        }

        de.amount = amount;
        if (de.amount <= 0) {
            m_status = Status_Error;
            m_errorString = "Invalid amount";
            break;
        }

        dsts.push_back(de);
        //std::vector<tools::wallet2::pending_tx> ptx_vector;
        std::vector<uint8_t> extra;


        try {
            transaction->m_pending_tx = m_wallet->create_transactions(dsts, fake_outs_count, 0 /* unlock_time */, 0 /* unused fee arg*/, extra);

        } catch (const tools::error::daemon_busy&) {
            // TODO: make it translatable with "tr"?
            m_errorString = tr("daemon is busy. Please try again later.");
            m_status = Status_Error;
        } catch (const tools::error::no_connection_to_daemon&) {
            m_errorString = tr("no connection to daemon. Please make sure daemon is running.");
            m_status = Status_Error;
        } catch (const tools::error::wallet_rpc_error& e) {
            m_errorString = tr("RPC error: ") +  e.to_string();
            m_status = Status_Error;
        } catch (const tools::error::get_random_outs_error&) {
            m_errorString = tr("failed to get random outputs to mix");
            m_status = Status_Error;

        } catch (const tools::error::not_enough_money& e) {
            m_status = Status_Error;
            std::ostringstream writer(m_errorString);

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());

        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer(m_errorString);
            writer << tr("not enough outputs for specified mixin_count") << " = " << e.mixin_count() << ":";
            for (const cryptonote::COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS::outs_for_amount& outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.amount) << ", " << tr("found outputs to mix") << " = " << outs_for_amount.outs.size();
            }
            m_status = Status_Error;
        } catch (const tools::error::tx_not_constructed&) {
            m_errorString = tr("transaction was not constructed");
            m_status = Status_Error;
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer(m_errorString);
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            m_status = Status_Error;
        } catch (const tools::error::tx_sum_overflow& e) {
            m_errorString = e.what();
            m_status = Status_Error;
        } catch (const tools::error::zero_destination&) {
            m_errorString =  tr("one of destinations is zero");
            m_status = Status_Error;
        } catch (const tools::error::tx_too_big& e) {
            m_errorString =  tr("failed to find a suitable way to split transactions");
            m_status = Status_Error;
        } catch (const tools::error::transfer_error& e) {
            m_errorString = string(tr("unknown transfer error: ")) + e.what();
            m_status = Status_Error;
        } catch (const tools::error::wallet_internal_error& e) {
            m_errorString =  string(tr("internal error: ")) + e.what();
            m_status = Status_Error;
        } catch (const std::exception& e) {
            m_errorString =  string(tr("unexpected error: ")) + e.what();
            m_status = Status_Error;
        } catch (...) {
            m_errorString = tr("unknown error");
            m_status = Status_Error;
        }
    } while (false);

    transaction->m_status = m_status;
    transaction->m_errorString = m_errorString;
    return transaction;
}

void WalletImpl::disposeTransaction(PendingTransaction *t)
{
    delete t;
}

bool WalletImpl::connectToDaemon()
{
    bool result = m_wallet->check_connection();
    m_status = result ? Status_Ok : Status_Error;
    if (!result) {
        m_errorString = "Error connecting to daemon at " + m_wallet->get_daemon_address();
    }
    return result;
}

void WalletImpl::clearStatus()
{
    m_status = Status_Ok;
    m_errorString.clear();
}




TransactionImpl::TransactionImpl(WalletImpl *wallet)
    : m_wallet(wallet)
{

}

TransactionImpl::~TransactionImpl()
{

}

int TransactionImpl::status() const
{
    return m_status;
}

string TransactionImpl::errorString() const
{
    return m_errorString;
}

bool TransactionImpl::commit()
{

    LOG_PRINT_L0("m_pending_tx size: " << m_pending_tx.size());
    assert(m_pending_tx.size() == 1);
    try {
        while (!m_pending_tx.empty()) {
            auto & ptx = m_pending_tx.back();
            m_wallet->m_wallet->commit_tx(ptx);
            // success_msg_writer(true) << tr("Money successfully sent, transaction ") << get_transaction_hash(ptx.tx);
            // if no exception, remove element from vector
            m_pending_tx.pop_back();
        } // TODO: extract method;
    } catch (const tools::error::daemon_busy&) {
        // TODO: make it translatable with "tr"?
        m_errorString = tr("daemon is busy. Please try again later.");
        m_status = Status_Error;
    } catch (const tools::error::no_connection_to_daemon&) {
        m_errorString = tr("no connection to daemon. Please make sure daemon is running.");
        m_status = Status_Error;
    } catch (const tools::error::tx_rejected& e) {
        std::ostringstream writer(m_errorString);
        writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
        m_status = Status_Error;
    } catch (std::exception &e) {
        m_errorString = string(tr("Unknown exception: ")) + e.what();
        m_status = Status_Error;
    } catch (...) {
        m_errorString = tr("Unhandled exception");
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
    }

    return m_status == Status_Ok;
}

uint64_t TransactionImpl::amount() const
{
    uint64_t result = 0;
    for (const auto &ptx : m_pending_tx)   {
        for (const auto &dest : ptx.dests) {
            result += dest.amount;
        }
    }
    return result;
}

uint64_t TransactionImpl::dust() const
{
    uint32_t result = 0;
    for (const auto & ptx : m_pending_tx) {
        result += ptx.dust;
    }
    return result;
}

uint64_t TransactionImpl::fee() const
{
    uint32_t result = 0;
    for (const auto ptx : m_pending_tx) {
        result += ptx.fee;
    }
    return result;
}



///////////////////////// WalletManager implementation /////////////////////////
class WalletManagerImpl : public WalletManager
{
public:
    Wallet * createWallet(const std::string &path, const std::string &password,
                          const std::string &language, bool testnet);
    Wallet * openWallet(const std::string &path, const std::string &password, bool testnet);
    virtual Wallet * recoveryWallet(const std::string &path, const std::string &memo, bool testnet);
    virtual bool closeWallet(Wallet *wallet);
    bool walletExists(const std::string &path);
    std::string errorString() const;
    void setDaemonHost(const std::string &hostname);


private:
    WalletManagerImpl() {}
    friend struct WalletManagerFactory;

    std::string m_errorString;
};

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
    return wallet;
}

Wallet *WalletManagerImpl::recoveryWallet(const std::string &path, const std::string &memo, bool testnet)
{
    WalletImpl * wallet = new WalletImpl(testnet);
    wallet->recover(path, memo);
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
    return false;
}

std::string WalletManagerImpl::errorString() const
{
    return m_errorString;
}

void WalletManagerImpl::setDaemonHost(const std::string &hostname)
{

}



///////////////////// WalletManagerFactory implementation //////////////////////
WalletManager *WalletManagerFactory::getWalletManager()
{

    if  (!g_walletManager) {
        epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_MAX);
        g_walletManager = new WalletManagerImpl();
    }

    return g_walletManager;
}




}
