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
#include "ipc/include/wallet.h"
#include "ipc/include/daemon_ipc_handlers.h"

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
  namespace DaemonDeprecated
  {
    const int SUCCESS = 0;
    const int FAILURE_DAEMON_NOT_RUNNING = 1;
    const int FAILURE_HTTP_SERVER = 2;
    int start();
    void stop();
  }
}

#endif
