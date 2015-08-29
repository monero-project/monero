#include "net_skeleton/net_skeleton.h"
#include <boost/thread.hpp>
#include <string>

#define MAX_METHODS

namespace RPC
{
  class Json_rpc_http_server
  {
    struct ns_mgr mgr;
    struct ns_connection *nc;
    Boost::thread *server_thread;
    void ev_handler(struct ns_connection *nc, int ev, void *ev_data);
    void poll();
    std::string m_ip;
    std::string m_port;
    bool m_is_running;
    char *m_method_names[MAX_METHODS];
    ns_rpc_handler_t m_handlers[MAX_METHODS];
    int m_method_count;
  public:
    Json_rpc_http_server(const std::string &ip, const std::string &port);
    ~Json_rpc_http_server();
    void start();
    void stop();
    bool add_handler(const std::string &method_name,
      void (*hander)(char *buf, int len, struct ns_rpc_request *req));
  };
}
