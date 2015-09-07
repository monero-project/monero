/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "net_skeleton.h"

static int s_signal_received = 0;
static const char *s_http_port = "8000";
static struct ns_serve_http_opts s_http_server_opts = { "." };

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

static int is_websocket(const struct ns_connection *nc) {
  return nc->flags & NSF_IS_WEBSOCKET;
}

static void broadcast(struct ns_connection *nc, const char *msg, size_t len) {
  struct ns_connection *c;
  char buf[500];

  snprintf(buf, sizeof(buf), "%p %.*s", nc, (int) len, msg);
  for (c = ns_next(nc->mgr, NULL); c != NULL; c = ns_next(nc->mgr, c)) {
    ns_send_websocket_frame(c, WEBSOCKET_OP_TEXT, buf, strlen(buf));
  }
}

static void ev_handler(struct ns_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  struct websocket_message *wm = (struct websocket_message *) ev_data;

  switch (ev) {
    case NS_HTTP_REQUEST:
      /* Usual HTTP request - serve static files */
      ns_serve_http(nc, hm, s_http_server_opts);
      nc->flags |= NSF_FINISHED_SENDING_DATA;
      break;
    case NS_WEBSOCKET_HANDSHAKE_DONE:
      /* New websocket connection. Tell everybody. */
      broadcast(nc, "joined", 6);
      break;
    case NS_WEBSOCKET_FRAME:
      /* New websocket message. Tell everybody. */
      broadcast(nc, (char *) wm->data, wm->size);
      break;
    case NS_CLOSE:
      /* Disconnect. Tell everybody. */
      if (is_websocket(nc)) {
        broadcast(nc, "left", 4);
      }
      break;
    default:
      break;
  }
}

int main(void) {
  struct ns_mgr mgr;
  struct ns_connection *nc;

  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);

  ns_mgr_init(&mgr, NULL);

  nc = ns_bind(&mgr, s_http_port, ev_handler);
  ns_set_protocol_http_websocket(nc);

  printf("Started on port %s\n", s_http_port);
  while (s_signal_received == 0) {
    ns_mgr_poll(&mgr, 200);
  }
  ns_mgr_free(&mgr);

  return 0;
}
