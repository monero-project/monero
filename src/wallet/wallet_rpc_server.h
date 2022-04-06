// Copyright (c) 2014-2022, The Monero Project
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

#pragma  once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>
#include "common/util.h"
#include "net/http_server_impl_base.h"
#include "math_helper.h"
#include "wallet_rpc_server_commands_defs.h"
#include "wallet2.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "wallet.rpc"

namespace tools
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class wallet_rpc_server: public epee::http_server_impl_base<wallet_rpc_server>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    static const char* tr(const char* str);

    wallet_rpc_server();
    ~wallet_rpc_server();

    bool init(const boost::program_options::variables_map *vm);
    bool run();
    void stop();
    void set_wallet(wallet2 *cr);

  private:

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("get_balance",        on_getbalance,         wallet_rpc::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("get_address",        on_getaddress,         wallet_rpc::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("get_address_index",  on_getaddress_index,   wallet_rpc::COMMAND_RPC_GET_ADDRESS_INDEX)
        MAP_JON_RPC_WE("getbalance",         on_getbalance,         wallet_rpc::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",         on_getaddress,         wallet_rpc::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("create_address",     on_create_address,     wallet_rpc::COMMAND_RPC_CREATE_ADDRESS)
        MAP_JON_RPC_WE("label_address",      on_label_address,      wallet_rpc::COMMAND_RPC_LABEL_ADDRESS)
        MAP_JON_RPC_WE("get_accounts",       on_get_accounts,       wallet_rpc::COMMAND_RPC_GET_ACCOUNTS)
        MAP_JON_RPC_WE("create_account",     on_create_account,     wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT)
        MAP_JON_RPC_WE("label_account",      on_label_account,      wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT)
        MAP_JON_RPC_WE("get_account_tags",   on_get_account_tags,   wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS)
        MAP_JON_RPC_WE("tag_accounts",       on_tag_accounts,       wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS)
        MAP_JON_RPC_WE("untag_accounts",     on_untag_accounts,     wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS)
        MAP_JON_RPC_WE("set_account_tag_description", on_set_account_tag_description, wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION)
        MAP_JON_RPC_WE("get_height",         on_getheight,          wallet_rpc::COMMAND_RPC_GET_HEIGHT)
        MAP_JON_RPC_WE("getheight",          on_getheight,          wallet_rpc::COMMAND_RPC_GET_HEIGHT)
        MAP_JON_RPC_WE("freeze",             on_freeze,             wallet_rpc::COMMAND_RPC_FREEZE)
        MAP_JON_RPC_WE("thaw",               on_thaw,               wallet_rpc::COMMAND_RPC_THAW)
        MAP_JON_RPC_WE("frozen",             on_frozen,             wallet_rpc::COMMAND_RPC_FROZEN)
        MAP_JON_RPC_WE("transfer",           on_transfer,           wallet_rpc::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("transfer_split",     on_transfer_split,     wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT)
        MAP_JON_RPC_WE("sign_transfer",      on_sign_transfer,      wallet_rpc::COMMAND_RPC_SIGN_TRANSFER)
        MAP_JON_RPC_WE("describe_transfer",  on_describe_transfer,  wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER)
        MAP_JON_RPC_WE("submit_transfer",    on_submit_transfer,    wallet_rpc::COMMAND_RPC_SUBMIT_TRANSFER)
        MAP_JON_RPC_WE("sweep_dust",         on_sweep_dust,         wallet_rpc::COMMAND_RPC_SWEEP_DUST)
        MAP_JON_RPC_WE("sweep_unmixable",    on_sweep_dust,         wallet_rpc::COMMAND_RPC_SWEEP_DUST)
        MAP_JON_RPC_WE("sweep_all",          on_sweep_all,          wallet_rpc::COMMAND_RPC_SWEEP_ALL)
        MAP_JON_RPC_WE("sweep_single",       on_sweep_single,       wallet_rpc::COMMAND_RPC_SWEEP_SINGLE)
        MAP_JON_RPC_WE("relay_tx",           on_relay_tx,           wallet_rpc::COMMAND_RPC_RELAY_TX)
        MAP_JON_RPC_WE("store",              on_store,              wallet_rpc::COMMAND_RPC_STORE)
        MAP_JON_RPC_WE("get_payments",       on_get_payments,       wallet_rpc::COMMAND_RPC_GET_PAYMENTS)
        MAP_JON_RPC_WE("get_bulk_payments",  on_get_bulk_payments,  wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS)
        MAP_JON_RPC_WE("incoming_transfers", on_incoming_transfers, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS)
        MAP_JON_RPC_WE("query_key",         on_query_key,         wallet_rpc::COMMAND_RPC_QUERY_KEY)
        MAP_JON_RPC_WE("make_integrated_address", on_make_integrated_address, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("split_integrated_address", on_split_integrated_address, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS)
        MAP_JON_RPC_WE("stop_wallet",        on_stop_wallet,        wallet_rpc::COMMAND_RPC_STOP_WALLET)
        MAP_JON_RPC_WE("rescan_blockchain",  on_rescan_blockchain,  wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN)
        MAP_JON_RPC_WE("set_tx_notes",       on_set_tx_notes,       wallet_rpc::COMMAND_RPC_SET_TX_NOTES)
        MAP_JON_RPC_WE("get_tx_notes",       on_get_tx_notes,       wallet_rpc::COMMAND_RPC_GET_TX_NOTES)
        MAP_JON_RPC_WE("set_attribute",      on_set_attribute,      wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE)
        MAP_JON_RPC_WE("get_attribute",      on_get_attribute,      wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE)
        MAP_JON_RPC_WE("get_tx_key",         on_get_tx_key,         wallet_rpc::COMMAND_RPC_GET_TX_KEY)
        MAP_JON_RPC_WE("check_tx_key",       on_check_tx_key,       wallet_rpc::COMMAND_RPC_CHECK_TX_KEY)
        MAP_JON_RPC_WE("get_tx_proof",       on_get_tx_proof,       wallet_rpc::COMMAND_RPC_GET_TX_PROOF)
        MAP_JON_RPC_WE("check_tx_proof",     on_check_tx_proof,     wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF)
        MAP_JON_RPC_WE("get_spend_proof",    on_get_spend_proof,    wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF)
        MAP_JON_RPC_WE("check_spend_proof",  on_check_spend_proof,  wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF)
        MAP_JON_RPC_WE("get_reserve_proof",    on_get_reserve_proof,    wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF)
        MAP_JON_RPC_WE("check_reserve_proof",  on_check_reserve_proof,  wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF)
        MAP_JON_RPC_WE("get_transfers",      on_get_transfers,      wallet_rpc::COMMAND_RPC_GET_TRANSFERS)
        MAP_JON_RPC_WE("get_transfer_by_txid", on_get_transfer_by_txid, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID)
        MAP_JON_RPC_WE("sign",               on_sign,               wallet_rpc::COMMAND_RPC_SIGN)
        MAP_JON_RPC_WE("verify",             on_verify,             wallet_rpc::COMMAND_RPC_VERIFY)
        MAP_JON_RPC_WE("export_outputs",     on_export_outputs,     wallet_rpc::COMMAND_RPC_EXPORT_OUTPUTS)
        MAP_JON_RPC_WE("import_outputs",     on_import_outputs,     wallet_rpc::COMMAND_RPC_IMPORT_OUTPUTS)
        MAP_JON_RPC_WE("export_key_images",  on_export_key_images,  wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES)
        MAP_JON_RPC_WE("import_key_images",  on_import_key_images,  wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES)
        MAP_JON_RPC_WE("make_uri",           on_make_uri,           wallet_rpc::COMMAND_RPC_MAKE_URI)
        MAP_JON_RPC_WE("parse_uri",          on_parse_uri,          wallet_rpc::COMMAND_RPC_PARSE_URI)
        MAP_JON_RPC_WE("get_address_book",   on_get_address_book,   wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("add_address_book",   on_add_address_book,   wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("edit_address_book",  on_edit_address_book,  wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("delete_address_book",on_delete_address_book,wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("refresh",            on_refresh,            wallet_rpc::COMMAND_RPC_REFRESH)
        MAP_JON_RPC_WE("auto_refresh",       on_auto_refresh,       wallet_rpc::COMMAND_RPC_AUTO_REFRESH)
        MAP_JON_RPC_WE("scan_tx",            on_scan_tx,            wallet_rpc::COMMAND_RPC_SCAN_TX)
        MAP_JON_RPC_WE("rescan_spent",       on_rescan_spent,       wallet_rpc::COMMAND_RPC_RESCAN_SPENT)
        MAP_JON_RPC_WE("start_mining",       on_start_mining,       wallet_rpc::COMMAND_RPC_START_MINING)
        MAP_JON_RPC_WE("stop_mining",        on_stop_mining,        wallet_rpc::COMMAND_RPC_STOP_MINING)
        MAP_JON_RPC_WE("get_languages",      on_get_languages,      wallet_rpc::COMMAND_RPC_GET_LANGUAGES)
        MAP_JON_RPC_WE("create_wallet",      on_create_wallet,      wallet_rpc::COMMAND_RPC_CREATE_WALLET)
        MAP_JON_RPC_WE("open_wallet",        on_open_wallet,        wallet_rpc::COMMAND_RPC_OPEN_WALLET)
        MAP_JON_RPC_WE("close_wallet",       on_close_wallet,       wallet_rpc::COMMAND_RPC_CLOSE_WALLET)
        MAP_JON_RPC_WE("change_wallet_password",        on_change_wallet_password,        wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD)
        MAP_JON_RPC_WE("generate_from_keys", on_generate_from_keys, wallet_rpc::COMMAND_RPC_GENERATE_FROM_KEYS)
        MAP_JON_RPC_WE("restore_deterministic_wallet",      on_restore_deterministic_wallet,      wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET)
        MAP_JON_RPC_WE("is_multisig",        on_is_multisig,        wallet_rpc::COMMAND_RPC_IS_MULTISIG)
        MAP_JON_RPC_WE("prepare_multisig",   on_prepare_multisig,   wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG)
        MAP_JON_RPC_WE("make_multisig",      on_make_multisig,      wallet_rpc::COMMAND_RPC_MAKE_MULTISIG)
        MAP_JON_RPC_WE("export_multisig_info", on_export_multisig,  wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG)
        MAP_JON_RPC_WE("import_multisig_info", on_import_multisig,  wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG)
        MAP_JON_RPC_WE("finalize_multisig",  on_finalize_multisig,  wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG)
        MAP_JON_RPC_WE("exchange_multisig_keys",  on_exchange_multisig_keys,  wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS)
        MAP_JON_RPC_WE("sign_multisig",      on_sign_multisig,      wallet_rpc::COMMAND_RPC_SIGN_MULTISIG)
        MAP_JON_RPC_WE("submit_multisig",    on_submit_multisig,    wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG)
        MAP_JON_RPC_WE("validate_address",   on_validate_address,   wallet_rpc::COMMAND_RPC_VALIDATE_ADDRESS)
        MAP_JON_RPC_WE("set_daemon",         on_set_daemon,         wallet_rpc::COMMAND_RPC_SET_DAEMON)
        MAP_JON_RPC_WE("set_log_level",      on_set_log_level,      wallet_rpc::COMMAND_RPC_SET_LOG_LEVEL)
        MAP_JON_RPC_WE("set_log_categories", on_set_log_categories, wallet_rpc::COMMAND_RPC_SET_LOG_CATEGORIES)
        MAP_JON_RPC_WE("estimate_tx_size_and_weight", on_estimate_tx_size_and_weight, wallet_rpc::COMMAND_RPC_ESTIMATE_TX_SIZE_AND_WEIGHT)
        MAP_JON_RPC_WE("get_version",        on_get_version,        wallet_rpc::COMMAND_RPC_GET_VERSION)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

      //json_rpc
      bool on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_getaddress_index(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_INDEX::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_INDEX::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_create_address(const wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_CREATE_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_label_address(const wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_LABEL_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_accounts(const wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_create_account(const wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_CREATE_ACCOUNT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_label_account(const wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::request& req, wallet_rpc::COMMAND_RPC_LABEL_ACCOUNT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_account_tags(const wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::request& req, wallet_rpc::COMMAND_RPC_GET_ACCOUNT_TAGS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_tag_accounts(const wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_TAG_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_untag_accounts(const wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::request& req, wallet_rpc::COMMAND_RPC_UNTAG_ACCOUNTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_account_tag_description(const wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::request& req, wallet_rpc::COMMAND_RPC_SET_ACCOUNT_TAG_DESCRIPTION::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_getheight(const wallet_rpc::COMMAND_RPC_GET_HEIGHT::request& req, wallet_rpc::COMMAND_RPC_GET_HEIGHT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_freeze(const wallet_rpc::COMMAND_RPC_FREEZE::request& req, wallet_rpc::COMMAND_RPC_FREEZE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_thaw(const wallet_rpc::COMMAND_RPC_THAW::request& req, wallet_rpc::COMMAND_RPC_THAW::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_frozen(const wallet_rpc::COMMAND_RPC_FROZEN::request& req, wallet_rpc::COMMAND_RPC_FROZEN::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sign_transfer(const wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_SIGN_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_describe_transfer(const wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_DESCRIBE_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_submit_transfer(const wallet_rpc::COMMAND_RPC_SUBMIT_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_TRANSFER::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sweep_dust(const wallet_rpc::COMMAND_RPC_SWEEP_DUST::request& req, wallet_rpc::COMMAND_RPC_SWEEP_DUST::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sweep_all(const wallet_rpc::COMMAND_RPC_SWEEP_ALL::request& req, wallet_rpc::COMMAND_RPC_SWEEP_ALL::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sweep_single(const wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::request& req, wallet_rpc::COMMAND_RPC_SWEEP_SINGLE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_relay_tx(const wallet_rpc::COMMAND_RPC_RELAY_TX::request& req, wallet_rpc::COMMAND_RPC_RELAY_TX::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_split_integrated_address(const wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_stop_wallet(const wallet_rpc::COMMAND_RPC_STOP_WALLET::request& req, wallet_rpc::COMMAND_RPC_STOP_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_rescan_blockchain(const wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::request& req, wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_tx_notes(const wallet_rpc::COMMAND_RPC_SET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_SET_TX_NOTES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_tx_notes(const wallet_rpc::COMMAND_RPC_GET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_GET_TX_NOTES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_attribute(const wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_SET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_attribute(const wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::request& req, wallet_rpc::COMMAND_RPC_GET_ATTRIBUTE::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_tx_key(const wallet_rpc::COMMAND_RPC_GET_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_GET_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_check_tx_key(const wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_tx_proof(const wallet_rpc::COMMAND_RPC_GET_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_TX_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_check_tx_proof(const wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_TX_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_spend_proof(const wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_SPEND_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_check_spend_proof(const wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_SPEND_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_reserve_proof(const wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_GET_RESERVE_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_check_reserve_proof(const wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::request& req, wallet_rpc::COMMAND_RPC_CHECK_RESERVE_PROOF::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_transfers(const wallet_rpc::COMMAND_RPC_GET_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFERS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_transfer_by_txid(const wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sign(const wallet_rpc::COMMAND_RPC_SIGN::request& req, wallet_rpc::COMMAND_RPC_SIGN::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_verify(const wallet_rpc::COMMAND_RPC_VERIFY::request& req, wallet_rpc::COMMAND_RPC_VERIFY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_export_outputs(const wallet_rpc::COMMAND_RPC_EXPORT_OUTPUTS::request& req, wallet_rpc::COMMAND_RPC_EXPORT_OUTPUTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_import_outputs(const wallet_rpc::COMMAND_RPC_IMPORT_OUTPUTS::request& req, wallet_rpc::COMMAND_RPC_IMPORT_OUTPUTS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_export_key_images(const wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_import_key_images(const wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_make_uri(const wallet_rpc::COMMAND_RPC_MAKE_URI::request& req, wallet_rpc::COMMAND_RPC_MAKE_URI::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_parse_uri(const wallet_rpc::COMMAND_RPC_PARSE_URI::request& req, wallet_rpc::COMMAND_RPC_PARSE_URI::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_address_book(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_add_address_book(const wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_edit_address_book(const wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_EDIT_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_delete_address_book(const wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_refresh(const wallet_rpc::COMMAND_RPC_REFRESH::request& req, wallet_rpc::COMMAND_RPC_REFRESH::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_auto_refresh(const wallet_rpc::COMMAND_RPC_AUTO_REFRESH::request& req, wallet_rpc::COMMAND_RPC_AUTO_REFRESH::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_scan_tx(const wallet_rpc::COMMAND_RPC_SCAN_TX::request& req, wallet_rpc::COMMAND_RPC_SCAN_TX::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_rescan_spent(const wallet_rpc::COMMAND_RPC_RESCAN_SPENT::request& req, wallet_rpc::COMMAND_RPC_RESCAN_SPENT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_start_mining(const wallet_rpc::COMMAND_RPC_START_MINING::request& req, wallet_rpc::COMMAND_RPC_START_MINING::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_stop_mining(const wallet_rpc::COMMAND_RPC_STOP_MINING::request& req, wallet_rpc::COMMAND_RPC_STOP_MINING::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_languages(const wallet_rpc::COMMAND_RPC_GET_LANGUAGES::request& req, wallet_rpc::COMMAND_RPC_GET_LANGUAGES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_create_wallet(const wallet_rpc::COMMAND_RPC_CREATE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CREATE_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_open_wallet(const wallet_rpc::COMMAND_RPC_OPEN_WALLET::request& req, wallet_rpc::COMMAND_RPC_OPEN_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_close_wallet(const wallet_rpc::COMMAND_RPC_CLOSE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CLOSE_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_change_wallet_password(const wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::request& req, wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_generate_from_keys(const wallet_rpc::COMMAND_RPC_GENERATE_FROM_KEYS::request& req, wallet_rpc::COMMAND_RPC_GENERATE_FROM_KEYS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_restore_deterministic_wallet(const wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::request& req, wallet_rpc::COMMAND_RPC_RESTORE_DETERMINISTIC_WALLET::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_is_multisig(const wallet_rpc::COMMAND_RPC_IS_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IS_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_prepare_multisig(const wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_PREPARE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_make_multisig(const wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_MAKE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_export_multisig(const wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_EXPORT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_import_multisig(const wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_IMPORT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_finalize_multisig(const wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_FINALIZE_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_exchange_multisig_keys(const wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::request& req, wallet_rpc::COMMAND_RPC_EXCHANGE_MULTISIG_KEYS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_sign_multisig(const wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SIGN_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_submit_multisig(const wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::request& req, wallet_rpc::COMMAND_RPC_SUBMIT_MULTISIG::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_validate_address(const wallet_rpc::COMMAND_RPC_VALIDATE_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_VALIDATE_ADDRESS::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_daemon(const wallet_rpc::COMMAND_RPC_SET_DAEMON::request& req, wallet_rpc::COMMAND_RPC_SET_DAEMON::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_log_level(const wallet_rpc::COMMAND_RPC_SET_LOG_LEVEL::request& req, wallet_rpc::COMMAND_RPC_SET_LOG_LEVEL::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_set_log_categories(const wallet_rpc::COMMAND_RPC_SET_LOG_CATEGORIES::request& req, wallet_rpc::COMMAND_RPC_SET_LOG_CATEGORIES::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_estimate_tx_size_and_weight(const wallet_rpc::COMMAND_RPC_ESTIMATE_TX_SIZE_AND_WEIGHT::request& req, wallet_rpc::COMMAND_RPC_ESTIMATE_TX_SIZE_AND_WEIGHT::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);
      bool on_get_version(const wallet_rpc::COMMAND_RPC_GET_VERSION::request& req, wallet_rpc::COMMAND_RPC_GET_VERSION::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);

      //json rpc v2
      bool on_query_key(const wallet_rpc::COMMAND_RPC_QUERY_KEY::request& req, wallet_rpc::COMMAND_RPC_QUERY_KEY::response& res, epee::json_rpc::error& er, const connection_context *ctx = NULL);

      // helpers
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const crypto::hash &payment_id, const tools::wallet2::payment_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::confirmed_transfer_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::unconfirmed_transfer_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &payment_id, const tools::wallet2::pool_payment_details &pd);
      bool not_open(epee::json_rpc::error& er);
      void handle_rpc_exception(const std::exception_ptr& e, epee::json_rpc::error& er, int default_error_code);

      template<typename Ts, typename Tu, typename Tk>
      bool fill_response(std::vector<tools::wallet2::pending_tx> &ptx_vector,
          bool get_tx_key, Ts& tx_key, Tu &amount, Tu &fee, Tu &weight, std::string &multisig_txset, std::string &unsigned_txset, bool do_not_relay,
          Ts &tx_hash, bool get_tx_hex, Ts &tx_blob, bool get_tx_metadata, Ts &tx_metadata, Tk &spent_key_images, epee::json_rpc::error &er);

      bool validate_transfer(const std::list<wallet_rpc::transfer_destination>& destinations, const std::string& payment_id, std::vector<cryptonote::tx_destination_entry>& dsts, std::vector<uint8_t>& extra, bool at_least_one_destination, epee::json_rpc::error& er);

      void check_background_mining();

      wallet2 *m_wallet;
      std::string m_wallet_dir;
      tools::private_file rpc_login_file;
      std::atomic<bool> m_stop;
      bool m_restricted;
      const boost::program_options::variables_map *m_vm;
      uint32_t m_auto_refresh_period;
      boost::posix_time::ptime m_last_auto_refresh_time;
  };
}
