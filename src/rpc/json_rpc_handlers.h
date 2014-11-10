#ifndef JSON_RPC_HANDLERS_H
#define JSON_RPC_HANDLERS_H

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

namespace RPC
{
  void init(cryptonote::core *p_core,
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
    bool p_testnet);

  void init_options(boost::program_options::options_description& desc);

  void get_address_and_port(const boost::program_options::variables_map& vm,
    std::string &ip_address, std::string &port);

  void ev_handler(struct ns_connection *nc, int ev, void *ev_data);
}

#endif
