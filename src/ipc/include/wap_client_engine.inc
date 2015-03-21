/*  =========================================================================
    wap_client_engine - Wallet Client API engine

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: wap_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
                                                                
    (insert license text here)                                  
    =========================================================================
*/


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    expect_open_ok_state = 2,
    connected_state = 3,
    expect_blocks_ok_state = 4,
    expect_get_ok_state = 5,
    expect_put_ok_state = 6,
    expect_save_ok_state = 7,
    expect_start_ok_state = 8,
    expect_stop_ok_state = 9,
    expect_output_indexes_ok_state = 10,
    expect_close_ok_state = 11,
    defaults_state = 12,
    have_error_state = 13,
    reexpect_open_ok_state = 14
} state_t;

typedef enum {
    NULL_event = 0,
    constructor_event = 1,
    bad_endpoint_event = 2,
    open_ok_event = 3,
    expired_event = 4,
    blocks_event = 5,
    get_event = 6,
    put_event = 7,
    save_event = 8,
    start_event = 9,
    stop_event = 10,
    output_indexes_event = 11,
    destructor_event = 12,
    blocks_ok_event = 13,
    get_ok_event = 14,
    put_ok_event = 15,
    save_ok_event = 16,
    start_ok_event = 17,
    stop_ok_event = 18,
    output_indexes_ok_event = 19,
    close_ok_event = 20,
    ping_ok_event = 21,
    error_event = 22,
    command_invalid_event = 23,
    other_event = 24
} event_t;

//  Names for state machine logging and error reporting
static char *
s_state_name [] = {
    "(NONE)",
    "start",
    "expect open ok",
    "connected",
    "expect blocks ok",
    "expect get ok",
    "expect put ok",
    "expect save ok",
    "expect start ok",
    "expect stop ok",
    "expect output indexes ok",
    "expect close ok",
    "defaults",
    "have error",
    "reexpect open ok"
};

static char *
s_event_name [] = {
    "(NONE)",
    "constructor",
    "bad_endpoint",
    "OPEN_OK",
    "expired",
    "BLOCKS",
    "GET",
    "PUT",
    "SAVE",
    "START",
    "STOP",
    "OUTPUT_INDEXES",
    "destructor",
    "BLOCKS_OK",
    "GET_OK",
    "PUT_OK",
    "SAVE_OK",
    "START_OK",
    "STOP_OK",
    "OUTPUT_INDEXES_OK",
    "CLOSE_OK",
    "PING_OK",
    "ERROR",
    "command_invalid",
    "other"
};
 

//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    uint32_t timeout;
    char *identity;
    zlist_t *block_ids;
    uint64_t start_height;
    zchunk_t *tx_data;
    char *tx_id;
    char *address;
    uint64_t thread_count;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    wap_proto_t *message;       //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool terminated;            //  True if client is shutdown
    size_t timeout;             //  inactivity timeout, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for timeouts
    int wakeup_timer;           //  zloop timer for alarms
    event_t wakeup_event;       //  Wake up with this event
} s_client_t;

static int
    client_initialize (client_t *self);
static void
    client_terminate (client_t *self);
static void
    s_client_destroy (s_client_t **self_p);
static void
    s_client_execute (s_client_t *self, event_t event);
static int
    s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument);
static int
    s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument);
static void
    s_satisfy_pedantic_compilers (void);
static void
    connect_to_server_endpoint (client_t *self);
static void
    set_client_identity (client_t *self);
static void
    use_connect_timeout (client_t *self);
static void
    signal_bad_endpoint (client_t *self);
static void
    signal_success (client_t *self);
static void
    use_heartbeat_timer (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    prepare_blocks_command (client_t *self);
static void
    prepare_get_command (client_t *self);
static void
    prepare_put_command (client_t *self);
static void
    prepare_save_command (client_t *self);
static void
    prepare_start_command (client_t *self);
static void
    prepare_get_output_indexes_command (client_t *self);
static void
    signal_have_blocks_ok (client_t *self);
static void
    signal_have_get_ok (client_t *self);
static void
    signal_have_put_ok (client_t *self);
static void
    signal_have_save_ok (client_t *self);
static void
    signal_have_start_ok (client_t *self);
static void
    signal_have_stop_ok (client_t *self);
static void
    signal_have_output_indexes_ok (client_t *self);
static void
    signal_failure (client_t *self);
static void
    check_status_code (client_t *self);
static void
    signal_unhandled_error (client_t *self);

//  Global tracing/animation indicator; we can't use a client method as
//  that only works after construction (which we often want to trace).
volatile int wap_client_verbose = false;
    
//  Create a new client connection

static s_client_t *
s_client_new (zsock_t *cmdpipe, zsock_t *msgpipe)
{
    s_client_t *self = (s_client_t *) zmalloc (sizeof (s_client_t));
    if (self) {
        assert ((s_client_t *) &self->client == self);
        self->cmdpipe = cmdpipe;
        self->msgpipe = msgpipe;
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = wap_proto_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
            self->state = start_state;
            self->event = NULL_event;
            self->client.cmdpipe = self->cmdpipe;
            self->client.msgpipe = self->msgpipe;
            self->client.dealer = self->dealer;
            self->client.message = self->message;
            self->client.args = &self->args;
            if (client_initialize (&self->client))
                s_client_destroy (&self);
        }
        else
            s_client_destroy (&self);
    }
    s_satisfy_pedantic_compilers ();
    return self;
}

//  Destroy the client connection

static void
s_client_destroy (s_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        s_client_t *self = *self_p;
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.identity);
        zlist_destroy (&self->args.block_ids);
        zchunk_destroy (&self->args.tx_data);
        zstr_free (&self->args.tx_id);
        zstr_free (&self->args.address);
        client_terminate (&self->client);
        wap_proto_destroy (&self->message);
        zsock_destroy (&self->msgpipe);
        zsock_destroy (&self->dealer);
        zloop_destroy (&self->loop);
        free (self);
        *self_p = NULL;
    }
}

//  ---------------------------------------------------------------------------
//  These methods are an internal API for actions

//  Set the next event, needed in at least one action in an internal
//  state; otherwise the state machine will wait for a message on the
//  dealer socket and treat that as the event.

static void
engine_set_next_event (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->next_event = event;
    }
}

//  Raise an exception with 'event', halting any actions in progress.
//  Continues execution of actions defined for the exception event.

static void
engine_set_exception (client_t *client, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->exception = event;
    }
}

//  Set wakeup alarm after 'delay' msecs. The next state should handle the
//  wakeup event. The alarm is cancelled on any other event.

static void
engine_set_wakeup_event (client_t *client, size_t delay, event_t event)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        if (self->wakeup_timer) {
            zloop_timer_end (self->loop, self->wakeup_timer);
            self->wakeup_timer = 0;
        }
        self->wakeup_timer = zloop_timer (
            self->loop, delay, 1, s_client_handle_wakeup, self);
        self->wakeup_event = event;
    }
}

//  Set timeout for next protocol read. By default, will wait forever
//  or until the process is interrupted. The timeout is in milliseconds.
//  The state machine must handle the "expired" event.

static void
engine_set_timeout (client_t *client, size_t timeout)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->timeout = timeout;
        if (self->timeout)
            self->expiry_timer = zloop_timer (
                self->loop, self->timeout, 1, s_client_handle_timeout, self);
    }
}

//  Poll socket for activity, invoke handler on any received message.
//  Handler must be a CZMQ zloop_fn function; receives client as arg.

static void
engine_handle_socket (client_t *client, zsock_t *sock, zloop_reader_fn handler)
{
    if (client && sock) {
        s_client_t *self = (s_client_t *) client;
        if (handler != NULL) {
            int rc = zloop_reader (self->loop, sock, handler, self);
            assert (rc == 0);
            zloop_reader_set_tolerant (self->loop, sock);
        }
        else
            zloop_reader_end (self->loop, sock);
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_timeout (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
}


//  ---------------------------------------------------------------------------
//  Generic methods on protocol messages
//  TODO: replace with lookup table, since ID is one byte

static event_t
s_protocol_event (s_client_t *self, wap_proto_t *message)
{
    assert (message);
    switch (wap_proto_id (message)) {
        case WAP_PROTO_OPEN_OK:
            return open_ok_event;
            break;
        case WAP_PROTO_BLOCKS:
            return blocks_event;
            break;
        case WAP_PROTO_BLOCKS_OK:
            return blocks_ok_event;
            break;
        case WAP_PROTO_PUT:
            return put_event;
            break;
        case WAP_PROTO_PUT_OK:
            return put_ok_event;
            break;
        case WAP_PROTO_OUTPUT_INDEXES:
            return output_indexes_event;
            break;
        case WAP_PROTO_OUTPUT_INDEXES_OK:
            return output_indexes_ok_event;
            break;
        case WAP_PROTO_GET:
            return get_event;
            break;
        case WAP_PROTO_GET_OK:
            return get_ok_event;
            break;
        case WAP_PROTO_SAVE:
            return save_event;
            break;
        case WAP_PROTO_SAVE_OK:
            return save_ok_event;
            break;
        case WAP_PROTO_START:
            return start_event;
            break;
        case WAP_PROTO_START_OK:
            return start_ok_event;
            break;
        case WAP_PROTO_STOP:
            return stop_event;
            break;
        case WAP_PROTO_STOP_OK:
            return stop_ok_event;
            break;
        case WAP_PROTO_CLOSE_OK:
            return close_ok_event;
            break;
        case WAP_PROTO_PING_OK:
            return ping_ok_event;
            break;
        case WAP_PROTO_ERROR:
            return error_event;
            break;
        default:
            zsys_error ("wap_client: unknown command %s, halting", wap_proto_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is terminated, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (wap_client_verbose) {
            zsys_debug ("wap_client: %s:", s_state_name [self->state]);
            zsys_debug ("wap_client:        %s", s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == constructor_event) {
                    if (!self->exception) {
                        //  connect to server endpoint
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ connect to server endpoint");
                        connect_to_server_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  set client identity
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ set client identity");
                        set_client_identity (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send OPEN
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send OPEN");
                        wap_proto_set_id (self->message, WAP_PROTO_OPEN);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_open_ok_state;
                }
                else
                if (self->event == bad_endpoint_event) {
                    if (!self->exception) {
                        //  signal bad endpoint
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal bad endpoint");
                        signal_bad_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("wap_client: unhandled event %s in %s",
                        s_event_name [self->event], s_state_name [self->state]);
                    assert (false);
                }
                break;

            case expect_open_ok_state:
                if (self->event == open_ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  use heartbeat timer
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ use heartbeat timer");
                        use_heartbeat_timer (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal server not present
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal server not present");
                        signal_server_not_present (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case connected_state:
                if (self->event == blocks_event) {
                    if (!self->exception) {
                        //  prepare blocks command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare blocks command");
                        prepare_blocks_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send BLOCKS
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send BLOCKS");
                        wap_proto_set_id (self->message, WAP_PROTO_BLOCKS);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_blocks_ok_state;
                }
                else
                if (self->event == get_event) {
                    if (!self->exception) {
                        //  prepare get command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare get command");
                        prepare_get_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send GET");
                        wap_proto_set_id (self->message, WAP_PROTO_GET);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_ok_state;
                }
                else
                if (self->event == put_event) {
                    if (!self->exception) {
                        //  prepare put command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare put command");
                        prepare_put_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send PUT
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send PUT");
                        wap_proto_set_id (self->message, WAP_PROTO_PUT);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_put_ok_state;
                }
                else
                if (self->event == save_event) {
                    if (!self->exception) {
                        //  prepare save command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare save command");
                        prepare_save_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SAVE
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send SAVE");
                        wap_proto_set_id (self->message, WAP_PROTO_SAVE);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_save_ok_state;
                }
                else
                if (self->event == start_event) {
                    if (!self->exception) {
                        //  prepare start command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare start command");
                        prepare_start_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send START
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send START");
                        wap_proto_set_id (self->message, WAP_PROTO_START);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_start_ok_state;
                }
                else
                if (self->event == stop_event) {
                    if (!self->exception) {
                        //  send STOP
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send STOP");
                        wap_proto_set_id (self->message, WAP_PROTO_STOP);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_stop_ok_state;
                }
                else
                if (self->event == output_indexes_event) {
                    if (!self->exception) {
                        //  prepare get output indexes command
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ prepare get output indexes command");
                        prepare_get_output_indexes_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send OUTPUT_INDEXES
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send OUTPUT_INDEXES");
                        wap_proto_set_id (self->message, WAP_PROTO_OUTPUT_INDEXES);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_output_indexes_ok_state;
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send CLOSE
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send CLOSE");
                        wap_proto_set_id (self->message, WAP_PROTO_CLOSE);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_close_ok_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  send PING
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send PING");
                        wap_proto_set_id (self->message, WAP_PROTO_PING);
                        wap_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_blocks_ok_state:
                if (self->event == blocks_ok_event) {
                    if (!self->exception) {
                        //  signal have blocks ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have blocks ok");
                        signal_have_blocks_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_ok_state:
                if (self->event == get_ok_event) {
                    if (!self->exception) {
                        //  signal have get ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have get ok");
                        signal_have_get_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_put_ok_state:
                if (self->event == put_ok_event) {
                    if (!self->exception) {
                        //  signal have put ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have put ok");
                        signal_have_put_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_save_ok_state:
                if (self->event == save_ok_event) {
                    if (!self->exception) {
                        //  signal have save ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have save ok");
                        signal_have_save_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_start_ok_state:
                if (self->event == start_ok_event) {
                    if (!self->exception) {
                        //  signal have start ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have start ok");
                        signal_have_start_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_stop_ok_state:
                if (self->event == stop_ok_event) {
                    if (!self->exception) {
                        //  signal have stop ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have stop ok");
                        signal_have_stop_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_output_indexes_ok_state:
                if (self->event == output_indexes_ok_event) {
                    if (!self->exception) {
                        //  signal have output indexes ok
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal have output indexes ok");
                        signal_have_output_indexes_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_close_ok_state:
                if (self->event == close_ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal success");
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal failure");
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case defaults_state:
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case have_error_state:
                if (self->event == command_invalid_event) {
                    if (!self->exception) {
                        //  use connect timeout
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ use connect timeout");
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send OPEN
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ send OPEN");
                        wap_proto_set_id (self->message, WAP_PROTO_OPEN);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = reexpect_open_ok_state;
                }
                else
                if (self->event == other_event) {
                    if (!self->exception) {
                        //  signal unhandled error
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ signal unhandled error");
                        signal_unhandled_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ terminate");
                        self->terminated = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("wap_client: unhandled event %s in %s",
                        s_event_name [self->event], s_state_name [self->state]);
                    assert (false);
                }
                break;

            case reexpect_open_ok_state:
                if (self->event == open_ok_event) {
                    if (!self->exception) {
                        //  use heartbeat timer
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ use heartbeat timer");
                        use_heartbeat_timer (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("wap_client:            $ check status code");
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (wap_client_verbose)
                zsys_debug ("wap_client:            ! %s", s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (wap_client_verbose)
            zsys_debug ("wap_client:            > %s", s_state_name [self->state]);
    }
}

//  zloop callback when client inactivity timer expires

static int
s_client_handle_timeout (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    return self->terminated? -1: 0;
}

//  zloop callback when client wakeup timer expires

static int
s_client_handle_wakeup (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, self->wakeup_event);
    return 0;
}


//  Handle command pipe to/from calling API

static int
s_client_handle_cmdpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    char *method = zstr_recv (self->cmdpipe);
    if (!method)
        return -1;                  //  Interrupted; exit zloop
    if (wap_client_verbose)
        zsys_debug ("wap_client:        API command=%s", method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "CONSTRUCTOR")) {
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.identity);
        zsock_recv (self->cmdpipe, "sis", &self->args.endpoint, &self->args.timeout, &self->args.identity);
        s_client_execute (self, constructor_event);
    }
    else
    if (streq (method, "DESTRUCTOR")) {
        s_client_execute (self, destructor_event);
    }
    else
    if (streq (method, "BLOCKS")) {
        zlist_destroy (&self->args.block_ids);
        zsock_recv (self->cmdpipe, "pi", &self->args.block_ids, &self->args.start_height);
        s_client_execute (self, blocks_event);
    }
    else
    if (streq (method, "PUT")) {
        zchunk_destroy (&self->args.tx_data);
        zsock_recv (self->cmdpipe, "p", &self->args.tx_data);
        s_client_execute (self, put_event);
    }
    else
    if (streq (method, "GET")) {
        zstr_free (&self->args.tx_id);
        zsock_recv (self->cmdpipe, "s", &self->args.tx_id);
        s_client_execute (self, get_event);
    }
    else
    if (streq (method, "SAVE")) {
        s_client_execute (self, save_event);
    }
    else
    if (streq (method, "OUTPUT INDEXES")) {
        zstr_free (&self->args.tx_id);
        zsock_recv (self->cmdpipe, "s", &self->args.tx_id);
        s_client_execute (self, output_indexes_event);
    }
    else
    if (streq (method, "START")) {
        zstr_free (&self->args.address);
        zsock_recv (self->cmdpipe, "si", &self->args.address, &self->args.thread_count);
        s_client_execute (self, start_event);
    }
    else
    if (streq (method, "STOP")) {
        s_client_execute (self, stop_event);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->cmdpipe)) {
        zsys_error ("wap_client: trailing API command frames (%s)", method);
        zmsg_t *more = zmsg_recv (self->cmdpipe);
        zmsg_print (more);
        zmsg_destroy (&more);
    }
    zstr_free (&method);
    return self->terminated? -1: 0;
}


//  Handle message pipe to/from calling API

static int
s_client_handle_msgpipe (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->msgpipe) & ZMQ_POLLIN) {
        char *method = zstr_recv (self->msgpipe);
        if (!method)
            return -1;              //  Interrupted; exit zloop
        if (wap_client_verbose)
            zsys_debug ("wap_client:        API message=%s", method);

        //  Front-end shuts down msgpipe before cmdpipe
        if (streq (method, "$TERM"))
            zsock_signal (self->cmdpipe, 0);
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->msgpipe)) {
            zsys_error ("wap_client: trailing API message frames (%s)", method);
            zmsg_t *more = zmsg_recv (self->msgpipe);
            zmsg_print (more);
            zmsg_destroy (&more);
        }
        zstr_free (&method);
    }
    return 0;
}


//  Handle a message (a protocol reply) from the server

static int
s_client_handle_protocol (zloop_t *loop, zsock_t *reader, void *argument)
{
    s_client_t *self = (s_client_t *) argument;

    //  We will process as many messages as we can, to reduce the overhead
    //  of polling and the reactor:
    while (zsock_events (self->dealer) & ZMQ_POLLIN) {
        if (wap_proto_recv (self->message, self->dealer))
            return -1;              //  Interrupted; exit zloop

        //  Any input from server counts as activity
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        //  Reset expiry timer if timeout is not zero
        if (self->timeout)
            self->expiry_timer = zloop_timer (
                self->loop, self->timeout, 1, s_client_handle_timeout, self);
        s_client_execute (self, s_protocol_event (self, self->message));
        if (self->terminated)
            return -1;
    }
    return 0;
}


//  ---------------------------------------------------------------------------
//  This is the client actor, which polls its two sockets and processes
//  incoming messages

void
wap_client (zsock_t *cmdpipe, void *msgpipe)
{
    //  Initialize
    s_client_t *self = s_client_new (cmdpipe, (zsock_t *) msgpipe);
    if (self) {
        zsock_signal (cmdpipe, 0);
        
        //  Set up handler for the sockets the client uses
        engine_handle_socket ((client_t *) self, self->cmdpipe, s_client_handle_cmdpipe);
        engine_handle_socket ((client_t *) self, self->msgpipe, s_client_handle_msgpipe);
        engine_handle_socket ((client_t *) self, self->dealer, s_client_handle_protocol);

        //  Run reactor until there's a termination signal
        zloop_start (self->loop);

        //  Reactor has ended
        s_client_destroy (&self);
    }
    else
        zsock_signal (cmdpipe, -1);
}


//  ---------------------------------------------------------------------------
//  Class interface

struct _wap_client_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    uint64_t start_height;      //  Returned by actor reply
    uint64_t curr_height;       //  Returned by actor reply
    zmsg_t *block_data;         //  Returned by actor reply
    zchunk_t *tx_data;          //  Returned by actor reply
    zframe_t *o_indexes;        //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new wap_client
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful. The caller may      
//  specify its address.                                                            

static int 
wap_client_constructor (wap_client_t *self, const char *endpoint, uint32_t timeout, const char *identity);

WAP_EXPORT wap_client_t *
wap_client_new (const char *endpoint, uint32_t timeout, const char *identity)
{
    wap_client_t *self = (wap_client_t *) zmalloc (sizeof (wap_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        self->actor = zactor_new (wap_client, backend);
        if (self->actor)
            self->status = wap_client_constructor (self, endpoint, timeout, identity);
        if (self->status == -1)
            zactor_destroy (&self->actor);
        if (!self->actor)
            wap_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the wap_client
//  Disconnect from server. Waits for a short timeout for confirmation from the     
//  server, then disconnects anyhow.                                                

static int 
wap_client_destructor (wap_client_t *self);

void
wap_client_destroy (wap_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        wap_client_t *self = *self_p;
        if (self->actor && !zsys_interrupted) {
            //  Shut down msgpipe first so that client can do clean shutdown,
            //  sending any pending messages and handshaking goodbye to server
            zstr_send (self->msgpipe, "$TERM");
            zsock_wait (self->actor);
            wap_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->reason);
        zmsg_destroy (&self->block_data);
        zchunk_destroy (&self->tx_data);
        zframe_destroy (&self->o_indexes);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.

zactor_t *
wap_client_actor (wap_client_t *self)
{
    assert (self);
    return self->actor;
}


//  ---------------------------------------------------------------------------
//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.

zsock_t *
wap_client_msgpipe (wap_client_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted or timed-out.

static int
s_accept_reply (wap_client_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted or timed-out
        
        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "SUCCESS")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "FAILURE")) {
                    zstr_free (&self->reason);
                    zsock_recv (self->actor, "is", &self->status, &self->reason);
                }
                else
                if (streq (reply, "BLOCKS OK")) {
                    zmsg_destroy (&self->block_data);
                    zsock_recv (self->actor, "iiip", &self->status, &self->start_height, &self->curr_height, &self->block_data);
                }
                else
                if (streq (reply, "PUT OK")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "GET OK")) {
                    zchunk_destroy (&self->tx_data);
                    zsock_recv (self->actor, "ip", &self->status, &self->tx_data);
                }
                else
                if (streq (reply, "SAVE OK")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "OUTPUT INDEXES OK")) {
                    zframe_destroy (&self->o_indexes);
                    zsock_recv (self->actor, "ip", &self->status, &self->o_indexes);
                }
                else
                if (streq (reply, "START OK")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "STOP OK")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                break;
            }
            filter = va_arg (args, char *);
        }
        va_end (args);
        //  If anything was remaining on pipe, flush it
        zsock_flush (self->actor);
        if (filter) {
            zstr_free (&reply);
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted or timed-out
}


//  ---------------------------------------------------------------------------
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful. The caller may      
//  specify its address.                                                            
//  Returns >= 0 if successful, -1 if interrupted.

static int 
wap_client_constructor (wap_client_t *self, const char *endpoint, uint32_t timeout, const char *identity)
{
    assert (self);
    zsock_send (self->actor, "ssis", "CONSTRUCTOR", endpoint, timeout, identity);
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Disconnect from server. Waits for a short timeout for confirmation from the     
//  server, then disconnects anyhow.                                                
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_destructor (wap_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "DESTRUCTOR");
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Request a set of blocks from the server.                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_blocks (wap_client_t *self, zlist_t **block_ids_p, uint64_t start_height)
{
    assert (self);
    zsock_send (self->actor, "spi", "BLOCKS", *block_ids_p, start_height);
    *block_ids_p = NULL;        //  Take ownership of block_ids
    if (s_accept_reply (self, "BLOCKS OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send a raw transaction to the daemon.                                           
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_put (wap_client_t *self, zchunk_t **tx_data_p)
{
    assert (self);
    zsock_send (self->actor, "sp", "PUT", *tx_data_p);
    *tx_data_p = NULL;          //  Take ownership of tx_data
    if (s_accept_reply (self, "PUT OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Request a set of blocks from the server.                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get (wap_client_t *self, const char *tx_id)
{
    assert (self);
    zsock_send (self->actor, "ss", "GET", tx_id);
    if (s_accept_reply (self, "GET OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Request a set of blocks from the server.                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_save (wap_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "SAVE");
    if (s_accept_reply (self, "SAVE OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for tx output indexes.                                                      
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_output_indexes (wap_client_t *self, const char *tx_id)
{
    assert (self);
    zsock_send (self->actor, "ss", "OUTPUT INDEXES", tx_id);
    if (s_accept_reply (self, "OUTPUT INDEXES OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send start command to server.                                                   
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_start (wap_client_t *self, const char *address, uint64_t thread_count)
{
    assert (self);
    zsock_send (self->actor, "ssi", "START", address, thread_count);
    if (s_accept_reply (self, "START OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send stop command to server.                                                    
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_stop (wap_client_t *self)
{
    assert (self);
    zsock_send (self->actor, "s", "STOP");
    if (s_accept_reply (self, "STOP OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received status

int 
wap_client_status (wap_client_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received reason

const char *
wap_client_reason (wap_client_t *self)
{
    assert (self);
    return self->reason;
}


//  ---------------------------------------------------------------------------
//  Return last received start_height

uint64_t 
wap_client_start_height (wap_client_t *self)
{
    assert (self);
    return self->start_height;
}


//  ---------------------------------------------------------------------------
//  Return last received curr_height

uint64_t 
wap_client_curr_height (wap_client_t *self)
{
    assert (self);
    return self->curr_height;
}


//  ---------------------------------------------------------------------------
//  Return last received block_data

zmsg_t *
wap_client_block_data (wap_client_t *self)
{
    assert (self);
    return self->block_data;
}


//  ---------------------------------------------------------------------------
//  Return last received tx_data

zchunk_t *
wap_client_tx_data (wap_client_t *self)
{
    assert (self);
    return self->tx_data;
}


//  ---------------------------------------------------------------------------
//  Return last received o_indexes

zframe_t *
wap_client_o_indexes (wap_client_t *self)
{
    assert (self);
    return self->o_indexes;
}
