/*  =========================================================================
    zee_server - ZeroMQ Echo Example Server

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: zee_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
      LGPL 3+
    =========================================================================
*/

#ifndef ZEE_SERVER_H_INCLUDED
#define ZEE_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with zee_server, use the CZMQ zactor API:
//
//  Create new zee_server instance, passing logging prefix:
//
//      zactor_t *zee_server = zactor_new (zee_server, "myname");
//
//  Destroy zee_server instance
//
//      zactor_destroy (&zee_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (zee_server, "VERBOSE");
//
//  Bind zee_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to acquire an ephemeral port:
//
//      zstr_sendx (zee_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (zee_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (zee_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (zee_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (zee_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (zee_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to zee_server:
//
//      zactor_send (zee_server, &msg);
//
//  Receive zmsg_t instance from zee_server:
//
//      zmsg_t *msg = zactor_recv (zee_server);
//
//  This is the zee_server constructor as a zactor_fn:
//
void
    zee_server (zsock_t *pipe, void *args);

//  Self test of this class
void
    zee_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
