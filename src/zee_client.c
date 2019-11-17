/*  =========================================================================
    zee_client - ZeroMQ Echo Example Client

    =========================================================================
*/

/*
@header
    Description of class for man page.
@discuss
    Detailed discussion of the class, if any.
@end
*/

#include "zee_classes.h"
//  TODO: Change these to match your project's needs
#include "../include/zee_proto.h"
#include "../include/zee_client.h"

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
    zee_proto_t *message;       //  Message to/from server
    client_args_t *args;        //  Arguments from methods

    //  TODO: Add specific properties for your application
} client_t;

//  Include the generated client engine
#include "zee_client_engine.inc"

//  Allocate properties and structures for a new client instance.
//  Return 0 if OK, -1 if failed

static int
client_initialize (client_t *self)
{
    return 0;
}

//  Free properties and structures for a client instance

static void
client_terminate (client_t *self)
{
    //  Destroy properties here
}


//  ---------------------------------------------------------------------------
//  Selftest

void
zee_client_test (bool verbose)
{
    zsys_init();
    printf (" * zee_client: ");
    if (verbose)
        printf ("\n");

    zactor_t* server = zactor_new(zee_server, "zee-server");
    if (verbose)
        zstr_send(server, "VERBOSE");
    zstr_sendx(server, "BIND", "ipc://@/zee_server", NULL);

    //  @selftest
    // TODO: fill this out
    zee_client_t *client = zee_client_new ();
    assert(client);
    zee_client_set_verbose(client, verbose);

    int rc = zee_client_connect(client, "ipc://@/zee_server", 500);
    assert(rc == 0);

    rc = zee_client_yodel(client, "Seek and you shall know");
    assert (rc == 0);

    zsock_t* msgpipe = zee_client_msgpipe(client);
    char* payload=0;
    zsock_recv(msgpipe, "ss", NULL, &payload);
    assert(payload);
    if (verbose)
        zsys_debug("\n\n\t%s\n", payload);

    zee_client_destroy (&client);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}


//  ---------------------------------------------------------------------------
//  set_client_identity
//

static void
set_client_identity (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  use_connect_timeout
//

static void
use_connect_timeout (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  signal_bad_endpoint
//

static void
signal_bad_endpoint (client_t *self)
{
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
//  signal_echo
//

static void
signal_echo (client_t *self)
{
    zsock_send (self->msgpipe, "ss", "ECHO",
                zee_proto_payload(self->message));
}


//  ---------------------------------------------------------------------------
//  check_if_connection_is_dead
//

static void
check_if_connection_is_dead (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  client_is_connected
//

static void
client_is_connected (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  check_status_code
//

static void
check_status_code (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  signal_internal_error
//

static void
signal_internal_error (client_t *self)
{
}


//  ---------------------------------------------------------------------------
//  signal_connected
//

static void
signal_connected (client_t *self)
{
    zsock_send (self->cmdpipe, "si", "CONNECTED", 0);
}


//  ---------------------------------------------------------------------------
//  set_payload
//

static void
set_payload (client_t *self)
{
    zee_proto_set_payload(self->message, self->args->payload);
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
