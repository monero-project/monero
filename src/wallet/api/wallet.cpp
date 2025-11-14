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


#include "wallet.h"
#include "device/device.hpp"
#include "crypto/crypto.h"
#include "enote_details.h"
#include "device/device.hpp"
#include "net/net_ssl.h"
#include "pending_transaction.h"
#include "string_tools.h"
#include "unsigned_transaction.h"
#include "transaction_history.h"
#include "address_book.h"
#include "subaddress.h"
#include "subaddress_account.h"
#include "common_defines.h"
#include "common/util.h"
#include "multisig/multisig_account.h"

#include "mnemonics/electrum-words.h"
#include "mnemonics/english.h"
#include "wallet/fee_priority.h"
#include <boost/format.hpp>
#include <sstream>
#include <unordered_map>

#ifdef WIN32
#include <boost/locale.hpp>
#endif

#include <boost/filesystem.hpp>

using namespace std;
using namespace cryptonote;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "WalletAPI"

#define LOCK_REFRESH() \
    bool refresh_enabled = m_refreshEnabled; \
    m_refreshEnabled = false; \
    stop(); \
    m_refreshCV.notify_one(); \
    boost::mutex::scoped_lock lock(m_refreshMutex); \
    boost::mutex::scoped_lock lock2(m_refreshMutex2); \
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler = epee::misc_utils::create_scope_leave_handler([&](){ \
        /* m_refreshMutex's still locked here */ \
        if (refresh_enabled) \
            startRefresh(); \
    })

#define PRE_VALIDATE_BACKGROUND_SYNC() \
  do \
  { \
    clearStatus(); \
    if (m_wallet->key_on_device()) \
    { \
        setStatusError(tr("HW wallet cannot use background sync")); \
        return false; \
    } \
    if (watchOnly()) \
    { \
        setStatusError(tr("View only wallet cannot use background sync")); \
        return false; \
    } \
    if (m_wallet->get_multisig_status().multisig_is_active) \
    { \
        setStatusError(tr("Multisig wallet cannot use background sync")); \
        return false; \
    } \
  } while (0)

namespace Monero {

namespace {
    // copy-pasted from simplewallet
    static const int    DEFAULT_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // limit maximum refresh interval as one minute
    static const int    MAX_REFRESH_INTERVAL_MILLIS = 1000 * 60 * 1;
    // Default refresh interval when connected to remote node
    static const int    DEFAULT_REMOTE_NODE_REFRESH_INTERVAL_MILLIS = 1000 * 10;
    // Connection timeout 20 sec
    static const int    DEFAULT_CONNECTION_TIMEOUT_MILLIS = 1000 * 20;

    std::string get_default_ringdb_path(cryptonote::network_type nettype)
    {
      boost::filesystem::path dir = tools::get_default_data_dir();
      // remove .bitmonero, replace with .shared-ringdb
      dir = dir.remove_filename();
      dir /= ".shared-ringdb";
      if (nettype == cryptonote::TESTNET)
        dir /= "testnet";
      else if (nettype == cryptonote::STAGENET)
        dir /= "stagenet";
      return dir.string();
    }

    void checkMultisigWalletReady(const tools::wallet2* wallet) {
        if (!wallet) {
            throw runtime_error("Wallet is not initialized yet");
        }

        const multisig::multisig_account_status ms_status{wallet->get_multisig_status()};

        if (!ms_status.multisig_is_active) {
            throw runtime_error("Wallet is not multisig");
        }

        if (!ms_status.is_ready) {
            throw runtime_error("Multisig wallet is not finalized yet");
        }
    }
    void checkMultisigWalletReady(const std::unique_ptr<tools::wallet2> &wallet) {
        return checkMultisigWalletReady(wallet.get());
    }

    void checkMultisigWalletNotReady(const tools::wallet2* wallet) {
        if (!wallet) {
            throw runtime_error("Wallet is not initialized yet");
        }

        const multisig::multisig_account_status ms_status{wallet->get_multisig_status()};

        if (!ms_status.multisig_is_active) {
            throw runtime_error("Wallet is not multisig");
        }

        if (ms_status.is_ready) {
            throw runtime_error("Multisig wallet is already finalized");
        }
    }
    void checkMultisigWalletNotReady(const std::unique_ptr<tools::wallet2> &wallet) {
        return checkMultisigWalletNotReady(wallet.get());
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

    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, uint64_t burnt, const cryptonote::subaddress_index& subaddr_index, bool is_change, uint64_t unlock_time, const crypto::public_key& enote_pub_key)
    {

        std::string tx_hash =  epee::string_tools::pod_to_hex(txid);
        std::string enote_pub_key_str = epee::string_tools::pod_to_hex(enote_pub_key);

        LOG_PRINT_L3(__FUNCTION__ << ": money received. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount - burnt)
                     << ", burnt: " << print_money(burnt)
                     << ", raw_output_value: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on received tx if unattended wallet is not syncronized completely
        if (m_listener && (m_wallet->synchronized() || !m_wallet->m_wallet->is_unattended())) {
            m_listener->moneyReceived(tx_hash, amount - burnt, burnt, enote_pub_key_str, is_change, cryptonote::is_coinbase(tx));
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
        // do not signal on received tx if unattended wallet is not syncronized completely
        if (m_listener && (m_wallet->synchronized() || !m_wallet->m_wallet->is_unattended())) {
            m_listener->unconfirmedMoneyReceived(tx_hash, amount);
            m_listener->updated();
        }
    }

    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx,
                                uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index,
                                const crypto::public_key& enote_pub_key)
    {
        // TODO;
        std::string tx_hash = epee::string_tools::pod_to_hex(txid);
        std::string enote_pub_key_str = epee::string_tools::pod_to_hex(enote_pub_key);
        LOG_PRINT_L3(__FUNCTION__ << ": money spent. height:  " << height
                     << ", tx: " << tx_hash
                     << ", amount: " << print_money(amount)
                     << ", idx: " << subaddr_index);
        // do not signal on received tx if unattended wallet is not syncronized completely
        if (m_listener && (m_wallet->synchronized() || !m_wallet->m_wallet->is_unattended())) {
            m_listener->moneySpent(tx_hash, amount, enote_pub_key_str, std::make_pair(subaddr_index.major, subaddr_index.minor));
            m_listener->updated();
        }
    }

    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx)
    {
        // TODO;
    }

    virtual void on_device_button_request(uint64_t code)
    {
      if (m_listener) {
        m_listener->onDeviceButtonRequest(code);
      }
    }

    virtual void on_device_button_pressed()
    {
      if (m_listener) {
        m_listener->onDeviceButtonPressed();
      }
    }

    virtual boost::optional<epee::wipeable_string> on_device_pin_request()
    {
      if (m_listener) {
        auto pin = m_listener->onDevicePinRequest();
        if (pin){
          return boost::make_optional(epee::wipeable_string((*pin).data(), (*pin).size()));
        }
      }
      return boost::none;
    }

    virtual boost::optional<epee::wipeable_string> on_device_passphrase_request(bool & on_device)
    {
      if (m_listener) {
        auto passphrase = m_listener->onDevicePassphraseRequest(on_device);
        if (passphrase) {
          return boost::make_optional(epee::wipeable_string((*passphrase).data(), (*passphrase).size()));
        }
      } else {
        on_device = true;
      }
      return boost::none;
    }

    virtual void on_device_progress(const hw::device_progress & event)
    {
      if (m_listener) {
        m_listener->onDeviceProgress(DeviceProgress(event.progress(), event.indeterminate()));
      }
    }

    virtual void on_reorg(std::uint64_t height, std::uint64_t blocks_detached, std::size_t transfers_detached)
    {
        if (m_listener) {
            m_listener->onReorg(height, blocks_detached, transfers_detached);
        }
    }

    virtual boost::optional<epee::wipeable_string> on_get_password(const char *reason)
    {
        if (m_listener) {
            auto password = m_listener->onGetPassword(reason);
            if (password) {
                return boost::optional<epee::wipeable_string>(*password);
            }
        }
        return boost::none;
    }

    virtual void on_pool_tx_removed(const crypto::hash &txid)
    {
        std::string txid_hex = epee::string_tools::pod_to_hex(txid);
        if (m_listener) {
            m_listener->onPoolTxRemoved(txid_hex);
        }
    }

    WalletListener * m_listener;
    WalletImpl     * m_wallet;
};

WalletKeysDecryptGuard::~WalletKeysDecryptGuard() {}

struct WalletKeysDecryptGuardImpl: public WalletKeysDecryptGuard
{
    WalletKeysDecryptGuardImpl(tools::wallet2 &w, const epee::wipeable_string &password):
        u(w, &password)
    {}

    ~WalletKeysDecryptGuardImpl() override = default;

    tools::wallet_keys_unlocker u;
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

bool Wallet::isSubaddress(const std::string &address, NetworkType nettype)
{
  cryptonote::address_parse_info info;
  return get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), address) ? info.is_subaddress : false;
}

std::string Wallet::getAddressFromIntegrated(const std::string &address, NetworkType nettype)
{
  cryptonote::address_parse_info info;
  if (!get_account_address_from_str(info, static_cast<cryptonote::network_type>(nettype), address))
    return "";
  return get_account_address_as_str(static_cast<cryptonote::network_type>(nettype), info.is_subaddress, info.address);
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

bool Wallet::walletExists(const std::string &path, bool &key_file_exists, bool &wallet_file_exists)
{
    tools::wallet2::wallet_exists(path, key_file_exists, wallet_file_exists);
    return (key_file_exists || wallet_file_exists);
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
WalletImpl::WalletImpl(NetworkType nettype, uint64_t kdf_rounds, const bool unattended /* = true */)
    :m_wallet(nullptr)
    , m_status(Wallet::Status_Ok)
    , m_wallet2Callback(nullptr)
    , m_recoveringFromSeed(false)
    , m_recoveringFromDevice(false)
    , m_synchronized(false)
    , m_rebuildWalletCache(false)
    , m_is_connected(false)
    , m_refreshShouldRescan(false)
    , m_kdf_rounds(kdf_rounds)
{
    m_wallet.reset(new tools::wallet2(static_cast<cryptonote::network_type>(nettype), kdf_rounds, unattended));
    m_history.reset(new TransactionHistoryImpl(this));
    m_wallet2Callback.reset(new Wallet2CallbackImpl(this));
    m_wallet->callback(m_wallet2Callback.get());
    m_refreshThreadDone = false;
    m_refreshEnabled = false;
    m_addressBook.reset(new AddressBookImpl(this));
    m_subaddress.reset(new SubaddressImpl(this));
    m_subaddressAccount.reset(new SubaddressAccountImpl(this));


    m_refreshIntervalMillis = DEFAULT_REFRESH_INTERVAL_MILLIS;

    m_refreshThread = boost::thread([this] () {
        this->refreshThreadFunc();
    });

}

WalletImpl::~WalletImpl()
{

    LOG_PRINT_L1(__FUNCTION__);
    m_wallet->callback(NULL);
    // Pause refresh thread - prevents refresh from starting again
    WalletImpl::pauseRefresh(); // Call the method directly (not polymorphically) to protect against UB in destructor.
    // Close wallet - stores cache and stops ongoing refresh operation 
    close(false); // do not store wallet as part of the closing activities
    // Stop refresh thread
    stopRefresh();

    if (m_wallet2Callback->getListener()) {
      m_wallet2Callback->getListener()->onSetWallet(nullptr);
    }

    LOG_PRINT_L1(__FUNCTION__ << " finished");
}

bool WalletImpl::create(const std::string &path, const std::string &password, const std::string &language, bool create_address_file /* = false */, bool non_deterministic /* = false */)
{

    clearStatus();
    m_recoveringFromSeed = false;
    m_recoveringFromDevice = false;
    bool keys_file_exists;
    bool wallet_file_exists;
    Wallet::walletExists(path, keys_file_exists, wallet_file_exists);
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);


    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        std::string error = "attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(error);
        setStatusCritical(error);
        return false;
    }
    if (!non_deterministic)
        setSeedLanguage(language);
    if (!statusOk())
        return false;
    crypto::secret_key recovery_val, secret_key;
    try {
        recovery_val = m_wallet->generate(path, password, secret_key, /* recover */ false, non_deterministic, create_address_file);
        clearStatus();
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating wallet: " << e.what());
        setStatusCritical(e.what());
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
    Wallet::walletExists(path, keys_file_exists, wallet_file_exists);
    LOG_PRINT_L3("wallet_path: " << path << "");
    LOG_PRINT_L3("keys_file_exists: " << std::boolalpha << keys_file_exists << std::noboolalpha
                 << "  wallet_file_exists: " << std::boolalpha << wallet_file_exists << std::noboolalpha);

    // add logic to error out if new wallet requested but named wallet file exists
    if (keys_file_exists || wallet_file_exists) {
        std::string error = "attempting to generate view only wallet, but specified file(s) exist.  Exiting to not risk overwriting.";
        LOG_ERROR(error);
        setStatusError(error);
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
        auto outputs = m_wallet->export_outputs(true/*all*/);
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
        auto key_images = m_wallet->export_key_images(true/*all*/);
        uint64_t spent = 0;
        uint64_t unspent = 0;
        view_wallet->import_key_images(key_images.second, key_images.first, spent, unspent, false);
        clearStatus();
    } catch (const std::exception &e) {
        LOG_ERROR("Error creating view only wallet: " << e.what());
        setStatusError(e.what());
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
        setStatusError(tr("failed to parse address"));
        return false;
    }

    // parse optional spend key
    crypto::secret_key spendkey;
    bool has_spendkey = false;
    if (!spendkey_string.empty()) {
        cryptonote::blobdata spendkey_data;
        if(!epee::string_tools::parse_hexstr_to_binbuff(spendkey_string, spendkey_data) || spendkey_data.size() != sizeof(crypto::secret_key))
        {
            setStatusError(tr("failed to parse secret spend key"));
            return false;
        }
        has_spendkey = true;
        spendkey = *reinterpret_cast<const crypto::secret_key*>(spendkey_data.data());
    }

    // parse view secret key
    bool has_viewkey = true;
    crypto::secret_key viewkey;
    if (viewkey_string.empty()) {
        if(has_spendkey) {
          has_viewkey = false;
        }
        else {
          setStatusError(tr("Neither view key nor spend key supplied, cancelled"));
          return false;
        }
    }
    if(has_viewkey) {
      cryptonote::blobdata viewkey_data;
      if(!epee::string_tools::parse_hexstr_to_binbuff(viewkey_string, viewkey_data) || viewkey_data.size() != sizeof(crypto::secret_key))
      {
          setStatusError(tr("failed to parse secret view key"));
          return false;
      }
      viewkey = *reinterpret_cast<const crypto::secret_key*>(viewkey_data.data());
    }
    // check the spend and view keys match the given address
    crypto::public_key pkey;
    if(has_spendkey) {
        if (!crypto::secret_key_to_public_key(spendkey, pkey)) {
            setStatusError(tr("failed to verify secret spend key"));
            return false;
        }
        if (info.address.m_spend_public_key != pkey) {
            setStatusError(tr("spend key does not match address"));
            return false;
        }
    }
    if(has_viewkey) {
       if (!crypto::secret_key_to_public_key(viewkey, pkey)) {
           setStatusError(tr("failed to verify secret view key"));
           return false;
       }
       if (info.address.m_view_public_key != pkey) {
           setStatusError(tr("view key does not match address"));
           return false;
       }
    }

    try
    {
        if (has_spendkey && has_viewkey) {
            m_wallet->generate(path, password, info.address, spendkey, viewkey);
            LOG_PRINT_L1("Generated new wallet from spend key and view key");
        }
        if(!has_spendkey && has_viewkey) {
            m_wallet->generate(path, password, info.address, viewkey);
            LOG_PRINT_L1("Generated new view only wallet from keys");
        }
        if(has_spendkey && !has_viewkey) {
           m_wallet->generate(path, password, spendkey, /* recover */ true, /* non-deterministic */ false);
           setSeedLanguage(language);
           if (!statusOk())
               return false;
           LOG_PRINT_L1("Generated deterministic wallet from spend key with seed language: " + language);
        }
        
    }
    catch (const std::exception& e) {
        setStatusError(string(tr("failed to generate new wallet: ")) + e.what());
        return false;
    }
    m_wallet->set_ring_database(get_default_ringdb_path(m_wallet->nettype()));
    return true;
}

bool WalletImpl::createFromJson(const std::string &json_file_path, std::string &pw_out)
{
    try
    {
        boost::program_options::variables_map vm;
        boost::program_options::store(
            boost::program_options::command_line_parser({
                nettype() == TESTNET ? "--testnet" : nettype() == STAGENET ? "--stagenet" : "",
            }).options([]{
                boost::program_options::options_description options_description{};
                tools::wallet2::init_options(options_description);
                return options_description;
            }()
            ).run(),
            vm
        );
        auto r = m_wallet->make_from_json(vm, m_wallet->is_unattended(), json_file_path, /* password_promper */ {});
        if (!r.first) {
            setStatusError(tr("failed to generate new wallet from json"));
            return false;
        }
        m_wallet = std::move(r.first);
        pw_out = std::string(r.second.password().data(), r.second.password().size());
        LOG_PRINT_L1("Generated new wallet from json");
    }
    catch (const std::exception& e) {
        setStatusError(string(tr("failed to generate new wallet from json: ")) + e.what());
        return false;
    }
    m_wallet->set_ring_database(get_default_ringdb_path(m_wallet->nettype()));
    return true;
}

bool WalletImpl::recoverFromMultisigSeed(const std::string &path,
                                         const std::string &password,
                                         const std::string &language,
                                         const std::string &multisig_seed,
                                         const std::string seed_pass /* = "" */,
                                         const bool do_create_address_file /* = false */)
{
    try
    {
        if (seed_pass.empty())
            m_wallet->generate(path, password, multisig_seed, do_create_address_file);
        else
        {
            crypto::secret_key key;
            crypto::cn_slow_hash(seed_pass.data(), seed_pass.size(), (crypto::hash&)key);
            sc_reduce32((unsigned char*)key.data);
            const epee::wipeable_string &msig_keys = m_wallet->decrypt<epee::wipeable_string>(std::string(multisig_seed.data(), multisig_seed.size()), key, true);
            m_wallet->generate(path, password, msig_keys, do_create_address_file);
        }
        setSeedLanguage(language);
        LOG_PRINT_L1("Generated new multisig wallet from multisig seed");
    }
    catch (const std::exception& e) {
        setStatusError(string(tr("failed to generate new multisig wallet: ")) + e.what());
        return false;
    }
    m_wallet->set_ring_database(get_default_ringdb_path(m_wallet->nettype()));
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
        setStatusError(string(tr("failed to generate new wallet: ")) + e.what());
        return false;
    }
    return true;
}

Wallet::Device WalletImpl::getDeviceType() const
{
    return static_cast<Wallet::Device>(m_wallet->get_device_type());
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
        Wallet::walletExists(path, keys_file_exists, wallet_file_exists);
        if(!wallet_file_exists){
            // Rebuilding wallet cache, using refresh height from .keys file
            m_rebuildWalletCache = true;
        }
        m_wallet->set_ring_database(get_default_ringdb_path(m_wallet->nettype()));
        m_wallet->load(path, password);

        m_password = password;
    } catch (const std::exception &e) {
        LOG_ERROR("Error opening wallet: " << e.what());
        setStatusCritical(e.what());
    }
    return statusOk();
}

bool WalletImpl::recover(const std::string &path, const std::string &seed)
{
    return recover(path, "", seed);
}

bool WalletImpl::recover(const std::string &path, const std::string &password, const std::string &seed, const std::string &seed_offset/* = {}*/)
{
    clearStatus();
    m_errorString.clear();
    if (seed.empty()) {
        LOG_ERROR("Electrum seed is empty");
        setStatusError(tr("Electrum seed is empty"));
        return false;
    }

    m_recoveringFromSeed = true;
    m_recoveringFromDevice = false;
    crypto::secret_key recovery_key;
    std::string old_language;
    if (!crypto::ElectrumWords::words_to_bytes(seed, recovery_key, old_language)) {
        setStatusError(tr("Electrum-style word list failed verification"));
        return false;
    }
    if (!seed_offset.empty())
    {
        recovery_key = cryptonote::decrypt_key(recovery_key, seed_offset);
    }

    if (old_language == crypto::ElectrumWords::old_language_name)
        old_language = Language::English().get_language_name();

    try {
        setSeedLanguage(old_language);
        if (!statusOk())
            return false;
        m_wallet->generate(path, password, recovery_key, true, false);

    } catch (const std::exception &e) {
        setStatusCritical(e.what());
    }
    return statusOk();
}

bool WalletImpl::close(bool do_store)
{

    bool result = false;
    LOG_PRINT_L1("closing wallet...");
    try {
        if (do_store) {
            // Do not store wallet with invalid status
            // Status Critical refers to errors on opening or creating wallets.
            if (status() != Status_Critical)
                store("");
            else
                LOG_ERROR("Status_Critical - not saving wallet");
            LOG_PRINT_L1("wallet::store done");
        }
        LOG_PRINT_L1("Calling wallet::stop...");
        stop();
        LOG_PRINT_L1("wallet::stop done");
        m_wallet->deinit();
        result = true;
        clearStatus();
    } catch (const std::exception &e) {
        setStatusCritical(e.what());
        LOG_ERROR("Error closing wallet: " << e.what());
    }
    return result;
}

std::string WalletImpl::seed(const std::string& seed_offset) const
{
    if (checkBackgroundSync("cannot get seed"))
        return std::string();
    epee::wipeable_string seed;
    if (m_wallet)
        m_wallet->get_seed(seed, seed_offset);
    return std::string(seed.data(), seed.size()); // TODO
}

int WalletImpl::status() const
{
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    return m_status;
}

std::string WalletImpl::errorString() const
{
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    return m_errorString;
}

void WalletImpl::statusWithErrorString(int& status, std::string& errorString) const {
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    status = m_status;
    errorString = m_errorString;
}

bool WalletImpl::setPassword(const std::string &password)
{
    if (checkBackgroundSync("cannot change password"))
        return false;
    clearStatus();
    try {
        m_wallet->change_password(m_wallet->get_wallet_file(), m_password, password);
        m_password = password;
    } catch (const std::exception &e) {
        setStatusError(e.what());
    }
    return statusOk();
}

const std::string& WalletImpl::getPassword() const
{
    return m_password;
}

bool WalletImpl::setDevicePin(const std::string &pin)
{
    clearStatus();
    try {
        m_wallet->get_account().get_device().set_pin(epee::wipeable_string(pin.data(), pin.size()));
    } catch (const std::exception &e) {
        setStatusError(e.what());
    }
    return statusOk();
}

bool WalletImpl::setDevicePassphrase(const std::string &passphrase)
{
    clearStatus();
    try {
        m_wallet->get_account().get_device().set_passphrase(epee::wipeable_string(passphrase.data(), passphrase.size()));
    } catch (const std::exception &e) {
        setStatusError(e.what());
    }
    return statusOk();
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
    return epee::string_tools::pod_to_hex(unwrap(unwrap(m_wallet->get_account().get_keys().m_view_secret_key)));
}

std::string WalletImpl::publicViewKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_view_public_key);
}

std::string WalletImpl::secretSpendKey() const
{
    return epee::string_tools::pod_to_hex(unwrap(unwrap(m_wallet->get_account().get_keys().m_spend_secret_key)));
}

std::string WalletImpl::publicSpendKey() const
{
    return epee::string_tools::pod_to_hex(m_wallet->get_account().get_keys().m_account_address.m_spend_public_key);
}

std::string WalletImpl::publicMultisigSignerKey() const
{
    try {
        crypto::public_key signer = m_wallet->get_multisig_signer_public_key();
        return epee::string_tools::pod_to_hex(signer);
    } catch (const std::exception&) {
        return "";
    }
}

std::string WalletImpl::path() const
{
    return m_wallet->path();
}

void WalletImpl::stop()
{
    m_wallet->stop();
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
        setStatusError(e.what());
        return false;
    }

    return true;
}

string WalletImpl::filename() const
{
    return m_wallet->get_wallet_file();
}

string WalletImpl::keysFilename() const
{
    return m_wallet->get_keys_file();
}

bool WalletImpl::init(const std::string &daemon_address, uint64_t upper_transaction_size_limit, const std::string &daemon_username, const std::string &daemon_password, bool use_ssl, bool lightWallet, const std::string &proxy_address)
{
    clearStatus();
    if(daemon_username != "")
        m_daemon_login.emplace(daemon_username, daemon_password);
    return doInit(daemon_address, proxy_address, upper_transaction_size_limit, use_ssl);
}

void WalletImpl::setRecoveringFromSeed(bool recoveringFromSeed)
{
    m_recoveringFromSeed = recoveringFromSeed;
}

void WalletImpl::setRecoveringFromDevice(bool recoveringFromDevice)
{
    m_recoveringFromDevice = recoveringFromDevice;
}

uint64_t WalletImpl::balance(uint32_t accountIndex) const
{
    return m_wallet->balance(accountIndex, false);
}

std::map<uint32_t, uint64_t> WalletImpl::balancePerSubaddress(uint32_t accountIndex /* = 0 */) const
{
    return m_wallet->balance_per_subaddress(accountIndex, false);
}

uint64_t WalletImpl::unlockedBalance(uint32_t accountIndex /* = 0 */,
                                     uint64_t *blocks_to_unlock /* = NULL */,
                                     uint64_t *time_to_unlock /* = NULL */) const
{
    return m_wallet->unlocked_balance(accountIndex, false, blocks_to_unlock, time_to_unlock);
}

std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> WalletImpl::unlockedBalancePerSubaddress(uint32_t accountIndex /* = 0 */) const
{
    return m_wallet->unlocked_balance_per_subaddress(accountIndex, false);
}

uint64_t WalletImpl::blockChainHeight() const
{
    return m_wallet->get_blockchain_current_height();
}
uint64_t WalletImpl::approximateBlockChainHeight() const
{
    return m_wallet->get_approximate_blockchain_height();
}

uint64_t WalletImpl::estimateBlockChainHeight() const
{
    return m_wallet->estimate_blockchain_height();
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
        setStatusError(err);
    } else {
        clearStatus();
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
        setStatusError(err);
    } else {
        clearStatus();
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

bool WalletImpl::refresh(std::uint64_t start_height /* = 0 */,
                         bool check_pool /* = true */,
                         bool try_incremental /* = false */,
                         std::uint64_t max_blocks /* = std::numeric_limits<uint64_t>::max() */,
                         std::uint64_t *blocks_fetched_out /* = nullptr */,
                         bool *received_money_out /* = nullptr */)
{
    clearStatus();
    bool did_error_occur;
    doRefresh(start_height, check_pool, try_incremental, max_blocks, &did_error_occur, blocks_fetched_out, received_money_out);
    return !did_error_occur;
}

void WalletImpl::refreshAsync()
{
    LOG_PRINT_L3(__FUNCTION__ << ": Refreshing asynchronously..");
    clearStatus();
    m_refreshCV.notify_one();
}

bool WalletImpl::rescanBlockchain(bool do_hard_rescan /* = false */, bool do_keep_key_images /* = false */, bool do_skip_refresh /* = false */)
{
    if (checkBackgroundSync("cannot rescan blockchain"))
        return false;
    clearStatus();
    m_refreshShouldRescan = true;
    m_do_hard_rescan = do_hard_rescan;
    m_do_keep_key_images_on_rescan = do_keep_key_images;
    if (!do_skip_refresh)
        doRefresh();
    return statusOk();
}

void WalletImpl::rescanBlockchainAsync(bool do_hard_rescan /* = false */, bool do_keep_key_images /* = false */)
{
    if (checkBackgroundSync("cannot rescan blockchain"))
        return;
    m_refreshShouldRescan = true;
    m_do_hard_rescan = do_hard_rescan;
    m_do_keep_key_images_on_rescan = do_keep_key_images;
    refreshAsync();
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
  if (checkBackgroundSync("cannot load tx") || !m_wallet->load_unsigned_tx(unsigned_filename, transaction->m_unsigned_tx_set)){
    setStatusError(tr("Failed to load unsigned transactions"));
    transaction->m_status = UnsignedTransaction::Status::Status_Error;
    transaction->m_errorString = errorString();

    return transaction;
  }
  
  // Check tx data and construct confirmation message
  std::string extra_message;
  if (!std::get<2>(transaction->m_unsigned_tx_set.transfers).empty())
    extra_message = (boost::format("%u outputs to import. ") % (unsigned)std::get<2>(transaction->m_unsigned_tx_set.transfers).size()).str();
  transaction->checkLoadedTx([&transaction](){return transaction->m_unsigned_tx_set.txes.size();}, [&transaction](size_t n)->const tools::wallet2::tx_construction_data&{return transaction->m_unsigned_tx_set.txes[n];}, extra_message);
  setStatus(transaction->status(), transaction->errorString());
    
  return transaction;
}

UnsignedTransaction *WalletImpl::loadUnsignedTxFromStr(const std::string &unsigned_tx_str)
{
    clearStatus();
    std::unique_ptr<UnsignedTransactionImpl> transaction(new UnsignedTransactionImpl(*this));
    if (checkBackgroundSync("cannot load tx") || !m_wallet->parse_unsigned_tx_from_str(unsigned_tx_str, transaction->m_unsigned_tx_set))
    {
        setStatusError(tr("Failed to load unsigned transaction"));
        transaction->m_status = UnsignedTransaction::Status::Status_Error;
        transaction->m_errorString = errorString();

        return transaction.release();
    }

    // Check tx data and construct confirmation message
    std::string extra_message;
    if (!std::get<2>(transaction->m_unsigned_tx_set.transfers).empty())
        extra_message = (boost::format("%u outputs to import. ") % (unsigned)std::get<2>(transaction->m_unsigned_tx_set.transfers).size()).str();
    transaction->checkLoadedTx([&transaction](){return transaction->m_unsigned_tx_set.txes.size();}, [&transaction](size_t n)->const tools::wallet2::tx_construction_data&{return transaction->m_unsigned_tx_set.txes[n];}, extra_message);
    setStatus(transaction->status(), transaction->errorString());

    return transaction.release();
}

bool WalletImpl::submitTransaction(const string &fileName) {
  clearStatus();
  if (checkBackgroundSync("cannot submit tx"))
    return false;
  std::unique_ptr<PendingTransactionImpl> transaction(new PendingTransactionImpl(*this));

  bool r = m_wallet->load_tx(fileName, transaction->m_pending_tx);
  if (!r) {
    setStatusError(tr("Failed to load transaction from file"));
    return false;
  }

  if(!transaction->commit()) {
    setStatusError(transaction->m_errorString);
    return false;
  }

  return true;
}

bool WalletImpl::exportKeyImages(const string &filename, bool all) 
{
  if (watchOnly())
  {
    setStatusError(tr("Wallet is view only"));
    return false;
  }
  if (checkBackgroundSync("cannot export key images"))
    return false;
  
  try
  {
    if (!m_wallet->export_key_images(filename, all))
    {
      setStatusError(tr("failed to save file ") + filename);
      return false;
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    setStatusError(e.what());
    return false;
  }
  return true;
}

std::string WalletImpl::exportKeyImagesAsString(bool all /* = false */)
{
    clearStatus();
    if (watchOnly())
    {
        setStatusError(tr("Wallet is view only"));
        return "";
    }
    if (checkBackgroundSync("cannot export key images"))
        return "";

    try
    {
        return m_wallet->export_key_images_to_str(all);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error exporting key images: " << e.what());
        setStatusError(e.what());
        return "";
    }
    return "";
}

bool WalletImpl::importKeyImages(const std::string &filename, std::uint64_t *spent_out /* = nullptr */, std::uint64_t *unspent_out /* = nullptr */, std::uint64_t *import_height /* = nullptr */)
{
  if (checkBackgroundSync("cannot import key images"))
    return false;
  if (!trustedDaemon()) {
    setStatusError(tr("Key images can only be imported with a trusted daemon"));
    return false;
  }
  try
  {
    uint64_t spent = 0, unspent = 0, height;
    if (!spent_out)
        spent_out = &spent;
    if (!unspent_out)
        unspent_out = &unspent;
    if (!import_height)
        import_height = &height;
    *import_height = m_wallet->import_key_images(filename, *spent_out, *unspent_out);
    LOG_PRINT_L2("Signed key images imported to height " << *import_height << ", "
        << print_money(*spent_out) << " spent, " << print_money(*unspent_out) << " unspent");
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error exporting key images: " << e.what());
    setStatusError(string(tr("Failed to import key images: ")) + e.what());
    return false;
  }

  return true;
}

bool WalletImpl::importKeyImagesFromStr(const std::string &data)
{
    clearStatus();
    if (checkBackgroundSync("cannot import key images"))
        return false;
    if (!trustedDaemon()) {
        setStatusError(tr("Key images can only be imported with a trusted daemon"));
        return false;
    }
    try
    {
        uint64_t spent = 0, unspent = 0;
        uint64_t height = m_wallet->import_key_images_from_str(data, spent, unspent);
        LOG_PRINT_L2("Signed key images imported to height " << height << ", "
                << print_money(spent) << " spent, " << print_money(unspent) << " unspent");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error importing key images: " << e.what());
        setStatusError(string(tr("Failed to import key images: ")) + e.what());
        return false;
    }

    return true;
}

bool WalletImpl::exportOutputs(const string &filename, bool all)
{
    if (checkBackgroundSync("cannot export outputs"))
        return false;
    if (m_wallet->key_on_device())
    {
        setStatusError(string(tr("Not supported on HW wallets.")) + filename);
        return false;
    }

    try
    {
        std::string data = m_wallet->export_outputs_to_str(all);
        bool r = m_wallet->save_to_file(filename, data);
        if (!r)
        {
            LOG_ERROR("Failed to save file " << filename);
            setStatusError(string(tr("Failed to save file: ")) + filename);
            return false;
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error exporting outputs: " << e.what());
        setStatusError(string(tr("Error exporting outputs: ")) + e.what());
        return false;
    }

    LOG_PRINT_L2("Outputs exported to " << filename);
    return true;
}

bool WalletImpl::importOutputs(const string &filename)
{
    if (checkBackgroundSync("cannot import outputs"))
        return false;
    if (m_wallet->key_on_device())
    {
        setStatusError(string(tr("Not supported on HW wallets.")) + filename);
        return false;
    }

    std::string data;
    bool r = m_wallet->load_from_file(filename, data);
    if (!r)
    {
        LOG_ERROR("Failed to read file: " << filename);
        setStatusError(string(tr("Failed to read file: ")) + filename);
        return false;
    }

    try
    {
        size_t n_outputs = m_wallet->import_outputs_from_str(data);
        if (!statusOk())
            throw runtime_error(errorString());
        LOG_PRINT_L2(std::to_string(n_outputs) << " outputs imported");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to import outputs: " << e.what());
        setStatusError(string(tr("Failed to import outputs: ")) + e.what());
        return false;
    }

    return true;
}

bool WalletImpl::scanTransactions(const std::vector<std::string> &txids, bool *wont_reprocess_recent_txs_via_untrusted_daemon /* = nullptr */ )
{
    if (checkBackgroundSync("cannot scan transactions"))
        return false;
    if (txids.empty())
    {
        setStatusError(string(tr("Failed to scan transactions: no transaction ids provided.")));
        return false;
    }

    // Parse and dedup args
    std::unordered_set<crypto::hash> txids_u;
    for (const auto &s : txids)
    {
        crypto::hash txid;
        if (!epee::string_tools::hex_to_pod(s, txid))
        {
            setStatusError(string(tr("Invalid txid specified: ")) + s);
            return false;
        }
        txids_u.insert(txid);
    }

    try
    {
        m_wallet->scan_tx(txids_u);
    }
    catch (const tools::error::wont_reprocess_recent_txs_via_untrusted_daemon &e)
    {
        if (wont_reprocess_recent_txs_via_untrusted_daemon)
            *wont_reprocess_recent_txs_via_untrusted_daemon = true;
        setStatusError(e.what());
        return false;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to scan transaction: " << e.what());
        setStatusError(string(tr("Failed to scan transaction: ")) + e.what());
        return false;
    }

    return true;
}

bool WalletImpl::setupBackgroundSync(const Wallet::BackgroundSyncType background_sync_type, const std::string &wallet_password, const optional<std::string> &background_cache_password)
{
    try
    {
        PRE_VALIDATE_BACKGROUND_SYNC();

        tools::wallet2::BackgroundSyncType bgs_type;
        switch (background_sync_type)
        {
            case Wallet::BackgroundSync_Off: bgs_type = tools::wallet2::BackgroundSyncOff; break;
            case Wallet::BackgroundSync_ReusePassword: bgs_type = tools::wallet2::BackgroundSyncReusePassword; break;
            case Wallet::BackgroundSync_CustomPassword: bgs_type = tools::wallet2::BackgroundSyncCustomPassword; break;
            default: setStatusError(tr("Unknown background sync type")); return false;
        }

        boost::optional<epee::wipeable_string> bgc_password = background_cache_password
            ? boost::optional<epee::wipeable_string>(*background_cache_password)
            : boost::none;

        LOCK_REFRESH();
        m_wallet->setup_background_sync(bgs_type, wallet_password, bgc_password);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to setup background sync: " << e.what());
        setStatusError(string(tr("Failed to setup background sync: ")) + e.what());
        return false;
    }
    return true;
}

Wallet::BackgroundSyncType WalletImpl::getBackgroundSyncType() const
{
    switch (m_wallet->background_sync_type())
    {
        case tools::wallet2::BackgroundSyncOff: return Wallet::BackgroundSync_Off;
        case tools::wallet2::BackgroundSyncReusePassword: return Wallet::BackgroundSync_ReusePassword;
        case tools::wallet2::BackgroundSyncCustomPassword: return Wallet::BackgroundSync_CustomPassword;
        default: setStatusError(tr("Unknown background sync type")); return Wallet::BackgroundSync_Off;
    }
}

bool WalletImpl::startBackgroundSync()
{
    try
    {
        PRE_VALIDATE_BACKGROUND_SYNC();
        LOCK_REFRESH();
        m_wallet->start_background_sync();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to start background sync: " << e.what());
        setStatusError(string(tr("Failed to start background sync: ")) + e.what());
        return false;
    }
    return true;
}

bool WalletImpl::stopBackgroundSync(const std::string &wallet_password)
{
    try
    {
        PRE_VALIDATE_BACKGROUND_SYNC();
        LOCK_REFRESH();
        m_wallet->stop_background_sync(epee::wipeable_string(wallet_password));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Failed to stop background sync: " << e.what());
        setStatusError(string(tr("Failed to stop background sync: ")) + e.what());
        return false;
    }
    return true;
}

void WalletImpl::addSubaddressAccount(const std::string& label)
{
    if (checkBackgroundSync("cannot add account"))
        return;
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
    if (checkBackgroundSync("cannot add subbaddress"))
        return;
    m_wallet->add_subaddress(accountIndex, label);
}
std::string WalletImpl::getSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex) const
{
    if (checkBackgroundSync("cannot get subbaddress label"))
        return "";
    try
    {
        return m_wallet->get_subaddress_label({accountIndex, addressIndex});
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error getting subaddress label: " << e.what());
        setStatusError(string(tr("Failed to get subaddress label: ")) + e.what());
        return "";
    }
}
void WalletImpl::setSubaddressLabel(uint32_t accountIndex, uint32_t addressIndex, const std::string &label)
{
    if (checkBackgroundSync("cannot set subbaddress label"))
        return;
    try
    {
        return m_wallet->set_subaddress_label({accountIndex, addressIndex}, label);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Error setting subaddress label: " << e.what());
        setStatusError(string(tr("Failed to set subaddress label: ")) + e.what());
    }
}

MultisigState WalletImpl::multisig() const {
    MultisigState state;
    if (checkBackgroundSync("cannot use multisig"))
        return state;

    const multisig::multisig_account_status ms_status{m_wallet->get_multisig_status()};

    state.isMultisig = ms_status.multisig_is_active;
    state.kexIsDone = ms_status.kex_is_done;
    state.isReady = ms_status.is_ready;
    state.threshold = ms_status.threshold;
    state.total = ms_status.total;

    return state;
}

string WalletImpl::getMultisigInfo() const {
    if (checkBackgroundSync("cannot use multisig"))
        return string();
    try {
        clearStatus();
        return m_wallet->get_multisig_first_kex_msg();
    } catch (const exception& e) {
        LOG_ERROR("Error on generating multisig info: " << e.what());
        setStatusError(string(tr("Failed to get multisig info: ")) + e.what());
    }

    return string();
}

string WalletImpl::makeMultisig(const vector<string>& info, const uint32_t threshold) {
    if (checkBackgroundSync("cannot make multisig"))
        return string();
    try {
        clearStatus();

        if (multisig().isMultisig) {
            throw runtime_error("Wallet is already multisig");
        }

        return m_wallet->make_multisig(epee::wipeable_string(m_password), info, threshold);
    } catch (const exception& e) {
        LOG_ERROR("Error on making multisig wallet: " << e.what());
        setStatusError(string(tr("Failed to make multisig: ")) + e.what());
    }

    return string();
}

std::string WalletImpl::exchangeMultisigKeys(const std::vector<std::string> &info, const bool force_update_use_with_caution /*= false*/) {
    try {
        clearStatus();
        checkMultisigWalletNotReady(m_wallet);

        return m_wallet->exchange_multisig_keys(epee::wipeable_string(m_password), info, force_update_use_with_caution);
    } catch (const exception& e) {
        LOG_ERROR("Error on exchanging multisig keys: " << e.what());
        setStatusError(string(tr("Failed to exchange multisig keys: ")) + e.what());
    }

    return string();
}

std::string WalletImpl::getMultisigKeyExchangeBooster(const std::vector<std::string> &info,
    const std::uint32_t threshold,
    const std::uint32_t num_signers) {
    try {
        clearStatus();

        return m_wallet->get_multisig_key_exchange_booster(epee::wipeable_string(m_password), info, threshold, num_signers);
    } catch (const exception& e) {
        LOG_ERROR("Error on boosting multisig key exchange: " << e.what());
        setStatusError(string(tr("Failed to boost multisig key exchange: ")) + e.what());
    }

    return string();
}

bool WalletImpl::exportMultisigImages(string& images) {
    try {
        clearStatus();
        checkMultisigWalletReady(m_wallet);

        auto blob = m_wallet->export_multisig();
        images = epee::string_tools::buff_to_hex_nodelimer(blob);
        return true;
    } catch (const exception& e) {
        LOG_ERROR("Error on exporting multisig images: " << e.what());
        setStatusError(string(tr("Failed to export multisig images: ")) + e.what());
    }

    return false;
}

size_t WalletImpl::importMultisigImages(const vector<string>& images) {
    try {
        clearStatus();
        checkMultisigWalletReady(m_wallet);

        std::vector<std::string> blobs;
        blobs.reserve(images.size());

        for (const auto& image: images) {
            std::string blob;
            if (!epee::string_tools::parse_hexstr_to_binbuff(image, blob)) {
                LOG_ERROR("Failed to parse imported multisig images");
                setStatusError(tr("Failed to parse imported multisig images"));
                return 0;
            }

            blobs.emplace_back(std::move(blob));
        }

        return m_wallet->import_multisig(blobs);
    } catch (const exception& e) {
        LOG_ERROR("Error on importing multisig images: " << e.what());
        setStatusError(string(tr("Failed to import multisig images: ")) + e.what());
    }

    return 0;
}

bool WalletImpl::hasMultisigPartialKeyImages() const {
    try {
        clearStatus();
        checkMultisigWalletReady(m_wallet);

        return m_wallet->has_multisig_partial_key_images();
    } catch (const exception& e) {
        LOG_ERROR("Error on checking for partial multisig key images: " << e.what());
        setStatusError(string(tr("Failed to check for partial multisig key images: ")) + e.what());
    }

    return false;
}

PendingTransaction* WalletImpl::restoreMultisigTransaction(const string& signData, bool ask_for_confirmation /* = false */) {
    try {
        clearStatus();
        checkMultisigWalletReady(m_wallet);

        string binary;
        if (!epee::string_tools::parse_hexstr_to_binbuff(signData, binary)) {
            throw runtime_error("Failed to deserialize multisig transaction");
        }

        tools::wallet2::multisig_tx_set txSet;
        if (!m_wallet->load_multisig_tx(binary, txSet, {}, ask_for_confirmation)) {
          throw runtime_error("couldn't parse multisig transaction data");
        }

        auto ptx = new PendingTransactionImpl(*this);
        ptx->m_pending_tx = txSet.m_ptx;
        ptx->m_signers = txSet.m_signers;

        // Check tx data and construct confirmation message
        if (ask_for_confirmation) {
            std::string extra_message;
            ptx->checkLoadedTx([&ptx](){return ptx->txCount();}, [&ptx](size_t n)->const tools::wallet2::tx_construction_data&{return ptx->m_pending_tx[n].construction_data;}, extra_message);
            setStatus(ptx->status(), ptx->errorString());
        }

        return ptx;
    } catch (exception& e) {
        LOG_ERROR("Error on restoring multisig transaction: " << e.what());
        setStatusError(string(tr("Failed to restore multisig transaction: ")) + e.what());
    }

    return nullptr;
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

PendingTransaction *WalletImpl::createTransactionMultDest(const std::vector<string> &dst_addr, const string &payment_id, optional<std::vector<uint64_t>> amount, uint32_t mixin_count, PendingTransaction::Priority priority, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices /* = {} */, std::set<uint32_t> subtract_fee_from_outputs /* = {} */, const std::string key_image /* = "" */, const size_t outputs /* = 1 */, const std::uint64_t below /* = 0 */)

{
    clearStatus();
    // Pause refresh thread while creating transaction
    pauseRefresh();
      
    cryptonote::address_parse_info info;

    const auto adjusted_priority = m_wallet->adjust_priority(static_cast<uint32_t>(priority));

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    if (!statusOk())
        return transaction;

    do {
        if (checkBackgroundSync("cannot create transactions"))
            break;

        std::vector<uint8_t> extra;
        std::string extra_nonce;
        vector<cryptonote::tx_destination_entry> dsts;
        if (!amount && dst_addr.size() > 1) {
            setStatusError(tr("Sending all requires one destination address"));
            break;
        }
        if (amount && (dst_addr.size() != (*amount).size())) {
            setStatusError(tr("Destinations and amounts are unequal"));
            break;
        }
        if (!payment_id.empty()) {
            crypto::hash payment_id_long;
            if (tools::wallet2::parse_long_payment_id(payment_id, payment_id_long)) {
                cryptonote::set_payment_id_to_tx_extra_nonce(extra_nonce, payment_id_long);
            } else {
                setStatusError(tr("payment id has invalid format, expected 64 character hex string: ") + payment_id);
                break;
            }
        }
        bool error = false;
        for (size_t i = 0; i < dst_addr.size() && !error; i++) {
            if(!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), dst_addr[i])) {
                // TODO: copy-paste 'if treating as an address fails, try as url' from simplewallet.cpp:1982
                setStatusError(tr("Invalid destination address"));
                error = true;
                break;
            }
            if (info.has_payment_id) {
                if (!extra_nonce.empty()) {
                    setStatusError(tr("a single transaction cannot use more than one payment id"));
                    error = true;
                    break;
                }
                set_encrypted_payment_id_to_tx_extra_nonce(extra_nonce, info.payment_id);
            }

            if (amount) {
                cryptonote::tx_destination_entry de;
                de.original = dst_addr[i];
                de.addr = info.address;
                de.amount = (*amount)[i];
                de.is_subaddress = info.is_subaddress;
                de.is_integrated = info.has_payment_id;
                dsts.push_back(de);
            } else {
                if (subaddr_indices.empty()) {
                    for (uint32_t index = 0; index < numSubaddresses(subaddr_account); ++index)
                        subaddr_indices.insert(index);
                }
            }
        }
        if (error) {
            break;
        }
        if (!extra_nonce.empty() && !add_extra_nonce_to_tx_extra(extra, extra_nonce)) {
            setStatusError(tr("failed to set up payment id, though it was decoded correctly"));
            break;
        }
        try {
            size_t fake_outs_count = mixin_count > 0 ? mixin_count : defaultMixin();
            fake_outs_count = m_wallet->adjust_mixin(fake_outs_count);

            if (amount) {
                transaction->m_pending_tx = m_wallet->create_transactions_2(dsts, fake_outs_count,
                                                                            adjusted_priority,
                                                                            extra, subaddr_account, subaddr_indices, subtract_fee_from_outputs);
            }
            else if (!key_image.empty()) { // sweep_single
                crypto::key_image ki;
                if (!epee::string_tools::hex_to_pod(key_image, ki))
                {
                    setStatusError(tr("Failed to parse key image"));
                    break;
                }
                transaction->m_pending_tx = m_wallet->create_transactions_single(ki,
                                                                                 info.address,
                                                                                 info.is_subaddress,
                                                                                 outputs,
                                                                                 fake_outs_count,
                                                                                 tools::fee_priority_utilities::from_integral(priority),
                                                                                 extra);
            } else { // sweep_account, sweep_all, sweep_below
                transaction->m_pending_tx = m_wallet->create_transactions_all(below, info.address, info.is_subaddress, outputs, fake_outs_count,
                                                                              adjusted_priority,
                                                                              extra, subaddr_account, subaddr_indices);
            }
            pendingTxPostProcess(transaction);

            if (multisig().isMultisig) {
                auto tx_set = m_wallet->make_multisig_tx_set(transaction->m_pending_tx);
                transaction->m_pending_tx = tx_set.m_ptx;
                transaction->m_signers = tx_set.m_signers;
            }
        } catch (const tools::error::daemon_busy&) {
            setStatusError(tr("daemon is busy. Please try again later."));
        } catch (const tools::error::no_connection_to_daemon&) {
            setStatusError(tr("no connection to daemon. Please make sure daemon is running."));
        } catch (const tools::error::wallet_rpc_error& e) {
            setStatusError(tr("RPC error: ") +  e.to_string());
        } catch (const tools::error::get_outs_error &e) {
            setStatusError((boost::format(tr("failed to get outputs to mix: %s")) % e.what()).str());
        } catch (const tools::error::not_enough_unlocked_money& e) {
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            setStatusError(writer.str());
        } catch (const tools::error::not_enough_money& e) {
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, overall balance only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            setStatusError(writer.str());
        } catch (const tools::error::tx_not_possible& e) {
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());
            setStatusError(writer.str());
        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer;
            writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
            for (const std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
            }
            writer << "\n" << tr("Please sweep unmixable outputs.");
            setStatusError(writer.str());
        } catch (const tools::error::tx_not_constructed&) {
            setStatusError(tr("transaction was not constructed"));
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer;
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            setStatusError(writer.str());
        } catch (const tools::error::tx_sum_overflow& e) {
            setStatusError(e.what());
        } catch (const tools::error::zero_amount&) {
            setStatusError(tr("destination amount is zero"));
        } catch (const tools::error::zero_destination&) {
            setStatusError(tr("transaction has no destination"));
        } catch (const tools::error::tx_too_big& e) {
            setStatusError(tr("failed to find a suitable way to split transactions"));
        } catch (const tools::error::transfer_error& e) {
            setStatusError(string(tr("unknown transfer error: ")) + e.what());
        } catch (const tools::error::wallet_internal_error& e) {
            setStatusError(string(tr("internal error: ")) + e.what());
        } catch (const std::exception& e) {
            setStatusError(string(tr("unexpected error: ")) + e.what());
        } catch (...) {
            setStatusError(tr("unknown error"));
        }
    } while (false);

    statusWithErrorString(transaction->m_status, transaction->m_errorString);
    // Resume refresh thread for unattended wallet (like GUI).
    // A non-unattended wallet (like CLI) could be waiting for confirmation prompt
    // and a refresh could mess up the output
    if (m_wallet->is_unattended())
        startRefresh();
    return transaction;
}

PendingTransaction *WalletImpl::createTransaction(const string &dst_addr, const string &payment_id, optional<uint64_t> amount, uint32_t mixin_count,
                                                  PendingTransaction::Priority priority, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices)

{
    return createTransactionMultDest(std::vector<string> {dst_addr}, payment_id, amount ? (std::vector<uint64_t> {*amount}) : (optional<std::vector<uint64_t>>()), mixin_count, priority, subaddr_account, subaddr_indices);
}

PendingTransaction *WalletImpl::createSweepUnmixableTransaction()

{
    clearStatus();
    cryptonote::tx_destination_entry de;

    PendingTransactionImpl * transaction = new PendingTransactionImpl(*this);

    do {
        if (checkBackgroundSync("cannot sweep"))
            break;

        try {
            transaction->m_pending_tx = m_wallet->create_unmixable_sweep_transactions();
            pendingTxPostProcess(transaction);

        } catch (const tools::error::daemon_busy&) {
            setStatusError(tr("daemon is busy. Please try again later."));
        } catch (const tools::error::no_connection_to_daemon&) {
            setStatusError(tr("no connection to daemon. Please make sure daemon is running."));
        } catch (const tools::error::wallet_rpc_error& e) {
            setStatusError(tr("RPC error: ") +  e.to_string());
        } catch (const tools::error::get_outs_error&) {
            setStatusError(tr("failed to get outputs to mix"));
        } catch (const tools::error::not_enough_unlocked_money& e) {
            setStatusError("");
            std::ostringstream writer;

            writer << boost::format(tr("not enough unlocked money to transfer, available only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            setStatusError(writer.str());
        } catch (const tools::error::not_enough_money& e) {
            setStatusError("");
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, overall balance only %s, sent amount %s")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount());
            setStatusError(writer.str());
        } catch (const tools::error::tx_not_possible& e) {
            setStatusError("");
            std::ostringstream writer;

            writer << boost::format(tr("not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)")) %
                      print_money(e.available()) %
                      print_money(e.tx_amount() + e.fee())  %
                      print_money(e.tx_amount()) %
                      print_money(e.fee());
            setStatusError(writer.str());
        } catch (const tools::error::not_enough_outs_to_mix& e) {
            std::ostringstream writer;
            writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
            for (const std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs()) {
                writer << "\n" << tr("output amount") << " = " << print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
            }
            setStatusError(writer.str());
        } catch (const tools::error::tx_not_constructed&) {
            setStatusError(tr("transaction was not constructed"));
        } catch (const tools::error::tx_rejected& e) {
            std::ostringstream writer;
            writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
            setStatusError(writer.str());
        } catch (const tools::error::tx_sum_overflow& e) {
            setStatusError(e.what());
        } catch (const tools::error::zero_destination&) {
            setStatusError(tr("one of destinations is zero"));
        } catch (const tools::error::tx_too_big& e) {
            setStatusError(tr("failed to find a suitable way to split transactions"));
        } catch (const tools::error::transfer_error& e) {
            setStatusError(string(tr("unknown transfer error: ")) + e.what());
        } catch (const tools::error::wallet_internal_error& e) {
            setStatusError(string(tr("internal error: ")) + e.what());
        } catch (const std::exception& e) {
            setStatusError(string(tr("unexpected error: ")) + e.what());
        } catch (...) {
            setStatusError(tr("unknown error"));
        }
    } while (false);

    statusWithErrorString(transaction->m_status, transaction->m_errorString);
    return transaction;
}

void WalletImpl::disposeTransaction(PendingTransaction *t)
{
    delete t;
}

uint64_t WalletImpl::estimateTransactionFee(const std::vector<std::pair<std::string, uint64_t>> &destinations,
                                            PendingTransaction::Priority priority) const
{
    const size_t pubkey_size = 33;
    const size_t encrypted_paymentid_size = 11;
    const size_t extra_size = pubkey_size + encrypted_paymentid_size;

    return m_wallet->estimate_fee(
        useForkRules(HF_VERSION_PER_BYTE_FEE, 0),
        useForkRules(4, 0),
        1,
        m_wallet->get_min_ring_size() - 1,
        destinations.size() + 1,
        extra_size,
        useForkRules(8, 0),
        useForkRules(HF_VERSION_CLSAG, 0),
        useForkRules(HF_VERSION_BULLETPROOF_PLUS, 0),
        useForkRules(HF_VERSION_VIEW_TAGS, 0),
        m_wallet->get_base_fee(static_cast<uint32_t>(priority)),
        m_wallet->get_fee_quantization_mask());
}

TransactionHistory *WalletImpl::history()
{
    return m_history.get();
}

AddressBook *WalletImpl::addressBook()
{
    return m_addressBook.get();
}

Subaddress *WalletImpl::subaddress()
{
    return m_subaddress.get();
}

SubaddressAccount *WalletImpl::subaddressAccount()
{
    return m_subaddressAccount.get();
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
    if (checkBackgroundSync("cannot set default mixin"))
        return;
    m_wallet->default_mixin(arg);
}

bool WalletImpl::setCacheAttribute(const std::string &key, const std::string &val)
{
    if (checkBackgroundSync("cannot set cache attribute"))
        return false;
    m_wallet->set_attribute(key, val);
    return true;
}

std::string WalletImpl::getCacheAttribute(const std::string &key) const
{
    std::string value;
    m_wallet->get_attribute(key, value);
    return value;
}

bool WalletImpl::setUserNote(const std::string &txid, const std::string &note)
{
    if (checkBackgroundSync("cannot set user note"))
        return false;
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
    {
      setStatusError(tr("failed to parse tx_id"));
      return false;
    }
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    m_wallet->set_tx_note(htxid, note);
    return true;
}

std::string WalletImpl::getUserNote(const std::string &txid) const
{
    if (checkBackgroundSync("cannot get user note"))
        return "";
    cryptonote::blobdata txid_data;
    if(!epee::string_tools::parse_hexstr_to_binbuff(txid, txid_data) || txid_data.size() != sizeof(crypto::hash))
    {
      setStatusError(tr("failed to parse tx_id"));
      return "";
    }
    const crypto::hash htxid = *reinterpret_cast<const crypto::hash*>(txid_data.data());

    return m_wallet->get_tx_note(htxid);
}

std::string WalletImpl::getTxKey(const std::string &txid_str) const
{
    if (checkBackgroundSync("cannot get tx key"))
        return "";

    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return "";
    }

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    try
    {
        clearStatus();
        if (m_wallet->get_tx_key(txid, tx_key, additional_tx_keys))
        {
            clearStatus();
            std::ostringstream oss;
            oss << epee::string_tools::pod_to_hex(unwrap(unwrap(tx_key)));
            for (size_t i = 0; i < additional_tx_keys.size(); ++i)
                oss << epee::string_tools::pod_to_hex(unwrap(unwrap(additional_tx_keys[i])));
            return oss.str();
        }
        else
        {
            setStatusError(tr("no tx keys found for this txid"));
            return "";
        }
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return "";
    }
}

bool WalletImpl::checkTxKey(const std::string &txid_str, std::string tx_key_str, const std::string &address_str, uint64_t &received, bool &in_pool, uint64_t &confirmations)
{
    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return false;
    }

    crypto::secret_key tx_key;
    std::vector<crypto::secret_key> additional_tx_keys;
    if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), tx_key))
    {
        setStatusError(tr("Failed to parse tx key"));
        return false;
    }
    tx_key_str = tx_key_str.substr(64);
    while (!tx_key_str.empty())
    {
        additional_tx_keys.resize(additional_tx_keys.size() + 1);
        if (!epee::string_tools::hex_to_pod(tx_key_str.substr(0, 64), additional_tx_keys.back()))
        {
            setStatusError(tr("Failed to parse tx key"));
            return false;
        }
        tx_key_str = tx_key_str.substr(64);
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        setStatusError(tr("Failed to parse address"));
        return false;
    }

    try
    {
        m_wallet->check_tx_key(txid, tx_key, additional_tx_keys, info.address, received, in_pool, confirmations);
        clearStatus();
        return true;
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return false;
    }
}

std::string WalletImpl::getTxProof(const std::string &txid_str, const std::string &address_str, const std::string &message) const
{
    if (checkBackgroundSync("cannot get tx proof"))
        return "";

    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return "";
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        setStatusError(tr("Failed to parse address"));
        return "";
    }

    try
    {
        clearStatus();
        return m_wallet->get_tx_proof(txid, info.address, info.is_subaddress, message);
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return "";
    }
}

bool WalletImpl::checkTxProof(const std::string &txid_str, const std::string &address_str, const std::string &message, const std::string &signature, bool &good, uint64_t &received, bool &in_pool, uint64_t &confirmations)
{
    crypto::hash txid;
    if (!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return false;
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address_str))
    {
        setStatusError(tr("Failed to parse address"));
        return false;
    }

    try
    {
        good = m_wallet->check_tx_proof(txid, info.address, info.is_subaddress, message, signature, received, in_pool, confirmations);
        clearStatus();
        return true;
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return false;
    }
}

std::string WalletImpl::getSpendProof(const std::string &txid_str, const std::string &message) const {
    if (checkBackgroundSync("cannot get spend proof"))
        return "";

    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return "";
    }

    try
    {
        clearStatus();
        return m_wallet->get_spend_proof(txid, message);
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return "";
    }
}

bool WalletImpl::checkSpendProof(const std::string &txid_str, const std::string &message, const std::string &signature, bool &good) const {
    good = false;
    crypto::hash txid;
    if(!epee::string_tools::hex_to_pod(txid_str, txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return false;
    }

    try
    {
        clearStatus();
        good = m_wallet->check_spend_proof(txid, message, signature);
        return true;
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return false;
    }
}

std::string WalletImpl::getReserveProof(bool all, uint32_t account_index, uint64_t amount, const std::string &message) const {
    if (checkBackgroundSync("cannot get reserve proof"))
        return "";

    try
    {
        clearStatus();
        boost::optional<std::pair<uint32_t, uint64_t>> account_minreserve;
        if (!all)
        {
            account_minreserve = std::make_pair(account_index, amount);
        }
        return m_wallet->get_reserve_proof(account_minreserve, message);
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return "";
    }
}

bool WalletImpl::checkReserveProof(const std::string &address, const std::string &message, const std::string &signature, bool &good, uint64_t &total, uint64_t &spent) const {
    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address))
    {
        setStatusError(tr("Failed to parse address"));
        return false;
    }
    if (info.is_subaddress)
    {
        setStatusError(tr("Address must not be a subaddress"));
        return false;
    }

    good = false;
    try
    {
        clearStatus();
        good = m_wallet->check_reserve_proof(info.address, message, signature, total, spent);
        return true;
    }
    catch (const std::exception &e)
    {
        setStatusError(e.what());
        return false;
    }
}

std::string WalletImpl::signMessage(const std::string &message, const std::string &address, bool sign_with_view_key)
{
    if (checkBackgroundSync("cannot sign message"))
        return "";

    tools::wallet2::message_signature_type_t sig_type = sign_with_view_key ? tools::wallet2::sign_with_view_key : tools::wallet2::sign_with_spend_key;

    if (address.empty()) {
        return m_wallet->sign(message, sig_type);
    }

    cryptonote::address_parse_info info;
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address)) {
        setStatusError(tr("Failed to parse address"));
        return "";
    }
    auto index = m_wallet->get_subaddress_index(info.address);
    if (!index) {
        setStatusError(tr("Address doesn't belong to the wallet"));
        return "";
    }

    return m_wallet->sign(message, sig_type, *index);
}

bool WalletImpl::verifySignedMessage(const std::string &message,
                                     const std::string &address,
                                     const std::string &signature,
                                     bool *is_old_out /* = nullptr */,
                                     std::string *signature_type_out /* = nullptr */) const
{
  cryptonote::address_parse_info info;

  if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address))
    return false;
  tools::wallet2::message_signature_result_t result = m_wallet->verify(message, info.address, signature);
  if (is_old_out)
      *is_old_out = result.old;
  if (signature_type_out)
      *signature_type_out = (result.type == tools::wallet2::sign_with_spend_key
                        ? "spend key" : result.type == tools::wallet2::sign_with_view_key
                        ? "view key" : "unknown key combination (suspicious)");
  return result.valid;
}

std::string WalletImpl::signMultisigParticipant(const std::string &message) const
{
    clearStatus();

    const multisig::multisig_account_status ms_status{m_wallet->get_multisig_status()};
    if (!ms_status.multisig_is_active || !ms_status.is_ready) {
        m_status = Status_Error;
        m_errorString = tr("The wallet must be in multisig ready state");
        return {};
    }

    try {
        return m_wallet->sign_multisig_participant(message);
    } catch (const std::exception& e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }

    return {};
}

bool WalletImpl::verifyMessageWithPublicKey(const std::string &message, const std::string &publicKey, const std::string &signature) const
{
    clearStatus();

    cryptonote::blobdata pkeyData;
    if(!epee::string_tools::parse_hexstr_to_binbuff(publicKey, pkeyData) || pkeyData.size() != sizeof(crypto::public_key))
    {
        m_status = Status_Error;
        m_errorString = tr("Given string is not a key");
        return false;
    }

    try {
        crypto::public_key pkey = *reinterpret_cast<const crypto::public_key*>(pkeyData.data());
        return m_wallet->verify_with_public_key(message, pkey, signature);
    } catch (const std::exception& e) {
        m_status = Status_Error;
        m_errorString = e.what();
    }

    return false;
}

bool WalletImpl::connectToDaemon(uint32_t *version /* = NULL*/,
                                 bool *ssl /* = NULL */,
                                 uint32_t timeout /* = 20000 */, // TODO : can we put DEFAULT_CONNECTION_TIMEOUT_MILLIS into wallet2_api.h and have it as default param?
                                 bool *wallet_is_outdated /* = NULL */,
                                 bool *daemon_is_outdated /* = NULL */)
{
    bool result = m_wallet->check_connection(version, ssl, timeout, wallet_is_outdated, daemon_is_outdated);
    if (!result) {
        setStatusError("Error connecting to daemon at " + m_wallet->get_daemon_address());
    } else {
        clearStatus();
        // start refreshing here
    }
    return result;
}

Wallet::ConnectionStatus WalletImpl::connected() const
{
    uint32_t version = 0;
    bool wallet_is_outdated = false, daemon_is_outdated = false;
    m_is_connected = m_wallet->check_connection(&version, NULL, DEFAULT_CONNECTION_TIMEOUT_MILLIS, &wallet_is_outdated, &daemon_is_outdated);
    if (!m_is_connected)
    {
        if (wallet_is_outdated || daemon_is_outdated)
            return Wallet::ConnectionStatus_WrongVersion;
        else
            return Wallet::ConnectionStatus_Disconnected;
    }
    if ((version >> 16) != CORE_RPC_VERSION_MAJOR)
        return Wallet::ConnectionStatus_WrongVersion;
    return Wallet::ConnectionStatus_Connected;
}

void WalletImpl::setTrustedDaemon(bool arg)
{
    m_wallet->set_trusted_daemon(arg);
}

bool WalletImpl::trustedDaemon() const
{
    return m_wallet->is_trusted_daemon();
}

bool WalletImpl::setProxy(const std::string &address)
{
    return m_wallet->set_proxy(address);
}

bool WalletImpl::watchOnly() const
{
    return m_wallet->watch_only();
}

bool WalletImpl::isDeterministic() const
{
    return m_wallet->is_deterministic();
}

bool WalletImpl::isBackgroundSyncing() const
{
    return m_wallet->is_background_syncing();
}

bool WalletImpl::isBackgroundWallet() const
{
    return m_wallet->is_background_wallet();
}

void WalletImpl::clearStatus() const
{
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    m_status = Status_Ok;
    m_errorString.clear();
}

void WalletImpl::setStatusError(const std::string& message) const
{
    setStatus(Status_Error, message);
}

void WalletImpl::setStatusCritical(const std::string& message) const
{
    setStatus(Status_Critical, message);
}

void WalletImpl::setStatus(int status, const std::string& message) const
{
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    m_status = status;
    m_errorString = message;
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
        LOG_PRINT_L3(__FUNCTION__ << ": m_status: " << status());
        LOG_PRINT_L3(__FUNCTION__ << ": m_refreshShouldRescan: " << m_refreshShouldRescan);
        if (m_refreshEnabled) {
            LOG_PRINT_L3(__FUNCTION__ << ": refreshing...");
            doRefresh();
        }
    }
    LOG_PRINT_L3(__FUNCTION__ << ": refresh thread stopped");
}

void WalletImpl::doRefresh(std::uint64_t start_height /* = 0 */,
                           bool check_pool /* = true */,
                           bool try_incremental /* = false */,
                           std::uint64_t max_blocks /* = std::numeric_limits<uint64_t>::max() */,
                           bool *error_out /* = nullptr */,
                           std::uint64_t *blocks_fetched_out /* = nullptr */,
                           bool *received_money_out /* = nullptr */)
{
    if (error_out)
        *error_out = false;
    bool rescan = m_refreshShouldRescan.exchange(false);
    // synchronizing async and sync refresh calls
    boost::lock_guard<boost::mutex> guarg(m_refreshMutex2);
    do try {
        LOG_PRINT_L3(__FUNCTION__ << ": doRefresh, rescan = "<<rescan);
        // Syncing daemon and refreshing wallet simultaneously is very resource intensive.
        // Disable refresh if wallet is disconnected or daemon isn't synced.
        if (daemonSynced()) {
            if(rescan)
                m_wallet->rescan_blockchain(m_do_hard_rescan, /* refresh */ false, m_do_keep_key_images_on_rescan);
            std::uint64_t blocks_fetched_ignored;
            bool received_money_ignored;
            m_wallet->refresh(trustedDaemon(),
                              start_height,
                              (blocks_fetched_out ? *blocks_fetched_out : blocks_fetched_ignored),
                              (received_money_out ? *received_money_out : received_money_ignored),
                              check_pool,
                              try_incremental,
                              max_blocks);
            m_synchronized = m_wallet->is_synced();
            // assuming if we have empty history, it wasn't initialized yet
            // for further history changes client need to update history in
            // "on_money_received" and "on_money_sent" callbacks
            if (m_history->count() == 0) {
                m_history->refresh();
            }
        } else {
           LOG_PRINT_L3(__FUNCTION__ << ": skipping refresh - daemon is not synced");
        }
    } catch (const tools::error::daemon_busy&) {
        setStatusError(tr("daemon is busy. Please try again later."));
        break;
    } catch (const tools::error::no_connection_to_daemon&) {
        setStatusError(tr("no connection to daemon. Please make sure daemon is running."));
        break;
    } catch (const tools::error::deprecated_rpc_access&) {
        setStatusError(tr("Daemon requires deprecated RPC payment. See https://github.com/monero-project/monero/issues/8722"));
        break;
    } catch (const tools::error::wallet_rpc_error& e) {
        LOG_ERROR("RPC error: " << e.to_string());
        setStatusError(std::string(tr("RPC error: ")) + e.what());
        break;
    } catch (const tools::error::refresh_error& e) {
        LOG_ERROR("refresh error: " << e.to_string());
        setStatusError(std::string(tr("refresh error: ")) + e.what());
        break;
    } catch (const tools::error::wallet_internal_error& e) {
        LOG_ERROR("internal error: " << e.to_string());
        setStatusError(std::string(tr("internal error: ")) + e.what());
        break;
    } catch (const std::exception& e) {
        LOG_ERROR("unexpected error: " << e.what());
        setStatusError(std::string(tr("unexpected error: ")) + e.what());
        break;
    } catch (...) {
        LOG_ERROR("unknown error");
        setStatusError(tr("unknown error"));
        break;
    }while(!rescan && (rescan=m_refreshShouldRescan.exchange(false))); // repeat if not rescanned and rescan was requested

   if (error_out)
       *error_out = !statusOk();

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

void WalletImpl::pendingTxPostProcess(PendingTransactionImpl * pending)
{
  // If the device being used is HW device with cold signing protocol, cold sign then.
  if (!m_wallet->get_account().get_device().has_tx_cold_sign()){
    return;
  }

  tools::wallet2::signed_tx_set exported_txs;
  std::vector<cryptonote::address_parse_info> dsts_info;

  m_wallet->cold_sign_tx(pending->m_pending_tx, exported_txs, dsts_info, pending->m_tx_device_aux);
  pending->m_key_images = exported_txs.key_images;
  pending->m_pending_tx = exported_txs.ptx;

  std::string extra_message;
  if (!exported_txs.key_images.empty())
    extra_message = (boost::format("%u key images to import. ") % (unsigned)exported_txs.key_images.size()).str();
  pending->checkLoadedTx([&pending](){return pending->txCount();}, [&pending](size_t n)->const tools::wallet2::tx_construction_data&{return pending->m_pending_tx[n].construction_data;}, extra_message);
}

bool WalletImpl::doInit(const string &daemon_address, const std::string &proxy_address, uint64_t upper_transaction_size_limit, bool ssl)
{
    if (!m_wallet->init(daemon_address, m_daemon_login, proxy_address, upper_transaction_size_limit))
       return false;

    // in case new wallet, this will force fast-refresh (pulling hashes instead of blocks)
    // If daemon isn't synced a calculated block height will be used instead
    if (isNewWallet() && daemonSynced()) {
        LOG_PRINT_L2(__FUNCTION__ << ":New Wallet - fast refresh until " << daemonBlockChainHeight());
        setRefreshFromBlockHeight(daemonBlockChainHeight());
    }

    if (m_rebuildWalletCache)
      LOG_PRINT_L2(__FUNCTION__ << ": Rebuilding wallet cache, fast refresh until block " << getRefreshFromBlockHeight());

    if (Utils::isAddressLocal(daemon_address)) {
        this->setTrustedDaemon(true);
        m_refreshIntervalMillis = DEFAULT_REFRESH_INTERVAL_MILLIS;
    } else {
        this->setTrustedDaemon(false);
        m_refreshIntervalMillis = DEFAULT_REMOTE_NODE_REFRESH_INTERVAL_MILLIS;
    }
    return true;
}

bool WalletImpl::checkBackgroundSync(const std::string &message) const
{
    clearStatus();
    if (m_wallet->is_background_wallet())
    {
        LOG_ERROR("Background wallets " + message);
        setStatusError(tr("Background wallets ") + message);
        return true;
    }
    if (m_wallet->is_background_syncing())
    {
        LOG_ERROR(message + " while background syncing");
        setStatusError(message + tr(" while background syncing. Stop background syncing first."));
        return true;
    }
    return false;
}

bool WalletImpl::parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error)
{
    return m_wallet->parse_uri(uri, address, payment_id, amount, tx_description, recipient_name, unknown_parameters, error);
}

std::string WalletImpl::make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error) const
{
    return m_wallet->make_uri(address, payment_id, amount, tx_description, recipient_name, error);
}

std::string WalletImpl::getDefaultDataDir() const
{
 return tools::get_default_data_dir();
}

bool WalletImpl::rescanSpent()
{
    clearStatus();
    if (checkBackgroundSync("cannot rescan spent"))
        return false;
    if (!trustedDaemon()) {
        setStatusError(tr("Rescan spent can only be used with a trusted daemon"));
        return false;
    }
    try {
        m_wallet->rescan_spent();
    }
    catch (const tools::error::daemon_busy&)
    {
        setStatusError(tr("daemon is busy. Please try again later."));
        return false;
    }
    catch (const tools::error::no_connection_to_daemon&)
    {
        setStatusError(tr("no connection to daemon. Please make sure daemon is running."));
        return false;
    }
    catch (const tools::error::deprecated_rpc_access&)
    {
        setStatusError(tr("Daemon requires deprecated RPC payment. See https://github.com/monero-project/monero/issues/8722"));
        return false;
    }
    catch (const tools::error::is_key_image_spent_error&)
    {
        setStatusError(tr("failed to get spent status"));
        return false;
    }
    catch (const tools::error::wallet_rpc_error& e)
    {
        LOG_ERROR("RPC error: " << e.to_string());
        setStatusError(std::string(tr("RPC error: ")) + e.to_string());
        return false;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(__FUNCTION__ << "unexpected error: " << e.what());
        setStatusError(std::string(tr("unexpected error: ")) + e.what());
        return false;
    }
    catch (...)
    {
        LOG_ERROR(__FUNCTION__ << "unknown error");
        setStatusError(tr("unknonw error: "));
        return false;
    }

    return true;
}

NetworkType WalletImpl::nettype() const
{
    return static_cast<NetworkType>(m_wallet->nettype());
}

void WalletImpl::setOffline(bool offline)
{
    m_wallet->set_offline(offline);
}

bool WalletImpl::isOffline() const
{
    return m_wallet->is_offline();
}

void WalletImpl::hardForkInfo(uint8_t &version, uint64_t &earliest_height) const
{
    m_wallet->get_hard_fork_info(version, earliest_height);
}

bool WalletImpl::useForkRules(uint8_t version, int64_t early_blocks) const 
{
    clearStatus();

    try
    {
        return m_wallet->use_fork_rules(version, early_blocks);
    }
    catch (const std::exception &e)
    {
        setStatusError((boost::format(tr("Failed to check if fork rules for version `%u` with `%d` early blocks should be used: %s")) % version % early_blocks % e.what()).str());
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------------

bool WalletImpl::blackballOutputs(const std::vector<std::string> &outputs, bool add)
{
    std::vector<std::pair<uint64_t, uint64_t>> raw_outputs;
    raw_outputs.reserve(outputs.size());
    uint64_t amount = std::numeric_limits<uint64_t>::max(), offset, num_offsets;
    for (const std::string &str: outputs)
    {
        if (sscanf(str.c_str(), "@%" PRIu64, &amount) == 1)
          continue;
        if (amount == std::numeric_limits<uint64_t>::max())
        {
          setStatusError("First line is not an amount");
          return true;
        }
        if (sscanf(str.c_str(), "%" PRIu64 "*%" PRIu64, &offset, &num_offsets) == 2 && num_offsets <= std::numeric_limits<uint64_t>::max() - offset)
        {
          while (num_offsets--)
            raw_outputs.push_back(std::make_pair(amount, offset++));
        }
        else if (sscanf(str.c_str(), "%" PRIu64, &offset) == 1)
        {
          raw_outputs.push_back(std::make_pair(amount, offset));
        }
        else
        {
          setStatusError(tr("Invalid output: ") + str);
          return false;
        }
    }
    bool ret = m_wallet->set_blackballed_outputs(raw_outputs, add);
    if (!ret)
    {
        setStatusError(tr("Failed to mark outputs as spent"));
        return false;
    }
    return true;
}

bool WalletImpl::blackballOutput(const std::string &amount, const std::string &offset)
{
    uint64_t raw_amount, raw_offset;
    if (!epee::string_tools::get_xtype_from_string(raw_amount, amount))
    {
        setStatusError(tr("Failed to parse output amount"));
        return false;
    }
    if (!epee::string_tools::get_xtype_from_string(raw_offset, offset))
    {
        setStatusError(tr("Failed to parse output offset"));
        return false;
    }
    bool ret = m_wallet->blackball_output(std::make_pair(raw_amount, raw_offset));
    if (!ret)
    {
        setStatusError(tr("Failed to mark output as spent"));
        return false;
    }
    return true;
}

bool WalletImpl::unblackballOutput(const std::string &amount, const std::string &offset)
{
    uint64_t raw_amount, raw_offset;
    if (!epee::string_tools::get_xtype_from_string(raw_amount, amount))
    {
        setStatusError(tr("Failed to parse output amount"));
        return false;
    }
    if (!epee::string_tools::get_xtype_from_string(raw_offset, offset))
    {
        setStatusError(tr("Failed to parse output offset"));
        return false;
    }
    bool ret = m_wallet->unblackball_output(std::make_pair(raw_amount, raw_offset));
    if (!ret)
    {
        setStatusError(tr("Failed to mark output as unspent"));
        return false;
    }
    return true;
}

bool WalletImpl::getRing(const std::string &key_image, std::vector<uint64_t> &ring) const
{
    crypto::key_image raw_key_image;
    if (!epee::string_tools::hex_to_pod(key_image, raw_key_image))
    {
        setStatusError(tr("Failed to parse key image"));
        return false;
    }
    bool ret = m_wallet->get_ring(raw_key_image, ring);
    if (!ret)
    {
        setStatusError(tr("Failed to get ring"));
        return false;
    }
    return true;
}

bool WalletImpl::getRings(const std::string &txid, std::vector<std::pair<std::string, std::vector<uint64_t>>> &rings) const
{
    crypto::hash raw_txid;
    if (!epee::string_tools::hex_to_pod(txid, raw_txid))
    {
        setStatusError(tr("Failed to parse txid"));
        return false;
    }
    std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> raw_rings;
    bool ret = m_wallet->get_rings(raw_txid, raw_rings);
    if (!ret)
    {
        setStatusError(tr("Failed to get rings"));
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
        setStatusError(tr("Failed to parse key image"));
        return false;
    }
    bool ret = m_wallet->set_ring(raw_key_image, ring, relative);
    if (!ret)
    {
        setStatusError(tr("Failed to set ring"));
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

bool WalletImpl::lockKeysFile()
{
    return m_wallet->lock_keys_file();
}

bool WalletImpl::unlockKeysFile()
{
    return m_wallet->unlock_keys_file();
}

bool WalletImpl::isKeysFileLocked()
{
    return m_wallet->is_keys_file_locked();
}

std::unique_ptr<WalletKeysDecryptGuard> WalletImpl::createKeysDecryptGuard(const std::string_view &password)
{
    return std::unique_ptr<WalletKeysDecryptGuard>(new WalletKeysDecryptGuardImpl(*m_wallet, epee::wipeable_string(password.data(), password.size())));
}

uint64_t WalletImpl::coldKeyImageSync(uint64_t &spent, uint64_t &unspent)
{
    return m_wallet->cold_key_image_sync(spent, unspent);
}

void WalletImpl::deviceShowAddress(uint32_t accountIndex, uint32_t addressIndex, const std::string &paymentId)
{
    clearStatus();
    boost::optional<crypto::hash8> payment_id_param = boost::none;
    if (!paymentId.empty())
    {
        crypto::hash8 payment_id;
        bool res = tools::wallet2::parse_short_payment_id(paymentId, payment_id);
        if (!res)
        {
            setStatusError(tr("Invalid payment ID"));
            return;
        }
        payment_id_param = payment_id;
    }

    m_wallet->device_show_address(accountIndex, addressIndex, payment_id_param);
}

bool WalletImpl::reconnectDevice()
{
    clearStatus();

    bool r;
    try {
        r = m_wallet->reconnect_device();
    }
    catch (const std::exception &e) {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError(e.what());
        return false;
    }

    return r;
}

uint64_t WalletImpl::getBytesReceived()
{
    return m_wallet->get_bytes_received();
}

uint64_t WalletImpl::getBytesSent()
{
    return m_wallet->get_bytes_sent();
}

//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::getMultisigSeed(const std::string &seed_offset) const
{
    clearStatus();

    if (checkBackgroundSync("cannot get multisig seed"))
        return std::string();

    try
    {
        checkMultisigWalletReady(m_wallet);

        epee::wipeable_string seed;
        if (m_wallet->get_multisig_seed(seed, seed_offset))
            return std::string(seed.data(), seed.size());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError(string(tr("Failed to get multisig seed: ")) + e.what());
    }
    return "";
}
//-------------------------------------------------------------------------------------------------------------------
std::pair<std::uint32_t, std::uint32_t> WalletImpl::getSubaddressIndex(const std::string &address) const
{
    clearStatus();

    cryptonote::address_parse_info info;
    std::pair<std::uint32_t, std::uint32_t> indices{0, 0};
    if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), address))
    {
        setStatusError(string(tr("Failed to parse address: ") + address));
        return indices;
    }

    auto index = m_wallet->get_subaddress_index(info.address);
    if (!index)
        setStatusError(string(tr("Address doesn't belong to the wallet: ") + address));
    else
        indices = std::make_pair((*index).major, (*index).minor);

    return indices;
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::freeze(const std::string &key_image)
{
    clearStatus();

    crypto::key_image ki_pod;
    if (!epee::string_tools::hex_to_pod(key_image, ki_pod))
    {
        setStatusError((boost::format(tr("Failed to parse key image `%s`")) % key_image).str());
        return;
    }

    try
    {
        m_wallet->freeze(ki_pod);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to freeze enote with key image `%s`: %s")) % key_image % e.what()).str());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::freezeByPubKey(const std::string &enote_pub_key)
{
    clearStatus();

    crypto::public_key pk;
    if (!epee::string_tools::hex_to_pod(enote_pub_key, pk))
    {
        setStatusError((boost::format(tr("Failed to parse public key `%s`")) % enote_pub_key).str());
        return;
    }

    try
    {
        m_wallet->freeze(m_wallet->get_output_index(pk));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to freeze enote with public key `%s`: %s")) % enote_pub_key % e.what()).str());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::thaw(const std::string &key_image)
{
    clearStatus();

    crypto::key_image ki_pod;
    if (!epee::string_tools::hex_to_pod(key_image, ki_pod))
    {
        setStatusError((boost::format(tr("Failed to parse key image `%s`")) % key_image).str());
        return;
    }

    try
    {
        m_wallet->thaw(ki_pod);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to thaw enote with key image `%s`: %s")) % key_image % e.what()).str());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::thawByPubKey(const std::string &enote_pub_key)
{
    clearStatus();

    crypto::public_key pk;
    if (!epee::string_tools::hex_to_pod(enote_pub_key, pk))
    {
        setStatusError((boost::format(tr("Failed to parse public key `%s`")) % enote_pub_key).str());
        return;
    }

    try
    {
        m_wallet->thaw(m_wallet->get_output_index(pk));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to thaw enote with public key `%s`: %s")) % enote_pub_key % e.what()).str());
    }
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::isFrozen(const std::string &key_image) const
{
    clearStatus();

    crypto::key_image ki_pod;
    if (!epee::string_tools::hex_to_pod(key_image, ki_pod))
    {
        setStatusError((boost::format(tr("Failed to parse key image `%s`")) % key_image).str());
        return false;
    }

    try
    {
        return m_wallet->frozen(ki_pod);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to determine if enote with key image `%s` is frozen: %s")) % key_image % e.what()).str());
    }

    return false;
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::createOneOffSubaddress(std::uint32_t account_index, std::uint32_t address_index)
{
    m_wallet->create_one_off_subaddress({account_index, address_index});
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::WalletState WalletImpl::getWalletState() const
{
    WalletState wallet_state{};

    wallet_state.is_deprecated  = m_wallet->is_deprecated();
    wallet_state.is_unattended  = m_wallet->is_unattended();
    wallet_state.daemon_address = m_wallet->get_daemon_address();
    wallet_state.ring_database  = m_wallet->get_ring_database();
    wallet_state.n_enotes       = m_wallet->get_num_transfer_details();

    return wallet_state;
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::DeviceState WalletImpl::getDeviceState() const
{
    DeviceState device_state{};
    auto &device = m_wallet->get_account().get_device();

    device_state.key_on_device       = m_wallet->key_on_device();
    device_state.device_name         = device.get_name();
    device_state.has_tx_cold_sign    = device.has_tx_cold_sign();
    device_state.has_ki_live_refresh = device.has_ki_live_refresh();
    device_state.has_ki_cold_sync    = device.has_ki_cold_sync();
    device_state.last_ki_sync        = m_wallet->get_device_last_key_image_sync();
    device_state.protocol            = static_cast<DeviceState::DeviceProtocol>(device.device_protocol());
    device_state.device_type         = static_cast<DeviceState::DeviceType>(device.get_type());

    return device_state;
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::rewriteWalletFile(const std::string &wallet_name, const std::string_view &password)
{
    clearStatus();

    try
    {
        m_wallet->rewrite(wallet_name, epee::wipeable_string(password.data(), password.size()));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError((boost::format(tr("Failed to rewrite wallet file with wallet name `%s`: %s")) % wallet_name % e.what()).str());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::writeWatchOnlyWallet(const std::string_view &password, std::string &new_keys_file_name)
{
    clearStatus();

    try
    {
        m_wallet->write_watch_only_wallet(m_wallet->get_wallet_file(), epee::wipeable_string(password.data(), password.size()), new_keys_file_name);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError(string(tr("Failed to write watch only wallet: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::refreshPoolOnly(bool refreshed /*false*/, bool try_incremental /*false*/)
{
    clearStatus();

    // Update pool state
    std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> process_txs_pod;
    try
    {
        m_wallet->update_pool_state(process_txs_pod, refreshed, try_incremental);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError(string(tr("Failed to update pool state: ")) + e.what());
        return;
    }

    if (process_txs_pod.empty())
        return;

    // Process pool state
    try
    {
        m_wallet->process_pool_state(process_txs_pod);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(__FUNCTION__ << " error: " << e.what());
        setStatusError(string(tr("Failed to process pool state: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::unique_ptr<EnoteDetails>> WalletImpl::getEnoteDetails() const
{
    std::vector<std::unique_ptr<EnoteDetails>> enotes_details{};
    std::size_t n_transfers = m_wallet->get_num_transfer_details();
    enotes_details.reserve(n_transfers);

    for (std::size_t i = 0; i < n_transfers; ++i)
    {
        enotes_details.push_back(getEnoteDetails(i));
    }
    return enotes_details;
}
//-------------------------------------------------------------------------------------------------------------------
std::unique_ptr<EnoteDetails> WalletImpl::getEnoteDetails(const std::string &enote_pub_key) const
{
    clearStatus();

    crypto::public_key pk{};
    if (!epee::string_tools::hex_to_pod(enote_pub_key, pk))
    {
        setStatusError((boost::format(tr("Failed to parse public key `%s`")) % enote_pub_key).str());
        return nullptr;
    }

    std::size_t enote_index{};
    try { enote_index = m_wallet->get_output_index(pk); }
    catch (const exception &e)
    {
        setStatusError(std::string(tr("Could not find index of enote: ")) + e.what());
        return nullptr;
    }
    return getEnoteDetails(enote_index);
}
//-------------------------------------------------------------------------------------------------------------------
std::unique_ptr<EnoteDetails> WalletImpl::getEnoteDetails(const std::size_t enote_index) const
{
    clearStatus();

    std::unique_ptr<EnoteDetails> enote_details = nullptr;
    tools::wallet2::transfer_details td{};
    try { td = m_wallet->get_transfer_details(enote_index); }
    catch (const exception &e)
    {
        setStatusError(std::string(tr("Could not find enote details for index: ")) + std::to_string(enote_index));
        return nullptr;
    }

    std::unique_ptr<EnoteDetailsImpl> ed(new EnoteDetailsImpl);
    crypto::public_key enote_pub_key{};
    cryptonote::get_output_public_key(td.m_tx.vout[td.m_internal_output_index], enote_pub_key);
    auto view_tag = cryptonote::get_output_view_tag(td.m_tx.vout[td.m_internal_output_index]);

    ed->m_onetime_address = epee::string_tools::pod_to_hex(enote_pub_key);
    ed->m_view_tag = view_tag ? epee::string_tools::pod_to_hex(*view_tag) : "";
    ed->m_payment_id = getPaymentIdFromExtra(td.m_tx.extra);
    ed->m_block_height = td.m_block_height;
    ed->m_unlock_time = td.m_tx.unlock_time;
    ed->m_is_unlocked = m_wallet->is_transfer_unlocked(td);
    ed->m_tx_id = epee::string_tools::pod_to_hex(td.m_txid);
    ed->m_internal_enote_index = td.m_internal_output_index;
    ed->m_global_enote_index = td.m_global_output_index;
    ed->m_spent = td.m_spent;
    ed->m_frozen = td.m_frozen;
    ed->m_spent_height = td.m_spent_height;
    ed->m_key_image = epee::string_tools::pod_to_hex(td.m_key_image);
    ed->m_mask = epee::string_tools::pod_to_hex(td.m_mask);
    ed->m_amount = td.m_amount;
    ed->m_protocol_version = td.m_rct ? EnoteDetails::TxProtocol::TxProtocol_RingCT : EnoteDetails::TxProtocol::TxProtocol_CryptoNote;
    ed->m_key_image_known = td.m_key_image_known;
    ed->m_key_image_request = td.m_key_image_request;
    ed->m_pk_index = td.m_pk_index;
    ed->m_uses.reserve(td.m_uses.size());
    ed->m_subaddress_index_major = td.m_subaddr_index.major;
    ed->m_subaddress_index_minor = td.m_subaddr_index.minor;
    for (auto &u : td.m_uses)
        ed->m_uses.push_back(std::make_pair(u.first, epee::string_tools::pod_to_hex(u.second)));
    ed->m_key_image_partial = td.m_key_image_partial;

    enote_details = std::move(ed);
    return enote_details;
}
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::convertMultisigTxToStr(const PendingTransaction &multisig_ptx) const
{
    clearStatus();

    if (checkBackgroundSync("cannot get multisig seed"))
        return std::string();

    try
    {
        checkMultisigWalletReady(m_wallet);

        const PendingTransactionImpl *ptx_impl = dynamic_cast<const PendingTransactionImpl*>(&multisig_ptx);

        tools::wallet2::multisig_tx_set multisig_tx_set;
        multisig_tx_set.m_ptx = ptx_impl->m_pending_tx;
        multisig_tx_set.m_signers = ptx_impl->m_signers;

        return m_wallet->save_multisig_tx(multisig_tx_set);
    }
    catch (const exception &e)
    {
        setStatusError(string(tr("Failed to convert pending multisig tx to string: ")) + e.what());
    }

    return "";
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::saveMultisigTx(const PendingTransaction &multisig_ptx, const std::string &filename) const
{
    clearStatus();

    if (checkBackgroundSync("cannot get multisig seed"))
        return false;

    try
    {
        checkMultisigWalletReady(m_wallet);

        const PendingTransactionImpl *ptx_impl = dynamic_cast<const PendingTransactionImpl*>(&multisig_ptx);

        tools::wallet2::multisig_tx_set multisig_tx_set;
        multisig_tx_set.m_ptx = ptx_impl->m_pending_tx;
        multisig_tx_set.m_signers = ptx_impl->m_signers;

        return m_wallet->save_multisig_tx(multisig_tx_set, filename);
    }
    catch (const exception &e)
    {
        setStatusError((boost::format(tr("Failed to save multisig tx to file with filename `%s`: %s")) % filename % e.what()).str());
    }

    return false;
}
//-------------------------------------------------------------------------------------------------------------------
PendingTransaction* WalletImpl::parseTxFromStr(const std::string &signed_tx_str)
{
    clearStatus();
    std::unique_ptr<PendingTransactionImpl> ptx(new PendingTransactionImpl(*this));
    std::shared_ptr<tools::wallet2::signed_tx_set> signed_tx_set_out = std::make_shared<tools::wallet2::signed_tx_set>();
    bool r = m_wallet->parse_tx_from_str(signed_tx_str,
                                         ptx->m_pending_tx,
                                         /* accept_func = */ NULL,
                                         signed_tx_set_out.get(),
                                         /* do_handle_key_images = */ false);
    if (!r)
    {
        setStatusError(tr("Failed to parse signed tx from str"));
        ptx->m_status = PendingTransaction::Status::Status_Error;
        ptx->m_errorString = errorString();
        return ptx.release();
    }

    ptx->m_tx_key_images = signed_tx_set_out->tx_key_images;

    // Check tx data and construct confirmation message
    std::string extra_message;
    ptx->checkLoadedTx([&ptx](){return ptx->txCount();}, [&ptx](size_t n)->const tools::wallet2::tx_construction_data & {return ptx->m_pending_tx[n].construction_data;}, extra_message);
    setStatus(ptx->status(), ptx->errorString());

    return ptx.release();
}
//-------------------------------------------------------------------------------------------------------------------
PendingTransaction* WalletImpl::parseMultisigTxFromStr(const std::string &multisig_tx_str)
{
    clearStatus();

    std::unique_ptr<PendingTransactionImpl> ptx(new PendingTransactionImpl(*this));
    try
    {
        checkMultisigWalletReady(m_wallet);

        tools::wallet2::multisig_tx_set multisig_tx;

        if (!m_wallet->parse_multisig_tx_from_str(multisig_tx_str, multisig_tx))
            throw runtime_error(tr("Failed to parse multisig transaction from string."));

        ptx->m_pending_tx = multisig_tx.m_ptx;
        ptx->m_signers = multisig_tx.m_signers;
    }
    catch (const exception &e)
    {
        setStatusError(e.what());
        return nullptr;
    }

    return ptx.release();
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getFeeMultiplier(std::uint32_t priority, int fee_algorithm) const
{
    clearStatus();

    try
    {
        return m_wallet->get_fee_multiplier(tools::fee_priority_utilities::from_integral(priority),
                                            static_cast<tools::fee_algorithm>(fee_algorithm));
    }
    catch (const exception &e)
    {
        setStatusError((boost::format(tr("Failed to get fee multiplier for priority `%u` and fee algorithm `%d`: %s")) % priority % fee_algorithm % e.what()).str());
    }
    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getBaseFee() const
{
    return m_wallet->get_base_fee();
}
//-------------------------------------------------------------------------------------------------------------------
std::uint32_t WalletImpl::adjustPriority(std::uint32_t priority)
{
    return tools::fee_priority_utilities::as_integral(m_wallet->adjust_priority(priority));
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::coldTxAuxImport(const PendingTransaction &ptx, const std::vector<std::string> &tx_device_aux) const
{
    clearStatus();

    const PendingTransactionImpl *ptx_impl = dynamic_cast<const PendingTransactionImpl*>(&ptx);

    try
    {
        m_wallet->cold_tx_aux_import(ptx_impl->m_pending_tx, tx_device_aux);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to import cold tx aux: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::coldSignTx(const PendingTransaction &ptx_in, PendingTransaction &exported_txs_out) const
{
    clearStatus();

    try
    {
        const PendingTransactionImpl *ptx_impl_in = dynamic_cast<const PendingTransactionImpl*>(&ptx_in);
        PendingTransactionImpl *ptx_impl_out = dynamic_cast<PendingTransactionImpl*>(&exported_txs_out);
        tools::wallet2::signed_tx_set signed_txs;
        std::vector<cryptonote::address_parse_info> dsts_info;

        m_wallet->cold_sign_tx(ptx_impl_in->m_pending_tx, signed_txs, dsts_info, ptx_impl_out->m_tx_device_aux);
        ptx_impl_out->m_key_images = signed_txs.key_images;
        ptx_impl_out->m_pending_tx = signed_txs.ptx;
        ptx_impl_out->m_tx_key_images = signed_txs.tx_key_images;
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to cold sign tx: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::discardUnmixableEnotes()
{
    clearStatus();

    try
    {
        m_wallet->discard_unmixable_outputs();
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to discard unmixable enotes: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setTxKey(const std::string &txid, const std::string &tx_key, const std::vector<std::string> &additional_tx_keys, const std::string &single_destination_subaddress)
{
    clearStatus();

    crypto::hash txid_pod;
    if (!epee::string_tools::hex_to_pod(txid, txid_pod))
    {
        setStatusError(string(tr("Failed to parse tx id: ")) + txid);
        return;
    }

    crypto::secret_key tx_key_pod;
    if (!epee::string_tools::hex_to_pod(tx_key, tx_key_pod))
    {
        setStatusError(tr("Failed to parse tx key"));
        return;
    }

    std::vector<crypto::secret_key> additional_tx_keys_pod;
    crypto::secret_key tmp_additional_tx_key_pod;
    additional_tx_keys_pod.reserve(additional_tx_keys.size());
    for (std::string additional_tx_key : additional_tx_keys)
    {
        if (!epee::string_tools::hex_to_pod(additional_tx_key, tmp_additional_tx_key_pod))
        {
            setStatusError(string(tr("Failed to parse additional tx key: ")) + additional_tx_key);
            return;
        }
        additional_tx_keys_pod.push_back(tmp_additional_tx_key_pod);
    }

    boost::optional<cryptonote::account_public_address> single_destination_subaddress_pod = boost::none;
    cryptonote::address_parse_info info;
    if (!single_destination_subaddress.empty())
    {
        if (!cryptonote::get_account_address_from_str(info, m_wallet->nettype(), single_destination_subaddress))
        {
            setStatusError(string(tr("Failed to get account address from string: ")) + single_destination_subaddress);
            return;
        }
        single_destination_subaddress_pod = info.address;
    }

    try
    {
        m_wallet->set_tx_key(txid_pod, tx_key_pod, additional_tx_keys_pod, info.address);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to set tx key: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
const std::pair<std::map<std::string, std::string>, std::vector<std::string>>& WalletImpl::getAccountTags() const
{
    return m_wallet->get_account_tags();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAccountTag(const std::set<uint32_t> &account_indices, const std::string &tag)
{
    clearStatus();

    try
    {
        m_wallet->set_account_tag(account_indices, tag);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to set account tag: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAccountTagDescription(const std::string &tag, const std::string &description)
{
    clearStatus();

    try
    {
        m_wallet->set_account_tag_description(tag, description);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to set account tag description: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::exportEnotesToStr(bool all, std::uint32_t start, std::uint32_t count) const
{
    clearStatus();
    if (checkBackgroundSync("cannot export enotes"))
        return "";
    if (m_wallet->key_on_device())
    {
        setStatusError(string(tr("Not supported on HW wallets.")));
        return "";
    }

    try
    {
        return m_wallet->export_outputs_to_str(all, start, count);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to export enotes to string: ")) + e.what());
    }
    return "";
}
//-------------------------------------------------------------------------------------------------------------------
std::size_t WalletImpl::importEnotesFromStr(const std::string &enotes_str)
{
    clearStatus();
    if (checkBackgroundSync("cannot import enotes"))
        return 0;
    if (m_wallet->key_on_device())
    {
        setStatusError(string(tr("Not supported on HW wallets.")));
        return 0;
    }

    try
    {
        return m_wallet->import_outputs_from_str(enotes_str);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to import enotes from string: ")) + e.what());
    }
    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getBlockchainHeightByDate(std::uint16_t year, std::uint8_t month, std::uint8_t day) const
{
    clearStatus();

    try
    {
        return m_wallet->get_blockchain_height_by_date(year, month, day);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to get blockchain height by date: ")) + e.what());
    }
    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::pair<std::uint64_t, std::uint64_t>> WalletImpl::estimateBacklog(const std::vector<std::pair<double, double>> &fee_levels) const
{
    clearStatus();

    try
    {
        return m_wallet->estimate_backlog(fee_levels);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to estimate backlog: ")) + e.what());
    }
    return { std::make_pair(0, 0) };
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::hashEnotes(std::uint64_t enote_idx, std::string &hash) const
{
    clearStatus();

    try
    {
        crypto::hash hash_pod;
        boost::optional<std::uint64_t> idx = enote_idx == 0 ? boost::none : boost::optional<std::uint64_t>(enote_idx);
        std::uint64_t current_height = m_wallet->hash_m_transfers(idx, hash_pod);
        hash = epee::string_tools::pod_to_hex(hash_pod);
        return current_height;
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to hash enotes: ")) + e.what());
    }
    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::finishRescanBcKeepKeyImages(std::uint64_t enote_idx, const std::string &hash)
{
    clearStatus();

    try
    {
        crypto::hash hash_pod;
        if (!epee::string_tools::hex_to_pod(hash, hash_pod))
            setStatusError(tr("Failed to parse hash"));
        else
            m_wallet->finish_rescan_bc_keep_key_images(enote_idx, hash_pod);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to finish rescan blockchain: ")) + e.what());
    }
}
//-------------------------------------------------------------------------------------------------------------------
std::vector<std::tuple<std::string, std::uint16_t, std::uint64_t>> WalletImpl::getPublicNodes(bool white_only /* = true */) const
{
    clearStatus();

    std::vector<cryptonote::public_node> public_nodes;
    std::vector<std::tuple<std::string, std::uint16_t, std::uint64_t>> public_nodes_out;
    try
    {
        public_nodes = m_wallet->get_public_nodes(white_only);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to get public nodes: ")) + e.what());
        return std::vector<std::tuple<std::string, std::uint16_t, std::uint64_t>>{};
    }

    for (auto pub_node : public_nodes)
        public_nodes_out.push_back(std::tuple<std::string, std::uint16_t, std::uint64_t>{pub_node.host, pub_node.rpc_port, pub_node.last_seen});

    return public_nodes_out;
}
//-------------------------------------------------------------------------------------------------------------------
std::pair<std::size_t, std::uint64_t> WalletImpl::estimateTxSizeAndWeight(bool use_rct, int n_inputs, int ring_size, int n_outputs, std::size_t extra_size) const
{
    clearStatus();

    try
    {
        return m_wallet->estimate_tx_size_and_weight(use_rct, n_inputs, ring_size, n_outputs, extra_size);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to estimate transaction size and weight: ")) + e.what());
    }
    return std::make_pair(0, 0);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::importKeyImages(const std::vector<std::pair<std::string, std::string>> &signed_key_images, std::size_t offset, std::uint64_t &spent, std::uint64_t &unspent, bool check_spent)
{
    clearStatus();

    std::vector<std::pair<crypto::key_image, crypto::signature>> signed_key_images_pod;
    crypto::key_image tmp_key_image_pod;
    crypto::signature tmp_signature_pod{};
    size_t sig_size = sizeof(crypto::signature);

    signed_key_images_pod.reserve(signed_key_images.size());

    for (auto ski : signed_key_images)
    {
        if (!epee::string_tools::hex_to_pod(ski.first, tmp_key_image_pod))
        {
            setStatusError(string(tr("Failed to parse key image: ")) + ski.first);
            return false;
        }
        if (!epee::string_tools::hex_to_pod(ski.second.substr(0, sig_size/2), tmp_signature_pod.c))
        {
            setStatusError(string(tr("Failed to parse signature.c: ")) + ski.second.substr(0, sig_size/2));
            return false;
        }
        if (!epee::string_tools::hex_to_pod(ski.second.substr(sig_size/2, sig_size), tmp_signature_pod.r))
        {
            setStatusError(string(tr("Failed to parse signature.r: ")) + ski.second.substr(sig_size/2, sig_size));
            return false;
        }
        signed_key_images_pod.push_back(std::make_pair(tmp_key_image_pod, tmp_signature_pod));
    }

    try
    {
        return m_wallet->import_key_images(signed_key_images_pod, offset, spent, unspent, check_spent);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to import key images: ")) + e.what());
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::importKeyImages(const std::vector<std::string> &key_images, std::size_t offset, const std::unordered_set<std::size_t> &selected_enotes_indices)
{
    clearStatus();

    std::vector<crypto::key_image> key_images_pod;
    crypto::key_image tmp_key_image_pod;
    key_images_pod.reserve(key_images.size());
    for (std::string key_image : key_images)
    {
        if (!epee::string_tools::hex_to_pod(key_image, tmp_key_image_pod))
        {
            setStatusError(string(tr("Failed to parse key image: ")) + key_image);
            return false;
        }
        key_images_pod.push_back(tmp_key_image_pod);
    }

    try
    {
        boost::optional<std::unordered_set<std::size_t>> optional_selected_enote_indices = selected_enotes_indices.empty() ? boost::none : boost::optional<std::unordered_set<std::size_t>>(selected_enotes_indices);
        return m_wallet->import_key_images(key_images_pod, offset, optional_selected_enote_indices);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to import key images: ")) + e.what());
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getAllowMismatchedDaemonVersion() const
{
    return m_wallet->is_mismatched_daemon_version_allowed();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAllowMismatchedDaemonVersion(bool allow_mismatch)
{
    m_wallet->allow_mismatched_daemon_version(allow_mismatch);
}
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::getDeviceDerivationPath() const
{
    return m_wallet->device_derivation_path();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setDeviceDerivationPath(std::string device_derivation_path)
{
    m_wallet->device_derivation_path(device_derivation_path);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::setDaemon(const std::string &daemon_address,
        const std::string &daemon_username /* = "" */,
        const std::string &daemon_password /* = "" */,
        bool trusted_daemon /* = false */,
        Wallet::SSLSupport ssl_support /* = Wallet::SSLSupport::SSLSupport_Autodetect */,
        const std::string &ssl_private_key_path /* = "" */,
        const std::string &ssl_certificate_path /* = "" */,
        const std::string &ssl_ca_file_path /* = "" */,
        const std::vector<std::string> &ssl_allowed_fingerprints_str /* = {} */,
        bool ssl_allow_any_cert /* = false */,
        const std::string &proxy /* = "" */)
{
    clearStatus();

    // SSL allowed fingerprints
    std::vector<std::vector<uint8_t>> ssl_allowed_fingerprints;
    ssl_allowed_fingerprints.reserve(ssl_allowed_fingerprints_str.size());
    for (const std::string &fp: ssl_allowed_fingerprints_str)
    {
        ssl_allowed_fingerprints.push_back({});
        std::vector<uint8_t> &v = ssl_allowed_fingerprints.back();
        for (auto c: fp)
            v.push_back(c);
    }

    // SSL options
    epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_enabled;
    if (ssl_allow_any_cert)
        ssl_options.verification = epee::net_utils::ssl_verification_t::none;
    else if (!ssl_allowed_fingerprints.empty() || !ssl_ca_file_path.empty())
        ssl_options = epee::net_utils::ssl_options_t{std::move(ssl_allowed_fingerprints), ssl_ca_file_path};

    ssl_options.support = static_cast<epee::net_utils::ssl_support_t>(ssl_support);

    ssl_options.auth = epee::net_utils::ssl_authentication_t{
        std::move(ssl_private_key_path), std::move(ssl_certificate_path)
    };

    const bool verification_required =
        ssl_options.verification != epee::net_utils::ssl_verification_t::none &&
        ssl_options.support == epee::net_utils::ssl_support_t::e_ssl_support_enabled;

    if (verification_required && !ssl_options.has_strong_verification(boost::string_ref{}))
    {
        setStatusError(string(tr("SSL is enabled but no user certificate or fingerprints were provided")));
        return false;
    }

    // daemon login
    if(daemon_username != "")
        m_daemon_login.emplace(daemon_username, daemon_password);

    // set daemon
    try
    {
        return m_wallet->set_daemon(daemon_address, m_daemon_login, trusted_daemon, ssl_options, proxy);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to set daemon: ")) + e.what());
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::verifyPassword(const std::string_view &password)
{
    clearStatus();

    bool r = false;
    bool was_locked = false;

    if (isKeysFileLocked())
    {
        was_locked = true;
        unlockKeysFile();
    }

    try
    {
        bool no_spend_key = getDeviceState().protocol == DeviceState::DeviceProtocol::DeviceProtocol_Cold || watchOnly() || multisig().isMultisig || isBackgroundWallet();
        r = tools::wallet2::verify_password(keysFilename(),
                                            epee::wipeable_string(password.data(), password.size()),
                                            no_spend_key,
                                            hw::get_device("default"),
                                            m_kdf_rounds);
    }
    catch (const std::exception &e)
    {
        setStatusError(string(tr("Failed to verify password: ")) + e.what());
    }

    if (was_locked)
        lockKeysFile();

    return r;
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::encryptKeys(const std::string_view &password)
{
    m_wallet->encrypt_keys(epee::wipeable_string(password.data(), password.size()));
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::decryptKeys(const std::string_view &password)
{
    m_wallet->decrypt_keys(epee::wipeable_string(password.data(), password.size()));
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getMinRingSize() const
{
    try { return m_wallet->get_min_ring_size(); }
    catch (const std::exception &e)
    { setStatusError(std::string(tr("failed to get min ring size")) + e.what()); }
    return 0;
}
std::uint64_t WalletImpl::getMaxRingSize() const
{
    try { return m_wallet->get_max_ring_size(); }
    catch (const std::exception &e)
    { setStatusError(std::string(tr("failed to get max ring size")) + e.what()); }
    return 0;
}
std::uint64_t WalletImpl::adjustMixin(const std::uint64_t fake_outs_count) const
{
    return m_wallet->adjust_mixin(fake_outs_count);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getDaemonAdjustedTime() const
{
    return m_wallet->get_daemon_adjusted_time();
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getLastBlockReward() const
{
    return m_wallet->get_last_block_reward();
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::hasUnknownKeyImages() const
{
    return m_wallet->has_unknown_key_images();
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getExplicitRefreshFromBlockHeight() const
{
    return m_wallet->explicit_refresh_from_block_height();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setExplicitRefreshFromBlockHeight(bool do_explicit_refresh)
{
    m_wallet->explicit_refresh_from_block_height(do_explicit_refresh);
}
//-------------------------------------------------------------------------------------------------------------------
// Wallet Settings getter/setter
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::getSeedLanguage() const
{
    return m_wallet->get_seed_language();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setSeedLanguage(const std::string &arg)
{
    if (checkBackgroundSync("cannot set seed language"))
        return;

    if (crypto::ElectrumWords::is_valid_language(arg))
        m_wallet->set_seed_language(arg);
    else
        setStatusError(string(tr("Failed to set seed language. Language not valid: ")) + arg);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getAlwaysConfirmTransfers() const
{
    return m_wallet->always_confirm_transfers();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAlwaysConfirmTransfers(bool do_always_confirm)
{
    m_wallet->always_confirm_transfers(do_always_confirm);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getPrintRingMembers() const
{
    return m_wallet->print_ring_members();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setPrintRingMembers(bool do_print_ring_members)
{
    m_wallet->print_ring_members(do_print_ring_members);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getStoreTxInfo() const
{
    return m_wallet->store_tx_info();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setStoreTxInfo(bool do_store_tx_info)
{
    m_wallet->store_tx_info(do_store_tx_info);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getAutoRefresh() const
{
    return m_wallet->auto_refresh();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAutoRefresh(bool do_auto_refresh)
{
    m_wallet->auto_refresh(do_auto_refresh);
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::RefreshType WalletImpl::getRefreshType() const
{
    return static_cast<Wallet::RefreshType>(m_wallet->get_refresh_type());
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setRefreshType(Wallet::RefreshType refresh_type)
{
    m_wallet->set_refresh_type(static_cast<tools::wallet2::RefreshType>(refresh_type));
}
//-------------------------------------------------------------------------------------------------------------------
std::uint32_t WalletImpl::getDefaultPriority() const
{
    return tools::fee_priority_utilities::as_integral(m_wallet->get_default_priority());
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setDefaultPriority(std::uint32_t default_priority)
{
    m_wallet->set_default_priority(tools::fee_priority_utilities::from_integral(default_priority));
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::AskPasswordType WalletImpl::getAskPasswordType() const
{
    return static_cast<AskPasswordType>(m_wallet->ask_password());
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAskPasswordType(AskPasswordType ask_password_type)
{
    m_wallet->ask_password(static_cast<tools::wallet2::AskPasswordType>(ask_password_type));
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getMaxReorgDepth() const
{
    return m_wallet->max_reorg_depth();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setMaxReorgDepth(std::uint64_t max_reorg_depth)
{
    m_wallet->max_reorg_depth(max_reorg_depth);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint32_t WalletImpl::getMinOutputCount() const
{
    return m_wallet->get_min_output_count();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setMinOutputCount(std::uint32_t min_output_count)
{
    m_wallet->set_min_output_count(min_output_count);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getMinOutputValue() const
{
    return m_wallet->get_min_output_value();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setMinOutputValue(std::uint64_t min_output_value)
{
    m_wallet->set_min_output_value(min_output_value);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getMergeDestinations() const
{
    return m_wallet->merge_destinations();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setMergeDestinations(bool do_merge_destinations)
{
    m_wallet->merge_destinations(do_merge_destinations);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getConfirmBacklog() const
{
    return m_wallet->confirm_backlog();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setConfirmBacklog(bool do_confirm_backlog)
{
    m_wallet->confirm_backlog(do_confirm_backlog);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint32_t WalletImpl::getConfirmBacklogThreshold() const
{
    return m_wallet->get_confirm_backlog_threshold();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setConfirmBacklogThreshold(std::uint32_t confirm_backlog_threshold)
{
    m_wallet->set_confirm_backlog_threshold(confirm_backlog_threshold);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getConfirmExportOverwrite() const
{
    return m_wallet->confirm_export_overwrite();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setConfirmExportOverwrite(bool do_confirm_export_overwrite)
{
    m_wallet->confirm_export_overwrite(do_confirm_export_overwrite);
}
//-------------------------------------------------------------------------------------------------------------------
uint64_t WalletImpl::getRefreshFromBlockHeight() const
{
    return m_wallet->get_refresh_from_block_height();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setRefreshFromBlockHeight(uint64_t refresh_from_block_height)
{
    if (checkBackgroundSync("cannot change refresh height"))
        return;
    m_wallet->set_refresh_from_block_height(refresh_from_block_height);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getAutoLowPriority() const
{
    return m_wallet->auto_low_priority();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setAutoLowPriority(bool use_auto_low_priority)
{
    m_wallet->auto_low_priority(use_auto_low_priority);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getSegregatePreForkOutputs() const
{
    return m_wallet->segregate_pre_fork_outputs();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setSegregatePreForkOutputs(bool do_segregate)
{
    m_wallet->segregate_pre_fork_outputs(do_segregate);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getKeyReuseMitigation2() const
{
    return m_wallet->key_reuse_mitigation2();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setKeyReuseMitigation2(bool mitigation)
{
    m_wallet->key_reuse_mitigation2(mitigation);
}
//-------------------------------------------------------------------------------------------------------------------
std::pair<std::uint32_t, std::uint32_t> WalletImpl::getSubaddressLookahead() const
{
    return m_wallet->get_subaddress_lookahead();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setSubaddressLookahead(uint32_t major, uint32_t minor)
{
    m_wallet->set_subaddress_lookahead(major, minor);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getSegregationHeight() const
{
    return m_wallet->segregation_height();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setSegregationHeight(std::uint64_t height)
{
    m_wallet->segregation_height(height);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getIgnoreFractionalOutputs() const
{
    return m_wallet->ignore_fractional_outputs();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setIgnoreFractionalOutputs(bool do_ignore_fractional_outputs)
{
    m_wallet->ignore_fractional_outputs(do_ignore_fractional_outputs);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getIgnoreOutputsAbove() const
{
    return m_wallet->ignore_outputs_above();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setIgnoreOutputsAbove(std::uint64_t ignore_outputs_above)
{
    m_wallet->ignore_outputs_above(ignore_outputs_above);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint64_t WalletImpl::getIgnoreOutputsBelow() const
{
    return m_wallet->ignore_outputs_below();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setIgnoreOutputsBelow(std::uint64_t ignore_outputs_below)
{
    m_wallet->ignore_outputs_below(ignore_outputs_below);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getTrackUses() const
{
    return m_wallet->track_uses();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setTrackUses(bool do_track_uses)
{
    m_wallet->track_uses(do_track_uses);
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::BackgroundMiningSetupType WalletImpl::getSetupBackgroundMining() const
{
    return static_cast<Wallet::BackgroundMiningSetupType>(m_wallet->setup_background_mining());
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setSetupBackgroundMining(Wallet::BackgroundMiningSetupType background_mining_setup)
{
    m_wallet->setup_background_mining(static_cast<tools::wallet2::BackgroundMiningSetupType>(background_mining_setup));
}
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::getDeviceName() const
{
    return m_wallet->device_name();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setDeviceName(const std::string &device_name)
{
    m_wallet->device_name(device_name);
}
//-------------------------------------------------------------------------------------------------------------------
Wallet::ExportFormat WalletImpl::getExportFormat() const
{
    return static_cast<Wallet::ExportFormat>(m_wallet->export_format());
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setExportFormat(Wallet::ExportFormat export_format)
{
    return m_wallet->set_export_format(static_cast<tools::wallet2::ExportFormat>(export_format));
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getShowWalletNameWhenLocked() const
{
    return m_wallet->show_wallet_name_when_locked();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setShowWalletNameWhenLocked(bool do_show_wallet_name)
{
    m_wallet->show_wallet_name_when_locked(do_show_wallet_name);
}
//-------------------------------------------------------------------------------------------------------------------
std::uint32_t WalletImpl::getInactivityLockTimeout() const
{
    return m_wallet->inactivity_lock_timeout();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setInactivityLockTimeout(std::uint32_t seconds)
{
    m_wallet->inactivity_lock_timeout(seconds);
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::getEnableMultisig() const
{
    return m_wallet->is_multisig_enabled();
}
//-------------------------------------------------------------------------------------------------------------------
void WalletImpl::setEnableMultisig(bool do_enable_multisig)
{
    m_wallet->enable_multisig(do_enable_multisig);
}
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
// PRIVATE
//-------------------------------------------------------------------------------------------------------------------
std::size_t WalletImpl::getEnoteIndex(const std::string &key_image) const
{
    std::vector<std::unique_ptr<EnoteDetails>> enote_details = getEnoteDetails();
    for (size_t idx = 0; idx < enote_details.size(); ++idx)
    {
        const auto &ed = dynamic_cast<EnoteDetailsImpl &>(*enote_details[idx].get());
        if (ed.m_key_image == key_image)
        {
            if (ed.m_key_image_known)
                return idx;
            else if (ed.m_key_image_partial)
            {
                setStatusError("Failed to get enote index by key image: Enote detail lookups are not allowed for multisig partial key images");
                return 0;
            }
        }
    }

    setStatusError("Failed to get enote index by key image: Key image not found");
    return 0;
}
//-------------------------------------------------------------------------------------------------------------------
std::string WalletImpl::getPaymentIdFromExtra(const std::vector<std::uint8_t> &tx_extra) const
{
    std::vector<tx_extra_field> tx_extra_fields;
    parse_tx_extra(tx_extra, tx_extra_fields); // failure ok
    tx_extra_nonce extra_nonce;
    tx_extra_pub_key extra_pub_key;
    crypto::hash8 payment_id8 = crypto::null_hash8;
    crypto::hash payment_id = crypto::null_hash;
    if (find_tx_extra_field_by_type(tx_extra_fields, extra_pub_key))
    {
        const crypto::public_key &tx_pub_key = extra_pub_key.pub_key;
        if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
        {
            if (get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
            {
                m_wallet->get_account().get_device().decrypt_payment_id(payment_id8, tx_pub_key, m_wallet->get_account().get_keys().m_view_secret_key);
                return epee::string_tools::pod_to_hex(payment_id8);
            }
            else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
            {
                return epee::string_tools::pod_to_hex(payment_id);
            }
        }
    }
    return std::string();
}
//-------------------------------------------------------------------------------------------------------------------
bool WalletImpl::statusOk() const
{
    boost::lock_guard<boost::mutex> l(m_statusMutex);
    return m_status == Status_Ok;
}
//-------------------------------------------------------------------------------------------------------------------


} // namespace
