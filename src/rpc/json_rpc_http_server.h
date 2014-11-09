#ifndef JSON_RPC_HTTP_SERVER_H
#define JSON_RPC_HTTP_SERVER_H

#include "net_skeleton/net_skeleton.h"
#include <boost/thread.hpp>
#include <string>

namespace RPC
{
  class Json_rpc_http_server
  {
    struct ns_mgr mgr;
    struct ns_connection *nc;
    boost::thread *server_thread;
    void poll();
    std::string m_ip;
    std::string m_port;
    bool m_is_running;
    void (*m_ev_handler)(struct ns_connection *nc, int ev, void *ev_data);
  public:
    Json_rpc_http_server(const std::string &ip, const std::string &port,
      void (*ev_handler)(struct ns_connection *nc, int ev, void *ev_data));
    ~Json_rpc_http_server();
    bool start();
    void stop();

    static int parse_error;
    static int invalid_request;
    static int invalid_params;
    static int internal_error;
  };
}

#endif
