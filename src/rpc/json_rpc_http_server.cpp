#include "json_rpc_http_server.h"

#include <iostream>

namespace RPC
{
  Json_rpc_http_server::Json_rpc_http_server(const std::string &ip, const std::string &port,
    void (*ev_handler)(struct ns_connection *nc, int ev, void *ev_data),
    const char **method_names, ns_rpc_handler_t *handlers) :
    m_method_names(method_names), m_handlers(handlers)
  {
    m_ip = ip;
    m_port = port;
    m_is_running = false;
    m_method_count = 0;
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

  void Json_rpc_http_server::ev_handler(struct ns_connection *nc, int ev, void *ev_data)
  {
    struct http_message *hm = (struct http_message *) ev_data;
    char buf[100];
    switch (ev) {
      case NS_HTTP_REQUEST:
        ns_rpc_dispatch(hm->body.p, hm->body.len, buf, sizeof(buf),
        m_method_names, m_handlers);
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
