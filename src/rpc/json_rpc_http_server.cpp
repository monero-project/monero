#include "json_rpc_http_server.h"

#include <iostream>

namespace RPC
{
  int Json_rpc_http_server::parse_error = -32700;
  int Json_rpc_http_server::invalid_request = -32600;
  int Json_rpc_http_server::invalid_params = -32602;
  int Json_rpc_http_server::internal_error = -32603;

  Json_rpc_http_server::Json_rpc_http_server(const std::string &ip, const std::string &port,
    void (*ev_handler)(struct ns_connection *nc, int ev, void *ev_data))
  {
    m_ip = ip;
    m_port = port;
    m_is_running = false;
    m_ev_handler = ev_handler;
  }

  Json_rpc_http_server::~Json_rpc_http_server()
  {
    stop();
  }

  bool Json_rpc_http_server::start()
  {
    m_is_running = true;
    ns_mgr_init(&mgr, NULL);
    nc = ns_bind(&mgr, (m_ip + ":" + m_port).c_str(), m_ev_handler);
    if (!nc)
    {
      return false;
    }
    ns_set_protocol_http_websocket(nc);
    server_thread = new boost::thread(&Json_rpc_http_server::poll, this);
    return true;
  }

  void Json_rpc_http_server::poll()
  {
    while (m_is_running) {
      ns_mgr_poll(&mgr, 1000);
    }
  }

  void Json_rpc_http_server::stop()
  {
    m_is_running = false;
    server_thread->join();
    delete server_thread;
    ns_mgr_free(&mgr);
  }
}
