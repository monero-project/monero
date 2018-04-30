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


#include "wallet.h"
#include "pending_transaction.h"
#include "unsigned_transaction.h"
#include "transaction_history.h"
#include "address_book.h"
#include "subaddress.h"
#include "subaddress_account.h"
#include "common_defines.h"
#include "common/util.h"

#include "mnemonics/electrum-words.h"
#include "mnemonics/english.h"
#include <boost/format.hpp>
#include <sstream>
#include <unordered_map>

#ifdef WIN32
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#endif

using namespace std;
using namespace cryptonote;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "WalletAPI"

namespace Monero {

namespace {
    // copy-pasted from simplewallet
    static const size_t DEFAULT_MIXIN = 6;
    static const int    DEFAULT_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // limit maximum refresh interval as one minute
    static const int    MAX_REFRESH_INTERVAL_MILLIS = 1000 * 60 * 1;
    // Default refresh interval when connected to remote node
    static const int    DEFAULT_REMOTE_NODE_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // Connection timeout 30 sec
    static const int    DEFAULT_CONNECTION_TIMEOUT_MILLIS = 1000 * 30;

    std::string get_default_ringdb_path()
    {
      boost::filesystem::path dir = tools::get_default_data_dir();
      // remove .bitmonero, replace with .shared-ringdb
      dir = dir.remove_filename();
      dir /= ".shared-ringdb";
      return dir.string();
    }
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

    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);

        LOG_PRINT_L3(__FUNCTION__ << ": money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on received tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->moneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);

        LOG_PRINT_L3(__FUNCTION__ << ": unconfirmed money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on received tx if wallet is not syncronized completely
        if (m_listener && m_wallet->synchronized()) {
            m_listener->unconfirmedMoneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx,
                                uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index)
    {
        // TODO;
        std::string tx_hash = epee::string_tools::pod_to_hex(txid);
        LOG_PRINT_L3(__FUNCTION__ << ": money spent. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
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

    // Light wallet callbacks
    virtual void on_lw_new_block(uint64_t height)
    {
      if (m_listener) {
        m_listener->newBlock(height);
      }
    }

    virtual void on_lw_money_received(uint64_t height, const crypto::hash &txid, uint64_t amount)
    {
      if (m_listener) {
        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);
        m_listener->moneyReceived(tx_hash, amount);
      }
    }

    virtual void on_lw_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, uint64_t amount)
    {
      if (m_listener) {
        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);
        m_listener->unconfirmedMoneyReceived(tx_hash, amount);
      }
    }

    virtual void on_lw_money_spent(uint64_t height, const crypto::hash &txid, uint64_t amount)
    {
      if (m_listener) {
        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);
        m_listener->moneySpent(tx_hash, amount);
      }
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

bool Wallet::addressValid(const std::string &str, NetworkType nettype)
{
  cryptonote::address_parse_info info;
  return get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), str);
}

bool Wallet::keyValid(const std::string &secret_key_string, const std::string &address_string, bool isViewKey, NetworkType nettype, std::string &error)
{
  cryptonote::address_parse_info info;
  if(!get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), address_string)) {
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
      matchAddress = info.address.m_view_public_key == pkey;
  else
      matchAddress = info.address.m_spend_public_key == pkey;

  if(!matchAddress) {
      error = tr("key does not match address");
      return false;
  }
  
  return true;
}

std::string Wallet::paymentIdFromAddress(const std::string &str, NetworkType nettype)
{
  cryptonote::address_parse_info info;
  if (!get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), str))
    return "";
  if (!info.has_payment_id)
    return "";
  return epee::string_tools::pod_to_hex(info.payment_id);
}

uint64_t Wallet::maximumAllowedAmount()
{
    return std::numeric_limits<uint64_t>::max();
}

void Wallet::init(const char *argv0, const char *default_log_base_name, const std::string &log_path, bool console) {
#ifdef WIN32
    // Activate UTF-8 support for Boost filesystem classes on Windows
    std::locale::global(boost::locale::generator().generate(""));
    boost::filesystem::path::imbue(std::locale());
#endif
    epee::string_tools::set_module_name_and_folder(argv0);
    mlog_configure(log_path.empty() ? mlog_get_default_log_path(default_log_base_name) : log_path.c_str(), console);
}

void Wallet::debug(const std::string &category, const std::string &str) {
    MCDEBUG(category.empty() ? MONERO_DEFAULT_LOG_CATEGORY : category.c_str(), str);
}

void Wallet::info(const std::string &category, const std::string &str) {
    MCINFO(category.empty() ? MONERO_DEFAULT_LOG_CATEGORY : category.c_str(), str);
}

void Wallet::warning(const std::string &category, const std::string &str) {
    MCWARNING(category.empty() ? MONERO_DEFAULT_LOG_CATEGORY : category.c_str(), str);
}

void Wallet::error(const std::string &category, const std::string &str) {
    MCERROR(category.empty() ? MONERO_DEFAULT_LOG_CATEGORY : category.c_str(), str);
}

///////////////////////// WalletImpl implementation ////////////////////////
WalletImpl::WalletImpl(NetworkType nettype)
    :m_wallet(nullptr)
    , m_status(Wallet::Status_Ok)
    , m_trustedDaemon(false)
    , m_wallet2Callback(nullptr)
    , m_recoveringFromSeed(false)
    , m_recoveringFromDevice(false)
    , m_synchronized(false)
    , m_rebuildWalletCache(false)
    , m_is_connected(false)
{
    m_wallet = new tools::wallet2(static_cast<cryptonote::network_type>(nettype));
    m_history = new TransactionHistoryImpl(this);
    m_wallet2Callback = new Wallet2CallbackImpl(this);
    m_wallet->callback(m_wallet2Callback);
    m_refreshThreadDone = false;
    m_refreshEnabled = false;
    m_addressBook = new AddressBookImpl(this);
    m_subaddress = new SubaddressImpl(this);
    m_subaddressAccount = new SubaddressAccountImpl(this);


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
    close(false); // do not store wallet as part of the closing activities
    // Stop refresh thread
    stopRefresh();
    delete m_wallet2Callback;
    delete m_history;
    delete m_addressBook;
    delete m_subaddress;
    delete m_subaddressAccount;
    delete m_wallet;
    LOG_PRINT_L1(__FUNCTION__ << " finished");
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language)
{

    clearStatus();
    m_recoveringFromSeed = false;
    m_recoveringFromDevice = false;
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
    std::unique_ptr<tools::wallet2> view_wallet(new tools::wallet2(m_wallet->nettype()));

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
        // Generate view only wallet
        view_wallet->generate(path, password, address, viewkey);

        // Export/Import outputs
        auto outputs = m_wallet->export_outputs();
        view_wallet->import_outputs(outputs);

        // Copy scanned blockchain
        auto bc = m_wallet->export_blockchain();
        view_wallet->import_blockchain(bc);

        // copy payments
        auto payments = m_wallet->export_payments();
        view_wallet->import_payments(payments);

        // copy confirmed outgoing payments
        std::list<std::pair<crypto::hash, tools::wallet2::confirmed_transfer_details>> out_payments;
        m_wallet->get_payments_out(out_payments, 0);
        view_wallet->import_payments_out(out_payments);

        // Export/Import key images
        // We already know the spent status from the outputs we exported, thus no need to check them again
        auto key_images = m_wallet->export_key_images();
        uint64_t spent = 0;
        uint64_t unspent = 0;
        view_wallet->import_key_images(key_images,spent,unspent,false);
        m_status = Status_Ok;
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating view only wallet: " << e.what());
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
    // Store wallet
    view_wallet->store();
    return true;
}

bool WalletImpl::recoverFromKeys(const std::string &path,
                                const std::string &language,
                                const std::string &address_string,
                                const std::string &viewkey_string,
                                const std::string &spendkey_string)
{
    return recoverFromKeysWithPassword(path, "", language, address_string, viewkey_string, spendkey_string);
}

bool WalletImpl::recoverFromKeysWithPassword(const std::string &path,
                                 const std::string &password,
                                 const std::string &language,
                                 const std::string &address_string,
                                 const std::string &viewkey_string,
                                 const std::string &spendkey_string)
{
    cryptonote::address_parse_info info;
    if(!get_account_address_from_str(info, m_wallet->nettype(), address_string))
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
        if (info.address.m_spend_public_key != pkey) {
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
    if (info.address.m_view_public_key != pkey) {
        m_errorString = tr("view key does not match address");
        m_status = Status_Error;
        return false;
    }

    try
    {
        if (has_spendkey) {
            m_wallet->generate(path, password, info.address, spendkey, viewkey);
            setSeedLanguage(language);
            LOG_PRINT_L1("Generated new wallet from keys with seed language: " + language);
        }
        else {
            m_wallet->generate(path, password, info.address, viewkey);
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

bool WalletImpl::recoverFromDevice(const std::string &path, const std::string &password, const std::string &device_name)
{
    clearStatus();
    m_recoveringFromSeed = false;
    m_recoveringFromDevice = true;
    try
    {
        m_wallet->restore(path, password, device_name);
        LOG_PRINT_L1("Generated new wallet from device: " + device_name);
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
    m_recoveringFromDevice = false;
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
        m_wallet->set_ring_database(get_default_ringdb_path());
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
    return recover(path, "", seed);
}

bool WalletImpl::recover(const std::string &path, const std::string &password, const std::string &seed)
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
    m_recoveringFromDevice = false;
    crypto::secret_key recovery_key;
    std::string old_language;
    if (!crypto::ElectrumWords::words_to_bytes(seed, recovery_key, old_language)) {
        m_errorString = "Electrum-style word list failed verification";
        m_status = Status_Error;
        return false;
    }

    if (old_language == crypto::ElectrumWords::old_language_name)
        old_language = Language::English().get_language_name();

    try {
        m_wallet->set_seed_language(old_language);
        m_wallet->generate(path, password, recovery_key, true, false);

    } catch (const std::exception &e) {
        m_status = Status_Critical;
        m_errorString = e.what();
    }
    return m_status == Status_Ok;
}

bool WalletImpl::close(bool store)
{

    bool result = false;
    LOG_PRINT_L1("closing wallet...");
    try {
        if (store) {
            // Do not store wallet with invalid status
            // Status Critical refers to errors on opening or creating wallets.
            if (status() != Status_Critical)
                m_wallet->store();
            else
                LOG_ERROR("Status_Critical - not saving wallet");
            LOG_PRINT_L1("wallet::store done");
        }
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

std::string WalletImpl::address(uint32_t accountIndex, uint32_t addressIndex) const
{
    return m_wallet->get_subaddress_as_str({accountIndex, addressIndex});
}

std::string WalletImpl::integratedAddress(const std::string &payment_id) const
{
    crypto::hash8 pid;
    if (!tools::wallet2::parse_short_payment_id(payment_id, pid)) {
        return "";
    }
    return m_wallet->get_integrated_address_as_str(pid);
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
        LOG_ERROR("Error saving wallet: " << e.what());
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

bool WalletImpl::init(const std::string &daemon_address, uint64_t upper_transaction_size_limit, const std::string &daemon_username, const std::string &daemon_password, bool use_ssl, bool lightWallet)
{
    clearStatus();
    m_wallet->set_light_wallet(lightWallet);
    if(daemon_username != "")
        m_daemon_login.emplace(daemon_username, daemon_password);
    return doInit(daemon_address, upper_transaction_size_limit, use_ssl);
}

bool WalletImpl::lightWalletLogin(bool &isNewWallet) const
{
  return m_wallet->light_wallet_login(isNewWallet);
}

bool WalletImpl::lightWalletImportWalletRequest(std::string &payment_id, uint64_t &fee, bool &new_request, bool &request_fulfilled, std::string &payment_address, std::string &status)
{
  try
  {
    cryptonote::COMMAND_RPC_IMPORT_WALLET_REQUEST::response response;
    if(!m_wallet->light_wallet_import_wallet_request(response)){
      m_errorString = tr("Failed to send import wallet request");
      m_status = Status_Error;
      return false;
    }
    fee = response.import_fee;
    payment_id = response.payment_id;
    new_request = response.new_request;
    request_fulfilled = response.request_fulfilled;
    payment_address = response.payment_address;
    status = response.status;
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error sending import wallet request: " << e.what());
    m_errorString = e.what();
    m_status = Status_Error;
    return false;
  }
  return true;
}

void WalletImpl::setRefreshFromBlockHeight(uint64_t refresh_from_block_height)
{
    m_wallet->set_refresh_from_block_height(refresh_from_block_height);
}

void WalletImpl::setRecoveringFromSeed(bool recoveringFromSeed)
{
    m_recoveringFromSeed = recoveringFromSeed;
}

void WalletImpl::setRecoveringFromDevice(bool recoveringFromDevice)
{
    m_recoveringFromDevice = recoveringFromDevice;
}

void WalletImpl::setSubaddressLookahead(uint32_t major, uint32_t minor)
{
    m_wallet->set_subaddress_lookahead(major, minor);
}

uint64_t WalletImpl::balance(uint32_t accountIndex) const
{
    return m_wallet->balance(accountIndex);
}

uint64_t WalletImpl::unlockedBalance(uint32_t accountIndex) const
{
    return m_wallet->unlocked_balance(accountIndex);
}

uint64_t WalletImpl::blockChainHeight() const
{
    if(m_wallet->light_wallet()) {
        return m_wallet->get_light_wallet_scanned_block_height();
    }
    return m_wallet->get_blockchain_current_height();
}
uint64_t WalletImpl::approximateBlockChainHeight() const
{
    return m_wallet->get_approximate_blockchain_height();
}
uint64_t WalletImpl::daemonBlockChainHeight() const
{
    if(m_wallet->light_wallet()) {
        return m_wallet->get_light_wallet_scanned_block_height();
    }
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
    if(m_wallet->light_wallet()) {
        return m_wallet->get_light_wallet_blockchain_height();
    }
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
  if (!trustedDaemon()) {
    m_status = Status_Error;
    m_errorString = tr("Key images can only be imported with a trusted daemon");
    return false;
  }
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

void WalletImpl::addSubaddressAccount(const std::string& label)
{
    m_wallet->add_subaddress_account(label);
}
size_t WalletImpl::numSubaddressAccounts() const
{
    return m_wallet->get_num_subaddress_accounts();
}
size_t WalletImpl::numSubaddresses(uint32_t accountIndex) const
{
    return m_wallet->get_num_subaddresses(accountIndex);
}
void WalletImpl::addSubaddress(uint32_t accountIndex, const std::string& label)
{
    m_wallet->add_subaddress(accountIndex, label);
}
std::string WalletImpl::getSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex) const
{
    try
    {
        return m_wallet->get_subaddress_label({accountIndex, addressIndex});
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error getting subaddress label: ") << e.what();
        m_errorString = string(tr("Failed to get subaddress label: ")) + e.what();
        m_status = Status_Error;
        return "";
    }
}
void WalletImpl::setSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label)
{
    try
    {
        return m_wallet->set_subaddress_label({accountIndex, addressIndex}, label);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error setting subaddress label: ") << e.what();
        m_errorString = string(tr("Failed to set subaddress label: ")) + e.what();
        m_status = Status_Error;
    }
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
                                                  PendingTransaction::Priority priority, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices)

{
    clearStatus();
    // Pause refresh thread while creating transaction
    pauseRefresh();
      
    cryptonote::address_parse_info info;

    // indicates if dst_addr is integrated address (address + payment_id)
    // TODO:  (https://bitcointalk.org/index.php?topic=753252.msg9985441#msg9985441)
    size_t fake_outs_count = mixin_count > 0 ? mixin_count : m_wallet->default_mixin();
    if (fake_outs_count == 0)
        fake_outs_count = DEFAULT_MIXIN;

    uint32_t adjusted_priority = m_wallet->adjust_priority(static_cast<uint32_t>(priority));

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    do {
        if(!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), dst_addr)) {
            // TODO: copy-paste 'if treating as an address fails, try as url' from simplewallet.cpp:1982
            m_status = Status_Error;
            m_errorString = "Invalid destination address";
            break;
        }


        std::vector<uint8_t> extra;
        // if dst_addr is not an integrated address, parse payment_id
        if (!info.has_payment_id && !payment_id.empty()) {
            // copy-pasted from simplewallet.cpp:2212
            crypto::hash payment_id_long;
            bool r = tools::wallet2::parse_long_payment_id(payment_id, payment_id_long);
            if (r) {
                std::string extra_nonce;
                cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id_long);
                r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
            } else {
                r = tools::wallet2::parse_short_payment_id(payment_id, info.payment_id);
                if (r) {
                    std::string extra_nonce;
                    set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
                    r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
                }
            }

            if (!r) {
                m_status = Status_Error;
                m_errorString = tr("payment id has invalid format, expected 16 or 64 character hex string: ") + payment_id;
                break;
            }
        }
        else if (info.has_payment_id) {
            std::string extra_nonce;
            set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
            bool r = add_extra_nonce_to_tx_extra(extra, extra_nonce);
            if (!r) {
                m_status = Status_Error;
                m_errorString = tr("Failed to add short payment id: ") + epee::string_tools::pod_to_hex(info.payment_id);
                break;
            }
        }


        //std::vector<tools::wallet2::pending_tx> ptx_vector;

        try {
            if (amount) {
                vector<cryptonote::tx_destination_entry> dsts;
                cryptonote::tx_destination_entry de;
                de.addr = info.address;
                de.amount = *amount;
                de.is_subaddress = info.is_subaddress;
                dsts.push_back(de);
                transaction->m_pending_tx = m_wallet->create_transactions_2(dsts, fake_outs_count, 0 /* unlock_time */,
                                                                          adjusted_priority,
                                                                          extra, subaddr_account, subaddr_indices, m_trustedDaemon);
            } else {
                // for the GUI, sweep_all (i.e. amount set as "(all)") will always sweep all the funds in all the addresses
                if (subaddr_indices.empty())
                {
                    for (uint32_t index = 0; index < m_wallet->get_num_subaddresses(subaddr_account); ++index)
                        subaddr_indices.insert(index);
                }
                transaction->m_pending_tx = m_wallet->create_transactions_all(0, info.address, info.is_subaddress, fake_outs_count, 0 /* unlock_time */,
                                                                          adjusted_priority,
                                                                          extra, subaddr_account, subaddr_indices, m_trustedDaemon);
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

        } catch (const tools::error::not_enough_unlocked_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            m_errorString = writer.str();

        } catch (const tools::error::not_enough_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, overall balance only %s, sent amount %s")) %
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
            writer << "\n" << tr("Please sweep unmixable outputs.");
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

        } catch (const tools::error::not_enough_unlocked_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            m_errorString = writer.str();

        } catch (const tools::error::not_enough_money& e) {
            m_status = Status_Error;
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, overall balance only %s, sent amount %s")) %
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

TransactionHistory *WalletImpl::history()
{
    return m_history;
}

AddressBook *WalletImpl::addressBook()
{
    return m_addressBook;
}

Subaddress *WalletImpl::subaddress()
{
    return m_subaddress;
}

SubaddressAccount *WalletImpl::subaddressAccount()
{
    return m_subaddressAccount;
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

std::string WalletImpl::getTxKey(const std::string &txid_str) const
{
    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return "";
    }

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    if (m_wallet->get_tx_key(txid, tx_key, additional_tx_keys))
    {
        m_status = Status_Ok;
        std::ostringstream oss;
        oss << epee::string_tools::pod_to_hex(tx_key);
        for (size_t i = 0; i < additional_tx_keys.size(); ++i)
            oss << epee::string_tools::pod_to_hex(additional_tx_keys[i]);
        return oss.str();
    }
    else
    {
        m_status = Status_Error;
        m_errorString = tr("no tx keys found for this txid");
        return "";
    }
}

bool WalletImpl::checkTxKey(const std::string &txid_str, std::string tx_key_str, const std::string &address_str, uint64_t &received, bool &in_pool, uint64_t &confirmations)
{
    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return false;
    }

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), tx_key))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse tx key");
        return false;
    }
    tx_key_str = tx_key_str.substr(64);
    while (!tx_key_str.empty())
    {
        additional_tx_keys.resize(additional_tx_keys.size() + 1);
        if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), additional_tx_keys.back()))
        {
            m_status = Status_Error;
            m_errorString = tr("Failed to parse tx key");
            return false;
        }
        tx_key_str = tx_key_str.substr(64);
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse address");
        return false;
    }

    try
    {
        m_wallet->check_tx_key(txid, tx_key, additional_tx_keys, info.address, received, in_pool, confirmations);
        m_status = Status_Ok;
        return true;
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
}

std::string WalletImpl::getTxProof(const std::string &txid_str, const std::string &address_str, const std::string &message) const
{
    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return "";
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse address");
        return "";
    }

    try
    {
        m_status = Status_Ok;
        return m_wallet->get_tx_proof(txid, info.address, info.is_subaddress, message);
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return "";
    }
}

bool WalletImpl::checkTxProof(const std::string &txid_str, const std::string &address_str, const std::string &message, const std::string &signature, bool &good, uint64_t &received, bool &in_pool, uint64_t &confirmations)
{
    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return false;
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse address");
        return false;
    }

    try
    {
        good = m_wallet->check_tx_proof(txid, info.address, info.is_subaddress, message, signature, received, in_pool, confirmations);
        m_status = Status_Ok;
        return true;
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
}

std::string WalletImpl::getSpendProof(const std::string &txid_str, const std::string &message) const {
    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return "";
    }

    try
    {
        m_status = Status_Ok;
        return m_wallet->get_spend_proof(txid, message);
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return "";
    }
}

bool WalletImpl::checkSpendProof(const std::string &txid_str, const std::string &message, const std::string &signature, bool &good) const {
    good = false;
    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return false;
    }

    try
    {
        m_status = Status_Ok;
        good = m_wallet->check_spend_proof(txid, message, signature);
        return true;
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
}

std::string WalletImpl::getReserveProof(bool all, uint32_t account_index, uint64_t amount, const std::string &message) const {
    try
    {
        m_status = Status_Ok;
        boost::optional<std::pair<uint32_t, uint64_t>> account_minreserve;
        if (!all)
        {
            account_minreserve = std::make_pair(account_index, amount);
        }
        return m_wallet->get_reserve_proof(account_minreserve, message);
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return "";
    }
}

bool WalletImpl::checkReserveProof(const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &total, uint64_t &spent) const {
    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse address");
        return false;
    }
    if (info.is_subaddress)
    {
        m_status = Status_Error;
        m_errorString = tr("Address must not be a subaddress");
        return false;
    }

    good = false;
    try
    {
        m_status = Status_Ok;
        good = m_wallet->check_reserve_proof(info.address, message, signature, total, spent);
        return true;
    }
    catch (const std::exception &e)
    {
        m_status = Status_Error;
        m_errorString = e.what();
        return false;
    }
}

std::string WalletImpl::signMessage(const std::string &message)
{
  return m_wallet->sign(message);
}

bool WalletImpl::verifySignedMessage(const std::string &message, const std::string &address, const std::string &signature) const
{
  cryptonote::address_parse_info info;

  if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address))
    return false;

  return m_wallet->verify(message, info.address, signature);
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
    // Version check is not implemented in light wallets nodes/wallets
    if (!m_wallet->light_wallet() && (version >> 16) != CORE_RPC_VERSION_MAJOR)
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
            boost::posix_time::milliseconds wait_for_ms(m_refreshIntervalMillis.load());
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
        if (m_wallet->light_wallet() || daemonSynced()) {
            m_wallet->refresh();
            if (!m_synchronized) {
                m_synchronized = true;
            }
            // assuming if we have empty history, it wasn't initialized yet
            // for further history changes client need to update history in
            // "on_money_received" and "on_money_sent" callbacks
            if (m_history->count() == 0) {
                m_history->refresh();
            }
            m_wallet->find_and_save_rings(false);
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
    return !(blockChainHeight() > 1 || m_recoveringFromSeed || m_recoveringFromDevice || m_rebuildWalletCache) && !watchOnly();
}

bool WalletImpl::doInit(const string &daemon_address, uint64_t upper_transaction_size_limit, bool ssl)
{
    if (!m_wallet->init(daemon_address, m_daemon_login, upper_transaction_size_limit, ssl))
       return false;

    // in case new wallet, this will force fast-refresh (pulling hashes instead of blocks)
    // If daemon isn't synced a calculated block height will be used instead
    //TODO: Handle light wallet scenario where block height = 0.
    if (isNewWallet() && daemonSynced()) {
        LOG_PRINT_L2(__FUNCTION__ << ":New Wallet - fast refresh until " << daemonBlockChainHeight());
        m_wallet->set_refresh_from_block_height(daemonBlockChainHeight());
    }

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

bool WalletImpl::blackballOutputs(const std::vector<std::string> &pubkeys, bool add)
{
    std::vector<crypto::public_key> raw_pubkeys;
    raw_pubkeys.reserve(pubkeys.size());
    for (const std::string &str: pubkeys)
    {
        crypto::public_key pkey;
        if (!epee::string_tools::hex_to_pod(str, pkey))
        {
            m_status = Status_Error;
            m_errorString = tr("Failed to parse output public key");
            return false;
        }
        raw_pubkeys.push_back(pkey);
    }
    bool ret = m_wallet->set_blackballed_outputs(raw_pubkeys, add);
    if (!ret)
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to set blackballed outputs");
        return false;
    }
    return true;
}

bool WalletImpl::unblackballOutput(const std::string &pubkey)
{
    crypto::public_key raw_pubkey;
    if (!epee::string_tools::hex_to_pod(pubkey, raw_pubkey))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse output public key");
        return false;
    }
    bool ret = m_wallet->unblackball_output(raw_pubkey);
    if (!ret)
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to unblackball output");
        return false;
    }
    return true;
}

bool WalletImpl::getRing(const std::string &key_image, std::vector<uint64_t> &ring) const
{
    crypto::key_image raw_key_image;
    if (!epee::string_tools::hex_to_pod(key_image, raw_key_image))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse key image");
        return false;
    }
    bool ret = m_wallet->get_ring(raw_key_image, ring);
    if (!ret)
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to get ring");
        return false;
    }
    return true;
}

bool WalletImpl::getRings(const std::string &txid, std::vector<std::pair<std::string, std::vector<uint64_t>>> &rings) const
{
    crypto::hash raw_txid;
    if (!epee::string_tools::hex_to_pod(txid, raw_txid))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse txid");
        return false;
    }
    std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> raw_rings;
    bool ret = m_wallet->get_rings(raw_txid, raw_rings);
    if (!ret)
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to get rings");
        return false;
    }
    for (const auto &r: raw_rings)
    {
      rings.push_back(std::make_pair(epee::string_tools::pod_to_hex(r.first), r.second));
    }
    return true;
}

bool WalletImpl::setRing(const std::string &key_image, const std::vector<uint64_t> &ring, bool relative)
{
    crypto::key_image raw_key_image;
    if (!epee::string_tools::hex_to_pod(key_image, raw_key_image))
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to parse key image");
        return false;
    }
    bool ret = m_wallet->set_ring(raw_key_image, ring, relative);
    if (!ret)
    {
        m_status = Status_Error;
        m_errorString = tr("Failed to set ring");
        return false;
    }
    return true;
}

void WalletImpl::segregatePreForkOutputs(bool segregate)
{
    m_wallet->segregate_pre_fork_outputs(segregate);
}

void WalletImpl::segregationHeight(uint64_t height)
{
    m_wallet->segregation_height(height);
}

void WalletImpl::keyReuseMitigation2(bool mitigation)
{
    m_wallet->key_reuse_mitigation2(mitigation);
}

} // namespace

namespace Bitmonero = Monero;
