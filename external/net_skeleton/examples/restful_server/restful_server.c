/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "net_skeleton.h"

static const char *s_http_port = "8000";
static struct ns_serve_http_opts s_http_server_opts = { "." };

static void handle_sum_call(struct ns_connection *nc, struct http_message *hm) {
  char n1[100], n2[100];

  /* Get form variables */
  ns_get_http_var(&hm->body, "n1", n1, sizeof(n1));
  ns_get_http_var(&hm->body, "n2", n2, sizeof(n2));

  ns_printf(nc, "%s", "HTTP/1.0 200 OK\n\n");
  ns_printf(nc, "{ \"result\": %lf }", strtod(n1, NULL) + strtod(n2, NULL));
}

static void ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;

  switch (ev) {
    case NS_HTTP_REQUEST:
      if (ns_vcmp(&hm->uri, "/api/v1/sum") == 0) {
        handle_sum_call(nc, hm);                    /* Handle RESTful call */
      } else {
        ns_serve_http(nc, hm, s_http_server_opts);  /* Serve static content */
      }
      nc->flags |= NSF_FINISHED_SENDING_DATA;
      break;
    default:
      break;
  }
}

int main(void) {
  struct ns_mgr mgr;
  struct ns_connection *nc;

  ns_mgr_init(&mgr, NULL);
  nc = ns_bind(&mgr, s_http_port, ev_handler);
  ns_set_protocol_http_websocket(nc);

  printf("Starting RESTful server on port %s\n", s_http_port);
  for (;;) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return 0;
}
