// Copyright (c) 2014 Cesanta Software Limited
// All rights reserved
//
// This software is dual-licensed: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation. For the terms of this
// license, see <http://www.gnu.org/licenses/>.
//
// You are free to use this software under the terms of the GNU General
// Public License, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// Alternatively, you can license this software under a commercial
// license, as set out in <http://cesanta.com/>.
//
// $Date: 2014-09-28 05:04:41 UTC $

#include "net_skeleton.h"

static void ev_handler(struct ns_connection *nc, int ev, void *p) {
  struct iobuf *io = &nc->recv_iobuf;
  (void) p;

  switch (ev) {
    case NS_RECV:
      ns_send(nc, io->buf, io->len);  // Echo message back
      iobuf_remove(io, io->len);        // Discard message from recv buffer
      break;
    default:
      break;
  }
}

int main(void) {
  struct ns_mgr mgr;
  const char *port1 = "1234", *port2 = "127.0.0.1:17000";

  ns_mgr_init(&mgr, NULL);
  ns_bind(&mgr, port1, ev_handler);
  ns_bind(&mgr, port2, ev_handler);

  printf("Starting echo mgr on ports %s, %s\n", port1, port2);
  for (;;) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return 0;
}
