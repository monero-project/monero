#include "net_skeleton/net_skeleton.h"
#include <iostream>

namespace RPC
{
  int foo(char *buf, int len, struct ns_rpc_request *req) {
    std::cout << "Method name: ";
    std::cout << req->method->ptr << std::endl;
    return 0;
  }

  const char *method_names[] = {"foo", NULL};
  ns_rpc_handler_t handlers[] = {foo, NULL};  

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
