/*  =========================================================================
    wap_client - Wallet Client API

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
#include "../include/wap_client.h"

//  Forward reference to method arguments structure
typedef struct _client_args_t client_args_t;

//  This structure defines the context for a client connection
typedef struct {
    //  These properties must always be present in the client_t
    //  and are set by the generated engine. The cmdpipe gets
    //  messages sent to the actor; the msgpipe may be used for
    //  faster asynchronous message flows.
    zsock_t *cmdpipe;           //  Command pipe to/from caller API
    zsock_t *msgpipe;           //  Message pipe to/from caller API
    zsock_t *dealer;            //  Socket to talk to server
    wap_proto_t *message;       //  Message to/from server
    client_args_t *args;        //  Arguments from methods
    
    //  Own properties
    int heartbeat_timer;        //  Timeout for heartbeats to server
} client_t;

//  Include the generated client engine
#include "wap_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    //  We'll ping the server once per second
    self->heartbeat_timer = 1000;
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}


//  ---------------------------------------------------------------------------
//  connect_to_server_endpoint
//

static void
connect_to_server_endpoint (client_t *self)
{
    if (zsock_connect (self->dealer, "%s", self->args->endpoint)) {
        engine_set_exception (self, bad_endpoint_event);
        zsys_warning ("could not connect to %s", self->args->endpoint);
    }
}


//  ---------------------------------------------------------------------------
//  set_client_identity
//

static void
set_client_identity (client_t *self)
{
    wap_proto_set_identity (self->message, self->args->identity);
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
    engine_set_timeout (self, self->args->timeout);
}


//  ---------------------------------------------------------------------------
//  use_heartbeat_timer
//

static void
use_heartbeat_timer (client_t *self)
{
    engine_set_timeout (self, self->heartbeat_timer);
}



//  ---------------------------------------------------------------------------
//  prepare_blocks_command
//

static void
prepare_blocks_command (client_t *self)
{
    wap_proto_set_block_ids (self->message, &self->args->block_ids);
    wap_proto_set_start_height (self->message, self->args->start_height);
}

// ---------------------------------------------------------------------------
// prepare_get_output_indexes_command
//

static void
prepare_get_output_indexes_command (client_t *self)
{
    wap_proto_set_tx_id (self->message, self->args->tx_id);
}

//  ---------------------------------------------------------------------------
//  signal_have_blocks_ok
//

static void
signal_have_blocks_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s888p", "BLOCKS OK", wap_proto_status(self->message),
                wap_proto_start_height (self->message),
                wap_proto_curr_height (self->message),
                wap_proto_get_block_data (self->message));
}


// ---------------------------------------------------------------------------
// prepare_start_command
//

static void
prepare_start_command (client_t *self)
{
    wap_proto_set_address (self->message, self->args->address);
    wap_proto_set_thread_count (self->message, self->args->thread_count);
}

//  ---------------------------------------------------------------------------
//  prepare_put_command
//

static void
prepare_put_command (client_t *self)
{
    wap_proto_set_tx_data (self->message, &self->args->tx_data);
}


//  ---------------------------------------------------------------------------
//  signal_have_put_ok
//

static void
signal_have_put_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s8s", "PUT OK", 0,
                wap_proto_tx_id (self->message));
}


//  ---------------------------------------------------------------------------
//  prepare_get_command
//

static void
prepare_get_command (client_t *self)
{
    wap_proto_set_tx_id (self->message, self->args->tx_id);
}


//  ---------------------------------------------------------------------------
//  signal_have_get_ok
//

static void
signal_have_get_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s8p", "GET OK", 0, 
                wap_proto_get_tx_data (self->message));
}


//  ---------------------------------------------------------------------------
//  prepare_save_command
//

static void
prepare_save_command (client_t *self)
{
}



//  ---------------------------------------------------------------------------
//  signal_have_save_ok
//

static void
signal_have_save_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s8", "SAVE OK", 0);
}


//  ---------------------------------------------------------------------------
//  signal_have_start_ok
//

static void
signal_have_start_ok (client_t *self)
{
    zsock_send(self->cmdpipe, "s8", "START OK", wap_proto_status(self->message));
}


//  ---------------------------------------------------------------------------
//  signal_have_stop_ok
//

static void
signal_have_stop_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s8", "STOP OK", 0);
}

//  ---------------------------------------------------------------------------
//  signal_have_output_indexes_ok
//

static void
signal_have_output_indexes_ok (client_t *self)
{
    zsock_send (self->cmdpipe, "s8p", "OUTPUT INDEXES OK",
        wap_proto_status (self->message),
        wap_proto_get_o_indexes (self->message));
}

//  ---------------------------------------------------------------------------
//  signal_success
//

static void
signal_success (client_t *self)
{
    zsock_send (self->cmdpipe, "si", "SUCCESS", 0);
}


//  ---------------------------------------------------------------------------
//  signal_bad_endpoint
//

static void
signal_bad_endpoint (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Bad server endpoint");
}


//  ---------------------------------------------------------------------------
//  signal_failure
//

static void
signal_failure (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, wap_proto_reason (self->message));
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
    if (wap_proto_status (self->message) == WAP_PROTO_COMMAND_INVALID)
        engine_set_next_event (self, command_invalid_event);
    else
        engine_set_next_event (self, other_event);
}


//  ---------------------------------------------------------------------------
//  signal_unhandled_error
//

static void
signal_unhandled_error (client_t *self)
{
    zsys_error ("unhandled error code from server");
}


//  ---------------------------------------------------------------------------
//  signal_server_not_present
//

static void
signal_server_not_present (client_t *self)
{
    zsock_send (self->cmdpipe, "sis", "FAILURE", -1, "Server is not reachable");
}

