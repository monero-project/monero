/*  =========================================================================
    wap_proto - Wallet Access Protocol

    Codec class for wap_proto.

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

/*
@header
    wap_proto - Wallet Access Protocol
@discuss
@end
*/

#include "../include/wap_proto.h"

//  Structure of our class

struct _wap_proto_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  wap_proto message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    // Wallet identity
    char identity [256];
    // block_ids
    zlist_t *block_ids;
    // start_height
    uint64_t start_height;
    // status
    uint64_t status;
    // curr_height
    uint64_t curr_height;
    // Frames of block data
    zmsg_t *block_data;
    // Transaction as hex
    zchunk_t *tx_as_hex;
    // Transaction ID
    zchunk_t *tx_id;
    // Output Indexes
    zframe_t *o_indexes;
    // Outs count
    uint64_t outs_count;
    // Amounts
    zframe_t *amounts;
    // Outputs
    zframe_t *random_outputs;
    // Transaction data
    zchunk_t *tx_data;
    // address
    char address [256];
    // thread_count
    uint64_t thread_count;
    // Printable explanation
    char reason [256];
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("wap_proto: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("wap_proto: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("wap_proto: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("wap_proto: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("wap_proto: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("wap_proto: GET_STRING failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("wap_proto: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new wap_proto

wap_proto_t *
wap_proto_new (void)
{
    wap_proto_t *self = (wap_proto_t *) zmalloc (sizeof (wap_proto_t));
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the wap_proto

void
wap_proto_destroy (wap_proto_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        wap_proto_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        if (self->block_ids)
            zlist_destroy (&self->block_ids);
        zmsg_destroy (&self->block_data);
        zchunk_destroy (&self->tx_as_hex);
        zchunk_destroy (&self->tx_id);
        zframe_destroy (&self->o_indexes);
        zframe_destroy (&self->amounts);
        zframe_destroy (&self->random_outputs);
        zchunk_destroy (&self->tx_data);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Receive a wap_proto from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.

int
wap_proto_recv (wap_proto_t *self, zsock_t *input)
{
    assert (input);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("wap_proto: no routing ID");
            return -1;          //  Interrupted or malformed
        }
    }
    zmq_msg_t frame;
    zmq_msg_init (&frame);
    int size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("wap_proto: interrupted");
        goto malformed;         //  Interrupted
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0)) {
        zsys_warning ("wap_proto: invalid signature");
        //  TODO: discard invalid messages and loop, and return
        //  -1 only on interrupt
        goto malformed;         //  Interrupted
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case WAP_PROTO_OPEN:
            {
                char protocol [256];
                GET_STRING (protocol);
                if (strneq (protocol, "WAP")) {
                    zsys_warning ("wap_proto: protocol is invalid");
                    goto malformed;
                }
            }
            {
                uint16_t version;
                GET_NUMBER2 (version);
                if (version != 1) {
                    zsys_warning ("wap_proto: version is invalid");
                    goto malformed;
                }
            }
            GET_STRING (self->identity);
            break;

        case WAP_PROTO_OPEN_OK:
            break;

        case WAP_PROTO_BLOCKS:
            {
                size_t list_size;
                GET_NUMBER4 (list_size);
                self->block_ids = zlist_new ();
                zlist_autofree (self->block_ids);
                while (list_size--) {
                    char *string = NULL;
                    GET_LONGSTR (string);
                    zlist_append (self->block_ids, string);
                    free (string);
                }
            }
            GET_NUMBER8 (self->start_height);
            break;

        case WAP_PROTO_BLOCKS_OK:
            GET_NUMBER8 (self->status);
            GET_NUMBER8 (self->start_height);
            GET_NUMBER8 (self->curr_height);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->block_data);
            if (zsock_rcvmore (input))
                self->block_data = zmsg_recv (input);
            else
                self->block_data = zmsg_new ();
            break;

        case WAP_PROTO_PUT:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("wap_proto: tx_as_hex is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->tx_as_hex);
                self->tx_as_hex = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case WAP_PROTO_PUT_OK:
            GET_NUMBER8 (self->status);
            break;

        case WAP_PROTO_OUTPUT_INDEXES:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("wap_proto: tx_id is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->tx_id);
                self->tx_id = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case WAP_PROTO_OUTPUT_INDEXES_OK:
            GET_NUMBER8 (self->status);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("wap_proto: o_indexes is missing");
                goto malformed;
            }
            zframe_destroy (&self->o_indexes);
            self->o_indexes = zframe_recv (input);
            break;

        case WAP_PROTO_RANDOM_OUTS:
            GET_NUMBER8 (self->outs_count);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("wap_proto: amounts is missing");
                goto malformed;
            }
            zframe_destroy (&self->amounts);
            self->amounts = zframe_recv (input);
            break;

        case WAP_PROTO_RANDOM_OUTS_OK:
            GET_NUMBER8 (self->status);
            //  Get next frame off socket
            if (!zsock_rcvmore (input)) {
                zsys_warning ("wap_proto: random_outputs is missing");
                goto malformed;
            }
            zframe_destroy (&self->random_outputs);
            self->random_outputs = zframe_recv (input);
            break;

        case WAP_PROTO_GET:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("wap_proto: tx_id is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->tx_id);
                self->tx_id = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case WAP_PROTO_GET_OK:
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling)) {
                    zsys_warning ("wap_proto: tx_data is missing data");
                    goto malformed;
                }
                zchunk_destroy (&self->tx_data);
                self->tx_data = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case WAP_PROTO_SAVE:
            break;

        case WAP_PROTO_SAVE_OK:
            break;

        case WAP_PROTO_START:
            GET_STRING (self->address);
            GET_NUMBER8 (self->thread_count);
            break;

        case WAP_PROTO_START_OK:
            GET_NUMBER8 (self->status);
            break;

        case WAP_PROTO_STOP:
            break;

        case WAP_PROTO_STOP_OK:
            break;

        case WAP_PROTO_CLOSE:
            break;

        case WAP_PROTO_CLOSE_OK:
            break;

        case WAP_PROTO_PING:
            break;

        case WAP_PROTO_PING_OK:
            break;

        case WAP_PROTO_ERROR:
            GET_NUMBER2 (self->status);
            GET_STRING (self->reason);
            break;

        default:
            zsys_warning ("wap_proto: bad message ID");
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return 0;

    //  Error returns
    malformed:
        zsys_warning ("wap_proto: wap_proto malformed message, fail");
        zmq_msg_close (&frame);
        return -1;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the wap_proto to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
wap_proto_send (wap_proto_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case WAP_PROTO_OPEN:
            frame_size += 1 + strlen ("WAP");
            frame_size += 2;            //  version
            frame_size += 1 + strlen (self->identity);
            break;
        case WAP_PROTO_BLOCKS:
            frame_size += 4;            //  Size is 4 octets
            if (self->block_ids) {
                char *block_ids = (char *) zlist_first (self->block_ids);
                while (block_ids) {
                    frame_size += 4 + strlen (block_ids);
                    block_ids = (char *) zlist_next (self->block_ids);
                }
            }
            frame_size += 8;            //  start_height
            break;
        case WAP_PROTO_BLOCKS_OK:
            frame_size += 8;            //  status
            frame_size += 8;            //  start_height
            frame_size += 8;            //  curr_height
            break;
        case WAP_PROTO_PUT:
            frame_size += 4;            //  Size is 4 octets
            if (self->tx_as_hex)
                frame_size += zchunk_size (self->tx_as_hex);
            break;
        case WAP_PROTO_PUT_OK:
            frame_size += 8;            //  status
            break;
        case WAP_PROTO_OUTPUT_INDEXES:
            frame_size += 4;            //  Size is 4 octets
            if (self->tx_id)
                frame_size += zchunk_size (self->tx_id);
            break;
        case WAP_PROTO_OUTPUT_INDEXES_OK:
            frame_size += 8;            //  status
            break;
        case WAP_PROTO_RANDOM_OUTS:
            frame_size += 8;            //  outs_count
            break;
        case WAP_PROTO_RANDOM_OUTS_OK:
            frame_size += 8;            //  status
            break;
        case WAP_PROTO_GET:
            frame_size += 4;            //  Size is 4 octets
            if (self->tx_id)
                frame_size += zchunk_size (self->tx_id);
            break;
        case WAP_PROTO_GET_OK:
            frame_size += 4;            //  Size is 4 octets
            if (self->tx_data)
                frame_size += zchunk_size (self->tx_data);
            break;
        case WAP_PROTO_START:
            frame_size += 1 + strlen (self->address);
            frame_size += 8;            //  thread_count
            break;
        case WAP_PROTO_START_OK:
            frame_size += 8;            //  status
            break;
        case WAP_PROTO_ERROR:
            frame_size += 2;            //  status
            frame_size += 1 + strlen (self->reason);
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);
    bool send_block_data = false;
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case WAP_PROTO_OPEN:
            PUT_STRING ("WAP");
            PUT_NUMBER2 (1);
            PUT_STRING (self->identity);
            break;

        case WAP_PROTO_BLOCKS:
            if (self->block_ids) {
                PUT_NUMBER4 (zlist_size (self->block_ids));
                char *block_ids = (char *) zlist_first (self->block_ids);
                while (block_ids) {
                    PUT_LONGSTR (block_ids);
                    block_ids = (char *) zlist_next (self->block_ids);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty string array
            PUT_NUMBER8 (self->start_height);
            break;

        case WAP_PROTO_BLOCKS_OK:
            PUT_NUMBER8 (self->status);
            PUT_NUMBER8 (self->start_height);
            PUT_NUMBER8 (self->curr_height);
            nbr_frames += self->block_data? zmsg_size (self->block_data): 1;
            send_block_data = true;
            break;

        case WAP_PROTO_PUT:
            if (self->tx_as_hex) {
                PUT_NUMBER4 (zchunk_size (self->tx_as_hex));
                memcpy (self->needle,
                        zchunk_data (self->tx_as_hex),
                        zchunk_size (self->tx_as_hex));
                self->needle += zchunk_size (self->tx_as_hex);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case WAP_PROTO_PUT_OK:
            PUT_NUMBER8 (self->status);
            break;

        case WAP_PROTO_OUTPUT_INDEXES:
            if (self->tx_id) {
                PUT_NUMBER4 (zchunk_size (self->tx_id));
                memcpy (self->needle,
                        zchunk_data (self->tx_id),
                        zchunk_size (self->tx_id));
                self->needle += zchunk_size (self->tx_id);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case WAP_PROTO_OUTPUT_INDEXES_OK:
            PUT_NUMBER8 (self->status);
            nbr_frames++;
            break;

        case WAP_PROTO_RANDOM_OUTS:
            PUT_NUMBER8 (self->outs_count);
            nbr_frames++;
            break;

        case WAP_PROTO_RANDOM_OUTS_OK:
            PUT_NUMBER8 (self->status);
            nbr_frames++;
            break;

        case WAP_PROTO_GET:
            if (self->tx_id) {
                PUT_NUMBER4 (zchunk_size (self->tx_id));
                memcpy (self->needle,
                        zchunk_data (self->tx_id),
                        zchunk_size (self->tx_id));
                self->needle += zchunk_size (self->tx_id);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case WAP_PROTO_GET_OK:
            if (self->tx_data) {
                PUT_NUMBER4 (zchunk_size (self->tx_data));
                memcpy (self->needle,
                        zchunk_data (self->tx_data),
                        zchunk_size (self->tx_data));
                self->needle += zchunk_size (self->tx_data);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case WAP_PROTO_START:
            PUT_STRING (self->address);
            PUT_NUMBER8 (self->thread_count);
            break;

        case WAP_PROTO_START_OK:
            PUT_NUMBER8 (self->status);
            break;

        case WAP_PROTO_ERROR:
            PUT_NUMBER2 (self->status);
            PUT_STRING (self->reason);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    //  Now send any frame fields, in order
    if (self->id == WAP_PROTO_OUTPUT_INDEXES_OK) {
        //  If o_indexes isn't set, send an empty frame
        if (self->o_indexes)
            zframe_send (&self->o_indexes, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send any frame fields, in order
    if (self->id == WAP_PROTO_RANDOM_OUTS) {
        //  If amounts isn't set, send an empty frame
        if (self->amounts)
            zframe_send (&self->amounts, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send any frame fields, in order
    if (self->id == WAP_PROTO_RANDOM_OUTS_OK) {
        //  If random_outputs isn't set, send an empty frame
        if (self->random_outputs)
            zframe_send (&self->random_outputs, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
        else
            zmq_send (zsock_resolve (output), NULL, 0, (--nbr_frames? ZMQ_SNDMORE: 0));
    }
    //  Now send the block_data if necessary
    if (send_block_data) {
        if (self->block_data) {
            zframe_t *frame = zmsg_first (self->block_data);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->block_data);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
wap_proto_print (wap_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case WAP_PROTO_OPEN:
            zsys_debug ("WAP_PROTO_OPEN:");
            zsys_debug ("    protocol=wap");
            zsys_debug ("    version=1");
            if (self->identity)
                zsys_debug ("    identity='%s'", self->identity);
            else
                zsys_debug ("    identity=");
            break;

        case WAP_PROTO_OPEN_OK:
            zsys_debug ("WAP_PROTO_OPEN_OK:");
            break;

        case WAP_PROTO_BLOCKS:
            zsys_debug ("WAP_PROTO_BLOCKS:");
            zsys_debug ("    block_ids=");
            if (self->block_ids) {
                char *block_ids = (char *) zlist_first (self->block_ids);
                while (block_ids) {
                    zsys_debug ("        '%s'", block_ids);
                    block_ids = (char *) zlist_next (self->block_ids);
                }
            }
            zsys_debug ("    start_height=%ld", (long) self->start_height);
            break;

        case WAP_PROTO_BLOCKS_OK:
            zsys_debug ("WAP_PROTO_BLOCKS_OK:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    start_height=%ld", (long) self->start_height);
            zsys_debug ("    curr_height=%ld", (long) self->curr_height);
            zsys_debug ("    block_data=");
            if (self->block_data)
                zmsg_print (self->block_data);
            else
                zsys_debug ("(NULL)");
            break;

        case WAP_PROTO_PUT:
            zsys_debug ("WAP_PROTO_PUT:");
            zsys_debug ("    tx_as_hex=[ ... ]");
            break;

        case WAP_PROTO_PUT_OK:
            zsys_debug ("WAP_PROTO_PUT_OK:");
            zsys_debug ("    status=%ld", (long) self->status);
            break;

        case WAP_PROTO_OUTPUT_INDEXES:
            zsys_debug ("WAP_PROTO_OUTPUT_INDEXES:");
            zsys_debug ("    tx_id=[ ... ]");
            break;

        case WAP_PROTO_OUTPUT_INDEXES_OK:
            zsys_debug ("WAP_PROTO_OUTPUT_INDEXES_OK:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    o_indexes=");
            if (self->o_indexes)
                zframe_print (self->o_indexes, NULL);
            else
                zsys_debug ("(NULL)");
            break;

        case WAP_PROTO_RANDOM_OUTS:
            zsys_debug ("WAP_PROTO_RANDOM_OUTS:");
            zsys_debug ("    outs_count=%ld", (long) self->outs_count);
            zsys_debug ("    amounts=");
            if (self->amounts)
                zframe_print (self->amounts, NULL);
            else
                zsys_debug ("(NULL)");
            break;

        case WAP_PROTO_RANDOM_OUTS_OK:
            zsys_debug ("WAP_PROTO_RANDOM_OUTS_OK:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    random_outputs=");
            if (self->random_outputs)
                zframe_print (self->random_outputs, NULL);
            else
                zsys_debug ("(NULL)");
            break;

        case WAP_PROTO_GET:
            zsys_debug ("WAP_PROTO_GET:");
            zsys_debug ("    tx_id=[ ... ]");
            break;

        case WAP_PROTO_GET_OK:
            zsys_debug ("WAP_PROTO_GET_OK:");
            zsys_debug ("    tx_data=[ ... ]");
            break;

        case WAP_PROTO_SAVE:
            zsys_debug ("WAP_PROTO_SAVE:");
            break;

        case WAP_PROTO_SAVE_OK:
            zsys_debug ("WAP_PROTO_SAVE_OK:");
            break;

        case WAP_PROTO_START:
            zsys_debug ("WAP_PROTO_START:");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            zsys_debug ("    thread_count=%ld", (long) self->thread_count);
            break;

        case WAP_PROTO_START_OK:
            zsys_debug ("WAP_PROTO_START_OK:");
            zsys_debug ("    status=%ld", (long) self->status);
            break;

        case WAP_PROTO_STOP:
            zsys_debug ("WAP_PROTO_STOP:");
            break;

        case WAP_PROTO_STOP_OK:
            zsys_debug ("WAP_PROTO_STOP_OK:");
            break;

        case WAP_PROTO_CLOSE:
            zsys_debug ("WAP_PROTO_CLOSE:");
            break;

        case WAP_PROTO_CLOSE_OK:
            zsys_debug ("WAP_PROTO_CLOSE_OK:");
            break;

        case WAP_PROTO_PING:
            zsys_debug ("WAP_PROTO_PING:");
            break;

        case WAP_PROTO_PING_OK:
            zsys_debug ("WAP_PROTO_PING_OK:");
            break;

        case WAP_PROTO_ERROR:
            zsys_debug ("WAP_PROTO_ERROR:");
            zsys_debug ("    status=%ld", (long) self->status);
            if (self->reason)
                zsys_debug ("    reason='%s'", self->reason);
            else
                zsys_debug ("    reason=");
            break;

    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
wap_proto_routing_id (wap_proto_t *self)
{
    assert (self);
    return self->routing_id;
}

void
wap_proto_set_routing_id (wap_proto_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the wap_proto id

int
wap_proto_id (wap_proto_t *self)
{
    assert (self);
    return self->id;
}

void
wap_proto_set_id (wap_proto_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
wap_proto_command (wap_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case WAP_PROTO_OPEN:
            return ("OPEN");
            break;
        case WAP_PROTO_OPEN_OK:
            return ("OPEN_OK");
            break;
        case WAP_PROTO_BLOCKS:
            return ("BLOCKS");
            break;
        case WAP_PROTO_BLOCKS_OK:
            return ("BLOCKS_OK");
            break;
        case WAP_PROTO_PUT:
            return ("PUT");
            break;
        case WAP_PROTO_PUT_OK:
            return ("PUT_OK");
            break;
        case WAP_PROTO_OUTPUT_INDEXES:
            return ("OUTPUT_INDEXES");
            break;
        case WAP_PROTO_OUTPUT_INDEXES_OK:
            return ("OUTPUT_INDEXES_OK");
            break;
        case WAP_PROTO_RANDOM_OUTS:
            return ("RANDOM_OUTS");
            break;
        case WAP_PROTO_RANDOM_OUTS_OK:
            return ("RANDOM_OUTS_OK");
            break;
        case WAP_PROTO_GET:
            return ("GET");
            break;
        case WAP_PROTO_GET_OK:
            return ("GET_OK");
            break;
        case WAP_PROTO_SAVE:
            return ("SAVE");
            break;
        case WAP_PROTO_SAVE_OK:
            return ("SAVE_OK");
            break;
        case WAP_PROTO_START:
            return ("START");
            break;
        case WAP_PROTO_START_OK:
            return ("START_OK");
            break;
        case WAP_PROTO_STOP:
            return ("STOP");
            break;
        case WAP_PROTO_STOP_OK:
            return ("STOP_OK");
            break;
        case WAP_PROTO_CLOSE:
            return ("CLOSE");
            break;
        case WAP_PROTO_CLOSE_OK:
            return ("CLOSE_OK");
            break;
        case WAP_PROTO_PING:
            return ("PING");
            break;
        case WAP_PROTO_PING_OK:
            return ("PING_OK");
            break;
        case WAP_PROTO_ERROR:
            return ("ERROR");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the identity field

const char *
wap_proto_identity (wap_proto_t *self)
{
    assert (self);
    return self->identity;
}

void
wap_proto_set_identity (wap_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->identity)
        return;
    strncpy (self->identity, value, 255);
    self->identity [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get the block_ids field, without transferring ownership

zlist_t *
wap_proto_block_ids (wap_proto_t *self)
{
    assert (self);
    return self->block_ids;
}

//  Get the block_ids field and transfer ownership to caller

zlist_t *
wap_proto_get_block_ids (wap_proto_t *self)
{
    assert (self);
    zlist_t *block_ids = self->block_ids;
    self->block_ids = NULL;
    return block_ids;
}

//  Set the block_ids field, transferring ownership from caller

void
wap_proto_set_block_ids (wap_proto_t *self, zlist_t **block_ids_p)
{
    assert (self);
    assert (block_ids_p);
    zlist_destroy (&self->block_ids);
    self->block_ids = *block_ids_p;
    *block_ids_p = NULL;
}



//  --------------------------------------------------------------------------
//  Get/set the start_height field

uint64_t
wap_proto_start_height (wap_proto_t *self)
{
    assert (self);
    return self->start_height;
}

void
wap_proto_set_start_height (wap_proto_t *self, uint64_t start_height)
{
    assert (self);
    self->start_height = start_height;
}


//  --------------------------------------------------------------------------
//  Get/set the status field

uint64_t
wap_proto_status (wap_proto_t *self)
{
    assert (self);
    return self->status;
}

void
wap_proto_set_status (wap_proto_t *self, uint64_t status)
{
    assert (self);
    self->status = status;
}


//  --------------------------------------------------------------------------
//  Get/set the curr_height field

uint64_t
wap_proto_curr_height (wap_proto_t *self)
{
    assert (self);
    return self->curr_height;
}

void
wap_proto_set_curr_height (wap_proto_t *self, uint64_t curr_height)
{
    assert (self);
    self->curr_height = curr_height;
}


//  --------------------------------------------------------------------------
//  Get the block_data field without transferring ownership

zmsg_t *
wap_proto_block_data (wap_proto_t *self)
{
    assert (self);
    return self->block_data;
}

//  Get the block_data field and transfer ownership to caller

zmsg_t *
wap_proto_get_block_data (wap_proto_t *self)
{
    zmsg_t *block_data = self->block_data;
    self->block_data = NULL;
    return block_data;
}

//  Set the block_data field, transferring ownership from caller

void
wap_proto_set_block_data (wap_proto_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->block_data);
    self->block_data = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the tx_as_hex field without transferring ownership

zchunk_t *
wap_proto_tx_as_hex (wap_proto_t *self)
{
    assert (self);
    return self->tx_as_hex;
}

//  Get the tx_as_hex field and transfer ownership to caller

zchunk_t *
wap_proto_get_tx_as_hex (wap_proto_t *self)
{
    zchunk_t *tx_as_hex = self->tx_as_hex;
    self->tx_as_hex = NULL;
    return tx_as_hex;
}

//  Set the tx_as_hex field, transferring ownership from caller

void
wap_proto_set_tx_as_hex (wap_proto_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->tx_as_hex);
    self->tx_as_hex = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the tx_id field without transferring ownership

zchunk_t *
wap_proto_tx_id (wap_proto_t *self)
{
    assert (self);
    return self->tx_id;
}

//  Get the tx_id field and transfer ownership to caller

zchunk_t *
wap_proto_get_tx_id (wap_proto_t *self)
{
    zchunk_t *tx_id = self->tx_id;
    self->tx_id = NULL;
    return tx_id;
}

//  Set the tx_id field, transferring ownership from caller

void
wap_proto_set_tx_id (wap_proto_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->tx_id);
    self->tx_id = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the o_indexes field without transferring ownership

zframe_t *
wap_proto_o_indexes (wap_proto_t *self)
{
    assert (self);
    return self->o_indexes;
}

//  Get the o_indexes field and transfer ownership to caller

zframe_t *
wap_proto_get_o_indexes (wap_proto_t *self)
{
    zframe_t *o_indexes = self->o_indexes;
    self->o_indexes = NULL;
    return o_indexes;
}

//  Set the o_indexes field, transferring ownership from caller

void
wap_proto_set_o_indexes (wap_proto_t *self, zframe_t **frame_p)
{
    assert (self);
    assert (frame_p);
    zframe_destroy (&self->o_indexes);
    self->o_indexes = *frame_p;
    *frame_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the outs_count field

uint64_t
wap_proto_outs_count (wap_proto_t *self)
{
    assert (self);
    return self->outs_count;
}

void
wap_proto_set_outs_count (wap_proto_t *self, uint64_t outs_count)
{
    assert (self);
    self->outs_count = outs_count;
}


//  --------------------------------------------------------------------------
//  Get the amounts field without transferring ownership

zframe_t *
wap_proto_amounts (wap_proto_t *self)
{
    assert (self);
    return self->amounts;
}

//  Get the amounts field and transfer ownership to caller

zframe_t *
wap_proto_get_amounts (wap_proto_t *self)
{
    zframe_t *amounts = self->amounts;
    self->amounts = NULL;
    return amounts;
}

//  Set the amounts field, transferring ownership from caller

void
wap_proto_set_amounts (wap_proto_t *self, zframe_t **frame_p)
{
    assert (self);
    assert (frame_p);
    zframe_destroy (&self->amounts);
    self->amounts = *frame_p;
    *frame_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the random_outputs field without transferring ownership

zframe_t *
wap_proto_random_outputs (wap_proto_t *self)
{
    assert (self);
    return self->random_outputs;
}

//  Get the random_outputs field and transfer ownership to caller

zframe_t *
wap_proto_get_random_outputs (wap_proto_t *self)
{
    zframe_t *random_outputs = self->random_outputs;
    self->random_outputs = NULL;
    return random_outputs;
}

//  Set the random_outputs field, transferring ownership from caller

void
wap_proto_set_random_outputs (wap_proto_t *self, zframe_t **frame_p)
{
    assert (self);
    assert (frame_p);
    zframe_destroy (&self->random_outputs);
    self->random_outputs = *frame_p;
    *frame_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get the tx_data field without transferring ownership

zchunk_t *
wap_proto_tx_data (wap_proto_t *self)
{
    assert (self);
    return self->tx_data;
}

//  Get the tx_data field and transfer ownership to caller

zchunk_t *
wap_proto_get_tx_data (wap_proto_t *self)
{
    zchunk_t *tx_data = self->tx_data;
    self->tx_data = NULL;
    return tx_data;
}

//  Set the tx_data field, transferring ownership from caller

void
wap_proto_set_tx_data (wap_proto_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->tx_data);
    self->tx_data = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the address field

const char *
wap_proto_address (wap_proto_t *self)
{
    assert (self);
    return self->address;
}

void
wap_proto_set_address (wap_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->address)
        return;
    strncpy (self->address, value, 255);
    self->address [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the thread_count field

uint64_t
wap_proto_thread_count (wap_proto_t *self)
{
    assert (self);
    return self->thread_count;
}

void
wap_proto_set_thread_count (wap_proto_t *self, uint64_t thread_count)
{
    assert (self);
    self->thread_count = thread_count;
}


//  --------------------------------------------------------------------------
//  Get/set the reason field

const char *
wap_proto_reason (wap_proto_t *self)
{
    assert (self);
    return self->reason;
}

void
wap_proto_set_reason (wap_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->reason)
        return;
    strncpy (self->reason, value, 255);
    self->reason [255] = 0;
}



//  --------------------------------------------------------------------------
//  Selftest

int
wap_proto_test (bool verbose)
{
    printf (" * wap_proto:");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    wap_proto_t *self = wap_proto_new ();
    assert (self);
    wap_proto_destroy (&self);

    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-wap_proto");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-wap_proto");
    assert (rc == 0);

    //  Encode/send/decode and verify each message type
    int instance;
    self = wap_proto_new ();
    wap_proto_set_id (self, WAP_PROTO_OPEN);

    wap_proto_set_identity (self, "Life is short but Now lasts for ever");
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (streq (wap_proto_identity (self), "Life is short but Now lasts for ever"));
    }
    wap_proto_set_id (self, WAP_PROTO_OPEN_OK);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_BLOCKS);

    zlist_t *blocks_block_ids = zlist_new ();
    zlist_append (blocks_block_ids, "Name: Brutus");
    zlist_append (blocks_block_ids, "Age: 43");
    wap_proto_set_block_ids (self, &blocks_block_ids);
    wap_proto_set_start_height (self, 123);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        zlist_t *block_ids = wap_proto_get_block_ids (self);
        assert (zlist_size (block_ids) == 2);
        assert (streq ((char *) zlist_first (block_ids), "Name: Brutus"));
        assert (streq ((char *) zlist_next (block_ids), "Age: 43"));
        zlist_destroy (&block_ids);
        zlist_destroy (&blocks_block_ids);
        assert (wap_proto_start_height (self) == 123);
    }
    wap_proto_set_id (self, WAP_PROTO_BLOCKS_OK);

    wap_proto_set_status (self, 123);
    wap_proto_set_start_height (self, 123);
    wap_proto_set_curr_height (self, 123);
    zmsg_t *blocks_ok_block_data = zmsg_new ();
    wap_proto_set_block_data (self, &blocks_ok_block_data);
    zmsg_addstr (wap_proto_block_data (self), "Captcha Diem");
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
        assert (wap_proto_start_height (self) == 123);
        assert (wap_proto_curr_height (self) == 123);
        assert (zmsg_size (wap_proto_block_data (self)) == 1);
        char *content = zmsg_popstr (wap_proto_block_data (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        zmsg_destroy (&blocks_ok_block_data);
    }
    wap_proto_set_id (self, WAP_PROTO_PUT);

    zchunk_t *put_tx_as_hex = zchunk_new ("Captcha Diem", 12);
    wap_proto_set_tx_as_hex (self, &put_tx_as_hex);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (memcmp (zchunk_data (wap_proto_tx_as_hex (self)), "Captcha Diem", 12) == 0);
        zchunk_destroy (&put_tx_as_hex);
    }
    wap_proto_set_id (self, WAP_PROTO_PUT_OK);

    wap_proto_set_status (self, 123);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
    }
    wap_proto_set_id (self, WAP_PROTO_OUTPUT_INDEXES);

    zchunk_t *output_indexes_tx_id = zchunk_new ("Captcha Diem", 12);
    wap_proto_set_tx_id (self, &output_indexes_tx_id);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (memcmp (zchunk_data (wap_proto_tx_id (self)), "Captcha Diem", 12) == 0);
        zchunk_destroy (&output_indexes_tx_id);
    }
    wap_proto_set_id (self, WAP_PROTO_OUTPUT_INDEXES_OK);

    wap_proto_set_status (self, 123);
    zframe_t *output_indexes_ok_o_indexes = zframe_new ("Captcha Diem", 12);
    wap_proto_set_o_indexes (self, &output_indexes_ok_o_indexes);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
        assert (zframe_streq (wap_proto_o_indexes (self), "Captcha Diem"));
        zframe_destroy (&output_indexes_ok_o_indexes);
    }
    wap_proto_set_id (self, WAP_PROTO_RANDOM_OUTS);

    wap_proto_set_outs_count (self, 123);
    zframe_t *random_outs_amounts = zframe_new ("Captcha Diem", 12);
    wap_proto_set_amounts (self, &random_outs_amounts);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_outs_count (self) == 123);
        assert (zframe_streq (wap_proto_amounts (self), "Captcha Diem"));
        zframe_destroy (&random_outs_amounts);
    }
    wap_proto_set_id (self, WAP_PROTO_RANDOM_OUTS_OK);

    wap_proto_set_status (self, 123);
    zframe_t *random_outs_ok_random_outputs = zframe_new ("Captcha Diem", 12);
    wap_proto_set_random_outputs (self, &random_outs_ok_random_outputs);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
        assert (zframe_streq (wap_proto_random_outputs (self), "Captcha Diem"));
        zframe_destroy (&random_outs_ok_random_outputs);
    }
    wap_proto_set_id (self, WAP_PROTO_GET);

    zchunk_t *get_tx_id = zchunk_new ("Captcha Diem", 12);
    wap_proto_set_tx_id (self, &get_tx_id);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (memcmp (zchunk_data (wap_proto_tx_id (self)), "Captcha Diem", 12) == 0);
        zchunk_destroy (&get_tx_id);
    }
    wap_proto_set_id (self, WAP_PROTO_GET_OK);

    zchunk_t *get_ok_tx_data = zchunk_new ("Captcha Diem", 12);
    wap_proto_set_tx_data (self, &get_ok_tx_data);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (memcmp (zchunk_data (wap_proto_tx_data (self)), "Captcha Diem", 12) == 0);
        zchunk_destroy (&get_ok_tx_data);
    }
    wap_proto_set_id (self, WAP_PROTO_SAVE);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_SAVE_OK);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_START);

    wap_proto_set_address (self, "Life is short but Now lasts for ever");
    wap_proto_set_thread_count (self, 123);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (streq (wap_proto_address (self), "Life is short but Now lasts for ever"));
        assert (wap_proto_thread_count (self) == 123);
    }
    wap_proto_set_id (self, WAP_PROTO_START_OK);

    wap_proto_set_status (self, 123);
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
    }
    wap_proto_set_id (self, WAP_PROTO_STOP);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_STOP_OK);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_CLOSE);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_CLOSE_OK);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_PING);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_PING_OK);

    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
    }
    wap_proto_set_id (self, WAP_PROTO_ERROR);

    wap_proto_set_status (self, 123);
    wap_proto_set_reason (self, "Life is short but Now lasts for ever");
    //  Send twice
    wap_proto_send (self, output);
    wap_proto_send (self, output);

    for (instance = 0; instance < 2; instance++) {
        wap_proto_recv (self, input);
        assert (wap_proto_routing_id (self));
        assert (wap_proto_status (self) == 123);
        assert (streq (wap_proto_reason (self), "Life is short but Now lasts for ever"));
    }

    wap_proto_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
