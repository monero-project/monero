/*  =========================================================================
    wap_server - wap_server

    Copyright (c) the Contributors as noted in the AUTHORS file.
                                                                
    (insert license text here)                                  
    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include "wap_classes.h"
//  TODO: Change these to match your project's needs
#include "../include/wap_proto.h"
#include "../include/wap_server.h"
#include "daemon_ipc_handlers.h"

//  ---------------------------------------------------------------------------
//  Forward declarations for the two main classes we use here

typedef struct _server_t server_t;
typedef struct _client_t client_t;

//  This structure defines the context for each running server. Store
//  whatever properties and structures you need for the server.

struct _server_t {
    //  These properties must always be present in the server_t
    //  and are set by the generated engine; do not modify them!
    zsock_t *pipe;              //  Actor pipe back to caller
    zconfig_t *config;          //  Current loaded configuration
    
    //  TODO: Add any properties you need here
};

//  ---------------------------------------------------------------------------
//  This structure defines the state for each client connection. It will
//  be passed to each action in the 'self' argument.

struct _client_t {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine; do not modify them!
    server_t *server;           //  Reference to parent server
    wap_proto_t *message;       //  Message in and out

    //  TODO: Add specific properties for your application
};

//  Include the generated server engine
#include "wap_server_engine.inc"

//  Allocate properties and structures for a new server instance.
//  Return 0 if OK, or -1 if there was an error.

static int
server_initialize (server_t *self)
{
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a server instance

static void
server_terminate (server_t *self)
{
    //  Destroy properties here
}

//  Process server API method, return reply message if any

static zmsg_t *
server_method (server_t *self, const char *method, zmsg_t *msg)
{
    return NULL;
}


//  Allocate properties and structures for a new client connection and
//  optionally engine_set_next_event (). Return 0 if OK, or -1 on error.

static int
client_initialize (client_t *self)
{
    //  Construct properties here
    return 0;
}

//  Free properties and structures for a client connection

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}

//  ---------------------------------------------------------------------------
//  Selftest

void
wap_server_test (bool verbose)
{
    static char server_string[] = "server";
    printf (" * wap_server: ");
    if (verbose)
        printf ("\n");
    
    //  @selftest
    zactor_t *server = zactor_new (wap_server, server_string);
    if (verbose)
        zstr_send (server, "VERBOSE");
    zstr_sendx (server, "BIND", "ipc://@/wap_server", NULL);

    zsock_t *client = zsock_new (ZMQ_DEALER);
    assert (client);
    zsock_set_rcvtimeo (client, 2000);
    zsock_connect (client, "ipc://@/wap_server");

    //  TODO: fill this out
    wap_proto_t *request = wap_proto_new ();
    wap_proto_destroy (&request);
    
    zsock_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  register_wallet
//

static void
register_wallet (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  retrieve_blocks
//

static void
retrieve_blocks (client_t *self)
{
    IPC::Daemon::retrieve_blocks(self->message);
}


//  ---------------------------------------------------------------------------
//  store_transaction
//

static void
store_transaction (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  retrieve_transaction
//

static void
retrieve_transaction (client_t *self)
{
    IPC::Daemon::get_transaction(self->message);
}


//  ---------------------------------------------------------------------------
//  start_mining_process
//

static void
start_mining_process (client_t *self)
{
    IPC::Daemon::start_mining(self->message);
}


//  ---------------------------------------------------------------------------
//  stop_mining_process
//

static void
stop_mining_process (client_t *self)
{
    IPC::Daemon::stop_mining(self->message);
}

//  ---------------------------------------------------------------------------
//  output_indexes
//

static void
output_indexes (client_t *self)
{
    IPC::Daemon::get_output_indexes(self->message);
}

//  ---------------------------------------------------------------------------
//  send_transaction
//

static void
send_transaction (client_t *self)
{
    IPC::Daemon::send_raw_transaction(self->message);
}


//  ---------------------------------------------------------------------------
//  deregister_wallet
//

static void
deregister_wallet (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  allow_time_to_settle
//

static void
allow_time_to_settle (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  register_new_client
//

static void
register_new_client (client_t *self)
{

}


//  ---------------------------------------------------------------------------
//  signal_command_not_valid
//

static void
signal_command_not_valid (client_t *self)
{
    wap_proto_set_status (self->message, WAP_PROTO_COMMAND_INVALID);
}

//  ---------------------------------------------------------------------------
//  random_outs
//

static void
random_outs (client_t *self)
{
    IPC::Daemon::get_random_outs(self->message);
}

//  ---------------------------------------------------------------------------
//  height
//

static void
height (client_t *self)
{
    IPC::Daemon::get_height(self->message);
}

//  ---------------------------------------------------------------------------
//  save_bc
//

static void
save_bc (client_t *self)
{
    IPC::Daemon::save_bc(self->message);
}

//  ---------------------------------------------------------------------------
//  getinfo
//

static void
getinfo (client_t *self)
{
    IPC::Daemon::get_info(self->message);
}

//  ---------------------------------------------------------------------------
//  get_peer_list
//

static void
get_peer_list (client_t *self)
{
    IPC::Daemon::get_peer_list(self->message);
}

//  ---------------------------------------------------------------------------
//  get_mining_status
//

static void
get_mining_status (client_t *self)
{
    IPC::Daemon::get_mining_status(self->message);
}

//  ---------------------------------------------------------------------------
//  set_log_hash_rate
//

static void
set_log_hash_rate (client_t *self)
{
    IPC::Daemon::set_log_hash_rate(self->message);
}



//  ---------------------------------------------------------------------------
//  set_log_level
//

static void
set_log_level (client_t *self)
{
    IPC::Daemon::set_log_level(self->message);
}

//  ---------------------------------------------------------------------------
//  start_save_graph
//

static void
start_save_graph (client_t *self)
{
    IPC::Daemon::start_save_graph(self->message);
}

//  ---------------------------------------------------------------------------
//  stop_save_graph
//

static void
stop_save_graph (client_t *self)
{
    IPC::Daemon::stop_save_graph(self->message);
}

//  ---------------------------------------------------------------------------
//  get_block_hash
//

static void
get_block_hash (client_t *self)
{
    IPC::Daemon::get_block_hash(self->message);
}

//  ---------------------------------------------------------------------------
//  get_block_template
//

static void
get_block_template (client_t *self)
{
    IPC::Daemon::get_block_template(self->message);
}

//  ---------------------------------------------------------------------------
//  get_hard_fork_info
//

static void
get_hard_fork_info (client_t *self)
{
    IPC::Daemon::get_hard_fork_info(self->message);
}

//  ---------------------------------------------------------------------------
//  get_connections_list
//

static void
get_connections_list (client_t *self)
{
    IPC::Daemon::get_connections_list(self->message);
}

//  ---------------------------------------------------------------------------
//  stop_daemon
//

static void
stop_daemon (client_t *self)
{
    IPC::Daemon::stop_daemon(self->message);
}

//  ---------------------------------------------------------------------------
//  get_block_by_height
//

static void
get_block_by_height (client_t *self)
{
    IPC::Daemon::get_block_by_height(self->message);
}

//  ---------------------------------------------------------------------------
//  get_block_by_hash
//

static void
get_block_by_hash (client_t *self)
{
    IPC::Daemon::get_block_by_hash(self->message);
}

//  ---------------------------------------------------------------------------
//  get_key_image_status
//

static void
get_key_image_status (client_t *self)
{
    IPC::Daemon::get_key_image_status(self->message);
}

//  ---------------------------------------------------------------------------
//  get_tx_pool
//

static void
get_tx_pool (client_t *self)
{
    IPC::Daemon::get_tx_pool(self->message);
}

//  ---------------------------------------------------------------------------
//  set_out_peers
//

static void
set_out_peers (client_t *self)
{
    IPC::Daemon::set_out_peers(self->message);
}

