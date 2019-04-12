// Copyright (c) 2014-2019, The Monero Project
// Copyright (c)      2018, The Loki Project
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
#include "cryptonote_config.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/subaddress_index.h"
#include "crypto/hash.h"
#include "wallet_rpc_server_error_codes.h"

#include "common/loki.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "wallet.rpc"

// When making *any* change here, bump minor
// If the change is incompatible, then bump major and set minor to 0
// This ensures WALLET_RPC_VERSION always increases, that every change
// has its own version, and that clients can just test major to see
// whether they can talk to a given wallet without having to know in
// advance which version they will stop working with
// Don't go over 32767 for any of these
#define WALLET_RPC_VERSION_MAJOR 1
#define WALLET_RPC_VERSION_MINOR 8
#define MAKE_WALLET_RPC_VERSION(major,minor) (((major)<<16)|(minor))
#define WALLET_RPC_VERSION MAKE_WALLET_RPC_VERSION(WALLET_RPC_VERSION_MAJOR, WALLET_RPC_VERSION_MINOR)
namespace tools
{
namespace wallet_rpc
{
#define WALLET_RPC_STATUS_OK      "OK"
#define WALLET_RPC_STATUS_BUSY    "BUSY"

  LOKI_RPC_DOC_INTROSPECT
  // Return the wallet's balance.
  struct COMMAND_RPC_GET_BALANCE
  {
    struct request_t
    {
      uint32_t account_index;             // Return balance for this account.
      std::set<uint32_t> address_indices; // (Optional) Return balance detail for those subaddresses.
      bool all_accounts;                  // If true, return balance for all accounts, subaddr_indices and account_index are ignored

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address_indices)
        KV_SERIALIZE_OPT(all_accounts, false);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct per_subaddress_info
    {
      uint32_t account_index;       // Index of the account in the wallet.
      uint32_t address_index;       // Index of the subaddress in the account.
      std::string address;          // Address at this index. Base58 representation of the public keys.
      uint64_t balance;             // Balance for the subaddress (locked or unlocked).
      uint64_t unlocked_balance;    // Unlocked funds are those funds that are sufficiently deep enough in the loki blockchain to be considered safe to spend.
      std::string label;            // Label for the subaddress.
      uint64_t num_unspent_outputs; // Number of unspent outputs available for the subaddress.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address_index)
        KV_SERIALIZE(address)
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(label)
        KV_SERIALIZE(num_unspent_outputs)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      uint64_t 	 balance;                              // The total balance (atomic units) of the currently opened wallet.
      uint64_t 	 unlocked_balance;                     // Unlocked funds are those funds that are sufficiently deep enough in the loki blockchain to be considered safe to spend.
      bool       multisig_import_needed;               // True if importing multisig data is needed for returning a correct balance.
      std::vector<per_subaddress_info> per_subaddress; // Balance information for each subaddress in an account.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(multisig_import_needed)
        KV_SERIALIZE(per_subaddress)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Return the wallet's addresses for an account. Optionally filter for specific set of subaddresses.
  struct COMMAND_RPC_GET_ADDRESS
  {
    struct request_t
    {
      uint32_t account_index;              // Get the wallet addresses for the specified account.
      std::vector<uint32_t> address_index; // (Optional) List of subaddresses to return from the aforementioned account.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address_index)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct address_info
    {
      std::string address;    // The (sub)address string.
      std::string label;      // Label of the (sub)address.
      uint32_t address_index; // Index of the subaddress
      bool used;              // True if the (sub)address has received funds before.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(label)
        KV_SERIALIZE(address_index)
        KV_SERIALIZE(used)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::string address;                  // (Deprecated) Remains to be compatible with older RPC format
      std::vector<address_info> addresses;  // Addresses informations.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(addresses)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get account and address indexes from a specific (sub)address.
  struct COMMAND_RPC_GET_ADDRESS_INDEX
  {
    struct request_t
    {
      std::string address; // (Sub)address to look for.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      cryptonote::subaddress_index index; // Account index followed by the subaddress index.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Create a new address for an account. Optionally, label the new address.
  struct COMMAND_RPC_CREATE_ADDRESS
  {
    struct request_t
    {
      uint32_t account_index; // Create a new subaddress for this account.
      std::string label;      // (Optional) Label for the new subaddress.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string   address;       // The newly requested address.
      uint32_t      address_index; // Index of the new address in the requested account index.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(address_index)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Label an address.
  struct COMMAND_RPC_LABEL_ADDRESS
  {
    struct request_t
    {
      cryptonote::subaddress_index index; // Major & minor address index 
      std::string label;                  // Label for the address.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get all accounts for a wallet. Optionally filter accounts by tag.
  struct COMMAND_RPC_GET_ACCOUNTS
  {
    struct request_t
    {
      std::string tag;      // (Optional) Tag for filtering accounts. All accounts if empty, otherwise those accounts with this tag

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct subaddress_account_info
    {
      uint32_t account_index;    // Index of the account.
      std::string base_address;  // The first address of the account (i.e. the primary address).
      uint64_t balance;          // Balance of the account (locked or unlocked).
      uint64_t unlocked_balance; // Unlocked balance for the account.
      std::string label;         // (Optional) Label of the account.
      std::string tag;           // (Optional) Tag for filtering accounts.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(base_address)
        KV_SERIALIZE(balance)
        KV_SERIALIZE(unlocked_balance)
        KV_SERIALIZE(label)
        KV_SERIALIZE(tag)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      uint64_t total_balance;                                   // Total balance of the selected accounts (locked or unlocked).
      uint64_t total_unlocked_balance;                          // Total unlocked balance of the selected accounts.
      std::vector<subaddress_account_info> subaddress_accounts; // Account information.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(total_balance)
        KV_SERIALIZE(total_unlocked_balance)
        KV_SERIALIZE(subaddress_accounts)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Create a new account with an optional label.
  struct COMMAND_RPC_CREATE_ACCOUNT
  {
    struct request_t
    {
      std::string label; // (Optional) Label for the account.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint32_t account_index;   // Index of the new account.
      std::string address;      // The primary address of the new account.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Label an account.
  struct COMMAND_RPC_LABEL_ACCOUNT
  {
    struct request_t
    {
      uint32_t account_index; // Account index to set the label for.
      std::string label;      // Label for the account.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(label)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get a list of user-defined account tags.
  struct COMMAND_RPC_GET_ACCOUNT_TAGS
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct account_tag_info
    {
      std::string tag;                // Filter tag.
      std::string label;              // Label for the tag.
      std::vector<uint32_t> accounts; // List of tagged account indices.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag);
        KV_SERIALIZE(label);
        KV_SERIALIZE(accounts);
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::vector<account_tag_info> account_tags; // Account tag information:

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(account_tags)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Apply a filtering tag to a list of accounts.
  struct COMMAND_RPC_TAG_ACCOUNTS
  {
    struct request_t
    {
      std::string tag;             // Tag for the accounts.
      std::set<uint32_t> accounts; // Tag this list of accounts.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
        KV_SERIALIZE(accounts)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Remove filtering tag from a list of accounts.
  struct COMMAND_RPC_UNTAG_ACCOUNTS
  {
    struct request_t
    {
      std::set<uint32_t> accounts; // Remove tag from this list of accounts.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(accounts)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Set description for an account tag.
  struct COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION
  {
    struct request_t
    {
      std::string tag;         // Set a description for this tag.
      std::string description; // Description for the tag.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tag)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Returns the wallet's current block height.
  struct COMMAND_RPC_GET_HEIGHT
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t  height; // The current wallet's blockchain height. If the wallet has been offline for a long time, it may need to catch up with the daemon.
      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  struct transfer_destination
  {
    uint64_t amount;     // Amount to send to each destination, in atomic units.
    std::string address; // Destination public address.

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  // Send loki to a number of recipients. To preview the transaction fee, set do_not_relay to true and get_tx_metadata to true. 
  // Submit the response using the data in get_tx_metadata in the RPC call, relay_tx.
  struct COMMAND_RPC_TRANSFER
  {
    struct request_t
    {
      std::list<transfer_destination> destinations; // Array of destinations to receive LOKI.
      uint32_t account_index;                       // (Optional) Transfer from this account index. (Defaults to 0)
      std::set<uint32_t> subaddr_indices;           // (Optional) Transfer from this set of subaddresses. (Defaults to 0)
      uint32_t priority;                            // Set a priority for the transaction. Accepted Values are: default (1), or 0-3 for: unimportant, normal, elevated, priority.
      uint64_t mixin;                               // (Deprecated) Set to 9. Number of outputs from the blockchain to mix with. Loki mixin statically set to 9.
      uint64_t ring_size;                           // (Deprecated) Set to 10. Sets ringsize to n (mixin + 1). Loki ring_size is statically set to 10.
      uint64_t unlock_time;                         // Number of blocks before the loki can be spent (0 to use the default lock time).
      std::string payment_id;                       // (Optional) Random 64-character hex string to identify a transaction.
      bool get_tx_key;                              // (Optional) Return the transaction key after sending.
      bool do_not_relay;                            // (Optional) If true, the newly created transaction will not be relayed to the loki network. (Defaults to false)
      bool get_tx_hex;                              // Return the transaction as hex string after sending. (Defaults to false)
      bool get_tx_metadata;                         // Return the metadata needed to relay the transaction. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_hash;        // Publically searchable transaction hash.
      std::string tx_key;         // Transaction key if get_tx_key is true, otherwise, blank string.
      uint64_t amount;            // Amount transferred for the transaction.
      uint64_t fee;               // Fee charged for the txn.
      std::string tx_blob;        // Raw transaction represented as hex string, if get_tx_hex is true.
      std::string tx_metadata;    // Set of transaction metadata needed to relay this transfer later, if get_tx_metadata is true.
      std::string multisig_txset; // Set of multisig transactions in the process of being signed (empty for non-multisig).
      std::string unsigned_txset; // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Same as transfer, but can split into more than one tx if necessary.
  struct COMMAND_RPC_TRANSFER_SPLIT
  {
    struct request_t
    {
      std::list<transfer_destination> destinations; // Array of destinations to receive LOKI:
      uint32_t account_index;                       // (Optional) Transfer from this account index. (Defaults to 0)
      std::set<uint32_t> subaddr_indices;           // (Optional) Transfer from this set of subaddresses. (Defaults to 0)
      uint32_t priority;                            // Set a priority for the transactions. Accepted Values are: 0-3 for: default, unimportant, normal, elevated, priority.
      uint64_t mixin;                               // (Ignored) Number of outputs from the blockchain to mix with. Loki mixin statically set to 9.
      uint64_t ring_size;                           // (Ignored) Sets ringsize to n (mixin + 1). Loki ring_size is statically set to 10.
      uint64_t unlock_time;                         // Number of blocks before the loki can be spent (0 to not add a lock).
      std::string payment_id;                       // (Optional) Random 32-byte/64-character hex string to identify a transaction.
      bool get_tx_keys;                             // (Optional) Return the transaction keys after sending.
      bool do_not_relay;                            // (Optional) If true, the newly created transaction will not be relayed to the loki network. (Defaults to false)
      bool get_tx_hex;                              // Return the transactions as hex string after sending.
      bool get_tx_metadata;                         // Return list of transaction metadata needed to relay the transfer later.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(destinations)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct key_list
    {
      std::list<std::string> keys; //

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::list<std::string> tx_hash_list;     // The tx hashes of every transaction.
      std::list<std::string> tx_key_list;      // The transaction keys for every transaction.
      std::list<uint64_t> amount_list;         // The amount transferred for every transaction.
      std::list<uint64_t> fee_list;            // The amount of fees paid for every transaction.
      std::list<std::string> tx_blob_list;     // The tx as hex string for every transaction.
      std::list<std::string> tx_metadata_list; // List of transaction metadata needed to relay the transactions later.
      std::string multisig_txset;              // The set of signing keys used in a multisig transaction (empty for non-multisig).
      std::string unsigned_txset;              // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT

  struct COMMAND_RPC_DESCRIBE_TRANSFER
  {
    struct recipient
    {
      std::string address; // Destination public address.
      uint64_t amount;     // Amount in atomic units.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(amount)
      END_KV_SERIALIZE_MAP()
    };

    struct transfer_description
    {
      uint64_t amount_in;              // Amount in, in atomic units.
      uint64_t amount_out;             // amount out, in atomic units.
      uint32_t ring_size;              // Ring size of transfer.
      uint64_t unlock_time;            // Number of blocks before the loki can be spent (0 represents the default network lock time).
      std::list<recipient> recipients; // List of addresses and amounts.
      std::string payment_id;          // Payment ID matching the input parameter.
      uint64_t change_amount;          // Change received from transaction in atomic units.
      std::string change_address;      // Address the change was sent to.
      uint64_t fee;                    // Fee of the transaction in atomic units.
      uint32_t dummy_outputs;          // 
      std::string extra;               // Data stored in the tx extra represented in hex.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(amount_in)
        KV_SERIALIZE(amount_out)
        KV_SERIALIZE(ring_size)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(recipients)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(change_amount)
        KV_SERIALIZE(change_address)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(dummy_outputs)
        KV_SERIALIZE(extra)
      END_KV_SERIALIZE_MAP()
    };

    struct request_t
    {
      std::string unsigned_txset; // Set of unsigned tx returned by "transfer" or "transfer_split" methods.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<transfer_description> desc; // List of information of transfers.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(desc)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Sign a transaction created on a read-only wallet (in cold-signing process).
  struct COMMAND_RPC_SIGN_TRANSFER
  {
    struct request_t
    {
      std::string unsigned_txset; // Set of unsigned tx returned by "transfer" or "transfer_split" methods.
      bool export_raw;            // (Optional) If true, return the raw transaction data. (Defaults to false)
      bool get_tx_keys;           // (Optional) Return the transaction keys after sending.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(unsigned_txset)
        KV_SERIALIZE_OPT(export_raw, false)
        KV_SERIALIZE_OPT(get_tx_keys, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signed_txset;            // Set of signed tx to be used for submitting transfer.
      std::list<std::string> tx_hash_list; // The tx hashes of every transaction.
      std::list<std::string> tx_raw_list;  // The tx raw data of every transaction.
      std::list<std::string> tx_key_list;  // The tx key data of every transaction.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signed_txset)
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_raw_list)
        KV_SERIALIZE(tx_key_list)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Submit a previously signed transaction on a read-only wallet (in cold-signing process).
  struct COMMAND_RPC_SUBMIT_TRANSFER
  {
    struct request_t
    {
      std::string tx_data_hex; // Set of signed tx returned by "sign_transfer".

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<std::string> tx_hash_list; // The tx hashes of every transaction.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Send all dust outputs back to the wallet's, to make them easier to spend (and mix).
  struct COMMAND_RPC_SWEEP_DUST
  {
    struct request_t
    {
      bool get_tx_keys;     // (Optional) Return the transaction keys after sending.
      bool do_not_relay;    // (Optional) If true, the newly created transaction will not be relayed to the loki network. (Defaults to false)
      bool get_tx_hex;      // (Optional) Return the transactions as hex string after sending. (Defaults to false)
      bool get_tx_metadata; // (Optional) Return list of transaction metadata needed to relay the transfer later. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct key_list
    {
      std::list<std::string> keys; 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::list<std::string> tx_hash_list;     // The tx hashes of every transaction.
      std::list<std::string> tx_key_list;      // The transaction keys for every transaction.
      std::list<uint64_t> amount_list;         // The amount transferred for every transaction.
      std::list<uint64_t> fee_list;            // The amount of fees paid for every transaction.
      std::list<std::string> tx_blob_list;     // The tx as hex string for every transaction.
      std::list<std::string> tx_metadata_list; // List of transaction metadata needed to relay the transactions later. 
      std::string multisig_txset;              // The set of signing keys used in a multisig transaction (empty for non-multisig).
      std::string unsigned_txset;              // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Send all unlocked balance to an address.
  struct COMMAND_RPC_SWEEP_ALL
  {
    struct request_t
    {
      std::string address;                // Destination public address.
      uint32_t account_index;             // Sweep transactions from this account.
      std::set<uint32_t> subaddr_indices; // (Optional) Sweep from this set of subaddresses in the account.
      uint32_t priority;                  // (Optional) Priority for sending the sweep transfer, partially determines fee. 
      uint64_t mixin;                     // (Deprecated) Set to 9. Number of outputs from the blockchain to mix with. Loki mixin statically set to 9.
      uint64_t ring_size;                 // (Deprecated) Set to 10. Sets ringsize to n (mixin + 1). Loki ring_size is statically set to 10.
      uint64_t outputs;                   // 
      uint64_t unlock_time;               // Number of blocks before the loki can be spent (0 to not add a lock). 
      std::string payment_id;             // (Optional) 64-character hex string to identify a transaction.
      bool get_tx_keys;                   // (Optional) Return the transaction keys after sending.
      uint64_t below_amount;              // (Optional) Include outputs below this amount.
      bool do_not_relay;                  // (Optional) If true, do not relay this sweep transfer. (Defaults to false)
      bool get_tx_hex;                    // (Optional) return the transactions as hex encoded string. (Defaults to false)
      bool get_tx_metadata;               // (Optional) return the transaction metadata as a string. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE_OPT(outputs, (uint64_t)1)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_keys)
        KV_SERIALIZE(below_amount)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct key_list
    {
      std::list<std::string> keys;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(keys)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::list<std::string> tx_hash_list;     // The tx hashes of every transaction.
      std::list<std::string> tx_key_list;      // The transaction keys for every transaction.
      std::list<uint64_t> amount_list;         // The amount transferred for every transaction.
      std::list<uint64_t> fee_list;            // The amount of fees paid for every transaction.
      std::list<std::string> tx_blob_list;     // The tx as hex string for every transaction.
      std::list<std::string> tx_metadata_list; // List of transaction metadata needed to relay the transactions later.
      std::string multisig_txset;              // The set of signing keys used in a multisig transaction (empty for non-multisig).
      std::string unsigned_txset;              // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
        KV_SERIALIZE(tx_key_list)
        KV_SERIALIZE(amount_list)
        KV_SERIALIZE(fee_list)
        KV_SERIALIZE(tx_blob_list)
        KV_SERIALIZE(tx_metadata_list)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Send all of a specific unlocked output to an address.
  struct COMMAND_RPC_SWEEP_SINGLE
  {
    struct request_t
    {
      std::string address;    // Destination public address.
      uint32_t priority;      // (Optional) Priority for sending the sweep transfer, partially determines fee.
      uint64_t mixin;         // (Deprecated) Set to 9. Number of outputs from the blockchain to mix with. Loki mixin statically set to 9.
      uint64_t ring_size;     // (Deprecated) Set to 10. Sets ringsize to n (mixin + 1). Loki ring_size is statically set to 10.
      uint64_t outputs;       // 
      uint64_t unlock_time;   // Number of blocks before the loki can be spent (0 to not add a lock).
      std::string payment_id; // (Optional) 64-character hex string to identify a transaction.
      bool get_tx_key;        // (Optional) Return the transaction keys after sending.
      std::string key_image;  // Key image of specific output to sweep.
      bool do_not_relay;      // (Optional) If true, do not relay this sweep transfer. (Defaults to false)
      bool get_tx_hex;        // (Optional) return the transactions as hex encoded string. (Defaults to false)
      bool get_tx_metadata;   // (Optional) return the transaction metadata as a string. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(priority)
        KV_SERIALIZE_OPT(mixin, (uint64_t)0)
        KV_SERIALIZE_OPT(ring_size, (uint64_t)0)
        KV_SERIALIZE_OPT(outputs, (uint64_t)1)
        KV_SERIALIZE(unlock_time)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE(key_image)
        KV_SERIALIZE_OPT(do_not_relay, false)
        KV_SERIALIZE_OPT(get_tx_hex, false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_hash;        // The tx hashes of the transaction.
      std::string tx_key;         // The tx key of the transaction.
      uint64_t amount;            // The amount transfered in atomic units.
      uint64_t fee;               // The fee paid in atomic units.
      std::string tx_blob;        // The tx as hex string.
      std::string tx_metadata;    // Transaction metadata needed to relay the transaction later.
      std::string multisig_txset; // The set of signing keys used in a multisig transaction (empty for non-multisig).
      std::string unsigned_txset; // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Relay transaction metadata to the daemon 
  struct COMMAND_RPC_RELAY_TX
  {
    struct request_t
    {
      std::string hex; // Transaction metadata returned from a transfer method with get_tx_metadata set to true.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(hex)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_hash; // String for the publically searchable transaction hash.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Save the wallet file.
  struct COMMAND_RPC_STORE
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // 
  struct payment_details
  {
    std::string payment_id;                     // Payment ID matching the input parameter.
    std::string tx_hash;                        // Transaction hash used as the transaction ID.
    uint64_t amount;                            // Amount for this payment.
    uint64_t block_height;                      // Height of the block that first confirmed this payment.
    uint64_t unlock_time;                       // Time (in block height) until this payment is safe to spend.
    cryptonote::subaddress_index subaddr_index; // Major & minor index, account and subaddress index respectively.
    std::string address;                        // Address receiving the payment.

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(payment_id)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(amount)
      KV_SERIALIZE(block_height)
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(subaddr_index)
      KV_SERIALIZE(address)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get a list of incoming payments using a given payment id.
  struct COMMAND_RPC_GET_PAYMENTS
  {
    struct request_t
    {
      std::string payment_id; // Payment ID used to find the payments (16 characters hex).

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<payment_details> payments; // List of payment details:

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get a list of incoming payments using a given payment id, 
  // or a list of payments ids, from a given height. 
  //
  // This method is the preferred method over  get_paymentsbecause it 
  // has the same functionality but is more extendable. 
  // Either is fine for looking up transactions by a single payment ID.
  struct COMMAND_RPC_GET_BULK_PAYMENTS
  {
    struct request_t
    {
      std::vector<std::string> payment_ids; // Payment IDs used to find the payments (16 characters hex).
      uint64_t min_block_height;            // The block height at which to start looking for payments.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payment_ids)
        KV_SERIALIZE(min_block_height)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<payment_details> payments; // List of payment details: 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(payments)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  LOKI_RPC_DOC_INTROSPECT
  // 
  struct transfer_details
  {
    uint64_t amount;                            // Amount of this transfer.
    bool spent;                                 // Indicates if this transfer has been spent.
    uint64_t global_index;                      // The index into the global list of transactions grouped by amount in the Loki network.
    std::string tx_hash;                        // Several incoming transfers may share the same hash if they were in the same transaction.
    cryptonote::subaddress_index subaddr_index; // Major & minor index, account and subaddress index respectively.
    std::string key_image;                      // Key image for the incoming transfer's unspent output (empty unless verbose is true).

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(amount)
      KV_SERIALIZE(spent)
      KV_SERIALIZE(global_index)
      KV_SERIALIZE(tx_hash)
      KV_SERIALIZE(subaddr_index)
      KV_SERIALIZE(key_image)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  // Return a list of incoming transfers to the wallet.
  struct COMMAND_RPC_INCOMING_TRANSFERS
  {
    struct request_t
    {
      std::string transfer_type;          // "all": all the transfers, "available": only transfers which are not yet spent, OR "unavailable": only transfers which are already spent.
      uint32_t account_index;             // (Optional) Return transfers for this account. (defaults to 0)
      std::set<uint32_t> subaddr_indices; // (Optional) Return transfers sent to these subaddresses.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer_type)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(subaddr_indices)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<transfer_details> transfers; // List of information of the transfers details.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfers)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  //JSON RPC V2
  LOKI_RPC_DOC_INTROSPECT
  // Return the spend or view private key.
  struct COMMAND_RPC_QUERY_KEY
  {
    struct request_t
    {
      std::string key_type; // Which key to retrieve: "mnemonic" - the mnemonic seed (older wallets do not have one) OR "view_key" - the view key

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_type)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string key; //  The view key will be hex encoded, while the mnemonic will be a string of words.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Make an integrated address from the wallet address and a payment id.
  struct COMMAND_RPC_MAKE_INTEGRATED_ADDRESS
  {
    struct request_t
    {
      std::string standard_address; // (Optional, defaults to primary address) Destination public address.
      std::string payment_id;       // (Optional, defaults to a random ID) 16 characters hex encoded.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string integrated_address; // 
      std::string payment_id;         // Hex encoded.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
        KV_SERIALIZE(payment_id)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Retrieve the standard address and payment id corresponding to an integrated address.
  struct COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS
  {
    struct request_t
    {
      std::string integrated_address; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(integrated_address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string standard_address; // 
      std::string payment_id;       // 
      bool is_subaddress;           // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(standard_address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(is_subaddress)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Stops the wallet, storing the current state.
  struct COMMAND_RPC_STOP_WALLET
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Rescan the blockchain from scratch, losing any information 
  // which can not be recovered from the blockchain itself.
  // This includes destination addresses, tx secret keys, tx notes, etc.
  
  // Warning: This blocks the Wallet RPC executable until rescanning is complete.
  struct COMMAND_RPC_RESCAN_BLOCKCHAIN
  {
    struct request_t
    {
      bool hard; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(hard, false);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Set arbitrary string notes for transactions.
  struct COMMAND_RPC_SET_TX_NOTES
  {
    struct request_t
    {
      std::list<std::string> txids; // Transaction ids.
      std::list<std::string> notes; // Notes for the transactions.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get string notes for transactions.
  struct COMMAND_RPC_GET_TX_NOTES
  {
    struct request_t
    {
      std::list<std::string> txids; // Transaction ids.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txids)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<std::string> notes; // Notes for the transactions.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(notes)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Set arbitrary attribute.
  struct COMMAND_RPC_SET_ATTRIBUTE
  {
    struct request_t
    {
      std::string key;   // Attribute name.
      std::string value; // Attribute value.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get attribute value by name.
  struct COMMAND_RPC_GET_ATTRIBUTE
  {
    struct request_t
    {

      std::string key; // Attribute name.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string value; // Attribute value.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(value)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get transaction secret key from transaction id.
  struct COMMAND_RPC_GET_TX_KEY
  {
    struct request_t
    {
      std::string txid; // Transaction id.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_key; // Transaction secret key.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_key)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Check a transaction in the blockchain with its secret key.
  struct COMMAND_RPC_CHECK_TX_KEY
  {
    struct request_t
    {
      std::string txid;    // Transaction id.
      std::string tx_key;  // Transaction secret key.
      std::string address; // Destination public address of the transaction.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t received;      // Amount of the transaction.
      bool in_pool;           // States if the transaction is still in pool or has been added to a block.
      uint64_t confirmations; // Number of block mined after the one with the transaction.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(received)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(confirmations)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get transaction signature to prove it.
  struct COMMAND_RPC_GET_TX_PROOF
  {
    struct request_t
    {
      std::string txid;    // Transaction id.
      std::string address; // Destination public address of the transaction.
      std::string message; // (Optional) add a message to the signature to further authenticate the prooving process.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signature; // Transaction signature.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Prove a transaction by checking its signature.
  struct COMMAND_RPC_CHECK_TX_PROOF
  {
    struct request_t
    {
      std::string txid;      // Transaction id.
      std::string address;   // Destination public address of the transaction.
      std::string message;   // (Optional) Should be the same message used in `get_tx_proof`.
      std::string signature; // Transaction signature to confirm.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool good;              // States if the inputs proves the transaction.
      uint64_t received;      // Amount of the transaction.
      bool in_pool;           // States if the transaction is still in pool or has been added to a block.
      uint64_t confirmations; // Number of block mined after the one with the transaction.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
        KV_SERIALIZE(received)
        KV_SERIALIZE(in_pool)
        KV_SERIALIZE(confirmations)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // 
  struct transfer_entry
  {
    std::string txid;                             // Transaction ID for this transfer.
    std::string payment_id;                       // Payment ID for this transfer.
    uint64_t height;                              // Height of the first block that confirmed this transfer (0 if not mined yet).
    uint64_t timestamp;                           // UNIX timestamp for when this transfer was first confirmed in a block (or timestamp submission if not mined yet).
    uint64_t amount;                              // Amount transferred.
    uint64_t fee;                                 // Transaction fee for this transfer.
    std::string note;                             // Note about this transfer.
    std::list<transfer_destination> destinations; // Array of transfer destinations.
    std::string type;                             // Type of transfer, one of the following: "in", "out", "pending", "failed", "pool".
    uint64_t unlock_time;                         // Number of blocks until transfer is safely spendable.
    cryptonote::subaddress_index subaddr_index;   // Major & minor index, account and subaddress index respectively.
    std::string address;                          // Address that transferred the funds.
    bool double_spend_seen;                       // True if the key image(s) for the transfer have been seen before.
    uint64_t confirmations;                       // Number of block mined since the block containing this transaction (or block height at which the transaction should be added to a block if not yet confirmed).
    uint64_t suggested_confirmations_threshold;   // Estimation of the confirmations needed for the transaction to be included in a block.

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(txid);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(height);
      KV_SERIALIZE(timestamp);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(fee);
      KV_SERIALIZE(note);
      KV_SERIALIZE(destinations);
      KV_SERIALIZE(type);
      KV_SERIALIZE(unlock_time)
      KV_SERIALIZE(subaddr_index);
      KV_SERIALIZE(address);
      KV_SERIALIZE(double_spend_seen)
      KV_SERIALIZE_OPT(confirmations, (uint64_t)0)
      KV_SERIALIZE_OPT(suggested_confirmations_threshold, (uint64_t)0)
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  // Generate a signature to prove a spend. Unlike proving a transaction, it does not requires the destination public address.
  struct COMMAND_RPC_GET_SPEND_PROOF
  {
    struct request_t
    {
      std::string txid;    // Transaction id.
      std::string message; // (Optional) add a message to the signature to further authenticate the prooving process.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signature; // Spend signature.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Prove a spend using a signature. Unlike proving a transaction, it does not requires the destination public address.
  struct COMMAND_RPC_CHECK_SPEND_PROOF
  {
    struct request_t
    {
      std::string txid;      // Transaction id.
      std::string message;   // (Optional) Should be the same message used in `get_spend_proof`.
      std::string signature; // Spend signature to confirm.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool good; // States if the inputs proves the spend.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Generate a signature to prove of an available amount in a wallet.
  struct COMMAND_RPC_GET_RESERVE_PROOF
  {
    struct request_t
    {
      bool all;               // Proves all wallet balance to be disposable.
      uint32_t account_index; // Specify the account from witch to prove reserve. (ignored if all is set to true)
      uint64_t amount;        // Amount (in atomic units) to prove the account has for reserve. (ignored if all is set to true)
      std::string message;    // (Optional) add a message to the signature to further authenticate the prooving process.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(all)
        KV_SERIALIZE(account_index)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(message)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signature; // Reserve signature.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Proves a wallet has a disposable reserve using a signature.
  struct COMMAND_RPC_CHECK_RESERVE_PROOF
  {
    struct request_t
    {
      std::string address;   // Public address of the wallet.
      std::string message;   // (Optional) Should be the same message used in get_reserve_proof.
      std::string signature; // Reserve signature to confirm.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(message)
        KV_SERIALIZE(signature)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool good;      // States if the inputs proves the reserve.
      uint64_t total; //
      uint64_t spent; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good)
        KV_SERIALIZE(total)
        KV_SERIALIZE(spent)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Returns a list of transfers.
  struct COMMAND_RPC_GET_TRANSFERS
  {
    struct request_t
    {
      bool in;                            // (Optional) Include incoming transfers.
      bool out;                           // (Optional) Include outgoing transfers.
      bool pending;                       // (Optional) Include pending transfers.
      bool failed;                        // (Optional) Include failed transfers.
      bool pool;                          // (Optional) Include transfers from the daemon's transaction pool.

      bool filter_by_height;              // (Optional) Filter transfers by block height.
      uint64_t min_height;                // (Optional) Minimum block height to scan for transfers, if filtering by height is enabled.
      uint64_t max_height;                // (Optional) Maximum block height to scan for transfers, if filtering by height is enabled (defaults to max block height).
      uint32_t account_index;             // (Optional) Index of the account to query for transfers. (defaults to 0)
      std::set<uint32_t> subaddr_indices; // (Optional) List of subaddress indices to query for transfers. (defaults to 0)
      bool all_accounts;                  // If true, return transfers for all accounts, subaddr_indices and account_index are ignored

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
        KV_SERIALIZE(filter_by_height);
        KV_SERIALIZE(min_height);
        KV_SERIALIZE_OPT(max_height, (uint64_t)CRYPTONOTE_MAX_BLOCK_NUMBER);
        KV_SERIALIZE(account_index);
        KV_SERIALIZE(subaddr_indices);
        KV_SERIALIZE_OPT(all_accounts, false);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<transfer_entry> in;      // 
      std::list<transfer_entry> out;     //
      std::list<transfer_entry> pending; //
      std::list<transfer_entry> failed;  //
      std::list<transfer_entry> pool;    // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(in);
        KV_SERIALIZE(out);
        KV_SERIALIZE(pending);
        KV_SERIALIZE(failed);
        KV_SERIALIZE(pool);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Show information about a transfer to/from this address.
  struct COMMAND_RPC_GET_TRANSFER_BY_TXID
  {
    struct request_t
    {
      std::string txid;       // Transaction ID used to find the transfer.
      uint32_t account_index; // (Optional) Index of the account to query for the transfer.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(txid);
        KV_SERIALIZE_OPT(account_index, (uint32_t)0)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      transfer_entry transfer;             // 
      std::list<transfer_entry> transfers; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(transfer);
        KV_SERIALIZE(transfers);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Sign a string.
  struct COMMAND_RPC_SIGN
  {
    struct request_t
    {
      std::string data; // Anything you need to sign.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string signature; // Signature generated against the "data" and the account public address.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Verify a signature on a string.
  struct COMMAND_RPC_VERIFY
  {
    struct request_t
    {
      std::string data;      // What should have been signed.
      std::string address;   // Public address of the wallet used to sign the data.
      std::string signature; // Signature generated by `sign` method.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(data);
        KV_SERIALIZE(address);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool good; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(good);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Export all outputs in hex format.
  struct COMMAND_RPC_EXPORT_OUTPUTS
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string outputs_data_hex; // Wallet outputs in hex format.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs_data_hex);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Import outputs in hex format.
  struct COMMAND_RPC_IMPORT_OUTPUTS
  {
    struct request_t
    {
      std::string outputs_data_hex; // Wallet outputs in hex format.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(outputs_data_hex);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t num_imported; // Number of outputs imported.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(num_imported);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Export a signed set of key images.
  struct COMMAND_RPC_EXPORT_KEY_IMAGES
  {
    struct request_t
    {
      bool requested_only; // Default `false`.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(requested_only, false);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct signed_key_image
    {
      std::string key_image; // 
      std::string signature; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      uint32_t offset;                                 //
      std::vector<signed_key_image> signed_key_images; //

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(offset);
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Import signed key images list and verify their spent status.
  struct COMMAND_RPC_IMPORT_KEY_IMAGES
  {
    struct signed_key_image
    {
      std::string key_image; // Key image of specific output
      std::string signature; // Transaction signature.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(key_image);
        KV_SERIALIZE(signature);
      END_KV_SERIALIZE_MAP()
    };

    struct request_t
    {
      uint32_t offset;
      std::vector<signed_key_image> signed_key_images;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(offset, (uint32_t)0);
        KV_SERIALIZE(signed_key_images);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t height;  
      uint64_t spent;   // Amount (in atomic units) spent from those key images.
      uint64_t unspent; // Amount (in atomic units) still available from those key images.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(height)
        KV_SERIALIZE(spent)
        KV_SERIALIZE(unspent)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  struct uri_spec
  {
    std::string address;        // Wallet address.
    std::string payment_id;     // (Optional) 16 or 64 character hexadecimal payment id.
    uint64_t amount;            // (Optional) the integer amount to receive, in atomic units.
    std::string tx_description; // (Optional) Description of the reason for the tx.
    std::string recipient_name; // (Optional) name of the payment recipient.

    BEGIN_KV_SERIALIZE_MAP()
      KV_SERIALIZE(address);
      KV_SERIALIZE(payment_id);
      KV_SERIALIZE(amount);
      KV_SERIALIZE(tx_description);
      KV_SERIALIZE(recipient_name);
    END_KV_SERIALIZE_MAP()
  };

  LOKI_RPC_DOC_INTROSPECT
  // Create a payment URI using the official URI spec.
  struct COMMAND_RPC_MAKE_URI
  {
    struct request_t: public uri_spec
    {
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string uri; // This contains all the payment input information as a properly formatted payment URI.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Parse a payment URI to get payment information.
  struct COMMAND_RPC_PARSE_URI
  {
    struct request_t
    {
      std::string uri; // This contains all the payment input information as a properly formatted payment URI.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uri_spec uri;                                // JSON object containing payment information:
      std::vector<std::string> unknown_parameters; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(uri);
        KV_SERIALIZE(unknown_parameters);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Add an entry to the address book.
  struct COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY
  {
    struct request_t
    {
      std::string address;     // Public address of the entry.
      std::string payment_id;  // (Optional), defaults to "0000000000000000000000000000000000000000000000000000000000000000".
      std::string description; // (Optional), defaults to "".

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t index; // The index of the address book entry.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Retrieves entries from the address book.
  struct COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY
  {
    struct request_t
    {
      std::list<uint64_t> entries; // Indices of the requested address book entries.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct entry
    {
      uint64_t index;          // Index of entry.
      std::string address;     // Public address of the entry
      std::string payment_id;  // (Optional) 64-character hex string to identify a transaction.
      std::string description; // Description of this address entry.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index)
        KV_SERIALIZE(address)
        KV_SERIALIZE(payment_id)
        KV_SERIALIZE(description)
      END_KV_SERIALIZE_MAP()
    };

    struct response_t
    {
      std::vector<entry> entries; // List of address book entries information.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(entries)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Delete an entry from the address book.
  struct COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY
  {
    struct request_t
    {
      uint64_t index; // The index of the address book entry.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(index);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Rescan the blockchain for spent outputs.
  struct COMMAND_RPC_RESCAN_SPENT
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Refresh a wallet after openning.
  struct COMMAND_RPC_REFRESH
  {
    struct request_t
    {
      uint64_t start_height; // (Optional) The block height from which to start refreshing.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(start_height, (uint64_t) 0)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t blocks_fetched; // Number of new blocks scanned.
      bool received_money;     // States if transactions to the wallet have been found in the blocks.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(blocks_fetched);
        KV_SERIALIZE(received_money);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Start mining in the loki daemon.
  struct COMMAND_RPC_START_MINING
  {
    struct request_t
    {
      uint64_t    threads_count;        // Number of threads created for mining.
      bool        do_background_mining; // Allow to start the miner in smart mining mode.
      bool        ignore_battery;       // Ignore battery status (for smart mining only).

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(threads_count)
        KV_SERIALIZE(do_background_mining)        
        KV_SERIALIZE(ignore_battery)        
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Stop mining in the loki daemon.
  struct COMMAND_RPC_STOP_MINING
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get a list of available languages for your wallet's seed.
  struct COMMAND_RPC_GET_LANGUAGES
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::vector<std::string> languages; // List of available languages.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(languages)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Create a new wallet. You need to have set the argument "'–wallet-dir" when launching loki-wallet-rpc to make this work.
  struct COMMAND_RPC_CREATE_WALLET
  {
    struct request_t
    {
      std::string filename; // Set the wallet file name.
      std::string password; // (Optional) Set the password to protect the wallet.
      std::string language; // Language for your wallets' seed.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
        KV_SERIALIZE(language)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Open a wallet. You need to have set the argument "–-wallet-dir" when launching loki-wallet-rpc to make this work.
  // The wallet rpc executable may only open wallet files within the same directory as wallet-dir, otherwise use the
  // "--wallet-file" flag to open specific wallets.
  struct COMMAND_RPC_OPEN_WALLET
  {
    struct request_t
    {
      std::string filename; // Wallet name stored in "–-wallet-dir".
      std::string password; // (Optional) only needed if the wallet has a password defined.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(filename)
        KV_SERIALIZE(password)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Close the currently opened wallet, after trying to save it.
  struct COMMAND_RPC_CLOSE_WALLET
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Change a wallet password.
  struct COMMAND_RPC_CHANGE_WALLET_PASSWORD
  {
    struct request_t
    {
      std::string old_password; // (Optional) Current wallet password, if defined.
      std::string new_password; // (Optional) New wallet password, if not blank.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(old_password)
        KV_SERIALIZE(new_password)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // 
  struct COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET
  {
    struct request_t
    {
      uint64_t restore_height; // Height in which to start scanning the blockchain for transactions into and out of this Wallet.
      std::string filename;    // Set the name of the Wallet.
      std::string seed;        // Mnemonic seed of wallet (25 words).
      std::string seed_offset; // 
      std::string password;    // Set password for Wallet.
      std::string language;    // Set language for the wallet.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(restore_height, (uint64_t)0)
        KV_SERIALIZE(filename)
        KV_SERIALIZE(seed)
        KV_SERIALIZE(seed_offset)
        KV_SERIALIZE(password)
        KV_SERIALIZE(language)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string address; // Public address of wallet.
      std::string seed;    // Seed of wallet.
      std::string info;    // Wallet information.
      bool was_deprecated; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(seed)
        KV_SERIALIZE(info)
        KV_SERIALIZE(was_deprecated)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  LOKI_RPC_DOC_INTROSPECT
  // Check if a wallet is a multisig one.
  struct COMMAND_RPC_IS_MULTISIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool multisig;      // States if the wallet is multisig.
      bool ready;         // 
      uint32_t threshold; // Amount of signature needed to sign a transfer.
      uint32_t total;     // Total amount of signature in the multisig wallet.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig)
        KV_SERIALIZE(ready)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(total)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Prepare a wallet for multisig by generating a multisig string to share with peers.
  struct COMMAND_RPC_PREPARE_MULTISIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string multisig_info; // Multisig string to share with peers to create the multisig wallet.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Make a wallet multisig by importing peers multisig string.
  struct COMMAND_RPC_MAKE_MULTISIG
  {
    struct request_t
    {
      std::vector<std::string> multisig_info; // List of multisig string from peers.
      uint32_t threshold;                     // Amount of signatures needed to sign a transfer. Must be less or equal than the amount of signature in `multisig_info`.
      std::string password;                   // Wallet password.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(multisig_info)
        KV_SERIALIZE(threshold)
        KV_SERIALIZE(password)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string address;       // Multisig wallet address.
      std::string multisig_info; // Multisig string to share with peers to create the multisig wallet (extra step for N-1/N wallets).

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Export multisig info for other participants.
  struct COMMAND_RPC_EXPORT_MULTISIG
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string info; // Multisig info in hex format for other participants.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Import multisig info from other participants.
  struct COMMAND_RPC_IMPORT_MULTISIG
  {
    struct request_t
    {
      std::vector<std::string> info; // List of multisig info in hex format from other participants.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint64_t n_outputs; // Number of outputs signed with those multisig info.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(n_outputs)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Turn this wallet into a multisig wallet, extra step for N-1/N wallets.
  struct COMMAND_RPC_FINALIZE_MULTISIG
  {
    struct request_t
    {
      std::string password;                   // Wallet password.
      std::vector<std::string> multisig_info; // List of multisig string from peers.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(password)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string address; // Multisig wallet address.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // 
  struct COMMAND_RPC_EXCHANGE_MULTISIG_KEYS
  {
    struct request_t
    {
      std::string password;                   // Wallet password.
      std::vector<std::string> multisig_info; // List of multisig string from peers.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(password)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string address;       // Multisig wallet address.
      std::string multisig_info; // Multisig string to share with peers to create the multisig wallet.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE(multisig_info)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Sign a transaction in multisig.
  struct COMMAND_RPC_SIGN_MULTISIG
  {
    struct request_t
    {
      std::string tx_data_hex; // Multisig transaction in hex format, as returned by transfer under `multisig_txset`.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_data_hex;             // Multisig transaction in hex format.
      std::list<std::string> tx_hash_list; // List of transaction Hash.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
        KV_SERIALIZE(tx_hash_list)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Submit a signed multisig transaction.
  struct COMMAND_RPC_SUBMIT_MULTISIG
  {
    struct request_t
    {
      std::string tx_data_hex; // Multisig transaction in hex format, as returned by sign_multisig under tx_data_hex.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_data_hex)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::list<std::string> tx_hash_list; // List of transaction hash.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash_list)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Get RPC version Major & Minor integer-format, where Major is the first 16 bits and Minor the last 16 bits.
  struct COMMAND_RPC_GET_VERSION
  {
    struct request_t
    {
      BEGIN_KV_SERIALIZE_MAP()
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      uint32_t version; // RPC version, formatted with Major * 2^16 + Minor(Major encoded over the first 16 bits, and Minor over the last 16 bits).

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(version)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Stake for Service Node.
  struct COMMAND_RPC_STAKE
  {
    struct request_t
    {
      std::string        destination;      // Primary Public address that the rewards will go to.
      uint64_t           amount;           // Amount of Loki to stake in atomic units.
      std::set<uint32_t> subaddr_indices;  // (Optional) Transfer from this set of subaddresses. (Defaults to 0)
      std::string        service_node_key; // Service Node Public Address.
      uint32_t           priority;         // Set a priority for the transaction. Accepted Values are: 0-3 for: default, unimportant, normal, elevated, priority.
      bool               get_tx_key;       // (Optional) Return the transaction key after sending.
      bool               do_not_relay;     // (Optional) If true, the newly created transaction will not be relayed to the loki network. (Defaults to false)
      bool               get_tx_hex;       // Return the transaction as hex string after sending (Defaults to false)
      bool               get_tx_metadata;  // Return the metadata needed to relay the transaction. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE_OPT(subaddr_indices, {});
        KV_SERIALIZE    (destination);
        KV_SERIALIZE    (amount);
        KV_SERIALIZE    (service_node_key);
        KV_SERIALIZE_OPT(priority,        (uint32_t)0);
        KV_SERIALIZE    (get_tx_key)
        KV_SERIALIZE_OPT(do_not_relay,    false)
        KV_SERIALIZE_OPT(get_tx_hex,      false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_hash;        // Publically searchable transaction hash.
      std::string tx_key;         // Transaction key if `get_tx_key` is `true`, otherwise, blank string.
      uint64_t amount;            // Amount transferred for the transaction in atomic units.
      uint64_t fee;               // Value in atomic units of the fee charged for the tx.
      std::string tx_blob;        // Raw transaction represented as hex string, if get_tx_hex is true.
      std::string tx_metadata;    // Set of transaction metadata needed to relay this transfer later, if `get_tx_metadata` is `true`.
      std::string multisig_txset; // Set of multisig transactions in the process of being signed (empty for non-multisig).
      std::string unsigned_txset; // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Register Service Node.
  struct COMMAND_RPC_REGISTER_SERVICE_NODE
  {
    struct request_t
    {
      std::string register_service_node_str; // String supplied by the prepare_registration command.
      bool        get_tx_key;                // (Optional) Return the transaction key after sending.
      bool        do_not_relay;              // (Optional) If true, the newly created transaction will not be relayed to the loki network. (Defaults to false)
      bool        get_tx_hex;                // Return the transaction as hex string after sending (Defaults to false)
      bool        get_tx_metadata;           // Return the metadata needed to relay the transaction. (Defaults to false)

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(register_service_node_str);
        KV_SERIALIZE(get_tx_key)
        KV_SERIALIZE_OPT(do_not_relay,    false)
        KV_SERIALIZE_OPT(get_tx_hex,      false)
        KV_SERIALIZE_OPT(get_tx_metadata, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      std::string tx_hash;        // Publically searchable transaction hash.
      std::string tx_key;         // Transaction key if get_tx_key is true, otherwise, blank string.
      uint64_t amount;            // Amount transferred for the transaction in atomic units.
      uint64_t fee;               // Value in atomic units of the fee charged for the tx.
      std::string tx_blob;        // Raw transaction represented as hex string, if get_tx_hex is true.
      std::string tx_metadata;    // Set of transaction metadata needed to relay this transfer later, if `get_tx_metadata` is `true`.
      std::string multisig_txset; // Set of multisig transactions in the process of being signed (empty for non-multisig).
      std::string unsigned_txset; // Set of unsigned tx for cold-signing purposes.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(tx_hash)
        KV_SERIALIZE(tx_key)
        KV_SERIALIZE(amount)
        KV_SERIALIZE(fee)
        KV_SERIALIZE(tx_blob)
        KV_SERIALIZE(tx_metadata)
        KV_SERIALIZE(multisig_txset)
        KV_SERIALIZE(unsigned_txset)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };

  LOKI_RPC_DOC_INTROSPECT
  // Request to unlock stake by deregistering Service Node.
  struct COMMAND_RPC_REQUEST_STAKE_UNLOCK
  {
    struct request_t
    {
      std::string service_node_key; // Service Node Public Key.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(service_node_key);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool unlocked;   // States if stake has been unlocked.
      std::string msg; // Information on the unlocking process.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(unlocked)
        KV_SERIALIZE(msg)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  LOKI_RPC_DOC_INTROSPECT
  // Check if Service Node can unlock it's stake.
  struct COMMAND_RPC_CAN_REQUEST_STAKE_UNLOCK
  {
    struct request_t
    {
      std::string service_node_key; // Service node public address.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(service_node_key);
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool can_unlock; // States if the stake can be locked.
      std::string msg; // Information on the unlocking process.

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(can_unlock)
        KV_SERIALIZE(msg)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
  
  LOKI_RPC_DOC_INTROSPECT
  // Parse an address to validate if it's a valid Loki address.
  struct COMMAND_RPC_VALIDATE_ADDRESS
  {
    struct request_t
    {
      std::string address;  // Address to check.
      bool any_net_type;    // 
      bool allow_openalias; // 

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(address)
        KV_SERIALIZE_OPT(any_net_type, false)
        KV_SERIALIZE_OPT(allow_openalias, false)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<request_t> request;

    struct response_t
    {
      bool valid;                    // States if it is a valid Loki address.
      bool integrated;               // States if it is an integrated address.
      bool subaddress;               // States if it is a subaddress.
      std::string nettype;           // States if the nettype is mainet, testnet, stagenet.
      std::string openalias_address;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(valid)
        KV_SERIALIZE(integrated)
        KV_SERIALIZE(subaddress)
        KV_SERIALIZE(nettype)
        KV_SERIALIZE(openalias_address)
      END_KV_SERIALIZE_MAP()
    };
    typedef epee::misc_utils::struct_init<response_t> response;
  };
}
}
