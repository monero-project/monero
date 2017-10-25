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


#include "wallet.h"
#include "pending_transaction.h"
#include "unsigned_transaction.h"
#include "transaction_history.h"
#include "address_book.h"
#include "common_defines.h"
#include "common/util.h"

#include "mnemonics/electrum-words.h"
#include <boost/format.hpp>
#include <sstream>
#include <unordered_map>

using namespace std;
using namespace cryptonote;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "WalletAPI"

namespace Monero {

namespace {
    // copy-pasted from simplewallet
    static const size_t DEFAULT_MIXIN = 4;
    static const int    DEFAULT_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // limit maximum refresh interval as one minute
    static const int    MAX_REFRESH_INTERVAL_MILLIS = 1000 * 60 * 1;
    // Default refresh interval when connected to remote node
    static const int    DEFAULT_REMOTE_NODE_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // Connection timeout 30 sec
    static const int    DEFAULT_CONNECTION_TIMEOUT_MILLIS = 1000 * 30;
}

struct Wallet2CallbackImpl : public tools::i_wallet2_callback
{

    Wallet2CallbackImpl(WalletImpl * wallet)
     : m_listener(nullptr)
     , m_wallet(wallet)
    {

    }

    ~Wallet2CallbackImpl()
    {

    }

    void setListener(WalletListener * listener)
    {
        m_listener = listener;
    }

    WalletListener * getListener() const
    {
        return m_listener;
    }

    virtual void on_new_block(uint64_t height, const cryptonote::block& block)
    {
        // Don't flood the GUI with signals. On fast refresh - send signal every 1000th block
        // get_refresh_from_block_height() returns the blockheight from when the wallet was 
        // created or the restore height specified when wallet was recovered
        if(height >= m_wallet->m_wallet->get_refresh_from_block_height() || height % 1000 == 0) {
            // LOG_PRINT_L3(__FUNCTION__ << ": new block. height: " << height);
            if (m_listener) {
                m_listener->newBlock(height);
            }
        }
    }

    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);

        LOG_PRINT_L3(__FUNCTION__ << ": money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount));
        // do not signal on received tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->moneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);

        LOG_PRINT_L3(__FUNCTION__ << ": unconfirmed money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount));
        // do not signal on received tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->unconfirmedMoneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx,
                                uint64_t amount, const cryptonote::transaction& spend_tx)
    {
        // TODO;
        std::string tx_hash = epee::string_tools::pod_to_hex(txid);
        LOG_PRINT_L3(__FUNCTION__ << ": money spent. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount));
        // do not signal on sent tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->moneySpent(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx)
    {
        // TODO;
    }

    WalletListener * m_listener;
    WalletImpl     * m_wallet;
};

Wallet::~Wallet() {}

WalletListener::~WalletListener() {}


string Wallet::displayAmount(uint64_t amount)
{
    return cryptonote::print_money(amount);
}

uint64_t Wallet::amountFromString(const string &amount)
{
    uint64_t result = 0;
    cryptonote::parse_amount(result, amount);
    return result;
}

uint64_t Wallet::amountFromDouble(double amount)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(CRYPTONOTE_DISPLAY_DECIMAL_POINT) << amount;
    return amountFromString(ss.str());
}

std::string Wallet::genPaymentId()
{
    crypto::hash8 payment_id = crypto::rand<crypto::hash8>();
    return epee::string_tools::pod_to_hex(payment_id);

}

bool Wallet::paymentIdValid(const string &paiment_id)
{
    crypto::hash8 pid8;
    if (tools::wallet2::parse_short_payment_id(paiment_id, pid8))
        return true;
    crypto::hash pid;
    if (tools::wallet2::parse_long_payment_id(paiment_id, pid))
        return true;
    return false;
}

bool Wallet::addressValid(const std::string &str, bool testnet)
{
  bool has_payment_id;
  cryptonote::account_public_address address;
  crypto::hash8 pid;
  return get_account_integrated_address_from_str(address, has_payment_id, pid, testnet, str);
}

bool Wallet::keyValid(const std::string &secret_key_string, const std::string &address_string, bool isViewKey, bool testnet, std::string &error)
{
  bool has_payment_id;
  cryptonote::account_public_address address;
  crypto::hash8 pid;
  if(!get_account_integrated_address_from_str(address, has_payment_id, pid, testnet, address_string)) {
      error = tr("Failed to parse address");
      return false;
  }
  
  cryptonote::blobdata key_data;
  if(!epee::string_tools::parse_hexstr_to_binbuff(secret_key_string, key_data) || key_data.size() != sizeof(crypto::secret_key))
  {
      error = tr("Failed to parse key");
      return false;
  }
  crypto::secret_key key = *reinterpret_cast<const crypto::secret_key*>(key_data.data());

  // check the key match the given address
  crypto::public_key pkey;
  if (!crypto::secret_key_to_public_key(key, pkey)) {
      error = tr("failed to verify key");
      return false;
  }
  bool matchAddress = false;
  if(isViewKey)
      matchAddress = address.m_view_public_key == pkey;
  else
      matchAddress = address.m_spend_public_key == pkey;

  if(!matchAddress) {
      error = tr("key does not match address");
      return false;
  }
  
  return true;
}

std::string Wallet::paymentIdFromAddress(const std::string &str, bool testnet)
{
  bool has_payment_id;
  cryptonote::account_public_address address;
  crypto::hash8 pid;
  if (!get_account_integrated_address_from_str(address, has_payment_id, pid, testnet, str))
    return "";
  if (!has_payment_id)
    return "";
  return epee::string_tools::pod_to_hex(pid);
}

uint64_t Wallet::maximumAllowedAmount()
{
    return std::numeric_limits<uint64_t>::max();
}

void Wallet::init(const char *argv0, const char *default_log_base_name) {
    epee::string_tools::set_module_name_and_folder(argv0);
    mlog_configure(mlog_get_default_log_path(default_log_base_name), true);
}

void Wallet::debug(const std::string &str) {
    MDEBUG(str);
}

///////////////////////// WalletImpl implementation ////////////////////////
WalletImpl::WalletImpl(bool testnet)
    :m_wallet(nullptr)
    , m_status(Wallet::Status_Ok)
    , m_trustedDaemon(false)
    , m_wallet2Callback(nullptr)
    , m_recoveringFromSeed(false)
    , m_synchronized(false)
    , m_rebuildWalletCache(false)
    , m_is_connected(false)
{
    m_wallet = new tools::wallet2(testnet);
    m_history = new TransactionHistoryImpl(this);
    m_wallet2Callback = new Wallet2CallbackImpl(this);
    m_wallet->callback(m_wallet2Callback);
    m_refreshThreadDone = false;
    m_refreshEnabled = false;
    m_addressBook = new AddressBookImpl(this);


    m_refreshIntervalMillis = DEFAULT_REFRESH_INTERVAL_MILLIS;

    m_refreshThread = boost::thread([this] () {
        this->refreshThreadFunc();
    });

}

WalletImpl::~WalletImpl()
{

    LOG_PRINT_L1(__FUNCTION__);
    // Pause refresh thread - prevents refresh from starting again
    pauseRefresh();
    // Close wallet - stores cache and stops ongoing refresh operation 
    close();
    // Stop refresh thread
    stopRefresh();
    delete m_wallet2Callback;
    delete m_history;
    delete m_addressBook;
    delete m_wallet;
    LOG_PRINT_L1(__FUNCTION__ << " finished");
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language)
{

    clearStatus();
    m_recoveringFromSeed = false;
    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);


    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        m_errorString = "attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(m_errorString);
        m_status = Status_Critical;
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
        m_status = Status_Critical;
        m_errorString = e.what();
        return false;
    }

    return true;
}

bool WalletImpl::createWatchOnly(const std::string &path, const std::string &password, const std::string &language) const
{
    clearStatus();
    std::unique_ptr<tools::wallet2> view_wallet(new tools::wallet2(m_wallet->testnet()));

    // Store same refresh height as original wallet
    view_wallet->set_refresh_from_block_height(m_wallet->get_refresh_from_block_height());

    bool keys_file_exists;
    bool wallet_file_exists;
    tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);

    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        m_errorString = "attempting to generate view only wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
        return false;
    }
    // TODO: validate language
    view_wallet->set_seed_language(language);

    const crypto::secret_key viewkey = m_wallet->get_account().get_keys().m_view_secret_key;
    const cryptonote::account_public_address address = m_wallet->get_account().get_keys().m_account_address;

    try {
        view_wallet->generate(path, password, address, viewkey);
        m_status = Status_Ok;
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating view only wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
    return true;
}

bool WalletImpl::recoverFromKeys(const std::string &path,
                                const std::string &language,
                                const std::string &address_string,
                                const std::string &viewkey_string,
                                const std::string &spendkey_string)
{
    cryptonote::account_public_address address;
    bool has_payment_id;
    crypto::hash8 new_payment_id;
    if(!get_account_integrated_address_from_str(address, has_payment_id, new_payment_id, m_wallet->testnet(), address_string))
    {
        m_errorString = tr("failed to parse address");
        m_status = Status_Error;
        return false;
    }

    // parse optional spend key
    crypto::secret_key spendkey;
    bool has_spendkey = false;
    if (!spendkey_string.empty()) {
        cryptonote::blobdata spendkey_data;
        if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
        {
            m_errorString = tr("failed to parse secret spend key");
            m_status = Status_Error;
            return false;
        }
        has_spendkey = true;
        spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
    }

    // parse view secret key
    if (viewkey_string.empty()) {
        m_errorString = tr("No view key supplied, cancelled");
        m_status = Status_Error;
        return false;
    }
    cryptonote::blobdata viewkey_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
    {
        m_errorString = tr("failed to parse secret view key");
        m_status = Status_Error;
        return false;
    }
    crypto::secret_key viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());

    // check the spend and view keys match the given address
    crypto::public_key pkey;
    if(has_spendkey) {
        if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
            m_errorString = tr("failed to verify secret spend key");
            m_status = Status_Error;
            return false;
        }
        if (address.m_spend_public_key != pkey) {
            m_errorString = tr("spend key does not match address");
            m_status = Status_Error;
            return false;
        }
    }
    if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
        m_errorString = tr("failed to verify secret view key");
        m_status = Status_Error;
        return false;
    }
    if (address.m_view_public_key != pkey) {
        m_errorString = tr("view key does not match address");
        m_status = Status_Error;
        return false;
    }

    try
    {
        if (has_spendkey) {
            m_wallet->generate(path, "", address, spendkey, viewkey);
            setSeedLanguage(language);
            LOG_PRINT_L1("Generated new wallet from keys with seed language: " + language);
        }
        else {
            m_wallet->generate(path, "", address, viewkey);
            LOG_PRINT_L1("Generated new view only wallet from keys");
        }
        
    }
    catch (const std::exception& e) {
        m_errorString = string(tr("failed to generate new wallet: ")) + e.what();
        m_status = Status_Error;
        return false;
    }
    return true;
}


bool WalletImpl::open(const std::string &path, const std::string &password)
{
    clearStatus();
    m_recoveringFromSeed = false;
    try {
        // TODO: handle "deprecated"
        // Check if wallet cache exists
        bool keys_file_exists;
        bool wallet_file_exists;
        tools::wallet2::wallet_exists(path, keys_file_exists, wallet_file_exists);
        if(!wallet_file_exists){
            // Rebuilding wallet cache, using refresh height from .keys file
            m_rebuildWalletCache = true;
        }
        m_wallet->load(path, password);

        m_password = password;
    } catch (const std::exception &e) {
        LOG_ERROR("Error opening wallet: " << e.what());
        m_status = Status_Critical;
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

    m_recoveringFromSeed = true;
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

    } catch (const std::exception &e) {
        m_status = Status_Critical;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

bool WalletImpl::close()
{

    bool result = false;
    LOG_PRINT_L1("closing wallet...");
    try {
        // Do not store wallet with invalid status
        // Status Critical refers to errors on opening or creating wallets.
        if (status() != Status_Critical)
            m_wallet->store();
        else
            LOG_ERROR("Status_Critical - not storing wallet");
        LOG_PRINT_L1("wallet::store done");
        LOG_PRINT_L1("Calling wallet::stop...");
        m_wallet->stop();
        LOG_PRINT_L1("wallet::stop done");
        result = true;
        clearStatus();
    } catch (const std::exception &e) {
        m_status = Status_Critical;
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

std::string WalletImpl::integratedAddress(const std::string &payment_id) const
{
    crypto::hash8 pid;
    if (!tools::wallet2::parse_short_payment_id(payment_id, pid)) {
        return "";
    }
    return m_wallet->get_account().get_public_integrated_address_str(pid, m_wallet->testnet());
}

std::string WalletImpl::secretViewKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_view_secret_key);
}

std::string WalletImpl::publicViewKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_view_public_key);
}

std::string WalletImpl::secretSpendKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_spend_secret_key);
}

std::string WalletImpl::publicSpendKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key);
}

std::string WalletImpl::path() const
{
    return m_wallet->path();
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

string WalletImpl::filename() const
{
    return m_wallet->get_wallet_file();
}

string WalletImpl::keysFilename() const
{
    return m_wallet->get_keys_file();
}

bool WalletImpl::init(const std::string &daemon_address, uint64_t upper_transaction_size_limit, const std::string &daemon_username, const std::string &daemon_password)
{
    clearStatus();
    if(daemon_username != "")
        m_daemon_login.emplace(daemon_username, daemon_password);
    return doInit(daemon_address, upper_transaction_size_limit);
}

void WalletImpl::setRefreshFromBlockHeight(uint64_t refresh_from_block_height)
{
    m_wallet->set_refresh_from_block_height(refresh_from_block_height);
}

void WalletImpl::setRecoveringFromSeed(bool recoveringFromSeed)
{
    m_recoveringFromSeed = recoveringFromSeed;
}

uint64_t WalletImpl::balance() const
{
    return m_wallet->balance();
}

uint64_t WalletImpl::unlockedBalance() const
{
    return m_wallet->unlocked_balance();
}

uint64_t WalletImpl::blockChainHeight() const
{
    return m_wallet->get_blockchain_current_height();
}
uint64_t WalletImpl::approximateBlockChainHeight() const
{
    return m_wallet->get_approximate_blockchain_height();
}
uint64_t WalletImpl::daemonBlockChainHeight() const
{
    if (!m_is_connected)
        return 0;
    std::string err;
    uint64_t result = m_wallet->get_daemon_blockchain_height(err);
    if (!err.empty()) {
        LOG_ERROR(__FUNCTION__ << ": " << err);
        result = 0;
        m_errorString = err;
        m_status = Status_Error;

    } else {
        m_status = Status_Ok;
        m_errorString = "";
    }
    return result;
}

uint64_t WalletImpl::daemonBlockChainTargetHeight() const
{
    if (!m_is_connected)
        return 0;
    std::string err;
    uint64_t result = m_wallet->get_daemon_blockchain_target_height(err);
    if (!err.empty()) {
        LOG_ERROR(__FUNCTION__ << ": " << err);
        result = 0;
        m_errorString = err;
        m_status = Status_Error;

    } else {
        m_status = Status_Ok;
        m_errorString = "";
    }
    // Target height can be 0 when daemon is synced. Use blockchain height instead. 
    if(result == 0)
        result = daemonBlockChainHeight();
    return result;
}

bool WalletImpl::daemonSynced() const
{   
    if(connected() == Wallet::ConnectionStatus_Disconnected)
        return false;
    uint64_t blockChainHeight = daemonBlockChainHeight();
    return (blockChainHeight >= daemonBlockChainTargetHeight() && blockChainHeight > 1);
}

bool WalletImpl::synchronized() const
{
    return m_synchronized;
}

bool WalletImpl::refresh()
{
    clearStatus();
    doRefresh();
    return m_status == Status_Ok;
}

void WalletImpl::refreshAsync()
{
    LOG_PRINT_L3(__FUNCTION__ << ": Refreshing asynchronously..");
    clearStatus();
    m_refreshCV.notify_one();
}

void WalletImpl::setAutoRefreshInterval(int millis)
{
    if (millis > MAX_REFRESH_INTERVAL_MILLIS) {
        LOG_ERROR(__FUNCTION__<< ": invalid refresh interval " << millis
                  << " ms, maximum allowed is " << MAX_REFRESH_INTERVAL_MILLIS << " ms");
        m_refreshIntervalMillis = MAX_REFRESH_INTERVAL_MILLIS;
    } else {
        m_refreshIntervalMillis = millis;
    }
}

int WalletImpl::autoRefreshInterval() const
{
    return m_refreshIntervalMillis;
}

UnsignedTransaction *WalletImpl::loadUnsignedTx(const std::string &unsigned_filename) {
  clearStatus();
  UnsignedTransactionImpl * transaction = new UnsignedTransactionImpl(*this);
  if (!m_wallet->load_unsigned_tx(unsigned_filename, transaction->m_unsigned_tx_set)){
    m_errorString = tr("Failed to load unsigned transactions");
    m_status = Status_Error;
  }
  
  // Check tx data and construct confirmation message
  std::string extra_message;
  if (!transaction->m_unsigned_tx_set.transfers.empty())
    extra_message = (boost::format("%u outputs to import. ") % (unsigned)transaction->m_unsigned_tx_set.transfers.size()).str();
  transaction->checkLoadedTx([&transaction](){return transaction->m_unsigned_tx_set.txes.size();}, [&transaction](size_t n)->const tools::wallet2::tx_construction_data&{return transaction->m_unsigned_tx_set.txes[n];}, extra_message);
  m_status = transaction->status();
  m_errorString = transaction->errorString(); 
    
  return transaction;
}

bool WalletImpl::submitTransaction(const string &fileName) {
  clearStatus();
  std::unique_ptr<PendingTransactionImpl> transaction(new PendingTransactionImpl(*this));

  bool r = m_wallet->load_tx(fileName, transaction->m_pending_tx);
  if (!r) {
    m_errorString = tr("Failed to load transaction from file");
    m_status = Status_Ok;
    return false;
  }
  
  if(!transaction->commit()) {
    m_errorString = transaction->m_errorString;
    m_status = Status_Error;
    return false;
  }

  return true;
}

bool WalletImpl::exportKeyImages(const string &filename) 
{
  if (m_wallet->watch_only())
  {
    m_errorString = tr("Wallet is view only");
    m_status = Status_Error;
    return false;
  }
  
  try
  {
    if (!m_wallet->export_key_images(filename))
    {
      m_errorString = tr("failed to save file ") + filename;
      m_status = Status_Error;
      return false;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    m_errorString = e.what();
    m_status = Status_Error;
    return false;
  }
  return true;
}

bool WalletImpl::importKeyImages(const string &filename)
{
  try
  {
    uint64_t spent = 0, unspent = 0;
    uint64_t height = m_wallet->import_key_images(filename, spent, unspent);
    LOG_PRINT_L2("Signed key images imported to height " << height << ", "
        << print_money(spent) << " spent, " << print_money(unspent) << " unspent");
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    m_errorString = string(tr("Failed to import key images: ")) + e.what();
    m_status = Status_Error;
    return false;
  }

  return true;
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

PendingTransaction *WalletImpl::createTransaction(const string &dst_addr, const string &payment_id, optional<uint64_t> amount, uint32_t mixin_count,
                                                  PendingTransaction::Priority priority)

{
    clearStatus();
    // Pause refresh thread while creating transaction
    pauseRefresh();
      
    cryptonote::account_public_address addr;

    // indicates if dst_addr is integrated address (address + payment_id)
    bool has_payment_id;
    crypto::hash8 payment_id_short;
    // TODO:  (https://bitcointalk.org/index.php?topic=753252.msg9985441#msg9985441)
    size_t fake_outs_count = mixin_count > 0 ? mixin_count : m_wallet->default_mixin();
    if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIXIN;

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    do {
        if(!cryptonote::get_account_integrated_address_from_str(addr, has_payment_id, payment_id_short, m_wallet->testnet(), dst_addr)) {
            // TODO: copy-paste 'if treating as an address fails, try as url' from simplewallet.cpp:1982
            m_status = Status_Error;
            m_errorString = "Invalid destination address";
            break;
        }


        std::vector<uint8_t> extra;
        // if dst_addr is not an integrated address, parse payment_id
        if (!has_payment_id && !payment_id.empty()) {
            // copy-pasted from simplewallet.cpp:2212
            crypto::hash payment_id_long;
            bool r = tools::wallet2::parse_long_payment_id(payment_id, payment_id_long);
            if (r) {
                std::string extra_nonce;
                cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id_long);
                r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
            } else {
                r = tools::wallet2::parse_short_payment_id(payment_id, payment_id_short);
                if (r) {
                    std::string extra_nonce;
                    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id_short);
                    r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
                }
            }

            if (!r) {
                m_status = Status_Error;
                m_errorString = tr("payment id has invalid format, expected 16 or 64 character hex string: ") + payment_id;
                break;
            }
        }
        else if (has_payment_id) {
            std::string extra_nonce;
            set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, payment_id_short);
            bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
            if (!r) {
                m_status = Status_Error;
                m_errorString = tr("Failed to add short payment id: ") + epee::string_tools::pod_to_hex(payment_id_short);
                break;
            }
        }


        //std::vector<tools::wallet2::pending_tx> ptx_vector;

        try {
            if (amount) {
                vector<cryptonote::tx_destination_entry> dsts;
                cryptonote::tx_destination_entry de;
                de.addr = addr;
                de.amount = *amount;
                dsts.push_back(de);
                transaction->m_pending_tx = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */,
                                                                          static_cast<uint32_t>(priority),
                                                                          extra, m_trustedDaemon);
            } else {
                transaction->m_pending_tx = m_wallet->create_transactions_all(0, addr, fake_outs_count, 0 /* unlock_time */,
                                                                          static_cast<uint32_t>(priority),
                                                                          extra, m_trustedDaemon);
            }

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
        } catch (const tools::error::get_random_outs_error &e) {
            m_errorString = (boost::format(tr("failed to get random outputs to mix: %s")) % e.what()).str();
            m_status = Status_Error;

        } catch (const tools::error::not_enough_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            m_errorString = writer.str();

        } catch (const tools::error::tx_not_possible& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());
            m_errorString = writer.str();

        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer;
            writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
            for (const std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
            }
            m_errorString = writer.str();
            m_status = Status_Error;
        } catch (const tools::error::tx_not_constructed&) {
            m_errorString = tr("transaction was not constructed");
            m_status = Status_Error;
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer;
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            m_errorString = writer.str();
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
    // Resume refresh thread
    startRefresh();
    return transaction;
}

PendingTransaction *WalletImpl::createSweepUnmixableTransaction()

{
    clearStatus();
    vector<cryptonote::tx_destination_entry> dsts;
    cryptonote::tx_destination_entry de;

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    do {
        try {
            transaction->m_pending_tx = m_wallet->create_unmixable_sweep_transactions(m_trustedDaemon);

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
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            m_errorString = writer.str();

        } catch (const tools::error::tx_not_possible& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());
            m_errorString = writer.str();

        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer;
            writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
            for (const std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
            }
            m_errorString = writer.str();
            m_status = Status_Error;
        } catch (const tools::error::tx_not_constructed&) {
            m_errorString = tr("transaction was not constructed");
            m_status = Status_Error;
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer;
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            m_errorString = writer.str();
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

TransactionHistory *WalletImpl::history() const
{
    return m_history;
}

AddressBook *WalletImpl::addressBook() const
{
    return m_addressBook;
}

void WalletImpl::setListener(WalletListener *l)
{
    // TODO thread synchronization;
    m_wallet2Callback->setListener(l);
}

uint32_t WalletImpl::defaultMixin() const
{
    return m_wallet->default_mixin();
}

void WalletImpl::setDefaultMixin(uint32_t arg)
{
    m_wallet->default_mixin(arg);
}

bool WalletImpl::setUserNote(const std::string &txid, const std::string &note)
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
      return false;
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    m_wallet->set_tx_note(htxid, note);
    return true;
}

std::string WalletImpl::getUserNote(const std::string &txid) const
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
      return "";
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    return m_wallet->get_tx_note(htxid);
}

std::string WalletImpl::getTxKey(const std::string &txid) const
{
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
    {
      return "";
    }
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    crypto::secret_key tx_key;
    if (m_wallet->get_tx_key(htxid, tx_key))
    {
      return epee::string_tools::pod_to_hex(tx_key);
    }
    else
    {
      return "";
    }
}

std::string WalletImpl::signMessage(const std::string &message)
{
  return m_wallet->sign(message);
}

bool WalletImpl::verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const
{
  cryptonote::account_public_address addr;
  bool has_payment_id;
  crypto::hash8 payment_id;

  if (!cryptonote::get_account_integrated_address_from_str(addr, has_payment_id, payment_id, m_wallet->testnet(), address))
    return false;

  return m_wallet->verify(message, addr, signature);
}

bool WalletImpl::connectToDaemon()
{
    bool result = m_wallet->check_connection(NULL, DEFAULT_CONNECTION_TIMEOUT_MILLIS);
    m_status = result ? Status_Ok : Status_Error;
    if (!result) {
        m_errorString = "Error connecting to daemon at " + m_wallet->get_daemon_address();
    } else {
        // start refreshing here
    }
    return result;
}

Wallet::ConnectionStatus WalletImpl::connected() const
{
    uint32_t version = 0;
    m_is_connected = m_wallet->check_connection(&version, DEFAULT_CONNECTION_TIMEOUT_MILLIS);
    if (!m_is_connected)
        return Wallet::ConnectionStatus_Disconnected;
    if ((version >> 16) != CORE_RPC_VERSION_MAJOR)
        return Wallet::ConnectionStatus_WrongVersion;
    return Wallet::ConnectionStatus_Connected;
}

void WalletImpl::setTrustedDaemon(bool arg)
{
    m_trustedDaemon = arg;
}

bool WalletImpl::trustedDaemon() const
{
    return m_trustedDaemon;
}

bool WalletImpl::watchOnly() const
{
    return m_wallet->watch_only();
}

void WalletImpl::clearStatus() const
{
    m_status = Status_Ok;
    m_errorString.clear();
}

void WalletImpl::refreshThreadFunc()
{
    LOG_PRINT_L3(__FUNCTION__ << ": starting refresh thread");

    while (true) {
        boost::mutex::scoped_lock lock(m_refreshMutex);
        if (m_refreshThreadDone) {
            break;
        }
        LOG_PRINT_L3(__FUNCTION__ << ": waiting for refresh...");
        // if auto refresh enabled, we wait for the "m_refreshIntervalSeconds" interval.
        // if not - we wait forever
        if (m_refreshIntervalMillis > 0) {
            boost::posix_time::milliseconds wait_for_ms(m_refreshIntervalMillis);
            m_refreshCV.timed_wait(lock, wait_for_ms);
        } else {
            m_refreshCV.wait(lock);
        }

        LOG_PRINT_L3(__FUNCTION__ << ": refresh lock acquired...");
        LOG_PRINT_L3(__FUNCTION__ << ": m_refreshEnabled: " << m_refreshEnabled);
        LOG_PRINT_L3(__FUNCTION__ << ": m_status: " << m_status);
        if (m_refreshEnabled) {
            LOG_PRINT_L3(__FUNCTION__ << ": refreshing...");
            doRefresh();
        }
    }
    LOG_PRINT_L3(__FUNCTION__ << ": refresh thread stopped");
}

void WalletImpl::doRefresh()
{
    // synchronizing async and sync refresh calls
    boost::lock_guard<boost::mutex> guarg(m_refreshMutex2);
    try {
        // Syncing daemon and refreshing wallet simultaneously is very resource intensive.
        // Disable refresh if wallet is disconnected or daemon isn't synced.
        if (daemonSynced()) {
            m_wallet->refresh();
            if (!m_synchronized) {
                m_synchronized = true;
            }
            // assuming if we have empty history, it wasn't initialized yet
            // for futher history changes client need to update history in
            // "on_money_received" and "on_money_sent" callbacks
            if (m_history->count() == 0) {
                m_history->refresh();
            }
        } else {
           LOG_PRINT_L3(__FUNCTION__ << ": skipping refresh - daemon is not synced");
        }
    } catch (const std::exception &e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }
    if (m_wallet2Callback->getListener()) {
        m_wallet2Callback->getListener()->refreshed();
    }
}


void WalletImpl::startRefresh()
{
    if (!m_refreshEnabled) {
        LOG_PRINT_L2(__FUNCTION__ << ": refresh started/resumed...");
        m_refreshEnabled = true;
        m_refreshCV.notify_one();
    }
}



void WalletImpl::stopRefresh()
{
    if (!m_refreshThreadDone) {
        m_refreshEnabled = false;
        m_refreshThreadDone = true;
        m_refreshCV.notify_one();
        m_refreshThread.join();
    }
}

void WalletImpl::pauseRefresh()
{
    LOG_PRINT_L2(__FUNCTION__ << ": refresh paused...");
    // TODO synchronize access
    if (!m_refreshThreadDone) {
        m_refreshEnabled = false;
    }
}


bool WalletImpl::isNewWallet() const
{
    // in case wallet created without daemon connection, closed and opened again,
    // it's the same case as if it created from scratch, i.e. we need "fast sync"
    // with the daemon (pull hashes instead of pull blocks).
    // If wallet cache is rebuilt, creation height stored in .keys is used.
    // Watch only wallet is a copy of an existing wallet. 
    return !(blockChainHeight() > 1 || m_recoveringFromSeed || m_rebuildWalletCache) && !watchOnly();
}

bool WalletImpl::doInit(const string &daemon_address, uint64_t upper_transaction_size_limit)
{
    if (!m_wallet->init(daemon_address, m_daemon_login, upper_transaction_size_limit))
       return false;

    if (m_rebuildWalletCache)
      LOG_PRINT_L2(__FUNCTION__ << ": Rebuilding wallet cache, fast refresh until block " << m_wallet->get_refresh_from_block_height());

    if (Utils::isAddressLocal(daemon_address)) {
        this->setTrustedDaemon(true);
        m_refreshIntervalMillis = DEFAULT_REFRESH_INTERVAL_MILLIS;
    } else {
        this->setTrustedDaemon(false);
        m_refreshIntervalMillis = DEFAULT_REMOTE_NODE_REFRESH_INTERVAL_MILLIS;
    }
    return true;
}

bool WalletImpl::parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error)
{
    return m_wallet->parse_uri(uri, address, payment_id, amount, tx_description, recipient_name, unknown_parameters, error);
}

std::string WalletImpl::getDefaultDataDir() const
{
 return tools::get_default_data_dir();
}

bool WalletImpl::rescanSpent()
{
  clearStatus();
  if (!trustedDaemon()) {
    m_status = Status_Error;
    m_errorString = tr("Rescan spent can only be used with a trusted daemon");
    return false;
  }
  try {
      m_wallet->rescan_spent();
  } catch (const std::exception &e) {
      LOG_ERROR(__FUNCTION__ << " error: " << e.what());
      m_status = Status_Error;
      m_errorString = e.what();
      return false;
  }
  return true;
}


void WalletImpl::hardForkInfo(uint8_t &version, uint64_t &earliest_height) const
{
    m_wallet->get_hard_fork_info(version, earliest_height);
}

bool WalletImpl::useForkRules(uint8_t version, int64_t early_blocks) const 
{
    return m_wallet->use_fork_rules(version,early_blocks);
}

} // namespace

namespace Bitmonero = Monero;
