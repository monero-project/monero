// Copyright (c) 2026, The Numista Coins Project
// Portions Copyright (c) 2014-2026, The Monero Project
// See LICENSE for licensing and attribution details.

#include "numista_assets.h"

#include <algorithm>
#include <sstream>

#include "serialization/binary_archive.h"
#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

namespace cryptonote
{
  namespace
  {
    constexpr size_t MAX_NUMISTA_USERNAME_SIZE = 64;
    constexpr const char SIGNING_DOMAIN[] = "NumistaAssetOperationV1";

  }

  bool get_numista_asset_signing_hash(const tx_extra_numista_asset& operation, crypto::hash& hash)
  {
    std::ostringstream stream;
    binary_archive<true> archive(stream);
    std::string domain(SIGNING_DOMAIN, sizeof(SIGNING_DOMAIN) - 1);
    tx_extra_numista_asset signing_copy = operation;
    signing_copy.signature = {};
    if (!::do_serialize(archive, domain) || !::do_serialize(archive, signing_copy))
      return false;
    const std::string data = stream.str();
    hash = crypto::cn_fast_hash(data.data(), data.size());
    return true;
  }

  bool sign_numista_asset_operation(tx_extra_numista_asset& operation, const crypto::public_key& public_key, const crypto::secret_key& secret_key)
  {
    crypto::hash hash;
    if (!get_numista_asset_signing_hash(operation, hash) || !crypto::check_key(public_key))
      return false;
    crypto::generate_signature(hash, public_key, secret_key, operation.signature);
    return true;
  }

  bool add_numista_asset_operation(std::vector<uint8_t>& tx_extra, const tx_extra_numista_asset& operation)
  {
    std::ostringstream stream;
    binary_archive<true> archive(stream);
    uint8_t tag = TX_EXTRA_TAG_NUMISTA_ASSET;
    tx_extra_numista_asset operation_copy = operation;
    if (!::do_serialize(archive, tag) || !::do_serialize(archive, operation_copy))
      return false;
    const std::string data = stream.str();
    tx_extra.insert(tx_extra.end(), data.begin(), data.end());
    return true;
  }

  bool has_numista_asset_operation(const std::vector<uint8_t>& tx_extra, bool& has_operation)
  {
    std::vector<tx_extra_field> fields;
    if (!parse_tx_extra(tx_extra, fields))
      return false;
    has_operation = false;
    for (const tx_extra_field& field : fields)
    {
      if (field.type() != typeid(tx_extra_numista_asset))
        continue;
      if (has_operation)
        return false;
      has_operation = true;
    }
    return true;
  }

  numista_asset_ledger::numista_asset_ledger()
    : m_issuer_key(crypto::public_key{})
  {
  }

  numista_asset_ledger::numista_asset_ledger(const crypto::public_key& issuer_key)
    : m_issuer_key(issuer_key)
  {
  }

  void numista_asset_ledger::set_issuer_key(const crypto::public_key& issuer_key)
  {
    m_issuer_key = issuer_key;
  }

  bool numista_asset_ledger::fail(std::string* error, const char* message) const
  {
    if (error)
      *error = message;
    return false;
  }

  bool numista_asset_ledger::valid_username(const std::string& username)
  {
    if (username.empty() || username.size() > MAX_NUMISTA_USERNAME_SIZE)
      return false;
    return std::all_of(username.begin(), username.end(), [](unsigned char ch) {
      return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
    });
  }

  bool numista_asset_ledger::valid_common_fields(const tx_extra_numista_asset& operation)
  {
    return operation.version == 1;
  }

  bool numista_asset_ledger::verify_issuer_signature(const tx_extra_numista_asset& operation) const
  {
    crypto::hash hash;
    return crypto::check_key(m_issuer_key)
      && get_numista_asset_signing_hash(operation, hash)
      && crypto::check_signature(hash, m_issuer_key, operation.signature);
  }

  bool numista_asset_ledger::verify_owner_signature(const tx_extra_numista_asset& operation, const std::string& username) const
  {
    const auto it = m_username_keys.find(username);
    if (it == m_username_keys.end())
      return false;
    crypto::hash hash;
    return get_numista_asset_signing_hash(operation, hash)
      && crypto::check_signature(hash, it->second, operation.signature);
  }

  bool numista_asset_ledger::apply(const tx_extra_numista_asset& operation, std::string* error)
  {
    if (error)
      error->clear();
    if (!valid_common_fields(operation))
      return fail(error, "Unsupported Numista asset operation version");

    switch (operation.action)
    {
      case numista_asset_action::register_username:
      {
        if (!valid_username(operation.username) || !operation.recipient_username.empty()
          || operation.asset_id != crypto::null_hash || operation.asset_type != numista_asset_type::collectible
          || operation.max_supply != 0 || operation.amount != 0 || operation.metadata_hash != crypto::null_hash
          || !crypto::check_key(operation.asset_key))
          return fail(error, "Invalid username registration fields");
        if (m_username_keys.count(operation.username))
          return fail(error, "Username is already registered");
        for (const auto& registered : m_username_keys)
        {
          if (registered.second == operation.asset_key)
            return fail(error, "Asset key is already registered to another username");
        }
        if (!verify_issuer_signature(operation))
          return fail(error, "Invalid issuer signature for username registration");
        m_username_keys.emplace(operation.username, operation.asset_key);
        return true;
      }

      case numista_asset_action::create_asset:
      {
        if (operation.asset_id == crypto::null_hash || !operation.username.empty()
          || !operation.recipient_username.empty() || operation.amount != 0
          || operation.asset_key != crypto::null_pkey || operation.max_supply == 0
          || operation.metadata_hash == crypto::null_hash
          || (operation.asset_type != numista_asset_type::collectible && operation.asset_type != numista_asset_type::fungible)
          || (operation.asset_type == numista_asset_type::collectible && operation.max_supply != 1))
          return fail(error, "Invalid asset creation fields");
        if (m_assets.count(operation.asset_id))
          return fail(error, "Asset ID is already registered");
        if (!verify_issuer_signature(operation))
          return fail(error, "Invalid issuer signature for asset creation");
        m_assets.emplace(operation.asset_id, numista_asset_record{operation.asset_type, operation.max_supply, 0, operation.metadata_hash});
        m_balances.emplace(operation.asset_id, balances{});
        return true;
      }

      case numista_asset_action::mint:
      {
        if (!valid_username(operation.recipient_username) || !operation.username.empty()
          || operation.asset_id == crypto::null_hash || operation.amount == 0
          || (operation.asset_type != numista_asset_type::collectible && operation.asset_type != numista_asset_type::fungible)
          || operation.max_supply != 0
          || operation.metadata_hash != crypto::null_hash || operation.asset_key != crypto::null_pkey)
          return fail(error, "Invalid asset mint fields");
        const auto asset = m_assets.find(operation.asset_id);
        if (asset == m_assets.end())
          return fail(error, "Unknown asset ID");
        if (operation.asset_type != asset->second.type)
          return fail(error, "Asset type does not match registered asset");
        if (!m_username_keys.count(operation.recipient_username))
          return fail(error, "Recipient username is not registered");
        if (!verify_issuer_signature(operation))
          return fail(error, "Invalid issuer signature for asset mint");
        if (asset->second.type == numista_asset_type::collectible && operation.amount != 1)
          return fail(error, "Collectible mint amount must be one");
        if (operation.amount > asset->second.max_supply - asset->second.total_supply)
          return fail(error, "Mint exceeds asset supply cap");
        asset->second.total_supply += operation.amount;
        m_balances[operation.asset_id][operation.recipient_username] += operation.amount;
        return true;
      }

      case numista_asset_action::transfer:
      {
        if (!valid_username(operation.username) || !valid_username(operation.recipient_username)
          || operation.username == operation.recipient_username || operation.asset_id == crypto::null_hash
          || operation.amount == 0
          || (operation.asset_type != numista_asset_type::collectible && operation.asset_type != numista_asset_type::fungible)
          || operation.max_supply != 0 || operation.metadata_hash != crypto::null_hash
          || operation.asset_key != crypto::null_pkey)
          return fail(error, "Invalid asset transfer fields");
        const auto asset = m_assets.find(operation.asset_id);
        if (asset == m_assets.end())
          return fail(error, "Unknown asset ID");
        if (operation.asset_type != asset->second.type)
          return fail(error, "Asset type does not match registered asset");
        if (!m_username_keys.count(operation.recipient_username))
          return fail(error, "Recipient username is not registered");
        if (!verify_owner_signature(operation, operation.username))
          return fail(error, "Invalid asset owner signature");
        if (asset->second.type == numista_asset_type::collectible && operation.amount != 1)
          return fail(error, "Collectible transfer amount must be one");
        balances& asset_balances = m_balances[operation.asset_id];
        const auto balance = asset_balances.find(operation.username);
        if (balance == asset_balances.end() || balance->second < operation.amount)
          return fail(error, "Insufficient asset balance");
        balance->second -= operation.amount;
        asset_balances[operation.recipient_username] += operation.amount;
        return true;
      }

      default:
        return fail(error, "Unknown Numista asset operation");
    }
  }

  bool numista_asset_ledger::apply_tx_extra(const std::vector<uint8_t>& tx_extra, std::string* error)
  {
    std::vector<tx_extra_field> fields;
    if (!parse_tx_extra(tx_extra, fields))
      return fail(error, "Malformed transaction extra");

    const tx_extra_numista_asset* operation = nullptr;
    for (const tx_extra_field& field : fields)
    {
      if (field.type() != typeid(tx_extra_numista_asset))
        continue;
      if (operation)
        return fail(error, "Multiple Numista asset operations in one transaction");
      operation = &boost::get<tx_extra_numista_asset>(field);
    }

    if (!operation)
    {
      if (error)
        error->clear();
      return true;
    }
    return apply(*operation, error);
  }

  bool numista_asset_ledger::revert_tx_extra(const std::vector<uint8_t>& tx_extra, std::string* error)
  {
    std::vector<tx_extra_field> fields;
    if (!parse_tx_extra(tx_extra, fields))
      return fail(error, "Malformed transaction extra");

    const tx_extra_numista_asset* operation = nullptr;
    for (const tx_extra_field& field : fields)
    {
      if (field.type() != typeid(tx_extra_numista_asset))
        continue;
      if (operation)
        return fail(error, "Multiple Numista asset operations in one transaction");
      operation = &boost::get<tx_extra_numista_asset>(field);
    }
    if (!operation)
      return true;
    if (error)
      error->clear();

    switch (operation->action)
    {
      case numista_asset_action::register_username:
      {
        const auto username = m_username_keys.find(operation->username);
        if (username == m_username_keys.end() || username->second != operation->asset_key)
          return fail(error, "Cannot undo missing username registration");
        m_username_keys.erase(username);
        return true;
      }
      case numista_asset_action::create_asset:
      {
        const auto asset = m_assets.find(operation->asset_id);
        const auto balances = m_balances.find(operation->asset_id);
        if (asset == m_assets.end() || balances == m_balances.end()
          || asset->second.total_supply != 0 || !balances->second.empty())
          return fail(error, "Cannot undo asset creation with issued supply");
        m_assets.erase(asset);
        m_balances.erase(balances);
        return true;
      }
      case numista_asset_action::mint:
      {
        const auto asset = m_assets.find(operation->asset_id);
        const auto balances = m_balances.find(operation->asset_id);
        if (asset == m_assets.end() || balances == m_balances.end()
          || asset->second.total_supply < operation->amount)
          return fail(error, "Cannot undo asset mint");
        const auto balance = balances->second.find(operation->recipient_username);
        if (balance == balances->second.end() || balance->second < operation->amount)
          return fail(error, "Cannot undo asset mint balance");
        balance->second -= operation->amount;
        if (!balance->second)
          balances->second.erase(balance);
        asset->second.total_supply -= operation->amount;
        return true;
      }
      case numista_asset_action::transfer:
      {
        auto balances = m_balances.find(operation->asset_id);
        if (balances == m_balances.end())
          return fail(error, "Cannot undo transfer for unknown asset");
        const auto recipient = balances->second.find(operation->recipient_username);
        if (recipient == balances->second.end() || recipient->second < operation->amount)
          return fail(error, "Cannot undo asset transfer balance");
        recipient->second -= operation->amount;
        if (!recipient->second)
          balances->second.erase(recipient);
        balances->second[operation->username] += operation->amount;
        return true;
      }
      default:
        return fail(error, "Cannot undo unknown Numista asset operation");
    }
  }

  bool numista_asset_ledger::get_asset(const crypto::hash& asset_id, numista_asset_record& record) const
  {
    const auto it = m_assets.find(asset_id);
    if (it == m_assets.end())
      return false;
    record = it->second;
    return true;
  }

  uint64_t numista_asset_ledger::get_balance(const crypto::hash& asset_id, const std::string& username) const
  {
    const auto asset = m_balances.find(asset_id);
    if (asset == m_balances.end())
      return 0;
    const auto balance = asset->second.find(username);
    return balance == asset->second.end() ? 0 : balance->second;
  }

  bool numista_asset_ledger::get_username_key(const std::string& username, crypto::public_key& key) const
  {
    const auto it = m_username_keys.find(username);
    if (it == m_username_keys.end())
      return false;
    key = it->second;
    return true;
  }
}
