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

#pragma  once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>
#include "net/http_server_impl_base.h"
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
        MAP_JON_RPC_WE("getbalance",         on_getbalance,         wallet_rpc::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",         on_getaddress,         wallet_rpc::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("getheight",          on_getheight,          wallet_rpc::COMMAND_RPC_GET_HEIGHT)
        MAP_JON_RPC_WE("transfer",           on_transfer,           wallet_rpc::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("transfer_split",     on_transfer_split,     wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT)
        MAP_JON_RPC_WE("sweep_dust",         on_sweep_dust,         wallet_rpc::COMMAND_RPC_SWEEP_DUST)
        MAP_JON_RPC_WE("sweep_all",          on_sweep_all,          wallet_rpc::COMMAND_RPC_SWEEP_ALL)
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
        MAP_JON_RPC_WE("get_transfers",      on_get_transfers,      wallet_rpc::COMMAND_RPC_GET_TRANSFERS)
        MAP_JON_RPC_WE("get_transfer_by_txid", on_get_transfer_by_txid, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID)
        MAP_JON_RPC_WE("sign",               on_sign,               wallet_rpc::COMMAND_RPC_SIGN)
        MAP_JON_RPC_WE("verify",             on_verify,             wallet_rpc::COMMAND_RPC_VERIFY)
        MAP_JON_RPC_WE("export_key_images",  on_export_key_images,  wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES)
        MAP_JON_RPC_WE("import_key_images",  on_import_key_images,  wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES)
        MAP_JON_RPC_WE("make_uri",           on_make_uri,           wallet_rpc::COMMAND_RPC_MAKE_URI)
        MAP_JON_RPC_WE("parse_uri",          on_parse_uri,          wallet_rpc::COMMAND_RPC_PARSE_URI)
        MAP_JON_RPC_WE("get_address_book",   on_get_address_book,   wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("add_address_book",   on_add_address_book,   wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("delete_address_book",on_delete_address_book,wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY)
        MAP_JON_RPC_WE("rescan_spent",       on_rescan_spent,       wallet_rpc::COMMAND_RPC_RESCAN_SPENT)
        MAP_JON_RPC_WE("start_mining",       on_start_mining,       wallet_rpc::COMMAND_RPC_START_MINING)
        MAP_JON_RPC_WE("stop_mining",        on_stop_mining,        wallet_rpc::COMMAND_RPC_STOP_MINING)
        MAP_JON_RPC_WE("get_languages",      on_get_languages,      wallet_rpc::COMMAND_RPC_GET_LANGUAGES)
        MAP_JON_RPC_WE("create_wallet",      on_create_wallet,      wallet_rpc::COMMAND_RPC_CREATE_WALLET)
        MAP_JON_RPC_WE("open_wallet",        on_open_wallet,        wallet_rpc::COMMAND_RPC_OPEN_WALLET)
        MAP_JON_RPC_WE("change_wallet_password", on_change_wallet_password, wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

      //json_rpc
      bool on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er);
      bool on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er);
      bool on_getheight(const wallet_rpc::COMMAND_RPC_GET_HEIGHT::request& req, wallet_rpc::COMMAND_RPC_GET_HEIGHT::response& res, epee::json_rpc::error& er);
      bool validate_transfer(const std::list<wallet_rpc::transfer_destination> destinations, const std::string payment_id, std::vector<cryptonote::tx_destination_entry>& dsts, std::vector<uint8_t>& extra, epee::json_rpc::error& er);
      bool on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er);
      bool on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er);
      bool on_sweep_dust(const wallet_rpc::COMMAND_RPC_SWEEP_DUST::request& req, wallet_rpc::COMMAND_RPC_SWEEP_DUST::response& res, epee::json_rpc::error& er);
      bool on_sweep_all(const wallet_rpc::COMMAND_RPC_SWEEP_ALL::request& req, wallet_rpc::COMMAND_RPC_SWEEP_ALL::response& res, epee::json_rpc::error& er);
      bool on_make_integrated_address(const wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_MAKE_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er);
      bool on_split_integrated_address(const wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_SPLIT_INTEGRATED_ADDRESS::response& res, epee::json_rpc::error& er);
      bool on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er);
      bool on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er);
      bool on_get_bulk_payments(const wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_BULK_PAYMENTS::response& res, epee::json_rpc::error& er);
      bool on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er);
      bool on_stop_wallet(const wallet_rpc::COMMAND_RPC_STOP_WALLET::request& req, wallet_rpc::COMMAND_RPC_STOP_WALLET::response& res, epee::json_rpc::error& er);
      bool on_rescan_blockchain(const wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::request& req, wallet_rpc::COMMAND_RPC_RESCAN_BLOCKCHAIN::response& res, epee::json_rpc::error& er);
      bool on_set_tx_notes(const wallet_rpc::COMMAND_RPC_SET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_SET_TX_NOTES::response& res, epee::json_rpc::error& er);
      bool on_get_tx_notes(const wallet_rpc::COMMAND_RPC_GET_TX_NOTES::request& req, wallet_rpc::COMMAND_RPC_GET_TX_NOTES::response& res, epee::json_rpc::error& er);
      bool on_get_transfers(const wallet_rpc::COMMAND_RPC_GET_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFERS::response& res, epee::json_rpc::error& er);
      bool on_get_transfer_by_txid(const wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::request& req, wallet_rpc::COMMAND_RPC_GET_TRANSFER_BY_TXID::response& res, epee::json_rpc::error& er);
      bool on_sign(const wallet_rpc::COMMAND_RPC_SIGN::request& req, wallet_rpc::COMMAND_RPC_SIGN::response& res, epee::json_rpc::error& er);
      bool on_verify(const wallet_rpc::COMMAND_RPC_VERIFY::request& req, wallet_rpc::COMMAND_RPC_VERIFY::response& res, epee::json_rpc::error& er);
      bool on_export_key_images(const wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_EXPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er);
      bool on_import_key_images(const wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::request& req, wallet_rpc::COMMAND_RPC_IMPORT_KEY_IMAGES::response& res, epee::json_rpc::error& er);
      bool on_make_uri(const wallet_rpc::COMMAND_RPC_MAKE_URI::request& req, wallet_rpc::COMMAND_RPC_MAKE_URI::response& res, epee::json_rpc::error& er);
      bool on_parse_uri(const wallet_rpc::COMMAND_RPC_PARSE_URI::request& req, wallet_rpc::COMMAND_RPC_PARSE_URI::response& res, epee::json_rpc::error& er);
      bool on_get_address_book(const wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er);
      bool on_add_address_book(const wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_ADD_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er);
      bool on_delete_address_book(const wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::request& req, wallet_rpc::COMMAND_RPC_DELETE_ADDRESS_BOOK_ENTRY::response& res, epee::json_rpc::error& er);
      bool on_rescan_spent(const wallet_rpc::COMMAND_RPC_RESCAN_SPENT::request& req, wallet_rpc::COMMAND_RPC_RESCAN_SPENT::response& res, epee::json_rpc::error& er);
      bool on_start_mining(const wallet_rpc::COMMAND_RPC_START_MINING::request& req, wallet_rpc::COMMAND_RPC_START_MINING::response& res, epee::json_rpc::error& er);
      bool on_stop_mining(const wallet_rpc::COMMAND_RPC_STOP_MINING::request& req, wallet_rpc::COMMAND_RPC_STOP_MINING::response& res, epee::json_rpc::error& er);
      bool on_get_languages(const wallet_rpc::COMMAND_RPC_GET_LANGUAGES::request& req, wallet_rpc::COMMAND_RPC_GET_LANGUAGES::response& res, epee::json_rpc::error& er);
      bool on_create_wallet(const wallet_rpc::COMMAND_RPC_CREATE_WALLET::request& req, wallet_rpc::COMMAND_RPC_CREATE_WALLET::response& res, epee::json_rpc::error& er);
      bool on_open_wallet(const wallet_rpc::COMMAND_RPC_OPEN_WALLET::request& req, wallet_rpc::COMMAND_RPC_OPEN_WALLET::response& res, epee::json_rpc::error& er);
      bool on_change_wallet_password(const wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::request& req, wallet_rpc::COMMAND_RPC_CHANGE_WALLET_PASSWORD::response& res, epee::json_rpc::error& er);

      //json rpc v2
      bool on_query_key(const wallet_rpc::COMMAND_RPC_QUERY_KEY::request& req, wallet_rpc::COMMAND_RPC_QUERY_KEY::response& res, epee::json_rpc::error& er);

      // helpers
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const crypto::hash &payment_id, const tools::wallet2::payment_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::confirmed_transfer_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &txid, const tools::wallet2::unconfirmed_transfer_details &pd);
      void fill_transfer_entry(tools::wallet_rpc::transfer_entry &entry, const crypto::hash &payment_id, const tools::wallet2::payment_details &pd);
      bool not_open(epee::json_rpc::error& er);
      uint64_t adjust_mixin(uint64_t mixin);
      void handle_rpc_exception(const std::exception_ptr& e, epee::json_rpc::error& er, int default_error_code);

      wallet2 *m_wallet;
      std::string m_wallet_dir;
      std::string rpc_login_filename;
      std::atomic<bool> m_stop;
      bool m_trusted_daemon;
      epee::net_utils::http::http_simple_client m_http_client;
      const boost::program_options::variables_map *m_vm;
  };
}
