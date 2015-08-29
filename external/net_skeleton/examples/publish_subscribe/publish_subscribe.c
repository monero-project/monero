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

static void *stdin_thread(void *param) {
  int ch, sock = * (int *) param;
  while ((ch = getchar()) != EOF) {
    unsigned char c = (unsigned char) ch;
    send(sock, &c, 1, 0);  // Forward all types characters to the socketpair
  }
  return NULL;
}

static void server_handler(struct ns_connection *nc, int ev, void *p) {
  (void) p;
  if (ev == NS_RECV) {
    // Push received message to all ncections
    struct iobuf *io = &nc->recv_iobuf;
    struct ns_connection *c;

    for (c = ns_next(nc->mgr, NULL); c != NULL; c = ns_next(nc->mgr, c)) {
      ns_send(c, io->buf, io->len);
    }
    iobuf_remove(io, io->len);
  }
}

static void client_handler(struct ns_connection *conn, int ev, void *p) {
  struct iobuf *io = &conn->recv_iobuf;
  (void) p;

  if (ev == NS_CONNECT) {
    if (conn->flags & NSF_CLOSE_IMMEDIATELY) {
      printf("%s\n", "Error connecting to server!");
      exit(EXIT_FAILURE);
    }
    printf("%s\n", "Connected to server. Type a message and press enter.");
  } else if (ev == NS_RECV) {
    if (conn->flags & NSF_USER_1) {
      // Received data from the stdin, forward it to the server
      struct ns_connection *c = (struct ns_connection *) conn->user_data;
      ns_send(c, io->buf, io->len);
      iobuf_remove(io, io->len);
    } else {
      // Received data from server connection, print it
      fwrite(io->buf, io->len, 1, stdout);
      iobuf_remove(io, io->len);
    }
  } else if (ev == NS_CLOSE) {
    // Connection has closed, most probably cause server has stopped
    exit(EXIT_SUCCESS);
  }
}

int main(int argc, char *argv[]) {
  struct ns_mgr mgr;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <port> <client|server>\n", argv[0]);
    exit(EXIT_FAILURE);
  } else if (strcmp(argv[2], "client") == 0) {
    int fds[2];
    struct ns_connection *ioconn, *server_conn;

    ns_mgr_init(&mgr, NULL);

    // Connect to the pubsub server
    server_conn = ns_connect(&mgr, argv[1], client_handler);
    if (server_conn == NULL) {
      fprintf(stderr, "Cannot connect to port %s\n", argv[1]);
      exit(EXIT_FAILURE);
    }

    // Create a socketpair and give one end to the thread that reads stdin
    ns_socketpair(fds);
    ns_start_thread(stdin_thread, &fds[1]);

    // The other end of a pair goes inside the server
    ioconn = ns_add_sock(&mgr, fds[0], client_handler);
    ioconn->flags |= NSF_USER_1;    // Mark this so we know this is a stdin
    ioconn->user_data = server_conn;

  } else {
    // Server code path
    ns_mgr_init(&mgr, NULL);
    ns_bind(&mgr, argv[1], server_handler);
    printf("Starting pubsub server on port %s\n", argv[1]);
  }

  for (;;) {
    ns_mgr_poll(&mgr, 1000);
  }
  ns_mgr_free(&mgr);

  return EXIT_SUCCESS;
}
