// Copyright (c) 2026, The Numista Coins Project
// Portions Copyright (c) 2014-2026, The Monero Project
// See LICENSE for licensing and attribution details.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "cryptonote_basic/tx_extra.h"

namespace cryptonote
{
  struct numista_asset_record
  {
    numista_asset_type type;
    uint64_t max_supply;
    uint64_t total_supply;
    crypto::hash metadata_hash;
  };

  bool get_numista_asset_signing_hash(const tx_extra_numista_asset& operation, crypto::hash& hash);
  bool sign_numista_asset_operation(tx_extra_numista_asset& operation, const crypto::public_key& public_key, const crypto::secret_key& secret_key);
  bool add_numista_asset_operation(std::vector<uint8_t>& tx_extra, const tx_extra_numista_asset& operation);
  bool has_numista_asset_operation(const std::vector<uint8_t>& tx_extra, bool& has_operation);

  class numista_asset_ledger
  {
  public:
    numista_asset_ledger();
    explicit numista_asset_ledger(const crypto::public_key& issuer_key);

    void set_issuer_key(const crypto::public_key& issuer_key);
    bool apply(const tx_extra_numista_asset& operation, std::string* error = nullptr);
    bool apply_tx_extra(const std::vector<uint8_t>& tx_extra, std::string* error = nullptr);
    bool revert_tx_extra(const std::vector<uint8_t>& tx_extra, std::string* error = nullptr);
    bool get_asset(const crypto::hash& asset_id, numista_asset_record& record) const;
    uint64_t get_balance(const crypto::hash& asset_id, const std::string& username) const;
    bool get_username_key(const std::string& username, crypto::public_key& key) const;

  private:
    using balances = std::map<std::string, uint64_t>;

    bool verify_issuer_signature(const tx_extra_numista_asset& operation) const;
    bool verify_owner_signature(const tx_extra_numista_asset& operation, const std::string& username) const;
    static bool valid_username(const std::string& username);
    static bool valid_common_fields(const tx_extra_numista_asset& operation);
    bool fail(std::string* error, const char* message) const;

    crypto::public_key m_issuer_key;
    std::map<std::string, crypto::public_key> m_username_keys;
    std::map<crypto::hash, numista_asset_record> m_assets;
    std::map<crypto::hash, balances> m_balances;
  };
}
