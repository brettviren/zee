/*  =========================================================================
    zee_client - ZeroMQ Echo Example Client

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zee_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    =========================================================================
*/

#ifndef ZEE_CLIENT_H_INCLUDED
#define ZEE_CLIENT_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef ZEE_CLIENT_T_DEFINED
typedef struct _zee_client_t zee_client_t;
#define ZEE_CLIENT_T_DEFINED
#endif

//  @interface
//  Create a new zee_client, return the reference if successful, or NULL
//  if construction failed due to lack of available memory.
zee_client_t *
    zee_client_new (void);

//  Destroy the zee_client and free all memory used by the object.
void
    zee_client_destroy (zee_client_t **self_p);

//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.
zactor_t *
    zee_client_actor (zee_client_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
zsock_t *
    zee_client_msgpipe (zee_client_t *self);

//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.
bool
    zee_client_connected (zee_client_t *self);

//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.
int
    zee_client_connect (zee_client_t *self, const char *endpoint, uint32_t timeout);

//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.
int
    zee_client_yodel (zee_client_t *self, const char *payload);

//  Return last received status
int
    zee_client_status (zee_client_t *self);

//  Return last received reason
const char *
    zee_client_reason (zee_client_t *self);

//  Return last received reason and transfer ownership to caller
const char *
    zee_client_get_reason (zee_client_t *self);

//  Enable verbose tracing (animation) of state machine activity.
void
    zee_client_set_verbose (zee_client_t *self, bool verbose);

//  Self test of this class
void
    zee_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
