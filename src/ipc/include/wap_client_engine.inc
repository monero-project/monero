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
    expect_send_raw_transaction_ok_state = 6,
    expect_save_bc_ok_state = 7,
    expect_start_ok_state = 8,
    expect_stop_ok_state = 9,
    expect_output_indexes_ok_state = 10,
    expect_random_outs_ok_state = 11,
    expect_get_height_ok_state = 12,
    expect_get_info_ok_state = 13,
    expect_get_peer_list_ok_state = 14,
    expect_get_mining_status_ok_state = 15,
    expect_set_log_hash_rate_ok_state = 16,
    expect_set_log_level_ok_state = 17,
    expect_start_save_graph_ok_state = 18,
    expect_stop_save_graph_ok_state = 19,
    expect_get_block_hash_ok_state = 20,
    expect_get_block_template_ok_state = 21,
    expect_get_hard_fork_info_ok_state = 22,
    expect_get_connections_list_ok_state = 23,
    expect_stop_daemon_ok_state = 24,
    expect_get_block_by_height_ok_state = 25,
    expect_get_block_by_hash_ok_state = 26,
    expect_close_ok_state = 27,
    defaults_state = 28,
    have_error_state = 29,
    reexpect_open_ok_state = 30
} state_t;

typedef enum {
    NULL_event = 0,
    connect_event = 1,
    bad_endpoint_event = 2,
    open_ok_event = 3,
    expired_event = 4,
    blocks_event = 5,
    get_event = 6,
    send_raw_transaction_event = 7,
    save_bc_event = 8,
    start_event = 9,
    stop_event = 10,
    output_indexes_event = 11,
    random_outs_event = 12,
    get_height_event = 13,
    get_info_event = 14,
    get_peer_list_event = 15,
    get_mining_status_event = 16,
    set_log_hash_rate_event = 17,
    set_log_level_event = 18,
    start_save_graph_event = 19,
    stop_save_graph_event = 20,
    get_block_hash_event = 21,
    get_block_template_event = 22,
    get_hard_fork_info_event = 23,
    get_connections_list_event = 24,
    stop_daemon_event = 25,
    get_block_by_height_event = 26,
    get_block_by_hash_event = 27,
    destructor_event = 28,
    blocks_ok_event = 29,
    get_ok_event = 30,
    send_raw_transaction_ok_event = 31,
    save_bc_ok_event = 32,
    start_ok_event = 33,
    stop_ok_event = 34,
    output_indexes_ok_event = 35,
    random_outs_ok_event = 36,
    get_height_ok_event = 37,
    get_info_ok_event = 38,
    get_peer_list_ok_event = 39,
    get_mining_status_ok_event = 40,
    set_log_hash_rate_ok_event = 41,
    set_log_level_ok_event = 42,
    start_save_graph_ok_event = 43,
    stop_save_graph_ok_event = 44,
    get_block_hash_ok_event = 45,
    get_block_template_ok_event = 46,
    get_hard_fork_info_ok_event = 47,
    get_connections_list_ok_event = 48,
    stop_daemon_ok_event = 49,
    get_block_by_height_ok_event = 50,
    get_block_by_hash_ok_event = 51,
    close_ok_event = 52,
    ping_ok_event = 53,
    error_event = 54,
    exception_event = 55,
    command_invalid_event = 56,
    other_event = 57
} event_t;

//  Names for state machine logging and error reporting
static const char *
s_state_name [] = {
    "(NONE)",
    "start",
    "expect open ok",
    "connected",
    "expect blocks ok",
    "expect get ok",
    "expect send raw transaction ok",
    "expect save bc ok",
    "expect start ok",
    "expect stop ok",
    "expect output indexes ok",
    "expect random outs ok",
    "expect get height ok",
    "expect get info ok",
    "expect get peer list ok",
    "expect get mining status ok",
    "expect set log hash rate ok",
    "expect set log level ok",
    "expect start save graph ok",
    "expect stop save graph ok",
    "expect get block hash ok",
    "expect get block template ok",
    "expect get hard fork info ok",
    "expect get connections list ok",
    "expect stop daemon ok",
    "expect get block by height ok",
    "expect get block by hash ok",
    "expect close ok",
    "defaults",
    "have error",
    "reexpect open ok"
};

static const char *
s_event_name [] = {
    "(NONE)",
    "connect",
    "bad_endpoint",
    "OPEN_OK",
    "expired",
    "BLOCKS",
    "GET",
    "SEND_RAW_TRANSACTION",
    "SAVE_BC",
    "START",
    "STOP",
    "OUTPUT_INDEXES",
    "RANDOM_OUTS",
    "GET_HEIGHT",
    "GET_INFO",
    "GET_PEER_LIST",
    "GET_MINING_STATUS",
    "SET_LOG_HASH_RATE",
    "SET_LOG_LEVEL",
    "START_SAVE_GRAPH",
    "STOP_SAVE_GRAPH",
    "GET_BLOCK_HASH",
    "GET_BLOCK_TEMPLATE",
    "GET_HARD_FORK_INFO",
    "GET_CONNECTIONS_LIST",
    "STOP_DAEMON",
    "GET_BLOCK_BY_HEIGHT",
    "GET_BLOCK_BY_HASH",
    "destructor",
    "BLOCKS_OK",
    "GET_OK",
    "SEND_RAW_TRANSACTION_OK",
    "SAVE_BC_OK",
    "START_OK",
    "STOP_OK",
    "OUTPUT_INDEXES_OK",
    "RANDOM_OUTS_OK",
    "GET_HEIGHT_OK",
    "GET_INFO_OK",
    "GET_PEER_LIST_OK",
    "GET_MINING_STATUS_OK",
    "SET_LOG_HASH_RATE_OK",
    "SET_LOG_LEVEL_OK",
    "START_SAVE_GRAPH_OK",
    "STOP_SAVE_GRAPH_OK",
    "GET_BLOCK_HASH_OK",
    "GET_BLOCK_TEMPLATE_OK",
    "GET_HARD_FORK_INFO_OK",
    "GET_CONNECTIONS_LIST_OK",
    "STOP_DAEMON_OK",
    "GET_BLOCK_BY_HEIGHT_OK",
    "GET_BLOCK_BY_HASH_OK",
    "CLOSE_OK",
    "PING_OK",
    "ERROR",
    "exception",
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
    zchunk_t *tx_as_hex;
    zchunk_t *tx_id;
    uint64_t outs_count;
    zframe_t *amounts;
    zchunk_t *address;
    uint64_t thread_count;
    uint8_t visible;
    uint8_t level;
    uint64_t height;
    uint64_t reserve_size;
    uint8_t header_only;
    uint8_t as_json;
    zchunk_t *hash;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    wap_proto_t *message;       //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool connected;             //  True if client is connected
    bool terminated;            //  True if client is shutdown
    bool fsm_stopped;           //  "terminate" action called
    size_t expiry;              //  Expiry timer, msecs
    size_t heartbeat;           //  Heartbeat timer, msecs
    state_t state;              //  Current state
    event_t event;              //  Current event
    event_t next_event;         //  The next event
    event_t exception;          //  Exception event, if any
    int expiry_timer;           //  zloop timer for expiry
    int wakeup_timer;           //  zloop timer for alarms
    int heartbeat_timer;        //  zloop timer for heartbeat
    event_t wakeup_event;       //  Wake up with this event
    char log_prefix [41];       //  Log prefix string
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
    s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument);
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
    client_is_connected (client_t *self);
static void
    signal_server_not_present (client_t *self);
static void
    prepare_blocks_command (client_t *self);
static void
    prepare_get_command (client_t *self);
static void
    prepare_send_raw_transaction_command (client_t *self);
static void
    prepare_start_command (client_t *self);
static void
    prepare_get_output_indexes_command (client_t *self);
static void
    prepare_get_random_outs_command (client_t *self);
static void
    prepare_set_log_hash_rate_command (client_t *self);
static void
    prepare_set_log_level_command (client_t *self);
static void
    prepare_get_block_hash_command (client_t *self);
static void
    prepare_get_block_template_command (client_t *self);
static void
    prepare_get_block_by_height_command (client_t *self);
static void
    prepare_get_block_by_hash_command (client_t *self);
static void
    check_if_connection_is_dead (client_t *self);
static void
    signal_have_blocks_ok (client_t *self);
static void
    signal_have_get_ok (client_t *self);
static void
    signal_have_send_raw_transaction_ok (client_t *self);
static void
    signal_have_save_bc_ok (client_t *self);
static void
    signal_have_start_ok (client_t *self);
static void
    signal_have_stop_ok (client_t *self);
static void
    signal_have_output_indexes_ok (client_t *self);
static void
    signal_have_random_outs_ok (client_t *self);
static void
    signal_have_get_height_ok (client_t *self);
static void
    signal_have_get_info_ok (client_t *self);
static void
    signal_have_get_peer_list_ok (client_t *self);
static void
    signal_have_get_mining_status_ok (client_t *self);
static void
    signal_have_set_log_hash_rate_ok (client_t *self);
static void
    signal_have_set_log_level_ok (client_t *self);
static void
    signal_have_start_save_graph_ok (client_t *self);
static void
    signal_have_stop_save_graph_ok (client_t *self);
static void
    signal_have_get_block_hash_ok (client_t *self);
static void
    signal_have_get_block_template_ok (client_t *self);
static void
    signal_have_get_hard_fork_info_ok (client_t *self);
static void
    signal_have_get_connections_list_ok (client_t *self);
static void
    signal_have_stop_daemon_ok (client_t *self);
static void
    signal_have_get_block_by_height_ok (client_t *self);
static void
    signal_have_get_block_by_hash_ok (client_t *self);
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
        self->state = start_state;
        self->event = NULL_event;
        snprintf (self->log_prefix, sizeof (self->log_prefix) - 1,
            "%6d:%-33s", randof (1000000), "wap_client");
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = wap_proto_new ();
        if (self->message)
            self->loop = zloop_new ();
        if (self->loop) {
            //  Give application chance to initialize and set next event
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
        zchunk_destroy (&self->args.tx_as_hex);
        zchunk_destroy (&self->args.tx_id);
        zframe_destroy (&self->args.amounts);
        zchunk_destroy (&self->args.address);
        zchunk_destroy (&self->args.hash);
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

//  Set a heartbeat timer. The interval is in msecs and must be
//  non-zero. The state machine must handle the "heartbeat" event.
//  The heartbeat happens every interval no matter what traffic the
//  client is sending or receiving.

static void
engine_set_heartbeat (client_t *client, size_t heartbeat)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->heartbeat = heartbeat;
    }
}


//  Set expiry timer. Setting a non-zero expiry causes the state machine
//  to receive an "expired" event if is no incoming traffic for that many
//  milliseconds. This cycles over and over until/unless the code sets a
//  zero expiry. The state machine must handle the "expired" event.

//  Macro to support deprecated name: remove after 2016-07-31
#define engine_set_timeout engine_set_expiry

static void
engine_set_expiry (client_t *client, size_t expiry)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->expiry = expiry;
        if (self->expiry_timer) {
            zloop_timer_end (self->loop, self->expiry_timer);
            self->expiry_timer = 0;
        }
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
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

//  Set connected to true/false. The client must call this if it wants
//  to provide the API with the connected status.

static void
engine_set_connected (client_t *client, bool connected)
{
    if (client) {
        s_client_t *self = (s_client_t *) client;
        self->connected = connected;
    }
}

//  Pedantic compilers don't like unused functions, so we call the whole
//  API, passing null references. It's nasty and horrid and sufficient.

static void
s_satisfy_pedantic_compilers (void)
{
    engine_set_next_event (NULL, NULL_event);
    engine_set_exception (NULL, NULL_event);
    engine_set_heartbeat (NULL, 0);
    engine_set_expiry (NULL, 0);
    engine_set_wakeup_event (NULL, 0, NULL_event);
    engine_handle_socket (NULL, 0, NULL);
    engine_set_connected (NULL, 0);
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
        case WAP_PROTO_SEND_RAW_TRANSACTION:
            return send_raw_transaction_event;
            break;
        case WAP_PROTO_SEND_RAW_TRANSACTION_OK:
            return send_raw_transaction_ok_event;
            break;
        case WAP_PROTO_OUTPUT_INDEXES:
            return output_indexes_event;
            break;
        case WAP_PROTO_OUTPUT_INDEXES_OK:
            return output_indexes_ok_event;
            break;
        case WAP_PROTO_RANDOM_OUTS:
            return random_outs_event;
            break;
        case WAP_PROTO_RANDOM_OUTS_OK:
            return random_outs_ok_event;
            break;
        case WAP_PROTO_GET_HEIGHT:
            return get_height_event;
            break;
        case WAP_PROTO_GET_HEIGHT_OK:
            return get_height_ok_event;
            break;
        case WAP_PROTO_GET:
            return get_event;
            break;
        case WAP_PROTO_GET_OK:
            return get_ok_event;
            break;
        case WAP_PROTO_SAVE_BC:
            return save_bc_event;
            break;
        case WAP_PROTO_SAVE_BC_OK:
            return save_bc_ok_event;
            break;
        case WAP_PROTO_START:
            return start_event;
            break;
        case WAP_PROTO_START_OK:
            return start_ok_event;
            break;
        case WAP_PROTO_GET_INFO:
            return get_info_event;
            break;
        case WAP_PROTO_GET_INFO_OK:
            return get_info_ok_event;
            break;
        case WAP_PROTO_GET_PEER_LIST:
            return get_peer_list_event;
            break;
        case WAP_PROTO_GET_PEER_LIST_OK:
            return get_peer_list_ok_event;
            break;
        case WAP_PROTO_GET_MINING_STATUS:
            return get_mining_status_event;
            break;
        case WAP_PROTO_GET_MINING_STATUS_OK:
            return get_mining_status_ok_event;
            break;
        case WAP_PROTO_SET_LOG_HASH_RATE:
            return set_log_hash_rate_event;
            break;
        case WAP_PROTO_SET_LOG_HASH_RATE_OK:
            return set_log_hash_rate_ok_event;
            break;
        case WAP_PROTO_SET_LOG_LEVEL:
            return set_log_level_event;
            break;
        case WAP_PROTO_SET_LOG_LEVEL_OK:
            return set_log_level_ok_event;
            break;
        case WAP_PROTO_START_SAVE_GRAPH:
            return start_save_graph_event;
            break;
        case WAP_PROTO_START_SAVE_GRAPH_OK:
            return start_save_graph_ok_event;
            break;
        case WAP_PROTO_STOP_SAVE_GRAPH:
            return stop_save_graph_event;
            break;
        case WAP_PROTO_STOP_SAVE_GRAPH_OK:
            return stop_save_graph_ok_event;
            break;
        case WAP_PROTO_GET_BLOCK_HASH:
            return get_block_hash_event;
            break;
        case WAP_PROTO_GET_BLOCK_HASH_OK:
            return get_block_hash_ok_event;
            break;
        case WAP_PROTO_GET_BLOCK_TEMPLATE:
            return get_block_template_event;
            break;
        case WAP_PROTO_GET_BLOCK_TEMPLATE_OK:
            return get_block_template_ok_event;
            break;
        case WAP_PROTO_STOP:
            return stop_event;
            break;
        case WAP_PROTO_STOP_OK:
            return stop_ok_event;
            break;
        case WAP_PROTO_GET_HARD_FORK_INFO:
            return get_hard_fork_info_event;
            break;
        case WAP_PROTO_GET_HARD_FORK_INFO_OK:
            return get_hard_fork_info_ok_event;
            break;
        case WAP_PROTO_GET_CONNECTIONS_LIST:
            return get_connections_list_event;
            break;
        case WAP_PROTO_GET_CONNECTIONS_LIST_OK:
            return get_connections_list_ok_event;
            break;
        case WAP_PROTO_STOP_DAEMON:
            return stop_daemon_event;
            break;
        case WAP_PROTO_STOP_DAEMON_OK:
            return stop_daemon_ok_event;
            break;
        case WAP_PROTO_GET_BLOCK_BY_HEIGHT:
            return get_block_by_height_event;
            break;
        case WAP_PROTO_GET_BLOCK_BY_HEIGHT_OK:
            return get_block_by_height_ok_event;
            break;
        case WAP_PROTO_GET_BLOCK_BY_HASH:
            return get_block_by_hash_event;
            break;
        case WAP_PROTO_GET_BLOCK_BY_HASH_OK:
            return get_block_by_hash_ok_event;
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
            zsys_error ("%s: unknown command %s, halting",
                        self->log_prefix, wap_proto_command (message));
            self->terminated = true;
            return NULL_event;
    }
}


//  Execute state machine as long as we have events; if event is NULL_event,
//  or state machine is stopped, do nothing.

static void
s_client_execute (s_client_t *self, event_t event)
{
    self->next_event = event;
    //  Cancel wakeup timer, if any was pending
    if (self->wakeup_timer) {
        zloop_timer_end (self->loop, self->wakeup_timer);
        self->wakeup_timer = 0;
    }
    while (!self->terminated                    //  Actor is dying
        && !self->fsm_stopped                   //  FSM has finished
        && self->next_event != NULL_event) {
        self->event = self->next_event;
        self->next_event = NULL_event;
        self->exception = NULL_event;
        if (wap_client_verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == connect_event) {
                    if (!self->exception) {
                        //  connect to server endpoint
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ connect to server endpoint", self->log_prefix);
                        connect_to_server_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  set client identity
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ set client identity", self->log_prefix);
                        set_client_identity (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ use connect timeout", self->log_prefix);
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send OPEN
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send OPEN",
                                self->log_prefix);
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
                            zsys_debug ("%s:         $ signal bad endpoint", self->log_prefix);
                        signal_bad_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("%s: unhandled event %s in %s",
                        self->log_prefix,
                        s_event_name [self->event],
                        s_state_name [self->state]);
                    assert (false);
                }
                break;

            case expect_open_ok_state:
                if (self->event == open_ok_event) {
                    if (!self->exception) {
                        //  signal success
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal server not present
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal server not present", self->log_prefix);
                        signal_server_not_present (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ prepare blocks command", self->log_prefix);
                        prepare_blocks_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send BLOCKS
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send BLOCKS",
                                self->log_prefix);
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
                            zsys_debug ("%s:         $ prepare get command", self->log_prefix);
                        prepare_get_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_ok_state;
                }
                else
                if (self->event == send_raw_transaction_event) {
                    if (!self->exception) {
                        //  prepare send raw transaction command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare send raw transaction command", self->log_prefix);
                        prepare_send_raw_transaction_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SEND_RAW_TRANSACTION
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send SEND_RAW_TRANSACTION",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_SEND_RAW_TRANSACTION);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_send_raw_transaction_ok_state;
                }
                else
                if (self->event == save_bc_event) {
                    if (!self->exception) {
                        //  send SAVE_BC
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send SAVE_BC",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_SAVE_BC);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_save_bc_ok_state;
                }
                else
                if (self->event == start_event) {
                    if (!self->exception) {
                        //  prepare start command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare start command", self->log_prefix);
                        prepare_start_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send START
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send START",
                                self->log_prefix);
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
                            zsys_debug ("%s:         $ send STOP",
                                self->log_prefix);
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
                            zsys_debug ("%s:         $ prepare get output indexes command", self->log_prefix);
                        prepare_get_output_indexes_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send OUTPUT_INDEXES
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send OUTPUT_INDEXES",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_OUTPUT_INDEXES);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_output_indexes_ok_state;
                }
                else
                if (self->event == random_outs_event) {
                    if (!self->exception) {
                        //  prepare get random outs command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare get random outs command", self->log_prefix);
                        prepare_get_random_outs_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send RANDOM_OUTS
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send RANDOM_OUTS",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_RANDOM_OUTS);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_random_outs_ok_state;
                }
                else
                if (self->event == get_height_event) {
                    if (!self->exception) {
                        //  send GET_HEIGHT
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_HEIGHT",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_HEIGHT);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_height_ok_state;
                }
                else
                if (self->event == get_info_event) {
                    if (!self->exception) {
                        //  send GET_INFO
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_INFO",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_INFO);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_info_ok_state;
                }
                else
                if (self->event == get_peer_list_event) {
                    if (!self->exception) {
                        //  send GET_PEER_LIST
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_PEER_LIST",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_PEER_LIST);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_peer_list_ok_state;
                }
                else
                if (self->event == get_mining_status_event) {
                    if (!self->exception) {
                        //  send GET_MINING_STATUS
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_MINING_STATUS",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_MINING_STATUS);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_mining_status_ok_state;
                }
                else
                if (self->event == set_log_hash_rate_event) {
                    if (!self->exception) {
                        //  prepare set log hash rate command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare set log hash rate command", self->log_prefix);
                        prepare_set_log_hash_rate_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SET_LOG_HASH_RATE
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send SET_LOG_HASH_RATE",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_SET_LOG_HASH_RATE);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_set_log_hash_rate_ok_state;
                }
                else
                if (self->event == set_log_level_event) {
                    if (!self->exception) {
                        //  prepare set log level command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare set log level command", self->log_prefix);
                        prepare_set_log_level_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send SET_LOG_LEVEL
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send SET_LOG_LEVEL",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_SET_LOG_LEVEL);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_set_log_level_ok_state;
                }
                else
                if (self->event == start_save_graph_event) {
                    if (!self->exception) {
                        //  send START_SAVE_GRAPH
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send START_SAVE_GRAPH",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_START_SAVE_GRAPH);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_start_save_graph_ok_state;
                }
                else
                if (self->event == stop_save_graph_event) {
                    if (!self->exception) {
                        //  send STOP_SAVE_GRAPH
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send STOP_SAVE_GRAPH",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_STOP_SAVE_GRAPH);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_stop_save_graph_ok_state;
                }
                else
                if (self->event == get_block_hash_event) {
                    if (!self->exception) {
                        //  prepare get block hash command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare get block hash command", self->log_prefix);
                        prepare_get_block_hash_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET_BLOCK_HASH
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_BLOCK_HASH",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_BLOCK_HASH);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_block_hash_ok_state;
                }
                else
                if (self->event == get_block_template_event) {
                    if (!self->exception) {
                        //  prepare get block template command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare get block template command", self->log_prefix);
                        prepare_get_block_template_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET_BLOCK_TEMPLATE
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_BLOCK_TEMPLATE",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_BLOCK_TEMPLATE);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_block_template_ok_state;
                }
                else
                if (self->event == get_hard_fork_info_event) {
                    if (!self->exception) {
                        //  send GET_HARD_FORK_INFO
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_HARD_FORK_INFO",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_HARD_FORK_INFO);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_hard_fork_info_ok_state;
                }
                else
                if (self->event == get_connections_list_event) {
                    if (!self->exception) {
                        //  send GET_CONNECTIONS_LIST
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_CONNECTIONS_LIST",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_CONNECTIONS_LIST);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_connections_list_ok_state;
                }
                else
                if (self->event == stop_daemon_event) {
                    if (!self->exception) {
                        //  send STOP_DAEMON
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send STOP_DAEMON",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_STOP_DAEMON);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_stop_daemon_ok_state;
                }
                else
                if (self->event == get_block_by_height_event) {
                    if (!self->exception) {
                        //  prepare get block by height command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare get block by height command", self->log_prefix);
                        prepare_get_block_by_height_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET_BLOCK_BY_HEIGHT
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_BLOCK_BY_HEIGHT",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_BLOCK_BY_HEIGHT);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_block_by_height_ok_state;
                }
                else
                if (self->event == get_block_by_hash_event) {
                    if (!self->exception) {
                        //  prepare get block by hash command
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ prepare get block by hash command", self->log_prefix);
                        prepare_get_block_by_hash_command (&self->client);
                    }
                    if (!self->exception) {
                        //  send GET_BLOCK_BY_HASH
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send GET_BLOCK_BY_HASH",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_GET_BLOCK_BY_HASH);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_get_block_by_hash_ok_state;
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send CLOSE
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send CLOSE",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_CLOSE);
                        wap_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_close_ok_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  check if connection is dead
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check if connection is dead", self->log_prefix);
                        check_if_connection_is_dead (&self->client);
                    }
                    if (!self->exception) {
                        //  send PING
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        wap_proto_set_id (self->message, WAP_PROTO_PING);
                        wap_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal have blocks ok", self->log_prefix);
                        signal_have_blocks_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal have get ok", self->log_prefix);
                        signal_have_get_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_send_raw_transaction_ok_state:
                if (self->event == send_raw_transaction_ok_event) {
                    if (!self->exception) {
                        //  signal have send raw transaction ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have send raw transaction ok", self->log_prefix);
                        signal_have_send_raw_transaction_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_save_bc_ok_state:
                if (self->event == save_bc_ok_event) {
                    if (!self->exception) {
                        //  signal have save bc ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have save bc ok", self->log_prefix);
                        signal_have_save_bc_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal have start ok", self->log_prefix);
                        signal_have_start_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal have stop ok", self->log_prefix);
                        signal_have_stop_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal have output indexes ok", self->log_prefix);
                        signal_have_output_indexes_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_random_outs_ok_state:
                if (self->event == random_outs_ok_event) {
                    if (!self->exception) {
                        //  signal have random outs ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have random outs ok", self->log_prefix);
                        signal_have_random_outs_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_height_ok_state:
                if (self->event == get_height_ok_event) {
                    if (!self->exception) {
                        //  signal have get height ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get height ok", self->log_prefix);
                        signal_have_get_height_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_info_ok_state:
                if (self->event == get_info_ok_event) {
                    if (!self->exception) {
                        //  signal have get info ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get info ok", self->log_prefix);
                        signal_have_get_info_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_peer_list_ok_state:
                if (self->event == get_peer_list_ok_event) {
                    if (!self->exception) {
                        //  signal have get peer list ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get peer list ok", self->log_prefix);
                        signal_have_get_peer_list_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_mining_status_ok_state:
                if (self->event == get_mining_status_ok_event) {
                    if (!self->exception) {
                        //  signal have get mining status ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get mining status ok", self->log_prefix);
                        signal_have_get_mining_status_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_set_log_hash_rate_ok_state:
                if (self->event == set_log_hash_rate_ok_event) {
                    if (!self->exception) {
                        //  signal have set log hash rate ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have set log hash rate ok", self->log_prefix);
                        signal_have_set_log_hash_rate_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_set_log_level_ok_state:
                if (self->event == set_log_level_ok_event) {
                    if (!self->exception) {
                        //  signal have set log level ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have set log level ok", self->log_prefix);
                        signal_have_set_log_level_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_start_save_graph_ok_state:
                if (self->event == start_save_graph_ok_event) {
                    if (!self->exception) {
                        //  signal have start save graph ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have start save graph ok", self->log_prefix);
                        signal_have_start_save_graph_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_stop_save_graph_ok_state:
                if (self->event == stop_save_graph_ok_event) {
                    if (!self->exception) {
                        //  signal have stop save graph ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have stop save graph ok", self->log_prefix);
                        signal_have_stop_save_graph_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_block_hash_ok_state:
                if (self->event == get_block_hash_ok_event) {
                    if (!self->exception) {
                        //  signal have get block hash ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get block hash ok", self->log_prefix);
                        signal_have_get_block_hash_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_block_template_ok_state:
                if (self->event == get_block_template_ok_event) {
                    if (!self->exception) {
                        //  signal have get block template ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get block template ok", self->log_prefix);
                        signal_have_get_block_template_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_hard_fork_info_ok_state:
                if (self->event == get_hard_fork_info_ok_event) {
                    if (!self->exception) {
                        //  signal have get hard fork info ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get hard fork info ok", self->log_prefix);
                        signal_have_get_hard_fork_info_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_connections_list_ok_state:
                if (self->event == get_connections_list_ok_event) {
                    if (!self->exception) {
                        //  signal have get connections list ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get connections list ok", self->log_prefix);
                        signal_have_get_connections_list_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_stop_daemon_ok_state:
                if (self->event == stop_daemon_ok_event) {
                    if (!self->exception) {
                        //  signal have stop daemon ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have stop daemon ok", self->log_prefix);
                        signal_have_stop_daemon_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_block_by_height_ok_state:
                if (self->event == get_block_by_height_ok_event) {
                    if (!self->exception) {
                        //  signal have get block by height ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get block by height ok", self->log_prefix);
                        signal_have_get_block_by_height_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case expect_get_block_by_hash_ok_state:
                if (self->event == get_block_by_hash_ok_event) {
                    if (!self->exception) {
                        //  signal have get block by hash ok
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal have get block by hash ok", self->log_prefix);
                        signal_have_get_block_by_hash_ok (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  signal failure
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ signal failure", self->log_prefix);
                        signal_failure (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case defaults_state:
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
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
                            zsys_debug ("%s:         $ use connect timeout", self->log_prefix);
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception) {
                        //  send OPEN
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ send OPEN",
                                self->log_prefix);
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
                            zsys_debug ("%s:         $ signal unhandled error", self->log_prefix);
                        signal_unhandled_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else {
                    //  Handle unexpected internal events
                    zsys_warning ("%s: unhandled event %s in %s",
                        self->log_prefix,
                        s_event_name [self->event],
                        s_state_name [self->state]);
                    assert (false);
                }
                break;

            case reexpect_open_ok_state:
                if (self->event == open_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == ping_ok_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (wap_client_verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception)
                        self->state = have_error_state;
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (wap_client_verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (wap_client_verbose)
            zsys_debug ("%s:         > %s",
                    self->log_prefix, s_state_name [self->state]);
    }
}

//  zloop callback when client expiry timeout expires

static int
s_client_handle_expiry (zloop_t *loop, int timer_id, void *argument)
{
    s_client_t *self = (s_client_t *) argument;
    s_client_execute (self, expired_event);
    if (self->terminated)
        return -1;

    if (self->expiry > 0)
        self->expiry_timer = zloop_timer (
            loop, self->expiry, 1, s_client_handle_expiry, self);
    return 0;
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
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "$CONNECTED"))
        zsock_send (self->cmdpipe, "i", self->connected);
    else
    if (streq (method, "CONNECT")) {
        zstr_free (&self->args.endpoint);
        zstr_free (&self->args.identity);
        zsock_recv (self->cmdpipe, "s4s", &self->args.endpoint, &self->args.timeout, &self->args.identity);
        s_client_execute (self, connect_event);
    }
    else
    if (streq (method, "DESTRUCTOR")) {
        s_client_execute (self, destructor_event);
    }
    else
    if (streq (method, "BLOCKS")) {
        zlist_destroy (&self->args.block_ids);
        zsock_recv (self->cmdpipe, "p8", &self->args.block_ids, &self->args.start_height);
        s_client_execute (self, blocks_event);
    }
    else
    if (streq (method, "SEND RAW TRANSACTION")) {
        zchunk_destroy (&self->args.tx_as_hex);
        zsock_recv (self->cmdpipe, "p", &self->args.tx_as_hex);
        s_client_execute (self, send_raw_transaction_event);
    }
    else
    if (streq (method, "GET")) {
        zchunk_destroy (&self->args.tx_id);
        zsock_recv (self->cmdpipe, "p", &self->args.tx_id);
        s_client_execute (self, get_event);
    }
    else
    if (streq (method, "SAVE BC")) {
        s_client_execute (self, save_bc_event);
    }
    else
    if (streq (method, "OUTPUT INDEXES")) {
        zchunk_destroy (&self->args.tx_id);
        zsock_recv (self->cmdpipe, "p", &self->args.tx_id);
        s_client_execute (self, output_indexes_event);
    }
    else
    if (streq (method, "RANDOM OUTS")) {
        zframe_destroy (&self->args.amounts);
        zsock_recv (self->cmdpipe, "8p", &self->args.outs_count, &self->args.amounts);
        s_client_execute (self, random_outs_event);
    }
    else
    if (streq (method, "GET HEIGHT")) {
        s_client_execute (self, get_height_event);
    }
    else
    if (streq (method, "GET INFO")) {
        s_client_execute (self, get_info_event);
    }
    else
    if (streq (method, "START")) {
        zchunk_destroy (&self->args.address);
        zsock_recv (self->cmdpipe, "p8", &self->args.address, &self->args.thread_count);
        s_client_execute (self, start_event);
    }
    else
    if (streq (method, "STOP")) {
        s_client_execute (self, stop_event);
    }
    else
    if (streq (method, "GET PEER LIST")) {
        s_client_execute (self, get_peer_list_event);
    }
    else
    if (streq (method, "GET MINING STATUS")) {
        s_client_execute (self, get_mining_status_event);
    }
    else
    if (streq (method, "SET LOG HASH RATE")) {
        zsock_recv (self->cmdpipe, "1", &self->args.visible);
        s_client_execute (self, set_log_hash_rate_event);
    }
    else
    if (streq (method, "SET LOG LEVEL")) {
        zsock_recv (self->cmdpipe, "1", &self->args.level);
        s_client_execute (self, set_log_level_event);
    }
    else
    if (streq (method, "START SAVE GRAPH")) {
        s_client_execute (self, start_save_graph_event);
    }
    else
    if (streq (method, "STOP SAVE GRAPH")) {
        s_client_execute (self, stop_save_graph_event);
    }
    else
    if (streq (method, "GET BLOCK HASH")) {
        zsock_recv (self->cmdpipe, "8", &self->args.height);
        s_client_execute (self, get_block_hash_event);
    }
    else
    if (streq (method, "GET BLOCK TEMPLATE")) {
        zchunk_destroy (&self->args.address);
        zsock_recv (self->cmdpipe, "8p", &self->args.reserve_size, &self->args.address);
        s_client_execute (self, get_block_template_event);
    }
    else
    if (streq (method, "GET HARD FORK INFO")) {
        s_client_execute (self, get_hard_fork_info_event);
    }
    else
    if (streq (method, "GET CONNECTIONS LIST")) {
        s_client_execute (self, get_connections_list_event);
    }
    else
    if (streq (method, "STOP DAEMON")) {
        s_client_execute (self, stop_daemon_event);
    }
    else
    if (streq (method, "GET BLOCK BY HEIGHT")) {
        zsock_recv (self->cmdpipe, "811", &self->args.height, &self->args.header_only, &self->args.as_json);
        s_client_execute (self, get_block_by_height_event);
    }
    else
    if (streq (method, "GET BLOCK BY HASH")) {
        zchunk_destroy (&self->args.hash);
        zsock_recv (self->cmdpipe, "p11", &self->args.hash, &self->args.header_only, &self->args.as_json);
        s_client_execute (self, get_block_by_hash_event);
    }
    //  Cleanup pipe if any argument frames are still waiting to be eaten
    if (zsock_rcvmore (self->cmdpipe)) {
        zsys_error ("%s: trailing API command frames (%s)",
            self->log_prefix, method);
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
            zsys_debug ("%s:     API message=%s", self->log_prefix, method);

        //  Front-end shuts down msgpipe before cmdpipe, this little
        //  handshake just ensures all traffic on the msgpipe has been
        //  flushed before the calling thread continues with destroying
        //  the actor.
        if (streq (method, "$FLUSH"))
            zsock_signal (self->cmdpipe, 0);
        //  Cleanup pipe if any argument frames are still waiting to be eaten
        if (zsock_rcvmore (self->msgpipe)) {
            zsys_error ("%s: trailing API message frames (%s)", self->log_prefix, method);
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
        //  Reset expiry timer if expiry timeout not zero
        if (self->expiry)
            self->expiry_timer = zloop_timer (
                self->loop, self->expiry, 1, s_client_handle_expiry, self);
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
    bool connected;             //  Client currently connected or not
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
    uint64_t start_height;      //  Returned by actor reply
    uint64_t curr_height;       //  Returned by actor reply
    zmsg_t *block_data;         //  Returned by actor reply
    zchunk_t *tx_data;          //  Returned by actor reply
    zframe_t *o_indexes;        //  Returned by actor reply
    zframe_t *random_outputs;   //  Returned by actor reply
    uint64_t height;            //  Returned by actor reply
    uint64_t target_height;     //  Returned by actor reply
    uint64_t difficulty;        //  Returned by actor reply
    uint64_t tx_count;          //  Returned by actor reply
    uint64_t tx_pool_size;      //  Returned by actor reply
    uint64_t alt_blocks_count;  //  Returned by actor reply
    uint64_t outgoing_connections_count;  //  Returned by actor reply
    uint64_t incoming_connections_count;  //  Returned by actor reply
    uint64_t white_peerlist_size;  //  Returned by actor reply
    uint64_t grey_peerlist_size;  //  Returned by actor reply
    uint8_t testnet;            //  Returned by actor reply
    zframe_t *white_list;       //  Returned by actor reply
    zframe_t *gray_list;        //  Returned by actor reply
    uint8_t active;             //  Returned by actor reply
    uint64_t speed;             //  Returned by actor reply
    uint64_t thread_count;      //  Returned by actor reply
    zchunk_t *address;          //  Returned by actor reply
    zchunk_t *hash;             //  Returned by actor reply
    uint64_t reserved_offset;   //  Returned by actor reply
    zchunk_t *prev_hash;        //  Returned by actor reply
    zchunk_t *block_template_blob;  //  Returned by actor reply
    uint8_t hfversion;          //  Returned by actor reply
    uint8_t enabled;            //  Returned by actor reply
    uint32_t window;            //  Returned by actor reply
    uint32_t votes;             //  Returned by actor reply
    uint32_t threshold;         //  Returned by actor reply
    uint8_t voting;             //  Returned by actor reply
    uint32_t hfstate;           //  Returned by actor reply
    zframe_t *connections;      //  Returned by actor reply
    zchunk_t *block;            //  Returned by actor reply
    uint8_t major_version;      //  Returned by actor reply
    uint8_t minor_version;      //  Returned by actor reply
    uint64_t timestamp;         //  Returned by actor reply
    uint32_t nonce;             //  Returned by actor reply
    uint8_t orphan;             //  Returned by actor reply
    uint64_t depth;             //  Returned by actor reply
    uint64_t reward;            //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new wap_client

WAP_EXPORT wap_client_t *
wap_client_new (void)
{
    wap_client_t *self = (wap_client_t *) zmalloc (sizeof (wap_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        if (self->msgpipe)
            self->actor = zactor_new (wap_client, backend);
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
            //  Before destroying the actor we have to flush any pending
            //  traffic on the msgpipe, otherwise it gets lost in a fire and
            //  forget scenario. We do this by sending $FLUSH to the msgpipe
            //  and waiting for a signal back on the cmdpipe.
            if (zstr_send (self->msgpipe, "$FLUSH") == 0)
                zsock_wait (self->actor);
            wap_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->reason);
        zmsg_destroy (&self->block_data);
        zchunk_destroy (&self->tx_data);
        zframe_destroy (&self->o_indexes);
        zframe_destroy (&self->random_outputs);
        zframe_destroy (&self->white_list);
        zframe_destroy (&self->gray_list);
        zchunk_destroy (&self->address);
        zchunk_destroy (&self->hash);
        zchunk_destroy (&self->prev_hash);
        zchunk_destroy (&self->block_template_blob);
        zframe_destroy (&self->connections);
        zchunk_destroy (&self->block);
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
//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.

bool
wap_client_connected (wap_client_t *self)
{
    assert (self);
    int connected;
    zsock_send (self->actor, "s", "$CONNECTED");
    zsock_recv (self->actor, "i", &connected);
    return (bool) connected;
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
                    zsock_recv (self->actor, "488p", &self->status, &self->start_height, &self->curr_height, &self->block_data);
                }
                else
                if (streq (reply, "SEND RAW TRANSACTION OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "GET OK")) {
                    zchunk_destroy (&self->tx_data);
                    zsock_recv (self->actor, "ip", &self->status, &self->tx_data);
                }
                else
                if (streq (reply, "SAVE BC OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "OUTPUT INDEXES OK")) {
                    zframe_destroy (&self->o_indexes);
                    zsock_recv (self->actor, "4p", &self->status, &self->o_indexes);
                }
                else
                if (streq (reply, "RANDOM OUTS OK")) {
                    zframe_destroy (&self->random_outputs);
                    zsock_recv (self->actor, "4p", &self->status, &self->random_outputs);
                }
                else
                if (streq (reply, "GET HEIGHT OK")) {
                    zsock_recv (self->actor, "48", &self->status, &self->height);
                }
                else
                if (streq (reply, "GET INFO OK")) {
                    zsock_recv (self->actor, "488888888881", &self->status, &self->height, &self->target_height, &self->difficulty, &self->tx_count, &self->tx_pool_size, &self->alt_blocks_count, &self->outgoing_connections_count, &self->incoming_connections_count, &self->white_peerlist_size, &self->grey_peerlist_size, &self->testnet);
                }
                else
                if (streq (reply, "START OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "STOP OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "GET PEER LIST OK")) {
                    zframe_destroy (&self->white_list);
                    zframe_destroy (&self->gray_list);
                    zsock_recv (self->actor, "4pp", &self->status, &self->white_list, &self->gray_list);
                }
                else
                if (streq (reply, "GET MINING STATUS OK")) {
                    zchunk_destroy (&self->address);
                    zsock_recv (self->actor, "4188p", &self->status, &self->active, &self->speed, &self->thread_count, &self->address);
                }
                else
                if (streq (reply, "SET LOG HASH RATE OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "SET LOG LEVEL OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "START SAVE GRAPH OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "STOP SAVE GRAPH OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "GET BLOCK HASH OK")) {
                    zchunk_destroy (&self->hash);
                    zsock_recv (self->actor, "4p", &self->status, &self->hash);
                }
                else
                if (streq (reply, "GET BLOCK TEMPLATE OK")) {
                    zchunk_destroy (&self->prev_hash);
                    zchunk_destroy (&self->block_template_blob);
                    zsock_recv (self->actor, "4888pp", &self->status, &self->reserved_offset, &self->height, &self->difficulty, &self->prev_hash, &self->block_template_blob);
                }
                else
                if (streq (reply, "GET HARD FORK INFO OK")) {
                    zsock_recv (self->actor, "41144414", &self->status, &self->hfversion, &self->enabled, &self->window, &self->votes, &self->threshold, &self->voting, &self->hfstate);
                }
                else
                if (streq (reply, "GET CONNECTIONS LIST OK")) {
                    zframe_destroy (&self->connections);
                    zsock_recv (self->actor, "4p", &self->status, &self->connections);
                }
                else
                if (streq (reply, "STOP DAEMON OK")) {
                    zsock_recv (self->actor, "4", &self->status);
                }
                else
                if (streq (reply, "GET BLOCK BY HEIGHT OK")) {
                    zchunk_destroy (&self->block);
                    zchunk_destroy (&self->prev_hash);
                    zchunk_destroy (&self->hash);
                    zsock_recv (self->actor, "4p118p4188p88", &self->status, &self->block, &self->major_version, &self->minor_version, &self->timestamp, &self->prev_hash, &self->nonce, &self->orphan, &self->height, &self->depth, &self->hash, &self->difficulty, &self->reward);
                }
                else
                if (streq (reply, "GET BLOCK BY HASH OK")) {
                    zchunk_destroy (&self->block);
                    zchunk_destroy (&self->prev_hash);
                    zchunk_destroy (&self->hash);
                    zsock_recv (self->actor, "4p118p4188p88", &self->status, &self->block, &self->major_version, &self->minor_version, &self->timestamp, &self->prev_hash, &self->nonce, &self->orphan, &self->height, &self->depth, &self->hash, &self->difficulty, &self->reward);
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

int 
wap_client_connect (wap_client_t *self, const char *endpoint, uint32_t timeout, const char *identity)
{
    assert (self);

    zsock_send (self->actor, "ss4s", "CONNECT", endpoint, timeout, identity);
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

    zsock_send (self->actor, "sp8", "BLOCKS", *block_ids_p, start_height);
    *block_ids_p = NULL;        //  Take ownership of block_ids
    if (s_accept_reply (self, "BLOCKS OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send a raw transaction to the daemon.                                           
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_send_raw_transaction (wap_client_t *self, zchunk_t **tx_as_hex_p)
{
    assert (self);

    zsock_send (self->actor, "sp", "SEND RAW TRANSACTION", *tx_as_hex_p);
    *tx_as_hex_p = NULL;        //  Take ownership of tx_as_hex
    if (s_accept_reply (self, "SEND RAW TRANSACTION OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Request a set of blocks from the server.                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get (wap_client_t *self, zchunk_t **tx_id_p)
{
    assert (self);

    zsock_send (self->actor, "sp", "GET", *tx_id_p);
    *tx_id_p = NULL;            //  Take ownership of tx_id
    if (s_accept_reply (self, "GET OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Request a set of blocks from the server.                                        
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_save_bc (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "SAVE BC");
    if (s_accept_reply (self, "SAVE BC OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for tx output indexes.                                                      
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_output_indexes (wap_client_t *self, zchunk_t **tx_id_p)
{
    assert (self);

    zsock_send (self->actor, "sp", "OUTPUT INDEXES", *tx_id_p);
    *tx_id_p = NULL;            //  Take ownership of tx_id
    if (s_accept_reply (self, "OUTPUT INDEXES OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for tx output indexes.                                                      
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_random_outs (wap_client_t *self, uint64_t outs_count, zframe_t **amounts_p)
{
    assert (self);

    zsock_send (self->actor, "s8p", "RANDOM OUTS", outs_count, *amounts_p);
    *amounts_p = NULL;          //  Take ownership of amounts
    if (s_accept_reply (self, "RANDOM OUTS OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for height.                                                                 
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_height (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET HEIGHT");
    if (s_accept_reply (self, "GET HEIGHT OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for height.                                                                 
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_info (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET INFO");
    if (s_accept_reply (self, "GET INFO OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Send start command to server.                                                   
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_start (wap_client_t *self, zchunk_t **address_p, uint64_t thread_count)
{
    assert (self);

    zsock_send (self->actor, "sp8", "START", *address_p, thread_count);
    *address_p = NULL;          //  Take ownership of address
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
//  Get peer list                                                                   
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_peer_list (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET PEER LIST");
    if (s_accept_reply (self, "GET PEER LIST OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get mining status                                                               
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_mining_status (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET MINING STATUS");
    if (s_accept_reply (self, "GET MINING STATUS OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Set log hash rate                                                               
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_set_log_hash_rate (wap_client_t *self, uint8_t visible)
{
    assert (self);

    zsock_send (self->actor, "s1", "SET LOG HASH RATE", visible);
    if (s_accept_reply (self, "SET LOG HASH RATE OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Set log hash rate                                                               
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_set_log_level (wap_client_t *self, uint8_t level)
{
    assert (self);

    zsock_send (self->actor, "s1", "SET LOG LEVEL", level);
    if (s_accept_reply (self, "SET LOG LEVEL OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Start save graph                                                                
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_start_save_graph (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "START SAVE GRAPH");
    if (s_accept_reply (self, "START SAVE GRAPH OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Stop save graph                                                                 
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_stop_save_graph (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "STOP SAVE GRAPH");
    if (s_accept_reply (self, "STOP SAVE GRAPH OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get block hash                                                                  
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_block_hash (wap_client_t *self, uint64_t height)
{
    assert (self);

    zsock_send (self->actor, "s8", "GET BLOCK HASH", height);
    if (s_accept_reply (self, "GET BLOCK HASH OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get block template                                                              
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_block_template (wap_client_t *self, uint64_t reserve_size, zchunk_t **address_p)
{
    assert (self);

    zsock_send (self->actor, "s8p", "GET BLOCK TEMPLATE", reserve_size, *address_p);
    *address_p = NULL;          //  Take ownership of address
    if (s_accept_reply (self, "GET BLOCK TEMPLATE OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Ask for hard fork info.                                                         
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_hard_fork_info (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET HARD FORK INFO");
    if (s_accept_reply (self, "GET HARD FORK INFO OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get connections                                                                 
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_connections_list (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "GET CONNECTIONS LIST");
    if (s_accept_reply (self, "GET CONNECTIONS LIST OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Stop daemon                                                                     
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_stop_daemon (wap_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "STOP DAEMON");
    if (s_accept_reply (self, "STOP DAEMON OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get block by height                                                             
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_block_by_height (wap_client_t *self, uint64_t height, uint8_t header_only, uint8_t as_json)
{
    assert (self);

    zsock_send (self->actor, "s811", "GET BLOCK BY HEIGHT", height, header_only, as_json);
    if (s_accept_reply (self, "GET BLOCK BY HEIGHT OK", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Get block by hash                                                               
//  Returns >= 0 if successful, -1 if interrupted.

int 
wap_client_get_block_by_hash (wap_client_t *self, zchunk_t **hash_p, uint8_t header_only, uint8_t as_json)
{
    assert (self);

    zsock_send (self->actor, "sp11", "GET BLOCK BY HASH", *hash_p, header_only, as_json);
    *hash_p = NULL;             //  Take ownership of hash
    if (s_accept_reply (self, "GET BLOCK BY HASH OK", "FAILURE", NULL))
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


//  ---------------------------------------------------------------------------
//  Return last received random_outputs

zframe_t *
wap_client_random_outputs (wap_client_t *self)
{
    assert (self);
    return self->random_outputs;
}


//  ---------------------------------------------------------------------------
//  Return last received height

uint64_t 
wap_client_height (wap_client_t *self)
{
    assert (self);
    return self->height;
}


//  ---------------------------------------------------------------------------
//  Return last received target_height

uint64_t 
wap_client_target_height (wap_client_t *self)
{
    assert (self);
    return self->target_height;
}


//  ---------------------------------------------------------------------------
//  Return last received difficulty

uint64_t 
wap_client_difficulty (wap_client_t *self)
{
    assert (self);
    return self->difficulty;
}


//  ---------------------------------------------------------------------------
//  Return last received tx_count

uint64_t 
wap_client_tx_count (wap_client_t *self)
{
    assert (self);
    return self->tx_count;
}


//  ---------------------------------------------------------------------------
//  Return last received tx_pool_size

uint64_t 
wap_client_tx_pool_size (wap_client_t *self)
{
    assert (self);
    return self->tx_pool_size;
}


//  ---------------------------------------------------------------------------
//  Return last received alt_blocks_count

uint64_t 
wap_client_alt_blocks_count (wap_client_t *self)
{
    assert (self);
    return self->alt_blocks_count;
}


//  ---------------------------------------------------------------------------
//  Return last received outgoing_connections_count

uint64_t 
wap_client_outgoing_connections_count (wap_client_t *self)
{
    assert (self);
    return self->outgoing_connections_count;
}


//  ---------------------------------------------------------------------------
//  Return last received incoming_connections_count

uint64_t 
wap_client_incoming_connections_count (wap_client_t *self)
{
    assert (self);
    return self->incoming_connections_count;
}


//  ---------------------------------------------------------------------------
//  Return last received white_peerlist_size

uint64_t 
wap_client_white_peerlist_size (wap_client_t *self)
{
    assert (self);
    return self->white_peerlist_size;
}


//  ---------------------------------------------------------------------------
//  Return last received grey_peerlist_size

uint64_t 
wap_client_grey_peerlist_size (wap_client_t *self)
{
    assert (self);
    return self->grey_peerlist_size;
}


//  ---------------------------------------------------------------------------
//  Return last received testnet

uint8_t 
wap_client_testnet (wap_client_t *self)
{
    assert (self);
    return self->testnet;
}


//  ---------------------------------------------------------------------------
//  Return last received white_list

zframe_t *
wap_client_white_list (wap_client_t *self)
{
    assert (self);
    return self->white_list;
}


//  ---------------------------------------------------------------------------
//  Return last received gray_list

zframe_t *
wap_client_gray_list (wap_client_t *self)
{
    assert (self);
    return self->gray_list;
}


//  ---------------------------------------------------------------------------
//  Return last received active

uint8_t 
wap_client_active (wap_client_t *self)
{
    assert (self);
    return self->active;
}


//  ---------------------------------------------------------------------------
//  Return last received speed

uint64_t 
wap_client_speed (wap_client_t *self)
{
    assert (self);
    return self->speed;
}


//  ---------------------------------------------------------------------------
//  Return last received thread_count

uint64_t 
wap_client_thread_count (wap_client_t *self)
{
    assert (self);
    return self->thread_count;
}


//  ---------------------------------------------------------------------------
//  Return last received address

zchunk_t *
wap_client_address (wap_client_t *self)
{
    assert (self);
    return self->address;
}


//  ---------------------------------------------------------------------------
//  Return last received hash

zchunk_t *
wap_client_hash (wap_client_t *self)
{
    assert (self);
    return self->hash;
}


//  ---------------------------------------------------------------------------
//  Return last received reserved_offset

uint64_t 
wap_client_reserved_offset (wap_client_t *self)
{
    assert (self);
    return self->reserved_offset;
}


//  ---------------------------------------------------------------------------
//  Return last received prev_hash

zchunk_t *
wap_client_prev_hash (wap_client_t *self)
{
    assert (self);
    return self->prev_hash;
}


//  ---------------------------------------------------------------------------
//  Return last received block_template_blob

zchunk_t *
wap_client_block_template_blob (wap_client_t *self)
{
    assert (self);
    return self->block_template_blob;
}


//  ---------------------------------------------------------------------------
//  Return last received hfversion

uint8_t 
wap_client_hfversion (wap_client_t *self)
{
    assert (self);
    return self->hfversion;
}


//  ---------------------------------------------------------------------------
//  Return last received enabled

uint8_t 
wap_client_enabled (wap_client_t *self)
{
    assert (self);
    return self->enabled;
}


//  ---------------------------------------------------------------------------
//  Return last received window

uint32_t 
wap_client_window (wap_client_t *self)
{
    assert (self);
    return self->window;
}


//  ---------------------------------------------------------------------------
//  Return last received votes

uint32_t 
wap_client_votes (wap_client_t *self)
{
    assert (self);
    return self->votes;
}


//  ---------------------------------------------------------------------------
//  Return last received threshold

uint32_t 
wap_client_threshold (wap_client_t *self)
{
    assert (self);
    return self->threshold;
}


//  ---------------------------------------------------------------------------
//  Return last received voting

uint8_t 
wap_client_voting (wap_client_t *self)
{
    assert (self);
    return self->voting;
}


//  ---------------------------------------------------------------------------
//  Return last received hfstate

uint32_t 
wap_client_hfstate (wap_client_t *self)
{
    assert (self);
    return self->hfstate;
}


//  ---------------------------------------------------------------------------
//  Return last received connections

zframe_t *
wap_client_connections (wap_client_t *self)
{
    assert (self);
    return self->connections;
}


//  ---------------------------------------------------------------------------
//  Return last received block

zchunk_t *
wap_client_block (wap_client_t *self)
{
    assert (self);
    return self->block;
}


//  ---------------------------------------------------------------------------
//  Return last received major_version

uint8_t 
wap_client_major_version (wap_client_t *self)
{
    assert (self);
    return self->major_version;
}


//  ---------------------------------------------------------------------------
//  Return last received minor_version

uint8_t 
wap_client_minor_version (wap_client_t *self)
{
    assert (self);
    return self->minor_version;
}


//  ---------------------------------------------------------------------------
//  Return last received timestamp

uint64_t 
wap_client_timestamp (wap_client_t *self)
{
    assert (self);
    return self->timestamp;
}


//  ---------------------------------------------------------------------------
//  Return last received nonce

uint32_t 
wap_client_nonce (wap_client_t *self)
{
    assert (self);
    return self->nonce;
}


//  ---------------------------------------------------------------------------
//  Return last received orphan

uint8_t 
wap_client_orphan (wap_client_t *self)
{
    assert (self);
    return self->orphan;
}


//  ---------------------------------------------------------------------------
//  Return last received depth

uint64_t 
wap_client_depth (wap_client_t *self)
{
    assert (self);
    return self->depth;
}


//  ---------------------------------------------------------------------------
//  Return last received reward

uint64_t 
wap_client_reward (wap_client_t *self)
{
    assert (self);
    return self->reward;
}
