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

#pragma once

#include <memory>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#if BOOST_VERSION >= 107400
#include <boost/serialization/library_version_type.hpp>
#endif
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/thread/lock_guard.hpp>
#include <atomic>
#include <random>

#include "include_base_utils.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/account_boost_serialization.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "net/http.h"
#include "storages/http_abstract_invoke.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "common/unordered_containers_boost_serialization.h"
#include "common/util.h"
#include "crypto/chacha.h"
#include "crypto/hash.h"
#include "ringct/rctTypes.h"
#include "ringct/rctOps.h"
#include "checkpoints/checkpoints.h"
#include "serialization/crypto.h"
#include "serialization/string.h"
#include "serialization/pair.h"
#include "serialization/tuple.h"
#include "serialization/containers.h"

#include "wallet_errors.h"
#include "common/password.h"
#include "node_rpc_proxy.h"
#include "message_store.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.wallet2"

#define THROW_ON_RPC_RESPONSE_ERROR(r, error, res, method, ...) \
  do { \
    throw_on_rpc_response_error(r, error, res.status, method); \
    THROW_WALLET_EXCEPTION_IF(res.status != CORE_RPC_STATUS_OK, ## __VA_ARGS__); \
  } while(0)

#define THROW_ON_RPC_RESPONSE_ERROR_GENERIC(r, err, res, method) \
  THROW_ON_RPC_RESPONSE_ERROR(r, err, res, method, tools::error::wallet_generic_rpc_error, method, res.status)

class Serialization_portability_wallet_Test;
class wallet_accessor_test;

namespace tools
{
  class ringdb;
  class wallet2;
  class Notify;

  class gamma_picker
  {
  public:
    uint64_t pick();
    gamma_picker(const std::vector<uint64_t> &rct_offsets);
    gamma_picker(const std::vector<uint64_t> &rct_offsets, double shape, double scale);
    uint64_t get_num_rct_outs() const { return num_rct_outputs; }

  private:
    struct gamma_engine
    {
      typedef uint64_t result_type;
      static constexpr result_type min() { return 0; }
      static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
      result_type operator()() { return crypto::rand<result_type>(); }
    } engine;

private:
    std::gamma_distribution<double> gamma;
    const std::vector<uint64_t> &rct_offsets;
    const uint64_t *begin, *end;
    uint64_t num_rct_outputs;
    double average_output_time;
  };

  class wallet_keys_unlocker
  {
  public:
    wallet_keys_unlocker(wallet2 &w, const boost::optional<tools::password_container> &password);
    wallet_keys_unlocker(wallet2 &w, bool locked, const epee::wipeable_string &password);
    ~wallet_keys_unlocker();
  private:
    wallet2 &w;
    bool locked;
    crypto::chacha_key key;
    static boost::mutex lockers_lock;
    static unsigned int lockers;
  };

  class i_wallet2_callback
  {
  public:
    // Full wallet callbacks
    virtual void on_new_block(uint64_t height, const cryptonote::block& block) {}
    virtual void on_reorg(uint64_t height, uint64_t blocks_detached, size_t transfers_detached) {}
    virtual void on_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, uint64_t burnt, const cryptonote::subaddress_index& subaddr_index, bool is_change, uint64_t unlock_time) {}
    virtual void on_unconfirmed_money_received(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t amount, const cryptonote::subaddress_index& subaddr_index) {}
    virtual void on_money_spent(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& in_tx, uint64_t amount, const cryptonote::transaction& spend_tx, const cryptonote::subaddress_index& subaddr_index) {}
    virtual void on_skip_transaction(uint64_t height, const crypto::hash &txid, const cryptonote::transaction& tx) {}
    virtual boost::optional<epee::wipeable_string> on_get_password(const char *reason) { return boost::none; }
    // Device callbacks
    virtual void on_device_button_request(uint64_t code) {}
    virtual void on_device_button_pressed() {}
    virtual boost::optional<epee::wipeable_string> on_device_pin_request() { return boost::none; }
    virtual boost::optional<epee::wipeable_string> on_device_passphrase_request(bool & on_device) { on_device = true; return boost::none; }
    virtual void on_device_progress(const hw::device_progress& event) {};
    // Common callbacks
    virtual void on_pool_tx_removed(const crypto::hash &txid) {}
    virtual ~i_wallet2_callback() {}
  };

  class wallet_device_callback : public hw::i_device_callback
  {
  public:
    wallet_device_callback(wallet2 * wallet): wallet(wallet) {};
    void on_button_request(uint64_t code=0) override;
    void on_button_pressed() override;
    boost::optional<epee::wipeable_string> on_pin_request() override;
    boost::optional<epee::wipeable_string> on_passphrase_request(bool & on_device) override;
    void on_progress(const hw::device_progress& event) override;
  private:
    wallet2 * wallet;
  };

  struct tx_dust_policy
  {
    uint64_t dust_threshold;
    bool add_to_fee;
    cryptonote::account_public_address addr_for_dust;

    tx_dust_policy(uint64_t a_dust_threshold = 0, bool an_add_to_fee = true, cryptonote::account_public_address an_addr_for_dust = cryptonote::account_public_address())
      : dust_threshold(a_dust_threshold)
      , add_to_fee(an_add_to_fee)
      , addr_for_dust(an_addr_for_dust)
    {
    }
  };

  class hashchain
  {
  public:
    hashchain(): m_genesis(crypto::null_hash), m_offset(0) {}

    size_t size() const { return m_blockchain.size() + m_offset; }
    size_t offset() const { return m_offset; }
    const crypto::hash &genesis() const { return m_genesis; }
    void push_back(const crypto::hash &hash) { if (m_offset == 0 && m_blockchain.empty()) m_genesis = hash; m_blockchain.push_back(hash); }
    bool is_in_bounds(size_t idx) const { return idx >= m_offset && idx < size(); }
    const crypto::hash &operator[](size_t idx) const { return m_blockchain[idx - m_offset]; }
    crypto::hash &operator[](size_t idx) { return m_blockchain[idx - m_offset]; }
    void crop(size_t height) { m_blockchain.resize(height - m_offset); }
    void clear() { m_offset = 0; m_blockchain.clear(); }
    bool empty() const { return m_blockchain.empty() && m_offset == 0; }
    void trim(size_t height) { while (height > m_offset && m_blockchain.size() > 1) { m_blockchain.pop_front(); ++m_offset; } m_blockchain.shrink_to_fit(); }
    void refill(const crypto::hash &hash) { m_blockchain.push_back(hash); --m_offset; }

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      a & m_offset;
      a & m_genesis;
      a & m_blockchain;
    }

    BEGIN_SERIALIZE_OBJECT()
      VERSION_FIELD(0)
      VARINT_FIELD(m_offset)
      FIELD(m_genesis)
      FIELD(m_blockchain)
    END_SERIALIZE()

  private:
    size_t m_offset;
    crypto::hash m_genesis;
    std::deque<crypto::hash> m_blockchain;
  };

  class wallet_keys_unlocker;
  class wallet2
  {
    friend class ::Serialization_portability_wallet_Test;
    friend class ::wallet_accessor_test;
    friend class wallet_keys_unlocker;
    friend class wallet_device_callback;
  public:
    static constexpr const std::chrono::seconds rpc_timeout = std::chrono::minutes(3) + std::chrono::seconds(30);

    enum RefreshType {
      RefreshFull,
      RefreshOptimizeCoinbase,
      RefreshNoCoinbase,
      RefreshDefault = RefreshOptimizeCoinbase,
    };

    enum AskPasswordType {
      AskPasswordNever = 0,
      AskPasswordOnAction = 1,
      AskPasswordToDecrypt = 2,
    };

    enum BackgroundMiningSetupType {
      BackgroundMiningMaybe = 0,
      BackgroundMiningYes = 1,
      BackgroundMiningNo = 2,
    };

    enum ExportFormat {
      Binary = 0,
      Ascii,
    };

    static const char* tr(const char* str);

    static bool has_testnet_option(const boost::program_options::variables_map& vm);
    static bool has_stagenet_option(const boost::program_options::variables_map& vm);
    static std::string device_name_option(const boost::program_options::variables_map& vm);
    static std::string device_derivation_path_option(const boost::program_options::variables_map &vm);
    static void init_options(boost::program_options::options_description& desc_params);

    //! Uses stdin and stdout. Returns a wallet2 if no errors.
    static std::pair<std::unique_ptr<wallet2>, password_container> make_from_json(const boost::program_options::variables_map& vm, bool unattended, const std::string& json_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Uses stdin and stdout. Returns a wallet2 and password for `wallet_file` if no errors.
    static std::pair<std::unique_ptr<wallet2>, password_container>
      make_from_file(const boost::program_options::variables_map& vm, bool unattended, const std::string& wallet_file, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Uses stdin and stdout. Returns a wallet2 and password for wallet with no file if no errors.
    static std::pair<std::unique_ptr<wallet2>, password_container> make_new(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    //! Just parses variables.
    static std::unique_ptr<wallet2> make_dummy(const boost::program_options::variables_map& vm, bool unattended, const std::function<boost::optional<password_container>(const char *, bool)> &password_prompter);

    static bool verify_password(const std::string& keys_file_name, const epee::wipeable_string& password, bool no_spend_key, hw::device &hwdev, uint64_t kdf_rounds);
    static bool query_device(hw::device::device_type& device_type, const std::string& keys_file_name, const epee::wipeable_string& password, uint64_t kdf_rounds = 1);

    wallet2(cryptonote::network_type nettype = cryptonote::MAINNET, uint64_t kdf_rounds = 1, bool unattended = false, std::unique_ptr<epee::net_utils::http::http_client_factory> http_client_factory = std::unique_ptr<epee::net_utils::http::http_client_factory>(new net::http::client_factory()));
    ~wallet2();

    struct multisig_info
    {
      struct LR
      {
        rct::key m_L;
        rct::key m_R;

        BEGIN_SERIALIZE_OBJECT()
          FIELD(m_L)
          FIELD(m_R)
        END_SERIALIZE()
      };

      crypto::public_key m_signer;
      std::vector<LR> m_LR;
      std::vector<crypto::key_image> m_partial_key_images; // one per key the participant has

      BEGIN_SERIALIZE_OBJECT()
        FIELD(m_signer)
        FIELD(m_LR)
        FIELD(m_partial_key_images)
      END_SERIALIZE()
    };

    struct tx_scan_info_t
    {
      cryptonote::keypair in_ephemeral;
      crypto::key_image ki;
      rct::key mask;
      uint64_t amount;
      uint64_t money_transfered;
      bool error;
      boost::optional<cryptonote::subaddress_receive_info> received;

      tx_scan_info_t(): amount(0), money_transfered(0), error(true) {}
    };

    struct transfer_details
    {
      uint64_t m_block_height;
      cryptonote::transaction_prefix m_tx;
      crypto::hash m_txid;
      uint64_t m_internal_output_index;
      uint64_t m_global_output_index;
      bool m_spent;
      bool m_frozen;
      uint64_t m_spent_height;
      crypto::key_image m_key_image; //TODO: key_image stored twice :(
      rct::key m_mask;
      uint64_t m_amount;
      bool m_rct;
      bool m_key_image_known;
      bool m_key_image_request; // view wallets: we want to request it; cold wallets: it was requested
      uint64_t m_pk_index;
      cryptonote::subaddress_index m_subaddr_index;
      bool m_key_image_partial;
      std::vector<rct::key> m_multisig_k;
      std::vector<multisig_info> m_multisig_info; // one per other participant
      std::vector<std::pair<uint64_t, crypto::hash>> m_uses;

      bool is_rct() const { return m_rct; }
      uint64_t amount() const { return m_amount; }
      const crypto::public_key get_public_key() const {
        crypto::public_key output_public_key;
        THROW_WALLET_EXCEPTION_IF(m_tx.vout.size() <= m_internal_output_index,
          error::wallet_internal_error, "Too few outputs, outputs may be corrupted");
        THROW_WALLET_EXCEPTION_IF(!get_output_public_key(m_tx.vout[m_internal_output_index], output_public_key),
          error::wallet_internal_error, "Unable to get output public key from output");
        return output_public_key;
      };

      BEGIN_SERIALIZE_OBJECT()
        FIELD(m_block_height)
        FIELD(m_tx)
        FIELD(m_txid)
        FIELD(m_internal_output_index)
        FIELD(m_global_output_index)
        FIELD(m_spent)
        FIELD(m_frozen)
        FIELD(m_spent_height)
        FIELD(m_key_image)
        FIELD(m_mask)
        FIELD(m_amount)
        FIELD(m_rct)
        FIELD(m_key_image_known)
        FIELD(m_key_image_request)
        FIELD(m_pk_index)
        FIELD(m_subaddr_index)
        FIELD(m_key_image_partial)
        FIELD(m_multisig_k)
        FIELD(m_multisig_info)
        FIELD(m_uses)
      END_SERIALIZE()
    };

    struct exported_transfer_details
    {
      crypto::public_key m_pubkey;
      uint64_t m_internal_output_index;
      uint64_t m_global_output_index;
      crypto::public_key m_tx_pubkey;
      union
      {
        struct
        {
          uint8_t m_spent: 1;
          uint8_t m_frozen: 1;
          uint8_t m_rct: 1;
          uint8_t m_key_image_known: 1;
          uint8_t m_key_image_request: 1; // view wallets: we want to request it; cold wallets: it was requested
          uint8_t m_key_image_partial: 1;
        };
        uint8_t flags;
      } m_flags;
      uint64_t m_amount;
      std::vector<crypto::public_key> m_additional_tx_keys;
      uint32_t m_subaddr_index_major;
      uint32_t m_subaddr_index_minor;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        if (version < 1)
          return false;
        FIELD(m_pubkey)
        VARINT_FIELD(m_internal_output_index)
        VARINT_FIELD(m_global_output_index)
        FIELD(m_tx_pubkey)
        FIELD(m_flags.flags)
        VARINT_FIELD(m_amount)
        FIELD(m_additional_tx_keys)
        VARINT_FIELD(m_subaddr_index_major)
        VARINT_FIELD(m_subaddr_index_minor)
      END_SERIALIZE()
    };

    typedef std::vector<uint64_t> amounts_container;
    struct payment_details
    {
      crypto::hash m_tx_hash;
      uint64_t m_amount;
      amounts_container m_amounts;
      uint64_t m_fee;
      uint64_t m_block_height;
      uint64_t m_unlock_time;
      uint64_t m_timestamp;
      bool m_coinbase;
      cryptonote::subaddress_index m_subaddr_index;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_tx_hash)
        VARINT_FIELD(m_amount)
        FIELD(m_amounts)
        VARINT_FIELD(m_fee)
        VARINT_FIELD(m_block_height)
        VARINT_FIELD(m_unlock_time)
        VARINT_FIELD(m_timestamp)
        FIELD(m_coinbase)
        FIELD(m_subaddr_index)
      END_SERIALIZE()
    };

    struct address_tx : payment_details
    {
      bool m_mempool;
      bool m_incoming;
    };

    struct pool_payment_details
    {
      payment_details m_pd;
      bool m_double_spend_seen;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_pd)
        FIELD(m_double_spend_seen)
      END_SERIALIZE()
    };

    struct unconfirmed_transfer_details
    {
      cryptonote::transaction_prefix m_tx;
      uint64_t m_amount_in;
      uint64_t m_amount_out;
      uint64_t m_change;
      time_t m_sent_time;
      std::vector<cryptonote::tx_destination_entry> m_dests;
      crypto::hash m_payment_id;
      enum { pending, pending_in_pool, failed } m_state;
      uint64_t m_timestamp;
      uint32_t m_subaddr_account;   // subaddress account of your wallet to be used in this transfer
      std::set<uint32_t> m_subaddr_indices;  // set of address indices used as inputs in this transfer
      std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> m_rings; // relative

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        FIELD(m_tx)
        VARINT_FIELD(m_amount_in)
        VARINT_FIELD(m_amount_out)
        VARINT_FIELD(m_change)
        VARINT_FIELD(m_sent_time)
        FIELD(m_dests)
        FIELD(m_payment_id)
        if (version >= 1)
          VARINT_FIELD(m_state)
        VARINT_FIELD(m_timestamp)
        VARINT_FIELD(m_subaddr_account)
        FIELD(m_subaddr_indices)
        FIELD(m_rings)
      END_SERIALIZE()
    };

    struct confirmed_transfer_details
    {
      cryptonote::transaction_prefix m_tx;
      uint64_t m_amount_in;
      uint64_t m_amount_out;
      uint64_t m_change;
      uint64_t m_block_height;
      std::vector<cryptonote::tx_destination_entry> m_dests;
      crypto::hash m_payment_id;
      uint64_t m_timestamp;
      uint64_t m_unlock_time;
      uint32_t m_subaddr_account;   // subaddress account of your wallet to be used in this transfer
      std::set<uint32_t> m_subaddr_indices;  // set of address indices used as inputs in this transfer
      std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> m_rings; // relative

      confirmed_transfer_details(): m_amount_in(0), m_amount_out(0), m_change((uint64_t)-1), m_block_height(0), m_payment_id(crypto::null_hash), m_timestamp(0), m_unlock_time(0), m_subaddr_account((uint32_t)-1) {}
      confirmed_transfer_details(const unconfirmed_transfer_details &utd, uint64_t height):
        m_tx(utd.m_tx), m_amount_in(utd.m_amount_in), m_amount_out(utd.m_amount_out), m_change(utd.m_change), m_block_height(height), m_dests(utd.m_dests), m_payment_id(utd.m_payment_id), m_timestamp(utd.m_timestamp), m_unlock_time(utd.m_tx.unlock_time), m_subaddr_account(utd.m_subaddr_account), m_subaddr_indices(utd.m_subaddr_indices), m_rings(utd.m_rings) {}

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        if (version >= 1)
          FIELD(m_tx)
        VARINT_FIELD(m_amount_in)
        VARINT_FIELD(m_amount_out)
        VARINT_FIELD(m_change)
        VARINT_FIELD(m_block_height)
        FIELD(m_dests)
        FIELD(m_payment_id)
        VARINT_FIELD(m_timestamp)
        VARINT_FIELD(m_unlock_time)
        VARINT_FIELD(m_subaddr_account)
        FIELD(m_subaddr_indices)
        FIELD(m_rings)
      END_SERIALIZE()
    };

    struct tx_construction_data
    {
      std::vector<cryptonote::tx_source_entry> sources;
      cryptonote::tx_destination_entry change_dts;
      std::vector<cryptonote::tx_destination_entry> splitted_dsts; // split, includes change
      std::vector<size_t> selected_transfers;
      std::vector<uint8_t> extra;
      uint64_t unlock_time;
      bool use_rct;
      rct::RCTConfig rct_config;
      bool use_view_tags;
      std::vector<cryptonote::tx_destination_entry> dests; // original setup, does not include change
      uint32_t subaddr_account;   // subaddress account of your wallet to be used in this transfer
      std::set<uint32_t> subaddr_indices;  // set of address indices used as inputs in this transfer

      enum construction_flags_ : uint8_t
      {
        _use_rct          = 1 << 0, // 00000001
        _use_view_tags    = 1 << 1  // 00000010
        // next flag      = 1 << 2  // 00000100
        // ...
        // final flag     = 1 << 7  // 10000000
      };
      uint8_t construction_flags;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(sources)
        FIELD(change_dts)
        FIELD(splitted_dsts)
        FIELD(selected_transfers)
        FIELD(extra)
        FIELD(unlock_time)

        // converted `use_rct` field into construction_flags when view tags
        // were introduced to maintain backwards compatibility
        if (!typename Archive<W>::is_saving())
        {
          FIELD_N("use_rct", construction_flags)
          use_rct = (construction_flags & _use_rct) > 0;
          use_view_tags = (construction_flags & _use_view_tags) > 0;
        }
        else
        {
          construction_flags = 0;
          if (use_rct)
            construction_flags ^= _use_rct;
          if (use_view_tags)
            construction_flags ^= _use_view_tags;

          FIELD_N("use_rct", construction_flags)
        }

        FIELD(rct_config)
        FIELD(dests)
        FIELD(subaddr_account)
        FIELD(subaddr_indices)
      END_SERIALIZE()
    };

    typedef std::vector<transfer_details> transfer_container;
    typedef serializable_unordered_multimap<crypto::hash, payment_details> payment_container;

    struct multisig_sig
    {
      rct::rctSig sigs;
      std::unordered_set<crypto::public_key> ignore;
      std::unordered_set<rct::key> used_L;
      std::unordered_set<crypto::public_key> signing_keys;
      rct::multisig_out msout;

      rct::keyM total_alpha_G;
      rct::keyM total_alpha_H;
      rct::keyV c_0;
      rct::keyV s;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        if (version < 1)
          return false;
        FIELD(sigs)
        FIELD(ignore)
        FIELD(used_L)
        FIELD(signing_keys)
        FIELD(msout)
        FIELD(total_alpha_G)
        FIELD(total_alpha_H)
        FIELD(c_0)
        FIELD(s)
      END_SERIALIZE()
    };

    // The convention for destinations is:
    // dests does not include change
    // splitted_dsts (in construction_data) does
    struct pending_tx
    {
      cryptonote::transaction tx;
      uint64_t dust, fee;
      bool dust_added_to_fee;
      cryptonote::tx_destination_entry change_dts;
      std::vector<size_t> selected_transfers;
      std::string key_images;
      crypto::secret_key tx_key;
      std::vector<crypto::secret_key> additional_tx_keys;
      std::vector<cryptonote::tx_destination_entry> dests;
      std::vector<multisig_sig> multisig_sigs;
      crypto::secret_key multisig_tx_key_entropy;

      tx_construction_data construction_data;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(1)
        FIELD(tx)
        FIELD(dust)
        FIELD(fee)
        FIELD(dust_added_to_fee)
        FIELD(change_dts)
        FIELD(selected_transfers)
        FIELD(key_images)
        FIELD(tx_key)
        FIELD(additional_tx_keys)
        FIELD(dests)
        FIELD(construction_data)
        FIELD(multisig_sigs)
        if (version < 1)
        {
          multisig_tx_key_entropy = crypto::null_skey;
          return true;
        }
        FIELD(multisig_tx_key_entropy)
      END_SERIALIZE()
    };

    // The term "Unsigned tx" is not really a tx since it's not signed yet.
    // It doesnt have tx hash, key and the integrated address is not separated into addr + payment id.
    struct unsigned_tx_set
    {
      std::vector<tx_construction_data> txes;
      std::tuple<uint64_t, uint64_t, wallet2::transfer_container> transfers;
      std::tuple<uint64_t, uint64_t, std::vector<wallet2::exported_transfer_details>> new_transfers;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(2)
        FIELD(txes)
        if (version == 0)
        {
          std::pair<size_t, wallet2::transfer_container> v0_transfers;
          FIELD(v0_transfers);
          std::get<0>(transfers) = std::get<0>(v0_transfers);
          std::get<1>(transfers) = std::get<0>(v0_transfers) + std::get<1>(v0_transfers).size();
          std::get<2>(transfers) = std::get<1>(v0_transfers);
          return true;
        }
        if (version == 1)
        {
          std::pair<size_t, std::vector<wallet2::exported_transfer_details>> v1_transfers;
          FIELD(v1_transfers);
          std::get<0>(new_transfers) = std::get<0>(v1_transfers);
          std::get<1>(new_transfers) = std::get<0>(v1_transfers) + std::get<1>(v1_transfers).size();
          std::get<2>(new_transfers) = std::get<1>(v1_transfers);
          return true;
        }

        FIELD(new_transfers)
      END_SERIALIZE()
    };

    struct signed_tx_set
    {
      std::vector<pending_tx> ptx;
      std::vector<crypto::key_image> key_images;
      serializable_unordered_map<crypto::public_key, crypto::key_image> tx_key_images;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(ptx)
        FIELD(key_images)
        FIELD(tx_key_images)
      END_SERIALIZE()
    };

    struct multisig_tx_set
    {
      std::vector<pending_tx> m_ptx;
      std::unordered_set<crypto::public_key> m_signers;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(m_ptx)
        FIELD(m_signers)
      END_SERIALIZE()
    };

    struct keys_file_data
    {
      crypto::chacha_iv iv;
      std::string account_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(account_data)
      END_SERIALIZE()
    };

    struct cache_file_data
    {
      crypto::chacha_iv iv;
      std::string cache_data;

      BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(cache_data)
      END_SERIALIZE()
    };

    // GUI Address book
    struct address_book_row
    {
      cryptonote::account_public_address m_address;
      crypto::hash8 m_payment_id;
      std::string m_description;   
      bool m_is_subaddress;
      bool m_has_payment_id;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(m_address)
        FIELD(m_payment_id)
        FIELD(m_description)
        FIELD(m_is_subaddress)
        FIELD(m_has_payment_id)
      END_SERIALIZE()
    };

    struct reserve_proof_entry
    {
      crypto::hash txid;
      uint64_t index_in_tx;
      crypto::public_key shared_secret;
      crypto::key_image key_image;
      crypto::signature shared_secret_sig;
      crypto::signature key_image_sig;

      BEGIN_SERIALIZE_OBJECT()
        VERSION_FIELD(0)
        FIELD(txid)
        VARINT_FIELD(index_in_tx)
        FIELD(shared_secret)
        FIELD(key_image)
        FIELD(shared_secret_sig)
        FIELD(key_image_sig)
      END_SERIALIZE()
    };

    typedef std::tuple<uint64_t, crypto::public_key, rct::key> get_outs_entry;

    struct parsed_block
    {
      crypto::hash hash;
      cryptonote::block block;
      std::vector<cryptonote::transaction> txes;
      cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices o_indices;
      bool error;
    };

    struct is_out_data
    {
      crypto::public_key pkey;
      crypto::key_derivation derivation;
      std::vector<boost::optional<cryptonote::subaddress_receive_info>> received;
    };

    struct tx_cache_data
    {
      std::vector<cryptonote::tx_extra_field> tx_extra_fields;
      std::vector<is_out_data> primary;
      std::vector<is_out_data> additional;

      bool empty() const { return tx_extra_fields.empty() && primary.empty() && additional.empty(); }
    };

    struct detached_blockchain_data
    {
      hashchain detached_blockchain;
      size_t original_chain_size;
      std::unordered_set<crypto::hash> detached_tx_hashes;
      std::unordered_map<crypto::hash, std::vector<cryptonote::tx_destination_entry>> detached_confirmed_txs_dests;
    };

    struct process_tx_entry_t
    {
      cryptonote::COMMAND_RPC_GET_TRANSACTIONS::entry tx_entry;
      cryptonote::transaction tx;
      crypto::hash tx_hash;
    };

    struct tx_entry_data
    {
      std::vector<process_tx_entry_t> tx_entries;
      uint64_t lowest_height;
      uint64_t highest_height;

      tx_entry_data(): lowest_height((uint64_t)-1), highest_height(0) {}
    };

    /*!
     * \brief  Generates a wallet or restores one. Assumes the multisig setup
      *        has already completed for the provided multisig info.
     * \param  wallet_              Name of wallet file
     * \param  password             Password of wallet file
     * \param  multisig_data        The multisig restore info and keys
     * \param  create_address_file  Whether to create an address file
     */
    void generate(const std::string& wallet_, const epee::wipeable_string& password,
      const epee::wipeable_string& multisig_data, bool create_address_file = false);

    /*!
     * \brief Generates a wallet or restores one.
     * \param  wallet_              Name of wallet file
     * \param  password             Password of wallet file
     * \param  recovery_param       If it is a restore, the recovery key
     * \param  recover              Whether it is a restore
     * \param  two_random           Whether it is a non-deterministic wallet
     * \param  create_address_file  Whether to create an address file
     * \return                      The secret key of the generated wallet
     */
    crypto::secret_key generate(const std::string& wallet, const epee::wipeable_string& password,
      const crypto::secret_key& recovery_param = crypto::secret_key(), bool recover = false,
      bool two_random = false, bool create_address_file = false);
    /*!
     * \brief Creates a wallet from a public address and a spend/view secret key pair.
     * \param  wallet_                 Name of wallet file
     * \param  password                Password of wallet file
     * \param  account_public_address  The account's public address
     * \param  spendkey                spend secret key
     * \param  viewkey                 view secret key
     * \param  create_address_file     Whether to create an address file
     */
    void generate(const std::string& wallet, const epee::wipeable_string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& spendkey, const crypto::secret_key& viewkey, bool create_address_file = false);
    /*!
     * \brief Creates a watch only wallet from a public address and a view secret key.
     * \param  wallet_                 Name of wallet file
     * \param  password                Password of wallet file
     * \param  account_public_address  The account's public address
     * \param  viewkey                 view secret key
     * \param  create_address_file     Whether to create an address file
     */
    void generate(const std::string& wallet, const epee::wipeable_string& password,
      const cryptonote::account_public_address &account_public_address,
      const crypto::secret_key& viewkey = crypto::secret_key(), bool create_address_file = false);
    /*!
     * \brief Restore a wallet hold by an HW.
     * \param  wallet_        Name of wallet file
     * \param  password       Password of wallet file
     * \param  device_name    name of HW to use
     * \param  create_address_file     Whether to create an address file
     */
    void restore(const std::string& wallet_, const epee::wipeable_string& password, const std::string &device_name, bool create_address_file = false);

    /*!
     * \brief Creates a multisig wallet
     * \return empty if done, non empty if we need to send another string
     * to other participants
     */
    std::string make_multisig(const epee::wipeable_string &password,
      const std::vector<std::string> &kex_messages,
      const std::uint32_t threshold);
    /*!
     * \brief Increment the multisig key exchange round
     * \return empty if done, non empty if we need to send another string
     * to other participants
     */
    std::string exchange_multisig_keys(const epee::wipeable_string &password,
      const std::vector<std::string> &kex_messages,
      const bool force_update_use_with_caution = false);
    /*!
     * \brief Get initial message to start multisig key exchange (before 'make_multisig()' is called)
     * \return string to send to other participants
     */
    std::string get_multisig_first_kex_msg() const;
    /*!
     * Export multisig info
     * This will generate and remember new k values
     */
    cryptonote::blobdata export_multisig();
    /*!
     * Import a set of multisig info from multisig partners
     * \return the number of inputs which were imported
     */
    size_t import_multisig(std::vector<cryptonote::blobdata> info);
    /*!
     * \brief Rewrites to the wallet file for wallet upgrade (doesn't generate key, assumes it's already there)
     * \param wallet_name Name of wallet file (should exist)
     * \param password    Password for wallet file
     */
    void rewrite(const std::string& wallet_name, const epee::wipeable_string& password);
    void write_watch_only_wallet(const std::string& wallet_name, const epee::wipeable_string& password, std::string &new_keys_filename);
    void load(const std::string& wallet, const epee::wipeable_string& password, const std::string& keys_buf = "", const std::string& cache_buf = "");
    void store();
    /*!
     * \brief store_to  Stores wallet to another file(s), deleting old ones
     * \param path      Path to the wallet file (keys and address filenames will be generated based on this filename)
     * \param password  Password that currently locks the wallet
     * \param force_rewrite_keys if true, always rewrite keys file
     *
     * Leave both "path" and "password" blank to restore the cache file to the current position in the disk
     * (which is the same as calling `store()`). If you want to store the wallet with a new password,
     * use the method `change_password()`.
     *
     * Normally the keys file is not overwritten when storing, except when force_rewrite_keys is true
     * or when `path` is a new wallet file.
     *
     * \throw error::invalid_password If storing keys file and old password is incorrect
     */
    void store_to(const std::string &path, const epee::wipeable_string &password, bool force_rewrite_keys = false);
    /*!
     * \brief get_keys_file_data  Get wallet keys data which can be stored to a wallet file.
     * \param password            Password that currently locks the wallet
     * \param watch_only          true to include only view key, false to include both spend and view keys
     * \return                    Encrypted wallet keys data which can be stored to a wallet file
     * \throw                     error::invalid_password if password does not match current wallet
     */
    boost::optional<wallet2::keys_file_data> get_keys_file_data(const epee::wipeable_string& password, bool watch_only);
    /*!
     * \brief get_cache_file_data   Get wallet cache data which can be stored to a wallet file.
     * \return                      Encrypted wallet cache data which can be stored to a wallet file (using current password)
     */
    boost::optional<wallet2::cache_file_data> get_cache_file_data();

    std::string path() const;

    /*!
     * \brief verifies given password is correct for default wallet keys file
     */
    bool verify_password(const epee::wipeable_string& password);
    cryptonote::account_base& get_account(){return m_account;}
    const cryptonote::account_base& get_account()const{return m_account;}

    void encrypt_keys(const crypto::chacha_key &key);
    void encrypt_keys(const epee::wipeable_string &password);
    void decrypt_keys(const crypto::chacha_key &key);
    void decrypt_keys(const epee::wipeable_string &password);

    void set_refresh_from_block_height(uint64_t height) {m_refresh_from_block_height = height;}
    uint64_t get_refresh_from_block_height() const {return m_refresh_from_block_height;}

    void explicit_refresh_from_block_height(bool expl) {m_explicit_refresh_from_block_height = expl;}
    bool explicit_refresh_from_block_height() const {return m_explicit_refresh_from_block_height;}

    void max_reorg_depth(uint64_t depth) {m_max_reorg_depth = depth;}
    uint64_t max_reorg_depth() const {return m_max_reorg_depth;}

    bool deinit();
    bool init(std::string daemon_address = "http://localhost:8080",
      boost::optional<epee::net_utils::http::login> daemon_login = boost::none,
      const std::string &proxy = "",
      uint64_t upper_transaction_weight_limit = 0,
      bool trusted_daemon = true,
      epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect);
    bool set_daemon(std::string daemon_address = "http://localhost:8080",
      boost::optional<epee::net_utils::http::login> daemon_login = boost::none, bool trusted_daemon = true,
      epee::net_utils::ssl_options_t ssl_options = epee::net_utils::ssl_support_t::e_ssl_support_autodetect);
    bool set_proxy(const std::string &address);

    void stop() { m_run.store(false, std::memory_order_relaxed); m_message_store.stop(); }

    i_wallet2_callback* callback() const { return m_callback; }
    void callback(i_wallet2_callback* callback) { m_callback = callback; }

    bool is_trusted_daemon() const { return m_trusted_daemon; }
    void set_trusted_daemon(bool trusted) { m_trusted_daemon = trusted; }

    /*!
     * \brief Checks if deterministic wallet
     */
    bool is_deterministic() const;
    bool get_seed(epee::wipeable_string& electrum_words, const epee::wipeable_string &passphrase = epee::wipeable_string()) const;

    /*!
     * \brief Gets the seed language
     */
    const std::string &get_seed_language() const;
    /*!
     * \brief Sets the seed language
     */
    void set_seed_language(const std::string &language);

    // Subaddress scheme
    cryptonote::account_public_address get_subaddress(const cryptonote::subaddress_index& index) const;
    cryptonote::account_public_address get_address() const { return get_subaddress({0,0}); }
    boost::optional<cryptonote::subaddress_index> get_subaddress_index(const cryptonote::account_public_address& address) const;
    crypto::public_key get_subaddress_spend_public_key(const cryptonote::subaddress_index& index) const;
    std::vector<crypto::public_key> get_subaddress_spend_public_keys(uint32_t account, uint32_t begin, uint32_t end) const;
    std::string get_subaddress_as_str(const cryptonote::subaddress_index& index) const;
    std::string get_address_as_str() const { return get_subaddress_as_str({0, 0}); }
    std::string get_integrated_address_as_str(const crypto::hash8& payment_id) const;
    void add_subaddress_account(const std::string& label);
    size_t get_num_subaddress_accounts() const { return m_subaddress_labels.size(); }
    size_t get_num_subaddresses(uint32_t index_major) const { return index_major < m_subaddress_labels.size() ? m_subaddress_labels[index_major].size() : 0; }
    void add_subaddress(uint32_t index_major, const std::string& label); // throws when index is out of bound
    void expand_subaddresses(const cryptonote::subaddress_index& index);
    void create_one_off_subaddress(const cryptonote::subaddress_index& index);
    std::string get_subaddress_label(const cryptonote::subaddress_index& index) const;
    void set_subaddress_label(const cryptonote::subaddress_index &index, const std::string &label);
    void set_subaddress_lookahead(size_t major, size_t minor);
    std::pair<size_t, size_t> get_subaddress_lookahead() const { return {m_subaddress_lookahead_major, m_subaddress_lookahead_minor}; }
    /*!
     * \brief Tells if the wallet file is deprecated.
     */
    bool is_deprecated() const;
    void refresh(bool trusted_daemon);
    void refresh(bool trusted_daemon, uint64_t start_height, uint64_t & blocks_fetched);
    void refresh(bool trusted_daemon, uint64_t start_height, uint64_t & blocks_fetched, bool& received_money, bool check_pool = true, bool try_incremental = true, uint64_t max_blocks = std::numeric_limits<uint64_t>::max());
    bool refresh(bool trusted_daemon, uint64_t & blocks_fetched, bool& received_money, bool& ok);

    void set_refresh_type(RefreshType refresh_type) { m_refresh_type = refresh_type; }
    RefreshType get_refresh_type() const { return m_refresh_type; }

    cryptonote::network_type nettype() const { return m_nettype; }
    bool watch_only() const { return m_watch_only; }
    bool multisig(bool *ready = NULL, uint32_t *threshold = NULL, uint32_t *total = NULL) const;
    bool has_multisig_partial_key_images() const;
    bool has_unknown_key_images() const;
    bool get_multisig_seed(epee::wipeable_string& seed, const epee::wipeable_string &passphrase = std::string()) const;
    bool key_on_device() const { return get_device_type() != hw::device::device_type::SOFTWARE; }
    hw::device::device_type get_device_type() const { return m_key_device_type; }
    bool reconnect_device();

    // locked & unlocked balance of given or current subaddress account
    uint64_t balance(uint32_t subaddr_index_major, bool strict) const;
    uint64_t unlocked_balance(uint32_t subaddr_index_major, bool strict, uint64_t *blocks_to_unlock = NULL, uint64_t *time_to_unlock = NULL);
    // locked & unlocked balance per subaddress of given or current subaddress account
    std::map<uint32_t, uint64_t> balance_per_subaddress(uint32_t subaddr_index_major, bool strict) const;
    std::map<uint32_t, std::pair<uint64_t, std::pair<uint64_t, uint64_t>>> unlocked_balance_per_subaddress(uint32_t subaddr_index_major, bool strict);
    // all locked & unlocked balances of all subaddress accounts
    uint64_t balance_all(bool strict) const;
    uint64_t unlocked_balance_all(bool strict, uint64_t *blocks_to_unlock = NULL, uint64_t *time_to_unlock = NULL);
    template<typename T>
    void transfer_selected(const std::vector<cryptonote::tx_destination_entry>& dsts, const std::vector<size_t>& selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs, std::unordered_set<crypto::public_key> &valid_public_keys_cache,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, T destination_split_strategy, const tx_dust_policy& dust_policy, cryptonote::transaction& tx, pending_tx &ptx, const bool use_view_tags);
    void transfer_selected_rct(std::vector<cryptonote::tx_destination_entry> dsts, const std::vector<size_t>& selected_transfers, size_t fake_outputs_count,
      std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs, std::unordered_set<crypto::public_key> &valid_public_keys_cache,
      uint64_t unlock_time, uint64_t fee, const std::vector<uint8_t>& extra, cryptonote::transaction& tx, pending_tx &ptx, const rct::RCTConfig &rct_config, const bool use_view_tags);

    void commit_tx(pending_tx& ptx_vector);
    void commit_tx(std::vector<pending_tx>& ptx_vector);
    bool save_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename) const;
    std::string dump_tx_to_str(const std::vector<pending_tx> &ptx_vector) const;
    std::string save_multisig_tx(multisig_tx_set txs);
    bool save_multisig_tx(const multisig_tx_set &txs, const std::string &filename);
    std::string save_multisig_tx(const std::vector<pending_tx>& ptx_vector);
    bool save_multisig_tx(const std::vector<pending_tx>& ptx_vector, const std::string &filename);
    multisig_tx_set make_multisig_tx_set(const std::vector<pending_tx>& ptx_vector) const;
    // load unsigned tx from file and sign it. Takes confirmation callback as argument. Used by the cli wallet
    bool sign_tx(const std::string &unsigned_filename, const std::string &signed_filename, std::vector<wallet2::pending_tx> &ptx, std::function<bool(const unsigned_tx_set&)> accept_func = NULL, bool export_raw = false);
    // sign unsigned tx. Takes unsigned_tx_set as argument. Used by GUI
    bool sign_tx(unsigned_tx_set &exported_txs, const std::string &signed_filename, std::vector<wallet2::pending_tx> &ptx, bool export_raw = false);
    bool sign_tx(unsigned_tx_set &exported_txs, std::vector<wallet2::pending_tx> &ptx, signed_tx_set &signed_txs);
    std::string sign_tx_dump_to_str(unsigned_tx_set &exported_txs, std::vector<wallet2::pending_tx> &ptx, signed_tx_set &signed_txes);
    // load unsigned_tx_set from file. 
    bool load_unsigned_tx(const std::string &unsigned_filename, unsigned_tx_set &exported_txs) const;
    bool parse_unsigned_tx_from_str(const std::string &unsigned_tx_st, unsigned_tx_set &exported_txs) const;
    bool load_tx(const std::string &signed_filename, std::vector<tools::wallet2::pending_tx> &ptx, std::function<bool(const signed_tx_set&)> accept_func = NULL);
    bool parse_tx_from_str(const std::string &signed_tx_st, std::vector<tools::wallet2::pending_tx> &ptx, std::function<bool(const signed_tx_set &)> accept_func);
    std::vector<wallet2::pending_tx> create_transactions_2(std::vector<cryptonote::tx_destination_entry> dsts, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices);     // pass subaddr_indices by value on purpose
    std::vector<wallet2::pending_tx> create_transactions_all(uint64_t below, const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra, uint32_t subaddr_account, std::set<uint32_t> subaddr_indices);
    std::vector<wallet2::pending_tx> create_transactions_single(const crypto::key_image &ki, const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra);
    std::vector<wallet2::pending_tx> create_transactions_from(const cryptonote::account_public_address &address, bool is_subaddress, const size_t outputs, std::vector<size_t> unused_transfers_indices, std::vector<size_t> unused_dust_indices, const size_t fake_outs_count, const uint64_t unlock_time, uint32_t priority, const std::vector<uint8_t>& extra);
    bool sanity_check(const std::vector<wallet2::pending_tx> &ptx_vector, std::vector<cryptonote::tx_destination_entry> dsts) const;
    void cold_tx_aux_import(const std::vector<pending_tx>& ptx, const std::vector<std::string>& tx_device_aux);
    void cold_sign_tx(const std::vector<pending_tx>& ptx_vector, signed_tx_set &exported_txs, std::vector<cryptonote::address_parse_info> &dsts_info, std::vector<std::string> & tx_device_aux);
    uint64_t cold_key_image_sync(uint64_t &spent, uint64_t &unspent);
    void device_show_address(uint32_t account_index, uint32_t address_index, const boost::optional<crypto::hash8> &payment_id);
    bool parse_multisig_tx_from_str(std::string multisig_tx_st, multisig_tx_set &exported_txs) const;
    bool load_multisig_tx(cryptonote::blobdata blob, multisig_tx_set &exported_txs, std::function<bool(const multisig_tx_set&)> accept_func = NULL);
    bool load_multisig_tx_from_file(const std::string &filename, multisig_tx_set &exported_txs, std::function<bool(const multisig_tx_set&)> accept_func = NULL);
    bool sign_multisig_tx_from_file(const std::string &filename, std::vector<crypto::hash> &txids, std::function<bool(const multisig_tx_set&)> accept_func);
    bool sign_multisig_tx(multisig_tx_set &exported_txs, std::vector<crypto::hash> &txids);
    bool sign_multisig_tx_to_file(multisig_tx_set &exported_txs, const std::string &filename, std::vector<crypto::hash> &txids);
    std::vector<pending_tx> create_unmixable_sweep_transactions();
    void discard_unmixable_outputs();
    bool check_connection(uint32_t *version = NULL, bool *ssl = NULL, uint32_t timeout = 200000, bool *wallet_is_outdated = NULL, bool *daemon_is_outdated = NULL);
    bool check_version(uint32_t *version, bool *wallet_is_outdated, bool *daemon_is_outdated);
    bool check_hard_fork_version(cryptonote::network_type nettype, const std::vector<std::pair<uint8_t, uint64_t>> &daemon_hard_forks, const uint64_t height, const uint64_t target_height, bool *wallet_is_outdated, bool *daemon_is_outdated);
    void get_transfers(wallet2::transfer_container& incoming_transfers) const;
    void get_payments(const crypto::hash& payment_id, std::list<wallet2::payment_details>& payments, uint64_t min_height = 0, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const;
    void get_payments(std::list<std::pair<crypto::hash,wallet2::payment_details>>& payments, uint64_t min_height, uint64_t max_height = (uint64_t)-1, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const;
    void get_payments_out(std::list<std::pair<crypto::hash,wallet2::confirmed_transfer_details>>& confirmed_payments,
      uint64_t min_height, uint64_t max_height = (uint64_t)-1, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const;
    void get_unconfirmed_payments_out(std::list<std::pair<crypto::hash,wallet2::unconfirmed_transfer_details>>& unconfirmed_payments, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const;
    void get_unconfirmed_payments(std::list<std::pair<crypto::hash,wallet2::pool_payment_details>>& unconfirmed_payments, const boost::optional<uint32_t>& subaddr_account = boost::none, const std::set<uint32_t>& subaddr_indices = {}) const;

    uint64_t get_blockchain_current_height() const { return m_blockchain.size(); }
    void rescan_spent();
    void rescan_blockchain(bool hard, bool refresh = true, bool keep_key_images = false);
    bool is_transfer_unlocked(const transfer_details& td);
    bool is_transfer_unlocked(uint64_t unlock_time, uint64_t block_height);

    uint64_t get_last_block_reward() const { return m_last_block_reward; }
    uint64_t get_device_last_key_image_sync() const { return m_device_last_key_image_sync; }

    std::vector<cryptonote::public_node> get_public_nodes(bool white_only = true);

    template <class t_archive>
    inline void serialize(t_archive &a, const unsigned int ver)
    {
      uint64_t dummy_refresh_height = 0; // moved to keys file
      if(ver < 5)
        return;
      if (ver < 19)
      {
        std::vector<crypto::hash> blockchain;
        a & blockchain;
        m_blockchain.clear();
        for (const auto &b: blockchain)
        {
          m_blockchain.push_back(b);
        }
      }
      else
      {
        a & m_blockchain;
      }
      a & m_transfers;
      a & m_account_public_address;
      a & m_key_images.parent();
      if(ver < 6)
        return;
      a & m_unconfirmed_txs.parent();
      if(ver < 7)
        return;
      a & m_payments.parent();
      if(ver < 8)
        return;
      a & m_tx_keys.parent();
      if(ver < 9)
        return;
      a & m_confirmed_txs.parent();
      if(ver < 11)
        return;
      a & dummy_refresh_height;
      if(ver < 12)
        return;
      a & m_tx_notes.parent();
      if(ver < 13)
        return;
      if (ver < 17)
      {
        // we're loading an old version, where m_unconfirmed_payments was a std::map
        std::unordered_map<crypto::hash, payment_details> m;
        a & m;
        m_unconfirmed_payments.clear();
        for (std::unordered_map<crypto::hash, payment_details>::const_iterator i = m.begin(); i != m.end(); ++i)
          m_unconfirmed_payments.insert(std::make_pair(i->first, pool_payment_details{i->second, false}));
      }
      if(ver < 14)
        return;
      if(ver < 15)
      {
        // we're loading an older wallet without a pubkey map, rebuild it
        m_pub_keys.clear();
        for (size_t i = 0; i < m_transfers.size(); ++i)
        {
          const transfer_details &td = m_transfers[i];
          m_pub_keys.emplace(td.get_public_key(), i);
        }
        return;
      }
      a & m_pub_keys.parent();
      if(ver < 16)
        return;
      a & m_address_book;
      if(ver < 17)
        return;
      if (ver < 22)
      {
        // we're loading an old version, where m_unconfirmed_payments payload was payment_details
        std::unordered_multimap<crypto::hash, payment_details> m;
        a & m;
        m_unconfirmed_payments.clear();
        for (const auto &i: m)
          m_unconfirmed_payments.insert(std::make_pair(i.first, pool_payment_details{i.second, false}));
      }
      if(ver < 18)
        return;
      a & m_scanned_pool_txs[0];
      a & m_scanned_pool_txs[1];
      if (ver < 20)
        return;
      a & m_subaddresses.parent();
      std::unordered_map<cryptonote::subaddress_index, crypto::public_key> dummy_subaddresses_inv;
      a & dummy_subaddresses_inv;
      a & m_subaddress_labels;
      a & m_additional_tx_keys.parent();
      if(ver < 21)
        return;
      a & m_attributes.parent();
      if(ver < 22)
        return;
      a & m_unconfirmed_payments.parent();
      if(ver < 23)
        return;
      a & (std::pair<std::map<std::string, std::string>, std::vector<std::string>>&)m_account_tags;
      if(ver < 24)
        return;
      a & m_ring_history_saved;
      if(ver < 25)
        return;
      a & m_last_block_reward;
      if(ver < 26)
        return;
      a & m_tx_device.parent();
      if(ver < 27)
        return;
      a & m_device_last_key_image_sync;
      if(ver < 28)
        return;
      a & m_cold_key_images.parent();
      if(ver < 29)
        return;
      crypto::secret_key dummy_rpc_client_secret_key; // Compatibility for old RPC payment system
      a & dummy_rpc_client_secret_key;
      if(ver < 30)
      {
        m_has_ever_refreshed_from_node = false;
        return;
      }
      a & m_has_ever_refreshed_from_node;
    }

    BEGIN_SERIALIZE_OBJECT()
      MAGIC_FIELD("monero wallet cache")
      VERSION_FIELD(1)
      FIELD(m_blockchain)
      FIELD(m_transfers)
      FIELD(m_account_public_address)
      FIELD(m_key_images)
      FIELD(m_unconfirmed_txs)
      FIELD(m_payments)
      FIELD(m_tx_keys)
      FIELD(m_confirmed_txs)
      FIELD(m_tx_notes)
      FIELD(m_unconfirmed_payments)
      FIELD(m_pub_keys)
      FIELD(m_address_book)
      FIELD(m_scanned_pool_txs[0])
      FIELD(m_scanned_pool_txs[1])
      FIELD(m_subaddresses)
      FIELD(m_subaddress_labels)
      FIELD(m_additional_tx_keys)
      FIELD(m_attributes)
      FIELD(m_account_tags)
      FIELD(m_ring_history_saved)
      FIELD(m_last_block_reward)
      FIELD(m_tx_device)
      FIELD(m_device_last_key_image_sync)
      FIELD(m_cold_key_images)
      crypto::secret_key dummy_rpc_client_secret_key; // Compatibility for old RPC payment system
      FIELD_N("m_rpc_client_secret_key", dummy_rpc_client_secret_key)
      if (version < 1)
      {
        m_has_ever_refreshed_from_node = false;
        return true;
      }
      FIELD(m_has_ever_refreshed_from_node)
    END_SERIALIZE()

    /*!
     * \brief  Check if wallet keys and bin files exist
     * \param  file_path           Wallet file path
     * \param  keys_file_exists    Whether keys file exists
     * \param  wallet_file_exists  Whether bin file exists
     */
    static void wallet_exists(const std::string& file_path, bool& keys_file_exists, bool& wallet_file_exists);
    /*!
     * \brief  Check if wallet file path is valid format
     * \param  file_path      Wallet file path
     * \return                Whether path is valid format
     */
    static bool wallet_valid_path_format(const std::string& file_path);
    static bool parse_long_payment_id(const std::string& payment_id_str, crypto::hash& payment_id);
    static bool parse_short_payment_id(const std::string& payment_id_str, crypto::hash8& payment_id);
    static bool parse_payment_id(const std::string& payment_id_str, crypto::hash& payment_id);

    bool always_confirm_transfers() const { return m_always_confirm_transfers; }
    void always_confirm_transfers(bool always) { m_always_confirm_transfers = always; }
    bool print_ring_members() const { return m_print_ring_members; }
    void print_ring_members(bool value) { m_print_ring_members = value; }
    bool store_tx_info() const { return m_store_tx_info; }
    void store_tx_info(bool store) { m_store_tx_info = store; }
    uint32_t default_mixin() const { return m_default_mixin; }
    void default_mixin(uint32_t m) { m_default_mixin = m; }
    uint32_t get_default_priority() const { return m_default_priority; }
    void set_default_priority(uint32_t p) { m_default_priority = p; }
    bool auto_refresh() const { return m_auto_refresh; }
    void auto_refresh(bool r) { m_auto_refresh = r; }
    AskPasswordType ask_password() const { return m_ask_password; }
    void ask_password(AskPasswordType ask) { m_ask_password = ask; }
    void set_min_output_count(uint32_t count) { m_min_output_count = count; }
    uint32_t get_min_output_count() const { return m_min_output_count; }
    void set_min_output_value(uint64_t value) { m_min_output_value = value; }
    uint64_t get_min_output_value() const { return m_min_output_value; }
    void merge_destinations(bool merge) { m_merge_destinations = merge; }
    bool merge_destinations() const { return m_merge_destinations; }
    bool confirm_backlog() const { return m_confirm_backlog; }
    void confirm_backlog(bool always) { m_confirm_backlog = always; }
    void set_confirm_backlog_threshold(uint32_t threshold) { m_confirm_backlog_threshold = threshold; };
    uint32_t get_confirm_backlog_threshold() const { return m_confirm_backlog_threshold; };
    bool confirm_export_overwrite() const { return m_confirm_export_overwrite; }
    void confirm_export_overwrite(bool always) { m_confirm_export_overwrite = always; }
    bool auto_low_priority() const { return m_auto_low_priority; }
    void auto_low_priority(bool value) { m_auto_low_priority = value; }
    bool segregate_pre_fork_outputs() const { return m_segregate_pre_fork_outputs; }
    void segregate_pre_fork_outputs(bool value) { m_segregate_pre_fork_outputs = value; }
    bool key_reuse_mitigation2() const { return m_key_reuse_mitigation2; }
    void key_reuse_mitigation2(bool value) { m_key_reuse_mitigation2 = value; }
    uint64_t segregation_height() const { return m_segregation_height; }
    void segregation_height(uint64_t height) { m_segregation_height = height; }
    bool ignore_fractional_outputs() const { return m_ignore_fractional_outputs; }
    void ignore_fractional_outputs(bool value) { m_ignore_fractional_outputs = value; }
    uint64_t ignore_outputs_above() const { return m_ignore_outputs_above; }
    void ignore_outputs_above(uint64_t value) { m_ignore_outputs_above = value; }
    uint64_t ignore_outputs_below() const { return m_ignore_outputs_below; }
    void ignore_outputs_below(uint64_t value) { m_ignore_outputs_below = value; }
    bool track_uses() const { return m_track_uses; }
    void track_uses(bool value) { m_track_uses = value; }
    bool show_wallet_name_when_locked() const { return m_show_wallet_name_when_locked; }
    void show_wallet_name_when_locked(bool value) { m_show_wallet_name_when_locked = value; }
    BackgroundMiningSetupType setup_background_mining() const { return m_setup_background_mining; }
    void setup_background_mining(BackgroundMiningSetupType value) { m_setup_background_mining = value; }
    uint32_t inactivity_lock_timeout() const { return m_inactivity_lock_timeout; }
    void inactivity_lock_timeout(uint32_t seconds) { m_inactivity_lock_timeout = seconds; }
    const std::string & device_name() const { return m_device_name; }
    void device_name(const std::string & device_name) { m_device_name = device_name; }
    const std::string & device_derivation_path() const { return m_device_derivation_path; }
    void device_derivation_path(const std::string &device_derivation_path) { m_device_derivation_path = device_derivation_path; }
    const ExportFormat & export_format() const { return m_export_format; }
    inline void set_export_format(const ExportFormat& export_format) { m_export_format = export_format; }
    bool load_deprecated_formats() const { return m_load_deprecated_formats; }
    void load_deprecated_formats(bool load) { m_load_deprecated_formats = load; }
    bool is_multisig_enabled() const { return m_enable_multisig; }
    void enable_multisig(bool enable) { m_enable_multisig = enable; }
    bool is_mismatched_daemon_version_allowed() const { return m_allow_mismatched_daemon_version; }
    void allow_mismatched_daemon_version(bool allow_mismatch) { m_allow_mismatched_daemon_version = allow_mismatch; }

    bool get_tx_key_cached(const crypto::hash &txid, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys) const;
    void set_tx_key(const crypto::hash &txid, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const boost::optional<cryptonote::account_public_address> &single_destination_subaddress = boost::none);
    bool get_tx_key(const crypto::hash &txid, crypto::secret_key &tx_key, std::vector<crypto::secret_key> &additional_tx_keys);
    void check_tx_key(const crypto::hash &txid, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const cryptonote::account_public_address &address, uint64_t &received, bool &in_pool, uint64_t &confirmations);
    void check_tx_key_helper(const crypto::hash &txid, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, const cryptonote::account_public_address &address, uint64_t &received, bool &in_pool, uint64_t &confirmations);
    void check_tx_key_helper(const cryptonote::transaction &tx, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, const cryptonote::account_public_address &address, uint64_t &received) const;
    bool is_out_to_acc(const cryptonote::account_public_address &address, const crypto::public_key& out_key, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, const size_t output_index, const boost::optional<crypto::view_tag> &view_tag_opt, crypto::key_derivation &found_derivation) const;
    std::string get_tx_proof(const crypto::hash &txid, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message);
    std::string get_tx_proof(const cryptonote::transaction &tx, const crypto::secret_key &tx_key, const std::vector<crypto::secret_key> &additional_tx_keys, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message) const;
    bool check_tx_proof(const crypto::hash &txid, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message, const std::string &sig_str, uint64_t &received, bool &in_pool, uint64_t &confirmations);
    bool check_tx_proof(const cryptonote::transaction &tx, const cryptonote::account_public_address &address, bool is_subaddress, const std::string &message, const std::string &sig_str, uint64_t &received) const;

    std::string get_spend_proof(const crypto::hash &txid, const std::string &message);
    bool check_spend_proof(const crypto::hash &txid, const std::string &message, const std::string &sig_str);

    void scan_tx(const std::unordered_set<crypto::hash> &txids);

    /*!
     * \brief  Generates a proof that proves the reserve of unspent funds
     * \param  account_minreserve       When specified, collect outputs only belonging to the given account and prove the smallest reserve above the given amount
     *                                  When unspecified, proves for all unspent outputs across all accounts
     * \param  message                  Arbitrary challenge message to be signed together
     * \return                          Signature string
     */
    std::string get_reserve_proof(const boost::optional<std::pair<uint32_t, uint64_t>> &account_minreserve, const std::string &message);
    /*!
     * \brief  Verifies a proof of reserve
     * \param  address                  The signer's address
     * \param  message                  Challenge message used for signing
     * \param  sig_str                  Signature string
     * \param  total                    [OUT] the sum of funds included in the signature
     * \param  spent                    [OUT] the sum of spent funds included in the signature
     * \return                          true if the signature verifies correctly
     */
    bool check_reserve_proof(const cryptonote::account_public_address &address, const std::string &message, const std::string &sig_str, uint64_t &total, uint64_t &spent);

   /*!
    * \brief GUI Address book get/store
    */
    std::vector<address_book_row> get_address_book() const { return m_address_book; }
    bool add_address_book_row(const cryptonote::account_public_address &address, const crypto::hash8 *payment_id, const std::string &description, bool is_subaddress);
    bool set_address_book_row(size_t row_id, const cryptonote::account_public_address &address, const crypto::hash8 *payment_id, const std::string &description, bool is_subaddress);
    bool delete_address_book_row(std::size_t row_id);
        
    uint64_t get_num_rct_outputs();
    size_t get_num_transfer_details() const { return m_transfers.size(); }
    const transfer_details &get_transfer_details(size_t idx) const;

    uint8_t get_current_hard_fork();
    void get_hard_fork_info(uint8_t version, uint64_t &earliest_height);
    bool use_fork_rules(uint8_t version, int64_t early_blocks = 0);
    int get_fee_algorithm();

    std::string get_wallet_file() const;
    std::string get_keys_file() const;
    std::string get_daemon_address() const;
    const boost::optional<epee::net_utils::http::login>& get_daemon_login() const { return m_daemon_login; }
    uint64_t get_daemon_blockchain_height(std::string& err);
    uint64_t get_daemon_blockchain_target_height(std::string& err);
    uint64_t get_daemon_adjusted_time();

   /*!
    * \brief Calculates the approximate blockchain height from current date/time.
    */
    uint64_t get_approximate_blockchain_height() const;
    uint64_t estimate_blockchain_height();
    std::vector<size_t> select_available_outputs_from_histogram(uint64_t count, bool atleast, bool unlocked, bool allow_rct);
    std::vector<size_t> select_available_outputs(const std::function<bool(const transfer_details &td)> &f);
    std::vector<size_t> select_available_unmixable_outputs();
    std::vector<size_t> select_available_mixable_outputs();

    size_t pop_best_value_from(const transfer_container &transfers, std::vector<size_t> &unused_dust_indices, const std::vector<size_t>& selected_transfers, bool smallest = false) const;
    size_t pop_best_value(std::vector<size_t> &unused_dust_indices, const std::vector<size_t>& selected_transfers, bool smallest = false) const;

    void set_tx_note(const crypto::hash &txid, const std::string &note);
    std::string get_tx_note(const crypto::hash &txid) const;

    void set_tx_device_aux(const crypto::hash &txid, const std::string &aux);
    std::string get_tx_device_aux(const crypto::hash &txid) const;

    void set_description(const std::string &description);
    std::string get_description() const;

    /*!
     * \brief  Get the list of registered account tags. 
     * \return first.Key=(tag's name), first.Value=(tag's label), second[i]=(i-th account's tag)
     */
    const std::pair<serializable_map<std::string, std::string>, std::vector<std::string>>& get_account_tags();
    /*!
     * \brief  Set a tag to the given accounts.
     * \param  account_indices  Indices of accounts.
     * \param  tag              Tag's name. If empty, the accounts become untagged.
     */
    void set_account_tag(const std::set<uint32_t> &account_indices, const std::string& tag);
    /*!
     * \brief  Set the label of the given tag.
     * \param  tag            Tag's name (which must be non-empty).
     * \param  description    Tag's description.
     */
    void set_account_tag_description(const std::string& tag, const std::string& description);

    enum message_signature_type_t { sign_with_spend_key, sign_with_view_key };
    std::string sign(const std::string &data, message_signature_type_t signature_type, cryptonote::subaddress_index index = {0, 0}) const;
    struct message_signature_result_t { bool valid; unsigned version; bool old; message_signature_type_t type; };
    message_signature_result_t verify(const std::string &data, const cryptonote::account_public_address &address, const std::string &signature) const;

    /*!
     * \brief sign_multisig_participant signs given message with the multisig public signer key
     * \param data                      message to sign
     * \throws                          if wallet is not multisig
     * \return                          signature
     */
    std::string sign_multisig_participant(const std::string& data) const;
    /*!
     * \brief verify_with_public_key verifies message was signed with given public key
     * \param data                   message
     * \param public_key             public key to check signature
     * \param signature              signature of the message
     * \return                       true if the signature is correct
     */
    bool verify_with_public_key(const std::string &data, const crypto::public_key &public_key, const std::string &signature) const;

    // Import/Export wallet data
    std::tuple<uint64_t, uint64_t, std::vector<tools::wallet2::exported_transfer_details>> export_outputs(bool all = false, uint32_t start = 0, uint32_t count = 0xffffffff) const;
    std::string export_outputs_to_str(bool all = false, uint32_t start = 0, uint32_t count = 0xffffffff) const;
    size_t import_outputs(const std::tuple<uint64_t, uint64_t, std::vector<tools::wallet2::exported_transfer_details>> &outputs);
    size_t import_outputs(const std::tuple<uint64_t, uint64_t, std::vector<tools::wallet2::transfer_details>> &outputs);
    size_t import_outputs_from_str(const std::string &outputs_st);
    payment_container export_payments() const;
    void import_payments(const payment_container &payments);
    void import_payments_out(const std::list<std::pair<crypto::hash,wallet2::confirmed_transfer_details>> &confirmed_payments);
    std::tuple<size_t, crypto::hash, std::vector<crypto::hash>> export_blockchain() const;
    void import_blockchain(const std::tuple<size_t, crypto::hash, std::vector<crypto::hash>> &bc);
    bool export_key_images(const std::string &filename, bool all = false) const;
    std::pair<uint64_t, std::vector<std::pair<crypto::key_image, crypto::signature>>> export_key_images(bool all = false) const;
    uint64_t import_key_images(const std::vector<std::pair<crypto::key_image, crypto::signature>> &signed_key_images, size_t offset, uint64_t &spent, uint64_t &unspent, bool check_spent = true);
    uint64_t import_key_images(const std::string &filename, uint64_t &spent, uint64_t &unspent);
    bool import_key_images(std::vector<crypto::key_image> key_images, size_t offset=0, boost::optional<std::unordered_set<size_t>> selected_transfers=boost::none);
    bool import_key_images(signed_tx_set & signed_tx, size_t offset=0, bool only_selected_transfers=false);
    crypto::public_key get_tx_pub_key_from_received_outs(const tools::wallet2::transfer_details &td) const;

    void update_pool_state(std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &process_txs, bool refreshed = false, bool try_incremental = false);
    void process_pool_state(const std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &txs);
    void remove_obsolete_pool_txs(const std::vector<crypto::hash> &tx_hashes, bool remove_if_found);

    std::string encrypt(const char *plaintext, size_t len, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string encrypt(const epee::span<char> &span, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string encrypt(const std::string &plaintext, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string encrypt(const epee::wipeable_string &plaintext, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string encrypt_with_view_secret_key(const std::string &plaintext, bool authenticated = true) const;
    template<typename T=std::string> T decrypt(const std::string &ciphertext, const crypto::secret_key &skey, bool authenticated = true) const;
    std::string decrypt_with_view_secret_key(const std::string &ciphertext, bool authenticated = true) const;

    std::string make_uri(const std::string &address, const std::string &payment_id, uint64_t amount, const std::string &tx_description, const std::string &recipient_name, std::string &error) const;
    bool parse_uri(const std::string &uri, std::string &address, std::string &payment_id, uint64_t &amount, std::string &tx_description, std::string &recipient_name, std::vector<std::string> &unknown_parameters, std::string &error);

    uint64_t get_blockchain_height_by_date(uint16_t year, uint8_t month, uint8_t day);    // 1<=month<=12, 1<=day<=31

    bool is_synced();

    std::vector<std::pair<uint64_t, uint64_t>> estimate_backlog(const std::vector<std::pair<double, double>> &fee_levels);
    std::vector<std::pair<uint64_t, uint64_t>> estimate_backlog(uint64_t min_tx_weight, uint64_t max_tx_weight, const std::vector<uint64_t> &fees);

    uint64_t estimate_fee(bool use_per_byte_fee, bool use_rct, int n_inputs, int mixin, int n_outputs, size_t extra_size, bool bulletproof, bool clsag, bool bulletproof_plus, bool use_view_tags, uint64_t base_fee, uint64_t fee_quantization_mask) const;
    uint64_t get_fee_multiplier(uint32_t priority, int fee_algorithm = -1);
    uint64_t get_base_fee(uint32_t priority);
    uint64_t get_base_fee();
    uint64_t get_fee_quantization_mask();
    uint64_t get_min_ring_size();
    uint64_t get_max_ring_size();
    uint64_t adjust_mixin(uint64_t mixin);

    uint32_t adjust_priority(uint32_t priority);

    bool is_unattended() const { return m_unattended; }

    std::pair<size_t, uint64_t> estimate_tx_size_and_weight(bool use_rct, int n_inputs, int ring_size, int n_outputs, size_t extra_size);


    /*
     * "attributes" are a mechanism to store an arbitrary number of string values
     * on the level of the wallet as a whole, identified by keys. Their introduction,
     * technically the unordered map m_attributes stored as part of a wallet file,
     * led to a new wallet file version, but now new singular pieces of info may be added
     * without the need for a new version.
     *
     * The first and so far only value stored as such an attribute is the description.
     * It's stored under the standard key ATTRIBUTE_DESCRIPTION (see method set_description).
     *
     * The mechanism is open to all clients and allows them to use it for storing basically any
     * single string values in a wallet. To avoid the problem that different clients possibly
     * overwrite or misunderstand each other's attributes, a two-part key scheme is
     * proposed: <client name>.<value name>
     */
    const char* const ATTRIBUTE_DESCRIPTION = "wallet2.description";
    void set_attribute(const std::string &key, const std::string &value);
    bool get_attribute(const std::string &key, std::string &value) const;

    crypto::public_key get_multisig_signer_public_key() const;
    crypto::public_key get_multisig_signing_public_key(size_t idx) const;
    crypto::public_key get_multisig_signing_public_key(const crypto::secret_key &skey) const;

    template<class t_request, class t_response>
    inline bool invoke_http_json(const boost::string_ref uri, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST")
    {
      if (m_offline) return false;
      boost::lock_guard<boost::recursive_mutex> lock(m_daemon_rpc_mutex);
      return epee::net_utils::invoke_http_json(uri, req, res, *m_http_client, timeout, http_method);
    }
    template<class t_request, class t_response>
    inline bool invoke_http_bin(const boost::string_ref uri, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST")
    {
      if (m_offline) return false;
      boost::lock_guard<boost::recursive_mutex> lock(m_daemon_rpc_mutex);
      return epee::net_utils::invoke_http_bin(uri, req, res, *m_http_client, timeout, http_method);
    }
    template<class t_request, class t_response>
    inline bool invoke_http_json_rpc(const boost::string_ref uri, const std::string& method_name, const t_request& req, t_response& res, std::chrono::milliseconds timeout = std::chrono::seconds(15), const boost::string_ref http_method = "POST", const std::string& req_id = "0")
    {
      if (m_offline) return false;
      boost::lock_guard<boost::recursive_mutex> lock(m_daemon_rpc_mutex);
      return epee::net_utils::invoke_http_json_rpc(uri, method_name, req, res, *m_http_client, timeout, http_method, req_id);
    }

    bool set_ring_database(const std::string &filename);
    const std::string get_ring_database() const { return m_ring_database; }
    bool get_ring(const crypto::key_image &key_image, std::vector<uint64_t> &outs);
    bool get_rings(const crypto::hash &txid, std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> &outs);
    bool get_rings(const crypto::chacha_key &key, const std::vector<crypto::key_image> &key_images, std::vector<std::vector<uint64_t>> &outs);
    bool set_ring(const crypto::key_image &key_image, const std::vector<uint64_t> &outs, bool relative);
    bool set_rings(const std::vector<std::pair<crypto::key_image, std::vector<uint64_t>>> &rings, bool relative);
    bool unset_ring(const std::vector<crypto::key_image> &key_images);
    bool unset_ring(const crypto::hash &txid);
    bool find_and_save_rings(bool force = true);

    bool blackball_output(const std::pair<uint64_t, uint64_t> &output);
    bool set_blackballed_outputs(const std::vector<std::pair<uint64_t, uint64_t>> &outputs, bool add = false);
    bool unblackball_output(const std::pair<uint64_t, uint64_t> &output);
    bool is_output_blackballed(const std::pair<uint64_t, uint64_t> &output) const;

    void freeze(size_t idx);
    void thaw(size_t idx);
    bool frozen(size_t idx) const;
    void freeze(const crypto::key_image &ki);
    void thaw(const crypto::key_image &ki);
    bool frozen(const crypto::key_image &ki) const;
    bool frozen(const transfer_details &td) const;
    bool frozen(const multisig_tx_set& txs) const; // does partially signed txset contain frozen enotes?

    bool save_to_file(const std::string& path_to_file, const std::string& binary, bool is_printable = false) const;
    static bool load_from_file(const std::string& path_to_file, std::string& target_str, size_t max_size = 1000000000);

    uint64_t get_bytes_sent() const;
    uint64_t get_bytes_received() const;

    // MMS -------------------------------------------------------------------------------------------------
    mms::message_store& get_message_store() { return m_message_store; };
    const mms::message_store& get_message_store() const { return m_message_store; };
    mms::multisig_wallet_state get_multisig_wallet_state() const;

    bool lock_keys_file();
    bool unlock_keys_file();
    bool is_keys_file_locked() const;

    void change_password(const std::string &filename, const epee::wipeable_string &original_password, const epee::wipeable_string &new_password);

    void set_tx_notify(const std::shared_ptr<tools::Notify> &notify) { m_tx_notify = notify; }

    bool is_tx_spendtime_unlocked(uint64_t unlock_time, uint64_t block_height);
    void hash_m_transfer(const transfer_details & transfer, crypto::hash &hash) const;
    uint64_t hash_m_transfers(boost::optional<uint64_t> transfer_height, crypto::hash &hash) const;
    void finish_rescan_bc_keep_key_images(uint64_t transfer_height, const crypto::hash &hash);
    void enable_dns(bool enable) { m_use_dns = enable; }
    void set_offline(bool offline = true);
    bool is_offline() const { return m_offline; }

    static std::string get_default_daemon_address() { CRITICAL_REGION_LOCAL(default_daemon_address_lock); return default_daemon_address; }

  private:
    /*!
     * \brief  Stores wallet information to wallet file.
     * \param  keys_file_name Name of wallet file
     * \param  password       Password of wallet file
     * \param  watch_only     true to save only view key, false to save both spend and view keys
     * \return                Whether it was successful.
     */
    bool store_keys(const std::string& keys_file_name, const epee::wipeable_string& password, bool watch_only = false);
    /*!
     * \brief Load wallet keys information from wallet file.
     * \param keys_file_name Name of wallet file
     * \param password       Password of wallet file
     */
    bool load_keys(const std::string& keys_file_name, const epee::wipeable_string& password);
    /*!
     * \brief Load wallet keys information from a string buffer.
     * \param keys_buf       Keys buffer to load
     * \param password       Password of keys buffer
     */
    bool load_keys_buf(const std::string& keys_buf, const epee::wipeable_string& password);
    bool load_keys_buf(const std::string& keys_buf, const epee::wipeable_string& password, boost::optional<crypto::chacha_key>& keys_to_encrypt);
    void process_new_transaction(const crypto::hash &txid, const cryptonote::transaction& tx, const std::vector<uint64_t> &o_indices, uint64_t height, uint8_t block_version, uint64_t ts, bool miner_tx, bool pool, bool double_spend_seen, const tx_cache_data &tx_cache_data, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache = NULL, bool ignore_callbacks = false);
    bool should_skip_block(const cryptonote::block &b, uint64_t height) const;
    void process_new_blockchain_entry(const cryptonote::block& b, const cryptonote::block_complete_entry& bche, const parsed_block &parsed_block, const crypto::hash& bl_id, uint64_t height, const std::vector<tx_cache_data> &tx_cache_data, size_t tx_cache_data_offset, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache = NULL);
    detached_blockchain_data detach_blockchain(uint64_t height, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache = NULL);
    void handle_reorg(uint64_t height, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache = NULL);
    void get_short_chain_history(std::list<crypto::hash>& ids, uint64_t granularity = 1) const;
    bool clear();
    void clear_soft(bool keep_key_images=false);
    void pull_blocks(bool first, bool try_incremental, uint64_t start_height, uint64_t& blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::vector<cryptonote::block_complete_entry> &blocks, std::vector<cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::block_output_indices> &o_indices, uint64_t &current_height, std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>>& process_pool_txs);
    void pull_hashes(uint64_t start_height, uint64_t& blocks_start_height, const std::list<crypto::hash> &short_chain_history, std::vector<crypto::hash> &hashes);
    void fast_refresh(uint64_t stop_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history, bool force = false);
    void pull_and_parse_next_blocks(bool first, bool try_incremental, uint64_t start_height, uint64_t &blocks_start_height, std::list<crypto::hash> &short_chain_history, const std::vector<cryptonote::block_complete_entry> &prev_blocks, const std::vector<parsed_block> &prev_parsed_blocks, std::vector<cryptonote::block_complete_entry> &blocks, std::vector<parsed_block> &parsed_blocks, std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>>& process_pool_txs, bool &last, bool &error, std::exception_ptr &exception);
    void process_parsed_blocks(uint64_t start_height, const std::vector<cryptonote::block_complete_entry> &blocks, const std::vector<parsed_block> &parsed_blocks, uint64_t& blocks_added, std::map<std::pair<uint64_t, uint64_t>, size_t> *output_tracker_cache = NULL);
    bool accept_pool_tx_for_processing(const crypto::hash &txid);
    void process_unconfirmed_transfer(bool incremental, const crypto::hash &txid, wallet2::unconfirmed_transfer_details &tx_details, bool seen_in_pool, std::chrono::system_clock::time_point now, bool refreshed);
    void process_pool_info_extent(const cryptonote::COMMAND_RPC_GET_BLOCKS_FAST::response &res, std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &process_txs, bool refreshed);
    void update_pool_state_by_pool_query(std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &process_txs, bool refreshed = false);
    void update_pool_state_from_pool_data(bool incremental, const std::vector<crypto::hash> &removed_pool_txids, const std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &added_pool_txs, std::vector<std::tuple<cryptonote::transaction, crypto::hash, bool>> &process_txs, bool refreshed);
    uint64_t select_transfers(uint64_t needed_money, std::vector<size_t> unused_transfers_indices, std::vector<size_t>& selected_transfers) const;
    bool prepare_file_names(const std::string& file_path);
    void process_unconfirmed(const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t height);
    void process_outgoing(const crypto::hash &txid, const cryptonote::transaction& tx, uint64_t height, uint64_t ts, uint64_t spent, uint64_t received, uint32_t subaddr_account, const std::set<uint32_t>& subaddr_indices);
    void add_unconfirmed_tx(const cryptonote::transaction& tx, uint64_t amount_in, const std::vector<cryptonote::tx_destination_entry> &dests, const crypto::hash &payment_id, uint64_t change_amount, uint32_t subaddr_account, const std::set<uint32_t>& subaddr_indices);
    void generate_genesis(cryptonote::block& b) const;
    void check_genesis(const crypto::hash& genesis_hash) const; //throws
    bool generate_chacha_key_from_secret_keys(crypto::chacha_key &key) const;
    void generate_chacha_key_from_password(const epee::wipeable_string &pass, crypto::chacha_key &key) const;
    crypto::hash get_payment_id(const pending_tx &ptx) const;
    void check_acc_out_precomp(const cryptonote::tx_out &o, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, size_t i, tx_scan_info_t &tx_scan_info) const;
    void check_acc_out_precomp(const cryptonote::tx_out &o, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, size_t i, const is_out_data *is_out_data, tx_scan_info_t &tx_scan_info) const;
    void check_acc_out_precomp_once(const cryptonote::tx_out &o, const crypto::key_derivation &derivation, const std::vector<crypto::key_derivation> &additional_derivations, size_t i, const is_out_data *is_out_data, tx_scan_info_t &tx_scan_info, bool &already_seen) const;
    void parse_block_round(const cryptonote::blobdata &blob, cryptonote::block &bl, crypto::hash &bl_id, bool &error) const;
    uint64_t get_upper_transaction_weight_limit();
    std::vector<uint64_t> get_unspent_amounts_vector(bool strict);
    uint64_t get_dynamic_base_fee_estimate();
    float get_output_relatedness(const transfer_details &td0, const transfer_details &td1) const;
    std::vector<size_t> pick_preferred_rct_inputs(uint64_t needed_money, uint32_t subaddr_account, const std::set<uint32_t> &subaddr_indices);
    void set_spent(size_t idx, uint64_t height);
    void set_unspent(size_t idx);
    bool is_spent(const transfer_details &td, bool strict = true) const;
    bool is_spent(size_t idx, bool strict = true) const;
    void get_outs(std::vector<std::vector<get_outs_entry>> &outs, const std::vector<size_t> &selected_transfers, size_t fake_outputs_count, bool rct, std::unordered_set<crypto::public_key> &valid_public_keys_cache);
    void get_outs(std::vector<std::vector<get_outs_entry>> &outs, const std::vector<size_t> &selected_transfers, size_t fake_outputs_count, std::vector<uint64_t> &rct_offsets, std::unordered_set<crypto::public_key> &valid_public_keys_cache);
    bool tx_add_fake_output(std::vector<std::vector<tools::wallet2::get_outs_entry>> &outs, uint64_t global_index, const crypto::public_key& tx_public_key, const rct::key& mask, uint64_t real_index, bool unlocked, std::unordered_set<crypto::public_key> &valid_public_keys_cache) const;
    bool should_pick_a_second_output(bool use_rct, size_t n_transfers, const std::vector<size_t> &unused_transfers_indices, const std::vector<size_t> &unused_dust_indices) const;
    std::vector<size_t> get_only_rct(const std::vector<size_t> &unused_dust_indices, const std::vector<size_t> &unused_transfers_indices) const;
    void scan_output(const cryptonote::transaction &tx, bool miner_tx, const crypto::public_key &tx_pub_key, size_t i, tx_scan_info_t &tx_scan_info, int &num_vouts_received, std::unordered_map<cryptonote::subaddress_index, uint64_t> &tx_money_got_in_outs, std::vector<size_t> &outs, bool pool);
    void trim_hashchain();
    crypto::key_image get_multisig_composite_key_image(size_t n) const;
    rct::multisig_kLRki get_multisig_composite_kLRki(size_t n,  const std::unordered_set<crypto::public_key> &ignore_set, std::unordered_set<rct::key> &used_L, std::unordered_set<rct::key> &new_used_L) const;
    rct::multisig_kLRki get_multisig_kLRki(size_t n, const rct::key &k) const;
    void get_multisig_k(size_t idx, const std::unordered_set<rct::key> &used_L, rct::key &nonce);
    void update_multisig_rescan_info(const std::vector<std::vector<rct::key>> &multisig_k, const std::vector<std::vector<tools::wallet2::multisig_info>> &info, size_t n);
    bool add_rings(const crypto::chacha_key &key, const cryptonote::transaction_prefix &tx);
    bool add_rings(const cryptonote::transaction_prefix &tx);
    bool remove_rings(const cryptonote::transaction_prefix &tx);
    bool get_ring(const crypto::chacha_key &key, const crypto::key_image &key_image, std::vector<uint64_t> &outs);
    crypto::chacha_key get_ringdb_key();
    void setup_keys(const epee::wipeable_string &password);
    size_t get_transfer_details(const crypto::key_image &ki) const;
    tx_entry_data get_tx_entries(const std::unordered_set<crypto::hash> &txids);
    void sort_scan_tx_entries(std::vector<process_tx_entry_t> &unsorted_tx_entries);
    void process_scan_txs(const tx_entry_data &txs_to_scan, const tx_entry_data &txs_to_reprocess, const std::unordered_set<crypto::hash> &tx_hashes_to_reprocess, detached_blockchain_data &dbd);

    void register_devices();
    hw::device& lookup_device(const std::string & device_descriptor);

    bool get_rct_distribution(uint64_t &start_height, std::vector<uint64_t> &distribution);

    uint64_t get_segregation_fork_height() const;

    void cache_tx_data(const cryptonote::transaction& tx, const crypto::hash &txid, tx_cache_data &tx_cache_data) const;
    std::shared_ptr<std::map<std::pair<uint64_t, uint64_t>, size_t>> create_output_tracker_cache() const;

    void init_type(hw::device::device_type device_type);
    void setup_new_blockchain();
    void create_keys_file(const std::string &wallet_, bool watch_only, const epee::wipeable_string &password, bool create_address_file);

    wallet_device_callback * get_device_callback();
    void on_device_button_request(uint64_t code);
    void on_device_button_pressed();
    boost::optional<epee::wipeable_string> on_device_pin_request();
    boost::optional<epee::wipeable_string> on_device_passphrase_request(bool & on_device);
    void on_device_progress(const hw::device_progress& event);

    std::string get_rpc_status(const std::string &s) const;
    void throw_on_rpc_response_error(bool r, const epee::json_rpc::error &error, const std::string &status, const char *method) const;

    bool should_expand(const cryptonote::subaddress_index &index) const;
    bool spends_one_of_ours(const cryptonote::transaction &tx) const;

    cryptonote::account_base m_account;
    boost::optional<epee::net_utils::http::login> m_daemon_login;
    std::string m_daemon_address;
    std::string m_wallet_file;
    std::string m_keys_file;
    std::string m_mms_file;
    const std::unique_ptr<epee::net_utils::http::abstract_http_client> m_http_client;
    hashchain m_blockchain;
    serializable_unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;
    serializable_unordered_map<crypto::hash, confirmed_transfer_details> m_confirmed_txs;
    serializable_unordered_multimap<crypto::hash, pool_payment_details> m_unconfirmed_payments;
    serializable_unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;
    cryptonote::checkpoints m_checkpoints;
    serializable_unordered_map<crypto::hash, std::vector<crypto::secret_key>> m_additional_tx_keys;

    transfer_container m_transfers;
    payment_container m_payments;
    serializable_unordered_map<crypto::key_image, size_t> m_key_images;
    serializable_unordered_map<crypto::public_key, size_t> m_pub_keys;
    cryptonote::account_public_address m_account_public_address;
    serializable_unordered_map<crypto::public_key, cryptonote::subaddress_index> m_subaddresses;
    std::vector<std::vector<std::string>> m_subaddress_labels;
    serializable_unordered_map<crypto::hash, std::string> m_tx_notes;
    serializable_unordered_map<std::string, std::string> m_attributes;
    std::vector<tools::wallet2::address_book_row> m_address_book;
    std::pair<serializable_map<std::string, std::string>, std::vector<std::string>> m_account_tags;
    uint64_t m_upper_transaction_weight_limit; //TODO: auto-calc this value or request from daemon, now use some fixed value
    const std::vector<std::vector<tools::wallet2::multisig_info>> *m_multisig_rescan_info;
    const std::vector<std::vector<rct::key>> *m_multisig_rescan_k;
    serializable_unordered_map<crypto::public_key, crypto::key_image> m_cold_key_images;

    std::atomic<bool> m_run;

    boost::recursive_mutex m_daemon_rpc_mutex;

    bool m_trusted_daemon;
    i_wallet2_callback* m_callback;
    hw::device::device_type m_key_device_type;
    cryptonote::network_type m_nettype;
    uint64_t m_kdf_rounds;
    std::string seed_language; /*!< Language of the mnemonics (seed). */
    bool is_old_file_format; /*!< Whether the wallet file is of an old file format */
    bool m_watch_only; /*!< no spend key */
    bool m_multisig; /*!< if > 1 spend secret key will not match spend public key */
    uint32_t m_multisig_threshold;
    std::vector<crypto::public_key> m_multisig_signers;
    //in case of general M/N multisig wallet we should perform N - M + 1 key exchange rounds and remember how many rounds are passed.
    uint32_t m_multisig_rounds_passed;
    std::vector<crypto::public_key> m_multisig_derivations;
    bool m_always_confirm_transfers;
    bool m_print_ring_members;
    bool m_store_tx_info; /*!< request txkey to be returned in RPC, and store in the wallet cache file */
    uint32_t m_default_mixin;
    uint32_t m_default_priority;
    RefreshType m_refresh_type;
    bool m_auto_refresh;
    bool m_first_refresh_done;
    uint64_t m_refresh_from_block_height;
    // If m_refresh_from_block_height is explicitly set to zero we need this to differentiate it from the case that
    // m_refresh_from_block_height was defaulted to zero.*/
    bool m_explicit_refresh_from_block_height;
    uint64_t m_pool_info_query_time;
    uint64_t m_skip_to_height;
    // m_skip_to_height is useful when we don't want to modify the wallet's restore height.
    // m_refresh_from_block_height is also a wallet's restore height which should remain constant unless explicitly modified by the user.
    bool m_confirm_non_default_ring_size;
    AskPasswordType m_ask_password;
    uint64_t m_max_reorg_depth;
    uint32_t m_min_output_count;
    uint64_t m_min_output_value;
    bool m_merge_destinations;
    bool m_confirm_backlog;
    uint32_t m_confirm_backlog_threshold;
    bool m_confirm_export_overwrite;
    bool m_auto_low_priority;
    bool m_segregate_pre_fork_outputs;
    bool m_key_reuse_mitigation2;
    uint64_t m_segregation_height;
    bool m_ignore_fractional_outputs;
    uint64_t m_ignore_outputs_above;
    uint64_t m_ignore_outputs_below;
    bool m_track_uses;
    bool m_show_wallet_name_when_locked;
    uint32_t m_inactivity_lock_timeout;
    BackgroundMiningSetupType m_setup_background_mining;
    float m_auto_mine_for_rpc_payment_threshold;
    bool m_is_initialized;
    NodeRPCProxy m_node_rpc_proxy;
    std::unordered_set<crypto::hash> m_scanned_pool_txs[2];
    size_t m_subaddress_lookahead_major, m_subaddress_lookahead_minor;
    std::string m_device_name;
    std::string m_device_derivation_path;
    uint64_t m_device_last_key_image_sync;
    bool m_use_dns;
    bool m_offline;
    uint32_t m_rpc_version;
    bool m_enable_multisig;
    bool m_allow_mismatched_daemon_version;

    // Aux transaction data from device
    serializable_unordered_map<crypto::hash, std::string> m_tx_device;

    std::string m_ring_database;
    bool m_ring_history_saved;
    std::unique_ptr<ringdb> m_ringdb;
    boost::optional<crypto::chacha_key> m_ringdb_key;

    uint64_t m_last_block_reward;
    std::unique_ptr<tools::file_locker> m_keys_file_locker;
    
    mms::message_store m_message_store;
    bool m_original_keys_available;
    cryptonote::account_public_address m_original_address;
    crypto::secret_key m_original_view_secret_key;

    crypto::chacha_key m_cache_key;
    std::shared_ptr<wallet_keys_unlocker> m_encrypt_keys_after_refresh;

    bool m_unattended;
    bool m_devices_registered;

    std::shared_ptr<tools::Notify> m_tx_notify;
    std::unique_ptr<wallet_device_callback> m_device_callback;

    ExportFormat m_export_format;
    bool m_load_deprecated_formats;

    bool m_has_ever_refreshed_from_node;

    static boost::mutex default_daemon_address_lock;
    static std::string default_daemon_address;
  };
}
BOOST_CLASS_VERSION(tools::wallet2, 30)
BOOST_CLASS_VERSION(tools::wallet2::transfer_details, 12)
BOOST_CLASS_VERSION(tools::wallet2::multisig_info, 1)
BOOST_CLASS_VERSION(tools::wallet2::multisig_info::LR, 0)
BOOST_CLASS_VERSION(tools::wallet2::multisig_tx_set, 1)
BOOST_CLASS_VERSION(tools::wallet2::payment_details, 5)
BOOST_CLASS_VERSION(tools::wallet2::pool_payment_details, 1)
BOOST_CLASS_VERSION(tools::wallet2::unconfirmed_transfer_details, 8)
BOOST_CLASS_VERSION(tools::wallet2::confirmed_transfer_details, 6)
BOOST_CLASS_VERSION(tools::wallet2::address_book_row, 18)
BOOST_CLASS_VERSION(tools::wallet2::reserve_proof_entry, 0)
BOOST_CLASS_VERSION(tools::wallet2::unsigned_tx_set, 1)
BOOST_CLASS_VERSION(tools::wallet2::signed_tx_set, 1)
BOOST_CLASS_VERSION(tools::wallet2::tx_construction_data, 4)
BOOST_CLASS_VERSION(tools::wallet2::pending_tx, 3)
BOOST_CLASS_VERSION(tools::wallet2::multisig_sig, 1)

namespace boost
{
  namespace serialization
  {
    template<class Archive, class F, class S, class T>
    inline void serialize(
        Archive & ar,
        std::tuple<F, S, T> & t,
        const unsigned int /* file_version */
    ){
        ar & boost::serialization::make_nvp("f", std::get<0>(t));
        ar & boost::serialization::make_nvp("s", std::get<1>(t));
        ar & boost::serialization::make_nvp("t", std::get<2>(t));
    }

    template <class Archive>
    inline typename std::enable_if<!Archive::is_loading::value, void>::type initialize_transfer_details(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
    }
    template <class Archive>
    inline typename std::enable_if<Archive::is_loading::value, void>::type initialize_transfer_details(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
        if (ver < 1)
        {
          x.m_mask = rct::identity();
          x.m_amount = x.m_tx.vout[x.m_internal_output_index].amount;
        }
        if (ver < 2)
        {
          x.m_spent_height = 0;
        }
        if (ver < 4)
        {
          x.m_rct = x.m_tx.vout[x.m_internal_output_index].amount == 0;
        }
        if (ver < 6)
        {
          x.m_key_image_known = true;
        }
        if (ver < 7)
        {
          x.m_pk_index = 0;
        }
        if (ver < 8)
        {
          x.m_subaddr_index = {};
        }
        if (ver < 9)
        {
          x.m_key_image_partial = false;
          x.m_multisig_k.clear();
          x.m_multisig_info.clear();
        }
        if (ver < 10)
        {
          x.m_key_image_request = false;
        }
        if (ver < 12)
        {
          x.m_frozen = false;
        }
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_block_height;
      a & x.m_global_output_index;
      a & x.m_internal_output_index;
      if (ver < 3)
      {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
        x.m_txid = cryptonote::get_transaction_hash(tx);
      }
      else
      {
        a & x.m_tx;
      }
      a & x.m_spent;
      a & x.m_key_image;
      if (ver < 1)
      {
        // ensure mask and amount are set
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_mask;
      a & x.m_amount;
      if (ver < 2)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_spent_height;
      if (ver < 3)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_txid;
      if (ver < 4)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_rct;
      if (ver < 5)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      if (ver < 6)
      {
        // v5 did not properly initialize
        uint8_t u;
        a & u;
        x.m_key_image_known = true;
        return;
      }
      a & x.m_key_image_known;
      if (ver < 7)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_pk_index;
      if (ver < 8)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_subaddr_index;
      if (ver < 9)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_multisig_info;
      a & x.m_multisig_k;
      a & x.m_key_image_partial;
      if (ver < 10)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_key_image_request;
      if (ver < 11)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_uses;
      if (ver < 12)
      {
        initialize_transfer_details(a, x, ver);
        return;
      }
      a & x.m_frozen;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::multisig_info::LR &x, const boost::serialization::version_type ver)
    {
      a & x.m_L;
      a & x.m_R;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::multisig_info &x, const boost::serialization::version_type ver)
    {
      a & x.m_signer;
      a & x.m_LR;
      a & x.m_partial_key_images;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::multisig_tx_set &x, const boost::serialization::version_type ver)
    {
      a & x.m_ptx;
      a & x.m_signers;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unconfirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_change;
      a & x.m_sent_time;
      if (ver < 5)
      {
        cryptonote::transaction tx;
        a & tx;
        x.m_tx = (const cryptonote::transaction_prefix&)tx;
      }
      else
      {
        a & x.m_tx;
      }
      if (ver < 1)
        return;
      a & x.m_dests;
      a & x.m_payment_id;
      if (ver < 2)
        return;
      a & x.m_state;
      if (ver < 3)
        return;
      a & x.m_timestamp;
      if (ver < 4)
        return;
      a & x.m_amount_in;
      a & x.m_amount_out;
      if (ver < 6)
      {
        // v<6 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs.
        // We convert it to include change from v6
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)-1)
          x.m_amount_out += x.m_change;
      }
      if (ver < 7)
      {
        x.m_subaddr_account = 0;
        return;
      }
      a & x.m_subaddr_account;
      a & x.m_subaddr_indices;
      if (ver < 8)
        return;
      a & x.m_rings;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::confirmed_transfer_details &x, const boost::serialization::version_type ver)
    {
      a & x.m_amount_in;
      a & x.m_amount_out;
      a & x.m_change;
      a & x.m_block_height;
      if (ver < 1)
        return;
      a & x.m_dests;
      a & x.m_payment_id;
      if (ver < 2)
        return;
      a & x.m_timestamp;
      if (ver < 3)
      {
        // v<3 may not have change accumulated in m_amount_out, which is a pain,
        // as it's readily understood to be sum of outputs. Whether it got added
        // or not depends on whether it came from a unconfirmed_transfer_details
        // (not included) or not (included). We can't reliably tell here, so we
        // check whether either yields a "negative" fee, or use the other if so.
        // We convert it to include change from v3
        if (!typename Archive::is_saving() && x.m_change != (uint64_t)-1)
        {
          if (x.m_amount_in > (x.m_amount_out + x.m_change))
            x.m_amount_out += x.m_change;
        }
      }
      if (ver < 4)
      {
        if (!typename Archive::is_saving())
          x.m_unlock_time = 0;
        return;
      }
      a & x.m_unlock_time;
      if (ver < 5)
      {
        x.m_subaddr_account = 0;
        return;
      }
      a & x.m_subaddr_account;
      a & x.m_subaddr_indices;
      if (ver < 6)
        return;
      a & x.m_rings;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_tx_hash;
      a & x.m_amount;
      a & x.m_block_height;
      a & x.m_unlock_time;
      if (ver < 1)
        return;
      a & x.m_timestamp;
      if (ver < 2)
      {
        x.m_coinbase = false;
        x.m_subaddr_index = {};
        return;
      }
      a & x.m_subaddr_index;
      if (ver < 3)
      {
        x.m_coinbase = false;
        x.m_fee = 0;
        return;
      }
      a & x.m_fee;
      if (ver < 4)
      {
        x.m_coinbase = false;
        return;
      }
      a & x.m_coinbase;
      if (ver < 5)
        return;
      a & x.m_amounts;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::pool_payment_details& x, const boost::serialization::version_type ver)
    {
      a & x.m_pd;
      a & x.m_double_spend_seen;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::address_book_row& x, const boost::serialization::version_type ver)
    {
      a & x.m_address;
      if (ver < 18)
      {
        crypto::hash payment_id;
        a & payment_id;
        x.m_has_payment_id = !(payment_id == crypto::null_hash);
        if (x.m_has_payment_id)
        {
          bool is_long = false;
          for (int i = 8; i < 32; ++i)
            is_long |= payment_id.data[i];
          if (is_long)
          {
            MWARNING("Long payment ID ignored on address book load");
            x.m_payment_id = crypto::null_hash8;
            x.m_has_payment_id = false;
          }
          else
            memcpy(x.m_payment_id.data, payment_id.data, 8);
        }
      }
      a & x.m_description;
      if (ver < 17)
      {
        x.m_is_subaddress = false;
        return;
      }
      a & x.m_is_subaddress;
      if (ver < 18)
        return;
      a & x.m_has_payment_id;
      if (x.m_has_payment_id)
        a & x.m_payment_id;
    }

    template <class Archive>
    inline void serialize(Archive& a, tools::wallet2::reserve_proof_entry& x, const boost::serialization::version_type ver)
    {
      a & x.txid;
      a & x.index_in_tx;
      a & x.shared_secret;
      a & x.key_image;
      a & x.shared_secret_sig;
      a & x.key_image_sig;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::unsigned_tx_set &x, const boost::serialization::version_type ver)
    {
      a & x.txes;
      if (ver == 0)
      {
        // load old version
        std::pair<size_t, tools::wallet2::transfer_container> old_transfers;
        a & old_transfers;
        std::get<0>(x.transfers) = std::get<0>(old_transfers);
        std::get<1>(x.transfers) = std::get<0>(old_transfers) + std::get<1>(old_transfers).size();
        std::get<2>(x.transfers) = std::get<1>(old_transfers);
        return;
      }
      throw std::runtime_error("Boost serialization not supported for newest unsigned_tx_set");
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::signed_tx_set &x, const boost::serialization::version_type ver)
    {
      a & x.ptx;
      a & x.key_images;
      if (ver < 1)
        return;
      a & x.tx_key_images.parent();
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::tx_construction_data &x, const boost::serialization::version_type ver)
    {
      a & x.sources;
      a & x.change_dts;
      a & x.splitted_dsts;
      if (ver < 2)
      {
        // load list to vector
        std::list<size_t> selected_transfers;
        a & selected_transfers;
        x.selected_transfers.clear();
        x.selected_transfers.reserve(selected_transfers.size());
        for (size_t t: selected_transfers)
          x.selected_transfers.push_back(t);
      }
      a & x.extra;
      a & x.unlock_time;
      a & x.use_rct;
      a & x.dests;
      if (ver < 1)
      {
        x.subaddr_account = 0;
        return;
      }
      a & x.subaddr_account;
      a & x.subaddr_indices;
      if (ver < 2)
      {
        if (!typename Archive::is_saving())
          x.rct_config = { rct::RangeProofBorromean, 0 };
        return;
      }
      a & x.selected_transfers;
      if (ver < 3)
      {
        if (!typename Archive::is_saving())
          x.rct_config = { rct::RangeProofBorromean, 0 };
        return;
      }
      if (ver < 4)
      {
        bool use_bulletproofs = x.rct_config.range_proof_type != rct::RangeProofBorromean;
        a & use_bulletproofs;
        if (!typename Archive::is_saving())
          x.rct_config = { use_bulletproofs ? rct::RangeProofBulletproof : rct::RangeProofBorromean, 0 };
        return;
      }
      a & x.rct_config;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::multisig_sig &x, const boost::serialization::version_type ver)
    {
      a & x.sigs;
      a & x.ignore;
      a & x.used_L;
      a & x.signing_keys;
      a & x.msout;
      if (ver < 1)
        return;
      a & x.total_alpha_G;
      a & x.total_alpha_H;
      a & x.c_0;
      a & x.s;
    }

    template <class Archive>
    inline void serialize(Archive &a, tools::wallet2::pending_tx &x, const boost::serialization::version_type ver)
    {
      a & x.tx;
      a & x.dust;
      a & x.fee;
      a & x.dust_added_to_fee;
      a & x.change_dts;
      if (ver < 2)
      {
        // load list to vector
        std::list<size_t> selected_transfers;
        a & selected_transfers;
        x.selected_transfers.clear();
        x.selected_transfers.reserve(selected_transfers.size());
        for (size_t t: selected_transfers)
          x.selected_transfers.push_back(t);
      }
      a & x.key_images;
      a & x.tx_key;
      a & x.dests;
      a & x.construction_data;
      if (ver < 1)
        return;
      a & x.additional_tx_keys;
      if (ver < 2)
        return;
      a & x.selected_transfers;
      if (ver < 3)
        return;
      a & x.multisig_sigs;
    }
  }
}

namespace tools
{

  namespace detail
  {
    //----------------------------------------------------------------------------------------------------
    inline void digit_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, std::vector<cryptonote::tx_destination_entry> &dust_dsts)
    {
      splitted_dsts.clear();
      dust_dsts.clear();

      for(auto& de: dsts)
      {
        cryptonote::decompose_amount_into_digits(de.amount, 0,
          [&](uint64_t chunk) { splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, de.addr, de.is_subaddress)); },
          [&](uint64_t a_dust) { splitted_dsts.push_back(cryptonote::tx_destination_entry(a_dust, de.addr, de.is_subaddress)); } );
      }

      cryptonote::decompose_amount_into_digits(change_dst.amount, 0,
        [&](uint64_t chunk) {
          if (chunk <= dust_threshold)
            dust_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_dst.addr, false));
          else
            splitted_dsts.push_back(cryptonote::tx_destination_entry(chunk, change_dst.addr, false));
        },
        [&](uint64_t a_dust) { dust_dsts.push_back(cryptonote::tx_destination_entry(a_dust, change_dst.addr, false)); } );
    }
    //----------------------------------------------------------------------------------------------------
    inline void null_split_strategy(const std::vector<cryptonote::tx_destination_entry>& dsts,
      const cryptonote::tx_destination_entry& change_dst, uint64_t dust_threshold,
      std::vector<cryptonote::tx_destination_entry>& splitted_dsts, std::vector<cryptonote::tx_destination_entry> &dust_dsts)
    {
      splitted_dsts = dsts;

      dust_dsts.clear();
      uint64_t change = change_dst.amount;

      if (0 != change)
      {
        splitted_dsts.push_back(cryptonote::tx_destination_entry(change, change_dst.addr, false));
      }
    }
    //----------------------------------------------------------------------------------------------------
    inline void print_source_entry(const cryptonote::tx_source_entry& src)
    {
      std::string indexes;
      std::for_each(src.outputs.begin(), src.outputs.end(), [&](const cryptonote::tx_source_entry::output_entry& s_e) { indexes += boost::to_string(s_e.first) + " "; });
      LOG_PRINT_L0("amount=" << cryptonote::print_money(src.amount) << ", real_output=" <<src.real_output << ", real_output_in_tx_index=" << src.real_output_in_tx_index << ", indexes: " << indexes);
    }
    //----------------------------------------------------------------------------------------------------
  }
  //----------------------------------------------------------------------------------------------------
}
