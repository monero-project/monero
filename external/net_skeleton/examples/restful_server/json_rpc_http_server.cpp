#include "json_rpc_http_server.h"

#include <iostream>

namespace RPC
{
  Json_rpc_http_server::Json_rpc_http_server(const std::string &ip, const std::string &port)
  {
    m_ip = ip;
    m_port = port;
    m_is_running = false;
    m_method_count = 0;
  }

  Json_rpc_http_server::~Json_rpc_http_server()
  {
    stop();
  }

  void Json_rpc_http_server::start()
  {
    m_is_running = true;
    server_thread = new boost::thread(poll);
  }

  void Json_rpc_http_server::poll()
  {
    ns_mgr_init(&mgr, NULL);
    nc = ns_bind(&mgr, s_http_port, ev_handler);
    ns_set_protocol_http_websocket(nc);
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

  void ev_handler(struct ns_connection *nc, int ev, void *ev_data)
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

  bool add_handler(const std::string &method_name,
    void (*hander)(char *buf, int len, struct ns_rpc_request *req))
  {
    if (m_method_count == MAX_METHODS)
    {
      return false;
    }
    m_method_names[m_method_count] = new char[method_name.length() + 1];
    strcpy(m_method_names[m_method_count], method_name.c_str());
    m_handlers[m_method_count] = hander;
    m_method_count++;
    return true;
  }
}
