/*  =========================================================================
    wap_server - Wallet Server

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: wap_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
                                                                
    (insert license text here)                                  
    =========================================================================
*/

#ifndef WAP_SERVER_H_INCLUDED
#define WAP_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with wap_server, use the CZMQ zactor API:
//
//  Create new wap_server instance, passing logging prefix:
//
//      zactor_t *wap_server = zactor_new (wap_server, "myname");
//
//  Destroy wap_server instance
//
//      zactor_destroy (&wap_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (wap_server, "VERBOSE");
//
//  Bind wap_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (wap_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (wap_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (wap_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (wap_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (wap_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (wap_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to wap_server:
//
//      zactor_send (wap_server, &msg);
//
//  Receive zmsg_t instance from wap_server:
//
//      zmsg_t *msg = zactor_recv (wap_server);
//
//  This is the wap_server constructor as a zactor_fn:
//
WAP_EXPORT void
    wap_server (zsock_t *pipe, void *args);

//  Self test of this class
WAP_EXPORT void
    wap_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
