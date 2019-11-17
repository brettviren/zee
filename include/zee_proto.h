/*  =========================================================================
    zee_proto - The ZeroMQ Echo Example Protocol

    Codec header for zee_proto.

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

#ifndef ZEE_PROTO_H_INCLUDED
#define ZEE_PROTO_H_INCLUDED

/*  These are the zee_proto messages:

    YODEL - Shout into the void.
        payload             string

    ECHO - The void shouts into you.
        payload             string

    PING -

    PONG -

    GOODBYE -

    LATER -

    ERROR -
        status              number 2    3-digit status code
        reason              string      Printable explanation
*/


#define ZEE_PROTO_YODEL                     1
#define ZEE_PROTO_ECHO                      2
#define ZEE_PROTO_PING                      3
#define ZEE_PROTO_PONG                      4
#define ZEE_PROTO_GOODBYE                   5
#define ZEE_PROTO_LATER                     6
#define ZEE_PROTO_ERROR                     7

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef ZEE_PROTO_T_DEFINED
typedef struct _zee_proto_t zee_proto_t;
#define ZEE_PROTO_T_DEFINED
#endif

//  @interface
//  Create a new empty zee_proto
zee_proto_t *
    zee_proto_new (void);

//  Create a new zee_proto from zpl/zconfig_t *
zee_proto_t *
    zee_proto_new_zpl (zconfig_t *config);

//  Destroy a zee_proto instance
void
    zee_proto_destroy (zee_proto_t **self_p);

//  Create a deep copy of a zee_proto instance
zee_proto_t *
    zee_proto_dup (zee_proto_t *other);

//  Receive a zee_proto from the socket. Returns 0 if OK, -1 if
//  the read was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.
int
    zee_proto_recv (zee_proto_t *self, zsock_t *input);

//  Send the zee_proto to the output socket, does not destroy it
int
    zee_proto_send (zee_proto_t *self, zsock_t *output);



//  Print contents of message to stdout
void
    zee_proto_print (zee_proto_t *self);

//  Export class as zconfig_t*. Caller is responsibe for destroying the instance
zconfig_t *
    zee_proto_zpl (zee_proto_t *self, zconfig_t* parent);

//  Get/set the message routing id
zframe_t *
    zee_proto_routing_id (zee_proto_t *self);
void
    zee_proto_set_routing_id (zee_proto_t *self, zframe_t *routing_id);

//  Get the zee_proto id and printable command
int
    zee_proto_id (zee_proto_t *self);
void
    zee_proto_set_id (zee_proto_t *self, int id);
const char *
    zee_proto_command (zee_proto_t *self);

//  Get/set the payload field
const char *
    zee_proto_payload (zee_proto_t *self);
void
    zee_proto_set_payload (zee_proto_t *self, const char *value);

//  Get/set the status field
uint16_t
    zee_proto_status (zee_proto_t *self);
void
    zee_proto_set_status (zee_proto_t *self, uint16_t status);

//  Get/set the reason field
const char *
    zee_proto_reason (zee_proto_t *self);
void
    zee_proto_set_reason (zee_proto_t *self, const char *value);

//  Self test of this class
void
    zee_proto_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define zee_proto_dump      zee_proto_print

#ifdef __cplusplus
}
#endif

#endif
