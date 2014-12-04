/*!
 * \file daemon_json_rpc_handlers.h
 * \brief Header for JSON RPC handlers (Daemon)
 */

#ifndef DAEMON_JSON_RPC_HANDLERS_H
#define DAEMON_JSON_RPC_HANDLERS_H

#include "net_skeleton/net_skeleton.h"
#include "json_rpc_http_server.h"
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

#include <iostream>

/*!
 * \namespace RPC
 * \brief RPC related utilities
 */
namespace RPC
{
  /*!
   * \namespace Daemon
   * \brief RPC relevant to daemon
   */
  namespace Daemon
  {
    /*!
     * \brief initializes module (must call this before handling requests)
     * \param p_core    Pointer to cryptonote core object
     * \param p_p2p     Pointer to P2P object
     * \param p_testnet True if testnet false otherwise
     */
    void init(cryptonote::core *p_core,
      nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
      bool p_testnet);

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
