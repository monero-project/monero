// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma  once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include "net/http_server_impl_base.h"
#include "wallet_rpc_server_commands_defs.h"
#include "wallet2.h"
#include "common/command_line.h"
namespace tools
{
  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class wallet_rpc_server: public epee::http_server_impl_base<wallet_rpc_server>
  {
  public:
    typedef epee::net_utils::connection_context_base connection_context;

    wallet_rpc_server(wallet2& cr);

    const static command_line::arg_descriptor<std::string> arg_rpc_bind_port;
    const static command_line::arg_descriptor<std::string> arg_rpc_bind_ip;


    static void init_options(boost::program_options::options_description& desc);
    bool init(const boost::program_options::variables_map& vm);
    bool run();
  private:

    CHAIN_HTTP_TO_MAP2(connection_context); //forward http requests to uri map

    BEGIN_URI_MAP2()
      BEGIN_JSON_RPC_MAP("/json_rpc")
        MAP_JON_RPC_WE("getbalance",         on_getbalance,         wallet_rpc::COMMAND_RPC_GET_BALANCE)
        MAP_JON_RPC_WE("getaddress",         on_getaddress,         wallet_rpc::COMMAND_RPC_GET_ADDRESS)
        MAP_JON_RPC_WE("transfer",           on_transfer,           wallet_rpc::COMMAND_RPC_TRANSFER)
        MAP_JON_RPC_WE("transfer_split",     on_transfer_split,     wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT)
        MAP_JON_RPC_WE("store",              on_store,              wallet_rpc::COMMAND_RPC_STORE)
        MAP_JON_RPC_WE("get_payments",       on_get_payments,       wallet_rpc::COMMAND_RPC_GET_PAYMENTS)
        MAP_JON_RPC_WE("incoming_transfers", on_incoming_transfers, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS)
      END_JSON_RPC_MAP()
    END_URI_MAP2()

      //json_rpc
      bool on_getbalance(const wallet_rpc::COMMAND_RPC_GET_BALANCE::request& req, wallet_rpc::COMMAND_RPC_GET_BALANCE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_getaddress(const wallet_rpc::COMMAND_RPC_GET_ADDRESS::request& req, wallet_rpc::COMMAND_RPC_GET_ADDRESS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool validate_transfer(const std::list<wallet_rpc::transfer_destination> destinations, const std::string payment_id, std::vector<cryptonote::tx_destination_entry>& dsts, std::vector<uint8_t> extra, epee::json_rpc::error& er);
      bool on_transfer(const wallet_rpc::COMMAND_RPC_TRANSFER::request& req, wallet_rpc::COMMAND_RPC_TRANSFER::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_transfer_split(const wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::request& req, wallet_rpc::COMMAND_RPC_TRANSFER_SPLIT::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_store(const wallet_rpc::COMMAND_RPC_STORE::request& req, wallet_rpc::COMMAND_RPC_STORE::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_get_payments(const wallet_rpc::COMMAND_RPC_GET_PAYMENTS::request& req, wallet_rpc::COMMAND_RPC_GET_PAYMENTS::response& res, epee::json_rpc::error& er, connection_context& cntx);
      bool on_incoming_transfers(const wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::request& req, wallet_rpc::COMMAND_RPC_INCOMING_TRANSFERS::response& res, epee::json_rpc::error& er, connection_context& cntx);

      bool handle_command_line(const boost::program_options::variables_map& vm);

      wallet2& m_wallet;
      std::string m_port;
      std::string m_bind_ip;
  };
}
