/*!
 * \file wallet_json_rpc_handlers.h
 * \brief Header for JSON RPC handlers (Wallet)
 */

#ifndef WALLET_JSON_RPC_HANDLERS_H
#define WALLET_JSON_RPC_HANDLERS_H

#include "net_skeleton/net_skeleton.h"
#include "rpc/json_rpc_http_server.h"
#include "common/command_line.h"
#include "net/http_server_impl_base.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <cstring>
#include "cryptonote_core/cryptonote_basic.h"
#include "crypto/hash-ops.h"
#include "wallet2.h"

#include <iostream>

/*!
 * \namespace RPC
 * \brief RPC related utilities
 */
namespace RPC
{
  /*!
   * \namespace Wallet
   * \brief RPC relevant to wallet
   */
  namespace Wallet
  {
    const command_line::arg_descriptor<std::string> arg_rpc_bind_ip = {
      "rpc-bind-ip",
      "Specify ip to bind rpc server",
      "127.0.0.1"
    };

    const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {
      "rpc-bind-port",
      "Starts wallet as rpc server for wallet operations, sets bind port for server",
      "",
      true
    };

    /*!
     * \brief initializes module (must call this before handling requests)
     * \param p_wallet    Pointer to wallet2 object
     */
    void init(tools::wallet2 *p_wallet);

    /*!
     * \Inits certain options used in Daemon CLI.
     * \param desc Instance of options description object
     */
    void init_options(boost::program_options::options_description& desc);

    /*!
     * \brief Gets IP address and port number from variable map
     * \param vm         Variable map
     * \param ip_address IP address
     * \param port       Port number
     */
    void get_address_and_port(const boost::program_options::variables_map& vm,
      std::string &ip_address, std::string &port);

    /*!
     * \brief Event handler that is invoked upon net_skeleton network events.
     * 
     * Any change in behavior of RPC should happen from this point.
     * \param nc      net_skeleton connection
     * \param ev      Type of event
     * \param ev_data Event data
     */
    void ev_handler(struct ns_connection *nc, int ev, void *ev_data);
  }
}

#endif
