/*  =========================================================================
    zee_proto - The ZeroMQ Echo Example Protocol

    Codec class for zee_proto.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zee_proto.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
      LGPL 3+
    =========================================================================
*/

/*
@header
    zee_proto - The ZeroMQ Echo Example Protocol
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "../include/zee_proto.h"

//  Structure of our class

struct _zee_proto_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  zee_proto message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char payload [256];                 //  payload
    uint16_t status;                    //  3-digit status code
    char reason [256];                  //  Printable explanation
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
        zsys_warning ("zee_proto: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
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
        zsys_warning ("zee_proto: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("zee_proto: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("zee_proto: GET_NUMBER4 failed"); \
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
        zsys_warning ("zee_proto: GET_NUMBER8 failed"); \
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
        zsys_warning ("zee_proto: GET_STRING failed"); \
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
        zsys_warning ("zee_proto: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  --------------------------------------------------------------------------
//  bytes cstring conversion macros

// create new array of unsigned char from properly encoded string
// len of resulting array is strlen (str) / 2
// caller is responsibe for freeing up the memory
#define BYTES_FROM_STR(dst, _str) { \
    char *str = (char*) (_str); \
    if (!str || (strlen (str) % 2) != 0) \
        return NULL; \
\
    size_t strlen_2 = strlen (str) / 2; \
    byte *mem = (byte*) zmalloc (strlen_2); \
    size_t i; \
\
    for (i = 0; i != strlen_2; i++) \
    { \
        char buff[3] = {0x0, 0x0, 0x0}; \
        strncpy (buff, str, 2); \
        unsigned int uint; \
        sscanf (buff, "%x", &uint); \
        assert (uint <= 0xff); \
        mem [i] = (byte) (0xff & uint); \
        str += 2; \
    } \
    dst = mem; \
}

// convert len bytes to hex string
// caller is responsibe for freeing up the memory
#define STR_FROM_BYTES(dst, _mem, _len) { \
    byte *mem = (byte*) (_mem); \
    size_t len = (size_t) (_len); \
    char* ret = (char*) zmalloc (2*len + 1); \
    char* aux = ret; \
    size_t i; \
    for (i = 0; i != len; i++) \
    { \
        sprintf (aux, "%02x", mem [i]); \
        aux+=2; \
    } \
    dst = ret; \
}

//  --------------------------------------------------------------------------
//  Create a new zee_proto

zee_proto_t *
zee_proto_new (void)
{
    zee_proto_t *self = (zee_proto_t *) zmalloc (sizeof (zee_proto_t));
    return self;
}

//  --------------------------------------------------------------------------
//  Create a new zee_proto from zpl/zconfig_t *

zee_proto_t *
    zee_proto_new_zpl (zconfig_t *config)
{
    assert (config);
    zee_proto_t *self = NULL;
    char *message = zconfig_get (config, "message", NULL);
    if (!message) {
        zsys_error ("Can't find 'message' section");
        return NULL;
    }

    if (streq ("ZEE_PROTO_YODEL", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_YODEL);
    }
    else
    if (streq ("ZEE_PROTO_ECHO", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_ECHO);
    }
    else
    if (streq ("ZEE_PROTO_PING", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_PING);
    }
    else
    if (streq ("ZEE_PROTO_PONG", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_PONG);
    }
    else
    if (streq ("ZEE_PROTO_GOODBYE", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_GOODBYE);
    }
    else
    if (streq ("ZEE_PROTO_LATER", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_LATER);
    }
    else
    if (streq ("ZEE_PROTO_ERROR", message)) {
        self = zee_proto_new ();
        zee_proto_set_id (self, ZEE_PROTO_ERROR);
    }
    else
       {
        zsys_error ("message=%s is not known", message);
        return NULL;
       }

    char *s = zconfig_get (config, "routing_id", NULL);
    if (s) {
        byte *bvalue;
        BYTES_FROM_STR (bvalue, s);
        if (!bvalue) {
            zee_proto_destroy (&self);
            return NULL;
        }
        zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
        free (bvalue);
        self->routing_id = frame;
    }


    zconfig_t *content = NULL;
    switch (self->id) {
        case ZEE_PROTO_YODEL:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                zee_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "payload", NULL);
            if (!s) {
                zee_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->payload, s, 255);
            }
            break;
        case ZEE_PROTO_ECHO:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                zee_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "payload", NULL);
            if (!s) {
                zee_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->payload, s, 255);
            }
            break;
        case ZEE_PROTO_PING:
            break;
        case ZEE_PROTO_PONG:
            break;
        case ZEE_PROTO_GOODBYE:
            break;
        case ZEE_PROTO_LATER:
            break;
        case ZEE_PROTO_ERROR:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                zee_proto_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status", NULL);
            if (!s) {
                zsys_error ("content/status not found");
                zee_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status: %s is not a number", s);
                zee_proto_destroy (&self);
                return NULL;
            }
            self->status = uvalue;
            }
            {
            char *s = zconfig_get (content, "reason", NULL);
            if (!s) {
                zee_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->reason, s, 255);
            }
            break;
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zee_proto

void
zee_proto_destroy (zee_proto_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zee_proto_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a zee_proto instance

zee_proto_t *
zee_proto_dup (zee_proto_t *other)
{
    assert (other);
    zee_proto_t *copy = zee_proto_new ();

    // Copy the routing and message id
    zee_proto_set_routing_id (copy, zee_proto_routing_id (other));
    zee_proto_set_id (copy, zee_proto_id (other));


    // Copy the rest of the fields
    zee_proto_set_payload (copy, zee_proto_payload (other));
    zee_proto_set_status (copy, zee_proto_status (other));
    zee_proto_set_reason (copy, zee_proto_reason (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a zee_proto from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
zee_proto_recv (zee_proto_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("zee_proto: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("zee_proto: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);


    //  Get and check protocol signature
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0)) {
        zsys_warning ("zee_proto: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case ZEE_PROTO_YODEL:
            GET_STRING (self->payload);
            break;

        case ZEE_PROTO_ECHO:
            GET_STRING (self->payload);
            break;

        case ZEE_PROTO_PING:
            break;

        case ZEE_PROTO_PONG:
            break;

        case ZEE_PROTO_GOODBYE:
            break;

        case ZEE_PROTO_LATER:
            break;

        case ZEE_PROTO_ERROR:
            GET_NUMBER2 (self->status);
            GET_STRING (self->reason);
            break;

        default:
            zsys_warning ("zee_proto: bad message ID");
            rc = -2;            //  Malformed
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return rc;

    //  Error returns
    malformed:
        zmq_msg_close (&frame);
        return rc;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the zee_proto to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
zee_proto_send (zee_proto_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID

    switch (self->id) {
        case ZEE_PROTO_YODEL:
            frame_size += 1 + strlen (self->payload);
            break;
        case ZEE_PROTO_ECHO:
            frame_size += 1 + strlen (self->payload);
            break;
        case ZEE_PROTO_ERROR:
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
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case ZEE_PROTO_YODEL:
            PUT_STRING (self->payload);
            break;

        case ZEE_PROTO_ECHO:
            PUT_STRING (self->payload);
            break;

        case ZEE_PROTO_ERROR:
            PUT_NUMBER2 (self->status);
            PUT_STRING (self->reason);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
zee_proto_print (zee_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case ZEE_PROTO_YODEL:
            zsys_debug ("ZEE_PROTO_YODEL:");
            zsys_debug ("    payload='%s'", self->payload);
            break;

        case ZEE_PROTO_ECHO:
            zsys_debug ("ZEE_PROTO_ECHO:");
            zsys_debug ("    payload='%s'", self->payload);
            break;

        case ZEE_PROTO_PING:
            zsys_debug ("ZEE_PROTO_PING:");
            break;

        case ZEE_PROTO_PONG:
            zsys_debug ("ZEE_PROTO_PONG:");
            break;

        case ZEE_PROTO_GOODBYE:
            zsys_debug ("ZEE_PROTO_GOODBYE:");
            break;

        case ZEE_PROTO_LATER:
            zsys_debug ("ZEE_PROTO_LATER:");
            break;

        case ZEE_PROTO_ERROR:
            zsys_debug ("ZEE_PROTO_ERROR:");
            zsys_debug ("    status=%ld", (long) self->status);
            zsys_debug ("    reason='%s'", self->reason);
            break;

    }
}

//  --------------------------------------------------------------------------
//  Export class as zconfig_t*. Caller is responsibe for destroying the instance

zconfig_t *
zee_proto_zpl (zee_proto_t *self, zconfig_t *parent)
{
    assert (self);

    zconfig_t *root = zconfig_new ("zee_proto", parent);

    switch (self->id) {
        case ZEE_PROTO_YODEL:
        {
            zconfig_put (root, "message", "ZEE_PROTO_YODEL");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "payload", "%s", self->payload);
            break;
            }
        case ZEE_PROTO_ECHO:
        {
            zconfig_put (root, "message", "ZEE_PROTO_ECHO");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "payload", "%s", self->payload);
            break;
            }
        case ZEE_PROTO_PING:
        {
            zconfig_put (root, "message", "ZEE_PROTO_PING");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case ZEE_PROTO_PONG:
        {
            zconfig_put (root, "message", "ZEE_PROTO_PONG");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case ZEE_PROTO_GOODBYE:
        {
            zconfig_put (root, "message", "ZEE_PROTO_GOODBYE");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case ZEE_PROTO_LATER:
        {
            zconfig_put (root, "message", "ZEE_PROTO_LATER");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            break;
            }
        case ZEE_PROTO_ERROR:
        {
            zconfig_put (root, "message", "ZEE_PROTO_ERROR");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }


            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "status", "%ld", (long) self->status);
            zconfig_putf (config, "reason", "%s", self->reason);
            break;
            }
    }
    return root;
}

//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
zee_proto_routing_id (zee_proto_t *self)
{
    assert (self);
    return self->routing_id;
}

void
zee_proto_set_routing_id (zee_proto_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the zee_proto id

int
zee_proto_id (zee_proto_t *self)
{
    assert (self);
    return self->id;
}

void
zee_proto_set_id (zee_proto_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
zee_proto_command (zee_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case ZEE_PROTO_YODEL:
            return ("YODEL");
            break;
        case ZEE_PROTO_ECHO:
            return ("ECHO");
            break;
        case ZEE_PROTO_PING:
            return ("PING");
            break;
        case ZEE_PROTO_PONG:
            return ("PONG");
            break;
        case ZEE_PROTO_GOODBYE:
            return ("GOODBYE");
            break;
        case ZEE_PROTO_LATER:
            return ("LATER");
            break;
        case ZEE_PROTO_ERROR:
            return ("ERROR");
            break;
    }
    return "?";
}


//  --------------------------------------------------------------------------
//  Get/set the payload field

const char *
zee_proto_payload (zee_proto_t *self)
{
    assert (self);
    return self->payload;
}

void
zee_proto_set_payload (zee_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->payload)
        return;
    strncpy (self->payload, value, 255);
    self->payload [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the status field

uint16_t
zee_proto_status (zee_proto_t *self)
{
    assert (self);
    return self->status;
}

void
zee_proto_set_status (zee_proto_t *self, uint16_t status)
{
    assert (self);
    self->status = status;
}


//  --------------------------------------------------------------------------
//  Get/set the reason field

const char *
zee_proto_reason (zee_proto_t *self)
{
    assert (self);
    return self->reason;
}

void
zee_proto_set_reason (zee_proto_t *self, const char *value)
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

void
zee_proto_test (bool verbose)
{
    printf (" * zee_proto: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    zconfig_t *config;
    zee_proto_t *self = zee_proto_new ();
    assert (self);
    zee_proto_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-zee_proto");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-zee_proto");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = zee_proto_new ();
    zee_proto_set_id (self, ZEE_PROTO_YODEL);
    zee_proto_set_payload (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        assert (streq (zee_proto_payload (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_ECHO);
    zee_proto_set_payload (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        assert (streq (zee_proto_payload (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_PING);
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_PONG);
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_GOODBYE);
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_LATER);
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }
    zee_proto_set_id (self, ZEE_PROTO_ERROR);
    zee_proto_set_status (self, 123);
    zee_proto_set_reason (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = zee_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    zee_proto_send (self, output);
    zee_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        zee_proto_t *self_temp = self;
        if (instance < 2)
            zee_proto_recv (self, input);
        else {
            self = zee_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (zee_proto_routing_id (self));
        assert (zee_proto_status (self) == 123);
        assert (streq (zee_proto_reason (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            zee_proto_destroy (&self);
            self = self_temp;
        }
    }

    zee_proto_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
#if defined (__WINDOWS__)
    zsys_shutdown();
#endif
    //  @end

    printf ("OK\n");
}
