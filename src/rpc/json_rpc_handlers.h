#include "net_skeleton/net_skeleton.h"
#include "common/command_line.h"
#include "net/http_server_impl_base.h"
#include "cryptonote_core/cryptonote_core.h"
#include "p2p/net_node.h"
#include "cryptonote_protocol/cryptonote_protocol_handler.h"
#include <string>

#include <iostream>

#define CHECK_CORE_BUSY() if (check_core_busy()) { return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", CORE_RPC_STATUS_BUSY); }

namespace RPC
{
  cryptonote::core *core;
  nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p2p;
  bool testnet;

  const command_line::arg_descriptor<std::string> arg_rpc_bind_ip   = {
    "rpc-bind-ip",
    "IP for RPC server",
    "127.0.0.1"
  };

  const command_line::arg_descriptor<std::string> arg_rpc_bind_port = {
    "rpc-bind-port",
    "Port for RPC server",
    std::to_string(config::RPC_DEFAULT_PORT)
  };

  const command_line::arg_descriptor<std::string> arg_testnet_rpc_bind_port = {
    "testnet-rpc-bind-port",
    "Port for testnet RPC server",
    std::to_string(config::testnet::RPC_DEFAULT_PORT)
  };

  void init(cryptonote::core *p_core,
    nodetool::node_server<cryptonote::t_cryptonote_protocol_handler<cryptonote::core> > *p_p2p,
    bool p_testnet)
  {
    core = p_core;
    p2p = p_p2p;
    testnet = p_testnet;
  }

  void init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_rpc_bind_ip);
    command_line::add_arg(desc, arg_rpc_bind_port);
    command_line::add_arg(desc, arg_testnet_rpc_bind_port);
  }

  void get_address_and_port(const boost::program_options::variables_map& vm,
    std::string &ip_address, std::string &port)
  {
    auto p2p_bind_arg = testnet ? arg_testnet_rpc_bind_port : arg_rpc_bind_port;

    ip_address = command_line::get_arg(vm, arg_rpc_bind_ip);
    port = command_line::get_arg(vm, p2p_bind_arg);
  }

  bool check_core_busy()
  {
    if (p2p->get_payload_object().get_core().get_blockchain_storage().is_storing_blockchain())
    {
      return true;
    }
    return false;
  }

  void create_map_from_request()
  {

  }

  int getheight(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    uint64_t height = core->get_current_blockchain_height();
    // TODO: uint64_t -> long not ok !
    return ns_rpc_create_reply(buf, len, req, "{s:i,s:s}", "height", height, "status", CORE_RPC_STATUS_OK);
  }

  int getblocks(char *buf, int len, struct ns_rpc_request *req)
  {
    CHECK_CORE_BUSY();
    if (!core->find_blockchain_supplement(req.start_height, req.block_ids, bs, res.current_height, res.start_height, COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT))
    {
      return ns_rpc_create_reply(buf, len, req, "{s:s}", "status", "Failed");
    }
    req->params
  }

  const char *method_names[] = {
    "getheight",
    "getblocks",
    NULL
  };

  ns_rpc_handler_t handlers[] = {
    getheight,
    getblocks,
    NULL
  };

  void ev_handler(struct ns_connection *nc, int ev, void *ev_data)
  {
    std::cout << "evenet\n\n";
    struct http_message *hm = (struct http_message *) ev_data;
    char buf[100];
    switch (ev) {
      case NS_HTTP_REQUEST:
        ns_rpc_dispatch(hm->body.p, hm->body.len, buf, sizeof(buf),
          method_names, handlers);
        ns_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
          "Content-Type: application/json\r\n\r\n%s",
          (int) strlen(buf), buf);
          nc->flags |= NSF_FINISHED_SENDING_DATA;
        break;
      default:
        break;
    }
  }
}
