/*  =========================================================================
    zee_client_engine - ZeroMQ Echo Example Client engine

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


//  ---------------------------------------------------------------------------
//  State machine constants

typedef enum {
    start_state = 1,
    connected_state = 2,
    expect_later_state = 3,
    defaults_state = 4
} state_t;

typedef enum {
    NULL_event = 0,
    connect_event = 1,
    bad_endpoint_event = 2,
    destructor_event = 3,
    yodel_event = 4,
    echo_event = 5,
    later_event = 6,
    expired_event = 7,
    pong_event = 8,
    error_event = 9,
    exception_event = 10
} event_t;

//  Names for state machine logging and error reporting
static const char *
s_state_name [] = {
    "(NONE)",
    "start",
    "connected",
    "expect later",
    "defaults"
};

static const char *
s_event_name [] = {
    "(NONE)",
    "connect",
    "bad_endpoint",
    "destructor",
    "YODEL",
    "ECHO",
    "LATER",
    "expired",
    "PONG",
    "ERROR",
    "exception"
};


//  ---------------------------------------------------------------------------
//  Context for the client. This embeds the application-level client context
//  at its start (the entire structure, not a reference), so we can cast a
//  pointer between client_t and s_client_t arbitrarily.

//  These are the different method arguments we manage automatically
struct _client_args_t {
    char *endpoint;
    uint32_t timeout;
    char *payload;
};

typedef struct {
    client_t client;            //  Application-level client context
    zsock_t *cmdpipe;           //  Get/send commands from caller API
    zsock_t *msgpipe;           //  Get/send messages from caller API
    zsock_t *dealer;            //  Socket to talk to server
    zloop_t *loop;              //  Listen to pipe and dealer
    zee_proto_t *message;       //  Message received or sent
    client_args_t args;         //  Method arguments structure
    bool verbose;               //  True if we're tracing state machine
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
    signal_connected (client_t *self);
static void
    use_connect_timeout (client_t *self);
static void
    signal_bad_endpoint (client_t *self);
static void
    signal_success (client_t *self);
static void
    signal_echo (client_t *self);
static void
    check_if_connection_is_dead (client_t *self);
static void
    client_is_connected (client_t *self);
static void
    check_status_code (client_t *self);
static void
    signal_internal_error (client_t *self);

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
        snprintf (self->log_prefix, sizeof (self->log_prefix),
            "%6d:%-33s", randof (1000000), "zee_client");
        self->dealer = zsock_new (ZMQ_DEALER);
        if (self->dealer)
            self->message = zee_proto_new ();
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
        zstr_free (&self->args.payload);
        client_terminate (&self->client);
        zee_proto_destroy (&self->message);
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
//  to receive an "expired" event if there is no incoming traffic for that many
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
s_protocol_event (s_client_t *self, zee_proto_t *message)
{
    assert (message);
    switch (zee_proto_id (message)) {
        case ZEE_PROTO_YODEL:
            return yodel_event;
            break;
        case ZEE_PROTO_ECHO:
            return echo_event;
            break;
        case ZEE_PROTO_PONG:
            return pong_event;
            break;
        case ZEE_PROTO_LATER:
            return later_event;
            break;
        case ZEE_PROTO_ERROR:
            return error_event;
            break;
        default:
            zsys_error ("%s: unknown command %s, halting",
                        self->log_prefix, zee_proto_command (message));
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
        if (self->verbose) {
            zsys_debug ("%s: %s:",
                self->log_prefix, s_state_name [self->state]);
            zsys_debug ("%s:     %s",
                self->log_prefix, s_event_name [self->event]);
        }
        switch (self->state) {
            case start_state:
                if (self->event == connect_event) {
                    if (!self->exception) {
                        //  signal connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal connected", self->log_prefix);
                        signal_connected (&self->client);
                    }
                    if (!self->exception) {
                        //  use connect timeout
                        if (self->verbose)
                            zsys_debug ("%s:         $ use connect timeout", self->log_prefix);
                        use_connect_timeout (&self->client);
                    }
                    if (!self->exception)
                        self->state = connected_state;
                }
                else
                if (self->event == bad_endpoint_event) {
                    if (!self->exception) {
                        //  signal bad endpoint
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal bad endpoint", self->log_prefix);
                        signal_bad_endpoint (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
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

            case connected_state:
                if (self->event == yodel_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  send YODEL
                        if (self->verbose)
                            zsys_debug ("%s:         $ send YODEL",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_YODEL);
                        zee_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == echo_event) {
                    if (!self->exception) {
                        //  signal echo
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal echo", self->log_prefix);
                        signal_echo (&self->client);
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send GOODBYE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send GOODBYE",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_GOODBYE);
                        zee_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_later_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  check if connection is dead
                        if (self->verbose)
                            zsys_debug ("%s:         $ check if connection is dead", self->log_prefix);
                        check_if_connection_is_dead (&self->client);
                    }
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_PING);
                        zee_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  signal internal error
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal internal error", self->log_prefix);
                        signal_internal_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                break;

            case expect_later_state:
                if (self->event == later_event) {
                    if (!self->exception) {
                        //  signal success
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal success", self->log_prefix);
                        signal_success (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send GOODBYE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send GOODBYE",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_GOODBYE);
                        zee_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_later_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  check if connection is dead
                        if (self->verbose)
                            zsys_debug ("%s:         $ check if connection is dead", self->log_prefix);
                        check_if_connection_is_dead (&self->client);
                    }
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_PING);
                        zee_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                }
                break;

            case defaults_state:
                if (self->event == destructor_event) {
                    if (!self->exception) {
                        //  send GOODBYE
                        if (self->verbose)
                            zsys_debug ("%s:         $ send GOODBYE",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_GOODBYE);
                        zee_proto_send (self->message, self->dealer);
                    }
                    if (!self->exception)
                        self->state = expect_later_state;
                }
                else
                if (self->event == expired_event) {
                    if (!self->exception) {
                        //  check if connection is dead
                        if (self->verbose)
                            zsys_debug ("%s:         $ check if connection is dead", self->log_prefix);
                        check_if_connection_is_dead (&self->client);
                    }
                    if (!self->exception) {
                        //  send PING
                        if (self->verbose)
                            zsys_debug ("%s:         $ send PING",
                                self->log_prefix);
                        zee_proto_set_id (self->message, ZEE_PROTO_PING);
                        zee_proto_send (self->message, self->dealer);
                    }
                }
                else
                if (self->event == pong_event) {
                    if (!self->exception) {
                        //  client is connected
                        if (self->verbose)
                            zsys_debug ("%s:         $ client is connected", self->log_prefix);
                        client_is_connected (&self->client);
                    }
                }
                else
                if (self->event == error_event) {
                    if (!self->exception) {
                        //  check status code
                        if (self->verbose)
                            zsys_debug ("%s:         $ check status code", self->log_prefix);
                        check_status_code (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                else
                if (self->event == exception_event) {
                }
                else {
                    //  Handle unexpected protocol events
                    if (!self->exception) {
                        //  signal internal error
                        if (self->verbose)
                            zsys_debug ("%s:         $ signal internal error", self->log_prefix);
                        signal_internal_error (&self->client);
                    }
                    if (!self->exception) {
                        //  terminate
                        if (self->verbose)
                            zsys_debug ("%s:         $ terminate", self->log_prefix);
                        self->fsm_stopped = true;
                    }
                }
                break;
        }
        //  If we had an exception event, interrupt normal programming
        if (self->exception) {
            if (self->verbose)
                zsys_debug ("%s:         ! %s",
                    self->log_prefix, s_event_name [self->exception]);
            self->next_event = self->exception;
        }
        else
        if (self->verbose)
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
    if (self->verbose)
        zsys_debug ("%s:     API command=%s", self->log_prefix, method);

    if (streq (method, "$TERM"))
        self->terminated = true;    //  Shutdown the engine
    else
    if (streq (method, "$CONNECTED"))
        zsock_send (self->cmdpipe, "i", self->connected);
    else
    if (streq (method, "SET VERBOSE"))
        zsock_recv (self->cmdpipe, "i", &self->verbose);
    else
    if (streq (method, "CONNECT")) {
        zstr_free (&self->args.endpoint);
        zsock_recv (self->cmdpipe, "s4", &self->args.endpoint, &self->args.timeout);
        s_client_execute (self, connect_event);
    }
    else
    if (streq (method, "DESTRUCTOR")) {
        s_client_execute (self, destructor_event);
    }
    else
    if (streq (method, "YODEL")) {
        zstr_free (&self->args.payload);
        zsock_recv (self->cmdpipe, "s", &self->args.payload);
        s_client_execute (self, yodel_event);
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
        if (self->verbose)
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
        if (zee_proto_recv (self->message, self->dealer))
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
zee_client (zsock_t *cmdpipe, void *msgpipe)
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

struct _zee_client_t {
    zactor_t *actor;            //  Client actor
    zsock_t *msgpipe;           //  Pipe for async message flow
    bool connected;             //  Client currently connected or not
    int status;                 //  Returned by actor reply
    char *reason;               //  Returned by actor reply
};


//  ---------------------------------------------------------------------------
//  Create a new zee_client

zee_client_t *
zee_client_new (void)
{
    zee_client_t *self = (zee_client_t *) zmalloc (sizeof (zee_client_t));
    if (self) {
        zsock_t *backend;
        self->msgpipe = zsys_create_pipe (&backend);
        if (self->msgpipe)
            self->actor = zactor_new (zee_client, backend);
        if (!self->actor)
            zee_client_destroy (&self);
    }
    return self;
}


//  ---------------------------------------------------------------------------
//  Destroy the zee_client
//  No explanation

static int
zee_client_destructor (zee_client_t *self);

void
zee_client_destroy (zee_client_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zee_client_t *self = *self_p;
        if (self->actor && !zsys_interrupted) {
            //  Before destroying the actor we have to flush any pending
            //  traffic on the msgpipe, otherwise it gets lost in a fire and
            //  forget scenario. We do this by sending $FLUSH to the msgpipe
            //  and waiting for a signal back on the cmdpipe.
            if (zstr_send (self->msgpipe, "$FLUSH") == 0)
                zsock_wait (self->actor);
            zee_client_destructor (self);
        }
        zactor_destroy (&self->actor);
        zsock_destroy (&self->msgpipe);
        zstr_free (&self->reason);
        free (self);
        *self_p = NULL;
    }
}


//  ---------------------------------------------------------------------------
//  Return actor, when caller wants to work with multiple actors and/or
//  input sockets asynchronously.

zactor_t *
zee_client_actor (zee_client_t *self)
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
zee_client_msgpipe (zee_client_t *self)
{
    assert (self);
    return self->msgpipe;
}


//  ---------------------------------------------------------------------------
//  Return true if client is currently connected, else false. Note that the
//  client will automatically re-connect if the server dies and restarts after
//  a successful first connection.

bool
zee_client_connected (zee_client_t *self)
{
    assert (self);
    int connected;
    zsock_send (self->actor, "s", "$CONNECTED");
    zsock_recv (self->actor, "i", &connected);
    return connected == 1;
}


//  ---------------------------------------------------------------------------
//  Get valid reply from actor; discard replies that does not match. Current
//  implementation filters on first frame of message. Blocks until a valid
//  reply is received, and properties can be loaded from it. Returns 0 if
//  matched, -1 if interrupted.

static int
s_accept_reply (zee_client_t *self, ...)
{
    assert (self);
    while (!zsys_interrupted) {
        char *reply = zstr_recv (self->actor);
        if (!reply)
            break;              //  Interrupted

        va_list args;
        va_start (args, self);
        char *filter = va_arg (args, char *);
        while (filter) {
            if (streq (reply, filter)) {
                if (streq (reply, "CONNECTED")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "SUCCESS")) {
                    zsock_recv (self->actor, "i", &self->status);
                }
                else
                if (streq (reply, "FAILURE")) {
                    zstr_free (&self->reason);
                    zsock_recv (self->actor, "is", &self->status, &self->reason);
                }
                break;
            }
            filter = va_arg (args, char *);
        }
        va_end (args);
        //  If anything was remaining on pipe, flush it
        zsock_flush (self->actor);
        zstr_free (&reply);
        if (filter) {
            return 0;           //  We matched one of the filters
        }
    }
    return -1;          //  Interrupted
}


//  ---------------------------------------------------------------------------
//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.

int
zee_client_connect (zee_client_t *self, const char *endpoint, uint32_t timeout)
{
    assert (self);

    zsock_send (self->actor, "ss4", "CONNECT", endpoint, timeout);
    if (s_accept_reply (self, "CONNECTED", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.

int
zee_client_destructor (zee_client_t *self)
{
    assert (self);

    zsock_send (self->actor, "s", "DESTRUCTOR");
    if (s_accept_reply (self, "SUCCESS", "FAILURE", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  No explanation
//  Returns >= 0 if successful, -1 if interrupted.

int
zee_client_yodel (zee_client_t *self, const char *payload)
{
    assert (self);

    zsock_send (self->actor, "ss", "YODEL", payload);
    if (s_accept_reply (self, "SUCCESS", NULL))
        return -1;              //  Interrupted or timed-out
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received status

int
zee_client_status (zee_client_t *self)
{
    assert (self);
    return self->status;
}


//  ---------------------------------------------------------------------------
//  Return last received reason

const char *
zee_client_reason (zee_client_t *self)
{
    assert (self);
    return self->reason;
}


//  ---------------------------------------------------------------------------
//  Return last received reason and transfer ownership to caller

const char *
zee_client_get_reason (zee_client_t *self)
{
    assert (self);
    char * reason = self->reason;
    self->reason = NULL;
    return reason;
}


//  ---------------------------------------------------------------------------
//  Enable verbose tracing (animation) of state machine activity.

void
zee_client_set_verbose (zee_client_t *self, bool verbose)
{
    assert (self);
    zsock_send (self->actor, "si", "SET VERBOSE", verbose);
}
