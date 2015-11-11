/*  =========================================================================
    wap_proto - Wallet Access Protocol

    Codec header for wap_proto.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: wap_proto.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
                                                                
    (insert license text here)                                  
    =========================================================================
*/

#ifndef WAP_PROTO_H_INCLUDED
#define WAP_PROTO_H_INCLUDED

/*  These are the wap_proto messages:

    OPEN - Wallet opens a connection to the daemon.
        protocol            string      
        version             number 2    Protocol version 1
        identity            string      Wallet identity

    OPEN_OK - Daemon accepts wallet open request.

    BLOCKS - Wallet requests a set of blocks from the daemon. Daemon replies with
BLOCKS-OK, or ERROR if the request is invalid.
        block_ids           strings     
        start_height        number 8    

    BLOCKS_OK - Daemon returns a set of blocks to the wallet.
        status              number 4    Status
        start_height        number 8    
        curr_height         number 8    
        block_data          msg         Frames of block data

    SEND_RAW_TRANSACTION - Wallet sends a raw transaction to the daemon. Daemon replies with
SEND-RAW-TRANSACTION-OK, or ERROR.
        tx_as_hex           chunk       Transaction as hex

    SEND_RAW_TRANSACTION_OK - Daemon confirms that it accepted the raw transaction.
        status              number 4    Status

    OUTPUT_INDEXES - Ask for tx output indexes.
        tx_id               chunk       Transaction ID

    OUTPUT_INDEXES_OK - Daemon returns tx output indexes.
        status              number 4    Status
        o_indexes           frame       Output Indexes

    RANDOM_OUTS - Get random outputs for amounts.
        outs_count          number 8    Outs count
        amounts             frame       Amounts

    RANDOM_OUTS_OK - Daemon returns random outputs for amounts.
        status              number 4    Status
        random_outputs      frame       Outputs

    GET_HEIGHT - Get height.

    GET_HEIGHT_OK - Daemon returns height.
        status              number 4    Status
        height              number 8    Height

    GET_TX - Wallet requests transaction data from the daemon. Daemon replies
with GET-OK, or ERROR.
        tx_id               chunk       Transaction ID
        as_json             number 1    As JSON

    GET_TX_OK - Daemon replies with transaction data.
        status              number 4    Status
        tx_data             chunk       Transaction data
        in_pool             number 1    In pool

    SAVE_BC - save_bc command. Details tbd.

    SAVE_BC_OK - Daemon replies to a save_bc command.
        status              number 4    Status

    START - Wallet asks daemon to start mining. Daemon replies with START-OK, or
ERROR.
        address             chunk       
        thread_count        number 8    

    START_OK - Daemon replies to a start mining request.
        status              number 4    

    GET_INFO - getinfo IPC

    GET_INFO_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        height              number 8    Height
        target_height       number 8    Target Height
        difficulty          number 8    Difficulty
        tx_count            number 8    TX Count
        tx_pool_size        number 8    TX Pool Size
        alt_blocks_count    number 8    Alt Blocks Count
        outgoing_connections_count  number 8  Outgoing Connections Count
        incoming_connections_count  number 8  Incoming Connections Count
        white_peerlist_size  number 8   White Peerlist Size
        grey_peerlist_size  number 8    Grey Peerlist Size
        testnet             number 1    Testnet

    GET_PEER_LIST - get_peer_list IPC

    GET_PEER_LIST_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        white_list          frame       White list
        gray_list           frame       Gray list

    GET_MINING_STATUS - get_mining_status IPC

    GET_MINING_STATUS_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        active              number 1    Active
        speed               number 8    Speed
        thread_count        number 8    Threads count
        address             chunk       Address

    SET_LOG_HASH_RATE - set_log_hash_rate IPC
        visible             number 1    Visible

    SET_LOG_HASH_RATE_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status

    SET_LOG_LEVEL - set_log_level IPC
        level               number 1    Level

    SET_LOG_LEVEL_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status

    START_SAVE_GRAPH - start_save_graph IPC

    START_SAVE_GRAPH_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status

    STOP_SAVE_GRAPH - stop_save_graph IPC

    STOP_SAVE_GRAPH_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status

    GET_BLOCK_HASH - get_block_hash IPC
        height              number 8    Height

    GET_BLOCK_HASH_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        hash                chunk       Hash

    GET_BLOCK_TEMPLATE - get_block_template IPC
        reserve_size        number 8    Reserve size
        address             chunk       Address

    GET_BLOCK_TEMPLATE_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        reserved_offset     number 8    Rservered Offset
        height              number 8    Height
        difficulty          number 8    Difficulty
        prev_hash           chunk       Previous Hash
        block_template_blob  chunk      Block template blob

    STOP - Wallet asks daemon to start mining. Daemon replies with STOP-OK, or
ERROR.

    STOP_OK - Daemon replies to a stop mining request.

    GET_HARD_FORK_INFO - get_hard_fork_info IPC
        hfversion           number 1    Version

    GET_HARD_FORK_INFO_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        hfversion           number 1    Version
        enabled             number 1    Enabled
        window              number 4    Window
        votes               number 4    Votes
        threshold           number 4    Threshold
        voting              number 1    Voting
        hfstate             number 4    State

    GET_CONNECTIONS_LIST - get_connections_list IPC

    GET_CONNECTIONS_LIST_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        connections         frame       Connections

    STOP_DAEMON - stop_daemon IPC

    STOP_DAEMON_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status

    GET_BLOCK_BY_HEIGHT - get_block_by_height IPC
        height              number 8    Height
        header_only         number 1    Header-only
        as_json             number 1    As JSON

    GET_BLOCK_BY_HEIGHT_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        block               chunk       Block blob
        major_version       number 1    
        minor_version       number 1    
        timestamp           number 8    
        prev_hash           chunk       
        nonce               number 4    
        orphan              number 1    
        height              number 8    
        depth               number 8    
        hash                chunk       
        difficulty          number 8    
        reward              number 8    

    GET_BLOCK_BY_HASH - get_block_by_hash IPC
        hash                chunk       Hash
        header_only         number 1    Header-only
        as_json             number 1    As JSON

    GET_BLOCK_BY_HASH_OK - This is a codec for a Bitcoin Wallet Access Protocol (RFC tbd)
        status              number 4    Status
        block               chunk       Block blob
        major_version       number 1    
        minor_version       number 1    
        timestamp           number 8    
        prev_hash           chunk       
        nonce               number 4    
        height              number 8    
        depth               number 8    
        hash                chunk       
        difficulty          number 8    
        reward              number 8    

    CLOSE - Wallet closes the connection. This is polite though not mandatory.
Daemon will reply with CLOSE-OK or ERROR.

    CLOSE_OK - Daemon replies to a wallet connection close request.

    PING - Wallet heartbeats an idle connection.

    PING_OK - Daemon replies to a wallet ping request.

    ERROR - Daemon replies with failure status. Status codes tbd.
        status              number 2    Error status
        reason              string      Printable explanation
*/

#define WAP_PROTO_SUCCESS                   200
#define WAP_PROTO_NOT_DELIVERED             300
#define WAP_PROTO_CONTENT_TOO_LARGE         301
#define WAP_PROTO_TIMEOUT_EXPIRED           302
#define WAP_PROTO_CONNECTION_REFUSED        303
#define WAP_PROTO_RESOURCE_LOCKED           400
#define WAP_PROTO_ACCESS_REFUSED            401
#define WAP_PROTO_NOT_FOUND                 404
#define WAP_PROTO_COMMAND_INVALID           500
#define WAP_PROTO_NOT_IMPLEMENTED           501
#define WAP_PROTO_INTERNAL_ERROR            502

#define WAP_PROTO_OPEN                      1
#define WAP_PROTO_OPEN_OK                   2
#define WAP_PROTO_BLOCKS                    3
#define WAP_PROTO_BLOCKS_OK                 4
#define WAP_PROTO_SEND_RAW_TRANSACTION      5
#define WAP_PROTO_SEND_RAW_TRANSACTION_OK   6
#define WAP_PROTO_OUTPUT_INDEXES            7
#define WAP_PROTO_OUTPUT_INDEXES_OK         8
#define WAP_PROTO_RANDOM_OUTS               9
#define WAP_PROTO_RANDOM_OUTS_OK            10
#define WAP_PROTO_GET_HEIGHT                11
#define WAP_PROTO_GET_HEIGHT_OK             12
#define WAP_PROTO_GET_TX                    13
#define WAP_PROTO_GET_TX_OK                 14
#define WAP_PROTO_SAVE_BC                   15
#define WAP_PROTO_SAVE_BC_OK                16
#define WAP_PROTO_START                     17
#define WAP_PROTO_START_OK                  18
#define WAP_PROTO_GET_INFO                  19
#define WAP_PROTO_GET_INFO_OK               20
#define WAP_PROTO_GET_PEER_LIST             21
#define WAP_PROTO_GET_PEER_LIST_OK          22
#define WAP_PROTO_GET_MINING_STATUS         23
#define WAP_PROTO_GET_MINING_STATUS_OK      24
#define WAP_PROTO_SET_LOG_HASH_RATE         25
#define WAP_PROTO_SET_LOG_HASH_RATE_OK      26
#define WAP_PROTO_SET_LOG_LEVEL             27
#define WAP_PROTO_SET_LOG_LEVEL_OK          28
#define WAP_PROTO_START_SAVE_GRAPH          29
#define WAP_PROTO_START_SAVE_GRAPH_OK       30
#define WAP_PROTO_STOP_SAVE_GRAPH           31
#define WAP_PROTO_STOP_SAVE_GRAPH_OK        32
#define WAP_PROTO_GET_BLOCK_HASH            33
#define WAP_PROTO_GET_BLOCK_HASH_OK         34
#define WAP_PROTO_GET_BLOCK_TEMPLATE        35
#define WAP_PROTO_GET_BLOCK_TEMPLATE_OK     36
#define WAP_PROTO_STOP                      37
#define WAP_PROTO_STOP_OK                   38
#define WAP_PROTO_GET_HARD_FORK_INFO        39
#define WAP_PROTO_GET_HARD_FORK_INFO_OK     40
#define WAP_PROTO_GET_CONNECTIONS_LIST      41
#define WAP_PROTO_GET_CONNECTIONS_LIST_OK   42
#define WAP_PROTO_STOP_DAEMON               43
#define WAP_PROTO_STOP_DAEMON_OK            44
#define WAP_PROTO_GET_BLOCK_BY_HEIGHT       45
#define WAP_PROTO_GET_BLOCK_BY_HEIGHT_OK    46
#define WAP_PROTO_GET_BLOCK_BY_HASH         47
#define WAP_PROTO_GET_BLOCK_BY_HASH_OK      48
#define WAP_PROTO_CLOSE                     49
#define WAP_PROTO_CLOSE_OK                  50
#define WAP_PROTO_PING                      51
#define WAP_PROTO_PING_OK                   52
#define WAP_PROTO_ERROR                     53

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef WAP_PROTO_T_DEFINED
typedef struct _wap_proto_t wap_proto_t;
#define WAP_PROTO_T_DEFINED
#endif

//  @interface
//  Create a new empty wap_proto
wap_proto_t *
    wap_proto_new (void);

//  Destroy a wap_proto instance
void
    wap_proto_destroy (wap_proto_t **self_p);

//  Receive a wap_proto from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.
int
    wap_proto_recv (wap_proto_t *self, zsock_t *input);

//  Send the wap_proto to the output socket, does not destroy it
int
    wap_proto_send (wap_proto_t *self, zsock_t *output);

//  Print contents of message to stdout
void
    wap_proto_print (wap_proto_t *self);

//  Get/set the message routing id
zframe_t *
    wap_proto_routing_id (wap_proto_t *self);
void
    wap_proto_set_routing_id (wap_proto_t *self, zframe_t *routing_id);

//  Get the wap_proto id and printable command
int
    wap_proto_id (wap_proto_t *self);
void
    wap_proto_set_id (wap_proto_t *self, int id);
const char *
    wap_proto_command (wap_proto_t *self);

//  Get/set the identity field
const char *
    wap_proto_identity (wap_proto_t *self);
void
    wap_proto_set_identity (wap_proto_t *self, const char *value);

//  Get/set the block_ids field
zlist_t *
    wap_proto_block_ids (wap_proto_t *self);
//  Get the block_ids field and transfer ownership to caller
zlist_t *
    wap_proto_get_block_ids (wap_proto_t *self);
//  Set the block_ids field, transferring ownership from caller
void
    wap_proto_set_block_ids (wap_proto_t *self, zlist_t **block_ids_p);

//  Get/set the start_height field
uint64_t
    wap_proto_start_height (wap_proto_t *self);
void
    wap_proto_set_start_height (wap_proto_t *self, uint64_t start_height);

//  Get/set the status field
uint32_t
    wap_proto_status (wap_proto_t *self);
void
    wap_proto_set_status (wap_proto_t *self, uint32_t status);

//  Get/set the curr_height field
uint64_t
    wap_proto_curr_height (wap_proto_t *self);
void
    wap_proto_set_curr_height (wap_proto_t *self, uint64_t curr_height);

//  Get a copy of the block_data field
zmsg_t *
    wap_proto_block_data (wap_proto_t *self);
//  Get the block_data field and transfer ownership to caller
zmsg_t *
    wap_proto_get_block_data (wap_proto_t *self);
//  Set the block_data field, transferring ownership from caller
void
    wap_proto_set_block_data (wap_proto_t *self, zmsg_t **msg_p);

//  Get a copy of the tx_as_hex field
zchunk_t *
    wap_proto_tx_as_hex (wap_proto_t *self);
//  Get the tx_as_hex field and transfer ownership to caller
zchunk_t *
    wap_proto_get_tx_as_hex (wap_proto_t *self);
//  Set the tx_as_hex field, transferring ownership from caller
void
    wap_proto_set_tx_as_hex (wap_proto_t *self, zchunk_t **chunk_p);

//  Get a copy of the tx_id field
zchunk_t *
    wap_proto_tx_id (wap_proto_t *self);
//  Get the tx_id field and transfer ownership to caller
zchunk_t *
    wap_proto_get_tx_id (wap_proto_t *self);
//  Set the tx_id field, transferring ownership from caller
void
    wap_proto_set_tx_id (wap_proto_t *self, zchunk_t **chunk_p);

//  Get a copy of the o_indexes field
zframe_t *
    wap_proto_o_indexes (wap_proto_t *self);
//  Get the o_indexes field and transfer ownership to caller
zframe_t *
    wap_proto_get_o_indexes (wap_proto_t *self);
//  Set the o_indexes field, transferring ownership from caller
void
    wap_proto_set_o_indexes (wap_proto_t *self, zframe_t **frame_p);

//  Get/set the outs_count field
uint64_t
    wap_proto_outs_count (wap_proto_t *self);
void
    wap_proto_set_outs_count (wap_proto_t *self, uint64_t outs_count);

//  Get a copy of the amounts field
zframe_t *
    wap_proto_amounts (wap_proto_t *self);
//  Get the amounts field and transfer ownership to caller
zframe_t *
    wap_proto_get_amounts (wap_proto_t *self);
//  Set the amounts field, transferring ownership from caller
void
    wap_proto_set_amounts (wap_proto_t *self, zframe_t **frame_p);

//  Get a copy of the random_outputs field
zframe_t *
    wap_proto_random_outputs (wap_proto_t *self);
//  Get the random_outputs field and transfer ownership to caller
zframe_t *
    wap_proto_get_random_outputs (wap_proto_t *self);
//  Set the random_outputs field, transferring ownership from caller
void
    wap_proto_set_random_outputs (wap_proto_t *self, zframe_t **frame_p);

//  Get/set the height field
uint64_t
    wap_proto_height (wap_proto_t *self);
void
    wap_proto_set_height (wap_proto_t *self, uint64_t height);

//  Get/set the as_json field
byte
    wap_proto_as_json (wap_proto_t *self);
void
    wap_proto_set_as_json (wap_proto_t *self, byte as_json);

//  Get a copy of the tx_data field
zchunk_t *
    wap_proto_tx_data (wap_proto_t *self);
//  Get the tx_data field and transfer ownership to caller
zchunk_t *
    wap_proto_get_tx_data (wap_proto_t *self);
//  Set the tx_data field, transferring ownership from caller
void
    wap_proto_set_tx_data (wap_proto_t *self, zchunk_t **chunk_p);

//  Get/set the in_pool field
byte
    wap_proto_in_pool (wap_proto_t *self);
void
    wap_proto_set_in_pool (wap_proto_t *self, byte in_pool);

//  Get a copy of the address field
zchunk_t *
    wap_proto_address (wap_proto_t *self);
//  Get the address field and transfer ownership to caller
zchunk_t *
    wap_proto_get_address (wap_proto_t *self);
//  Set the address field, transferring ownership from caller
void
    wap_proto_set_address (wap_proto_t *self, zchunk_t **chunk_p);

//  Get/set the thread_count field
uint64_t
    wap_proto_thread_count (wap_proto_t *self);
void
    wap_proto_set_thread_count (wap_proto_t *self, uint64_t thread_count);

//  Get/set the target_height field
uint64_t
    wap_proto_target_height (wap_proto_t *self);
void
    wap_proto_set_target_height (wap_proto_t *self, uint64_t target_height);

//  Get/set the difficulty field
uint64_t
    wap_proto_difficulty (wap_proto_t *self);
void
    wap_proto_set_difficulty (wap_proto_t *self, uint64_t difficulty);

//  Get/set the tx_count field
uint64_t
    wap_proto_tx_count (wap_proto_t *self);
void
    wap_proto_set_tx_count (wap_proto_t *self, uint64_t tx_count);

//  Get/set the tx_pool_size field
uint64_t
    wap_proto_tx_pool_size (wap_proto_t *self);
void
    wap_proto_set_tx_pool_size (wap_proto_t *self, uint64_t tx_pool_size);

//  Get/set the alt_blocks_count field
uint64_t
    wap_proto_alt_blocks_count (wap_proto_t *self);
void
    wap_proto_set_alt_blocks_count (wap_proto_t *self, uint64_t alt_blocks_count);

//  Get/set the outgoing_connections_count field
uint64_t
    wap_proto_outgoing_connections_count (wap_proto_t *self);
void
    wap_proto_set_outgoing_connections_count (wap_proto_t *self, uint64_t outgoing_connections_count);

//  Get/set the incoming_connections_count field
uint64_t
    wap_proto_incoming_connections_count (wap_proto_t *self);
void
    wap_proto_set_incoming_connections_count (wap_proto_t *self, uint64_t incoming_connections_count);

//  Get/set the white_peerlist_size field
uint64_t
    wap_proto_white_peerlist_size (wap_proto_t *self);
void
    wap_proto_set_white_peerlist_size (wap_proto_t *self, uint64_t white_peerlist_size);

//  Get/set the grey_peerlist_size field
uint64_t
    wap_proto_grey_peerlist_size (wap_proto_t *self);
void
    wap_proto_set_grey_peerlist_size (wap_proto_t *self, uint64_t grey_peerlist_size);

//  Get/set the testnet field
byte
    wap_proto_testnet (wap_proto_t *self);
void
    wap_proto_set_testnet (wap_proto_t *self, byte testnet);

//  Get a copy of the white_list field
zframe_t *
    wap_proto_white_list (wap_proto_t *self);
//  Get the white_list field and transfer ownership to caller
zframe_t *
    wap_proto_get_white_list (wap_proto_t *self);
//  Set the white_list field, transferring ownership from caller
void
    wap_proto_set_white_list (wap_proto_t *self, zframe_t **frame_p);

//  Get a copy of the gray_list field
zframe_t *
    wap_proto_gray_list (wap_proto_t *self);
//  Get the gray_list field and transfer ownership to caller
zframe_t *
    wap_proto_get_gray_list (wap_proto_t *self);
//  Set the gray_list field, transferring ownership from caller
void
    wap_proto_set_gray_list (wap_proto_t *self, zframe_t **frame_p);

//  Get/set the active field
byte
    wap_proto_active (wap_proto_t *self);
void
    wap_proto_set_active (wap_proto_t *self, byte active);

//  Get/set the speed field
uint64_t
    wap_proto_speed (wap_proto_t *self);
void
    wap_proto_set_speed (wap_proto_t *self, uint64_t speed);

//  Get/set the visible field
byte
    wap_proto_visible (wap_proto_t *self);
void
    wap_proto_set_visible (wap_proto_t *self, byte visible);

//  Get/set the level field
byte
    wap_proto_level (wap_proto_t *self);
void
    wap_proto_set_level (wap_proto_t *self, byte level);

//  Get a copy of the hash field
zchunk_t *
    wap_proto_hash (wap_proto_t *self);
//  Get the hash field and transfer ownership to caller
zchunk_t *
    wap_proto_get_hash (wap_proto_t *self);
//  Set the hash field, transferring ownership from caller
void
    wap_proto_set_hash (wap_proto_t *self, zchunk_t **chunk_p);

//  Get/set the reserve_size field
uint64_t
    wap_proto_reserve_size (wap_proto_t *self);
void
    wap_proto_set_reserve_size (wap_proto_t *self, uint64_t reserve_size);

//  Get/set the reserved_offset field
uint64_t
    wap_proto_reserved_offset (wap_proto_t *self);
void
    wap_proto_set_reserved_offset (wap_proto_t *self, uint64_t reserved_offset);

//  Get a copy of the prev_hash field
zchunk_t *
    wap_proto_prev_hash (wap_proto_t *self);
//  Get the prev_hash field and transfer ownership to caller
zchunk_t *
    wap_proto_get_prev_hash (wap_proto_t *self);
//  Set the prev_hash field, transferring ownership from caller
void
    wap_proto_set_prev_hash (wap_proto_t *self, zchunk_t **chunk_p);

//  Get a copy of the block_template_blob field
zchunk_t *
    wap_proto_block_template_blob (wap_proto_t *self);
//  Get the block_template_blob field and transfer ownership to caller
zchunk_t *
    wap_proto_get_block_template_blob (wap_proto_t *self);
//  Set the block_template_blob field, transferring ownership from caller
void
    wap_proto_set_block_template_blob (wap_proto_t *self, zchunk_t **chunk_p);

//  Get/set the hfversion field
byte
    wap_proto_hfversion (wap_proto_t *self);
void
    wap_proto_set_hfversion (wap_proto_t *self, byte hfversion);

//  Get/set the enabled field
byte
    wap_proto_enabled (wap_proto_t *self);
void
    wap_proto_set_enabled (wap_proto_t *self, byte enabled);

//  Get/set the window field
uint32_t
    wap_proto_window (wap_proto_t *self);
void
    wap_proto_set_window (wap_proto_t *self, uint32_t window);

//  Get/set the votes field
uint32_t
    wap_proto_votes (wap_proto_t *self);
void
    wap_proto_set_votes (wap_proto_t *self, uint32_t votes);

//  Get/set the threshold field
uint32_t
    wap_proto_threshold (wap_proto_t *self);
void
    wap_proto_set_threshold (wap_proto_t *self, uint32_t threshold);

//  Get/set the voting field
byte
    wap_proto_voting (wap_proto_t *self);
void
    wap_proto_set_voting (wap_proto_t *self, byte voting);

//  Get/set the hfstate field
uint32_t
    wap_proto_hfstate (wap_proto_t *self);
void
    wap_proto_set_hfstate (wap_proto_t *self, uint32_t hfstate);

//  Get a copy of the connections field
zframe_t *
    wap_proto_connections (wap_proto_t *self);
//  Get the connections field and transfer ownership to caller
zframe_t *
    wap_proto_get_connections (wap_proto_t *self);
//  Set the connections field, transferring ownership from caller
void
    wap_proto_set_connections (wap_proto_t *self, zframe_t **frame_p);

//  Get/set the header_only field
byte
    wap_proto_header_only (wap_proto_t *self);
void
    wap_proto_set_header_only (wap_proto_t *self, byte header_only);

//  Get a copy of the block field
zchunk_t *
    wap_proto_block (wap_proto_t *self);
//  Get the block field and transfer ownership to caller
zchunk_t *
    wap_proto_get_block (wap_proto_t *self);
//  Set the block field, transferring ownership from caller
void
    wap_proto_set_block (wap_proto_t *self, zchunk_t **chunk_p);

//  Get/set the major_version field
byte
    wap_proto_major_version (wap_proto_t *self);
void
    wap_proto_set_major_version (wap_proto_t *self, byte major_version);

//  Get/set the minor_version field
byte
    wap_proto_minor_version (wap_proto_t *self);
void
    wap_proto_set_minor_version (wap_proto_t *self, byte minor_version);

//  Get/set the timestamp field
uint64_t
    wap_proto_timestamp (wap_proto_t *self);
void
    wap_proto_set_timestamp (wap_proto_t *self, uint64_t timestamp);

//  Get/set the nonce field
uint32_t
    wap_proto_nonce (wap_proto_t *self);
void
    wap_proto_set_nonce (wap_proto_t *self, uint32_t nonce);

//  Get/set the orphan field
byte
    wap_proto_orphan (wap_proto_t *self);
void
    wap_proto_set_orphan (wap_proto_t *self, byte orphan);

//  Get/set the depth field
uint64_t
    wap_proto_depth (wap_proto_t *self);
void
    wap_proto_set_depth (wap_proto_t *self, uint64_t depth);

//  Get/set the reward field
uint64_t
    wap_proto_reward (wap_proto_t *self);
void
    wap_proto_set_reward (wap_proto_t *self, uint64_t reward);

//  Get/set the reason field
const char *
    wap_proto_reason (wap_proto_t *self);
void
    wap_proto_set_reason (wap_proto_t *self, const char *value);

//  Self test of this class
void
    wap_proto_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define wap_proto_dump      wap_proto_print

#ifdef __cplusplus
}
#endif

#endif
