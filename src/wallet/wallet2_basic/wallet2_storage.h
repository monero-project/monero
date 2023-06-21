// Copyright (c) 2023, The Monero Project
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

#pragma once

#include <rapidjson/fwd.h>

#include "wallet2_constants.h"
#include "wallet2_types.h"

namespace wallet2_basic
{
struct cache
{
    hashchain m_blockchain;
    transfer_container m_transfers;
    cryptonote::account_public_address m_account_public_address;
    serializable_unordered_map<crypto::key_image, size_t> m_key_images;
    serializable_unordered_map<crypto::hash, unconfirmed_transfer_details> m_unconfirmed_txs;
    payment_container m_payments;
    serializable_unordered_map<crypto::hash, crypto::secret_key> m_tx_keys;
    serializable_unordered_map<crypto::hash, confirmed_transfer_details> m_confirmed_txs;
    serializable_unordered_map<crypto::hash, std::string> m_tx_notes;
    serializable_unordered_multimap<crypto::hash, pool_payment_details> m_unconfirmed_payments;
    serializable_unordered_map<crypto::public_key, size_t> m_pub_keys;
    std::vector<address_book_row> m_address_book;
    std::unordered_set<crypto::hash> m_scanned_pool_txs[2];
    serializable_unordered_map<crypto::public_key, cryptonote::subaddress_index> m_subaddresses;
    std::vector<std::vector<std::string>> m_subaddress_labels;
    serializable_unordered_map<crypto::hash, std::vector<crypto::secret_key>> m_additional_tx_keys;
    serializable_unordered_map<std::string, std::string> m_attributes;
    std::pair<serializable_map<std::string, std::string>, std::vector<std::string>> m_account_tags;
    bool m_ring_history_saved;
    uint64_t m_last_block_reward;
    // Aux transaction data from device
    serializable_unordered_map<crypto::hash, std::string> m_tx_device;
    uint64_t m_device_last_key_image_sync;
    serializable_unordered_map<crypto::public_key, crypto::key_image> m_cold_key_images;
    bool m_has_ever_refreshed_from_node;

    // There's special key derivations for specifically wallet cache files for some reason
    static crypto::chacha_key pwd_to_cache_key(const char* pwd, size_t len, uint64_t kdf_rounds = 1);
    static crypto::chacha_key account_to_old_cache_key(const cryptonote::account_base& account, uint64_t kdf_rounds = 1);

    static cache load_from_memory
    (
        const std::string& cache_file_buf,
        const epee::wipeable_string& password,
        const cryptonote::account_base& wallet_account,
        uint64_t kdf_rounds = 1
    );

    std::string store_to_memory(const epee::wipeable_string& password, uint64_t kdf_rounds = 1);
    std::string store_to_memory(const crypto::chacha_key& encryption_key);

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
};

struct keys_data
{
    cryptonote::account_base m_account;

    bool is_old_file_format = false;
    bool m_watch_only = false; /*!< no spend key */
    bool m_multisig = false; /*!< if > 1 spend secret key will not match spend public key */
    std::string seed_language = ""; /*!< Language of the mnemonics (seed). */
    cryptonote::network_type m_nettype = cryptonote::UNDEFINED;
    uint32_t m_multisig_threshold = 0;
    std::vector<crypto::public_key> m_multisig_signers;
    //in case of general M/N multisig wallet we should perform N - M + 1 key exchange rounds and remember how many rounds are passed.
    uint32_t m_multisig_rounds_passed = 0;
    std::vector<crypto::public_key> m_multisig_derivations;
    bool m_always_confirm_transfers = true;
    bool m_print_ring_members = false;
    bool m_store_tx_info = true; /*!< request txkey to be returned in RPC, and store in the wallet cache file */
    uint32_t m_default_mixin = 0;
    uint32_t m_default_priority = 0;
    bool m_auto_refresh = true;
    RefreshType m_refresh_type = RefreshDefault;
    uint64_t m_refresh_from_block_height = 0;
    uint64_t m_skip_to_height = 0;
    // m_skip_to_height is useful when we don't want to modify the wallet's restore height.
    // m_refresh_from_block_height is also a wallet's restore height which should remain constant unless explicitly modified by the user.
    bool m_confirm_non_default_ring_size = true;
    AskPasswordType m_ask_password = AskPasswordToDecrypt;
    uint64_t m_max_reorg_depth = ORPHANED_BLOCKS_MAX_COUNT;
    uint32_t m_min_output_count = 0;
    uint64_t m_min_output_value = 0;
    bool m_merge_destinations = false;
    bool m_confirm_backlog = true;
    uint32_t m_confirm_backlog_threshold = 0;
    bool m_confirm_export_overwrite = true;
    bool m_auto_low_priority = true;
    bool m_segregate_pre_fork_outputs = true;
    bool m_key_reuse_mitigation2 = true;
    uint64_t m_segregation_height = 0;
    bool m_ignore_fractional_outputs = true;
    uint64_t m_ignore_outputs_above = MONEY_SUPPLY;
    uint64_t m_ignore_outputs_below = 0;
    bool m_track_uses = false;
    bool m_show_wallet_name_when_locked = false;
    uint32_t m_inactivity_lock_timeout = DEFAULT_INACTIVITY_LOCK_TIMEOUT;
    BackgroundMiningSetupType m_setup_background_mining = BackgroundMiningMaybe;
    size_t m_subaddress_lookahead_major = SUBADDRESS_LOOKAHEAD_MAJOR;
    size_t m_subaddress_lookahead_minor = SUBADDRESS_LOOKAHEAD_MINOR;
    bool m_original_keys_available = false;
    cryptonote::account_public_address m_original_address;
    crypto::secret_key m_original_view_secret_key;
    ExportFormat m_export_format = ExportFormat::Binary;
    bool m_load_deprecated_formats = false;
    std::string m_device_name = "";
    std::string m_device_derivation_path = "";
    hw::device::device_type m_key_device_type = hw::device::device_type::SOFTWARE;
    bool m_enable_multisig = false;
    bool m_allow_mismatched_daemon_version = false;

    static crypto::chacha_key pwd_to_keys_data_key(const char* pwd, size_t len, uint64_t kdf_rounds = 1);

    static keys_data load_from_memory
    (
        const std::string& keys_file_buf,
        const epee::wipeable_string& password,
        cryptonote::network_type nettype = cryptonote::UNDEFINED,
        uint64_t kdf_rounds = 1
    );
    static keys_data load_from_memory
    (
        const std::string& keys_file_buf,
        const crypto::chacha_key& encryption_key,
        cryptonote::network_type nettype = cryptonote::UNDEFINED
    );

    std::string store_to_memory
    (
        const epee::wipeable_string& password,
        bool downgrade_to_watch_only = false,
        uint64_t kdf_rounds = 1
    );
    std::string store_to_memory
    (
        const crypto::chacha_key& encryption_key,
        bool downgrade_to_watch_only = false
    );

    bool requires_external_device() const
    { return m_key_device_type != hw::device::device_type::SOFTWARE; }

    bool keys_were_encrypted_on_load() const { return m_keys_were_encrypted_on_load; }

    void setup_account_keys_and_devices
    (
        const epee::wipeable_string& password,
        hw::i_device_callback* device_cb = nullptr,
        uint64_t kdf_rounds = 1
    );

    void setup_account_keys_and_devices
    (
        const crypto::chacha_key& encryption_key,
        hw::i_device_callback* device_cb = nullptr
    );

    bool verify_account_keys
    (
        bool view_only = false,
        hw::device* alt_device = nullptr
    ) const;

    hw::device& reconnect_device(hw::i_device_callback* device_cb = nullptr);

private:
    bool m_keys_were_encrypted_on_load = false;

    template <bool IsSaving>
    void adapt_tofrom_json_object
    (
        rapidjson::Document& obj,
        const crypto::chacha_key& keys_key,
        bool downgrade_to_watch_only = false
    );
}; // struct keys_data

bool verify_account_keys
(
    const cryptonote::account_keys& keys,
    bool view_only = false,
    hw::device* alt_device = nullptr
);

void load_keys_and_cache_from_memory
(
    const std::string& cache_file_buf,
    const std::string& keys_file_buf,
    const epee::wipeable_string& password,
    cache& c,
    keys_data& k,
    cryptonote::network_type nettype = cryptonote::UNDEFINED,
    bool allow_external_devices_setup = true,
    hw::i_device_callback* device_cb = nullptr,
    uint64_t kdf_rounds = 1
);

void load_keys_and_cache_from_file
(
    const std::string& cache_path,
    const epee::wipeable_string& password,
    cache& c,
    keys_data& k,
    cryptonote::network_type nettype = cryptonote::UNDEFINED,
    std::string keys_path = "",
    bool allow_external_devices_setup = true,
    hw::i_device_callback* device_cb = nullptr,
    uint64_t kdf_rounds = 1
);

struct cache_file_data
{
    crypto::chacha_iv iv;
    std::string cache_data;

    BEGIN_SERIALIZE_OBJECT()
        FIELD(iv)
        FIELD(cache_data)
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
} // namespace wallet2_basic
