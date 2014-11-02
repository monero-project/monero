#include "net_skeleton/net_skeleton.h"
#include <boost/thread.hpp>
#include <string>

#define JSON_RPC_MAX_METHODS 100

namespace RPC
{
  class Json_rpc_http_server
  {
    struct ns_mgr mgr;
    struct ns_connection *nc;
    boost::thread *server_thread;
    void ev_handler(struct ns_connection *nc, int ev, void *ev_data);
    void poll();
    std::string m_ip;
    std::string m_port;
    bool m_is_running;
    const char **m_method_names;
    ns_rpc_handler_t *m_handlers;
    int m_method_count;
    void (*m_ev_handler)(struct ns_connection *nc, int ev, void *ev_data);
  public:
    Json_rpc_http_server(const std::string &ip, const std::string &port,
      void (*ev_handler)(struct ns_connection *nc, int ev, void *ev_data),
      const char **method_names, ns_rpc_handler_t *handlers);
    ~Json_rpc_http_server();
    bool start();
    void stop();
    bool add_handler(const std::string &method_name,
      int (*hander)(char *buf, int len, struct ns_rpc_request *req));
  };
}
